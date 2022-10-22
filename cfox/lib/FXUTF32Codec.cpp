/********************************************************************************
*                                                                               *
*                      U T F - 3 2  T e x t   C o d e c                         *
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
#include "FXUTF32Codec.h"


/*
  Notes:
*/

/*******************************************************************************/

namespace FX {


const FXwchar BOM_BE=0x0000FEFF;
const FXwchar BOM_LE=0xFFFE0000;


FXIMPLEMENT(FXUTF32BECodec,FXTextCodec,nullptr,0)


// Convert from utf32be
FXint FXUTF32BECodec::mb2wc(FXwchar& wc,const FXchar* src,FXint nsrc) const {
  if(nsrc<4) return -4;
#if (FOX_BIGENDIAN == 1)
  ((FXuchar*)&wc)[0]=src[0];
  ((FXuchar*)&wc)[1]=src[1];
  ((FXuchar*)&wc)[2]=src[2];
  ((FXuchar*)&wc)[3]=src[3];
#else
  ((FXuchar*)&wc)[3]=src[0];
  ((FXuchar*)&wc)[2]=src[1];
  ((FXuchar*)&wc)[1]=src[2];
  ((FXuchar*)&wc)[0]=src[3];
#endif
  return 4;
  }


// Convert to utf32be
FXint FXUTF32BECodec::wc2mb(FXchar* dst,FXint ndst,FXwchar wc) const {
  if(ndst<4) return -4;
#if (FOX_BIGENDIAN == 1)
  dst[0]=((FXuchar*)&wc)[0];
  dst[1]=((FXuchar*)&wc)[1];
  dst[2]=((FXuchar*)&wc)[2];
  dst[3]=((FXuchar*)&wc)[3];
#else
  dst[0]=((FXuchar*)&wc)[3];
  dst[1]=((FXuchar*)&wc)[2];
  dst[2]=((FXuchar*)&wc)[1];
  dst[3]=((FXuchar*)&wc)[0];
#endif
  return 4;
  }


// Return name
const FXchar* FXUTF32BECodec::name() const {
  return "UTF-32BE";
  }


// Return the IANA mime name for this codec
const FXchar* FXUTF32BECodec::mimeName() const {
  return "UTF-32BE";
  }


// Return code for UTF-32
FXint FXUTF32BECodec::mibEnum() const {
  return 1018;
  }


// Return aliases
const FXchar* const* FXUTF32BECodec::aliases() const {
  static const FXchar *const list[]={"UTF-32BE",nullptr};
  return list;
  }


/*******************************************************************************/

FXIMPLEMENT(FXUTF32LECodec,FXTextCodec,nullptr,0)


// Convert from utf32le
FXint FXUTF32LECodec::mb2wc(FXwchar& wc,const FXchar* src,FXint nsrc) const {
  if(nsrc<4) return -4;
#if (FOX_BIGENDIAN == 1)
  ((FXuchar*)&wc)[0]=src[3];
  ((FXuchar*)&wc)[1]=src[2];
  ((FXuchar*)&wc)[2]=src[1];
  ((FXuchar*)&wc)[3]=src[0];
#else
  ((FXuchar*)&wc)[3]=src[3];
  ((FXuchar*)&wc)[2]=src[2];
  ((FXuchar*)&wc)[1]=src[1];
  ((FXuchar*)&wc)[0]=src[0];
#endif
  return 4;
  }


// Convert to utf32le
FXint FXUTF32LECodec::wc2mb(FXchar* dst,FXint ndst,FXwchar wc) const {
  if(ndst<4) return -4;
#if (FOX_BIGENDIAN == 1)
  dst[3]=((FXuchar*)&wc)[0];
  dst[2]=((FXuchar*)&wc)[1];
  dst[1]=((FXuchar*)&wc)[2];
  dst[0]=((FXuchar*)&wc)[3];
#else
  dst[3]=((FXuchar*)&wc)[3];
  dst[2]=((FXuchar*)&wc)[2];
  dst[1]=((FXuchar*)&wc)[1];
  dst[0]=((FXuchar*)&wc)[0];
#endif
  return 4;
  }


// Return name
const FXchar* FXUTF32LECodec::name() const {
  return "UTF-32LE";
  }


// Return the IANA mime name for this codec
const FXchar* FXUTF32LECodec::mimeName() const {
  return "UTF-32LE";
  }


// Return code for UTF-32
FXint FXUTF32LECodec::mibEnum() const {
  return 1019;
  }


// Return aliases
const FXchar* const* FXUTF32LECodec::aliases() const {
  static const FXchar *const list[]={"UTF-32LE",nullptr};
  return list;
  }


/*******************************************************************************/

FXIMPLEMENT(FXUTF32Codec,FXTextCodec,nullptr,0)


// Convert utf32
FXint FXUTF32Codec::mb2wc(FXwchar& wc,const FXchar* src,FXint nsrc) const {
  if(nsrc<4) return -4;
#if (FOX_BIGENDIAN == 1)
  ((FXuchar*)&wc)[0]=src[0];
  ((FXuchar*)&wc)[1]=src[1];
  ((FXuchar*)&wc)[2]=src[2];
  ((FXuchar*)&wc)[3]=src[3];
#else
  ((FXuchar*)&wc)[3]=src[0];
  ((FXuchar*)&wc)[2]=src[1];
  ((FXuchar*)&wc)[1]=src[2];
  ((FXuchar*)&wc)[0]=src[3];
#endif
  if(wc==BOM_BE){
    if(nsrc<8) return -8;
#if (FOX_BIGENDIAN == 1)
    ((FXuchar*)&wc)[0]=src[4];
    ((FXuchar*)&wc)[1]=src[5];
    ((FXuchar*)&wc)[2]=src[6];
    ((FXuchar*)&wc)[3]=src[7];
#else
    ((FXuchar*)&wc)[3]=src[4];
    ((FXuchar*)&wc)[2]=src[5];
    ((FXuchar*)&wc)[1]=src[6];
    ((FXuchar*)&wc)[0]=src[7];
#endif
    return 8;
    }
  if(wc==BOM_LE){
    if(nsrc<8) return -8;
#if (FOX_BIGENDIAN == 1)
    ((FXuchar*)&wc)[0]=src[7];
    ((FXuchar*)&wc)[1]=src[6];
    ((FXuchar*)&wc)[2]=src[5];
    ((FXuchar*)&wc)[3]=src[4];
#else
    ((FXuchar*)&wc)[3]=src[7];
    ((FXuchar*)&wc)[2]=src[6];
    ((FXuchar*)&wc)[1]=src[5];
    ((FXuchar*)&wc)[0]=src[4];
#endif
    return 8;
    }
  return 4;
  }


// Number of bytes for wide character
static inline FXint utflen(FXwchar w){
  if(w<0x80) return 1;
  if(w<0x800) return 2;
  if(w<0x10000) return 3;
  if(w<0x200000) return 4;
  if(w<0x4000000) return 5;
  return 6;
  }


// Count number of utf8 characters needed to convert multi-byte characters from src
FXint FXUTF32Codec::mb2utflen(const FXchar* src,FXint nsrc) const {
  FXint len=0;
  FXwchar w;
  if(src && 0<nsrc){
    if(nsrc<4) return -4;
#if (FOX_BIGENDIAN == 1)
    ((FXuchar*)&w)[0]=src[0];
    ((FXuchar*)&w)[1]=src[1];
    ((FXuchar*)&w)[2]=src[2];
    ((FXuchar*)&w)[3]=src[3];
#else
    ((FXuchar*)&w)[3]=src[0];
    ((FXuchar*)&w)[2]=src[1];
    ((FXuchar*)&w)[1]=src[2];
    ((FXuchar*)&w)[0]=src[3];
#endif
    if(w!=BOM_LE){          // Big-endian (default)
      if(w==BOM_BE){
        src+=4;
        nsrc-=4;
        }
      while(0<nsrc){
        if(nsrc<4) return -4;
#if (FOX_BIGENDIAN == 1)
        ((FXuchar*)&w)[0]=src[0];
        ((FXuchar*)&w)[1]=src[1];
        ((FXuchar*)&w)[2]=src[2];
        ((FXuchar*)&w)[3]=src[3];
#else
        ((FXuchar*)&w)[3]=src[0];
        ((FXuchar*)&w)[2]=src[1];
        ((FXuchar*)&w)[1]=src[2];
        ((FXuchar*)&w)[0]=src[3];
#endif
        src+=4;
        nsrc-=4;
        len+=utflen(w);
        }
      }
    else{                       // Little-endian
      src+=4;
      nsrc-=4;
      while(0<nsrc){
        if(nsrc<4) return -4;
#if (FOX_BIGENDIAN == 1)
        ((FXuchar*)&w)[0]=src[3];
        ((FXuchar*)&w)[1]=src[2];
        ((FXuchar*)&w)[2]=src[1];
        ((FXuchar*)&w)[3]=src[0];
#else
        ((FXuchar*)&w)[3]=src[3];
        ((FXuchar*)&w)[2]=src[2];
        ((FXuchar*)&w)[1]=src[1];
        ((FXuchar*)&w)[0]=src[0];
#endif
        src+=4;
        nsrc-=4;
        len+=utflen(w);
        }
      }
    }
  return len;
  }


// Convert multi-byte characters from src to utf8 characters at dst
FXint FXUTF32Codec::mb2utf(FXchar* dst,FXint ndst,const FXchar* src,FXint nsrc) const {
  FXint nw,len=0;
  FXwchar w;
  if(dst && src && 0<nsrc){
    if(nsrc<4) return -4;
#if (FOX_BIGENDIAN == 1)
    ((FXuchar*)&w)[0]=src[0];
    ((FXuchar*)&w)[1]=src[1];
    ((FXuchar*)&w)[2]=src[2];
    ((FXuchar*)&w)[3]=src[3];
#else
    ((FXuchar*)&w)[3]=src[0];
    ((FXuchar*)&w)[2]=src[1];
    ((FXuchar*)&w)[1]=src[2];
    ((FXuchar*)&w)[0]=src[3];
#endif
    if(w!=BOM_LE){          // Big-endian (default)
      if(w==BOM_BE){
        src+=4;
        nsrc-=4;
        }
      while(0<nsrc){
        if(nsrc<4) return -4;
#if (FOX_BIGENDIAN == 1)
        ((FXuchar*)&w)[0]=src[0];
        ((FXuchar*)&w)[1]=src[1];
        ((FXuchar*)&w)[2]=src[2];
        ((FXuchar*)&w)[3]=src[3];
#else
        ((FXuchar*)&w)[3]=src[0];
        ((FXuchar*)&w)[2]=src[1];
        ((FXuchar*)&w)[1]=src[2];
        ((FXuchar*)&w)[0]=src[3];
#endif
        if(FX::wc2utf(w)>ndst) break;
        nw=FX::wc2utf(dst,w);
        src+=4;
        nsrc-=4;
        len+=nw;
        dst+=nw;
        ndst-=nw;
        }
      }
    else{                       // Little-endian
      src+=4;
      nsrc-=4;
      while(0<nsrc){
        if(nsrc<4) return -4;
#if (FOX_BIGENDIAN == 1)
        ((FXuchar*)&w)[0]=src[3];
        ((FXuchar*)&w)[1]=src[2];
        ((FXuchar*)&w)[2]=src[1];
        ((FXuchar*)&w)[3]=src[0];
#else
        ((FXuchar*)&w)[3]=src[3];
        ((FXuchar*)&w)[2]=src[2];
        ((FXuchar*)&w)[1]=src[1];
        ((FXuchar*)&w)[0]=src[0];
#endif
        if(FX::wc2utf(w)>ndst) break;
        nw=FX::wc2utf(dst,w);
        src+=4;
        nsrc-=4;
        len+=nw;
        dst+=nw;
        ndst-=nw;
        }
      }
    }
  return len;
  }


// Convert to utf32
FXint FXUTF32Codec::wc2mb(FXchar* dst,FXint ndst,FXwchar wc) const {
  if(ndst<4) return 0;
#if (FOX_BIGENDIAN == 1)
  dst[0]=((FXuchar*)&wc)[0];
  dst[1]=((FXuchar*)&wc)[1];
  dst[2]=((FXuchar*)&wc)[2];
  dst[3]=((FXuchar*)&wc)[3];
#else
  dst[0]=((FXuchar*)&wc)[3];
  dst[1]=((FXuchar*)&wc)[2];
  dst[2]=((FXuchar*)&wc)[1];
  dst[3]=((FXuchar*)&wc)[0];
#endif
  return 4;
  }


// Count multi-byte characters characters needed to convert utf8 from src
FXint FXUTF32Codec::utf2mblen(const FXchar* src,FXint nsrc) const {
  FXint nr,len=0;
  if(src && 0<nsrc){
    len+=4;
    while(0<nsrc){
      nr=FX::wclen(src);
      if(nr>nsrc) break;
      src+=nr;
      nsrc-=nr;
      len+=4;
      }
    }
  return len;
  }


// Convert utf8 characters at src to multi-byte characters at dst
FXint FXUTF32Codec::utf2mb(FXchar* dst,FXint ndst,const FXchar* src,FXint nsrc) const {
  FXint nr,len=0;
  FXwchar w;
  if(dst && src && 0<nsrc){
    if(ndst<4) return 0;
    dst[0]='\0';
    dst[1]='\0';
    dst[2]='\xFE';
    dst[3]='\xFF';
    dst+=4;
    len+=4;
    while(0<nsrc){
      nr=FX::wclen(src);
      if(nr>nsrc) break;
      w=wc(src);
      if(ndst<4) break;
#if (FOX_BIGENDIAN == 1)
      dst[0]=((FXuchar*)&w)[0];
      dst[1]=((FXuchar*)&w)[1];
      dst[2]=((FXuchar*)&w)[2];
      dst[3]=((FXuchar*)&w)[3];
#else
      dst[0]=((FXuchar*)&w)[3];
      dst[1]=((FXuchar*)&w)[2];
      dst[2]=((FXuchar*)&w)[1];
      dst[3]=((FXuchar*)&w)[0];
#endif
      src+=nr;
      nsrc-=nr;
      len+=4;
      dst+=4;
      ndst-=4;
      }
    }
  return len;
  }


// Return name
const FXchar* FXUTF32Codec::name() const {
  return "UTF-32";
  }


// Return the IANA mime name for this codec
const FXchar* FXUTF32Codec::mimeName() const {
  return "UTF-32";
  }


// Return code for UTF-32
FXint FXUTF32Codec::mibEnum() const {
  return 1017;
  }


// Return aliases
const FXchar* const* FXUTF32Codec::aliases() const {
  static const FXchar *const list[]={"UTF-32",nullptr};
  return list;
  }

}

