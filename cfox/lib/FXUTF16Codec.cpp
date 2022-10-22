/********************************************************************************
*                                                                               *
*                      U T F - 1 6  T e x t   C o d e c                         *
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
#include "fxmath.h"
#include "FXArray.h"
#include "FXHash.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXTextCodec.h"
#include "FXUTF16Codec.h"


/*
  Notes:
  - Default byte-order in absence of BOM is big-endian.
  - UTF-16BE and UTF-16LE do not expect BOM, always same
    endianness.
  - Single character wc2mb does not write BOM.
  - Single character mb2wc does, however, read over BOM.
  - Error when trying to write surrogate character.
*/



/*******************************************************************************/

namespace FX {

const FXnchar BOM_BE=0xFEFF;
const FXnchar BOM_LE=0xFFFE;


FXIMPLEMENT(FXUTF16BECodec,FXTextCodec,nullptr,0)


// Convert from utf16be
FXint FXUTF16BECodec::mb2wc(FXwchar& wc,const FXchar* src,FXint nsrc) const {
  const FXint SURROGATE_OFFSET=0x10000-(0xD800<<10)-0xDC00;
  FXwchar w;
  if(nsrc<2) return 0;
  wc=(((FXuchar)src[0])<<8)|((FXuchar)src[1]);
  if(__unlikely(leadUTF16(wc))){
    if(nsrc<4) return 0;
    w=(((FXuchar)src[2])<<8)|((FXuchar)src[3]);
    if(__unlikely(!followUTF16(w))) return 0;
    wc=(wc<<10)+w+SURROGATE_OFFSET;
    return 4;
    }
  return 2;
  }


// Convert to utf16be
FXint FXUTF16BECodec::wc2mb(FXchar* dst,FXint ndst,FXwchar wc) const {
  const FXint LEAD_OFFSET=0xD800-(0x10000>>10);
  const FXint TAIL_OFFSET=0xDC00;
  FXwchar w;
  if(ndst<2) return 0;
  if(0xD800<=wc && wc<0xE000) return 0;
  dst[0]=(FXchar)(wc>>8);
  dst[1]=(FXchar)(wc);
  if(__unlikely(0xFFFF<wc)){
    if(ndst<4) return 0;
    w=(wc>>10)+LEAD_OFFSET;
    dst[0]=(FXchar)(w>>8);
    dst[1]=(FXchar)(w);
    w=(wc&0x3FF)+TAIL_OFFSET;
    dst[2]=(FXchar)(w>>8);
    dst[3]=(FXchar)(w);
    return 4;
    }
  return 2;
  }


// Return the name for this codec
const FXchar* FXUTF16BECodec::name() const {
  return "UTF-16BE";
  }


// Return the IANA mime name for this codec
const FXchar* FXUTF16BECodec::mimeName() const {
  return "UTF-16BE";
  }


// Return code for UTF-16
FXint FXUTF16BECodec::mibEnum() const {
  return 1013;
  }


// Return aliases
const FXchar* const* FXUTF16BECodec::aliases() const {
  static const FXchar *const list[]={"iso10646-1","UTF-16BE",nullptr};
  return list;
  }


/*******************************************************************************/

FXIMPLEMENT(FXUTF16LECodec,FXTextCodec,nullptr,0)


// Convert from utf16le
FXint FXUTF16LECodec::mb2wc(FXwchar& wc,const FXchar* src,FXint nsrc) const {
  const FXint SURROGATE_OFFSET=0x10000-(0xD800<<10)-0xDC00;
  FXwchar w;
  if(nsrc<2) return 0;
  wc=(((FXuchar)src[1])<<8)|((FXuchar)src[0]);
  if(__unlikely(leadUTF16(wc))){
    if(nsrc<4) return 0;
    w=(((FXuchar)src[3])<<8)|((FXuchar)src[2]);
    if(__unlikely(!followUTF16(w))) return 0;
    wc=(wc<<10)+w+SURROGATE_OFFSET;
    return 4;
    }
  return 2;
  }


// Convert to utf16le
FXint FXUTF16LECodec::wc2mb(FXchar* dst,FXint ndst,FXwchar wc) const {
  const FXint LEAD_OFFSET=0xD800-(0x10000>>10);
  const FXint TAIL_OFFSET=0xDC00;
  FXwchar w;
  if(ndst<2) return 0;
  if(0xD800<=wc && wc<0xE000) return 0;
  dst[0]=(FXchar)(wc);
  dst[1]=(FXchar)(wc>>8);
  if(0xFFFF<wc){
    if(ndst<4) return 0;
    w=(wc>>10)+LEAD_OFFSET;
    dst[0]=(FXchar)(w);
    dst[1]=(FXchar)(w>>8);
    w=(wc&0x3FF)+TAIL_OFFSET;
    dst[2]=(FXchar)(w);
    dst[3]=(FXchar)(w>>8);
    return 4;
    }
  return 2;
  }


// Return the name for this codec
const FXchar* FXUTF16LECodec::name() const {
  return "UTF-16LE";
  }


// Return the IANA mime name for this codec
const FXchar* FXUTF16LECodec::mimeName() const {
  return "UTF-16LE";
  }


// Return code for UTF-16
FXint FXUTF16LECodec::mibEnum() const {
  return 1014;
  }


// Return aliases
const FXchar* const* FXUTF16LECodec::aliases() const {
  static const FXchar *const list[]={"UTF-16LE",nullptr};
  return list;
  }

/*******************************************************************************/

FXIMPLEMENT(FXUTF16Codec,FXTextCodec,nullptr,0)


// Convert utf16 but strip BOM
FXint FXUTF16Codec::mb2wc(FXwchar& wc,const FXchar* src,FXint nsrc) const {
  const FXint SURROGATE_OFFSET=0x10000-(0xD800<<10)-0xDC00;
  const FXuchar *s=(const FXuchar*)src;
  FXwchar w;
  if(nsrc<2) return 0;
  wc=(s[0]<<8)|s[1];
  if(wc==BOM_BE){
    if(nsrc<4) return 0;
    wc=(s[2]<<8)|s[3];
    if(__unlikely(leadUTF16(wc))){
      if(nsrc<6) return 0;
      w=(s[4]<<8)|s[5];
      if(__unlikely(!followUTF16(w))) return 0;
      wc=(wc<<10)+w+SURROGATE_OFFSET;
      return 6;
      }
    return 4;
    }
  if(wc==BOM_LE){
    if(nsrc<4) return 0;
    wc=(s[3]<<8)|s[2];
    if(__unlikely(leadUTF16(wc))){
      if(nsrc<6) return 0;
      w=(s[5]<<8)|s[4];
      if(__unlikely(!followUTF16(w))) return 0;
      wc=(wc<<10)+w+SURROGATE_OFFSET;
      return 6;
      }
    return 4;
    }
  if(__unlikely(leadUTF16(wc))){
    if(nsrc<4) return 0;
    w=(s[2]<<8)|s[3];
    if(__unlikely(!followUTF16(w))) return 0;
    wc=(wc<<10)+w+SURROGATE_OFFSET;
    return 4;
    }
  return 2;
  }


// Count number of utf8 characters needed to convert multi-byte characters from src
FXint FXUTF16Codec::mb2utflen(const FXchar* src,FXint nsrc) const {
  const FXint SURROGATE_OFFSET=0x10000-(0xD800<<10)-0xDC00;
  FXint len=0;
  FXwchar w,v;
  if(src && 0<nsrc){
    if(nsrc<2) return -2;
    w=(((FXuchar)src[0])<<8)|((FXuchar)src[1]);
    if(w!=BOM_LE){              // Big-endian (default)
      if(w==BOM_BE){
        src+=2;
        nsrc-=2;
        }
      while(0<nsrc){
        if(nsrc<2) return -2;
        w=(((FXuchar)src[0])<<8)|((FXuchar)src[1]);
        src+=2;
        nsrc-=2;
        if(__unlikely(leadUTF16(w))){
          if(nsrc<2) return -2;
          v=(((FXuchar)src[0])<<8)|((FXuchar)src[1]);
          if(__unlikely(!followUTF16(v))) return 0;
          w=(w<<10)+v+SURROGATE_OFFSET;
          src+=2;
          nsrc-=2;
          }
        len+=FX::wc2utf(w);
        }
      }
    else{                       // Little-endian
      src+=2;
      nsrc-=2;
      while(0<nsrc){
        if(nsrc<2) return -2;
        w=(((FXuchar)src[1])<<8)|((FXuchar)src[0]);
        src+=2;
        nsrc-=2;
        if(__unlikely(leadUTF16(w))){
          if(nsrc<2) return -2;
          v=(((FXuchar)src[1])<<8)|((FXuchar)src[0]);
          if(__unlikely(!followUTF16(v))) return 0;
          w=(w<<10)+v+SURROGATE_OFFSET;
          src+=2;
          nsrc-=2;
          }
        len+=FX::wc2utf(w);
        }
      }
    }
  return len;
  }


// Convert multi-byte characters from src to utf8 characters at dst
FXint FXUTF16Codec::mb2utf(FXchar* dst,FXint ndst,const FXchar* src,FXint nsrc) const {
  const FXint SURROGATE_OFFSET=0x10000-(0xD800<<10)-0xDC00;
  FXint nw,len=0;
  FXwchar w,v;
  if(dst && src && 0<nsrc){
    if(nsrc<2) return -2;
    w=(((FXuchar)src[0])<<8)|((FXuchar)src[1]);
    if(w!=BOM_LE){              // Big-endian (default)
      if(w==BOM_BE){
        src+=2;
        nsrc-=2;
        }
      while(0<nsrc){
        if(nsrc<2) return -2;
        w=(((FXuchar)src[0])<<8)|((FXuchar)src[1]);
        src+=2;
        nsrc-=2;
        if(__unlikely(leadUTF16(w))){
          if(nsrc<2) return -2;
          v=(((FXuchar)src[0])<<8)|((FXuchar)src[1]);
          if(__unlikely(!followUTF16(v))) return 0;
          w=(w<<10)+v+SURROGATE_OFFSET;
          src+=2;
          nsrc-=2;
          }
        if(FX::wc2utf(w)>ndst) break;
        nw=wc2utf(dst,w);
        len+=nw;
        dst+=nw;
        ndst-=nw;
        }
      }
    else{                       // Little-endian
      src+=2;
      nsrc-=2;
      while(0<nsrc){
        if(nsrc<2) return -2;
        w=(((FXuchar)src[1])<<8)|((FXuchar)src[0]);
        src+=2;
        nsrc-=2;
        if(__unlikely(leadUTF16(w))){
          if(nsrc<2) return -2;
          v=(((FXuchar)src[1])<<8)|((FXuchar)src[0]);
          if(__unlikely(!followUTF16(v))) return 0;
          w=(w<<10)+v+SURROGATE_OFFSET;
          src+=2;
          nsrc-=2;
          }
        if(FX::wc2utf(w)>ndst) break;
        nw=wc2utf(dst,w);
        len+=nw;
        dst+=nw;
        ndst-=nw;
        }
      }
    }
  return len;
  }


// Convert to utf16
FXint FXUTF16Codec::wc2mb(FXchar* dst,FXint ndst,FXwchar wc) const {
  const FXint LEAD_OFFSET=0xD800-(0x10000>>10);
  const FXint TAIL_OFFSET=0xDC00;
  FXwchar w;
  if(0xD800<=wc && wc<0xE000) return 0;
  if(ndst<2) return -2;
  dst[0]=(FXchar)(wc>>8);
  dst[1]=(FXchar)(wc);
  if(0xFFFF<wc){
    if(ndst<4) return -4;
    w=(wc>>10)+LEAD_OFFSET;
    dst[0]=(FXchar)(w>>8);
    dst[1]=(FXchar)(w);
    w=(wc&0x3FF)+TAIL_OFFSET;
    dst[2]=(FXchar)(w>>8);
    dst[3]=(FXchar)(w);
    return 4;
    }
  return 2;
  }


// Count multi-byte characters characters needed to convert utf8 from src
FXint FXUTF16Codec::utf2mblen(const FXchar* src,FXint nsrc) const {
  FXint nr,len=0;
  FXchar buffer[64];
  FXwchar w;
  if(0<nsrc){
    len+=2;             // BOM
    while(0<nsrc){
      nr=FX::wclen(src);
      if(nr>nsrc) break;
      w=wc(src);
      len+=wc2mb(buffer,sizeof(buffer),w);
      src+=nr;
      nsrc-=nr;
      }
    }
  return len;
  }


// Convert utf8 characters at src to multi-byte characters at dst
FXint FXUTF16Codec::utf2mb(FXchar* dst,FXint ndst,const FXchar* src,FXint nsrc) const {
  FXint nr,nw,len=0;
  FXwchar w;
  if(dst && src && 0<nsrc){
    dst[0]='\xFE';
    dst[1]='\xFF';
    dst+=2;
    len+=2;
    while(0<nsrc){
      nr=FX::wclen(src);
      if(nr>nsrc) break;
      w=wc(src);
      nw=wc2mb(dst,ndst,w);
      if(nw<=0) return nw;
      src+=nr;
      nsrc-=nr;
      len+=nw;
      dst+=nw;
      ndst-=nw;
      }
    }
  return len;
  }


// Return the name for this codec
const FXchar* FXUTF16Codec::name() const {
  return "UTF-16";
  }


// Return the IANA mime name for this codec
const FXchar* FXUTF16Codec::mimeName() const {
  return "UTF-16";
  }


// Return code for UTF-16
FXint FXUTF16Codec::mibEnum() const {
  return 1015;
  }


// Return aliases
const FXchar* const* FXUTF16Codec::aliases() const {
  static const FXchar *const list[]={"UTF-16",nullptr};
  return list;
  }


}

