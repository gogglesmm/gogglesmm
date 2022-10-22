/********************************************************************************
*                                                                               *
*     U n i c o d e   C h a r a c t e r   E n c o d i n g   S u p p o r t       *
*                                                                               *
*********************************************************************************
* Copyright (C) 1997,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#define __NEW_DEFS__ 1
#include "fxdefs.h"
#include "fxchar.h"
#include "fxmath.h"
#include "fxascii.h"
#include "fxunicode.h"


/*
  Notes:

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


using namespace FX;

/*******************************************************************************/

namespace FX {

// For conversion from UTF16 to UTF32
const FXint SURROGATE_OFFSET=0x10000-(0xD800<<10)-0xDC00;

// For conversion of UTF32 to UTF16
const FXint LEAD_OFFSET=0xD800-(0x10000>>10);

// For conversion of UTF32 to UTF16
const FXint TAIL_OFFSET=0xDC00;

/*******************************************************************************/

// Return non-zero if valid utf8 character sequence
FXival wcvalid(const FXchar* ptr){
  if((FXuchar)ptr[0]<0x80) return 1;
  if((FXuchar)ptr[0]<0xC0) return 0;
  if((FXuchar)ptr[1]<0x80) return 0;
  if((FXuchar)ptr[1]>0xBF) return 0;
  if((FXuchar)ptr[0]<0xE0) return 2;
  if((FXuchar)ptr[2]<0x80) return 0;
  if((FXuchar)ptr[2]>0xBF) return 0;
  if((FXuchar)ptr[0]<0xF0) return 3;
  if((FXuchar)ptr[0]>0xF7) return 0;
  if((FXuchar)ptr[3]<0x80) return 0;
  if((FXuchar)ptr[3]>0xBF) return 0;
  return 4;
  }


// Return non-zero if valid utf16 character sequence
FXival wcvalid(const FXnchar* ptr){
  if(ptr[0]<0xD800) return 1;
  if(ptr[0]>0xDFFF) return 1;
  if(ptr[0]>0xDBFF) return 0;
  if(ptr[1]<0xDC00) return 0;
  if(ptr[1]>0xDFFF) return 0;
  return 2;
  }

/*******************************************************************************/

// Return number of bytes for utf8 representation of wide character string
FXival wcs2utf(const FXwchar* src,FXival srclen){
  const FXwchar* srcend=src+srclen;
  FXival p=0;
  FXwchar w;
  while(src<srcend){
    w=*src++;
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
  while(src<srcend){
    w=*src++;
    if(leadUTF16(w)){
      if(src>=srcend) break;
      if(!followUTF16(*src)) break;
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
    if(leadUTF16(w)){
      if(!followUTF16(*src)) break;
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
  while(src<srcend){
    c=src[0];
    if(0xC0<=c){
      if(src+1>=srcend) break;
      if(!followUTF8(src[1])) break;
      if(0xE0<=c){
        if(src+2>=srcend) break;
        if(!followUTF8(src[2])) break;
        if(0xF0<=c){
          if(src+3>=srcend) break;
          if(!followUTF8(src[3])) break;
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
      if(!followUTF8(src[1])) break;
      if(0xE0<=c){
        if(!followUTF8(src[2])) break;
        if(0xF0<=c){
          if(!followUTF8(src[3])) break;
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
  while(src<end){
    c=src[0];
    if(0xC0<=c){
      if(src+1>=end) break;
      if(!followUTF8(src[1])) break;
      if(0xE0<=c){
        if(src+2>=end) break;
        if(!followUTF8(src[2])) break;
        if(0xF0<=c){
          if(src+3>=end) break;
          if(!followUTF8(src[3])) break;
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
      if(!followUTF8(src[1])) break;
      if(0xE0<=c){
        if(!followUTF8(src[2])) break;
        if(0xF0<=c){
          if(!followUTF8(src[3])) break;
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

/*******************************************************************************/

// Convert wide character to utf8 string
FXival wc2utf(FXchar *dst,FXwchar w){
  if(0x80<=w){
    if(0x800<=w){
      if(0x10000<=w){
        dst[0]=(w>>18)|0xF0;
        dst[1]=((w>>12)&0x3F)|0x80;
        dst[2]=((w>>6)&0x3F)|0x80;
        dst[3]=(w&0x3F)|0x80;
        return 4;
        }
      dst[0]=(w>>12)|0xE0;
      dst[1]=((w>>6)&0x3F)|0x80;
      dst[2]=(w&0x3F)|0x80;
      return 3;
      }
    dst[0]=(w>>6)|0xC0;
    dst[1]=(w&0x3F)|0x80;
    return 2;
    }
  dst[0]=w;
  return 1;
  }


// Convert wide character to narrow character string
FXival wc2nc(FXnchar *dst,FXwchar w){
  if(0x10000<=w){
    dst[0]=LEAD_OFFSET+(w>>10);
    dst[1]=TAIL_OFFSET+(w&0x3FF);
    return 2;
    }
  dst[0]=w;
  return 1;
  }


// Convert wide character string to utf8 string
FXival wcs2utf(FXchar *dst,const FXwchar* src,FXival dstlen,FXival srclen){
  const FXwchar* srcend=src+srclen;
  FXchar* ptrend=dst+dstlen;
  FXchar* ptr=dst;
  FXwchar w;
  while(src<srcend){
    w=*src++;
    if(0x80<=w){
      if(0x800<=w){
        if(0x10000<=w){
          if(ptr+3>=ptrend) break;
          *ptr++=(w>>18)|0xF0;
          *ptr++=((w>>12)&0x3F)|0x80;
          *ptr++=((w>>6)&0x3F)|0x80;
          *ptr++=(w&0x3F)|0x80;
          continue;
          }
        if(ptr+2>=ptrend) break;
        *ptr++=(w>>12)|0xE0;
        *ptr++=((w>>6)&0x3F)|0x80;
        *ptr++=(w&0x3F)|0x80;
        continue;
        }
      if(ptr+1>=ptrend) break;
      *ptr++=(w>>6)|0xC0;
      *ptr++=(w&0x3F)|0x80;
      continue;
      }
    if(ptr>=ptrend) break;
    *ptr++=w;
    }
  if(ptr<ptrend) *ptr='\0';
  return ptr-dst;
  }


// Convert wide character string to utf8 string
FXival wcs2utf(FXchar *dst,const FXwchar* src,FXival dstlen){
  FXchar* ptrend=dst+dstlen;
  FXchar* ptr=dst;
  FXwchar w;
  while((w=*src++)!=0){
    if(0x80<=w){
      if(0x800<=w){
        if(0x10000<=w){
          if(ptr+3>=ptrend) break;
          *ptr++=(w>>18)|0xF0;
          *ptr++=((w>>12)&0x3F)|0x80;
          *ptr++=((w>>6)&0x3F)|0x80;
          *ptr++=(w&0x3F)|0x80;
          continue;
          }
        if(ptr+2>=ptrend) break;
        *ptr++=(w>>12)|0xE0;
        *ptr++=((w>>6)&0x3F)|0x80;
        *ptr++=(w&0x3F)|0x80;
        continue;
        }
      if(ptr+1>=ptrend) break;
      *ptr++=(w>>6)|0xC0;
      *ptr++=(w&0x3F)|0x80;
      continue;
      }
    if(ptr>=ptrend) break;
    *ptr++=w;
    }
  if(ptr<ptrend) *ptr='\0';
  return ptr-dst;
  }


// Convert narrow character string to utf8 string
FXival ncs2utf(FXchar *dst,const FXnchar* src,FXival dstlen,FXival srclen){
  const FXnchar* srcend=src+srclen;
  FXchar* ptrend=dst+dstlen;
  FXchar* ptr=dst;
  FXwchar w;
  while(src<srcend){
    w=*src++;
    if(0x80<=w){
      if(0x800<=w){
        if(leadUTF16(w)){
          if(src>=srcend) break;
          if(!followUTF16(*src)) break;
          if(ptr+3>=ptrend) break;
          w=SURROGATE_OFFSET+(w<<10)+*src++;
          *ptr++=(w>>18)|0xF0;
          *ptr++=((w>>12)&0x3F)|0x80;
          *ptr++=((w>>6)&0x3F)|0x80;
          *ptr++=(w&0x3F)|0x80;
          continue;
          }
        if(ptr+2>=ptrend) break;
        *ptr++=(w>>12)|0xE0;
        *ptr++=((w>>6)&0x3F)|0x80;
        *ptr++=(w&0x3F)|0x80;
        continue;
        }
      if(ptr+1>=ptrend) break;
      *ptr++=(w>>6)|0xC0;
      *ptr++=(w&0x3F)|0x80;
      continue;
      }
    if(ptr>=ptrend) break;
    *ptr++=w;
    }
  if(ptr<ptrend) *ptr='\0';
  return ptr-dst;
  }


// Convert narrow character string to utf8 string
FXival ncs2utf(FXchar *dst,const FXnchar* src,FXival dstlen){
  FXchar* ptrend=dst+dstlen;
  FXchar* ptr=dst;
  FXwchar w;
  while((w=*src++)!=0){
    if(0x80<=w){
      if(0x800<=w){
        if(leadUTF16(w)){
          if(!followUTF16(*src)) break;
          if(ptr+3>=ptrend) break;
          w=SURROGATE_OFFSET+(w<<10)+*src++;
          *ptr++=(w>>18)|0xF0;
          *ptr++=((w>>12)&0x3F)|0x80;
          *ptr++=((w>>6)&0x3F)|0x80;
          *ptr++=(w&0x3F)|0x80;
          continue;
          }
        if(ptr+2>=ptrend) break;
        *ptr++=(w>>12)|0xE0;
        *ptr++=((w>>6)&0x3F)|0x80;
        *ptr++=(w&0x3F)|0x80;
        continue;
        }
      if(ptr+1>=ptrend) break;
      *ptr++=(w>>6)|0xC0;
      *ptr++=(w&0x3F)|0x80;
      continue;
      }
    if(ptr>=ptrend) break;
    *ptr++=w;
    }
  if(ptr<ptrend) *ptr='\0';
  return ptr-dst;
  }


// Convert utf8 string to wide character string
FXival utf2wcs(FXwchar *dst,const FXchar* src,FXival dstlen,FXival srclen){
  const FXchar* srcend=src+srclen;
  FXwchar* ptrend=dst+dstlen;
  FXwchar* ptr=dst;
  FXwchar w;
  FXuchar c;
  while(src<srcend){
    w=c=*src++;
    if(0xC0<=w){
      if(src>=srcend) break;
      c=*src++;
      if(!followUTF8(c)) break;
      w=(w<<6) ^ c ^ 0x3080;
      if(0x800<=w){
        if(src>=srcend) break;
        c=*src++;
        if(!followUTF8(c)) break;
        w=(w<<6) ^ c ^ 0x20080;
        if(0x10000<=w){
          if(src>=srcend) break;
          c=*src++;
          if(!followUTF8(c)) break;
          w=(w<<6) ^ c ^ 0x400080;
          }
        }
      }
    if(ptr>=ptrend) break;
    *ptr++=w;
    }
  if(ptr<ptrend) *ptr=0;
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
      if(!followUTF8(c)) break;
      w=(w<<6) ^ c ^ 0x3080;
      if(0x800<=w){
        c=*src++;
        if(!followUTF8(c)) break;
        w=(w<<6) ^ c ^ 0x20080;
        if(0x10000<=w){
          c=*src++;
          if(!followUTF8(c)) break;
          w=(w<<6) ^ c ^ 0x400080;
          }
        }
      }
    if(ptr>=ptrend) break;
    *ptr++=w;
    }
  if(ptr<ptrend) *ptr=0;
  return ptr-dst;
  }


// Convert utf8 string to narrow character string
FXival utf2ncs(FXnchar *dst,const FXchar* src,FXival dstlen,FXival srclen){
  const FXchar* srcend=src+srclen;
  FXnchar* ptrend=dst+dstlen;
  FXnchar* ptr=dst;
  FXwchar w;
  FXuchar c;
  while(src<srcend){
    w=c=*src++;
    if(0xC0<=w){
      if(src>=srcend) break;
      c=*src++;
      if(!followUTF8(c)) break;
      w=(w<<6) ^ c ^ 0x3080;
      if(0x800<=w){
        if(src>=srcend) break;
        c=*src++;
        if(!followUTF8(c)) break;
        w=(w<<6) ^ c ^ 0x20080;
        if(0x10000<=w){
          if(src>=srcend) break;
          c=*src++;
          if(!followUTF8(c)) break;
          if(ptr+1>=ptrend) break;
          w=(w<<6) ^ c ^ 0x400080;
          *ptr++=LEAD_OFFSET+(w>>10);
          *ptr++=TAIL_OFFSET+(w&0x3FF);
          continue;
          }
        }
      }
    if(ptr>=ptrend) break;
    *ptr++=w;
    }
  if(ptr<ptrend) *ptr=0;
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
      if(!followUTF8(c)) break;
      w=(w<<6) ^ c ^ 0x3080;
      if(0x800<=w){
        c=*src++;
        if(!followUTF8(c)) break;
        w=(w<<6) ^ c ^ 0x20080;
        if(0x10000<=w){
          c=*src++;
          if(!followUTF8(c)) break;
          if(ptr+1>=ptrend) break;
          w=(w<<6) ^ c ^ 0x400080;
          *ptr++=LEAD_OFFSET+(w>>10);
          *ptr++=TAIL_OFFSET+(w&0x3FF);
          continue;
          }
        }
      }
    if(ptr>=ptrend) break;
    *ptr++=w;
    }
  if(ptr<ptrend) *ptr=0;
  return ptr-dst;
  }

}
