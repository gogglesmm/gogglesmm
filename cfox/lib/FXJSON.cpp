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

  - Closing the file does not reset comment and line number (in case of error,
    this point to the problem).

  - JSON5 syntax is very simple:

         value       : object | array | string | number | boolean | empty
                     ;

         object      : '{' '}'
                     | '{' members [ ',' ] '}'
                     ;

         members     : pair
                     | pair ',' members
                     ;

         pair        : name ':' value
                     ;

         name        : string
                     | ident
                     ;

         array       : '[' ']'
                     | '[' elements [ ',' ] ']'
                     ;

         elements    : value
                     | value ',' elements
                     ;

         string      : '"' dq-chars '"'
                     | '\'' sq-chars '\''
                     ;

         dq-chars    : dq-char
                     | dq-char dq-chars
                     | empty
                     ;

         sq-chars    : sq-char
                     : char sq-chars
                     | empty
                     ;

         dq-char     : any-character-except-double-quotes
                     | '\\"' | '\\\\' | '\\/' | '\\b' | '\\f' | '\\n' | '\\r' | '\\t'
                     | '\\x' hxdigit hxdigit
                     | '\\u' hxdigit hxdigit hxdigit hxdigit
                     | '\\' octaldigits
                     | '\\' newline
                     ;

         sq-char     : any-character-except-single-quotes
                     | '\\\'' | '\\\\' | '\\/' | '\\b' | '\\f' | '\\n' | '\\r' | '\\t'
                     | '\\x' hxdigit hxdigit
                     | '\\u' hxdigit hxdigit hxdigit hxdigit
                     | '\\' octaldigits
                     | '\\' newline
                     ;

         newline     : '\n'
                     | '\r\n'
                     | '\r'
                     ;

         empty       : 'null'
                     ;

         boolean     : 'true'
                     | 'false'
                     ;

         ident       : alpha
                     | alpha alphanums
                     ;

         alpha       : 'a'...'z' | 'A'...'Z' | '_' | '$'
                      | unicode-categories in { Lu, Ll, Lt, Lm, Lo, Nl }
                     ;

         alphanums   : alphanum
                     | alphanum alphanums
                     ;

         alphanum    : 'a'...'z' | 'A'...'Z' | '0'...'9'  | '_' | '$'
                     | unicode-categories in { Lu, Ll, Lt, Lm, Lo, Nl, Mn, Mc, Nd, Pc }
                     ;

         number      : [ '+' | '-' ] literal
                     ;

         literal     : decimal
                     | hex
                     | ('n' | 'N') ('a' | 'A') ('n' | 'N')
                     | ('i' | 'I') ('n' | 'N') ('f' | 'F')
                     | ('i' | 'I') ('n' | 'N') ('f' | 'F') ('i' | 'I') ('n' | 'N') ('i' | 'I') ('t' | 'T') ('y' | 'Y')
                     ;

         decimal     : digits
                     | fraction
                     | fraction exponent
                     ;

         fraction    : digits '.'
                     | '.' digits
                     | digits '.' digits
                     ;

         exponent    : ('e' | 'E') [ '+' | '-' ] digits
                     ;

         hex         : '0' ('x' | 'X') hexdigits
                     ;

         digits      : digit
                     | digit digits
                     ;

         octaldigits : octaldigit
                     | octaldigit octaldigits
                     ;

         hexdigits   : hexdigit
                     | hexdigit hexdigits
                     ;

         digit       : '0'...'9'
                     ;

         octaldigit  : '0'...'7'
                     ;

         hexdigit    : '0'...'9' | 'a'...'f' | 'A'...'F'
                     ;

  - Our version accepts 'Inf' in lieu of 'Infinity' for infinite float values; also, it
    relaxes capitalization; this is for compatibility with various C-libraries
    implementations, which have minor differences in the way these numbers are printed.

  - Our implementation __vsnprintf() produces either INF or inf for infinity, and NAN
    or nan for not-a-number, while our __vsscanf() implementation accepts any
    capitalization of nan, inf, or infinity.

  - In our JSON5 implementation, the reserved words "true", "false", "null", etc.
    may serve as identifiers in an Object.  This prevents unexpected surprises
    loading back JSON5 files.

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

    Quotes will be escaped; output will be written to JSON standard

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
    quotes; it is upon the user to ensure that keys follow proper identifier syntax
    if the json5 file is to be loaded correctly.
