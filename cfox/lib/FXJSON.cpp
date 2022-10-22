/********************************************************************************
*                                                                               *
*                      J S O N   R e a d e r  &  W r i t e r                    *
*                                                                               *
*********************************************************************************
* Copyright (C) 2013,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
*********************************************************************************
* This library is free software; you can redistribute it and/or modify          *
* it under the terms of the GNU Lesser General Public License as published by   *
* the Free Software Foundation; either version 3 of the License, or             *
* (at your option) any later version.                                           *
*                                                                               *
* This library is distributed in the hope that it will be useful,               *
* but WITHOUT ANY WARRANTY; without even the implied warranty of                *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                 *
* GNU Lesser General Public License for more details.                           *
*                                                                               *
* You should have received a copy of the GNU Lesser General Public License      *
* along with this program.  If not, see <http://www.gnu.org/licenses/>          *
********************************************************************************/
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "fxchar.h"
#include "fxmath.h"
#include "fxascii.h"
#include "fxunicode.h"
#include "FXElement.h"
#include "FXArray.h"
#include "FXString.h"
#include "FXParseBuffer.h"
#include "FXException.h"
#include "FXVariant.h"
#include "FXVariantArray.h"
#include "FXVariantMap.h"
#include "FXJSON.h"

/*
  Notes:

  - Load and save FXVariant object from/to JSON5 format files.

  - We read over C and C++-style comments; this implementation supports nested
    comments, which is an extension to the JSON5 standard. However, its very
    convenient to comment out blocks of input.

  - Closing the file does not reset comment and line number (in case of error, this
    point to the problem).

  - JSON5 syntax is very simple:

      value     : object | array | string | number | boolean | empty
                ;

      object    : '{' '}'
                | '{' members [ ',' ] '}'
                ;

      members   : pair
                | pair ',' members
                ;

      pair      : name ':' value
                ;

      name      : string
                | ident
                ;

      array     : '[' ']'
                | '[' elements [ ',' ] ']'
                ;

      elements  : value
                | value ',' elements
                ;

      string    : '"' dq-chars '"'
                | '\'' sq-chars '\''
                ;

      dq-chars  : dq-char [ dq-chars ]
                ;

      sq-chars  : sq-char [ sq-chars ]
                ;

      dq-char   : any-character-except-double-quotes
                | '\\"' | '\\\\' | '\\/' | '\\b' | '\\f' | '\\n' | '\\r' | '\\t'
                | '\\x' hxdigit hxdigit
                | '\\u' hxdigit hxdigit hxdigit hxdigit
                | '\\' newline
                ;

      sq-char   : any-character-except-single-quotes
                | '\\\'' | '\\\\' | '\\/' | '\\b' | '\\f' | '\\n' | '\\r' | '\\t'
                | '\\x' hxdigit hxdigit
                | '\\u' hxdigit hxdigit hxdigit hxdigit
                | '\\' newline
                ;

      newline   : '\n'
                | '\r\n'
                | '\r'
                ;

      empty     : 'null'
                ;

      boolean   : 'true'
                | 'false'
                ;

      ident     : alpha
                | alpha alphanums
                ;

      alpha     : 'a'...'z' | 'A'...'Z' | '_' | '$'
                | unicode-categories in { Lu, Ll, Lt, Lm, Lo, Nl }
                ;

      alphanums : alphanum
                | alphanum alphanums
                ;

      alphanum  : 'a'...'z' | 'A'...'Z' | '0'...'9'  | '_' | '$'
                | unicode-categories in { Lu, Ll, Lt, Lm, Lo, Nl, Mn, Mc, Nd, Pc }
                ;

      number    : [ '+' | '-' ] literal
                ;

      literal   : decimal
                | hex
                | ('n' | 'N') ('a' | 'A') ('n' | 'N')
                | ('i' | 'I') ('n' | 'N') ('f' | 'F')
                | ('i' | 'I') ('n' | 'N') ('f' | 'F') ('i' | 'I') ('n' | 'N') ('i' | 'I') ('t' | 'T') ('y' | 'Y')
                ;

      decimal   : digits
                | digits '.' [ digits ] [ exponent ]
                | '.' digits [ exponent ]
                ;

      exponent  : ('e' | 'E') [ '+' | '-' ] digits
                ;

      digits    : digit
                | digit digits
                ;

      hex       : '0' ('x' | 'X') hexdigits
                ;

      hexdigits : hexdigit
                | hexdigit hexdigits
                ;

      digit     : '0'...'9'
                ;

      hexdigit  : '0'...'9' | 'a'...'f' | 'A'...'F'
                ;

  - Our version accepts 'Inf' in lieu of 'Infinity' for infinite float values; also, it
    relaxes capitalization; this is for compatibility with various C-libraries
    implementations, which have minor differences in the way these numbers are printed.

  - Our implementation __vsnprintf() produces either INF or inf for infinity, and NAN
    or nan for not-a-number, while our __vsscanf() implementation accepts any
    capitalization if nan, inf, or infinity.

  - Flow controls the looks of the output.  The current values supported are:

      Stream  No formatting whatsoever.  This is the most compact format but is essentially
              not human-readable.

      Compact Try to cram as much as possible on a line, but break lines beyond
              a certain size.

      Pretty  Nicely indented and pretty printed, but fluffy, output.

  - Escape controls the escaping of unsafe characters when writing data; there are
    three levels:

      0       Don't encode UTF8 multi-byte characters.  This may impair viewing
              the output with editors that don't support UTF8.
      1       Encode UTF8 characters, as well as other unsafe characters, as
              hex escape sequences of the form '\xXX'.
      2       Encode unicode characters as '\uXXXX' or '\uXXXX\uXXXX', the latter
              is used to implement surrogate pairs, i.e. code points exceeding
              16 bits.

    Quotes will be escaped; output will be written to JSON standard; in future,
    we may support JSON5 output as an option.

  - Feature: if FXVariant not initialized to empty, a 2nd call to FXJSON::load() may
    be issued, overwriting only thoses keys present in the original file.  This is
    because we can recurse through the same structure multiple times, as long as the
    structure is the same.

  - Technically, column counts UTF8 characters, not spacing columns.  This may be
    changed in the future.

  - Parsing is done in a way that things will likely work if input is NOT UTF8, as long
    as those character sequences are not actually equal to those few UTF8 sequences
    we care about!

  - When saving, ver=5 forces JSON5 output mode.  In this mode, keys are written w/o
    quotes if possible, and strings may be written either with single quotes (') or
    double quotes (").
*/

