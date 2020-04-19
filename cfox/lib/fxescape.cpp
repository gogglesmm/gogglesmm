/********************************************************************************
*                                                                               *
*                   E s c a p e  /  U n e s c a p e   S t r i n g               *
*                                                                               *
*********************************************************************************
* Copyright (C) 2018,2019 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "FXArray.h"
#include "FXHash.h"
#include "FXStream.h"
#include "FXString.h"


/*
  Notes:

  - Escape or unescape string, optionally adding or removing quotes.

  - This makes the string safe when it contains non-printing characters.

  - UTF8 characters may be encoded as hex (in the form of: \xHH), or as unicode
    escape sequences (of the form \uHHHH).  Code points exceeding 16-bits will be
    encoded as hex-encoded surrogate-pairs (\uHHHH\uHHHH) in unicode escape mode.

  - Control characters, and non-ASCII characters other than UTF8 leaders or followers
    will always be escaped.

  - UTF8 leaders or followers will be hex escaped if hex-escape mode in effect.

  - If unicode escape in effect, UTF8 followers will be escaped as \xHH when not
    preceeded by  UTF8 leaders.

  - Bad UTF8 sequences will be escaped as hex \xHH if unicode escaping in effect.

  - End-of-line continuation: '\<LF>', '\<CR>', '\<CR><LF>' will be replaced by nothing
    when unescaped.  Allows long strings from being broken up into separate lines.
    
  - Bottom line: an escaped string shall not contain any control characters, UTF8
    followers not preceeded by a UTF8 leader, line terminators, or leading or
    trailing spaces surrounding printable characters.
*/


using namespace FX;

/*******************************************************************************/

