/********************************************************************************
*                                                                               *
*                      U T F - 8  T e x t   C o d e c                           *
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
#include "FXUTF8Codec.h"


/*
  Notes:
  - This is the utf-8 codec used for external inputs; it takes care of
    things like BOM's (0xFEFF or "\xEFxBB\xBF" )being inserted.
*/

/*******************************************************************************/

namespace FX {


// Convert utf8 but strip BOM
FXint FXUTF8Codec::mb2wc(FXwchar& w,const FXchar* src,FXint nsrc) const {
  FXint n=0;
a:if(n>=nsrc) return 0;
  w=(FXuchar)src[n++];
  if(__unlikely(0x80<=w)){
    if(__unlikely(w<0xC0)) return 0;
    if(__unlikely(n>=nsrc)) return 0;
    if(__unlikely(!followUTF8(src[n]))) return 0;
    w=(w<<6)^(FXuchar)src[n++]^0x3080;
    if(__unlikely(0x800<=w)){
      if(__unlikely(n>=nsrc)) return 0;
      if(__unlikely(!followUTF8(src[n]))) return 0;
      w=(w<<6)^(FXuchar)src[n++]^0x20080;
      if(__unlikely(0x10000<=w)){
        if(__unlikely(n>=nsrc)) return 0;
        if(__unlikely(!followUTF8(src[n]))) return 0;
        w=(w<<6)^(FXuchar)src[n++]^0x400080;
        if(__unlikely(0x110000<=w)) return 0;
        }
      if(__unlikely(w==0xFEFF)) goto a;
      }
    }
  return n;
  }


// Convert to utf8
FXint FXUTF8Codec::wc2mb(FXchar* dst,FXint ndst,FXwchar w) const {
  if(__unlikely(ndst<wc2utf(w))) return 0;
  return wc2utf(dst,w);
  }


// Return name
const FXchar* FXUTF8Codec::name() const {
  return "UTF-8";
  }

// Return the IANA mime name for this codec
const FXchar* FXUTF8Codec::mimeName() const {
  return "UTF-8";
  }


// Return code for UTF-8
FXint FXUTF8Codec::mibEnum() const {
  return 106;
  }


// Return aliases
const FXchar* const* FXUTF8Codec::aliases() const {
  static const FXchar *const list[]={"UTF-8",nullptr};
  return list;
  }

}

