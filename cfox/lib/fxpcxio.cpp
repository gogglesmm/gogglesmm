/********************************************************************************
*                                                                               *
*                          P C X   I n p u t / O u t p u t                      *
*                                                                               *
*********************************************************************************
* Copyright (C) 2001,2022 by Janusz Ganczarski.   All Rights Reserved.          *
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
  - Loading 1-bit/1-plane, 4-bit/1-plane, 8-bit/1-plane and 8-bit/3-plane
    images appears to work.
  - Need to check if fewer colors, if so fall back on lower pixel depth
    mode to save space.
*/

using namespace FX;

/*******************************************************************************/

namespace FX {


#ifndef FXLOADPCX
extern FXAPI FXbool fxcheckPCX(FXStream& store);
extern FXAPI FXbool fxloadPCX(FXStream& store,FXColor*& data,FXint& width,FXint& height);
extern FXAPI FXbool fxsavePCX(FXStream& store,const FXColor *data,FXint width,FXint height);
#endif


// Check if stream contains a PCX
FXbool fxcheckPCX(FXStream& store){
  FXuchar signature[4];
  store.load(signature,4);
  store.position(-4,FXFromCurrent);
  return signature[0]==10 && (signature[3]==1 || signature[3]==2 || signature[3]==4 || signature[3]==8);
  }


// Read possibly compressed scanline
static void readscanline(FXStream& store,FXuchar line[],int size,int compressed){
  FXint count,i=0;
  FXuchar cc;
  if(compressed==1){
    while(i<size){
      count=1;
      store >> cc;
      if(cc>0xc0){
        count=cc-0xc0;
        store >> cc;
        }
      while(count-- && i<size){
        line[i++]=cc;
        }
      }
    }
  else{
    while(i<size){
      store >> cc;
      line[i++]=cc;
      }
    }
  }


// Load PCX image from stream
FXbool fxloadPCX(FXStream& store,FXColor*& data,FXint& width,FXint& height){
  const FXuchar Mono[2]={0,255};
  FXuchar  Colormap[256][3];
  FXuchar  Manufacturer;
  FXuchar  Version;
  FXuchar  Encoding;
  FXuchar  BitsPerPixel;
  FXuchar  NPlanes;
  FXuchar  Reserved;
  FXshort  PaletteInfo;
  FXshort  HRes;
  FXshort  VRes;
  FXshort  Xmin;
  FXshort  Ymin;
  FXshort  Xmax;
  FXshort  Ymax;
  FXshort  BytesPerLine;
  FXuchar  fill,*line,*pp,c;
  FXint    NumPixels,TotalBytes,index,shift,clr,x,y,i;
  FXbool   swap;
  FXbool   ok=false;

  // Null out
  data=nullptr;
  width=0;
  height=0;

  // Check Manufacturer
  store >> Manufacturer;
  if(Manufacturer==10){

    // Get Version
    store >> Version;

    // Get Encoding
    store >> Encoding;

    // Get BitsPerPixel
    store >> BitsPerPixel;

    // One of these four possibilities?
    if(BitsPerPixel==1 || BitsPerPixel==2 || BitsPerPixel==4 || BitsPerPixel==8){

      // Switch to little-endian
      swap=store.swapBytes();
      store.setBigEndian(false);

      // Get Xmin, Ymin, Xmax, Ymax
      store >> Xmin;
      store >> Ymin;
      store >> Xmax;
      store >> Ymax;

      // HDpi, VDpi
      store >> HRes;
      store >> VRes;

      // Get EGA/VGA Colormap
      store.load(Colormap[0],48);

      // Get Reserved
      store >> Reserved;

      // Get NPlanes
      store >> NPlanes;

      // Does it make sense?
      if(NPlanes==1 || NPlanes==3 || NPlanes==4){

        // Get BytesPerLine
        store >> BytesPerLine;

        // Total bytes for scanline
        TotalBytes=BytesPerLine*NPlanes;

        // Read PaletteInfo
        store >> PaletteInfo;

        // Get 58 bytes, to get to 128 byte header
        for(i=0; i<58; i++) store >> fill;

        // Calculate width and height
        width=Xmax-Xmin+1;
        height=Ymax-Ymin+1;

        // Total number of pixels
        NumPixels=width*height;

        FXTRACE((100,"fxloadPCX: width=%d height=%d Version=%d BitsPerPixel=%d NPlanes=%d BytesPerLine=%d Encoding=%d\n",width,height,Version,BitsPerPixel,NPlanes,BytesPerLine,Encoding));

        // Scanline buffer
        if(allocElms(line,TotalBytes)){

          // Allocate memory
          if(callocElms(data,NumPixels)){

            // Load 1 bit/pixel
            if(BitsPerPixel==1 && NPlanes==1){
              pp=(FXuchar*)data;
              for(y=0; y<height; y++){
                readscanline(store,line,BytesPerLine,Encoding);
                for(x=0; x<width; x++){
                  clr=((FXuchar)(line[x>>3]<<(x&7))>>7);
                  *pp++=Mono[clr];
                  *pp++=Mono[clr];
                  *pp++=Mono[clr];
                  *pp++=255;
                  }
                }
              }

            // Load 8 bit/pixel
            else if(BitsPerPixel==8 && NPlanes==1){
              pp=(FXuchar*)data;
              for(y=0; y<height; y++){
                readscanline(store,line,BytesPerLine,Encoding);
                for(x=0; x<width; x++,pp+=4){
                  *pp=line[x];
                  }
                }
              store >> c;               // Get VGApaletteID
              if(c==12){;               // Check VGApaletteID
                store.load(Colormap[0],768);
                pp=(FXuchar*)data;
                for(i=0; i<NumPixels; i++){   // Apply colormap
                  clr=pp[0];
                  *pp++=Colormap[clr][2];
                  *pp++=Colormap[clr][1];
                  *pp++=Colormap[clr][0];
                  *pp++=255;
                  }
                }
              }

            // Load 24 bits/pixel
            else if(BitsPerPixel==8 && NPlanes==3){
              pp=(FXuchar*)data;
              for(y=0; y<height; y++){
                readscanline(store,line,TotalBytes,Encoding);
                for(x=0; x<width; x++){
                  *pp++=line[(BytesPerLine<<1)+x];
                  *pp++=line[BytesPerLine+x];
                  *pp++=line[x];
                  *pp++=0xFF;
                  }
                }
              }

            // Load 4 bit/pixel
            else if((BitsPerPixel==4) || (BitsPerPixel==1 && NPlanes==4)){
              pp=(FXuchar*)data;
              for(y=0; y<height; y++){
                readscanline(store,line,BytesPerLine*4,Encoding);
                for(x=0; x<width; x++){
                  clr=0;
                  index=x>>3;
                  shift=7-(x&7);
                  clr|=0x01&((line[index]>>shift));
                  clr|=0x02&((line[index+BytesPerLine]>>shift)<<1);
                  clr|=0x04&((line[index+BytesPerLine*2]>>shift)<<2);
                  clr|=0x08&((line[index+BytesPerLine*3]>>shift)<<3);
                  *pp++=Colormap[clr][2];
                  *pp++=Colormap[clr][1];
                  *pp++=Colormap[clr][0];
                  *pp++=255;
                  }
                }
              }
            ok=true;
            }

          // Done with that
          freeElms(line);
          }
        }

      // Reset byte order
      store.swapBytes(swap);
      }
    }
  return ok;
  }



/*******************************************************************************/


// Save a PCX file to a stream
FXbool fxsavePCX(FXStream& store,const FXColor *data,FXint width,FXint height){
  const FXuchar Colormap[16][3]={{0,0,0},{255,255,255},{0,170,0},{0,170,170},{170,0,0},{170,0,170},{170,170,0},{170,170,170},{85,85,85},{85,85,255},{85,255,85},{85,255,255},{255,85,85},{255,85,255},{255,255,85},{255,255,255}};
  const FXuchar *pp;
  FXuchar  Manufacturer=10;
  FXuchar  Version=5;
  FXuchar  Encoding=1;
  FXuchar  BitsPerPixel=8;
  FXuchar  NPlanes=3;
  FXuchar  Reserved=0;
  FXshort  PaletteInfo=1;
  FXshort  HRes=75;
  FXshort  VRes=75;
  FXuchar  fill=0;
  FXshort  Xmin=0;
  FXshort  Ymin=0;
  FXshort  Xmax=width-1;
  FXshort  Ymax=height-1;
  FXshort  BytesPerLine=width;          // FIXME see PCX.txt docs
  FXuchar  Current,Last,RLECount,rc;
  FXint    i,x,y,rgb;
  FXbool   swap;

  // Must make sense
  if(!data || width<=0 || height<=0) return false;

  // Switch to little-endian
  swap=store.swapBytes();
  store.setBigEndian(false);

  // Manufacturer, Version, Encoding and BitsPerPixel
  store << Manufacturer;
  store << Version;
  store << Encoding;
  store << BitsPerPixel;

  // Xmin = 0
  store << Xmin;

  // Ymin = 0
  store << Ymin;

  // Xmax = width - 1
  store << Xmax;

  // Ymax = height - 1
  store << Ymax;

  // HDpi = 75
  store << HRes;

  // VDpi = 75
  store << VRes;

  // Colormap
  store.save(Colormap[0],48);

  // Reserved
  store << Reserved;

  // NPlanes
  store << NPlanes;

  // BytesPerLine
  store << BytesPerLine;

  // PaletteInfo=1
  store << PaletteInfo;

  // Filler
  for(i=0; i<58; i++) store << fill;

  // Save as 24 bits/pixel
  for(y=0; y<height; y++){
    for(rgb=2; rgb>=0; rgb--){
      pp=((const FXuchar*)(data+y*width))+rgb;
      Last=*pp;
      pp+=4;
      RLECount=1;
      for(x=1; x<width; x++){
        Current=*pp;
        pp+=4;
        if(Current==Last){
          RLECount++;
          if(RLECount==63){
            rc=0xC0|RLECount;
            store << rc << Last;
            RLECount=0;
            }
          }
        else{
          if(RLECount){
            if((RLECount==1) && (0xC0!=(0xC0&Last))){
              store << Last;
              }
            else{
              rc=0xC0|RLECount;
              store << rc << Last;
              }
            }
          Last=Current;
          RLECount=1;
          }
        }
      if(RLECount){
        if((RLECount==1) && (0xC0!=(0xC0&Last))){
          store << Last;
          }
        else{
          rc=0xC0|RLECount;
          store << rc << Last;
          }
        }
      }
    }

  // Reset byte order
  store.swapBytes(swap);
  return true;
  }

}
