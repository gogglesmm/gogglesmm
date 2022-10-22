/********************************************************************************
*                                                                               *
*                         P N G    I n p u t / O u t p u t                      *
*                                                                               *
*********************************************************************************
* Copyright (C) 1998,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#ifdef HAVE_PNG_H
#include "png.h"
#endif

/*
  Notes:
  - References:
    http://www.w3.org/TR/REC-png.html
    http://www.graphicswiz.com/png/
    http://www.inforamp.net/~poynton
    http://www.libpng.org/pub/png/
    https://libspng.org/download.html (new!)
*/

using namespace FX;

/*******************************************************************************/

namespace FX {


#ifndef FXLOADPNG
extern FXAPI FXbool fxcheckPNG(FXStream& store);
extern FXAPI FXbool fxloadPNG(FXStream& store,FXColor*& data,FXint& width,FXint& height);
extern FXAPI FXbool fxsavePNG(FXStream& store,const FXColor* data,FXint width,FXint height);
#endif


#ifdef HAVE_PNG_H


// Custom read function, which will read from the stream in our case
static void user_read_fn(png_structp png_ptr, png_bytep buffer, png_size_t size){
  FXStream *store=(FXStream*)png_get_io_ptr(png_ptr);
  store->load((FXchar*)buffer,size);
  }


// Custom write function, which will write to the stream in our case
static void user_write_fn(png_structp png_ptr, png_bytep buffer, png_size_t size){
  FXStream *store=(FXStream*)png_get_io_ptr(png_ptr);
  store->save((FXchar*)buffer,size);
  }


// Custom output flush function, a no-op in our case
static void user_flush_fn(png_structp ){ }


// Custom error handler; this is unrecoverable
static void user_error_fn(png_structp png_ptr,png_const_charp){
  FXStream* store=(FXStream*)png_get_error_ptr(png_ptr);
  store->setError(FXStreamFormat);                      // Flag this as a format error in FXStream
#if (PNG_LIBPNG_VER < 10500)
  longjmp(png_ptr->jmpbuf,1);                           // Bail out
#else
  png_longjmp(png_ptr,1);                               // Bail out
#endif
  }


// Custom warning handler; we assume this is recoverable
static void user_warning_fn(png_structp,png_const_charp){
  }


// Check if stream contains a PNG
FXbool fxcheckPNG(FXStream& store){
  FXuchar signature[8];
  store.load(signature,8);
  store.position(-8,FXFromCurrent);
  return signature[0]==137 && signature[1]==80 && signature[2]==78 && signature[3]==71 && signature[4]==13 && signature[5]==10 && signature[6]==26 && signature[7]==10;
  }


// Load a PNG image
FXbool fxloadPNG(FXStream& store,FXColor*& data,FXint& width,FXint& height){
  png_structp png_ptr;
  png_infop info_ptr;
  png_uint_32 ww,hh,i;
  int bit_depth,color_type,interlace_type;
  png_bytep *row_pointers;

  // Null out
  data=nullptr;
  width=0;
  height=0;

  // Create png_struct
  png_ptr=png_create_read_struct(PNG_LIBPNG_VER_STRING,&store,user_error_fn,user_warning_fn);
  if(!png_ptr) return false;

  // Allocate/initialize the memory for image information
  info_ptr=png_create_info_struct(png_ptr);
  if(!info_ptr){
    png_destroy_read_struct(&png_ptr,(png_infopp)nullptr,(png_infopp)nullptr);
    return false;
    }

  // Set error handling
#if (PNG_LIBPNG_VER < 10500)
  if(setjmp(png_jmpbuf(png_ptr))){
#else
  if(setjmp((*png_set_longjmp_fn((png_ptr),(png_longjmp_ptr)longjmp,sizeof(jmp_buf))))){
#endif
    png_destroy_read_struct(&png_ptr,&info_ptr,(png_infopp)nullptr);
    return false;
    }

  // Using replacement read functions
  png_set_read_fn(png_ptr,(void *)&store,user_read_fn);

  // If we have already read some of the signature
  //png_set_sig_bytes(png_ptr,sig_read);

  // Get all of the information from the PNG file before the first IDAT (image data chunk).
  png_read_info(png_ptr,info_ptr);

  // Get the goods
  png_get_IHDR(png_ptr,info_ptr,&ww,&hh,&bit_depth,&color_type,&interlace_type,nullptr,nullptr);

  FXTRACE((100,"fxloadPNG: width=%d height=%d bit_depth=%d color_type=%d\n",(int)ww,(int)hh,bit_depth,color_type));

  // tell libpng to strip 16 bit/color files down to 8 bits/color
  png_set_strip_16(png_ptr);

  // rgb(a)->bgr(a)
  png_set_bgr(png_ptr);

  // Expand paletted colors into true RGB triplets
  if(color_type==PNG_COLOR_TYPE_PALETTE) png_set_expand(png_ptr);

  // Expand grayscale images to the full 8 bits from 1, 2, or 4 bits/pixel
  if(color_type==PNG_COLOR_TYPE_GRAY && bit_depth<8) png_set_expand(png_ptr);

  // Expand paletted or RGB images with transparency to full alpha channels
  // so the data will be available as RGBA quartets.
  if(png_get_valid(png_ptr,info_ptr,PNG_INFO_tRNS)) png_set_expand(png_ptr);

  // Grey images are upgraded to RGB
  if(color_type==PNG_COLOR_TYPE_GRAY || color_type==PNG_COLOR_TYPE_GRAY_ALPHA) png_set_gray_to_rgb(png_ptr);

  // If we don't have true alpha, pad with an alpha channel representing opaque
  png_set_filler(png_ptr,0xff,PNG_FILLER_AFTER);

  // Turn on interlace handling
  png_set_interlace_handling(png_ptr);

  // Update image info based on transformations
  png_read_update_info(png_ptr,info_ptr);

  // Make room for data
  if(!allocElms(data,hh*ww)){
    png_destroy_read_struct(&png_ptr,&info_ptr,(png_infopp)nullptr);
    return false;
    }

  // Row pointers
  if(!allocElms(row_pointers,hh)){
    freeElms(data);
    png_destroy_read_struct(&png_ptr,&info_ptr,(png_infopp)nullptr);
    return false;
    }

  // Set up row pointers
  for(i=0; i<hh; i++){
    row_pointers[i]=(png_bytep)&data[i*ww];
    }

  FXTRACE((100,"Reading image...\n"));

  // Finally...
  png_read_image(png_ptr,row_pointers);

  // read rest of file, and get additional chunks in info_ptr
  png_read_end(png_ptr,info_ptr);

  // clean up after the read, and free any memory allocated
  png_destroy_read_struct(&png_ptr,&info_ptr,(png_infopp)nullptr);

  // Get rid of it
  freeElms(row_pointers);

  width=ww;
  height=hh;

  return true;
  }


/*******************************************************************************/

// Save a PNG image
FXbool fxsavePNG(FXStream& store,const FXColor* data,FXint width,FXint height){
  png_structp png_ptr;
  png_infop info_ptr;
  png_bytep *row_pointers;
  int i;

  // Must make sense
  if(!data || width<=0 || height<=0) return false;

  // Create and initialize the png_struct with the desired error handler functions.
  png_ptr=png_create_write_struct(PNG_LIBPNG_VER_STRING,&store,user_error_fn,user_warning_fn);
  if(!png_ptr) return false;

  // Allocate/initialize the image information data.
  info_ptr=png_create_info_struct(png_ptr);
  if(!info_ptr){
    png_destroy_write_struct(&png_ptr,(png_infopp)nullptr);
    return false;
    }

  // Set error handling.
#if (PNG_LIBPNG_VER < 10500)
  if(setjmp(png_jmpbuf(png_ptr))){
#else
  if(setjmp((*png_set_longjmp_fn((png_ptr),(png_longjmp_ptr)longjmp,sizeof(jmp_buf))))){
#endif
    png_destroy_write_struct(&png_ptr,&info_ptr);
    return false;
    }

  // Using replacement read functions
  png_set_write_fn(png_ptr,(void *)&store,user_write_fn,user_flush_fn);

  // Set the header
  png_set_IHDR(png_ptr,info_ptr,width,height,8,PNG_COLOR_TYPE_RGB_ALPHA,PNG_INTERLACE_NONE,PNG_COMPRESSION_TYPE_BASE,PNG_FILTER_TYPE_BASE);

  // Use bgr instead of rgb
  png_set_bgr(png_ptr);

  png_write_info(png_ptr,info_ptr);

  // Row pointers
  if(!allocElms(row_pointers,height)){
    png_destroy_write_struct(&png_ptr,&info_ptr);
    return false;
    }

  // Set up row pointers
  for(i=0; i<height; i++){
    row_pointers[i]=(png_bytep)&data[i*width];
    }

  // Save entire image
  png_write_image(png_ptr,row_pointers);

  // Wrap up
  png_write_end(png_ptr,info_ptr);

  // clean up after the write, and free any memory allocated
  png_destroy_write_struct(&png_ptr,&info_ptr);

  // Get rid of it
  freeElms(row_pointers);

  return true;
  }


/*******************************************************************************/


#else


// Check if stream contains a PNG
FXbool fxcheckPNG(FXStream&){
  return false;
  }


// Stub routine
FXbool fxloadPNG(FXStream&,FXColor*& data,FXint& width,FXint& height){
  static const FXColor color[2]={FXRGB(0,0,0),FXRGB(255,255,255)};
  static const FXuchar png_bits[] = {
   0xff, 0xff, 0xff, 0xff, 0x01, 0x00, 0x00, 0x80, 0xfd, 0xff, 0xff, 0xbf,
   0x05, 0x00, 0x00, 0xa0, 0x05, 0x00, 0x00, 0xa0, 0x05, 0x00, 0x00, 0xa0,
   0x05, 0x00, 0x00, 0xa0, 0x05, 0x00, 0x00, 0xa0, 0x05, 0x00, 0x00, 0xa0,
   0x05, 0x00, 0x00, 0xa0, 0x05, 0x00, 0x00, 0xa0, 0x05, 0x00, 0x00, 0xa0,
   0x05, 0x00, 0x00, 0xa0, 0x05, 0x00, 0x00, 0xa0, 0x05, 0x00, 0x00, 0xa0,
   0x05, 0x00, 0x00, 0xa0, 0x05, 0x00, 0x00, 0xa0, 0xc5, 0x23, 0xc4, 0xa1,
   0x45, 0x24, 0x24, 0xa2, 0x45, 0x64, 0x24, 0xa0, 0x45, 0xa4, 0x24, 0xa0,
   0x45, 0x24, 0x25, 0xa0, 0xc5, 0x23, 0x26, 0xa3, 0x45, 0x20, 0x24, 0xa2,
   0x45, 0x20, 0x24, 0xa2, 0x45, 0x20, 0xc4, 0xa1, 0x05, 0x00, 0x00, 0xa0,
   0x05, 0x00, 0x00, 0xa0, 0x05, 0x00, 0x00, 0xa0, 0xfd, 0xff, 0xff, 0xbf,
   0x01, 0x00, 0x00, 0x80, 0xff, 0xff, 0xff, 0xff};
  allocElms(data,32*32);
  for(FXint p=0; p<32*32; p++){
    data[p]=color[(png_bits[p>>3]>>(p&7))&1];
    }
  width=32;
  height=32;
  return true;
  }


// Stub routine
FXbool fxsavePNG(FXStream&,const FXColor*,FXint,FXint){
  return false;
  }


#endif

}