namespace FX {

// For conversion from UTF16 to UTF32
const FXint SURROGATE_OFFSET=0x10000-(0xD800<<10)-0xDC00;

// For conversion of UTF32 to UTF16
const FXint LEAD_OFFSET=0xD800-(0x10000>>10);

// For conversion of UTF32 to UTF16
const FXint TAIL_OFFSET=0xDC00;


// Check if the string contains special characters or leading or trailing whitespace, or contains utf8 if flag!=0
FXbool FXString::shouldEscape(const FXString& str,FXchar lquote,FXchar rquote,FXint flag){
  if(0<str.length()){
    FXint p=0; FXuchar c;
    c=str.head();
    if(c<=0x20) return true;
    c=str.tail();
    if(c<=0x20) return true;
    while(p<str.length()){
      c=str[p++];
      if(c<=0x1F || 0xF8<=c || c==0x7F) return true;
      if(c=='\\' || c==lquote || c==rquote) return true;
      if(0x80<=c && flag) return true;
      }
    }
  return false;
  }


// Escape special characters, and optionally enclose with left and right quotes
// and escape utf8 as \xHH if flag=1, or as \uHHHH if flag=2.
// UTF8 characters may be encoded as hex (in the form of: \xHH), or as Unicode
// escape sequences (of the form \uHHHH).  Code points exceeding 16-bits will be
// encoded as hex-encoded surrogate-pairs (\uHHHH\uHHHH) in Unicode escape mode.
// UTF8 followers will be always escaped if not preceeded by UTF8 leaders, if
// escaping is enabled.
FXString FXString::escape(const FXString& str,FXchar lquote,FXchar rquote,FXint flag){
  FXString result;
  FXint p,q,w,v;
  FXuchar c,cc;
  p=q=0;
  if(lquote) q++;                               // Opening quote
  while(p<str.length()){                        // Measure length of converted string
    c=str[p++];
    switch(c){
      case '\a':                                // Special control characters
      case '\b':
      case '\t':
      case '\n':
      case '\v':
      case '\f':
      case '\r':
      case '\\':
        q+=2;
        continue;
      case 0x00:                                // Non-special control characters
      case 0x01:
      case 0x02:
      case 0x03:
      case 0x04:
      case 0x05:
      case 0x06:
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
      case 0xF8:                                // Bad UTF8 leaders
      case 0xF9:
      case 0xFA:
      case 0xFB:
      case 0xFC:
      case 0xFD:
      case 0xFF:
hex1:   q+=4;                                   // Escape as \xHH
        continue;
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
        if(flag==0) goto nml1;                  // Pass UTF8 through
        goto hex1;                              // Escape UTF8 follower not preceeded by UTF8 leader
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
      case 0xF0:                                // 4-byte UTF8 sequences
      case 0xF1:
      case 0xF2:
      case 0xF3:
      case 0xF4:
      case 0xF5:
      case 0xF6:
      case 0xF7:
        if(flag==0) goto nml1;                  // Pass UTF8 through
        if(flag==1) goto hex1;                  // Simple hex escaping
        cc=str[p];
        if(!FXISFOLLOWUTF8(cc)) goto hex1;      // UTF8 follower?
        w=(c<<6)^cc^0x3080;
        if(0x800<=w){
          cc=str[p+1];
          if(!FXISFOLLOWUTF8(cc)) goto hex1;    // UTF8 follower?
          w=(w<<6)^cc^0x20080;
          if(0x10000<=w){                       // Surrogate pair needed
            cc=str[p+2];
            if(!FXISFOLLOWUTF8(cc)) goto hex1;  // UTF8 follower?
            w=(w<<6)^cc^0x400080;
            if(0x110000<=w) goto hex1;          // Beyond assigned code space?
            q+=6;
            p++;
            }
          p++;
          }
        q+=6;
        p++;
        continue;
      default:
        if(c==lquote && lquote){                // Escape opening quote if found in string
          q+=2;
          continue;
          }
        if(c==rquote && rquote){                // Escape closing quote if found in string
          q+=2;
          continue;
          }
nml1:   q+=1;                                   // Normal characters
        continue;
      }
    }
  if(rquote) q++;                               // Closing quote
  result.length(q);
  p=q=0;
  if(lquote) result[q++]=lquote;                // Opening quote
  while(p<str.length()){                        // Then convert the string
    c=str[p++];
    switch(c){
      case '\a':                                // Special control characters
        result[q++]='\\';
        result[q++]='a';
        continue;
      case '\b':
        result[q++]='\\';
        result[q++]='b';
        continue;
      case '\t':
        result[q++]='\\';
        result[q++]='t';
        continue;
      case '\n':
        result[q++]='\\';
        result[q++]='n';
        continue;
      case '\v':
        result[q++]='\\';
        result[q++]='v';
        continue;
      case '\f':
        result[q++]='\\';
        result[q++]='f';
        continue;
      case '\r':
        result[q++]='\\';
        result[q++]='r';
        continue;
      case '\\':
        result[q++]='\\';
        result[q++]='\\';
        continue;
      case 0x00:                                // Non-special control characters
      case 0x01:
      case 0x02:
      case 0x03:
      case 0x04:
      case 0x05:
      case 0x06:
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
      case 0xF8:                                // Bad UTF8 leaders
      case 0xF9:
      case 0xFA:
      case 0xFB:
      case 0xFC:
      case 0xFD:
      case 0xFF:
hex2:   result[q++]='\\';                       // Escape as \xHH
        result[q++]='x';
        result[q++]=FXString::value2Digit[(c>>4)&15];
        result[q++]=FXString::value2Digit[c&15];
        continue;
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
        if(flag==0) goto nml2;                  // Pass UTF8 through
        goto hex2;                              // Escape UTF8 follower not preceeded by UTF8 leader
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
      case 0xF0:                                 // 4-byte UTF8 sequences
      case 0xF1:
      case 0xF2:
      case 0xF3:
      case 0xF4:
      case 0xF5:
      case 0xF6:
      case 0xF7:
        if(flag==0) goto nml2;                  // Pass UTF8 through
        if(flag==1) goto hex2;                  // Simple hex escaping
        cc=str[p];
        if(!FXISFOLLOWUTF8(cc)) goto hex2;      // UTF8 follower?
        w=(c<<6)^cc^0x3080;
        if(0x800<=w){
          cc=str[p+1];
          if(!FXISFOLLOWUTF8(cc)) goto hex2;    // UTF8 follower?
          w=(w<<6)^cc^0x20080;
          if(0x10000<=w){                       // Surrogate pair needed
            cc=str[p+2];
            if(!FXISFOLLOWUTF8(cc)) goto hex2;  // UTF8 follower?
            w=(w<<6)^cc^0x400080;
            if(0x110000<=w) goto hex2;          // Beyond assigned code space?
            v=LEAD_OFFSET+(w>>10);
            w=TAIL_OFFSET+(w&0x3FF);
            result[q++]='\\';                   // Escape as \uHHHH
            result[q++]='u';
            result[q++]=FXString::value2Digit[(v>>12)&15];
            result[q++]=FXString::value2Digit[(v>>8)&15];
            result[q++]=FXString::value2Digit[(v>>4)&15];
            result[q++]=FXString::value2Digit[v&15];
            p++;
            }
          p++;
          }
        result[q++]='\\';                       // Escape as \uHHHH
        result[q++]='u';
        result[q++]=FXString::value2Digit[(w>>12)&15];
        result[q++]=FXString::value2Digit[(w>>8)&15];
        result[q++]=FXString::value2Digit[(w>>4)&15];
        result[q++]=FXString::value2Digit[w&15];
        p++;
        continue;
      default:
        if(__unlikely(c==lquote && lquote)){    // Escape opening quote if found in string
          result[q++]='\\';
          result[q++]=lquote;
          continue;
          }
        if(__unlikely(c==rquote && rquote)){    // Escape closing quote if found in string
          result[q++]='\\';
          result[q++]=rquote;
          continue;
          }
nml2:   result[q++]=c;                          // Normal characters
        continue;
      }
    }
  if(rquote) result[q++]=rquote;                // Closing quote
  FXASSERT(q==result.length());
  return result;
  }

/*******************************************************************************/

// Unescape special characters in a string; optionally strip quote characters
FXString FXString::unescape(const FXString& str,FXchar lquote,FXchar rquote){
  FXString result;
  FXint p,q,w,c;
  p=q=c=w=0;
  if(str[p]==lquote && lquote) p++;     // Opening quote
  while(p<str.length()){                // Measure length of converted string
    w=c;                                // Keep previous decoded character
    c=str[p++];
    if(c==rquote && rquote) break;      // Closing quote
    if(c=='\\' && p<str.length()){      // Escape sequence
      switch((c=str[p++])){
        case 'u':                       // Unicode escape
          c=0;
          if(Ascii::isHexDigit(str[p])){
            c=(c<<4)+Ascii::digitValue(str[p++]);
            if(Ascii::isHexDigit(str[p])){
              c=(c<<4)+Ascii::digitValue(str[p++]);
              if(Ascii::isHexDigit(str[p])){
                c=(c<<4)+Ascii::digitValue(str[p++]);
                if(Ascii::isHexDigit(str[p])){
                  c=(c<<4)+Ascii::digitValue(str[p++]);
                  }
                }
              }
            }
          if(FXISLEADUTF16(c)) continue;
          if(FXISFOLLOWUTF16(c)){
            if(!FXISLEADUTF16(w)) continue;
            c=SURROGATE_OFFSET+(w<<10)+c;
            }
          q+=wc2utf(c);
          continue;
        case 'x':                       // Hex escape
          if(Ascii::isHexDigit(str[p])){
            p++;
            if(Ascii::isHexDigit(str[p])) p++;
            }
          q++;
          continue;
        case '\r':                      // End-of-line continuation
          if(str[p]=='\n') p++;         // Eat both <CR> and <LF> of <CR><LF>
          continue;
        case '\n':                      // End-of-line continuation
          continue;
        case '0':                       // Octal escape
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
          if(Ascii::isOctDigit(str[p])){
            p++;
            if(Ascii::isOctDigit(str[p])) p++;
            }
          q++;
          continue;
        case 'n':                       // Special characters
        case 'r':
        case 'b':
        case 'v':
        case 'a':
        case 'e':                       // Escape
        case 'f':
        case 't':
        case '\\':
        default:                        // Unneccessarily escaped character
          q++;
          continue;
        }
      }
    q++;                                // Normal characters
    }
  result.length(q);                     // Resize result string
  p=q=c=0;
  if(str[p]==lquote && lquote) p++;     // Opening quote
  while(p<str.length()){                // Then convert the string
    w=c;                                // Keep previous decoded character
    c=str[p++];
    if(c==rquote && rquote) break;      // Closing quote
    if(c=='\\' && p<str.length()){      // Escape sequence
      switch((c=str[p++])){
        case 'u':                       // Unicode escape
          if(Ascii::isHexDigit(str[p])){
            c=Ascii::digitValue(str[p++]);
            if(Ascii::isHexDigit(str[p])){
              c=(c<<4)+Ascii::digitValue(str[p++]);
              if(Ascii::isHexDigit(str[p])){
                c=(c<<4)+Ascii::digitValue(str[p++]);
                if(Ascii::isHexDigit(str[p])){
                  c=(c<<4)+Ascii::digitValue(str[p++]);
                  }
                }
              }
            }
          if(FXISLEADUTF16(c)) continue;
          if(FXISFOLLOWUTF16(c)){
            if(!FXISLEADUTF16(w)) continue;
            c=SURROGATE_OFFSET+(w<<10)+c;
            }
          q+=wc2utf(&result[q],c);
          continue;
        case 'x':                       // Hex escape
          if(Ascii::isHexDigit(str[p])){
            c=Ascii::digitValue(str[p++]);
            if(Ascii::isHexDigit(str[p])){
              c=(c<<4)+Ascii::digitValue(str[p++]);
              }
            }
          result[q++]=c;
          continue;
        case '\r':                      // End-of-line continuation
          if(str[p]=='\n') p++;         // Eat both <CR> and <LF> of <CR><LF>
          continue;
        case '\n':                      // End-of-line continuation
          continue;
        case '0':                       // Octal escape
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
          c=c-'0';
          if(Ascii::isOctDigit(str[p])){
            c=(c<<3)+str[p++]-'0';
            if(Ascii::isOctDigit(str[p])){
              c=(c<<3)+str[p++]-'0';
              }
            }
          result[q++]=c;
          continue;
        case 'n':                       // Special characters
          result[q++]='\n';
          continue;
        case 'r':
          result[q++]='\r';
          continue;
        case 'b':
          result[q++]='\b';
          continue;
        case 'v':
          result[q++]='\v';
          continue;
        case 'a':
          result[q++]='\a';
          continue;
        case 'e':                       // Escape
          result[q++]='\033';
          continue;
        case 'f':
          result[q++]='\f';
          continue;
        case 't':
          result[q++]='\t';
          continue;
        case '\\':
          result[q++]='\\';
          continue;
        default:                        // Unneccessarily escaped character
          result[q++]=c;
          continue;
        }
      }
    result[q++]=c;                      // Normal characters
    }
  FXASSERT(q==result.length());
  return result;
  }


}
