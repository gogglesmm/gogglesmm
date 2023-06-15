/********************************************************************************
*                                                                               *
*                      J P E G    I n p u t / O u t p u t                       *
*                                                                               *
*********************************************************************************
* Copyright (C) 2000,2022 by David Tyree.   All Rights Reserved.                *
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
#ifdef HAVE_JPEG_H
#include <setjmp.h>
#undef FAR
extern "C" {
/* Theo Veenker <Theo.Veenker@let.uu.nl> says this is needed for CYGWIN */
#if (defined(__CYGWIN__) || defined(__MINGW32__) || defined(_MSC_VER)) && !defined(XMD_H)
#define XMD_H
typedef short INT16;
typedef int INT32;
#include "jpeglib.h"
#undef XMD_H
#elif defined __WINE__
#define XMD_H
#include "jpeglib.h"
#else
#include "jpeglib.h"
#endif
}
#endif



/*
  Notes:
  - Add more options for fast jpeg loading.
  - Write a more detailed class that offers more options.
  - Add the ability to load jpegs in the background.
  - We should NOT assume that we can reposition the current stream position;
    for example, with bzip2 or gzip streams this is not possible.
  - References:

      http://www.ijg.org/
      ftp://ftp.uu.net/graphics/jpeg/
      http://the-labs.com

  - Compression rationale for optimize_coding=true flag:

     https://github.com/bither/bither-android-lib/blob/master/REASON.md
*/

#define JPEG_BUFFER_SIZE 4096


using namespace FX;

/*******************************************************************************/

