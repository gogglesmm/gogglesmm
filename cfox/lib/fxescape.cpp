/********************************************************************************
*                                                                               *
*                   E s c a p e  /  U n e s c a p e   S t r i n g               *
*                                                                               *
*********************************************************************************
* Copyright (C) 2018 by Jeroen van der Zijp.   All Rights Reserved.             *
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
FXbool shouldEscape(const FXString& str,FXchar lquote,FXchar rquote,FXint flag){
  if(0<str.length()){
    FXuchar c;
    if(Ascii::isSpace(str.head())) return true;
    if(Ascii::isSpace(str.tail())) return true;
    for(FXint p=0; p<str.length(); p++){
      if((c=str[p])<0x20 || c==0x7F || c=='\\' || c==lquote || c==rquote || (0x80<=c && flag)) return true;
      }
    }
  return false;
  }


// Escape special characters, and optionally enclose with left and right quotes; escape utf8 as \xHH if flag=1, or as \uHHHH if flag=2
FXString escape(const FXString& str,FXchar lquote,FXchar rquote,FXint flag){
  register FXint p,q,w;
  register FXwchar c;
  FXString result;
  p=q=0;
  if(lquote) q++;                       // Opening quote
  while(p<str.length()){                // Measure length of converted string
    c=str[p++];
    switch(c){
    case '\x00':                        // Control characters
    case '\x01':
    case '\x02':
    case '\x03':
    case '\x04':
    case '\x05':
    case '\x06':
    case '\x0E':
    case '\x0F':
    case '\x10':
    case '\x11':
    case '\x12':
    case '\x13':
    case '\x14':
    case '\x15':
    case '\x16':
    case '\x17':
    case '\x18':
    case '\x19':
    case '\x1A':
    case '\x1B':                        // Or represent as \e ?
    case '\x1C':
    case '\x1D':
    case '\x1E':
    case '\x1F':
    case '\x7F':
hex1: q+=4;                             // Escape as \xHH
      continue;
    case '\a':                          // Special characters
    case '\b':
    case '\t':
    case '\n':
    case '\v':
    case '\f':
    case '\r':
    case '\\':
      q+=2;
      continue;
    default:
      if(__unlikely(c==lquote && lquote)){      // Escape opening quote if found in string
        q+=2;
        continue;
        }
      if(__unlikely(c==rquote && rquote)){      // Escape closing quote if found in string
        q+=2;
        continue;
        }
      if(__unlikely(0x80<=c)){          // Escape specials
        if(flag&1) goto hex1;           // Output \xHH for everything
        if(flag&2){
          if(!FXISLEADUTF8(c)) goto hex1;               // UTF8 starter?
          if(!FXISFOLLOWUTF8(str[p])) goto hex1;        // UTF8 follower?
          c=(c<<6)^(FXuchar)str[p]^0x3080;
          if(0x800<=c){
            if(!FXISFOLLOWUTF8(str[p+1])) goto hex1;    // UTF8 follower?
            c=(c<<6)^(FXuchar)str[p+1]^0x20080;
            if(0x10000<=c){                             // Surrogate pair needed
              if(!FXISFOLLOWUTF8(str[p+2])) goto hex1;  // UTF8 follower?
              c=(c<<6)^(FXuchar)str[p+2]^0x400080;
              if(0x110000<=c) goto hex1;                // Beyond assigned code space?
              p++;
              q+=6;
              }
            p++;
            }
          p++;
          q+=6;                                         // Escape as \uHHHH
          continue;
          }
        }
      q+=1;                             // Normal characters
      continue;
      }
    }
  if(rquote) q++;                       // Closing quote
  result.length(q);
  p=q=0;
  if(lquote) result[q++]=lquote;        // Opening quote
  while(p<str.length()){                // Then convert the string
    c=str[p++];
    switch(c){
    case '\x00':                        // Control characters
    case '\x01':
    case '\x02':
    case '\x03':
    case '\x04':
    case '\x05':
    case '\x06':
    case '\x0E':
    case '\x0F':
    case '\x10':
    case '\x11':
    case '\x12':
    case '\x13':
    case '\x14':
    case '\x15':
    case '\x16':
    case '\x17':
    case '\x18':
    case '\x19':
    case '\x1A':
    case '\x1B':                        // Or represent as \e ?
    case '\x1C':
    case '\x1D':
    case '\x1E':
    case '\x1F':
    case '\x7F':
hex2: result[q++]='\\';                 // Escape as \xHH
      result[q++]='x';
      result[q++]=FXString::value2Digit[(c>>4)&15];
      result[q++]=FXString::value2Digit[c&15];
      continue;
    case '\a':                          // Special characters
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
    default:
      if(__unlikely(c==lquote && lquote)){      // Escape opening quote if found in string
        result[q++]='\\';
        result[q++]=lquote;
        continue;
        }
      if(__unlikely(c==rquote && rquote)){      // Escape closing quote if found in string
        result[q++]='\\';
        result[q++]=rquote;
        continue;
        }
      if(__unlikely(0x80<=c)){          // Escape specials
        if(flag&1) goto hex2;           // Output \xHH for everything
        if(flag&2){
          if(!FXISLEADUTF8(c)) goto hex2;               // UTF8 starter?
          if(!FXISFOLLOWUTF8(str[p])) goto hex2;        // UTF8 follower?
          c=(c<<6)^(FXuchar)str[p]^0x3080;
          if(0x800<=c){
            if(!FXISFOLLOWUTF8(str[p+1])) goto hex2;    // UTF8 follower?
            c=(c<<6)^(FXuchar)str[p+1]^0x20080;
            if(0x10000<=c){                             // Surrogate pair needed
              if(!FXISFOLLOWUTF8(str[p+2])) goto hex2;  // UTF8 follower?
              c=(c<<6)^(FXuchar)str[p+2]^0x400080;
              if(0x110000<=c) goto hex1;                // Beyond assigned code space?
              w=LEAD_OFFSET+(c>>10);
              c=TAIL_OFFSET+(c&0x3FF);
              result[q++]='\\';
              result[q++]='u';
              result[q++]=FXString::value2Digit[(w>>12)&15];
              result[q++]=FXString::value2Digit[(w>>8)&15];
              result[q++]=FXString::value2Digit[(w>>4)&15];
              result[q++]=FXString::value2Digit[w&15];
              p++;
              }
            p++;
            }
          p++;
          result[q++]='\\';                             // Escape as \uHHHH
          result[q++]='u';
          result[q++]=FXString::value2Digit[(c>>12)&15];
          result[q++]=FXString::value2Digit[(c>>8)&15];
          result[q++]=FXString::value2Digit[(c>>4)&15];
          result[q++]=FXString::value2Digit[c&15];
          continue;
          }
        }
      result[q++]=c;                    // Normal characters
      continue;
      }
    }
  if(rquote) result[q++]=rquote;        // Closing quote
  FXASSERT(q==result.length());
  return result;
  }

/*******************************************************************************/