using namespace FX;

/*******************************************************************************/

namespace FX {

// Maximum token size
enum {
   MAXTOKEN  = 256
   };

// Tokens
enum {
  TK_ERROR   = 0,       // Syntax error
  TK_EOF     = 1,       // End of file
  TK_COMMA   = 2,       // Element separator
  TK_COLON   = 3,       // Key:value pair separator
  TK_NULL    = 4,       // NULL value
  TK_FALSE   = 5,       // False value
  TK_TRUE    = 6,       // True value
  TK_INT     = 7,       // Integer value (decimal)
  TK_HEX     = 8,       // Integer value (hexadecimal)
  TK_REAL    = 9,       // Real value
  TK_INF     = 10,      // Infinity
  TK_NAN     = 11,      // NaN
  TK_STRING  = 12,      // String
  TK_LBRACK  = 13,      // Start of array
  TK_LBRACE  = 14,      // Start of map
  TK_RBRACK  = 15,      // End of array
  TK_RBRACE  = 16,      // End of map
  TK_IDENT   = 17       // Identifier
  };


// Error messages
const FXchar *const FXJSON::errors[]={
  "OK",
  "Unable to save",
  "Unable to load",
  "Illegal token",
  "Expected a ':'",
  "Expected a ','",
  "Unmatched ']'",
  "Unmatched '}'",
  "Unmatched '\"'",
  "Unmatched '\''",
  "Bad number",
  "Unexpected end of file"
  };


// Special double constants
static const union{ FXulong u; FXdouble f; } inf={FXULONG(0x7ff0000000000000)};
static const union{ FXulong u; FXdouble f; } nan={FXULONG(0x7fffffffffffffff)};


// Construct JSON serializer
FXJSON::FXJSON():offset(0),token(TK_EOF),column(0),indent(0),line(1),wrap(80),quote('"'),flow(Compact),prec(16),fmt(2),esc(0),dent(2),ver(4){
  FXTRACE((100,"FXJSON::FXJSON\n"));
  }


// Construct and open for loading
FXJSON::FXJSON(FXchar* buffer,FXuval sz,Direction d):FXParseBuffer(buffer,sz,d),offset(0),token(TK_EOF),column(0),indent(0),line(1),wrap(80),quote('"'),flow(Compact),prec(16),fmt(2),esc(0),dent(2),ver(4){
  FXTRACE((100,"FXJSON::FXJSON(%p,%lu,%s)\n",buffer,sz,(d==Save)?"Save":(d==Load)?"Load":"Stop"));
  open(buffer,sz,d);
  }


// Open JSON stream for given direction and set its buffer
FXbool FXJSON::open(FXchar* buffer,FXuval sz,Direction d){
  FXTRACE((101,"FXJSON::open(%p,%lu,%s)\n",buffer,sz,(d==Save)?"Save":(d==Load)?"Load":"Stop"));
  if(FXParseBuffer::open(buffer,sz,d)){
    value=FXString::null;
    token=TK_ERROR;
    offset=0;
    column=0;
    line=1;
    indent=0;
    return true;
    }
  return false;
  }

/*******************************************************************************/

// Get next token
FXint FXJSON::next(){
  FXint comment=0;
  FXuchar c,cc;

  // While more data
  while(need(MAXTOKEN)){

    // Start new token
    rptr=sptr;

    FXASSERT(sptr<wptr);

    // Process characters
    c=sptr[0];
    switch(c){
      case '\t':                                        // Tab hops to next tabstop
        column+=(8-column%8);
        offset++;
        sptr++;
        continue;
      case ' ':                                         // Space
        column++;
      case '\v':                                        // Vertical tab
      case '\f':                                        // Form feed
        offset++;
        sptr++;
        continue;
      case '\r':                                        // Carriage return
        if(sptr+1<wptr && sptr[1]=='\n'){
          offset++;
          sptr++;
          }
      case '\n':                                        // Newline
        if(comment<0) comment=0;                        // End single-line comment
        column=0;
        offset++;
        sptr++;
        line++;
        continue;
      case '/':                                         // Start or end of comment
      case '*':
        if(sptr+1<wptr){
          if(sptr[0]=='/' && sptr[1]=='/' && comment==0){
            comment=-1;                                 // Comment until end of line
            column+=2;
            offset+=2;
            sptr+=2;
            continue;
            }
          if(sptr[0]=='/' && sptr[1]=='*' && comment>=0){
            comment+=1;                                 // Increase comment nesting level
            column+=2;
            offset+=2;
            sptr+=2;
            continue;
            }
          if(sptr[0]=='*' && sptr[1]=='/' && comment>=1){
            comment-=1;                                 // Decrease comment nesting level
            column+=2;
            offset+=2;
            sptr+=2;
            continue;
            }
          }
      default:                                          // Normal characters
        if(!comment) return TK_ERROR;
        column++;
        offset++;
        sptr++;
        continue;
      case '{':                                         // Begin of map
        column++;
        offset++;
        sptr++;
        if(comment) continue;
        return TK_LBRACE;
      case '}':                                         // End of map
        column++;
        offset++;
        sptr++;
        if(comment) continue;
        return TK_RBRACE;
      case '[':                                         // Begin of array
        column++;
        offset++;
        sptr++;
        if(comment) continue;
        return TK_LBRACK;
      case ']':                                         // End of array
        column++;
        offset++;
        sptr++;
        if(comment) continue;
        return TK_RBRACK;
      case ',':                                         // Element separator
        column++;
        offset++;
        sptr++;
        if(comment) continue;
        return TK_COMMA;
      case ':':                                         // Key:value separator
        column++;
        offset++;
        sptr++;
        if(comment) continue;
        return TK_COLON;
      case '"':                                         // String double quotes
      case '\'':                                        // String single quote
        if(!comment) return string();
        column++;
        offset++;
        sptr++;
        continue;
      case '+':                                         // Number value
      case '-':
      case '.':
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        if(!comment) return number();
        column++;
        offset++;
        sptr++;
        continue;
      case 'a':                                         // Identifier or reserved word
      case 'b':
      case 'c':
      case 'd':
      case 'e':
      case 'f':
      case 'g':
      case 'h':
      case 'i':
      case 'j':
      case 'k':
      case 'l':
      case 'm':
      case 'n':
      case 'o':
      case 'p':
      case 'q':
      case 'r':
      case 's':
      case 't':
      case 'u':
      case 'v':
      case 'w':
      case 'x':
      case 'y':
      case 'z':
      case 'A':
      case 'B':
      case 'C':
      case 'D':
      case 'E':
      case 'F':
      case 'G':
      case 'H':
      case 'I':
      case 'J':
      case 'K':
      case 'L':
      case 'M':
      case 'N':
      case 'O':
      case 'P':
      case 'Q':
      case 'R':
      case 'S':
      case 'T':
      case 'U':
      case 'V':
      case 'W':
      case 'X':
      case 'Y':
      case 'Z':
      case '_':
      case '$':
        if(!comment) return ident();
        column++;
        offset++;
        sptr++;
        continue;
      case 0x00:                                        // Non-special control characters
      case 0x01:
      case 0x02:
      case 0x03:
      case 0x04:
      case 0x05:
      case 0x06:
      case 0x07:
      case 0x08:
      case 0x0E:
      case 0x0F:
      case 0x10:
      case 0x11:
      case 0x12:
      case 0x13:
      case 0x14:
      case 0x15:
      case 0x16:
      case 0x17:
      case 0x18:
      case 0x19:
      case 0x1A:
      case 0x1B:
      case 0x1C:
      case 0x1D:
      case 0x1E:
      case 0x1F:
      case 0x7F:                                        // DEL
      case 0x80:                                        // UTF8 followers
      case 0x81:
      case 0x82:
      case 0x83:
      case 0x84:
      case 0x85:
      case 0x86:
      case 0x87:
      case 0x88:
      case 0x89:
      case 0x8A:
      case 0x8B:
      case 0x8C:
      case 0x8D:
      case 0x8E:
      case 0x8F:
      case 0x90:
      case 0x91:
      case 0x92:
      case 0x93:
      case 0x94:
      case 0x95:
      case 0x96:
      case 0x97:
      case 0x98:
      case 0x99:
      case 0x9A:
      case 0x9B:
      case 0x9C:
      case 0x9D:
      case 0x9E:
      case 0x9F:
      case 0xA0:
      case 0xA1:
      case 0xA2:
      case 0xA3:
      case 0xA4:
      case 0xA5:
      case 0xA6:
      case 0xA7:
      case 0xA8:
      case 0xA9:
      case 0xAA:
      case 0xAB:
      case 0xAC:
      case 0xAD:
      case 0xAE:
      case 0xAF:
      case 0xB0:
      case 0xB1:
      case 0xB2:
      case 0xB3:
      case 0xB4:
      case 0xB5:
      case 0xB6:
      case 0xB7:
      case 0xB8:
      case 0xB9:
      case 0xBA:
      case 0xBB:
      case 0xBC:
      case 0xBD:
      case 0xBE:
      case 0xBF:
      case 0xF8:                                        // Bad UTF8 leaders
      case 0xF9:
      case 0xFA:
      case 0xFB:
      case 0xFC:
      case 0xFD:
      case 0xFE:
      case 0xFF:
        return TK_ERROR;                                // Bad token
      case 0xC0:                                        // 2-byte UTF8 sequences
      case 0xC1:
      case 0xC2:
      case 0xC3:
      case 0xC4:
      case 0xC5:
      case 0xC6:
      case 0xC7:
      case 0xC8:
      case 0xC9:
      case 0xCA:
      case 0xCB:
      case 0xCC:
      case 0xCD:
      case 0xCE:
      case 0xCF:
      case 0xD0:
      case 0xD1:
      case 0xD2:
      case 0xD3:
      case 0xD4:
      case 0xD5:
      case 0xD6:
      case 0xD7:
      case 0xD8:
      case 0xD9:
      case 0xDA:
      case 0xDB:
      case 0xDC:
      case 0xDD:
      case 0xDE:
      case 0xDF:
        if(sptr+1>=wptr) return TK_EOF;                 // Premature EOF
        if((sptr[1]&192)!=128) return TK_ERROR;         // Bad followers
        if(sptr[0]=='\xC2' && sptr[1]=='\xA0'){
          column++;
          offset+=2;                                    // Non Breakable Space (\xC2\xA0)
          sptr+=2;
          continue;
          }
        if(!comment){
          cc=Unicode::charCategory(wc2(sptr));
          if(cc<CatLetterUpper) return TK_ERROR;
          if(CatNumberLetter<cc) return TK_ERROR;
          return ident();                               // Identifier
          }
        column++;
        offset+=2;
        sptr+=2;
        continue;
      case 0xE0:                                        // 3-byte UTF8 sequences
      case 0xE1:
      case 0xE2:
      case 0xE3:
      case 0xE4:
      case 0xE5:
      case 0xE6:
      case 0xE7:
      case 0xE8:
      case 0xE9:
      case 0xEA:
      case 0xEB:
      case 0xEC:
      case 0xED:
      case 0xEE:
      case 0xEF:
        if(sptr+2>=wptr) return TK_EOF;                 // Premature EOF
        if((sptr[1]&192)!=128) return TK_ERROR;         // Non-follower
        if((sptr[2]&192)!=128) return TK_ERROR;         // Non-follower
        if(sptr[0]=='\xEF' && sptr[1]=='\xBB' && sptr[2]=='\xBF'){
          offset+=3;
          sptr+=3;                                      // Byte order mark
          continue;
          }
        if(sptr[0]=='\xE2' && sptr[1]=='\x80' && (sptr[2]=='\xA8' || sptr[2]=='\xA9')){
          if(comment<0) comment=0;                      // End single-line comment
          column=0;
          offset+=3;
          sptr+=3;                                      // Line Separator or Paragraph Separator
          line++;
          continue;
          }
        if(!comment){
          cc=Unicode::charCategory(wc3(sptr));
          if(cc<CatLetterUpper) return TK_ERROR;
          if(CatNumberLetter<cc) return TK_ERROR;
          return ident();                               // Identifier
          }
        column++;
        offset+=3;
        sptr+=3;
        continue;
      case 0xF0:                                        // 4-byte UTF8 sequences
      case 0xF1:
      case 0xF2:
      case 0xF3:
      case 0xF4:
      case 0xF5:
      case 0xF6:
      case 0xF7:
        if(sptr+3>=wptr) return TK_EOF;                 // Premature EOF
        if((sptr[1]&192)!=128) return TK_ERROR;         // Non-follower
        if((sptr[2]&192)!=128) return TK_ERROR;
        if((sptr[3]&192)!=128) return TK_ERROR;
        if(!comment){
          cc=Unicode::charCategory(wc4(sptr));
          if(cc<CatLetterUpper) return TK_ERROR;
          if(CatNumberLetter<cc) return TK_ERROR;
          return ident();                               // Identifier
          }
        column++;
        offset+=4;
        sptr+=4;
        continue;
      }
    }
  return TK_EOF;
  }


// Number
FXint FXJSON::number(){
  FXint tok=TK_ERROR;
  value=FXString::null;
  if(sptr[0]=='-' || sptr[0]=='+'){                     // Both '+' and '-' sign may appear
    column++;
    offset++;
    sptr++;
    }
  if(sptr+2<wptr && (sptr[0]|0x20)=='n'){               // NaN
    if((sptr[1]|0x20)!='a') return TK_ERROR;
    if((sptr[2]|0x20)!='n') return TK_ERROR;
    column+=3;
    offset+=3;
    sptr+=3;
    return TK_NAN;
    }
  if(sptr+2<wptr && (sptr[0]|0x20)=='i'){               // Inf{inity}
    if((sptr[1]|0x20)!='n') return TK_ERROR;
    if((sptr[2]|0x20)!='f') return TK_ERROR;
    if(sptr+7<wptr && (sptr[3]|0x20)=='i'){
      if((sptr[4]|0x20)!='n') return TK_ERROR;
      if((sptr[5]|0x20)!='i') return TK_ERROR;
      if((sptr[6]|0x20)!='t') return TK_ERROR;
      if((sptr[7]|0x20)!='y') return TK_ERROR;
      column+=5;
      offset+=5;
      sptr+=5;
      }
    column+=3;
    offset+=3;
    sptr+=3;
    return TK_INF;
    }
  if(sptr<wptr && sptr[0]=='0'){                        // Support for hexadecimal as in 0xHHHH
    column++;
    offset++;
    sptr++;
    tok=TK_INT;
    if(sptr+1<wptr && (sptr[0]|0x20)=='x' && Ascii::isHexDigit(sptr[1])){
      column+=2;
      offset+=2;
      sptr+=2;
      while(sptr<wptr && Ascii::isHexDigit(sptr[0])){   // Eat hex digits
        column++;
        offset++;
        sptr++;
        }
      value.assign(rptr,sptr-rptr);
      return TK_HEX;
      }
    }
  while(sptr<wptr && Ascii::isDigit(sptr[0])){          // Eat integer digits
    tok=TK_INT;
    column++;
    offset++;
    sptr++;
    }
  if(sptr<wptr && sptr[0]=='.'){                        // Fraction part
    column++;
    offset++;
    sptr++;
    if(tok==TK_INT) tok=TK_REAL;
    while(sptr<wptr && Ascii::isDigit(sptr[0])){        // Eat fraction digits
      tok=TK_REAL;
      column++;
      offset++;
      sptr++;
      }
    }
  if(tok!=TK_ERROR){
    if(sptr<wptr && (sptr[0]|0x20)=='e'){               // Exponent part
      column++;
      offset++;
      sptr++;
      if(sptr<wptr && (sptr[0]=='-' || sptr[0]=='+')){  // Both '+' and '-' exponent sign may appear
        column++;
        offset++;
        sptr++;
        }
      tok=TK_ERROR;
      while(sptr<wptr && Ascii::isDigit(sptr[0])){      // Eat exponent digits
        tok=TK_REAL;
        column++;
        offset++;
        sptr++;
        }
      }
    if(tok!=TK_ERROR){                                  // Matched a number
      value.assign(rptr,sptr-rptr);
      }
    }
  return tok;
  }


// Identifier
FXint FXJSON::ident(){
  FXint size=0;
  FXuchar c,cc;
  value=FXString::null;
  while(need(MAXTOKEN)){
    FXASSERT(sptr<wptr);
    c=sptr[0];
    switch(c){
      case 'a':                                         // Identifier characters
      case 'b':
      case 'c':
      case 'd':
      case 'e':
      case 'f':
      case 'g':
      case 'h':
      case 'i':
      case 'j':
      case 'k':
      case 'l':
      case 'm':
      case 'n':
      case 'o':
      case 'p':
      case 'q':
      case 'r':
      case 's':
      case 't':
      case 'u':
      case 'v':
      case 'w':
      case 'x':
      case 'y':
      case 'z':
      case 'A':
      case 'B':
      case 'C':
      case 'D':
      case 'E':
      case 'F':
      case 'G':
      case 'H':
      case 'I':
      case 'J':
      case 'K':
      case 'L':
      case 'M':
      case 'N':
      case 'O':
      case 'P':
      case 'Q':
      case 'R':
      case 'S':
      case 'T':
      case 'U':
      case 'V':
      case 'W':
      case 'X':
      case 'Y':
      case 'Z':
      case '_':
      case '$':
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        column++;                                       // Add legal identifier characters
        offset++;
        sptr++;
        size++;
chk:    if(sptr+MAXTOKEN>wptr && wptr==endptr){         // Big token
          value.append(rptr,sptr-rptr);
          rptr=sptr;
          }
        continue;
      case 0xC0:                                        // 2-byte UTF8 sequences
      case 0xC1:
      case 0xC2:
      case 0xC3:
      case 0xC4:
      case 0xC5:
      case 0xC6:
      case 0xC7:
      case 0xC8:
      case 0xC9:
      case 0xCA:
      case 0xCB:
      case 0xCC:
      case 0xCD:
      case 0xCE:
      case 0xCF:
      case 0xD0:
      case 0xD1:
      case 0xD2:
      case 0xD3:
      case 0xD4:
      case 0xD5:
      case 0xD6:
      case 0xD7:
      case 0xD8:
      case 0xD9:
      case 0xDA:
      case 0xDB:
      case 0xDC:
      case 0xDD:
      case 0xDE:
      case 0xDF:
        if(sptr+1>=wptr) return TK_EOF;                 // Premature EOF
        if((sptr[1]&192)!=128) return TK_ERROR;         // Non-follower
        cc=Unicode::charCategory(wc2(sptr));
        if(cc<CatMarkNonSpacing) goto end;
        if(CatPunctConnector<cc) goto end;
        if(CatMarkSpacingCombining<cc && cc<CatLetterUpper) goto end;
        column++;
        offset+=2;
        sptr+=2;
        size+=2;
        goto chk;
      case 0xE0:                                        // 3-byte UTF8 sequences
      case 0xE1:
      case 0xE2:
      case 0xE3:
      case 0xE4:
      case 0xE5:
      case 0xE6:
      case 0xE7:
      case 0xE8:
      case 0xE9:
      case 0xEA:
      case 0xEB:
      case 0xEC:
      case 0xED:
      case 0xEE:
      case 0xEF:
        if(sptr+2>=wptr) return TK_EOF;                 // Premature EOF
        if((sptr[1]&192)!=128) return TK_ERROR;         // Non-follower
        if((sptr[2]&192)!=128) return TK_ERROR;         // Non-follower
        cc=Unicode::charCategory(wc3(sptr));
        if(cc<CatMarkNonSpacing) goto end;
        if(CatPunctConnector<cc) goto end;
        if(CatMarkSpacingCombining<cc && cc<CatLetterUpper) goto end;
        column++;
        offset+=3;
        sptr+=3;
        size+=3;
        goto chk;
      case 0xF0:                                        // 4-byte UTF8 sequences
      case 0xF1:
      case 0xF2:
      case 0xF3:
      case 0xF4:
      case 0xF5:
      case 0xF6:
      case 0xF7:
        if(sptr+3>=wptr) return ErrEnd;                 // Premature EOF
        if((sptr[1]&192)!=128) return TK_ERROR;         // Non-follower
        if((sptr[2]&192)!=128) return TK_ERROR;
        if((sptr[3]&192)!=128) return TK_ERROR;
        cc=Unicode::charCategory(wc4(sptr));
        if(cc<CatMarkNonSpacing) goto end;
        if(CatPunctConnector<cc) goto end;
        if(CatMarkSpacingCombining<cc && cc<CatLetterUpper) goto end;
        column++;
        offset+=4;
        sptr+=4;
        size+=4;
        goto chk;
      default:                                          // End of identifier
        if(rptr[0]=='n' && rptr[1]=='u' && rptr[2]=='l' && rptr[3]=='l' && size==4) return TK_NULL;
        if(rptr[0]=='t' && rptr[1]=='r' && rptr[2]=='u' && rptr[3]=='e' && size==4) return TK_TRUE;
        if(rptr[0]=='f' && rptr[1]=='a' && rptr[2]=='l' && rptr[3]=='s' && rptr[4]=='e' && size==5) return TK_FALSE;
        if((rptr[0]|0x20)=='n' && (rptr[1]|0x20)=='a' && (rptr[2]|0x20)=='n' && size==3) return TK_NAN;
        if((rptr[0]|0x20)=='i' && (rptr[1]|0x20)=='n' && (rptr[2]|0x20)=='f' && size==3) return TK_INF;
        if((rptr[0]|0x20)=='i' && (rptr[1]|0x20)=='n' && (rptr[2]|0x20)=='f' && (rptr[3]|0x20)=='i' && (rptr[4]|0x20)=='n' && (rptr[5]|0x20)=='i' && (rptr[6]|0x20)=='t' && (rptr[7]|0x20)=='y' && size==8) return TK_INF;
end:    value.append(rptr,sptr-rptr);
        rptr=sptr;
        return TK_IDENT;                                // Identifier
      }
    }
  return TK_EOF;
  }


// String
FXint FXJSON::string(){
  FXchar q=rptr[0];
  FXuchar c;
  column++;
  offset++;
  sptr++;
  value=FXString::null;
  while(need(MAXTOKEN)){
    c=sptr[0];
    switch(c){
      case '\t':                                        // Tab hops to next tabstop
        column+=(8-column%8);
        offset++;
        sptr++;
        goto nxt;
      case ' ':                                         // Space
        column++;
      case '\v':                                        // Vertical tab
      case '\f':                                        // Form feed
        offset++;
        sptr++;
        goto nxt;
      case '\r':                                        // Carriage return
      case '\n':                                        // Newline
        return TK_ERROR;
      case '"':                                         // End of string
      case '\'':
        column++;
        offset++;
        sptr++;
        if(q!=c) goto nxt;
        value.append(rptr,sptr-rptr);
        rptr=sptr;
        value=FXString::unescape(value,q,q);            // Of course
        return TK_STRING;
      case 0x00:                                        // Non-special control characters
      case 0x01:
      case 0x02:
      case 0x03:
      case 0x04:
      case 0x05:
      case 0x06:
      case 0x07:
      case 0x08:
      case 0x0E:
      case 0x0F:
      case 0x10:
      case 0x11:
      case 0x12:
      case 0x13:
      case 0x14:
      case 0x15:
      case 0x16:
      case 0x17:
      case 0x18:
      case 0x19:
      case 0x1A:
      case 0x1B:
      case 0x1C:
      case 0x1D:
      case 0x1E:
      case 0x1F:
      case 0x7F:                                        // DEL
      case 0x80:                                        // UTF8 followers
      case 0x81:
      case 0x82:
      case 0x83:
      case 0x84:
      case 0x85:
      case 0x86:
      case 0x87:
      case 0x88:
      case 0x89:
      case 0x8A:
      case 0x8B:
      case 0x8C:
      case 0x8D:
      case 0x8E:
      case 0x8F:
      case 0x90:
      case 0x91:
      case 0x92:
      case 0x93:
      case 0x94:
      case 0x95:
      case 0x96:
      case 0x97:
      case 0x98:
      case 0x99:
      case 0x9A:
      case 0x9B:
      case 0x9C:
      case 0x9D:
      case 0x9E:
      case 0x9F:
      case 0xA0:
      case 0xA1:
      case 0xA2:
      case 0xA3:
      case 0xA4:
      case 0xA5:
      case 0xA6:
      case 0xA7:
      case 0xA8:
      case 0xA9:
      case 0xAA:
      case 0xAB:
      case 0xAC:
      case 0xAD:
      case 0xAE:
      case 0xAF:
      case 0xB0:
      case 0xB1:
      case 0xB2:
      case 0xB3:
      case 0xB4:
      case 0xB5:
      case 0xB6:
      case 0xB7:
      case 0xB8:
      case 0xB9:
      case 0xBA:
      case 0xBB:
      case 0xBC:
      case 0xBD:
      case 0xBE:
      case 0xBF:
      case 0xF8:                                        // Bad UTF8 leaders
      case 0xF9:
      case 0xFA:
      case 0xFB:
      case 0xFC:
      case 0xFD:
      case 0xFE:
      case 0xFF:
        return TK_ERROR;                                // Bad characters
      case 0xC0:                                        // 2-byte UTF8 sequences
      case 0xC1:
      case 0xC2:
      case 0xC3:
      case 0xC4:
      case 0xC5:
      case 0xC6:
      case 0xC7:
      case 0xC8:
      case 0xC9:
      case 0xCA:
      case 0xCB:
      case 0xCC:
      case 0xCD:
      case 0xCE:
      case 0xCF:
      case 0xD0:
      case 0xD1:
      case 0xD2:
      case 0xD3:
      case 0xD4:
      case 0xD5:
      case 0xD6:
      case 0xD7:
      case 0xD8:
      case 0xD9:
      case 0xDA:
      case 0xDB:
      case 0xDC:
      case 0xDD:
      case 0xDE:
      case 0xDF:
        if(sptr+1>=wptr) return TK_EOF;                 // Premature EOF
        if((sptr[1]&192)!=128) return TK_ERROR;         // Bad UTF8 followers
        column++;
        offset+=2;
        sptr+=2;
        goto nxt;
      case 0xE0:                                        // 3-byte UTF8 sequences
      case 0xE1:
      case 0xE2:
      case 0xE3:
      case 0xE4:
      case 0xE5:
      case 0xE6:
      case 0xE7:
      case 0xE8:
      case 0xE9:
      case 0xEA:
      case 0xEB:
      case 0xEC:
      case 0xED:
      case 0xEE:
      case 0xEF:
        if(sptr+2>=wptr) return TK_EOF;                 // Premature EOF
        if((sptr[1]&192)!=128) return TK_ERROR;         // Bad UTF8 followers
        if((sptr[2]&192)!=128) return TK_ERROR;
        if(sptr[0]=='\xE2' && sptr[1]=='\x80' && (sptr[2]=='\xA8' || sptr[2]=='\xA9')){
          column=-1;
          line++;                                       // Line Separator or Paragraph Separator
          }
        column++;
        offset+=3;
        sptr+=3;
        goto nxt;
      case 0xF0:                                        // 4-byte UTF8 sequences
      case 0xF1:
      case 0xF2:
      case 0xF3:
      case 0xF4:
      case 0xF5:
      case 0xF6:
      case 0xF7:
        if(sptr+3>=wptr) return TK_EOF;                 // Premature EOF
        if((sptr[1]&192)!=128) return TK_ERROR;         // Bad UTF8 followers
        if((sptr[2]&192)!=128) return TK_ERROR;
        if((sptr[3]&192)!=128) return TK_ERROR;
        column++;
        offset+=4;
        sptr+=4;
        goto nxt;
      case '\\':                                        // Escape next character
        column++;
        offset++;
        sptr++;
        if(sptr>=wptr) return TK_EOF;                   // Premature EOF
        if(sptr[0]=='\n'){                              // Check line-continuation
          column=0;                                     // Reset to line start
          line++;                                       // Add line
          offset++;
          sptr++;
          goto nxt;
          }
        if(sptr[0]=='\r'){                              // Check line-continuation
          if(sptr+1<wptr && sptr[1]=='\n'){
            offset++;
            sptr++;
            }
          column=0;                                     // Reset to line start
          line++;                                       // Add line
          offset++;
          sptr++;
          goto nxt;
          }
      default:                                          // Advance
        column++;
        offset++;
        sptr++;
nxt:    if(sptr+MAXTOKEN>wptr && wptr==endptr){         // Big token
          value.append(rptr,sptr-rptr);
          rptr=sptr;
          }
        continue;
      }
    }
  return TK_EOF;
  }


// Load map elements int o var
FXJSON::Error FXJSON::loadMap(FXVariant& var){
  FXString key;
  Error err;

  // Make it into an array now
  var.setType(FXVariant::MapType);

  // While more keys
  while(token==TK_STRING || token==TK_IDENT){

    // Load key
    key=value;

    // Token following the string
    token=next();

    // Expect colon
    if(token!=TK_COLON) return ErrColon;

    // Eat the colon
    token=next();

    // Load item directly into associated slot
    if((err=loadVariant(var[key]))!=ErrOK) return err;

    // Expect another key-value pair
    if(token!=TK_COMMA) break;

    // Eat the comma
    token=next();
    }
  return ErrOK;
  }


// Load array elements into var
FXJSON::Error FXJSON::loadArray(FXVariant& var){
  FXival index=0;
  Error err;

  // Make it into an array now
  var.setType(FXVariant::ArrayType);

  // While possible item start token
  while(TK_NULL<=token && token<=TK_LBRACE){

    // Load item directly into array slot
    if((err=loadVariant(var[index]))!=ErrOK) return err;

    // Expect another value
    if(token!=TK_COMMA) break;

    // Next token
    token=next();

    // Next array index
    index++;
    }
  return ErrOK;
  }


// Load variant
FXJSON::Error FXJSON::loadVariant(FXVariant& var){
  Error err;
  switch(token){
  case TK_EOF:                                  // Unexpected end of file
    var=FXVariant::null;
    return ErrEnd;
  case TK_NULL:                                 // Null
    var=FXVariant::null;
    token=next();
    return ErrOK;
  case TK_FALSE:                                // False
    var=false;
    token=next();
    return ErrOK;
  case TK_TRUE:                                 // True
    var=true;
    token=next();
    return ErrOK;
  case TK_INT:
    var=value.toLong(10);
    token=next();
    return ErrOK;
  case TK_HEX:
    var=value.toLong(16);
    token=next();
    return ErrOK;
  case TK_REAL:
    var=value.toDouble();
    token=next();
    return ErrOK;
  case TK_INF:                                  // To infinity...
    var=(rptr[0]=='-')?-inf.f:inf.f;
    token=next();
    return ErrOK;
  case TK_NAN:                                  // ...and beyond!
    var=(rptr[0]=='-')?-nan.f:nan.f;
    token=next();
    return ErrOK;
  case TK_LBRACK:                               // Array
    token=next();
    if((err=loadArray(var))!=ErrOK) return err;
    if(token!=TK_RBRACK) return ErrBracket;     // Expected closing bracket
    token=next();
    return ErrOK;
  case TK_LBRACE:                               // Map
    token=next();
    if((err=loadMap(var))!=ErrOK) return err;
    if(token!=TK_RBRACE) return ErrBrace;       // Expected closing brace
    token=next();
    return ErrOK;
  case TK_STRING:                               // String
    var=value;
    token=next();
    return ErrOK;
  default:                                      // Illegal token
    var=FXVariant::null;
    return ErrToken;
    }
  return ErrToken;
  }


// Load a variant
FXJSON::Error FXJSON::load(FXVariant& variant){
  FXTRACE((101,"FXJSON::load(variant)\n"));
  Error err=ErrLoad;
  if(dir==Load){
    token=next();
    err=loadVariant(variant);
    }
  return err;
  }

/*******************************************************************************/

// Save string after escaping it
FXJSON::Error FXJSON::saveString(const FXString& str){
  FXString string=FXString::escape(str,quote,quote,esc);
  if(!emit(string.text(),string.length())) return ErrSave;
  column+=string.count();
  offset+=string.length();
  return ErrOK;
  }


// Save identifier, no escaping
FXJSON::Error FXJSON::saveIdent(const FXString& str){
  if(!emit(str.text(),str.length())) return ErrSave;
  column+=str.count();
  offset+=str.length();
  return ErrOK;
  }


// Check string has identifier syntax
static FXbool isIdent(const FXString& str){
  const FXchar* p=str.text();
  FXwchar w;
  if((w=wcnxt(p))!=0 && (Unicode::isLetter(w) || w=='_' || w=='$')){
    while((w=wcnxt(p))!=0 && (Unicode::isAlphaNumeric(w) || w=='_' || w=='$')){
      }
    }
  return w==0;
  }


// Save map elements from var
FXJSON::Error FXJSON::saveMap(const FXVariant& var){
  FXival count=var.asMap().used();

  FXASSERT(var.getType()==FXVariant::MapType);

  // Object start
  if(!emit("{",1)) return ErrSave;
  column+=1;
  offset+=1;

  // Skip the whole thing if no items
  if(count){
    FXint oldindent=indent;

    // Figure indent
    indent=(flow==Pretty)?indent+dent:(flow==Compact)?column:0;

    // Write indent
    if(flow==Pretty){
      if(!emit(ENDLINE,strlen(ENDLINE))) return ErrSave;
      column=0;
      offset+=strlen(ENDLINE);
      line++;
      if(!emit(' ',indent)) return ErrSave;
      column+=indent;
      offset+=indent;
      }

    // Loop over the items
    for(FXival i=0; i<var.asMap().no(); ++i){

      // Skip empty slots
      if(var.asMap().key(i).empty()) continue;

      // JSON5 output AND key has identifier syntax
      if((5<=ver) && isIdent(var.asMap().key(i))){
        if(saveIdent(var.asMap().key(i))!=ErrOK) return ErrSave;
        }

      // Original JSON output OR key contains special characters
      else{
        if(saveString(var.asMap().key(i))!=ErrOK) return ErrSave;
        }

      // Write separator
      if(flow==Stream){
        if(!emit(":",1)) return ErrSave;
        column+=1;
        offset+=1;
        }
      else{
        if(!emit(" : ",3)) return ErrSave;
        column+=3;
        offset+=3;
        }

      // Write variant
      if(saveVariant(var.asMap().data(i))!=ErrOK) return ErrSave;

      // Another item to follow?
      if(--count>0){

        // Write comma
        if(!emit(",",1)) return ErrSave;
        column+=1;
        offset+=1;

        // Write newline and indent
        if(flow || wrap<column){
          if(!emit(ENDLINE,strlen(ENDLINE))) return ErrSave;
          column=0;
          offset+=strlen(ENDLINE);
          line++;
          if(!emit(' ',indent)) return ErrSave;
          column+=indent;
          offset+=indent;
          }
        }
      }

    indent=oldindent;

    // Write indent
    if(flow==Pretty){
      if(!emit(ENDLINE,strlen(ENDLINE))) return ErrSave;
      column=0;
      offset+=strlen(ENDLINE);
      line++;
      if(!emit(' ',indent)) return ErrSave;
      column+=indent;
      offset+=indent;
      }
    }

  // Object end
  if(!emit("}",1)) return ErrSave;
  column+=1;
  offset+=1;

  return ErrOK;
  }


// Save array elements from var
FXJSON::Error FXJSON::saveArray(const FXVariant& var){

  FXASSERT(var.getType()==FXVariant::ArrayType);

  // Array start
  if(!emit("[",1)) return ErrSave;
  column+=1;
  offset+=1;

  // Skip the whole thing if no items
  if(var.asArray().no()){
    FXint oldindent=indent;

    // Figure indent
    indent=(flow==Pretty)?indent+dent:(flow==Compact)?column:0;

    // Write indent
    if(flow==Pretty){
      if(!emit(ENDLINE,strlen(ENDLINE))) return ErrSave;
      column=0;
      offset+=strlen(ENDLINE);
      line++;
      if(!emit(' ',indent)) return ErrSave;
      column+=indent;
      offset+=indent;
      }

    // Loop over the items
    for(FXival i=0; i<var.asArray().no(); ++i){

      // Write variant
      if(saveVariant(var.asArray().at(i))!=ErrOK) return ErrSave;

      // Another item to follow?
      if(i+1<var.asArray().no()){

        // Write comma
        if(!emit(",",1)) return ErrSave;
        column+=1;
        offset+=1;

        // Write space or newline and indent
        if(flow==Pretty || wrap<column || (flow==Compact && FXVariant::MapType==var.asArray().at(i).getType())){
          if(!emit(ENDLINE,strlen(ENDLINE))) return ErrSave;
          column=0;
          offset+=strlen(ENDLINE);
          line++;
          if(!emit(' ',indent)) return ErrSave;
          column+=indent;
          offset+=indent;
          }
        else if(flow){
          if(!emit(" ",1)) return ErrSave;
          column+=1;
          offset+=1;
          }
        }
      }

    indent=oldindent;

    // Write indent
    if(flow==Pretty){
      if(!emit(ENDLINE,strlen(ENDLINE))) return ErrSave;
      column=0;
      offset+=strlen(ENDLINE);
      line++;
      if(!emit(' ',indent)) return ErrSave;
      column+=indent;
      offset+=indent;
      }
    }

  // Array end
  if(!emit("]",1)) return ErrSave;
  column+=1;
  offset+=1;
  return ErrOK;
  }


// Recursively save variant var
FXJSON::Error FXJSON::saveVariant(const FXVariant& var){
  FXString string;
  switch(var.getType()){
  case FXVariant::NullType:
    if(!emit("null",4)) return ErrSave;
    column+=4;
    offset+=4;
    break;
  case FXVariant::BoolType:
    if(var.asULong()){
      if(!emit("true",4)) return ErrSave;
      column+=4;
      offset+=4;
      }
    else{
      if(!emit("false",5)) return ErrSave;
      column+=5;
      offset+=5;
      }
    break;
  case FXVariant::CharType:
    string.fromULong(var.asULong());
    if(!emit(string.text(),string.length())) return ErrSave;
    column+=string.length();
    offset+=string.length();
    break;
  case FXVariant::IntType:
  case FXVariant::LongType:
    string.fromLong(var.asLong());
    if(!emit(string.text(),string.length())) return ErrSave;
    column+=string.length();
    offset+=string.length();
    break;
  case FXVariant::UIntType:
  case FXVariant::ULongType:
    string.fromULong(var.asULong());
    if(!emit(string.text(),string.length())) return ErrSave;
    column+=string.length();
    offset+=string.length();
    break;
  case FXVariant::FloatType:
  case FXVariant::DoubleType:
    string.fromDouble(var.asDouble(),prec,fmt);
    if(!emit(string.text(),string.length())) return ErrSave;
    column+=string.length();
    offset+=string.length();
    break;
  case FXVariant::PointerType:
    string.format("%p",var.asPtr());
    if(!emit(string.text(),string.length())) return ErrSave;
    column+=string.length();
    offset+=string.length();
    break;
  case FXVariant::StringType:
    if(saveString(var.asString())!=ErrOK) return ErrSave;
    break;
  case FXVariant::ArrayType:
    if(saveArray(var)!=ErrOK) return ErrSave;
    break;
  case FXVariant::MapType:
    if(saveMap(var)!=ErrOK) return ErrSave;
    break;
    }
  return ErrOK;
  }


// Save a variant
FXJSON::Error FXJSON::save(const FXVariant& variant){
  FXTRACE((101,"FXJSON::save(variant)\n"));
  Error err=ErrSave;
  if(dir==Save){
    err=saveVariant(variant);
    if(flush(0)<0) err=ErrSave;
    }
  return err;
  }

/*******************************************************************************/

// Close json parse buffer
FXbool FXJSON::close(){
  FXTRACE((101,"FXJSON::close()\n"));
  if(FXParseBuffer::close()){
    value=FXString::null;
    return true;
    }
  return false;
  }


// Close stream and clean up
FXJSON::~FXJSON(){
  FXTRACE((100,"FXJSON::~FXJSON\n"));
  close();
  }

}