*/

using namespace FX;

/*******************************************************************************/

namespace FX {

// Maximum token size
enum {
  MAXTOKEN = 256
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
  "Unexpected identifier",
  "Unexpected end of file"
  };


// Special double constants
static const union{ FXulong u; FXdouble f; } dblinf={FXULONG(0x7ff0000000000000)};
static const union{ FXulong u; FXdouble f; } dblnan={FXULONG(0x7fffffffffffffff)};

// Furnish our own versions
extern FXAPI FXlong __strtoll(const FXchar *beg,const FXchar** end=nullptr,FXint base=0,FXbool* ok=nullptr);
extern FXAPI FXdouble __strtod(const FXchar *beg,const FXchar** end=nullptr,FXbool* ok=nullptr);

// Unicode IdentifierStart character category set
const FXuint IdentStart=(1<<CatLetterUpper)|(1<<CatLetterLower)|(1<<CatLetterTitle)|(1<<CatLetterModifier)|(1<<CatLetterOther)|(1<<CatNumberLetter);


// Unicode IdentifierPart character category set
const FXuint IdentPart=(1<<CatLetterUpper)|(1<<CatLetterLower)|(1<<CatLetterTitle)|(1<<CatLetterModifier)|(1<<CatLetterOther)|(1<<CatNumberLetter)|(1<<CatNumberDecimal)|(1<<CatMarkNonSpacing)|(1<<CatMarkSpacingCombining)|(1<<CatPunctConnector);

/*******************************************************************************/

// Construct JSON serializer
FXJSON::FXJSON():offset(0),token(TK_EOF),column(0),indent(0),line(1),wrap(80),quote('"'),flow(Compact),prec(15),fmt(2),esc(0),dent(2),ver(4){
  FXTRACE((100,"FXJSON::FXJSON\n"));
  }


// Construct and open for loading
FXJSON::FXJSON(FXchar* buffer,FXuval sz,Direction d):FXParseBuffer(buffer,sz,d),offset(0),token(TK_EOF),column(0),indent(0),line(1),wrap(80),quote('"'),flow(Compact),prec(15),fmt(2),esc(0),dent(2),ver(4){
  FXTRACE((100,"FXJSON::FXJSON(%p,%lu,%s)\n",buffer,sz,(d==Save)?"Save":(d==Load)?"Load":"Stop"));
  open(buffer,sz,d);
  }


// Open JSON stream for given direction and set its buffer
FXbool FXJSON::open(FXchar* buffer,FXuval sz,Direction d){
  FXTRACE((101,"FXJSON::open(%p,%lu,%s)\n",buffer,sz,(d==Save)?"Save":(d==Load)?"Load":"Stop"));
  if(FXParseBuffer::open(buffer,Math::imax(sz,MAXTOKEN),d)){
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


// Close json parse buffer
FXbool FXJSON::close(){
  FXTRACE((101,"FXJSON::close()\n"));
  if(FXParseBuffer::close()){
    value=FXString::null;
    return true;
    }
  return false;
  }

/*******************************************************************************/

// Get next token
FXJSON::Token FXJSON::next(){
  Token tok=TK_EOF;
  FXint comment=0;
  FXuchar c;

  // While more data
  while(need(MAXTOKEN)){

    // Start new token
    tok=TK_ERROR;
    rptr=sptr;

    // Process characters
    switch((c=sptr[0])){
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
        if(sptr[1]=='\n'){
          offset++;
          sptr++;
          }
        //FALL//
      case '\n':                                        // Newline
        if(comment<0) comment=0;                        // End single-line comment
        column=0;
        offset++;
        sptr++;
        line++;
        continue;
      case '!':                                         // Unassigned characters
      case '#':
      case '%':
      case '&':
      case '(':
      case ')':
      case ';':
      case '<':
      case '=':
      case '>':
      case '?':
      case '@':
      case '^':
      case '`':
      case '|':
      case '~':
        if(!comment) return TK_ERROR;
cmt:    column++;                                       // Comments
        offset++;
        sptr++;
        continue;
      case '/':                                         // Start or end of comment
        if(sptr[1]=='/' && comment==0){
          comment=-1;                                   // Comment until end of line
          column+=2;
          offset+=2;
          sptr+=2;
          continue;
          }
        if(sptr[1]=='*' && comment>=0){
          comment+=1;                                   // Increase comment nesting level
          column+=2;
          offset+=2;
          sptr+=2;
          continue;
          }
        if(comment) goto cmt;
        return TK_ERROR;
      case '*':
        if(sptr[1]=='/' && comment>=1){
          comment-=1;                                   // Decrease comment nesting level
          column+=2;
          offset+=2;
          sptr+=2;
          continue;
          }
        if(comment) goto cmt;
        return TK_ERROR;
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
        if(comment) goto cmt;
        return string();
      case '+':                                         // Plus sign
        column++;
        offset++;
        sptr++;
        if(comment) continue;
        return TK_PLUS;
      case '-':                                         // Minus sign
        column++;
        offset++;
        sptr++;
        if(comment) continue;
        return TK_MINUS;
      case '0':                                         // Number
        column++;
        offset++;
        sptr++;
        if(comment) continue;
        if((sptr[0]|0x20)=='x'){                        // Hex
          column++;
          offset++;
          sptr++;
          while(Ascii::isHexDigit(sptr[0])){
            tok=TK_HEX;
            column++;
            offset++;
            sptr++;
            }
          return tok;
          }
        //FALL//
      case '1':                                         // Number
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        tok=TK_INT;
        while('0'<= sptr[0] && sptr[0]<='9'){
          tok=TK_INT;
          column++;
          offset++;
          sptr++;
          }
        //FALL//
      case '.':                                         // Fraction
        if(sptr[0]=='.'){
          column++;
          offset++;
          sptr++;
          if(tok==TK_INT) tok=TK_REAL;
          while('0'<= sptr[0] && sptr[0]<='9'){
            tok=TK_REAL;
            column++;
            offset++;
            sptr++;
            }
          }
        if(comment) continue;
        if(tok!=TK_ERROR){
          if((sptr[0]|0x20)=='e'){                      // Exponent part
            column++;
            offset++;
            sptr++;
            if(sptr[0]=='-' || sptr[0]=='+'){
              column++;
              offset++;
              sptr++;
              }
            tok=TK_ERROR;
            while('0'<= sptr[0] && sptr[0]<='9'){
              tok=TK_REAL;
              column++;
              offset++;
              sptr++;
              }
            }
          return tok;
          }
        return TK_ERROR;                                // Error
      case 'a':                                         // Identifier
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
        if(comment){
          column++;
          offset++;
          sptr++;
          continue;
          }
        return ident();
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
        if((sptr[1]&192)!=128) return TK_ERROR;         // Bad UTF8 followers
        if(sptr[0]=='\xC2' && sptr[1]=='\xA0'){
          column++;
          offset+=2;                                    // Non Breakable Space (\xC2\xA0)
          sptr+=2;
          continue;
          }
        if(comment){
          column++;
          offset+=2;
          sptr+=2;
          continue;
          }
        return ident();
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
        if((sptr[1]&192)!=128) return TK_ERROR;         // Bad UTF8 followers
        if((sptr[2]&192)!=128) return TK_ERROR;
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
        if(comment){
          column++;
          offset+=3;
          sptr+=3;
          continue;
          }
        return ident();
      case 0xF0:                                        // 4-byte UTF8 sequences
      case 0xF1:
      case 0xF2:
      case 0xF3:
      case 0xF4:
      case 0xF5:
      case 0xF6:
      case 0xF7:
        if((sptr[1]&192)!=128) return TK_ERROR;         // Bad UTF8 followers
        if((sptr[2]&192)!=128) return TK_ERROR;
        if((sptr[3]&192)!=128) return TK_ERROR;
        if(comment){
          column++;
          offset+=4;
          sptr+=4;
          continue;
          }
        return ident();
      default:                                          // Bad token
        return TK_ERROR;
      }
    }
  return TK_EOF;
  }


// Identifier token
FXJSON::Token FXJSON::identoken(const FXString& str){
  switch(str[0]){
  case 'n':                                     // Check for 'null'
    if(str[1]=='u' && str[2]=='l' && str[3]=='l' && str.length()==4) return TK_NULL;
    // FALL //
  case 'N':                                     // Check for 'NaN'
    if((str[1]|0x20)=='a' && (str[2]|0x20)=='n' && str.length()==3) return TK_NAN;
    return TK_IDENT;
  case 'i':
  case 'I':                                     // Check for 'Inf' or 'Infinity'
    if((str[1]|0x20)=='n' && (str[2]|0x20)=='f' && (str.length()==3 || ((str[3]|0x20)=='i' && (str[4]|0x20)=='n' && (str[5]|0x20)=='i' && (str[6]|0x20)=='t' && (str[7]|0x20)=='y' && str.length()==8))) return TK_INF;
    return TK_IDENT;
  case 't':                                     // Check for 'true'
    if(str[1]=='r' && str[2]=='u' && str[3]=='e' && str.length()==4) return TK_TRUE;
    return TK_IDENT;
  case 'f':                                     // Check for 'false'
    if(str[1]=='a' && str[2]=='l' && str[3]=='s' && str[4]=='e' && str.length()==5) return TK_FALSE;
    return TK_IDENT;
  default:
    return TK_IDENT;
    }
  return TK_IDENT;
  }


// Identifier
FXJSON::Token FXJSON::ident(){
  FXuint charset=IdentStart;
  FXuchar c,cc;
  value=FXString::null;
  while(need(MAXTOKEN)){
    switch((c=sptr[0])){
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
        break;
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
        if((sptr[1]&192)!=128) return TK_ERROR;         // Bad UTF8 followers
        cc=Unicode::charCategory(wc2(sptr));
        if(!FXBIT(charset,cc)) goto end;
        column++;
        offset+=2;
        sptr+=2;
        break;
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
        if((sptr[1]&192)!=128) return TK_ERROR;         // Bad UTF8 followers
        if((sptr[2]&192)!=128) return TK_ERROR;
        cc=Unicode::charCategory(wc3(sptr));
        if(!FXBIT(charset,cc)) goto end;
        column++;
        offset+=3;
        sptr+=3;
        break;
      case 0xF0:                                        // 4-byte UTF8 sequences
      case 0xF1:
      case 0xF2:
      case 0xF3:
      case 0xF4:
      case 0xF5:
      case 0xF6:
      case 0xF7:
        if((sptr[1]&192)!=128) return TK_ERROR;         // Bad UTF8 followers
        if((sptr[2]&192)!=128) return TK_ERROR;
        if((sptr[3]&192)!=128) return TK_ERROR;
        cc=Unicode::charCategory(wc4(sptr));
        if(!FXBIT(charset,cc)) goto end;
        column++;
        offset+=4;
        sptr+=4;
        break;
      default:                                          // Identifier or reserved word
end:    value.append(rptr,sptr-rptr);                   // Reserved words can be interpreted as identifier,
        return identoken(value);                        // so we must copy them, just in case.
      }

    // Character set
    charset=IdentPart;

    // Big token handling
    if(sptr+MAXTOKEN>wptr && wptr==endptr){
      value.append(rptr,sptr-rptr);
      rptr=sptr;
      }
    }
  return TK_EOF;
  }


// String
FXJSON::Token FXJSON::string(){
  FXuchar q=rptr[0];
  FXuchar c;
  column++;
  offset++;
  sptr++;
  value=FXString::null;
  while(need(MAXTOKEN)){
    switch((c=sptr[0])){
      case '\t':                                        // Tab hops to next tabstop
        column+=(8-column%8);
        offset++;
        sptr++;
        break;
      case '\v':                                        // Vertical tab
      case '\f':                                        // Form feed
        offset++;
        sptr++;
        break;
      case '\r':                                        // Carriage return
      case '\n':                                        // Newline
        return TK_ERROR;
      case '"':                                         // End of string
      case '\'':
        column++;
        offset++;
        sptr++;
        if(q!=c) break;                                 // Opening quote?
        value.append(rptr,sptr-rptr);                   // Copy tail-end of string
        return TK_STRING;
      case '\\':                                        // Escape next character
        column++;
        offset++;
        sptr++;
        if(sptr[0]=='\n'){                              // Check line-continuation
          column=0;                                     // Reset to line start
          line++;                                       // Add line
          offset++;
          sptr++;
          break;
          }
        if(sptr[0]=='\r'){                              // Check line-continuation
          if(sptr[1]=='\n'){ offset++; sptr++; }
          column=0;                                     // Reset to line start
          line++;                                       // Add line
          offset++;
          sptr++;
          break;
          }
        //FALL//
      case ' ':                                         // Characters
      case '!':
      case '#':
      case '$':
      case '%':
      case '&':
      case '(':
      case ')':
      case '*':
      case '+':
      case ',':
      case '-':
      case '.':
      case '/':
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
      case ':':
      case ';':
      case '<':
      case '=':
      case '>':
      case '?':
      case '@':
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
      case '[':
      case ']':
      case '^':
      case '_':
      case '`':
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
      case '{':
      case '|':
      case '}':
      case '~':
        column++;
        offset++;
        sptr++;
        break;
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
        if((sptr[1]&192)!=128) return TK_ERROR;         // Bad UTF8 followers
        column++;
        offset+=2;
        sptr+=2;
        break;
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
        if((sptr[1]&192)!=128) return TK_ERROR;         // Bad UTF8 followers
        if((sptr[2]&192)!=128) return TK_ERROR;
        if(sptr[0]=='\xE2' && sptr[1]=='\x80' && (sptr[2]=='\xA8' || sptr[2]=='\xA9')){
          column=-1;
          line++;                                       // Line Separator or Paragraph Separator
          }
        column++;
        offset+=3;
        sptr+=3;
        break;
      case 0xF0:                                        // 4-byte UTF8 sequences
      case 0xF1:
      case 0xF2:
      case 0xF3:
      case 0xF4:
      case 0xF5:
      case 0xF6:
      case 0xF7:
        if((sptr[1]&192)!=128) return TK_ERROR;         // Bad UTF8 followers
        if((sptr[2]&192)!=128) return TK_ERROR;
        if((sptr[3]&192)!=128) return TK_ERROR;
        column++;
        offset+=4;
        sptr+=4;
        break;
      default:
        return TK_ERROR;                                // Bad characters
      }

    // Big token handling
    if(sptr+MAXTOKEN>wptr && wptr==endptr){
      value.append(rptr,sptr-rptr);
      rptr=sptr;
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

  // Parse key : value pairs
  // Allow either string (old JSON), or identifier (JSON5)
  // syntax (this includes reserved words such as null, true
  // false, etc, as there can be no confusion here).
  while(TK_IDENT<=token && token<=TK_STRING){

    // Decode the keys
    if(token==TK_STRING){
      key=FXString::unescape(value,value.head(),value.tail());
      }
    else{
      key=value;
      }

    // Token following the string
    token=next();
    if(token==TK_EOF) return ErrEnd;

    // Expect colon
    if(token!=TK_COLON) return ErrColon;

    // Eat the colon
    token=next();
    if(token==TK_EOF) return ErrEnd;

    // Load item directly into associated slot
    if((err=loadVariant(var[key]))!=ErrOK) return err;

    // Expect another key-value pair
    if(token!=TK_COMMA) break;

    // Eat the comma
    token=next();
    if(token==TK_EOF) return ErrEnd;
    }
  return ErrOK;
  }


// Load array elements into var
FXJSON::Error FXJSON::loadArray(FXVariant& var){
  FXival index=0;
  Error err;

  // Make it into an array now
  var.setType(FXVariant::ArrayType);

  // Parse values
  // Here reserved words have special meanings;
  // note that identifiers are excluded.
  while(TK_NAN<=token && token<=TK_LBRACE){

    // Load item directly into array slot
    if((err=loadVariant(var[index]))!=ErrOK) return err;

    // Expect another value
    if(token!=TK_COMMA) break;

    // Next token
    token=next();
    if(token==TK_EOF) return ErrEnd;

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
  case TK_PLUS:
    token=next();
    if(token==TK_EOF) return ErrEnd;
    if(token==TK_INT){                          // +Integer
      var=__strtoll(rptr,nullptr,10);
      token=next();
      return ErrOK;
      }
    if(token==TK_HEX){                          // +Hexadecimal
      var=__strtoll(rptr,nullptr,16);
      token=next();
      return ErrOK;
      }
    if(token==TK_REAL){                         // +Real
      var=__strtod(rptr,nullptr);
      token=next();
      return ErrOK;
      }
    if(token==TK_INF){                          // +Infinity
      var=dblinf.f;
      token=next();
      return ErrOK;
      }
    if(token==TK_NAN){                          // +NaN
      var=dblnan.f;
      token=next();
      return ErrOK;
      }
    return ErrToken;
  case TK_MINUS:
    token=next();
    if(token==TK_EOF) return ErrEnd;
    if(token==TK_INT){                          // -Integer
      var=-__strtoll(rptr,nullptr,10);
      token=next();
      return ErrOK;
      }
    if(token==TK_HEX){                          // -Hexadecimal
      var=-__strtoll(rptr,nullptr,16);
      token=next();
      return ErrOK;
      }
    if(token==TK_REAL){                         // -Real
      var=-__strtod(rptr,nullptr);
      token=next();
      return ErrOK;
      }
    if(token==TK_INF){                          // -Infinity
      var=-dblinf.f;
      token=next();
      return ErrOK;
      }
    if(token==TK_NAN){                          // -NaN
      var=-dblnan.f;
      token=next();
      return ErrOK;
      }
    return ErrToken;
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
  case TK_INT:                                  // Integer
    var=__strtoll(rptr,nullptr,10);
    token=next();
    return ErrOK;
  case TK_HEX:                                  // Hex
    var=__strtoll(rptr,nullptr,16);
    token=next();
    return ErrOK;
  case TK_REAL:                                 // Real
    var=__strtod(rptr,nullptr);
    token=next();
    return ErrOK;
  case TK_INF:                                  // Infinity
    var=dblinf.f;
    token=next();
    return ErrOK;
  case TK_NAN:                                  // NaN
    var=dblnan.f;
    token=next();
    return ErrOK;
  case TK_LBRACK:                               // Array
    token=next();
    if(token==TK_EOF) return ErrEnd;
    if((err=loadArray(var))!=ErrOK) return err;
    if(token!=TK_RBRACK) return ErrBracket;     // Expected closing bracket
    token=next();
    return ErrOK;
  case TK_LBRACE:                               // Map
    token=next();
    if(token==TK_EOF) return ErrEnd;
    if((err=loadMap(var))!=ErrOK) return err;
    if(token!=TK_RBRACE) return ErrBrace;       // Expected closing brace
    token=next();
    return ErrOK;
  case TK_STRING:                               // String
    var=FXString::unescape(value,value.head(),value.tail());
    token=next();
    return ErrOK;
  case TK_IDENT:                                // Unexpected identifier
    var=FXVariant::null;
    return ErrIdent;
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

      // JSON5 output
      if(5<=ver){
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

// Close stream and clean up
FXJSON::~FXJSON(){
  FXTRACE((100,"FXJSON::~FXJSON\n"));
  close();
  }

}
