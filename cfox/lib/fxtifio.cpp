/********************************************************************************
*                                                                               *
*                        T I F F   I n p u t / O u t p u t                      *
*                                                                               *
*********************************************************************************
* Copyright (C) 2001,2022 Eric Gillet.   All Rights Reserved.                   *
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
#ifdef HAVE_TIFF_H
#include <tiffio.h>
#endif


/*
  Notes:
  - Made error and warning handlers call FOX's warning handler.
  - References:
    http://www.libtiff.org/libtiff.html
    ftp://ftp.onshore.com/pub/libtiff/TIFF6.ps.Z
    ftp://ftp.sgi.com/graphics/tiff/TTN2.draft.txt
    http://partners.adobe.com/asn/developer/technotes.html
  - Bugs: libtiff does not gracefully recover from certain errors;
    this causes core dump!
  - ARGB means Alpha(0), Red(1), Green(2), Blue(3) in memory.
*/


using namespace FX;

/*******************************************************************************/

namespace FX {


// Declarations
#ifndef FXLOADTIF
extern FXAPI FXbool fxcheckTIF(FXStream& store);
extern FXAPI FXbool fxloadTIF(FXStream& store,FXColor*& data,FXint& width,FXint& height,FXushort& codec);
extern FXAPI FXbool fxsaveTIF(FXStream& store,const FXColor* data,FXint width,FXint height,FXushort codec);
#endif


#ifdef HAVE_TIFF_H

// Stuff being passed around
struct tiff_store_handle {
  FXStream *store;
  FXlong    begin;
  FXlong    end;
  };


// Read bytes from stream
static tsize_t tif_read_store(thandle_t handle,tdata_t data,tsize_t size){
  tiff_store_handle *h=(tiff_store_handle*)handle;
  h->store->load((FXuchar*)data,size);
  if(h->store->eof()!=FXStreamOK) return 0;
  if(h->store->position() > h->end) h->end=h->store->position();
  return size;
  }


// Dummy read bytes
static tsize_t tif_dummy_read_store(thandle_t,tdata_t,tsize_t){
  return 0;
  }


// Write bytes to stream
static tsize_t tif_write_store(thandle_t handle,tdata_t data,tsize_t size){
  tiff_store_handle *h=(tiff_store_handle*)handle;
  h->store->save((FXuchar*)data,size);
  if(h->store->status()!=FXStreamOK) return 0;
  if(h->store->position()>h->end) h->end=h->store->position();
  return size;
  }


// Seek to a position in the stream
static toff_t tif_seek_store(thandle_t handle,toff_t offset,int whence){
  tiff_store_handle *h=(tiff_store_handle*)handle;
  unsigned long off;
  if(whence==SEEK_SET){
    off=h->begin+offset;
    }
  else if(whence==SEEK_CUR){
    off=h->store->position()+offset;
    }
  else{ // SEEK_END
    off=h->end+offset;
    }
  h->store->position(off);
  return off;
  }


// Dummy close store
static int tif_close_store(thandle_t){
  return 0;
  }


// Compute size of what's been written
static toff_t tif_size_store(thandle_t handle){
  tiff_store_handle *h=(tiff_store_handle*)handle;
  return (h->end-h->begin);
  }


// Check if stream contains a TIFF
FXbool fxcheckTIF(FXStream& store){
  FXuchar signature[2];
  store.load(signature,2);
  store.position(-2,FXFromCurrent);
  return (signature[0]==0x4d && signature[1]==0x4d) || (signature[0]==0x49 && signature[1]==0x49);
  }


// Load a TIFF image
FXbool fxloadTIF(FXStream& store,FXColor*& data,FXint& width,FXint& height,FXushort& codec){
  tiff_store_handle s_handle;
  FXuval size,s;

  // Null out
  data=nullptr;
  width=0;
  height=0;

  // Set error/warning handlers
  TIFFSetErrorHandler(nullptr);
  TIFFSetWarningHandler(nullptr);

  // Initialize
  s_handle.store=&store;
  s_handle.begin=store.position();
  s_handle.end=store.position();

  FXTRACE((100,"fxloadTIF\n"));

  // Open image
  TIFF* image=TIFFClientOpen("tiff","rm",(thandle_t)&s_handle,tif_read_store,tif_write_store,tif_seek_store,tif_close_store,tif_size_store,nullptr,nullptr);
  if(image){

    // Get sizes
    TIFFGetField(image,TIFFTAG_IMAGEWIDTH,&width);
    TIFFGetField(image,TIFFTAG_IMAGELENGTH,&height);
    TIFFGetField(image,TIFFTAG_COMPRESSION,&codec);

    FXTRACE((100,"fxloadTIF: width=%d height=%d codec=%d\n",width,height,codec));

    // Make room for data
    size=width*height;
    if(allocElms(data,size)){
      if(TIFFReadRGBAImageOriented(image,width,height,data,ORIENTATION_TOPLEFT,0)){
        for(s=0; s<size; s++){
          data[s]=((data[s]&0xff)<<16)|((data[s]&0xff0000)>>16)|(data[s]&0xff00)|(data[s]&0xff000000);
          }
        TIFFClose(image);
        return true;
        }
      freeElms(data);
      }
    TIFFClose(image);
    }
  return false;
  }

#if 0

/*******************************************************************************/

// Load GEO TIFF
FXbool fxloadTIF__(FXStream& store,FXColor*& data,FXint& width,FXint& height,FXushort& codec){
  tiff_store_handle s_handle;
  FXbool result=false;
  TIFF* image;

  // Null out
  data=nullptr;
  width=0;
  height=0;
  codec=0;

  // Set error/warning handlers
  TIFFSetErrorHandler(nullptr);
  TIFFSetWarningHandler(nullptr);

  // Initialize
  s_handle.store=&store;
  s_handle.begin=store.position();
  s_handle.end=store.position();

  FXTRACE((100,"fxloadGEOTIF\n"));

  // Open image
  if((image=TIFFClientOpen("tiff","rm",(thandle_t)&s_handle,tif_read_store,tif_write_store,tif_seek_store,tif_close_store,tif_size_store,nullptr,nullptr))!=nullptr){
    FXushort samples=0;
    FXushort samplebits=0;
    FXushort format=0;
    FXuint   scanlinesize;
    FXuchar *scanline;

    // Get size
    TIFFGetField(image,TIFFTAG_IMAGEWIDTH,&width);
    TIFFGetField(image,TIFFTAG_IMAGELENGTH,&height);
    TIFFGetField(image,TIFFTAG_SAMPLESPERPIXEL,&samples);
    TIFFGetField(image,TIFFTAG_BITSPERSAMPLE,&samplebits);
    TIFFGetField(image,TIFFTAG_SAMPLEFORMAT,&format);

    // We try to remember the codec for later when we save the image back out...
    TIFFGetField(image,TIFFTAG_COMPRESSION,&codec);

    // Get line size (bytes)
    scanlinesize=TIFFScanlineSize(image);

    // Show image configuration
    FXTRACE((100,"width=%d height=%d codec=%u samples=%u samplebits=%u format=%u scanlinesize=%u\n",width,height,codec,samples,samplebits,format,scanlinesize));

    // Supported formats
    if((format==SAMPLEFORMAT_UINT || format==SAMPLEFORMAT_INT || format==SAMPLEFORMAT_IEEEFP) && (samples==1 || samples==3)){

      // Allocate scanline buffer
      if(callocElms(scanline,scanlinesize)){

        // Make room for data
        if(callocElms(data,width*height)){

/*

    FXuint nPlanarConfig=0;
    FXuint nCompressFlag=0;
    FXuint nPhotometric=0;




    TIFFGetField(image,TIFFTAG_PLANARCONFIG,&nPlanarConfig);
    TIFFGetField(image,TIFFTAG_COMPRESSION,&nCompressFlag);
    TIFFGetField(image,TIFFTAG_PHOTOMETRIC,&nPhotometric);

    FXTRACE((100,"nPlanarConfig=%u\n",nPlanarConfig));
    FXTRACE((100,"nCompressFlag=%u\n",nCompressFlag));
    FXTRACE((100,"nPhotometric=%u\n",nPhotometric));

    switch(nSampleFormat){
      case SAMPLEFORMAT_UINT:
        break;
      case SAMPLEFORMAT_INT:
        break;
      case SAMPLEFORMAT_IEEEFP:
        break;
      case SAMPLEFORMAT_VOID:
        break;
      case SAMPLEFORMAT_COMPLEXINT:
        break;
      case SAMPLEFORMAT_COMPLEXIEEEFP:
        break;
      default:
        break;
      }
*/

          // Read lines
          for(FXint y=0; y<height; ++y){
            TIFFReadScanline(image,scanline,y,0);

            if(samples==3){
              if(samplebits==8){
                for(FXint x=0; x<width; ++x){
                  ((FXuchar*)&data[y*width+x])[0]=scanline[3*x+2];        // Blue
                  ((FXuchar*)&data[y*width+x])[1]=scanline[3*x+1];        // Green
                  ((FXuchar*)&data[y*width+x])[2]=scanline[3*x+0];        // Red
                  ((FXuchar*)&data[y*width+x])[3]=255;                    // Alpha
                  }
                }
              else if(samplebits==16){
                for(FXint x=0; x<width; ++x){
                  ((FXuchar*)&data[y*width+x])[0]=((FXushort*)scanline)[3*x+2]/257;
                  ((FXuchar*)&data[y*width+x])[1]=((FXushort*)scanline)[3*x+1]/257;
                  ((FXuchar*)&data[y*width+x])[2]=((FXushort*)scanline)[3*x+0]/257;
                  ((FXuchar*)&data[y*width+x])[3]=255;
                  }
                }
              }
            else{
              if(samplebits==8){
                for(FXint x=0; x<width; ++x){
                  ((FXuchar*)&data[y*width+x])[0]=scanline[x];          // Blue
                  ((FXuchar*)&data[y*width+x])[1]=scanline[x];          // Green
                  ((FXuchar*)&data[y*width+x])[2]=scanline[x];          // Red
                  ((FXuchar*)&data[y*width+x])[3]=255;                  // Alpha
                  }
                }
              else if(samplebits==16){
                for(FXint x=0; x<width; ++x){
                  ((FXuchar*)&data[y*width+x])[0]=((FXushort*)scanline)[x]/257;
                  ((FXuchar*)&data[y*width+x])[1]=((FXushort*)scanline)[x]/257;
                  ((FXuchar*)&data[y*width+x])[2]=((FXushort*)scanline)[x]/257;
                  ((FXuchar*)&data[y*width+x])[3]=255;
                  }
                }
              }
            }

          // Got as far as this if success
          result=true;
          }
        }
      freeElms(scanline);
      }
    TIFFClose(image);
    }
  return result;
  }
#endif

/*******************************************************************************/

// Save a TIFF image
FXbool fxsaveTIF(FXStream& store,const FXColor* data,FXint width,FXint height,FXushort codec){
  FXbool result=false;

  // Must make sense
  if(data && 0<width && 0<height){

    // Correct for unsupported codecs
    const TIFFCodec* coder=TIFFFindCODEC(codec);
    if(coder==nullptr) codec=COMPRESSION_PACKBITS;

    // Due to the infamous UNISYS patent, we can read LZW TIFF's but not
    // write them back as that would require the LZW compression algorithm!
    if(codec==COMPRESSION_LZW) codec=COMPRESSION_PACKBITS;

    FXTRACE((100,"fxsaveTIF: codec=%d\n",codec));

    // Set error/warning handlers
    TIFFSetErrorHandler(nullptr);
    TIFFSetWarningHandler(nullptr);

    // Initialize
    tiff_store_handle s_handle;
    s_handle.store=&store;
    s_handle.begin=store.position();
    s_handle.end=store.position();

    // Open image
    TIFF* image=TIFFClientOpen("tiff","w",(thandle_t)&s_handle,tif_dummy_read_store,tif_write_store,tif_seek_store,tif_close_store,tif_size_store,nullptr,nullptr);
    if(image){
      FXColor *buffer=nullptr;

      // Size of a strip is 16kb
      FXint rows_per_strip=16*1024/width;
      if(rows_per_strip<1) rows_per_strip=1;

      // Set fields
      TIFFSetField(image,TIFFTAG_IMAGEWIDTH,width);
      TIFFSetField(image,TIFFTAG_IMAGELENGTH,height);
      TIFFSetField(image,TIFFTAG_COMPRESSION,codec);
      TIFFSetField(image,TIFFTAG_ORIENTATION,ORIENTATION_TOPLEFT);
      TIFFSetField(image,TIFFTAG_ROWSPERSTRIP,rows_per_strip);
      TIFFSetField(image,TIFFTAG_BITSPERSAMPLE,8);
      TIFFSetField(image,TIFFTAG_SAMPLESPERPIXEL,4);
      TIFFSetField(image,TIFFTAG_PLANARCONFIG,PLANARCONFIG_CONTIG);
      TIFFSetField(image,TIFFTAG_PHOTOMETRIC,PHOTOMETRIC_RGB);

      // Allocate scanline buffer
      if(allocElms(buffer,width)){

        // Dump each line
        for(FXint y=0; y<height; data+=width,y++){

          // Convert byte order
          for(FXint x=0; x<width; x++){
            buffer[x]=FXREDVAL(data[x]) | FXGREENVAL(data[x])<<8 | FXBLUEVAL(data[x])<<16 | FXALPHAVAL(data[x])<<24;
            }

          // Write scanline
          if(TIFFWriteScanline(image,buffer,y,1)!=1) goto x;
          }

        // All done
        result=true;

        // Delete scanline buffer
x:      freeElms(buffer);
        }

      // Close image
      TIFFClose(image);
      }
    }
  return result;
  }

/*******************************************************************************/

#else

// Check if stream contains a TIFF
FXbool fxcheckTIF(FXStream&){
  return false;
  }


// Stub routine
FXbool fxloadTIF(FXStream&,FXColor*& data,FXint& width,FXint& height,FXushort& codec){
  static const FXColor color[2]={FXRGB(0,0,0),FXRGB(255,255,255)};
  static const FXuchar tiff_bits[]={
   0xff, 0xff, 0xff, 0xff, 0x01, 0x00, 0x00, 0x80, 0xfd, 0xff, 0xff, 0xbf,
   0x05, 0x00, 0x00, 0xa0, 0x05, 0x00, 0x00, 0xa0, 0x05, 0x00, 0x00, 0xa0,
   0x05, 0x00, 0x00, 0xa0, 0x05, 0x00, 0x00, 0xa0, 0x05, 0x00, 0x00, 0xa0,
   0x05, 0x00, 0x00, 0xa0, 0x05, 0x00, 0x00, 0xa0, 0x05, 0x00, 0x00, 0xa0,
   0x05, 0x00, 0x00, 0xa0, 0x05, 0x00, 0x00, 0xa0, 0x05, 0x00, 0x00, 0xa0,
   0x05, 0x00, 0x00, 0xa0, 0x05, 0x00, 0x00, 0xa0, 0xf5, 0x39, 0x9f, 0xaf,
   0x45, 0x10, 0x81, 0xa0, 0x45, 0x10, 0x81, 0xa0, 0x45, 0x10, 0x87, 0xa3,
   0x45, 0x10, 0x81, 0xa0, 0x45, 0x10, 0x81, 0xa0, 0x45, 0x10, 0x81, 0xa0,
   0x45, 0x38, 0x81, 0xa0, 0x05, 0x00, 0x00, 0xa0, 0x05, 0x00, 0x00, 0xa0,
   0x05, 0x00, 0x00, 0xa0, 0x05, 0x00, 0x00, 0xa0, 0xfd, 0xff, 0xff, 0xbf,
   0x01, 0x00, 0x00, 0x80, 0xff, 0xff, 0xff, 0xff};
  allocElms(data,32*32);
  for(FXint p=0; p<32*32; p++){
    data[p]=color[(tiff_bits[p>>3]>>(p&7))&1];
    }
  width=32;
  height=32;
  codec=1;
  return true;
  }


// Stub routine
FXbool fxsaveTIF(FXStream&,const FXColor*,FXint,FXint,FXushort){
  return false;
  }

#endif

}
