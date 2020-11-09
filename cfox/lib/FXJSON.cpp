/********************************************************************************
*                                                                               *
*                      J S O N   R e a d e r  &  W r i t e r                    *
*                                                                               *
*********************************************************************************
* Copyright (C) 2013,2020 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "fxmath.h"
#include "fxascii.h"
#include "fxunicode.h"
#include "FXElement.h"
#include "FXArray.h"
#include "FXString.h"
#include "FXIO.h"
#include "FXIODevice.h"
#include "FXStat.h"
#include "FXFile.h"
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

         value     : object | array | string | number | 'null' | 'true' | 'false'
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
*/


#define MAXTOKEN  256           // Maximum token size

using namespace FX;

/*******************************************************************************/

namespace FX {

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
  TK_QUOTES  = 12,      // Quotes
  TK_QUOTE   = 13,      // Single quote
  TK_LBRACK  = 14,      // Start of array
  TK_LBRACE  = 15,      // Start of map
  TK_RBRACK  = 16,      // End of array
  TK_RBRACE  = 17,      // End of map
  TK_IDENT   = 18       // Identifier
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


// Special IEEE-754 floating-point standard constants
static const union{ FXulong u; FXdouble f; } inf={FXULONG(0x7ff0000000000000)};
static const union{ FXulong u; FXdouble f; } nan={FXULONG(0x7fffffffffffffff)};


// Construct JSON serializer
FXJSON::FXJSON():begptr(NULL),endptr(NULL),wptr(NULL),rptr(NULL),sptr(NULL),offset(0),column(0),indent(0),line(1),dir(Stop),token(TK_EOF),wrap(80),flow(Compact),prec(15),fmt(2),esc(0),dent(2){
  FXTRACE((100,"FXJSON::FXJSON\n"));
  }


// Construct and open for loading
FXJSON::FXJSON(FXchar* buffer,FXuval sz,Direction d):begptr(NULL),endptr(NULL),wptr(NULL),rptr(NULL),sptr(NULL),offset(0),column(0),indent(0),line(1),dir(Stop),token(TK_EOF),wrap(80),flow(Compact),prec(16),fmt(2),esc(0),dent(2){
  FXTRACE((100,"FXJSON::FXJSON(%p,%lu,%s)\n",buffer,sz,(d==Save)?"Save":(d==Load)?"Load":"Stop"));
  open(buffer,sz,d);
  }


// Open JSON stream for given direction and set its buffer
FXbool FXJSON::open(FXchar* buffer,FXuval sz,Direction d){
  FXTRACE((100,"FXJSON::open(%p,%lu,%s)\n",buffer,sz,(d==Save)?"Save":(d==Load)?"Load":"Stop"));
  FXASSERT(dir==Stop);
  if((dir==Stop) && (d!=Stop) && (0<sz) && buffer){
    begptr=buffer;
    endptr=buffer+sz;
    wptr=(d==Load)?endptr:begptr;
    rptr=begptr;
    sptr=begptr;
    token=TK_ERROR;
    offset=0;
    column=0;
    line=1;
    indent=0;
    dir=d;
    return true;
    }
  return false;
  }

/*******************************************************************************/

// Read at least count bytes into buffer; return bytes available, or -1 for error
FXival FXJSON::fill(FXival){
  FXASSERT(dir==Load);
  return wptr-sptr;
  }


// Write at least count bytes from buffer; return space available, or -1 for error
FXival FXJSON::flush(FXival){
  FXASSERT(dir==Save);
  return endptr-wptr;
  }


// Ensure we have a requisite number of bytes in the buffer, calling fill()
// to load additional data into the buffer if needed.
// Near the end of the file, there may be fewer than n bytes in the buffer
// even after fill() is called.
FXbool FXJSON::need(FXival count){
  FXASSERT(dir==Load);
  if(sptr+count>wptr){
    if(wptr==endptr){
      if(fill(count)<0) return false;
      }
    return sptr<wptr;
    }
  return true;
  }


// Emit characters to buffer
FXbool FXJSON::emit(FXchar ch,FXival count){
  FXival num;
  FXASSERT(dir==Save);
  while(0<count){
    if(wptr>=endptr){
      if(flush(count)<=0) return false;
      }
    FXASSERT(wptr<endptr);
    num=FXMIN(count,endptr-wptr);
    fillElms(wptr,ch,num);
    wptr+=num;
    count-=num;
    }
  return true;
  }


// Emit text to buffer
FXbool FXJSON::emit(const FXchar* str,FXival count){
  FXival num;
  FXASSERT(dir==Save);
  while(0<count){
    if(wptr>=endptr){
      if(flush(count)<=0) return false;
      }
    FXASSERT(wptr<endptr);
    num=FXMIN(count,endptr-wptr);
    copyElms(wptr,str,num);
    wptr+=num;
    str+=num;
    count-=num;
    }
  return true;
  }

/*******************************************************************************/

// Get next token
FXint FXJSON::next(){
  FXint comment=0;
  FXint tok;
  FXuchar c,cc;

  FXASSERT(dir==Load);

  // While more data
  while(need(MAXTOKEN)){

    // Start new token
    rptr=sptr;

    FXASSERT(sptr<wptr);

    // Process characters
    c=sptr[0];
    switch(c){
      case '\t':                                // Tab hops to next tabstop
        column+=(8-column%8);
        offset++;
        sptr++;
        continue;
      case ' ':                                 // Space
        column++;
      case '\v':                                // Vertical tab
      case '\f':                                // Form feed
        offset++;
        sptr++;
        continue;
      case '\r':                                // Carriage return
        if(sptr+1<wptr && sptr[1]=='\n'){
          offset++;
          sptr++;
          }
      case '\n':                                // Newline
        if(comment<0) comment=0;                // End single-line comment
        column=0;
        offset++;
        sptr++;
        line++;
        continue;
      case '/':                                 // Possible start of comment
        if(sptr+1<wptr && sptr[1]=='*' && comment>=0){
          column+=2;
          offset+=2;
          sptr+=2;
          comment+=1;                           // Increase comment nesting level
          continue;
          }
        if(sptr+1<wptr && sptr[1]=='/' && comment==0){
          column+=2;
          offset+=2;
          sptr+=2;
          comment=-1;                           // Comment till end of line
          continue;
          }
        if(!comment) return TK_ERROR;           // Illegal character outside of comment or string
        column++;
        offset++;
        sptr++;                                 // Comment
        continue;
      case '*':                                 // Possible end of comment
        if(sptr+1<wptr && sptr[1]=='/' && comment>=1){
          comment-=1;                           // Decrease comment nesting level
          column+=2;
          offset+=2;
          sptr+=2;
          continue;
          }
        if(!comment) return TK_ERROR;           // Illegal character outside of comment or string
        column++;
        offset++;
        sptr++;                                 // Comment
        continue;
      case '{':                                 // Begin of map
        column++;
        offset++;
        sptr++;
        if(!comment) return TK_LBRACE;
        continue;
      case '}':                                 // End of map
        column++;
        offset++;
        sptr++;
        if(!comment) return TK_RBRACE;
        continue;
      case '[':                                 // Begin of array
        column++;
        offset++;
        sptr++;
        if(!comment) return TK_LBRACK;
        continue;
      case ']':                                 // End of array
        column++;
        offset++;
        sptr++;
        if(!comment) return TK_RBRACK;
        continue;
      case ',':                                 // Element separator
        column++;
        offset++;
        sptr++;
        if(!comment) return TK_COMMA;
        continue;
      case ':':                                 // Key:value separator
        column++;
        offset++;
        sptr++;
        if(!comment) return TK_COLON;
        continue;
      case '"':                                 // String double quotes
        column++;
        offset++;
        sptr++;
        if(!comment) return TK_QUOTES;
        continue;
      case '\'':                                // String single quote
        column++;
        offset++;
        sptr++;
        if(!comment) return TK_QUOTE;
        continue;
      case '+':                                 // Number value
      case '-':
      case '.':                                 // Leading period is allowed
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
        if(!comment){
          tok=TK_ERROR;
          if(sptr[0]=='-' || sptr[0]=='+'){                     // Both '+' and '-' sign may appear
            column++;
            offset++;
            sptr++;
            }
          if(sptr+2<wptr && (sptr[0]|0x20)=='i' && (sptr[1]|0x20)=='n' && (sptr[2]|0x20)=='f'){
            column+=3;
            offset+=3;
            sptr+=3;
            if(sptr+4<wptr && (sptr[0]|0x20)=='i' && (sptr[1]|0x20)=='n' && (sptr[2]|0x20)=='i' && (sptr[3]|0x20)=='t' && (sptr[4]|0x20)=='y'){
              column+=5;
              offset+=5;
              sptr+=5;
              }
            return TK_INF;
            }
          if(sptr+2<wptr && (sptr[0]|0x20)=='n' && (sptr[1]|0x20)=='a' && (sptr[2]|0x20)=='n'){
            column+=3;
            offset+=3;
            sptr+=3;
            return TK_NAN;
            }
          if(sptr<wptr && sptr[0]=='0'){                        // Support for hexadecimal as in 0xHHHHH
            column++;
            offset++;
            sptr++;
            tok=TK_INT;
            if(sptr+1<wptr && (sptr[0]=='x' || sptr[0]=='X') && Ascii::isHexDigit(sptr[1])){
              column+=2;
              offset+=2;
              sptr+=2;
              while(sptr<wptr && Ascii::isHexDigit(sptr[0])){   // Eat hex digits
                column++;
                offset++;
                sptr++;
                }
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
            }
          return tok;
          }
        column++;
        offset++;
        sptr++;                                 // Comment
        continue;
      case 'a':
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
        if(!comment){
          if(sptr+3<wptr && sptr[0]=='n' && sptr[1]=='u' && sptr[2]=='l' && sptr[3]=='l'){
            column+=4;
            offset+=4;
            sptr+=4;
            return TK_NULL;                     // Null
            }
          if(sptr+3<wptr && sptr[0]=='t' && sptr[1]=='r' && sptr[2]=='u' && sptr[3]=='e'){
            column+=4;
            offset+=4;
            sptr+=4;
            return TK_TRUE;                     // True
            }
          if(sptr+4<wptr && sptr[0]=='f' && sptr[1]=='a' && sptr[2]=='l' && sptr[3]=='s' && sptr[4]=='e'){
            column+=5;
            offset+=5;
            sptr+=5;
            return TK_FALSE;                    // False
            }
          if(sptr+2<wptr && (sptr[0]|0x20)=='i' && (sptr[1]|0x20)=='n' && (sptr[2]|0x20)=='f'){
            column+=3;
            offset+=3;
            sptr+=3;
            if(sptr+4<wptr && (sptr[0]|0x20)=='i' && (sptr[1]|0x20)=='n' && (sptr[2]|0x20)=='i' && (sptr[3]|0x20)=='t' && (sptr[4]|0x20)=='y'){
              column+=5;
              offset+=5;
              sptr+=5;
              }
            return TK_INF;                      // Infinity
            }
          if(sptr+2<wptr && (sptr[0]|0x20)=='n' && (sptr[1]|0x20)=='a' && (sptr[2]|0x20)=='n'){
            column+=3;
            offset+=3;
            sptr+=3;
            return TK_NAN;                      // NaN
            }
          column++;
          offset++;
          sptr++;
          return TK_IDENT;                      // Identifier
          }
        column++;
        offset++;
        sptr++;                                 // Comment
        continue;
      case 0x00:                                // Non-special control characters
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
      case 0x7F:                                // DEL
      case 0x80:                                // UTF8 followers
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
        return ErrToken;                        // Bad token
      case 0xC0:                                // 2-byte UTF8 sequences
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
        if(sptr+1>=wptr) return TK_EOF;         // Premature EOF
        if(sptr[0]=='\xC2' && sptr[1]=='\xA0'){
          column++;
          offset+=2;
          sptr+=2;                              // Non Breakable Space
          continue;
          }
        if(!comment){
          cc=Unicode::charCategory(wc(sptr));
          if(!(CatLetterUpper<=cc && cc<=CatNumberLetter)) return TK_ERROR;
          column++;
          offset+=2;
          sptr+=2;
          return TK_IDENT;                      // Identifier
          }
        column++;
        offset+=2;
        sptr+=2;                                // Comment
        continue;
      case 0xE0:                                // 3-byte UTF8 sequences
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
        if(sptr+2>=wptr) return TK_EOF;         // Premature EOF
        if(sptr[0]=='\xEF' && sptr[1]=='\xBB' && sptr[2]=='\xBF'){
          offset+=3;
          sptr+=3;                              // Byte order mark
          continue;
          }
        if(sptr[0]=='\xE2' && sptr[1]=='\x80' && (sptr[2]=='\xA8' || sptr[2]=='\xA9')){
          if(comment<0) comment=0;              // End single-line comment
          column=0;
          offset+=3;
          sptr+=3;                              // Line Separator or Paragraph Separator
          line++;
          continue;
          }
        if(!comment){
          cc=Unicode::charCategory(wc(sptr));
          if(!(CatLetterUpper<=cc && cc<=CatNumberLetter)) return TK_ERROR;
          column++;
          offset+=3;
          sptr+=3;
          return TK_IDENT;                      // Identifier
          }
        column++;
        offset+=3;
        sptr+=3;                                // Comment
        continue;
      case 0xF0:                                // 4-byte UTF8 sequences
      case 0xF1:
      case 0xF2:
      case 0xF3:
      case 0xF4:
      case 0xF5:
      case 0xF6:
      case 0xF7:
        if(sptr+3>=wptr) return TK_EOF;         // Premature EOF
        if(!comment){
          cc=Unicode::charCategory(wc(sptr));
          if(!(CatLetterUpper<=cc && cc<=CatNumberLetter)) return TK_ERROR;
          column++;
          offset+=4;
          sptr+=4;
          return TK_IDENT;                      // Identifier
          }
        column++;
        offset+=4;
        sptr+=4;
        continue;                               // Comment
      case 0xF8:                                // Bad UTF8 leaders
      case 0xF9:
      case 0xFA:
      case 0xFB:
      case 0xFC:
      case 0xFD:
      case 0xFF:
        return ErrToken;                        // Bad token
      default:                                  // Normal characters
        if(!comment) return TK_ERROR;           // Illegal character outside of comment or string
        column++;
        offset++;
        sptr++;                                 // Comment
        continue;
      }
    }
  return TK_EOF;
  }


// Load characters into string
// Parse single- or double-quoted string, and after matching the terminating
// quote, eat the final quote, leaving sptr at the start of the next token.
// We keep track of columns even inside the string, so error reporting remains
// useful for sensible input.
FXJSON::Error FXJSON::loadString(FXString& str){
  FXuchar quote=rptr[0];
  FXString string;
  FXuchar c;

  // Nothing yet
  str=FXString::null;

  // Process string
  while(need(MAXTOKEN)){

    // Allows for strings to exceed buffer length
    if((wptr==endptr) && (wptr<=sptr+MAXTOKEN)){
      string.append(rptr,sptr-rptr);
      rptr=sptr;
      }

    FXASSERT(sptr<wptr);

    // Process characters
    c=sptr[0];
    switch(c){
      case '\t':                                // Tab hops to next tabstop
        column+=(8-column%8);
        offset++;
        sptr++;
        continue;
      case ' ':                                 // Space
        column++;
      case '\v':                                // Vertical tab
      case '\f':                                // Form feed
        offset++;
        sptr++;
        continue;
      case '\r':                                // Carriage return
      case '\n':                                // Newline
        return ErrQuotes;                       // Missing quotes
      case '"':                                 // End of string
      case '\'':
        column++;
        offset++;
        sptr++;
        if(quote!=c) continue;
        string.append(rptr,sptr-rptr);
        rptr=sptr;
        str=FXString::unescape(string,quote,quote);       // Of course
        return ErrOK;
      case 0x00:                                // Non-special control characters
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
      case 0x7F:                                // DEL
      case 0x80:                                // UTF8 followers
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
        return ErrToken;                        // Bad token
      case 0xC0:                                // 2-byte UTF8 sequences
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
        if(sptr+1>=wptr) return ErrEnd;         // Premature EOF
        column++;
        offset+=2;
        sptr+=2;
        continue;
      case 0xE0:                                // 3-byte UTF8 sequences
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
        if(sptr+2>=wptr) return ErrEnd;         // Premature EOF
        if(sptr[0]=='\xE2' && sptr[1]=='\x80' && (sptr[2]=='\xA8' || sptr[2]=='\xA9')){
          column=0;
          offset+=3;
          sptr+=3;                              // Line Separator or Paragraph Separator
          line++;
          continue;
          }
        column++;
        offset+=3;
        sptr+=3;
        continue;
      case 0xF0:                                // 4-byte UTF8 sequences
      case 0xF1:
      case 0xF2:
      case 0xF3:
      case 0xF4:
      case 0xF5:
      case 0xF6:
      case 0xF7:
        if(sptr+3>=wptr) return ErrEnd;         // Premature EOF
        column++;
        offset+=4;
        sptr+=4;
        continue;
      case 0xF8:                                // Bad UTF8 leaders
      case 0xF9:
      case 0xFA:
      case 0xFB:
      case 0xFC:
      case 0xFD:
      case 0xFF:
        return ErrToken;                        // Bad token
      case '\\':                                // Escape next character
        column++;
        offset++;
        sptr++;
        if(sptr>=wptr) return ErrEnd;           // Premature EOF
        if(sptr[0]=='\r' || sptr[0]=='\n'){     // Check line-continuation
          if(sptr[0]=='\r' && sptr+1<wptr && sptr[1]=='\n'){
            offset++;
            sptr++;
            }
          column=0;                             // Reset to line start
          line++;                               // Add line
          offset++;
          sptr++;
          continue;
          }
        column++;                               // Advance over escaped character
        offset++;
        sptr++;
        continue;
      default:                                  // Normal characters
        column++;
        offset++;
        sptr++;
        continue;
      }
    }
  return ErrEnd;
  }


// Load identifier
FXJSON::Error FXJSON::loadIdent(FXString& str){
  FXuchar c,cc;

  // Empty
  str=FXString::null;

  // Process identifier
  while(need(MAXTOKEN)){

    // Allows for strings to exceed buffer length
    if((wptr==endptr) && (wptr<=sptr+MAXTOKEN)){
      str.append(rptr,sptr-rptr);
      rptr=sptr;
      }

    FXASSERT(sptr<wptr);

    // Process characters
    c=sptr[0];
    switch(c){
      case 'a':                                 // Identifier characters
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
        column++;
        offset++;
        sptr++;
        continue;
      case 0xC0:                                // 2-byte UTF8 sequences
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
        if(sptr+1>=wptr) return ErrEnd;         // Premature EOF
        cc=Unicode::charCategory(wc(sptr));
        if(!(CatLetterUpper<=cc && cc<=CatPunctConnector) && !(CatMarkNonSpacing<=cc && cc<=CatMarkSpacingCombining)) goto eot;
        column++;
        offset+=2;
        sptr+=2;
        continue;
      case 0xE0:                                // 3-byte UTF8 sequences
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
        if(sptr+2>=wptr) return ErrEnd;         // Premature EOF
        cc=Unicode::charCategory(wc(sptr));
        if(!(CatLetterUpper<=cc && cc<=CatPunctConnector) && !(CatMarkNonSpacing<=cc && cc<=CatMarkSpacingCombining)) goto eot;
        column++;
        offset+=3;
        sptr+=3;
        continue;
      case 0xF0:                                // 4-byte UTF8 sequences
      case 0xF1:
      case 0xF2:
      case 0xF3:
      case 0xF4:
      case 0xF5:
      case 0xF6:
      case 0xF7:
        if(sptr+3>=wptr) return ErrEnd;         // Premature EOF
        cc=Unicode::charCategory(wc(sptr));
        if(!(CatLetterUpper<=cc && cc<=CatPunctConnector) && !(CatMarkNonSpacing<=cc && cc<=CatMarkSpacingCombining)) goto eot;
        column++;
        offset+=4;
        sptr+=4;
        continue;
      default:                                  // Not part of identifier
eot:    str.append(rptr,sptr-rptr);
        rptr=sptr;
        return ErrOK;
      }
    }
  return ErrEnd;
  }


// Load map elements int o var
FXJSON::Error FXJSON::loadMap(FXVariant& var){
  FXString key;
  Error err;

  // Make it into an array now
  var.setType(FXVariant::VMap);

  // While more keys
  while(token==TK_QUOTES || token==TK_QUOTE || token==TK_IDENT){

    // Load key
    if(token==TK_IDENT){
      if((err=loadIdent(key))!=ErrOK) return err;
      }
    else{
      if((err=loadString(key))!=ErrOK) return err;
      }

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
  var.setType(FXVariant::VArray);

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
  FXString value(rptr,sptr-rptr);
  FXbool ok=false;
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
    var=value.toLong(10,&ok);
    if(!ok) return ErrNumber;                   // Numeric conversion error
    token=next();
    return ErrOK;
  case TK_HEX:
    var=value.toLong(16,&ok);
    if(!ok) return ErrNumber;                   // Numeric conversion error
    token=next();
    return ErrOK;
  case TK_REAL:
    var=value.toDouble(&ok);
    if(!ok) return ErrNumber;                   // Numeric conversion error
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
  case TK_QUOTES:                               // Double quoted string
    if((err=loadString(value))!=ErrOK) return err;
    token=next();
    var=value;
    return ErrOK;
  case TK_QUOTE:                                // Single quoted string
    if((err=loadString(value))!=ErrOK) return err;
    token=next();
    var=value;
    return ErrOK;
  default:                                      // Illegal token
    var=FXVariant::null;
    return ErrToken;
    }
  return ErrToken;
  }


// Load a variant
FXJSON::Error FXJSON::load(FXVariant& variant){
  FXTRACE((100,"FXJSON::load(variant)\n"));
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
  FXString string=FXString::escape(str,'"','"',esc);
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


// Save map elements from var
FXJSON::Error FXJSON::saveMap(const FXVariant& var){
  FXival count=var.asMap().used();

  FXASSERT(var.getType()==FXVariant::VMap);

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

      // Save string
      if(saveString(var.asMap().key(i))!=ErrOK) return ErrSave;

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

  FXASSERT(var.getType()==FXVariant::VArray);

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
        if(flow==Pretty || wrap<column || (flow==Compact && FXVariant::VMap==var.asArray().at(i).getType())){
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
  case FXVariant::VNull:
    if(!emit("null",4)) return ErrSave;
    column+=4;
    offset+=4;
    break;
  case FXVariant::VBool:
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
  case FXVariant::VChar:
    string.fromULong(var.asULong());
    if(!emit(string.text(),string.length())) return ErrSave;
    column+=string.length();
    offset+=string.length();
    break;
  case FXVariant::VInt:
  case FXVariant::VLong:
    string.fromLong(var.asLong());
    if(!emit(string.text(),string.length())) return ErrSave;
    column+=string.length();
    offset+=string.length();
    break;
  case FXVariant::VUInt:
  case FXVariant::VULong:
    string.fromULong(var.asULong());
    if(!emit(string.text(),string.length())) return ErrSave;
    column+=string.length();
    offset+=string.length();
    break;
  case FXVariant::VFloat:
  case FXVariant::VDouble:
    string.fromDouble(var.asDouble(),prec,fmt);
    if(!emit(string.text(),string.length())) return ErrSave;
    column+=string.length();
    offset+=string.length();
    break;
  case FXVariant::VPointer:
    string.format("%p",var.asPtr());
    if(!emit(string.text(),string.length())) return ErrSave;
    column+=string.length();
    offset+=string.length();
    break;
  case FXVariant::VString:
    if(saveString(var.asString())!=ErrOK) return ErrSave;
    break;
  case FXVariant::VArray:
    if(saveArray(var)!=ErrOK) return ErrSave;
    break;
  case FXVariant::VMap:
    if(saveMap(var)!=ErrOK) return ErrSave;
    break;
    }
  return ErrOK;
  }


// Save a variant
FXJSON::Error FXJSON::save(const FXVariant& variant){
  FXTRACE((100,"FXJSON::save(variant)\n"));
  Error err=ErrSave;
  if(dir==Save){
    err=saveVariant(variant);
    if(flush(0)<0) err=ErrSave;
    }
  return err;
  }

/*******************************************************************************/

// Close stream and delete buffers
FXbool FXJSON::close(){
  FXTRACE((100,"FXJSON::close()\n"));
  if(dir!=Stop){
    if((dir==Load) || 0<=flush(0)){             // Error during final flush is possible
      begptr=NULL;
      endptr=NULL;
      wptr=NULL;
      rptr=NULL;
      sptr=NULL;
      dir=Stop;
      return true;
      }
    begptr=NULL;
    endptr=NULL;
    wptr=NULL;
    rptr=NULL;
    sptr=NULL;
    dir=Stop;
    }
  return false;
  }


// Close stream and clean up
FXJSON::~FXJSON(){
  FXTRACE((100,"FXJSON::~FXJSON\n"));
  close();
  }

}
