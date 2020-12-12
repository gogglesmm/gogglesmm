/********************************************************************************
*                                                                               *
*                           S t r i n g   O b j e c t                           *
*                                                                               *
*********************************************************************************
* Copyright (C) 1997,2020 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "FXException.h"


/*
  Notes:
  - The special pointer-value null represents an empty "" string.

  - Strings are never NULL:- this speeds things up a lot as there is no
    need to check for NULL strings anymore.

  - In the new representation, '\0' is allowed as a character everywhere; but there
    is always an (uncounted) '\0' at the end.

  - The length preceeds the text in the buffer.

  - UTF-8 Encoding scheme:

      Hex Range                 Binary                          Encoding
      -----------------------   -----------------------------   -----------------------------------
      U-00000000 - U-0000007F   0000 0000 0000 0000 0xxx xxxx   0xxxxxxx
      U-00000080 - U-000007FF   0000 0000 0000 0yyy yyxx xxxx   110yyyyy 10xxxxxx
      U-00000800 - U-0000FFFF   0000 0000 zzzz yyyy yyxx xxxx   1110zzzz 10yyyyyy 10xxxxxx
      U-00010000 - U-001FFFFF   000u uuzz zzzz yyyy yyxx xxxx   11110uuu 10zzzzzz 10yyyyyy 10xxxxxx

  - UTF-16 Encoding scheme:

      Hex Range                 Binary                          Encoding
      -----------------------   -----------------------------   -----------------------------------
      U-00000000 - U-0000FFFF   0000 0000 xxxx xxxx xxxx xxxx   xxxxxxxx xxxxxxxx
      U-00010000 - U-0010FFFF   000y yyyy yyyy yyxx xxxx xxxx   110110zz zzzzzzzz 110111xx xxxxxxxx

    The range U-D800 - U-DFFF is reserved for surrogate pairs; Leading-surrogates or high-surrogates
    are from U-D800 to U-DBFF, and trailing-surrogates or low-surrogates are from U-DC00 to U-DFFF.
    Surrogates CH and CL are computed as follows for U > 0x10000:

      CH = (U >> 10) + 0xD800
      CL = (U & 0x3FF) + 0xDC00

*/


// The string buffer is always rounded to a multiple of ROUNDVAL
// which must be 2^n.  Thus, small size changes will not result in any
// actual resizing of the buffer except when ROUNDVAL is exceeded.
#define ROUNDVAL    16

// Round up to nearest ROUNDVAL
#define ROUNDUP(n)  (((n)+ROUNDVAL-1)&-ROUNDVAL)

// Special empty string value
#define EMPTY       ((FXchar*)(void*)(__string__empty__+1))

using namespace FX;

/*******************************************************************************/

