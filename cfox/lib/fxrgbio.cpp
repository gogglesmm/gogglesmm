/********************************************************************************
*                                                                               *
*                    I R I S   R G B   I n p u t / O u t p u t                  *
*                                                                               *
*********************************************************************************
* Copyright (C) 2002,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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



/*
  Notes:
  - Need to implement RLE compression some time.
  - ARGB means Alpha(0), Red(1), Green(2), Blue(3) in memory.
  - Uses a wee bit of memory during the load; but the advantage is that it
    doesn't seek around on the file but loads all data sequentially.
  - We now handle Luminance, Luminance/Alpha, RGB, and RGBA flavors of IRIS RGB.
*/

using namespace FX;

/*******************************************************************************/

namespace FX {


#ifndef FXLOADRGB
extern FXAPI FXbool fxcheckRGB(FXStream& store);
extern FXAPI FXbool fxloadRGB(FXStream& store,FXColor*& data,FXint& width,FXint& height);
extern FXAPI FXbool fxsaveRGB(FXStream& store,const FXColor *data,FXint width,FXint height);
#endif


// RLE decompress with some checks against overruns
static void expand(FXuchar dst[],FXint dlen,const FXuchar src[],FXint slen){
  FXuchar pixel,count;
  FXint d=0,s=0;
  while(s<slen){
    pixel=src[s++];
    count=pixel&0x7F;
    if(count==0) break;         // Normal termination with RLE 0-code
    if(d+count>dlen) break;     // Not enough space in destination
    if(pixel&0x80){             // Literal bytes
      if(s+count>slen) break;   // Not enough bytes in source
      while(count--){
        dst[d++]=src[s++];
        }
      }
    else{                       // Repeated bytes
      if(s+1>slen) break;       // Not enough bytes in source
      pixel=src[s++];
      while(count--){
        dst[d++]=pixel;
        }
      }
    }
  }


#if 0
static FXint compress(lbuf, rlebuf, z, cnt)
     byte *lbuf, *rlebuf;
     int   z, cnt;



// RLE compress with some checks against overruns
static FXint compress(FXuchar dst[],FXint dlen,const FXuchar src[],FXint slen){
  byte *iptr, *ibufend, *sptr, *optr;
  short todo, cc;
  long  count;

  lbuf    += z;
  iptr    = lbuf;
  ibufend = iptr+cnt*4;
  optr    = rlebuf;

  while (iptr<ibufend) {
    sptr = iptr;
    iptr += 8;
    while((iptr<ibufend) && ((iptr[-8]!=iptr[-4]) || (iptr[-4]!=iptr[0])))
      iptr += 4;
    iptr -= 8;

    count = (iptr-sptr)/4;

    while (count) {
      todo = count>126 ? 126 : count;
      count -= todo;
      *optr++ = 0x80|todo;

      while (todo--) {
	*optr++ = *sptr;
	sptr += 4;
        }
      }

    sptr = iptr;
    cc = *iptr;
    iptr += 4;
    while ((iptr<ibufend) && (*iptr == cc))  iptr += 4;

    count = (iptr-sptr)/4;
    while (count) {
      todo = count>126 ? 126:count;
      count -= todo;
      *optr++ = todo;
      *optr++ = cc;
      }
    }

  *optr++ = 0;
  return (optr - rlebuf);
  }
#endif


// Convert planar grey scale to RGBA
static void bwtorgba(FXColor *l,const FXuchar *b,FXint n){
  while(n--){
    l[0]=FXRGB(b[0],b[0],b[0]);
    l++;
    b++;
    }
  }


// Convert planar luminance-alpha to interleaved RGBA
static void latorgba(FXColor *l,const FXuchar *b,const FXuchar *a,FXint n){
  while(n--){
    l[0]=FXRGBA(b[0],b[0],b[0],a[0]);
    l++;
    b++;
    a++;
    }
  }


// Convert planar rgb to interleaved RGBA
static void rgbtorgba(FXColor *l,const FXuchar *r,const FXuchar *g,const FXuchar *b,FXint n){
  while(n--){
    l[0]=FXRGB(r[0],g[0],b[0]);
    l++;
    r++;
    g++;
    b++;
    }
  }


// Convert planar rgba to interleaved RGBA
static void rgbatorgba(FXColor *l,const FXuchar *r,const FXuchar *g,const FXuchar *b,const FXuchar *a,FXint n){
  while(n--){
    l[0]=FXRGBA(r[0],g[0],b[0],a[0]);
    l++;
    r++;
    g++;
    b++;
    a++;
    }
  }


// Check if stream contains a RGB
FXbool fxcheckRGB(FXStream& store){
  FXuchar signature[2];
  store.load(signature,2);
  store.position(-2,FXFromCurrent);
  return signature[0]==0x01 && signature[1]==0xDA;
  }


// Load image from stream
FXbool fxloadRGB(FXStream& store,FXColor*& data,FXint& width,FXint& height){
  FXlong   base=store.position();
  FXbool   swap=store.swapBytes();
  FXbool   result=false;
  FXushort magic;
  FXuchar  storage;
  FXuchar  bpc;
  FXushort dimension;
  FXushort w;
  FXushort h;
  FXushort channels;
  FXuint   maxpix;
  FXuint   minpix;
  FXuint   dummy;
  FXchar   name[80];
  FXuint   colormap;

  // Null out
  data=nullptr;
  width=0;
  height=0;

  // Remember swap state
  store.setBigEndian(true);

  // Load header
  store >> magic;       // MAGIC (2)
  store >> storage;     // STORAGE (1)
  store >> bpc;         // BPC (1)
  store >> dimension;   // DIMENSION (2)
  store >> w;           // XSIZE (2)
  store >> h;           // YSIZE (2)
  store >> channels;    // ZSIZE (2)
  store >> minpix;      // PIXMIN (4)
  store >> maxpix;      // PIXMAX (4)
  store >> dummy;       // DUMMY (4)
  store.load(name,80);  // IMAGENAME (80)
  store >> colormap;    // Colormap ID (4)

  FXTRACE((100,"fxloadRGB: magic=%d name=%s width=%d height=%d nchannels=%d dimension=%d storage=%d bpc=%d\n",magic,name,w,h,channels,dimension,storage,bpc));

  // Check magic number and other parameters
  if(magic==474 && 1<=channels && channels<=4 && bpc==1 && 0<w && 0<h){
    FXint tablen=h*channels;    // Number of chunk start/chunk length table entries
    FXint size=w*h;             // Total number of pixels
    FXint total=channels*size;  // Total number of samples
    FXuchar *planar;

    // Skip to data
    store.position(404,FXFromCurrent);

    // Allocate planar array
    if(allocElms(planar,total)){

      // Allocate image data
      if(allocElms(data,size)){
        FXint i,j,k;

        // Set width and height
        width=w;
        height=h;

        // Compressed
        if(storage){
          FXuint *starttab;
          FXuint *lengthtab;

          // Allocate line tables
          if(allocElms(starttab,tablen<<1)){
            lengthtab=&starttab[tablen];

            // Read line tables
            store.load(starttab,tablen);
            store.load(lengthtab,tablen);

            // Offset of RLE chunks in the file
            FXuint sub=store.position()-base;
            FXuint chunklen=0;
            FXuchar *chunk;

            // Fix up the line table & figure space for RLE chunks
            // Intelligent RGB writers (not ours ;-)) may re-use RLE
            // chunks for more than 1 line...
            for(i=0; i<tablen; i++){
              starttab[i]-=sub;
              chunklen=FXMAX(chunklen,(starttab[i]+lengthtab[i]));
              }

            // Make room for the compressed lines
            if(allocElms(chunk,chunklen)){

              // Load all RLE chunks in one fell swoop
              store.load(chunk,chunklen);

              // Decompress chunks into planar
              for(k=0; k<tablen; ++k){
                expand(&planar[k*width],width,&chunk[starttab[k]],lengthtab[k]);
                }

              // Free RLE chunks
              freeElms(chunk);
              }

            // Free line tables
            freeElms(starttab);
            }
          }

        // Uncompressed
        else{
          store.load(planar,total);
          }

        // Combine the channels properly
        switch(channels){
          case 1:
            for(i=0,j=(height-1)*width; 0<=j; i+=width,j-=width){
              bwtorgba(&data[i],&planar[j],width);
              }
            break;
          case 2:
            for(i=0,j=(height-1)*width; 0<=j; i+=width,j-=width){
              latorgba(&data[i],&planar[j],&planar[j+size],width);
              }
            break;
          case 3:
            for(i=0,j=(height-1)*width; 0<=j; i+=width,j-=width){
              rgbtorgba(&data[i],&planar[j],&planar[j+size],&planar[j+size+size],width);
              }
            break;
          case 4:
            for(i=0,j=(height-1)*width; 0<=j; i+=width,j-=width){
              rgbatorgba(&data[i],&planar[j],&planar[j+size],&planar[j+size+size],&planar[j+size+size+size],width);
              }
            break;
          }

        // We're good
        result=true;
        }
      freeElms(planar);
      }
    }

  // Reset swap status
  store.swapBytes(swap);
  return result;
  }


/*******************************************************************************/


// Save a bmp file to a stream
FXbool fxsaveRGB(FXStream& store,const FXColor *data,FXint width,FXint height){
  FXushort magic=474;
  FXuchar  storage=0;
  FXuchar  bpc=1;
  FXushort dimension=3;
  FXushort w=width;
  FXushort h=height;
  FXushort channels=3;
  FXuint   maxpix=255;
  FXuint   minpix=0;
  FXuint   dummy=0;
  FXuint   colormap=0;
  FXint    size=width*height;
  FXuchar  temp[512];
  FXuchar *array;
  FXint    i,j,k;
  FXbool   swap;

  // Must make sense
  if(data && 0<width && 0<height){

    // Reorganize in planes
    if(allocElms(array,size*channels)){

      // Remember swap state
      swap=store.swapBytes();
      store.setBigEndian(true);

      // Save header
      store << magic;             // MAGIC (2)
      store << storage;           // STORAGE (1)
      store << bpc;               // BPC (1)
      store << dimension;         // DIMENSION (2)
      store << w;                 // XSIZE (2)
      store << h;                 // YSIZE (2)
      store << channels;          // ZSIZE (2)
      store << minpix;            // PIXMIN (4)
      store << maxpix;            // PIXMAX (4)
      store << dummy;             // DUMMY (4)
      memset(temp,0,80);          // Clean it
      memcpy(temp,"IRIS RGB",8);  // Write name
      store.save(temp,80);        // IMAGENAME (80)
      store << colormap;             // COLORMAP (4)
      memset(temp,0,404);         // Clean it
      store.save(temp,404);       // DUMMY (404)

      // Copy
      for(j=height-1,k=0; j>=0; --j){
        for(i=0; i<width; ++i,++k){
          array[j*width+i]=((const FXuchar*)&data[k])[2];
          array[j*width+i+size]=((const FXuchar*)&data[k])[1];
          array[j*width+i+size+size]=((const FXuchar*)&data[k])[0];
          }
        }

      // Save it
      store.save(array,size*channels);

      // Clean up temp memory
      freeElms(array);

      // Reset swap status
      store.swapBytes(swap);
      return true;
      }
    }
  return false;
  }

}