// Unescape special characters in a string; optionally strip quote characters
FXString unescape(const FXString& str,FXchar lquote,FXchar rquote){
  register FXint p,q,c,w;
  FXString result;
  p=q=c=0;
  if(str[p]==lquote && lquote) p++;     // Opening quote
  while(p<str.length()){                // Measure length of converted string
    w=c;                                // Keep previous decoded character
    c=str[p++];
    if(c==rquote && rquote) break;      // Closing quote
    if(c=='\\' && p<str.length()){      // Escape sequence
      switch((c=str[p++])){
      case 'u':                         // Unicode escape
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
      case 'x':                         // Hex escape
        if(Ascii::isHexDigit(str[p])){
          p++;
          if(Ascii::isHexDigit(str[p])) p++;
          }
        q++;
        continue;
      case '0':                         // Octal escape
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
      case 'n':                         // Special characters
      case 'r':
      case 'b':
      case 'v':
      case 'a':
      case 'e':                         // Escape
      case 'f':
      case 't':
      case '\\':
      default:                          // Unneccessarily escaped character
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
      case 'u':                         // Unicode escape
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
      case 'x':                         // Hex escape
        if(Ascii::isHexDigit(str[p])){
          c=Ascii::digitValue(str[p++]);
          if(Ascii::isHexDigit(str[p])){
            c=(c<<4)+Ascii::digitValue(str[p++]);
            }
          }
        result[q++]=c;
        continue;
      case '0':                         // Octal escape
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
      case 'n':                         // Special characters
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
      case 'e':                         // Escape
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
      default:                          // Unneccessarily escaped character
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