namespace FX {

// For conversion from UTF16 to UTF32
const FXint SURROGATE_OFFSET=0x10000-(0xD800<<10)-0xDC00;

// For conversion of UTF32 to UTF16
const FXint LEAD_OFFSET=0xD800-(0x10000>>10);

// For conversion of UTF32 to UTF16
const FXint TAIL_OFFSET=0xDC00;


// Empty string
extern const FXint __string__empty__[];
const FXint __string__empty__[2]={0,0};


// Special NULL string
const FXchar FXString::null[4]={0,0,0,0};


// Hexadecimal digit of value
const FXchar FXString::value2Digit[36]={
  '0','1','2','3','4','5','6','7','8','9','A','B',
  'C','D','E','F','G','H','I','J','K','L','M','N',
  'O','P','Q','R','S','T','U','V','W','X','Y','Z',
  };


// Hexadecimal value of digit
const FXschar FXString::digit2Value[256]={
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
   0, 1, 2, 3, 4, 5, 6, 7, 8, 9,-1,-1,-1,-1,-1,-1,
  -1,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,
  25,26,27,28,29,30,31,32,33,34,35,-1,-1,-1,-1,-1,
  -1,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,
  25,26,27,28,29,30,31,32,33,34,35,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
  };


// Length of a utf8 character representation
const FXschar FXString::utfBytes[256]={
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
  4,4,4,4,4,4,4,4,5,5,5,5,6,6,1,1
  };


// Furnish our own versions
extern FXAPI FXint __vsscanf(const FXchar* string,const FXchar* format,va_list arg_ptr);
extern FXAPI FXint __vsnprintf(FXchar* string,FXint length,const FXchar* format,va_list args);
extern FXAPI FXint __snprintf(FXchar* string,FXint length,const FXchar* format,...);
extern FXAPI FXlong __strtoll(const FXchar *beg,const FXchar** end=NULL,FXint base=0,FXbool* ok=NULL);
extern FXAPI FXulong __strtoull(const FXchar* beg,const FXchar** end=NULL,FXint base=0,FXbool* ok=NULL);
extern FXAPI FXint __strtol(const FXchar *beg,const FXchar** end=NULL,FXint base=0,FXbool* ok=NULL);
extern FXAPI FXuint __strtoul(const FXchar* beg,const FXchar** end=NULL,FXint base=0,FXbool* ok=NULL);
extern FXAPI FXdouble __strtod(const FXchar *beg,const FXchar** end=NULL,FXbool* ok=NULL);
extern FXAPI FXfloat __strtof(const FXchar *beg,const FXchar** end=NULL,FXbool* ok=NULL);

/*******************************************************************************/

// Length of character string
static inline FXint strlen(const FXchar *src){
  return ::strlen(src);
  }


// Length of narrow character string
static inline FXint strlen(const FXnchar *src){
  FXint i=0;
  while(src[i]) i++;
  return i;
  }


// Length of wide character string
static inline FXint strlen(const FXwchar *src){
  FXint i=0;
  while(src[i]) i++;
  return i;
  }

/*******************************************************************************/

// Return wide character from utf8 string at ptr
FXwchar wc(const FXchar* ptr){
  FXwchar w=(FXuchar)ptr[0];
  if(__unlikely(0xC0<=w)){ w=(w<<6)^(FXuchar)ptr[1]^0x3080;
  if(__unlikely(0x800<=w)){ w=(w<<6)^(FXuchar)ptr[2]^0x20080;
  if(__unlikely(0x10000<=w)){ w=(w<<6)^(FXuchar)ptr[3]^0x400080; }}}
  return w;
  }


// Return wide character from utf16 string at ptr
FXwchar wc(const FXnchar* ptr){
  FXwchar w=ptr[0];
  if(__unlikely(FXISLEADUTF16(w))){ w=SURROGATE_OFFSET+(w<<10)+ptr[1]; }
  return w;
  }


// Increment to start of next wide character in utf8 string
const FXchar* wcinc(const FXchar* ptr){
  return (FXISUTF8(*++ptr) || FXISUTF8(*++ptr) || FXISUTF8(*++ptr) || ++ptr), ptr;
  }


// Increment to start of next wide character in utf8 string
FXchar* wcinc(FXchar* ptr){
  return (FXISUTF8(*++ptr) || FXISUTF8(*++ptr) || FXISUTF8(*++ptr) || ++ptr), ptr;
  }


// Increment to start of next wide character in utf16 string
const FXnchar* wcinc(const FXnchar* ptr){
  return (FXISUTF16(*++ptr) || ++ptr), ptr;
  }


// Increment to start of next wide character in utf16 string
FXnchar* wcinc(FXnchar* ptr){
  return (FXISUTF16(*++ptr) || ++ptr), ptr;
  }


// Decrement to start of previous wide character in utf8 string
const FXchar* wcdec(const FXchar* ptr){
  return (FXISUTF8(*--ptr) || FXISUTF8(*--ptr) || FXISUTF8(*--ptr) || --ptr), ptr;
  }

// Decrement to start of previous wide character in utf8 string
FXchar* wcdec(FXchar* ptr){
  return (FXISUTF8(*--ptr) || FXISUTF8(*--ptr) || FXISUTF8(*--ptr) || --ptr), ptr;
  }


// Decrement to start of previous wide character in utf16 string
const FXnchar* wcdec(const FXnchar* ptr){
  return (FXISUTF16(*--ptr) || --ptr), ptr;
  }


// Decrement to start of previous wide character in utf16 string
FXnchar* wcdec(FXnchar* ptr){
  return (FXISUTF16(*--ptr) || --ptr), ptr;
  }


// Return start of utf8 character containing position
const FXchar* wcstart(const FXchar* ptr){
  return (FXISUTF8(*ptr) || FXISUTF8(*--ptr) || FXISUTF8(*--ptr) || --ptr), ptr;
  }


// Adjust ptr to point to leader of multi-byte sequence
FXchar* wcstart(FXchar* ptr){
  return (FXISUTF8(*ptr) || FXISUTF8(*--ptr) || FXISUTF8(*--ptr) || --ptr), ptr;
  }


// Adjust ptr to point to leader of surrogate pair sequence
const FXnchar* wcstart(const FXnchar *ptr){
  return (FXISUTF16(*ptr) || --ptr), ptr;
  }


// Adjust ptr to point to leader of surrogate pair sequence
FXnchar* wcstart(FXnchar *ptr){
  return (FXISUTF16(*ptr) || --ptr), ptr;
  }


// Return number of FXchar's of wide character at ptr
FXival wclen(const FXchar *ptr){
  return FXUTF8LEN(*ptr);
  }


// Return number of FXnchar's of narrow character at ptr
FXival wclen(const FXnchar *ptr){
  return FXUTF16LEN(*ptr);
  }


// Return true if valid utf8 character sequence
FXival wcvalid(const FXchar* ptr){
  if((FXuchar)ptr[0]<0x80) return 1;
  if((FXuchar)ptr[0]<0xC0) return 0;
  if((FXuchar)ptr[1]<0x80) return 0;
  if((FXuchar)ptr[1]>0xBF) return 0;
  if((FXuchar)ptr[0]<0xE0) return 2;
  if((FXuchar)ptr[2]<0x80) return 0;
  if((FXuchar)ptr[2]>0xBF) return 0;
  if((FXuchar)ptr[0]<0xF0) return 3;
  if((FXuchar)ptr[3]<0x80) return 0;
  if((FXuchar)ptr[3]>0xBF) return 0;
  if((FXuchar)ptr[0]>0xF7) return 0;
  return 4;
  }


// Return true if valid utf16 character sequence
FXival wcvalid(const FXnchar* ptr){
  if(ptr[0]<0xD800) return 1;
  if(ptr[0]>0xDFFF) return 1;
  if(ptr[0]>0xDBFF) return 0;
  if(ptr[1]<0xDC00) return 0;
  if(ptr[1]>0xDFFF) return 0;
  return 2;
  }

/*******************************************************************************/

// Return number of bytes for utf8 representation of wide character w
FXival wc2utf(FXwchar w){
  return (w>=0x80)+(w>=0x800)+(w>=0x10000)+1;
  }


// Return number of narrow characters for utf16 representation of wide character w
FXival wc2nc(FXwchar w){
  return (w>=0x10000)+1;
  }


// Return number of bytes for utf8 representation of wide character string
FXival wcs2utf(const FXwchar* src,FXival srclen){
  const FXwchar* srcend=src+srclen;
  FXival p=0;
  FXwchar w;
  while(src<srcend && (w=*src++)!=0){
    p+=wc2utf(w);
    }
  return p;
  }


// Return number of bytes for utf8 representation of wide character string
FXival wcs2utf(const FXwchar *src){
  FXival p=0;
  FXwchar w;
  while((w=*src++)!=0){
    p+=wc2utf(w);
    }
  return p;
  }


// Return number of bytes for utf8 representation of narrow character string
FXival ncs2utf(const FXnchar *src,FXival srclen){
  const FXnchar* srcend=src+srclen;
  FXival p=0;
  FXnchar w;
  while(src<srcend && (w=*src++)!=0){
    if(FXISLEADUTF16(w)){
      if(__unlikely(src>=srcend)) break;
      if(__unlikely(!FXISFOLLOWUTF16(*src))) break;
      w=SURROGATE_OFFSET+(w<<10)+*src++;
      }
    p+=wc2utf(w);
    }
  return p;
  }


// Return number of bytes for utf8 representation of narrow character string
FXival ncs2utf(const FXnchar *src){
  FXival p=0;
  FXnchar w;
  while((w=*src++)!=0){
    if(FXISLEADUTF16(w)){
      if(__unlikely(!FXISFOLLOWUTF16(*src))) break;
      w=SURROGATE_OFFSET+(w<<10)+*src++;
      }
    p+=wc2utf(w);
    }
  return p;
  }


// Return number of wide characters for utf8 character string
FXival utf2wcs(const FXchar *src,FXival srclen){
  const FXchar* srcend=src+srclen;
  FXival p=0;
  FXuchar c;
  while(src<srcend && (c=src[0])!=0){
    if(0xC0<=c){
      if(__unlikely(src+1>=srcend)) break;
      if(__unlikely(!FXISFOLLOWUTF8(src[1]))) break;
      if(0xE0<=c){
        if(__unlikely(src+2>=srcend)) break;
        if(__unlikely(!FXISFOLLOWUTF8(src[2]))) break;
        if(0xF0<=c){
          if(__unlikely(src+3>=srcend)) break;
          if(__unlikely(!FXISFOLLOWUTF8(src[3]))) break;
          src++;
          }
        src++;
        }
      src++;
      }
    src++;
    p++;
    }
  return p;
  }


// Return number of wide characters for utf8 character string
FXival utf2wcs(const FXchar *src){
  FXival p=0;
  FXuchar c;
  while((c=src[0])!=0){
    if(0xC0<=c){
      if(__unlikely(!FXISFOLLOWUTF8(src[1]))) break;
      if(0xE0<=c){
        if(__unlikely(!FXISFOLLOWUTF8(src[2]))) break;
        if(0xF0<=c){
          if(__unlikely(!FXISFOLLOWUTF8(src[3]))) break;
          src++;
          }
        src++;
        }
      src++;
      }
    src++;
    p++;
    }
  return p;
  }


// Return number of narrow characters for utf8 character string
FXival utf2ncs(const FXchar *src,FXival len){
  const FXchar* end=src+len;
  FXival p=0;
  FXuchar c;
  while(src<end && (c=src[0])!=0){
    if(0xC0<=c){
      if(__unlikely(src+1>=end)) break;
      if(__unlikely(!FXISFOLLOWUTF8(src[1]))) break;
      if(0xE0<=c){
        if(__unlikely(src+2>=end)) break;
        if(__unlikely(!FXISFOLLOWUTF8(src[2]))) break;
        if(0xF0<=c){
        if(__unlikely(src+3>=end)) break;
          if(__unlikely(!FXISFOLLOWUTF8(src[3]))) break;
          src++;
          p++;
          }
        src++;
        }
      src++;
      }
    src++;
    p++;
    }
  return p;
  }


// Return number of narrow characters for utf8 character string
FXival utf2ncs(const FXchar *src){
  FXival p=0;
  FXuchar c;
  while((c=src[0])!=0){
    if(0xC0<=c){
      if(__unlikely(!FXISFOLLOWUTF8(src[1]))) break;
      if(0xE0<=c){
        if(__unlikely(!FXISFOLLOWUTF8(src[2]))) break;
        if(0xF0<=c){
          if(__unlikely(!FXISFOLLOWUTF8(src[3]))) break;
          src++;
          p++;
          }
        src++;
        }
      src++;
      }
    src++;
    p++;
    }
  return p;
  }


// Convert wide character to utf8 string
FXival wc2utf(FXchar *dst,FXwchar w){
  if(__likely(w<0x80)){
    dst[0]=w;
    return 1;
    }
  if(__likely(w<0x800)){
    dst[0]=(w>>6)|0xC0;
    dst[1]=(w&0x3F)|0x80;
    return 2;
    }
  if(__likely(w<0x10000)){
    dst[0]=(w>>12)|0xE0;
    dst[1]=((w>>6)&0x3F)|0x80;
    dst[2]=(w&0x3F)|0x80;
    return 3;
    }
  dst[0]=(w>>18)|0xF0;
  dst[1]=((w>>12)&0x3F)|0x80;
  dst[2]=((w>>6)&0x3F)|0x80;
  dst[3]=(w&0x3F)|0x80;
  return 4;
  }


// Convert wide character to narrow character string
FXival wc2nc(FXnchar *dst,FXwchar w){
  if(__likely(w<0x10000)){
    dst[0]=w;
    return 1;
    }
  dst[0]=LEAD_OFFSET+(w>>10);
  dst[1]=TAIL_OFFSET+(w&0x3FF);
  return 2;
  }


// Convert wide character string to utf8 string
FXival wcs2utf(FXchar *dst,const FXwchar* src,FXival dstlen,FXival srclen){
  const FXwchar* srcend=src+srclen;
  FXchar* ptrend=dst+dstlen;
  FXchar* ptr=dst;
  FXwchar w;
  while(src<srcend && (w=*src++)!=0){
    if(w<0x80){
      if(__unlikely(ptr>=ptrend)) break;
      *ptr++=w;
      continue;
      }
    if(w<0x800){
      if(__unlikely(ptr+1>=ptrend)) break;
      *ptr++=(w>>6)|0xC0;
      *ptr++=(w&0x3F)|0x80;
      continue;
      }
    if(w<0x10000){
      if(__unlikely(ptr+2>=ptrend)) break;
      *ptr++=(w>>12)|0xE0;
      *ptr++=((w>>6)&0x3F)|0x80;
      *ptr++=(w&0x3F)|0x80;
      continue;
      }
    if(w<0x110000){
      if(__unlikely(ptr+3>=ptrend)) break;
      *ptr++=(w>>18)|0xF0;
      *ptr++=((w>>12)&0x3F)|0x80;
      *ptr++=((w>>6)&0x3F)|0x80;
      *ptr++=(w&0x3F)|0x80;
      continue;
      }
    break;
    }
  if(ptr<ptrend){
    *ptr='\0';
    }
  return ptr-dst;
  }


// Convert wide character string to utf8 string
FXival wcs2utf(FXchar *dst,const FXwchar* src,FXival dstlen){
  FXchar* ptrend=dst+dstlen;
  FXchar* ptr=dst;
  FXwchar w;
  while((w=*src++)!=0){
    if(w<0x80){
      if(__unlikely(ptr>=ptrend)) break;
      *ptr++=w;
      continue;
      }
    if(w<0x800){
      if(__unlikely(ptr+1>=ptrend)) break;
      *ptr++=(w>>6)|0xC0;
      *ptr++=(w&0x3F)|0x80;
      continue;
      }
    if(w<0x10000){
      if(__unlikely(ptr+2>=ptrend)) break;
      *ptr++=(w>>12)|0xE0;
      *ptr++=((w>>6)&0x3F)|0x80;
      *ptr++=(w&0x3F)|0x80;
      continue;
      }
    if(w<0x110000){
      if(__unlikely(ptr+3>=ptrend)) break;
      *ptr++=(w>>18)|0xF0;
      *ptr++=((w>>12)&0x3F)|0x80;
      *ptr++=((w>>6)&0x3F)|0x80;
      *ptr++=(w&0x3F)|0x80;
      continue;
      }
    break;
    }
  if(ptr<ptrend){
    *ptr='\0';
    }
  return ptr-dst;
  }


// Convert narrow character string to utf8 string
FXival ncs2utf(FXchar *dst,const FXnchar* src,FXival dstlen,FXival srclen){
  const FXnchar* srcend=src+srclen;
  FXchar* ptrend=dst+dstlen;
  FXchar* ptr=dst;
  FXwchar w;
  while(src<srcend && (w=*src++)!=0){
    if(w<0x80){
      if(__unlikely(ptr>=ptrend)) break;
      *ptr++=w;
      continue;
      }
    if(w<0x800){
      if(__unlikely(ptr+1>=ptrend)) break;
      *ptr++=(w>>6)|0xC0;
      *ptr++=(w&0x3F)|0x80;
      continue;
      }
    if(!FXISLEADUTF16(w)){
      if(__unlikely(ptr+2>=ptrend)) break;
      *ptr++=(w>>12)|0xE0;
      *ptr++=((w>>6)&0x3F)|0x80;
      *ptr++=(w&0x3F)|0x80;
      continue;
      }
    if(src<srcend && FXISFOLLOWUTF16(*src)){
      if(__unlikely(ptr+3>=ptrend)) break;
      w=SURROGATE_OFFSET+(w<<10)+*src++;
      *ptr++=(w>>18)|0xF0;
      *ptr++=((w>>12)&0x3F)|0x80;
      *ptr++=((w>>6)&0x3F)|0x80;
      *ptr++=(w&0x3F)|0x80;
      continue;
      }
    break;
    }
  if(ptr<ptrend){
    *ptr='\0';
    }
  return ptr-dst;
  }


// Convert narrow character string to utf8 string
FXival ncs2utf(FXchar *dst,const FXnchar* src,FXival dstlen){
  FXchar* ptrend=dst+dstlen;
  FXchar* ptr=dst;
  FXwchar w;
  while((w=*src++)!=0){
    if(w<0x80){
      if(__unlikely(ptr>=ptrend)) break;
      *ptr++=w;
      continue;
      }
    if(w<0x800){
      if(__unlikely(ptr+1>=ptrend)) break;
      *ptr++=(w>>6)|0xC0;
      *ptr++=(w&0x3F)|0x80;
      continue;
      }
    if(!FXISSEQUTF16(w)){
      if(__unlikely(ptr+2>=ptrend)) break;
      *ptr++=(w>>12)|0xE0;
      *ptr++=((w>>6)&0x3F)|0x80;
      *ptr++=(w&0x3F)|0x80;
      continue;
      }
    if(FXISFOLLOWUTF16(*src)){
      if(__unlikely(ptr+3>=ptrend)) break;
      w=SURROGATE_OFFSET+(w<<10)+*src++;
      *ptr++=(w>>18)|0xF0;
      *ptr++=((w>>12)&0x3F)|0x80;
      *ptr++=((w>>6)&0x3F)|0x80;
      *ptr++=(w&0x3F)|0x80;
      continue;
      }
    break;
    }
  if(ptr<ptrend){
    *ptr='\0';
    }
  return ptr-dst;
  }


// Convert utf8 string to wide character string
FXival utf2wcs(FXwchar *dst,const FXchar* src,FXival dstlen,FXival srclen){
  const FXchar* srcend=src+srclen;
  FXwchar* ptrend=dst+dstlen;
  FXwchar* ptr=dst;
  FXwchar w;
  FXuchar c;
  while(src<srcend && (w=c=*src++)!=0){
    if(0xC0<=w){
      if(__unlikely(src>=srcend)) break;
      c=*src++;
      if(__unlikely(!FXISFOLLOWUTF8(c))) break;
      w=(w<<6) ^ c ^ 0x3080;
      if(0x800<=w){
        if(__unlikely(src>=srcend)) break;
        c=*src++;
        if(__unlikely(!FXISFOLLOWUTF8(c))) break;
        w=(w<<6) ^ c ^ 0x20080;
        if(0x10000<=w){
          if(__unlikely(src>=srcend)) break;
          c=*src++;
          if(__unlikely(!FXISFOLLOWUTF8(c))) break;
          w=(w<<6) ^ c ^ 0x400080;
          }
        }
      }
    if(__unlikely(ptr>=ptrend)) break;
    *ptr++=w;
    }
  if(ptr<ptrend){
    *ptr=0;
    }
  return ptr-dst;
  }


// Convert utf8 string to wide character string
FXival utf2wcs(FXwchar *dst,const FXchar* src,FXival dstlen){
  FXwchar* ptrend=dst+dstlen;
  FXwchar* ptr=dst;
  FXwchar w;
  FXuchar c;
  while((w=c=*src++)!=0){
    if(0xC0<=c){
      c=*src++;
      if(__unlikely(!FXISFOLLOWUTF8(c))) break;
      w=(w<<6) ^ c ^ 0x3080;
      if(0x800<=w){
        c=*src++;
        if(__unlikely(!FXISFOLLOWUTF8(c))) break;
        w=(w<<6) ^ c ^ 0x20080;
        if(0x10000<=w){
          c=*src++;
          if(__unlikely(!FXISFOLLOWUTF8(c))) break;
          w=(w<<6) ^ c ^ 0x400080;
          }
        }
      }
    if(__unlikely(ptr>=ptrend)) break;
    *ptr++=w;
    }
  if(ptr<ptrend){
    *ptr=0;
    }
  return ptr-dst;
  }


// Convert utf8 string to narrow character string
FXival utf2ncs(FXnchar *dst,const FXchar* src,FXival dstlen,FXival srclen){
  const FXchar* srcend=src+srclen;
  FXnchar* ptrend=dst+dstlen;
  FXnchar* ptr=dst;
  FXwchar w;
  FXuchar c;
  while(src<srcend && (w=c=*src++)!=0){
    if(0xC0<=w){
      if(__unlikely(src>=srcend)) break;
      c=*src++;
      if(__unlikely(!FXISFOLLOWUTF8(c))) break;
      w=(w<<6) ^ c ^ 0x3080;
      if(0x800<=w){
        if(__unlikely(src>=srcend)) break;
        c=*src++;
        if(__unlikely(!FXISFOLLOWUTF8(c))) break;
        w=(w<<6) ^ c ^ 0x20080;
        if(0x10000<=w){
          if(__unlikely(src>=srcend)) break;
          c=*src++;
          if(__unlikely(!FXISFOLLOWUTF8(c))) break;
          w=(w<<6) ^ c ^ 0x400080;
          if(__unlikely(ptr+1>=ptrend)) break;
          *ptr++=LEAD_OFFSET+(w>>10);
          *ptr++=TAIL_OFFSET+(w&0x3FF);
          continue;
          }
        }
      }
    if(__unlikely(ptr>=ptrend)) break;
    *ptr++=w;
    }
  if(ptr<ptrend){
    *ptr=0;
    }
  return ptr-dst;
  }


// Convert utf8 string to narrow character string
FXival utf2ncs(FXnchar *dst,const FXchar* src,FXival dstlen){
  FXnchar* ptrend=dst+dstlen;
  FXnchar* ptr=dst;
  FXwchar w;
  FXuchar c;
  while((w=c=*src++)!=0){
    if(0xC0<=w){
      c=*src++;
      if(__unlikely(!FXISFOLLOWUTF8(c))) break;
      w=(w<<6) ^ c ^ 0x3080;
      if(0x800<=w){
        c=*src++;
        if(__unlikely(!FXISFOLLOWUTF8(c))) break;
        w=(w<<6) ^ c ^ 0x20080;
        if(0x10000<=w){
          c=*src++;
          if(__unlikely(!FXISFOLLOWUTF8(c))) break;
          w=(w<<6) ^ c ^ 0x400080;
          if(__unlikely(ptr+1>=ptrend)) break;
          *ptr++=LEAD_OFFSET+(w>>10);
          *ptr++=TAIL_OFFSET+(w&0x3FF);
          continue;
          }
        }
      }
    if(__unlikely(ptr>=ptrend)) break;
    *ptr++=w;
    }
  if(ptr<ptrend){
    *ptr=0;
    }
  return ptr-dst;
  }

/*******************************************************************************/

// Change the length of the string to len
void FXString::length(FXint len){
  if(__likely(len!=length())){
    FXchar *ptr;
    if(0<len){
      if(str==EMPTY){
        ptr=(FXchar*)::malloc(ROUNDUP(1+len)+sizeof(FXint));
        }
      else{
        ptr=(FXchar*)::realloc(str-sizeof(FXint),ROUNDUP(1+len)+sizeof(FXint));
        }
      if(__unlikely(!ptr)){
        throw FXMemoryException("FXString::length: out of memory\n");
        }
      str=ptr+sizeof(FXint);
      str[len]=0;
      *(((FXint*)str)-1)=len;
      }
    else if(str!=EMPTY){
      ::free(str-sizeof(FXint));
      str=EMPTY;
      }
    }
  }


// Initialize to empty
FXString::FXString():str(EMPTY){
  }


// Construct copy of another string
FXString::FXString(const FXString& s):str(EMPTY){
  FXint n=s.length();
  if(0<n){
    length(n);
    memcpy(str,s.str,n);
    }
  }


// Construct and initialize with string s
FXString::FXString(const FXchar* s):str(EMPTY){
  FXint m;
  if(__likely(s && 0<(m=strlen(s)))){
    length(m);
    memcpy(str,s,m);
    }
  }


// Construct and init from narrow character string
FXString::FXString(const FXnchar* s):str(EMPTY){
  FXint m;
  if(__likely(s && 0<(m=ncs2utf(s)))){
    length(m);
    ncs2utf(str,s,m);
    }
  }


// Construct and init from wide character string
FXString::FXString(const FXwchar* s):str(EMPTY){
  FXint m;
  if(__likely(s && 0<(m=wcs2utf(s)))){
    length(m);
    wcs2utf(str,s,m);
    }
  }


// Construct and init with substring
FXString::FXString(const FXchar* s,FXint n):str(EMPTY){
  if(__likely(s && 0<n)){
    length(n);
    memcpy(str,s,n);
    }
  }


// Construct and init with narrow character substring
FXString::FXString(const FXnchar* s,FXint n):str(EMPTY){
  FXint m;
  if(__likely(s && 0<(m=ncs2utf(s,n)))){
    length(m);
    ncs2utf(str,s,m,n);
    }
  }


// Construct and init with wide character substring
FXString::FXString(const FXwchar* s,FXint n):str(EMPTY){
  FXint m;
  if(__likely(s && 0<(m=wcs2utf(s,n)))){
    length(m);
    wcs2utf(str,s,m,n);
    }
  }


// Construct and fill with constant
FXString::FXString(FXchar c,FXint n):str(EMPTY){
  if(__likely(0<n)){
    length(n);
    memset(str,c,n);
    }
  }


// Destructor
FXString::~FXString(){
  if(str!=EMPTY){::free(str-sizeof(FXint));}
  }


// Return wide character starting at offset i
FXwchar FXString::wc(FXint p) const {
  FXwchar w=(FXuchar)str[p];
  if(__unlikely(0xC0<=w)){ w=(w<<6)^(FXuchar)str[p+1]^0x3080;
  if(__unlikely(0x800<=w)){ w=(w<<6)^(FXuchar)str[p+2]^0x20080;
  if(__unlikely(0x10000<=w)){ w=(w<<6)^(FXuchar)str[p+3]^0x400080; }}}
  return w;
  }


// Increment byte offset by one utf8 character
FXint FXString::inc(FXint p) const {
  return (++p>=length() || FXISUTF8(str[p]) || ++p>=length() || FXISUTF8(str[p]) || ++p>=length() || FXISUTF8(str[p]) || ++p), p;
  }


// Increment byte offset by n utf8 characters
FXint FXString::inc(FXint p,FXint n) const {
  while(p<length() && 0<n){ p=inc(p); --n; }
  return p;
  }


// Decrement byte offset by one utf8 character
FXint FXString::dec(FXint p) const {
  return (--p<=0 || FXISUTF8(str[p]) || --p<=0 || FXISUTF8(str[p]) || --p<=0 || FXISUTF8(str[p]) || --p), p;
  }


// Decrement byte offset by n utf8 characters
FXint FXString::dec(FXint p,FXint n) const {
  while(0<=p && 0<n){ p=dec(p); --n; }
  return p;
  }


// Count number of utf8 characters in subrange start...end
FXint FXString::count(FXint start,FXint end) const {
  FXint cnt=0;
  while(start<end){
    start+=FXUTF8LEN(str[start]);
    cnt++;
    }
  return cnt;
  }


// Count number of utf8 characters
FXint FXString::count() const {
  return count(0,length());
  }


// Return byte offset of utf8 character at index
FXint FXString::offset(FXint indx) const {
  FXint len=length();
  FXint i=0;
  FXint p=0;
  while(i<indx && p<len){
    p+=FXUTF8LEN(str[p]);
    i++;
    }
  return p;
  }


// Return index of utf8 character at byte offset
FXint FXString::index(FXint offs) const {
   FXint len=length();
   FXint i=0;
   FXint p=0;
  while(p<offs && p<len){
    p+=FXUTF8LEN(str[p]);
    i++;
    }
  return i;
  }


// Return start of utf8 character containing position
FXint FXString::validate(FXint p) const {
  return (p<=0 || FXISUTF8(str[p]) || --p<=0 || FXISUTF8(str[p]) || --p<=0 || FXISUTF8(str[p]) || --p), p;
  }


// Assign a constant string to this string
FXString& FXString::operator=(const FXchar* s){
  return assign(s);
  }


// Assign a narrow character string to this string
FXString& FXString::operator=(const FXnchar* s){
  return assign(s);
  }


// Assign a wide character string to this string
FXString& FXString::operator=(const FXwchar* s){
  return assign(s);
  }


// Assign another string to this string
FXString& FXString::operator=(const FXString& s){
  if(__likely(str!=s.str)){ assign(s.str,s.length()); }
  return *this;
  }


// Append single character to this string
FXString& FXString::operator+=(FXchar c){
  return append(c);
  }


// Append constant string after this string
FXString& FXString::operator+=(const FXchar* s){
  return append(s);
  }


// Append narrow character string after this string
FXString& FXString::operator+=(const FXnchar* s){
  return append(s);
  }


// Append wide characeter string after this string
FXString& FXString::operator+=(const FXwchar* s){
  return append(s);
  }


// Append another string after this string
FXString& FXString::operator+=(const FXString& s){
  return append(s);
  }


// Adopt string s, leaving s empty
FXString& FXString::adopt(FXString& s){
  if(__likely(str!=s.str)){
    swap(str,s.str);
    s.clear();
    }
  return *this;
  }


// Assign input character to this string
FXString& FXString::assign(FXchar c){
  length(1);
  str[0]=c;
  return *this;
  }


// Assign input n characters c to this string
FXString& FXString::assign(FXchar c,FXint n){
  length(n);
  memset(str,c,n);
  return *this;
  }


// Assign input string to this string
FXString& FXString::assign(const FXchar* s){
  FXint m;
  if(__likely(s && 0<(m=strlen(s)))){
    length(m);
    memmove(str,s,m);
    }
  else{
    length(0);
    }
  return *this;
  }


// Assign narrow character string s to this string
FXString& FXString::assign(const FXnchar* s){
  FXint m;
  if(__likely(s && 0<(m=ncs2utf(s)))){
    length(m);
    ncs2utf(str,s,m);
    }
  else{
    length(0);
    }
  return *this;
  }


// Assign wide character string s to this string
FXString& FXString::assign(const FXwchar* s){
  FXint m;
  if(__likely(s && 0<(m=wcs2utf(s)))){
    length(m);
    wcs2utf(str,s,m);
    }
  else{
    length(0);
    }
  return *this;
  }


// Assign first n characters of input string to this string
FXString& FXString::assign(const FXchar* s,FXint n){
  if(__likely(s && 0<n)){
    length(n);
    memmove(str,s,n);
    }
  else{
    length(0);
    }
  return *this;
  }


// Assign first n characters of narrow character string s to this string
FXString& FXString::assign(const FXnchar* s,FXint n){
  FXint m;
  if(__likely(s && 0<(m=ncs2utf(s,n)))){
    length(m);
    ncs2utf(str,s,m,n);
    }
  else{
    length(0);
    }
  return *this;
  }


// Assign first n characters of wide character string s to this string
FXString& FXString::assign(const FXwchar* s,FXint n){
  FXint m;
  if(__likely(s && 0<(m=wcs2utf(s,n)))){
    length(m);
    wcs2utf(str,s,m,n);
    }
  else{
    length(0);
    }
  return *this;
  }


// Assign input string to this string
FXString& FXString::assign(const FXString& s){
  if(__likely(str!=s.str)){ assign(s.str,s.length()); }
  return *this;
  }


// Insert character at position
FXString& FXString::insert(FXint pos,FXchar c){
  FXint len=length();
  length(len+1);
  if(pos<=0){
    memmove(str+1,str,len);
    str[0]=c;
    }
  else if(pos>=len){
    str[len]=c;
    }
  else{
    memmove(str+pos+1,str+pos,len-pos);
    str[pos]=c;
    }
  return *this;
  }


// Insert n characters c at specified position
FXString& FXString::insert(FXint pos,FXchar c,FXint n){
  if(__likely(0<n)){
    FXint len=length();
    length(len+n);
    if(pos<=0){
      memmove(str+n,str,len);
      memset(str,c,n);
      }
    else if(pos>=len){
      memset(str+len,c,n);
      }
    else{
      memmove(str+pos+n,str+pos,len-pos);
      memset(str+pos,c,n);
      }
    }
  return *this;
  }


// Insert string at position
FXString& FXString::insert(FXint pos,const FXchar* s){
  FXint m;
  if(__likely(s && 0<(m=strlen(s)))){
    FXint len=length();
    length(len+m);
    if(pos<=0){
      memmove(str+m,str,len);
      memcpy(str,s,m);
      }
    else if(pos>=len){
      memcpy(str+len,s,m);
      }
    else{
      memmove(str+pos+m,str+pos,len-pos);
      memcpy(str+pos,s,m);
      }
    }
  return *this;
  }


// Insert narrow character string at position
FXString& FXString::insert(FXint pos,const FXnchar* s){
  FXint m;
  if(__likely(s && 0<(m=ncs2utf(s)))){
    FXint len=length();
    length(len+m);
    if(pos<=0){
      memmove(str+m,str,len);
      ncs2utf(str,s,m);
      }
    else if(pos>=len){
      ncs2utf(str+len,s,m);
      }
    else{
      memmove(str+pos+m,str+pos,len-pos);
      ncs2utf(str+pos,s,m);
      }
    }
  return *this;
  }


// Insert wide character string at position
FXString& FXString::insert(FXint pos,const FXwchar* s){
  FXint m;
  if(__likely(s && 0<(m=wcs2utf(s)))){
    FXint len=length();
    length(len+m);
    if(pos<=0){
      memmove(str+m,str,len);
      wcs2utf(str,s,m);
      }
    else if(pos>=len){
      wcs2utf(str+len,s,m);
      }
    else{
      memmove(str+pos+m,str+pos,len-pos);
      wcs2utf(str+pos,s,m);
      }
    }
  return *this;
  }



// Insert string at position
FXString& FXString::insert(FXint pos,const FXchar* s,FXint n){
  if(__likely(s && 0<n)){
    FXint len=length();
    length(len+n);
    if(pos<=0){
      memmove(str+n,str,len);
      memcpy(str,s,n);
      }
    else if(pos>=len){
      memcpy(str+len,s,n);
      }
    else{
      memmove(str+pos+n,str+pos,len-pos);
      memcpy(str+pos,s,n);
      }
    }
  return *this;
  }


// Insert narrow character string at position
FXString& FXString::insert(FXint pos,const FXnchar* s,FXint n){
  FXint m;
  if(__likely(s && 0<(m=ncs2utf(s,n)))){
    FXint len=length();
    length(len+m);
    if(pos<=0){
      memmove(str+m,str,len);
      ncs2utf(str,s,m,n);
      }
    else if(pos>=len){
      ncs2utf(str+len,s,m,n);
      }
    else{
      memmove(str+pos+m,str+pos,len-pos);
      ncs2utf(str+pos,s,m,n);
      }
    }
  return *this;
  }


// Insert wide character string at position
FXString& FXString::insert(FXint pos,const FXwchar* s,FXint n){
  FXint m;
  if(__likely(s && 0<(m=wcs2utf(s,n)))){
    FXint len=length();
    length(len+m);
    if(pos<=0){
      memmove(str+m,str,len);
      wcs2utf(str,s,m,n);
      }
    else if(pos>=len){
      wcs2utf(str+len,s,m,n);
      }
    else{
      memmove(str+pos+m,str+pos,len-pos);
      wcs2utf(str+pos,s,m,n);
      }
    }
  return *this;
  }


// Insert string at position
FXString& FXString::insert(FXint pos,const FXString& s){
  return insert(pos,s.str,s.length());
  }


// Prepend character
FXString& FXString::prepend(FXchar c){
  FXint len=length();
  length(len+1);
  memmove(str+1,str,len);
  str[0]=c;
  return *this;
  }


// Prepend string with n characters c
FXString& FXString::prepend(FXchar c,FXint n){
  if(__likely(0<n)){
    FXint len=length();
    length(len+n);
    memmove(str+n,str,len);
    memset(str,c,n);
    }
  return *this;
  }


// Prepend string
FXString& FXString::prepend(const FXchar* s){
  FXint m;
  if(__likely(s && 0<(m=strlen(s)))){
    FXint len=length();
    length(len+m);
    memmove(str+m,str,len);
    memcpy(str,s,m);
    }
  return *this;
  }


// Prepend narrow character string
FXString& FXString::prepend(const FXnchar* s){
  FXint m;
  if(__likely(s && 0<(m=ncs2utf(s)))){
    FXint len=length();
    length(len+m);
    memmove(str+m,str,len);
    ncs2utf(str,s,m);
    }
  return *this;
  }


// Prepend wide character string
FXString& FXString::prepend(const FXwchar* s){
  FXint m;
  if(__likely(s && 0<(m=wcs2utf(s)))){
    FXint len=length();
    length(len+m);
    memmove(str+m,str,len);
    wcs2utf(str,s,m);
    }
  return *this;
  }


// Prepend string
FXString& FXString::prepend(const FXchar* s,FXint n){
  if(__likely(s && 0<n)){
    FXint len=length();
    length(len+n);
    memmove(str+n,str,len);
    memcpy(str,s,n);
    }
  return *this;
  }


// Prepend narrow character string
FXString& FXString::prepend(const FXnchar* s,FXint n){
  FXint m;
  if(__likely(s && 0<(m=ncs2utf(s,n)))){
    FXint len=length();
    length(len+m);
    memmove(str+m,str,len);
    ncs2utf(str,s,m,n);
    }
  return *this;
  }


// Prepend wide character string
FXString& FXString::prepend(const FXwchar* s,FXint n){
  FXint m;
  if(__likely(s && 0<(m=wcs2utf(s,n)))){
    FXint len=length();
    length(len+m);
    memmove(str+m,str,len);
    wcs2utf(str,s,m,n);
    }
  return *this;
  }


// Prepend string
FXString& FXString::prepend(const FXString& s){
  return prepend(s.str,s.length());
  }


// Append character c to this string
FXString& FXString::append(FXchar c){
  FXint len=length();
  length(len+1);
  str[len]=c;
  return *this;
  }


// Append n characters c to this string
FXString& FXString::append(FXchar c,FXint n){
  if(__likely(0<n)){
    FXint len=length();
    length(len+n);
    memset(str+len,c,n);
    }
  return *this;
  }


// Append string to this string
FXString& FXString::append(const FXchar* s,FXint n){
  if(__likely(s && 0<n)){
    FXint len=length();
    length(len+n);
    memcpy(str+len,s,n);
    }
  return *this;
  }


// Append string to this string
FXString& FXString::append(const FXnchar* s,FXint n){
  FXint m;
  if(__likely(s && 0<(m=ncs2utf(s,n)))){
    FXint len=length();
    length(len+m);
    ncs2utf(str+len,s,m,n);
    }
  return *this;
  }


// Append string to this string
FXString& FXString::append(const FXwchar* s,FXint n){
  FXint m;
  if(__likely(s && 0<(m=wcs2utf(s,n)))){
    FXint len=length();
    length(len+m);
    wcs2utf(str+len,s,m,n);
    }
  return *this;
  }


// Append string to this string
FXString& FXString::append(const FXchar* s){
  FXint m;
  if(__likely(s && 0<(m=strlen(s)))){
    FXint len=length();
    length(len+m);
    memcpy(str+len,s,m);
    }
  return *this;
  }


// Append string to this string
FXString& FXString::append(const FXnchar* s){
  FXint m;
  if(__likely(s && 0<(m=ncs2utf(s)))){
    FXint len=length();
    length(len+m);
    ncs2utf(str+len,s,m);
    }
  return *this;
  }


// Append string to this string
FXString& FXString::append(const FXwchar* s){
  FXint m;
  if(__likely(s && 0<(m=wcs2utf(s)))){
    FXint len=length();
    length(len+m);
    wcs2utf(str+len,s,m);
    }
  return *this;
  }


// Append string to this string
FXString& FXString::append(const FXString& s){
  return append(s.str,s.length());
  }


// Replace character in string
FXString& FXString::replace(FXint pos,FXchar c){
  str[pos]=c;
  return *this;
  }


// Replace the r characters at pos with n characters c
FXString& FXString::replace(FXint pos,FXint r,FXchar c,FXint n){
  FXint len=length();
  if(r<n){
    length(len+n-r);
    memmove(str+pos+n,str+pos+r,len-pos-r);
    }
  else if(r>n){
    memmove(str+pos+n,str+pos+r,len-pos-r);
    length(len+n-r);
    }
  memset(str+pos,c,n);
  return *this;
  }


// Replace the r characters at pos with string s
FXString& FXString::replace(FXint pos,FXint r,const FXchar* s){
  return replace(pos,r,s,strlen(s));
  }


// Replace the r characters at pos with narrow character string s
FXString& FXString::replace(FXint pos,FXint r,const FXnchar* s){
  return replace(pos,r,s,strlen(s));
  }


// Replace the r characters at pos with wide character string s
FXString& FXString::replace(FXint pos,FXint r,const FXwchar* s){
  return replace(pos,r,s,strlen(s));
  }


// Replaces the r characters at pos with first n characters of string s
FXString& FXString::replace(FXint pos,FXint r,const FXchar* s,FXint n){
  FXint len=length();
  if(r<n){
    length(len+n-r);
    memmove(str+pos+n,str+pos+r,len-pos-r);
    }
  else if(r>n){
    memmove(str+pos+n,str+pos+r,len-pos-r);
    length(len+n-r);
    }
  memcpy(str+pos,s,n);
  return *this;
  }


// Replaces the r characters at pos with first n characters of narrow character string s
FXString& FXString::replace(FXint pos,FXint r,const FXnchar* s,FXint n){
  FXint m=ncs2utf(s,n);
  FXint len=length();
  if(r<m){
    length(len+m-r);
    memmove(str+pos+m,str+pos+r,len-pos-r);
    }
  else if(r>m){
    memmove(str+pos+m,str+pos+r,len-pos-r);
    length(len+m-r);
    }
  ncs2utf(str+pos,s,m,n);
  return *this;
  }


// Replaces the r characters at pos with first n characters of wide character string s
FXString& FXString::replace(FXint pos,FXint r,const FXwchar* s,FXint n){
  FXint m=wcs2utf(s,n);
  FXint len=length();
  if(r<m){
    length(len+m-r);
    memmove(str+pos+m,str+pos+r,len-pos-r);
    }
  else if(r>m){
    memmove(str+pos+m,str+pos+r,len-pos-r);
    length(len+m-r);
    }
  wcs2utf(str+pos,s,m,n);
  return *this;
  }


// Replace part of string
FXString& FXString::replace(FXint pos,FXint r,const FXString& s){
  return replace(pos,r,s.str,s.length());
  }


// Move range of m characters from src position to dst position
FXString& FXString::move(FXint dst,FXint src,FXint n){
  FXint len=length();
  if(__likely(0<n && 0<=src && src+n<=len)){
    if(dst<0){                                  // Move below begin
      if(dst<-n) dst=-n;
      length(len-dst);
      memmove(str-dst,str,len);
      memmove(str,str-dst+src,n);
      }
    else if(dst+n>len){                         // Move beyond end
      if(dst>len) dst=len;
      length(dst+n);
      memmove(str+dst,str+src,n);
      }
    else{
      memmove(str+dst,str+src,n);               // Move inside
      }
    }
  return *this;
  }


// Remove one character
FXString& FXString::erase(FXint pos){
  FXint len=length();
  if(__likely(0<=pos && pos<len)){
    memmove(str+pos,str+pos+1,len-pos-1);
    length(len-1);
    }
  return *this;
  }


// Remove section from buffer
FXString& FXString::erase(FXint pos,FXint n){
  FXint len=length();
  if(__likely(0<n && 0<pos+n && pos<len)){
    if(pos<0){n+=pos;pos=0;}
    if(len<pos+n){n=len-pos;}
    memmove(str+pos,str+pos+n,len-pos-n);
    length(len-n);
    }
  return *this;
  }


// Truncate string
FXString& FXString::trunc(FXint pos){
  length(FXMIN(pos,length()));
  return *this;
  }


// Clean string
FXString& FXString::clear(){
  length(0);
  return *this;
  }


// Return number of occurrences of ch in string
FXint FXString::contains(FXchar ch) const {
  FXint len=length();
  FXint c=ch;
  FXint m=0;
  FXint i=0;
  while(i<len){
    if(str[i]==c){
      m++;
      }
    i++;
    }
  return m;
  }


// Return number of occurrences of string sub in string
FXint FXString::contains(const FXchar* sub,FXint n) const {
  FXint len=length()-n;
  FXint m=0;
  FXint i=0;
  while(i<=len){
    if(compare(str+i,sub,n)==0){
      m++;
      }
    i++;
    }
  return m;
  }


// Return number of occurrences of string sub in string
FXint FXString::contains(const FXchar* sub) const {
  return contains(sub,strlen(sub));
  }


// Return number of occurrences of string sub in string
FXint FXString::contains(const FXString& sub) const {
  return contains(sub.text(),sub.length());
  }


// Substitute one character by another
FXString& FXString::substitute(FXchar org,FXchar sub,FXbool all){
  FXint len=length();
  FXint c=org;
  FXint s=sub;
  FXint i=0;
  while(i<len){
    if(str[i]==c){
      str[i]=s;
      if(!all) break;
      }
    i++;
    }
  return *this;
  }


// Substitute one string by another
FXString& FXString::substitute(const FXchar* org,FXint olen,const FXchar* rep,FXint rlen,FXbool all){
  if(0<olen){
    FXint pos=0;
    while(pos<=length()-olen){
      if(compare(str+pos,org,olen)==0){
        replace(pos,olen,rep,rlen);
        if(!all) break;
        pos+=rlen;
        continue;
        }
      pos++;
      }
    }
  return *this;
  }


// Substitute one string by another
FXString& FXString::substitute(const FXchar* org,const FXchar* rep,FXbool all){
  return substitute(org,strlen(org),rep,strlen(rep),all);
  }


// Substitute one string by another
FXString& FXString::substitute(const FXString& org,const FXString& rep,FXbool all){
  return substitute(org.text(),org.length(),rep.text(),rep.length(),all);
  }


// Convert to lower case
FXString& FXString::lower(){
  FXint p,ow,nw;
  FXwchar w;
  for(p=0; p<length(); p+=nw){
    w=wc(p);
    ow=wc2utf(w);
    w=Unicode::toLower(w);
    nw=wc2utf(w);
    replace(p,ow,&w,1);
    }
  return *this;
  }


// Convert to upper case
FXString& FXString::upper(){
  FXint p,ow,nw;
  FXwchar w;
  for(p=0; p<length(); p+=nw){
    w=wc(p);
    ow=wc2utf(w);
    w=Unicode::toUpper(w);
    nw=wc2utf(w);
    replace(p,ow,&w,1);
    }
  return *this;
  }


// Simplify whitespace in string
FXString& FXString::simplify(){
  if(str!=EMPTY){
    FXint s=0;
    FXint d=0;
    FXint e=length();
    while(s<e && Ascii::isSpace(str[s])) s++;
    while(1){
      while(s<e && !Ascii::isSpace(str[s])) str[d++]=str[s++];
      while(s<e && Ascii::isSpace(str[s])) s++;
      if(s>=e) break;
      str[d++]=' ';
      }
    length(d);
    }
  return *this;
  }


// Remove leading and trailing whitespace
FXString& FXString::trim(){
  if(str!=EMPTY){
    FXint s=0;
    FXint e=length();
    while(0<e && Ascii::isSpace(str[e-1])) e--;
    while(s<e && Ascii::isSpace(str[s])) s++;
    memmove(str,&str[s],e-s);
    length(e-s);
    }
  return *this;
  }


// Remove leading whitespace
FXString& FXString::trimBegin(){
  if(str!=EMPTY){
    FXint s=0;
    FXint e=length();
    while(s<e && Ascii::isSpace(str[s])) s++;
    memmove(str,str+s,e-s);
    length(e-s);
    }
  return *this;
  }


// Remove trailing whitespace
FXString& FXString::trimEnd(){
  if(str!=EMPTY){
    FXint e=length();
    while(0<e && Ascii::isSpace(str[e-1])) e--;
    length(e);
    }
  return *this;
  }


// Get leftmost part
FXString FXString::left(FXint n) const {
  FXint len=length();
  if(0<n){
    if(n>len) n=len;
    return FXString(str,n);
    }
  return FXString::null;
  }


// Get rightmost part
FXString FXString::right(FXint n) const {
  FXint len=length();
  if(0<n){
    if(n>len) n=len;
    return FXString(str+len-n,n);
    }
  return FXString::null;
  }


// Get some part in the middle
FXString FXString::mid(FXint pos,FXint n) const {
  FXint len=length();
  if(__likely(0<n && 0<pos+n && pos<len)){
    if(pos<0){n+=pos;pos=0;}
    if(len-pos<n){n=len-pos;}
    return FXString(str+pos,n);
    }
  return FXString::null;
  }


// Return partition of string separated by delimiter delim
FXString FXString::section(FXchar delim,FXint start,FXint num) const {
  FXint len=length(),s,e;
  s=0;
  if(0<start){
    while(s<len){
      ++s;
      if(str[s-1]==delim && --start==0) break;
      }
    }
  e=s;
  if(0<num){
    while(e<len){
      if(str[e]==delim && --num==0) break;
      ++e;
      }
    }
  return FXString(str+s,e-s);
  }


// Return partition of string separated by delimiters in delim
FXString FXString::section(const FXchar* delim,FXint n,FXint start,FXint num) const {
  FXint len=length(),s,e,i;
  FXchar c;
  s=0;
  if(0<start){
    while(s<len){
      c=str[s++];
      i=n;
      while(--i>=0){
        if(delim[i]==c){
          if(--start==0) goto a;
          break;
          }
        }
      }
    }
a:e=s;
  if(0<num){
    while(e<len){
      c=str[e];
      i=n;
      while(--i>=0){
        if(delim[i]==c){
          if(--num==0) goto b;
          break;
          }
        }
      ++e;
      }
    }
b:return FXString(str+s,e-s);
  }


// Return partition of string separated by delimiters in delim
FXString FXString::section(const FXchar* delim,FXint start,FXint num) const {
  return section(delim,strlen(delim),start,num);
  }


// Return partition of string separated by delimiters in delim
FXString FXString::section(const FXString& delim,FXint start,FXint num) const {
  return section(delim.text(),delim.length(),start,num);
  }


// Return all characters before the nth occurrence of ch, searching forward
FXString FXString::before(FXchar c,FXint n) const {
  FXint len=length();
  FXint p=0;
  if(0<n){
    while(p<len){
      if(str[p]==c && --n==0) break;
      p++;
      }
    }
  return FXString(str,p);
  }


// Return all characters before the nth occurrence of ch, searching backward
FXString FXString::rbefore(FXchar c,FXint n) const {
  FXint p=length();
  if(0<n){
    while(0<p){
      p--;
      if(str[p]==c && --n==0) break;
      }
    }
  return FXString(str,p);
  }


// Return all characters after the nth occurrence of ch, searching forward
FXString FXString::after(FXchar c,FXint n) const {
  FXint len=length();
  FXint p=0;
  if(0<n){
    while(p<len){
      p++;
      if(str[p-1]==c && --n==0) break;
      }
    }
  return FXString(str+p,len-p);
  }


// Return all characters after the nth occurrence of ch, searching backward
FXString FXString::rafter(FXchar c,FXint n) const {
  FXint len=length();
  FXint p=len;
  if(0<n){
    while(0<p){
      if(str[p-1]==c && --n==0) break;
      p--;
      }
    }
  return FXString(str+p,len-p);
  }


// Find n-th occurrence of character, searching forward; return position or -1
FXint FXString::find(FXchar c,FXint pos,FXint n) const {
  FXint len=length();
  FXint p=pos;
  FXint cc=c;
  if(p<0) p=0;
  if(n<=0) return p;
  while(p<len){
    if(str[p]==cc){ if(--n==0) return p; }
    ++p;
    }
  return -1;
  }


// Find n-th occurrence of character, searching backward; return position or -1
FXint FXString::rfind(FXchar c,FXint pos,FXint n) const {
  FXint len=length();
  FXint p=pos;
  FXint cc=c;
  if(p>=len) p=len-1;
  if(n<=0) return p;
  while(0<=p){
    if(str[p]==cc){ if(--n==0) return p; }
    --p;
    }
  return -1;
  }


// Find a character, searching forward; return position or -1
FXint FXString::find(FXchar c,FXint pos) const {
  FXint len=length();
  FXint p=pos;
  FXint cc=c;
  if(p<0) p=0;
  while(p<len){ if(str[p]==cc){ return p; } ++p; }
  return -1;
  }


// Find a character, searching backward; return position or -1
FXint FXString::rfind(FXchar c,FXint pos) const {
  FXint len=length();
  FXint p=pos;
  FXint cc=c;
  if(p>=len) p=len-1;
  while(0<=p){ if(str[p]==cc){ return p; } --p; }
  return -1;
  }


// Find a substring of length n, searching forward; return position or -1
FXint FXString::find(const FXchar* substr,FXint n,FXint pos) const {
  FXint len=length();
  if(0<=pos && 0<n && n<=len){
    FXint c=substr[0];
    len=len-n+1;
    while(pos<len){
      if(str[pos]==c){
        if(!compare(str+pos,substr,n)){
          return pos;
          }
        }
      pos++;
      }
    }
  return -1;
  }


// Find a substring, searching forward; return position or -1
FXint FXString::find(const FXchar* substr,FXint pos) const {
  return find(substr,strlen(substr),pos);
  }


// Find a substring, searching forward; return position or -1
FXint FXString::find(const FXString& substr,FXint pos) const {
  return find(substr.text(),substr.length(),pos);
  }


// Find a substring of length n, searching backward; return position or -1
FXint FXString::rfind(const FXchar* substr,FXint n,FXint pos) const {
  FXint len=length();
  if(0<=pos && 0<n && n<=len){
    FXint c=substr[0];
    len-=n;
    if(pos>len) pos=len;
    while(0<=pos){
      if(str[pos]==c){
        if(!compare(str+pos,substr,n)){
          return pos;
          }
        }
      pos--;
      }
    }
  return -1;
  }


// Find a substring, searching backward; return position or -1
FXint FXString::rfind(const FXchar* substr,FXint pos) const {
  return rfind(substr,strlen(substr),pos);
  }


// Find a substring, searching backward; return position or -1
FXint FXString::rfind(const FXString& substr,FXint pos) const {
  return rfind(substr.text(),substr.length(),pos);
  }


// Find first character in the set of size n, starting from pos; return position or -1
FXint FXString::find_first_of(const FXchar* set,FXint n,FXint pos) const {
  FXint len=length();
  FXint p=pos;
  if(p<0) p=0;
  while(p<len){
    FXint c=str[p];
    FXint i=n;
    while(--i>=0){ if(set[i]==c) return p; }
    p++;
    }
  return -1;
  }


// Find first character in the set, starting from pos; return position or -1
FXint FXString::find_first_of(const FXchar* set,FXint pos) const {
  return find_first_of(set,strlen(set),pos);
  }


// Find first character in the set, starting from pos; return position or -1
FXint FXString::find_first_of(const FXString& set,FXint pos) const {
  return find_first_of(set.text(),set.length(),pos);
  }


// Find first character, starting from pos; return position or -1
FXint FXString::find_first_of(FXchar c,FXint pos) const {
  FXint len=length();
  FXint p=pos;
  FXint cc=c;
  if(p<0) p=0;
  while(p<len){ if(str[p]==cc){ return p; } p++; }
  return -1;
  }


// Find last character in the set of size n, starting from pos; return position or -1
FXint FXString::find_last_of(const FXchar* set,FXint n,FXint pos) const {
  FXint len=length();
  FXint p=pos;
  if(p>=len) p=len-1;
  while(0<=p){
    FXint c=str[p];
    FXint i=n;
    while(--i>=0){ if(set[i]==c) return p; }
    p--;
    }
  return -1;
  }


// Find last character in the set, starting from pos; return position or -1
FXint FXString::find_last_of(const FXchar* set,FXint pos) const {
  return find_last_of(set,strlen(set),pos);
  }


// Find last character in the set, starting from pos; return position or -1
FXint FXString::find_last_of(const FXString& set,FXint pos) const {
  return find_last_of(set.text(),set.length(),pos);
  }


// Find last character, starting from pos; return position or -1
FXint FXString::find_last_of(FXchar c,FXint pos) const {
  FXint len=length();
  FXint p=pos;
  FXint cc=c;
  if(p>=len) p=len-1;
  while(0<=p){ if(str[p]==cc){ return p; } p--; }
  return -1;
  }


// Find first character NOT in the set of size n, starting from pos; return position or -1
FXint FXString::find_first_not_of(const FXchar* set,FXint n,FXint pos) const {
  FXint len=length();
  FXint p=pos;
  if(p<0) p=0;
  while(p<len){
    FXint c=str[p];
    FXint i=n;
    while(--i>=0){ if(set[i]==c) goto x; }
    return p;
x:  p++;
    }
  return -1;
  }


// Find first character NOT in the set, starting from pos; return position or -1
FXint FXString::find_first_not_of(const FXchar* set,FXint pos) const {
  return find_first_not_of(set,strlen(set),pos);
  }


// Find first character NOT in the set, starting from pos; return position or -1
FXint FXString::find_first_not_of(const FXString& set,FXint pos) const {
  return find_first_not_of(set.text(),set.length(),pos);
  }


// Find first character NOT equal to c, starting from pos; return position or -1
FXint FXString::find_first_not_of(FXchar c,FXint pos) const {
  FXint len=length();
  FXint p=pos;
  FXint cc=c;
  if(p<0) p=0;
  while(p<len){ if(str[p]!=cc){ return p; } p++; }
  return -1;
  }


// Find last character NOT in the set of size n, starting from pos; return position or -1
FXint FXString::find_last_not_of(const FXchar* set,FXint n,FXint pos) const {
  FXint len=length();
  FXint p=pos;
  if(p>=len) p=len-1;
  while(0<=p){
    FXint c=str[p];
    FXint i=n;
    while(--i>=0){ if(set[i]==c) goto x; }
    return p;
x:  p--;
    }
  return -1;
  }


// Find last character NOT in the set, starting from pos; return position or -1
FXint FXString::find_last_not_of(const FXchar* set,FXint pos) const {
  return find_last_not_of(set,strlen(set),pos);
  }


// Find last character NOT in the set, starting from pos; return position or -1
FXint FXString::find_last_not_of(const FXString& set,FXint pos) const {
  return find_last_not_of(set.text(),set.length(),pos);
  }


// Find last character NOT equal to c, starting from pos; return position or -1
FXint FXString::find_last_not_of(FXchar c,FXint pos) const {
  FXint len=length();
  FXint p=pos;
  FXint cc=c;
  if(p>=len) p=len-1;
  while(0<=p){ if(str[p]!=cc){ return p; } p--; }
  return -1;
  }

/*******************************************************************************/

#ifdef WIN32
#ifndef va_copy
#define va_copy(arg,list) ((arg)=(list))
#endif
#endif


// Scan
FXint FXString::vscan(const FXchar* fmt,va_list args) const {
  return __vsscanf(str,fmt,args);
  }


FXint FXString::scan(const FXchar* fmt,...) const {
  FXint result;
  va_list args;
  va_start(args,fmt);
  result=vscan(fmt,args);
  va_end(args);
  return result;
  }


// Print formatted string a-la vprintf
FXint FXString::vformat(const FXchar* fmt,va_list args){
  FXint result=0;
  if(fmt && *fmt){
    va_list ag;
    va_copy(ag,args);
    result=__vsnprintf(str,length(),fmt,ag);       // Try to see if existing buffer fits
    va_end(ag);
    if(length()<result){                           // FOX's own __vsnprintf() truncates at buffer size
      length(result);
      result=__vsnprintf(str,length(),fmt,args);   // Now try again with exactly the right size
      return result;
      }
    }
  length(result);
  return result;
  }


// Print formatted string a-la printf
FXint FXString::format(const FXchar* fmt,...){
  va_list args;
  va_start(args,fmt);
  FXint result=vformat(fmt,args);
  va_end(args);
  return result;
  }

/*******************************************************************************/

// Convert to integer
FXint FXString::toInt(FXint base,FXbool* ok) const {
  return __strtol(str,NULL,base,ok);
  }


// Convert to unsigned integer
FXuint FXString::toUInt(FXint base,FXbool* ok) const {
  return __strtoul(str,NULL,base,ok);
  }


// Convert to long integer
FXlong FXString::toLong(FXint base,FXbool* ok) const {
  return __strtoll(str,NULL,base,ok);
  }


// Convert to unsigned long integer
FXulong FXString::toULong(FXint base,FXbool* ok) const {
  return __strtoull(str,NULL,base,ok);
  }


// Convert to float
FXfloat FXString::toFloat(FXbool* ok) const {
  return __strtof(str,NULL,ok);
  }


// Convert to double number
FXdouble FXString::toDouble(FXbool* ok) const {
  return __strtod(str,NULL,ok);
  }


// Convert from integer
FXString& FXString::fromInt(FXint number,FXint base){
  FXuint nn=FXABS(number);
  FXchar buf[34],*p=buf+sizeof(buf);
  if(base<2 || base>16){ fxerror("FXString::fromInt: base out of range.\n"); }
  do{
    *--p=FXString::value2Digit[nn%base];
    nn/=base;
    }
  while(nn);
  if(number<0) *--p='-';
  return assign(p,buf+sizeof(buf)-p);
  }


// Convert from unsigned integer
FXString& FXString::fromUInt(FXuint number,FXint base){
  FXuint nn=number;
  FXchar buf[34],*p=buf+sizeof(buf);
  if(base<2 || base>16){ fxerror("FXString::fromUInt: base out of range.\n"); }
  do{
    *--p=FXString::value2Digit[nn%base];
    nn/=base;
    }
  while(nn);
  return assign(p,buf+sizeof(buf)-p);
  }


// Convert from long integer
FXString& FXString::fromLong(FXlong number,FXint base){
  FXulong nn=FXABS(number);
  FXchar buf[66],*p=buf+sizeof(buf);
  if(base<2 || base>16){ fxerror("FXString::fromLong: base out of range.\n"); }
  do{
    *--p=FXString::value2Digit[nn%base];
    nn/=base;
    }
  while(nn);
  if(number<0) *--p='-';
  return assign(p,buf+sizeof(buf)-p);
  }


// Convert from unsigned long integer
FXString& FXString::fromULong(FXulong number,FXint base){
  FXulong nn=number;
  FXchar buf[66],*p=buf+sizeof(buf);
  if(base<2 || base>16){ fxerror("FXString::fromULong: base out of range.\n"); }
  do{
    *--p=FXString::value2Digit[nn%base];
    nn/=base;
    }
  while(nn);
  return assign(p,buf+sizeof(buf)-p);
  }


// Formatting for reals
static const char conversionformat[8][8]={"%.*lF","%.*lE","%.*lG","%.*lF","%'.*lF","%'.*lE","%'.*lG","%'.*lF"};


// Convert from float
FXString& FXString::fromFloat(FXfloat number,FXint prec,FXint fmt){
  format(conversionformat[fmt&7],prec,(FXdouble)number);
  return *this;
  }


// Convert from double
FXString& FXString::fromDouble(FXdouble number,FXint prec,FXint fmt){
  format(conversionformat[fmt&7],prec,number);
  return *this;
  }


// Return string by converting a integer
FXString FXString::value(FXint num,FXint base){
  FXString result;
  return result.fromInt(num,base);
  }


// Return string by converting an unsigned integer
FXString FXString::value(FXuint num,FXint base){
  FXString result;
  return result.fromUInt(num,base);
  }


// Return string by converting a long integer
FXString FXString::value(FXlong num,FXint base){
  FXString result;
  return result.fromLong(num,base);
  }


// Return string by converting an unsigned long
FXString FXString::value(FXulong num,FXint base){
  FXString result;
  return result.fromULong(num,base);
  }


// Return string by converting a float
FXString FXString::value(FXfloat num,FXint prec,FXint fmt){
  FXString result;
  return result.fromFloat(num,prec,fmt);
  }


// Return string by converting a double
FXString FXString::value(FXdouble num,FXint prec,FXint fmt){
  FXString result;
  return result.fromDouble(num,prec,fmt);
  }


// Return string from printf-like format arguments
FXString FXString::value(const FXchar* fmt,...){
  FXString result;
  va_list args;
  va_start(args,fmt);
  result.vformat(fmt,args);
  va_end(args);
  return result;
  }


// Return string from vprintf-like format arguments
FXString FXString::vvalue(const FXchar* fmt,va_list args){
  FXString result;
  result.vformat(fmt,args);
  return result;
  }

/*******************************************************************************/

// Compute FNV1a hash value of string
FXuint FXString::hash(const FXchar* s){
  FXuint result=0x811C9DC5;
  FXuchar c;
  while((c=*s++)!='\0'){
    result=(result^c)*0x01000193;
    }
  return result;
  }


// Compute hash value of string
FXuint FXString::hash() const {
  return FXString::hash(str);
  }

/*******************************************************************************/

// Save
FXStream& operator<<(FXStream& store,const FXString& s){
  FXint len=s.length();
  store << len;
  store.save(s.str,len);
  return store;
  }


// Load
FXStream& operator>>(FXStream& store,FXString& s){
  FXint len;
  store >> len;
  s.length(len);
  store.load(s.str,len);
  return store;
  }

/*******************************************************************************/

// Compare string and string
FXint compare(const FXchar* s1,const FXchar* s2){
  FXint c1,c2;
  do{
    c1=(FXuchar) *s1++;
    c2=(FXuchar) *s2++;
    }
  while(c1 && (c1==c2));
  return c1-c2;
  }


// Compare string and FXString
FXint compare(const FXchar* s1,const FXString& s2){
  return compare(s1,s2.text());
  }


// Compare FXString and string
FXint compare(const FXString& s1,const FXchar* s2){
  return compare(s1.text(),s2);
  }


// Compare FXString and FXString
FXint compare(const FXString& s1,const FXString& s2){
  return compare(s1.text(),s2.text());
  }


// Compare string and string, up to n
FXint compare(const FXchar* s1,const FXchar* s2,FXint n){
  if(0<n){
    const FXchar* e1=s1+n;
    FXint c1,c2;
    do{
      c1=(FXuchar) *s1++;
      c2=(FXuchar) *s2++;
      }
    while(c1 && (c1==c2) && (s1<e1));
    return c1-c2;
    }
  return 0;
  }


// Compare string and FXString, up to n
FXint compare(const FXchar* s1,const FXString& s2,FXint n){
  return compare(s1,s2.text(),n);
  }


// Compare FXString and string, up to n
FXint compare(const FXString& s1,const FXchar* s2,FXint n){
  return compare(s1.text(),s2,n);
  }


// Compare FXString and FXString, up to n
FXint compare(const FXString& s1,const FXString& s2,FXint n){
  return compare(s1.text(),s2.text(),n);
  }


// Compare string and string case insensitive
FXint comparecase(const FXchar* s1,const FXchar* s2){
  FXint c1,c2;
  do{
    c1=Unicode::toLower(wc(s1));
    c2=Unicode::toLower(wc(s2));
    if(!c1 || (c1!=c2)) break;
    s1=wcinc(s1);
    s2=wcinc(s2);
    }
  while(1);
  return c1-c2;
  }


// Compare string and FXString case insensitive
FXint comparecase(const FXchar* s1,const FXString& s2){
  return comparecase(s1,s2.text());
  }


// Compare FXString and string case insensitive
FXint comparecase(const FXString& s1,const FXchar* s2){
  return comparecase(s1.text(),s2);
  }


// Compare FXString and FXString case insensitive
FXint comparecase(const FXString& s1,const FXString& s2){
  return comparecase(s1.text(),s2.text());
  }


// Compare string and string case insensitive, up to n
FXint comparecase(const FXchar* s1,const FXchar* s2,FXint n){
  if(0<n){
    const FXchar* e1=s1+n;
    const FXchar* e2=s2+n;
    FXint c1,c2;
    do{
      c1=Unicode::toLower(wc(s1));
      c2=Unicode::toLower(wc(s2));
      if(!c1 || (c1!=c2)) break;
      s1=wcinc(s1);
      s2=wcinc(s2);
      }
    while((s1<e1) && (s2<e2));
    return c1-c2;
    }
  return 0;
  }


// Compare string and FXString case insensitive, up to n
FXint comparecase(const FXchar* s1,const FXString& s2,FXint n){
  return comparecase(s1,s2.text(),n);
  }


// Compare FXString and string case insensitive, up to n
FXint comparecase(const FXString& s1,const FXchar* s2,FXint n){
  return comparecase(s1.text(),s2,n);
  }


// Compare FXString and FXString case insensitive, up to n
FXint comparecase(const FXString& s1,const FXString& s2,FXint n){
  return comparecase(s1.text(),s2.text(),n);
  }


/*******************************************************************************/

enum {
  S_N = 0x0,    // Normal
  S_I = 0x4,    // Comparing integral part
  S_F = 0x8,    // Comparing fractional parts
  S_Z = 0xC     // Idem but with leading zeroes only
  };


enum {
  CMP = 2,      // Return diff
  LEN = 3       // Compare using len_diff/diff
  };


// Symbol(s)    0       [1-9]   others  (padding)
// Transition   (10) 0  (01) d  (00) x  (11) -
static const unsigned int next_state[]={
  /* state    x    d    0    - */
  /* S_N */  S_N, S_I, S_Z, S_N,
  /* S_I */  S_N, S_I, S_I, S_I,
  /* S_F */  S_N, S_F, S_F, S_F,
  /* S_Z */  S_N, S_F, S_Z, S_Z
  };

static const int result_type[]={
  /* state   x/x  x/d  x/0  x/-  d/x  d/d  d/0  d/-  0/x  0/d  0/0  0/-  -/x  -/d  -/0  -/- */
  /* S_N */  CMP, CMP, CMP, CMP, CMP, LEN, CMP, CMP, CMP, CMP, CMP, CMP, CMP, CMP, CMP, CMP,
  /* S_I */  CMP,  -1,  -1, CMP,  +1, LEN, LEN, CMP,  +1, LEN, LEN, CMP, CMP, CMP, CMP, CMP,
  /* S_F */  CMP, CMP, CMP, CMP, CMP, LEN, CMP, CMP, CMP, CMP, CMP, CMP, CMP, CMP, CMP, CMP,
  /* S_Z */  CMP,  +1,  +1, CMP,  -1, CMP, CMP, CMP,  -1, CMP, CMP, CMP
  };


// Compare string and string as versions numbers
FXint compareversion(const FXchar *s1,const FXchar *s2){
  const FXuchar *p1=(const FXuchar*)s1;
  const FXuchar *p2=(const FXuchar*)s2;
  FXuchar c1,c2;
  FXint state;
  FXint diff;

  c1 = *p1++;
  c2 = *p2++;

  // Hint: '0' is a digit too.
  state=S_N | ((c1=='0')+(Ascii::isDigit(c1)!=0));
  while((diff=c1-c2)==0 && c1!='\0'){
    state=next_state[state];
    c1=*p1++;
    c2=*p2++;
    state|=(c1=='0')+(Ascii::isDigit(c1)!=0);
    }
  state=result_type[state<<2 | (((c2=='0')+(Ascii::isDigit(c2)!=0)))];
  switch(state){
    case LEN:
      while(Ascii::isDigit(*p1++)){
	if(!Ascii::isDigit(*p2++)) return 1;
        }
      if(Ascii::isDigit(*p2)) return -1;
    case CMP:
      return diff;
    }
  return state;
  }


// Compare string and FXString natural interpretation
FXint compareversion(const FXchar* s1,const FXString& s2){
  return compareversion(s1,s2.text());
  }


// Compare FXString and string natural interpretation
FXint compareversion(const FXString& s1,const FXchar* s2){
  return compareversion(s1.text(),s2);
  }


// Compare FXString and FXString natural interpretation
FXint compareversion(const FXString& s1,const FXString& s2){
  return compareversion(s1.text(),s2.text());
  }


// Compare case insensitive with natural interpretation
FXint compareversioncase(const FXchar* s1,const FXchar* s2){
  const FXuchar *p1=(const FXuchar*)s1;
  const FXuchar *p2=(const FXuchar*)s2;
  FXuchar c1,c2;
  FXint state;
  FXint diff;

  if(p1==p2) return 0;

  c1=Ascii::toLower(*p1++);
  c2=Ascii::toLower(*p2++);

  // Hint: '0' is a digit too.
  state=S_N | ((c1=='0')+(Ascii::isDigit(c1)!=0));
  while((diff=c1-c2)==0 && c1!='\0'){
    state=next_state[state];
    c1=Ascii::toLower(*p1++);
    c2=Ascii::toLower(*p2++);
    state|=(c1=='0')+(Ascii::isDigit(c1)!=0);
    }
  state=result_type[state<<2 | (((c2=='0')+(Ascii::isDigit(c2)!=0)))];
  switch(state){
    case LEN:
      while(Ascii::isDigit(*p1++)){
	if(!Ascii::isDigit(*p2++)) return 1;
        }
      if(Ascii::isDigit(*p2)) return -1;
    case CMP:
      return diff;
    }
  return state;
  }


// Compare case insensitive with natural interpretation
FXint compareversioncase(const FXchar* s1,const FXString& s2){
  return compareversioncase(s1,s2.text());
  }


// Compare case insensitive with natural interpretation
FXint compareversioncase(const FXString& s1,const FXchar* s2){
  return compareversioncase(s1.text(),s2);
  }


// Compare case insensitive with natural interpretation
FXint compareversioncase(const FXString& s1,const FXString& s2){
  return compareversioncase(s1.text(),s2.text());
  }

/*******************************************************************************/

// Comparison operators
FXbool operator==(const FXString& s1,const FXString& s2){
  return compare(s1.text(),s2.text())==0;
  }

FXbool operator==(const FXString& s1,const FXchar* s2){
  return compare(s1.text(),s2)==0;
  }

FXbool operator==(const FXchar* s1,const FXString& s2){
  return compare(s1,s2.text())==0;
  }

FXbool operator!=(const FXString& s1,const FXString& s2){
  return compare(s1.text(),s2.text())!=0;
  }

FXbool operator!=(const FXString& s1,const FXchar* s2){
  return compare(s1.text(),s2)!=0;
  }

FXbool operator!=(const FXchar* s1,const FXString& s2){
  return compare(s1,s2.text())!=0;
  }

FXbool operator<(const FXString& s1,const FXString& s2){
  return compare(s1.text(),s2.text())<0;
  }

FXbool operator<(const FXString& s1,const FXchar* s2){
  return compare(s1.text(),s2)<0;
  }

FXbool operator<(const FXchar* s1,const FXString& s2){
  return compare(s1,s2.text())<0;
  }

FXbool operator<=(const FXString& s1,const FXString& s2){
  return compare(s1.text(),s2.text())<=0;
  }

FXbool operator<=(const FXString& s1,const FXchar* s2){
  return compare(s1.text(),s2)<=0;
  }

FXbool operator<=(const FXchar* s1,const FXString& s2){
  return compare(s1,s2.text())<=0;
  }

FXbool operator>(const FXString& s1,const FXString& s2){
  return compare(s1.text(),s2.text())>0;
  }

FXbool operator>(const FXString& s1,const FXchar* s2){
  return compare(s1.text(),s2)>0;
  }

FXbool operator>(const FXchar* s1,const FXString& s2){
  return compare(s1,s2.text())>0;
  }

FXbool operator>=(const FXString& s1,const FXString& s2){
  return compare(s1.text(),s2.text())>=0;
  }

FXbool operator>=(const FXString& s1,const FXchar* s2){
  return compare(s1.text(),s2)>=0;
  }

FXbool operator>=(const FXchar* s1,const FXString& s2){
  return compare(s1,s2.text())>=0;
  }


/*******************************************************************************/

// Concatenate two FXStrings
FXString operator+(const FXString& s1,const FXString& s2){
  FXString result(s1);
  return result.append(s2);
  }


// Concatenate FXString and string
FXString operator+(const FXString& s1,const FXchar* s2){
  FXString result(s1);
  return result.append(s2);
  }


// Concatenate FXString and narrow character string
FXString operator+(const FXString& s1,const FXnchar* s2){
  FXString result(s1);
  return result.append(s2);
  }


// Concatenate FXString and wide character string
FXString operator+(const FXString& s1,const FXwchar* s2){
  FXString result(s1);
  return result.append(s2);
  }


// Concatenate string and FXString
FXString operator+(const FXchar* s1,const FXString& s2){
  FXString result(s1);
  return result.append(s2);
  }


// Concatenate narrow character string and FXString
FXString operator+(const FXnchar* s1,const FXString& s2){
  FXString result(s1);
  return result.append(s2);
  }


// Concatenate wide character string and FXString
FXString operator+(const FXwchar* s1,const FXString& s2){
  FXString result(s1);
  return result.append(s2);
  }


// Concatenate FXString and character
FXString operator+(const FXString& s,FXchar c){
  FXString result(s);
  return result.append(c);
  }


// Concatenate character and FXString
FXString operator+(FXchar c,const FXString& s){
  FXString result(&c,1);
  return result.append(s);
  }

/*******************************************************************************/

// Convert unix string to dos string
FXString& unixToDos(FXString& str){
  FXint f=0,t=0;
  while(f<str.length() && str[f]){
    if(str[f++]=='\n') t++;
    t++;
    }
  str.length(t);
  while(0<f){
    if((str[--t]=str[--f])=='\n') str[--t]='\r';
    }
  return str;
  }


// Convert dos string to unix string
FXString& dosToUnix(FXString& str){
  FXint f=0,t=0,c;
  while(f<str.length() && str[f]){
    if((c=str[f++])!='\r') str[t++]=c;
    }
  str.length(t);
  return str;
  }

}