namespace FX {


#ifndef FXLOADJPG
extern FXAPI FXbool fxcheckJPG(FXStream& store);
extern FXAPI FXbool fxloadJPG(FXStream& store,FXColor*& data,FXint& width,FXint& height,FXint& quality);
extern FXAPI FXbool fxsaveJPG(FXStream& store,const FXColor* data,FXint width,FXint height,FXint quality);
#endif


#ifdef HAVE_JPEG_H

// Source Manager for libjpeg
struct FOX_jpeg_source_mgr {
  struct jpeg_source_mgr pub;
  JOCTET    buffer[JPEG_BUFFER_SIZE];
  FXStream *stream;
  };


// Destination Manager for libjpeg
struct FOX_jpeg_dest_mgr {
  struct jpeg_destination_mgr pub;
  JOCTET    buffer[JPEG_BUFFER_SIZE];
  FXStream *stream;
  };


// For error handler
struct FOX_jpeg_error_mgr {
  struct jpeg_error_mgr error_mgr;
  jmp_buf jmpbuf;
  };


/*******************************************************************************/


// Fatal error use FOX's way of reporing errors
static void fatal_error(j_common_ptr cinfo){
  longjmp(((FOX_jpeg_error_mgr*)cinfo->err)->jmpbuf,1);
  }


// A no-op in our case
static void init_source(j_decompress_ptr){
  }


// Read JPEG_BUFFER_SIZE bytes into the buffer
// NOTE:- we need to read in one byte at a time, so as to make sure that
// data belonging to the objects following this JPEG remain in the stream!
static boolean fill_input_buffer(j_decompress_ptr cinfo){
  FOX_jpeg_source_mgr *src=(FOX_jpeg_source_mgr*)cinfo->src;
/*
src->stream->load(src->buffer,JPEG_BUFFER_SIZE);
src->pub.next_input_byte=src->buffer;
src->pub.bytes_in_buffer=JPEG_BUFFER_SIZE;
*/
  *src->stream >> src->buffer[0];
  if(src->stream->eof()){    // Insert a fake EOI marker
    src->buffer[0]=0xff;
    src->buffer[1]=JPEG_EOI;
    src->pub.next_input_byte=src->buffer;
    src->pub.bytes_in_buffer=2;
    }
  else{
    src->pub.next_input_byte=src->buffer;
    src->pub.bytes_in_buffer=1;
    }
  return TRUE;
  }


// Skip ahead some number of bytes
static void skip_input_data(j_decompress_ptr cinfo,long num_bytes){
  FOX_jpeg_source_mgr *src=(FOX_jpeg_source_mgr*)cinfo->src;
  if(num_bytes>0){
    while(num_bytes>(long)src->pub.bytes_in_buffer){
      num_bytes-=(long)src->pub.bytes_in_buffer;
      fill_input_buffer(cinfo);
      }
    src->pub.next_input_byte+=(size_t) num_bytes;
    src->pub.bytes_in_buffer-=(size_t) num_bytes;
    }
  }


// A no-op in our case
static void term_source(j_decompress_ptr){
  }


// Check if stream contains a JPG
FXbool fxcheckJPG(FXStream& store){
  FXuchar signature[3];
  store.load(signature,3);
  store.position(-3,FXFromCurrent);
  return signature[0]==0xFF && signature[1]==0xD8 && signature[2]==0xFF;
  }


// Load a JPEG image
FXbool fxloadJPG(FXStream& store,FXColor*& data,FXint& width,FXint& height,FXint&){
  jpeg_decompress_struct srcinfo;
  FOX_jpeg_error_mgr jerr;
  FOX_jpeg_source_mgr src;
  JSAMPLE *buffer[1];
  FXColor *pp;
  JSAMPLE *qq;
  int row_stride,color,i;

  // Null out
  data=nullptr;
  width=0;
  height=0;

  // No sample buffer
  buffer[0]=nullptr;

  // initialize the jpeg data structure;
  memset(&srcinfo,0,sizeof(srcinfo));
  jpeg_create_decompress(&srcinfo);

  // setup the error handler
  srcinfo.err=jpeg_std_error(&jerr.error_mgr);
  jerr.error_mgr.error_exit=fatal_error;

  // Set error handling
  if(setjmp(jerr.jmpbuf)){
    jpeg_destroy_decompress(&srcinfo);
    return false;
    }

  // setup our src manager
  src.pub.init_source=init_source;
  src.pub.fill_input_buffer=fill_input_buffer;
  src.pub.resync_to_restart=jpeg_resync_to_restart;   // Use the default method
  src.pub.skip_input_data=skip_input_data;
  src.pub.term_source=term_source;
  src.pub.bytes_in_buffer=0;
  src.pub.next_input_byte=nullptr;
  src.stream=&store;

  // Set our src manager
  srcinfo.src=&src.pub;

  // read the header from the jpg;
  jpeg_read_header(&srcinfo,TRUE);

  // Output format supported by libjpeg
  switch (srcinfo.jpeg_color_space) {
    case JCS_GRAYSCALE: // 1
    case JCS_RGB:       // 2
    case JCS_YCbCr:     // 3
      srcinfo.out_color_space=JCS_RGB;
      break;
    case JCS_CMYK:      // 4
    case JCS_YCCK:      // 5
      srcinfo.out_color_space=JCS_CMYK;
      break;
    default:
      return false;
    }

  jpeg_start_decompress(&srcinfo);

  row_stride=srcinfo.output_width*srcinfo.output_components;

  // Data to receive
  if(!allocElms(data,srcinfo.image_height*srcinfo.image_width)){
    jpeg_destroy_decompress(&srcinfo);
    return false;
    }

  height=srcinfo.image_height;
  width=srcinfo.image_width;

  // Sample buffer
  if(!allocElms(buffer[0],row_stride)){
    freeElms(data);
    jpeg_destroy_decompress(&srcinfo);
    return false;
    }

  // Read the jpeg data
  pp=data;
  color=srcinfo.out_color_space;
  while(srcinfo.output_scanline<srcinfo.output_height){
    jpeg_read_scanlines(&srcinfo,buffer,1);
    qq=buffer[0];
    if(color==JCS_RGB){
      for(i=0; i<width; i++,pp++){
        ((FXuchar*)pp)[3]=255;
        ((FXuchar*)pp)[2]=*qq++;
        ((FXuchar*)pp)[1]=*qq++;
        ((FXuchar*)pp)[0]=*qq++;
        }
      }
    else{
      for(i=0; i<width; i++,pp++){
        ((FXuchar*)pp)[3]=255;
        if(qq[3]==255){
          ((FXuchar*)pp)[2]=qq[0];                      // No black
          ((FXuchar*)pp)[1]=qq[1];
          ((FXuchar*)pp)[0]=qq[2];
          }
        else{
          ((FXuchar*)pp)[2]=(qq[0]*qq[3])/255;          // Approximated CMYK -> RGB
          ((FXuchar*)pp)[1]=(qq[1]*qq[3])/255;
          ((FXuchar*)pp)[0]=(qq[2]*qq[3])/255;
          }
        qq+=4;
        }
      }
    }

  // Clean up
  jpeg_finish_decompress(&srcinfo);
  jpeg_destroy_decompress(&srcinfo);
  freeElms(buffer[0]);
  return true;
  }


/*******************************************************************************/


// Initialize the buffer
static void init_destination(j_compress_ptr cinfo){
  FOX_jpeg_dest_mgr *dest=(FOX_jpeg_dest_mgr*)cinfo->dest;
  dest->pub.next_output_byte=dest->buffer;
  dest->pub.free_in_buffer=JPEG_BUFFER_SIZE;
  }


// Write the buffer to the stream
static boolean empty_output_buffer(j_compress_ptr cinfo){
  FOX_jpeg_dest_mgr *dest=(FOX_jpeg_dest_mgr*)cinfo->dest;
  dest->stream->save(dest->buffer,JPEG_BUFFER_SIZE);
  dest->pub.free_in_buffer=JPEG_BUFFER_SIZE;
  dest->pub.next_output_byte=dest->buffer;
  return TRUE;
  }


// Write any remaining data in the buffer to the stream
static void term_destination(j_compress_ptr cinfo){
  FOX_jpeg_dest_mgr *dest=(FOX_jpeg_dest_mgr*)cinfo->dest;
  dest->stream->save(dest->buffer,JPEG_BUFFER_SIZE-dest->pub.free_in_buffer);
  }


// Save a JPEG image
FXbool fxsaveJPG(FXStream& store,const FXColor* data,FXint width,FXint height,FXint quality){
  jpeg_compress_struct dstinfo;
  FOX_jpeg_error_mgr jerr;
  FOX_jpeg_dest_mgr dst;
  JSAMPLE *buffer[1];
  const FXColor *pp;
  JSAMPLE *qq;

  // Must make sense
  if(!data || width<=0 || height<=0 || quality<=0 || 100<quality) return false;

  // Row buffer
  if(!allocElms(buffer[0],width*3)) return false;

  // Specify the error manager
  memset(&dstinfo,0,sizeof(dstinfo));
  dstinfo.err=jpeg_std_error(&jerr.error_mgr);
  jerr.error_mgr.error_exit=fatal_error;

  // Set error handling
  if(setjmp(jerr.jmpbuf)){
    freeElms(buffer[0]);
    jpeg_destroy_compress(&dstinfo);
    return false;
    }

  // initialize the structure
  jpeg_create_compress(&dstinfo);

  // Specify the use of our destination manager
  dst.pub.init_destination=init_destination;
  dst.pub.empty_output_buffer=empty_output_buffer;
  dst.pub.term_destination=term_destination;
  dst.pub.free_in_buffer=0;
  dst.pub.next_output_byte=nullptr;
  dst.stream=&store;

  // Set up the input parameters for the file
  dstinfo.image_width=width;
  dstinfo.image_height=height;
  dstinfo.input_components=3;
  dstinfo.in_color_space=JCS_RGB;
  dstinfo.dest=&dst.pub;

  // Based on Sander's recommendation
  dstinfo.optimize_coding=true;

  jpeg_set_defaults(&dstinfo);
  jpeg_set_quality(&dstinfo,quality,TRUE);
  jpeg_start_compress(&dstinfo,TRUE);

  // Write the jpeg data
  pp=data;
  while(dstinfo.next_scanline<dstinfo.image_height){
    qq=buffer[0];
    for(FXint i=0; i<width; i++,pp++){
      *qq++=((const FXuchar*)pp)[2];
      *qq++=((const FXuchar*)pp)[1];
      *qq++=((const FXuchar*)pp)[0];
      }
    jpeg_write_scanlines(&dstinfo,buffer,1);
    }

  // Clean up
  jpeg_finish_compress(&dstinfo);
  jpeg_destroy_compress(&dstinfo);
  freeElms(buffer[0]);
  return true;
  }


/*******************************************************************************/


#else


// Check if stream contains a JPG
FXbool fxcheckJPG(FXStream&){
  return false;
  }


// Stub routine
FXbool fxloadJPG(FXStream&,FXColor*& data,FXint& width,FXint& height,FXint& quality){
  static const FXColor color[2]={FXRGB(0,0,0),FXRGB(255,255,255)};
  static const FXuchar jpeg_bits[]={
   0xff, 0xff, 0xff, 0xff, 0x01, 0x00, 0x00, 0x80, 0xfd, 0xff, 0xff, 0xbf,
   0x05, 0x00, 0x00, 0xa0, 0x05, 0x00, 0x00, 0xa0, 0x05, 0x00, 0x00, 0xa0,
   0x05, 0x00, 0x00, 0xa0, 0x05, 0x00, 0x00, 0xa0, 0x05, 0x00, 0x00, 0xa0,
   0x05, 0x00, 0x00, 0xa0, 0x05, 0x00, 0x00, 0xa0, 0x05, 0x00, 0x00, 0xa0,
   0x05, 0x00, 0x00, 0xa0, 0x05, 0x00, 0x00, 0xa0, 0x05, 0x00, 0x00, 0xa0,
   0x05, 0x00, 0x00, 0xa0, 0x05, 0x00, 0x00, 0xa0, 0xf5, 0x3d, 0x9f, 0xa3,
   0x05, 0x45, 0x41, 0xa4, 0x05, 0x45, 0x41, 0xa0, 0x05, 0x45, 0x47, 0xa0,
   0x05, 0x3d, 0x41, 0xa6, 0x05, 0x05, 0x41, 0xa4, 0x15, 0x05, 0x41, 0xa4,
   0xe5, 0x04, 0x9f, 0xa3, 0x05, 0x00, 0x00, 0xa0, 0x05, 0x00, 0x00, 0xa0,
   0x05, 0x00, 0x00, 0xa0, 0x05, 0x00, 0x00, 0xa0, 0xfd, 0xff, 0xff, 0xbf,
   0x01, 0x00, 0x00, 0x80, 0xff, 0xff, 0xff, 0xff};
  allocElms(data,32*32);
  for(FXint p=0; p<32*32; p++){
    data[p]=color[(jpeg_bits[p>>3]>>(p&7))&1];
    }
  width=32;
  height=32;
  quality=75;
  return true;
  }


// Stub routine
FXbool fxsaveJPG(FXStream&,const FXColor*,FXint,FXint,FXint){
  return false;
  }


#endif

}
