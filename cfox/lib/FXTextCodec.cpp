/********************************************************************************
*                                                                               *
*                   U n i c o d e   T e x t   C o d e c                         *
*                                                                               *
*********************************************************************************
* Copyright (C) 2002,2022 by L.Johnson & J.van der Zijp.  All Rights Reserved.  *
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
#include "FXArray.h"
#include "FXHash.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXTextCodec.h"


/*
  Notes:

  - IANA defined mime names for character sets are found in:

        http://www.iana.org/assignments/character-sets.

  - Need API to count expected #characters needed for decode.
  - Unicode Transformation Formats information http://czyborra.com/utf.
  - The decoder should replace a malformed sequence with U+FFFD
  - See also: RFC 2279.
  - See RFC-1759 for printer MIB Enums.
  - FIXME still unhappy with FXTextCodec's API's.  Change in FOX 1.8.
  - Note also, representation (e.g. 8, 16, 32 bit) should be independent
    of encoding/decoding; need to be able to store ascii in wide characters.
*/




/*******************************************************************************/

namespace FX {


// Base class is not instantiated
FXIMPLEMENT_ABSTRACT(FXTextCodec,FXObject,nullptr,0)


/*********  Convert arrays of characters from multi-byte to unicode  ***********/


// Convert multi-byte characters from src to single wide character
FXint FXTextCodec::mb2wc(FXwchar& w,const FXchar* src,FXint nsrc) const {
  if(0<nsrc){
    FXint nr=wclen(src);
    if(nr<=nsrc){
      w=wc(src);
      return nr;
      }
    }
  return 0;
  }


// Count number of utf8 characters needed to convert multi-byte characters from src
FXint FXTextCodec::mb2utflen(const FXchar* src,FXint nsrc) const {
  FXint nr,len=0;
  FXwchar w;
  while(0<nsrc){
    nr=mb2wc(w,src,nsrc);
    if(nr<=0) break;
    src+=nr;
    nsrc-=nr;
    if(w==0xFEFF) continue;     // Byte order mark
    len+=FX::wc2utf(w);
    }
  return len;
  }


// Count utf8 characters needed to convert multi-byte characters from src
FXint FXTextCodec::mb2utflen(const FXString& src) const {
  return mb2utflen(src.text(),src.length());
  }


// Convert multi-byte characters from src to utf8 characters at dst
FXint FXTextCodec::mb2utf(FXchar* dst,FXint ndst,const FXchar* src,FXint nsrc) const {
  FXint nr,nw,len=0;
  FXwchar w;
  while(0<nsrc && 0<ndst){
    nr=mb2wc(w,src,nsrc);
    if(nr<=0) break;
    if(FX::wc2utf(w)>ndst) break;
    nw=FX::wc2utf(dst,w);
    src+=nr;
    nsrc-=nr;
    len+=nw;
    dst+=nw;
    ndst-=nw;
    }
  return len;
  }


// Convert multi-byte characters from src to utf8 characters at dst
FXint FXTextCodec::mb2utf(FXchar* dst,FXint ndst,const FXchar* src) const {
  return mb2utf(dst,ndst,src,strlen(src));
  }


// Convert multi-byte characters from src to utf8 characters at dst
FXint FXTextCodec::mb2utf(FXchar* dst,FXint ndst,const FXString& src) const {
  return mb2utf(dst,ndst,src.text(),src.length());
  }


// Convert multi-byte characters from src to utf8 string
FXString FXTextCodec::mb2utf(const FXchar* src,FXint nsrc) const {
  FXint len=mb2utflen(src,nsrc);
  FXString result;
  if(0<len){
    result.length(len);
    mb2utf(&result[0],len,src,nsrc);
    }
  return result;
  }


// Convert multi-byte characters from src to utf8 string
FXString FXTextCodec::mb2utf(const FXchar* src) const {
  return mb2utf(src,strlen(src));
  }


// Convert multi-byte string to utf8 string
FXString FXTextCodec::mb2utf(const FXString& src) const {
  return mb2utf(src.text(),src.length());
  }


/*********  Convert arrays of characters from unicode to multi-byte  ***********/


// Convert single wide character to multi-byte characters at dst
FXint FXTextCodec::wc2mb(FXchar* dst,FXint ndst,FXwchar w) const {
  if(0<ndst){
    FXint nw=FX::wc2utf(w);
    if(nw<=ndst){
      wc2utf(dst,w);            // FIXME there's some asymmetry here w.r.t. utf2wc (doesn't exist!)
      return nw;
      }
    }
  return 0;
  }


// Count multi-byte characters characters needed to convert utf8 from src
FXint FXTextCodec::utf2mblen(const FXchar* src,FXint nsrc) const {
  FXint nr,len=0;
  FXchar buffer[64];
  FXwchar w;
  while(0<nsrc){
    nr=FX::wclen(src);
    if(nr>nsrc) break;
    w=wc(src);
    len+=wc2mb(buffer,sizeof(buffer),w);
    src+=nr;
    nsrc-=nr;
    }
  return len;
  }


// Count multi-byte characters characters needed to convert utf8 from src
FXint FXTextCodec::utf2mblen(const FXString& src) const {
  return utf2mblen(src.text(),src.length());
  }


// Convert utf8 characters at src to multi-byte characters at dst
FXint FXTextCodec::utf2mb(FXchar* dst,FXint ndst,const FXchar* src,FXint nsrc) const {
  FXint nr,nw,len=0;
  FXwchar w;
  while(0<nsrc && 0<ndst){
    nr=FX::wclen(src);
    if(nr>nsrc) break;
    w=wc(src);
    nw=wc2mb(dst,ndst,w);
    if(nw<=0) break;
    src+=nr;
    nsrc-=nr;
    len+=nw;
    dst+=nw;
    ndst-=nw;
    }
  return len;
  }


// Convert utf8 characters at src to multi-byte characters at dst
FXint FXTextCodec::utf2mb(FXchar* dst,FXint ndst,const FXchar* src) const {
  return utf2mb(dst,ndst,src,strlen(src));
  }


// Convert utf8 characters at src to multi-byte characters at dst
FXint FXTextCodec::utf2mb(FXchar* dst,FXint ndst,const FXString& src) const {
  return utf2mb(dst,ndst,src.text(),src.length());
  }


// Convert utf8 characters at src to multi-byte string
FXString FXTextCodec::utf2mb(const FXchar* src,FXint nsrc) const {
  FXint len=utf2mblen(src,nsrc)+utf2mblen(FXString::null,1);   // Reserve extra space for explicit conversion of end-of-string
  FXint end;
  FXString result;
  result.length(len);
  end=utf2mb(&result[0],len,src,nsrc);                                  // Convert the text itself
  utf2mb(&result[end],len-end,FXString::null,1);                        // And append converted end-of-string
  return result;
  }


// Convert utf8 characters at src to multi-byte string
FXString FXTextCodec::utf2mb(const FXchar* src) const {
  return utf2mb(src,strlen(src));
  }


// Convert utf8 string to multi-byte string
FXString FXTextCodec::utf2mb(const FXString& src) const {
  return utf2mb(src.text(),src.length());
  }


}
