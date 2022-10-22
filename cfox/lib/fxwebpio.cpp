/********************************************************************************
*                                                                               *
*                    W e b - P   I m a g e   I n p u t / O u t p u t            *
*                                                                               *
*********************************************************************************
* Copyright (C) 2011,2022 by S. Jansen & J. van der Zijp.  All Rights Reserved. *
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
#include "FXArray.h"
#include "FXHash.h"
#include "FXElement.h"
#include "FXStream.h"
#ifdef HAVE_WEBP_H
#include "webp/types.h"
#include "webp/encode.h"
#include "webp/decode.h"
#endif



/*
  Notes:
  - Support for Google WEBP image file compression.
  - Reference:
        http://code.google.com/speed/webp/

  - Header layout:

      Offset    Description

       0        "RIFF" 4-byte tag
       4        Size of image data (including metadata) starting at offset 8
       8        "WEBP" the form-type signature
      12        "VP8 " 4-bytes tags, describing the raw video format used
      16        Size of the raw VP8 image data, starting at offset 20; should be even
      20        The VP8 bytes...

*/


using namespace FX;

/*******************************************************************************/

namespace FX {

#ifndef FXLOADWEBP
extern FXAPI FXbool fxcheckWEBP(FXStream& store);
extern FXAPI FXbool fxloadWEBP(FXStream& store,FXColor*& data,FXint& width,FXint& height);
extern FXAPI FXbool fxsaveWEBP(FXStream& store,const FXColor* data,FXint width,FXint height,FXfloat quality);
#endif

/*******************************************************************************/

#ifdef HAVE_WEBP_H

// Check if stream contains a WebP
FXbool fxcheckWEBP(FXStream& store){
  FXuchar signature[16];
  store.load(signature,16);
  store.position(-16,FXFromCurrent);
  return (signature[ 0]=='R' && signature[ 1]=='I' && signature[ 2]=='F' && signature[ 3]=='F' &&
          signature[ 8]=='W' && signature[ 9]=='E' && signature[10]=='B' && signature[11]=='P' &&
          signature[12]=='V' && signature[13]=='P' && signature[14]=='8' && signature[15]==' ');
  }

/*******************************************************************************/

// Load a WebP image
FXbool fxloadWEBP(FXStream& store,FXColor*& data,FXint& width,FXint& height){
  FXuchar *buffer=nullptr;
  FXlong   start;
  FXuint   size;
  FXbool   swap;

  // Everyone remember where we parked.
  start=store.position();

  // Go directly to size chunk
  store.position(4,FXFromCurrent);

  // Read size in little endian
  swap=store.swapBytes();
  store.setBigEndian(false);
  store >> size;
  size+=8; // add riff and size fields
  store.setBigEndian(swap);

  // Start over
  store.position(start);

  // Allocate Buffer
  if(allocElms(buffer,size)){

    // Read the complete data
    store.load(buffer,size);

    // Get Info
    if(WebPGetInfo(buffer,size,&width,&height) && width>0 && height>0){

      // Allocate Output Buffer
      if(allocElms(data,width*height)){

        // Try Decoding
        if(WebPDecodeBGRAInto(buffer,size,(FXuchar*)data,width*height*4,width*4)){
          freeElms(buffer);
          return true;
          }
        freeElms(data);
        }
      }
    freeElms(buffer);
    }
  data=nullptr;
  height=0;
  width=0;
  return false;
  }

/*******************************************************************************/

// Save a WebP image
FXbool fxsaveWEBP(FXStream& store,const FXColor* colors,FXint width,FXint height,FXfloat quality){
  FXuchar *data=nullptr;
  FXuint   size=WebPEncodeBGRA((const FXuchar*)colors,width,height,width*4,quality,&data);
  if(size){
    store.save(data,size);
    free(data);
    return true;
    }
  return false;
  }


/*******************************************************************************/

#else

// Check if stream contains a PNG
FXbool fxcheckWEBP(FXStream&){
  return false;
  }


// Stub routine
FXbool fxloadWEBP(FXStream&,FXColor*& data,FXint& width,FXint& height){
  static const FXColor color[2]={FXRGB(0,0,0),FXRGB(255,255,255)};
  static const FXuchar webp_bits[] = {
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0xc6,0x00,0x06,0x00,0x82,0x00,0x04,0x00,0x82,
  0x00,0x04,0x00,0x82,0x00,0x04,0x00,0x92,0x38,0x3c,0x3e,0x92,0x44,0x44,0x44,
  0x92,0x82,0x84,0x84,0xaa,0xfe,0x84,0x84,0xaa,0x02,0x84,0x84,0xaa,0x02,0x84,
  0x84,0x44,0x82,0x84,0x44,0x44,0x84,0x44,0x3c,0x44,0x78,0x3c,0x04,0x00,0x00,
  0x00,0x04,0x00,0x00,0x00,0x1e,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };
  allocElms(data,32*32);
  for(FXint p=0; p<32*32; p++){
    data[p]=color[(webp_bits[p>>3]>>(p&7))&1];
    }
  width=32;
  height=32;
  return true;
  }


// Stub routine
FXbool fxsaveWEBP(FXStream&,const FXColor*,FXint,FXint,FXfloat){
  return false;
  }

#endif

}
