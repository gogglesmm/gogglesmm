/********************************************************************************
*                                                                               *
*                      T A R G A   I n p u t / O u t p u t                      *
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
  - Need to have checks in RLE decoder for out-of-bounds checking.
  - Need to try save image with fewer bits/pixel if possible.
  - To map from 5-bit to 8-bit, we use value*8+floor(value/4) which
    is almost the same as the correct value*8.225806.
  - Yes, in 16 bit its still 5,5,5 and not 5,6,5.
*/


using namespace FX;


/*******************************************************************************/

namespace FX {


#ifndef FXLOADTGA
extern FXAPI FXbool fxcheckTGA(FXStream& store);
extern FXAPI FXbool fxloadTGA(FXStream& store,FXColor*& data,FXint& width,FXint& height);
extern FXAPI FXbool fxsaveTGA(FXStream& store,const FXColor *data,FXint width,FXint height);
#endif


static FXbool loadTarga32(FXStream& store,FXColor* data,FXint width,FXint height,FXuchar imgdescriptor,FXuchar ImageType){
  FXint i,j,rc;
  FXuchar *pp,R,G,B,A,c;

  // 2 - Uncompressed, RGB images.
  if(ImageType==2){

    // Origin in upper left-hand corner
    if((imgdescriptor&0x20)==0x20){
      pp=(FXuchar*)data;
      for(i=0; i<height; i++){
        for(j=0; j<width; j++){
          store >> pp[0];       // Blue
          store >> pp[1];       // Green
          store >> pp[2];       // Red
          store >> pp[3];       // Alpha
          pp+=4;
          }
        }
      }

    // Origin in lower left-hand corner
    else{
      for(i=height-1; i>=0; i--){
        pp=(FXuchar*)(data+i*width);
        for(j=0; j<width; j++){
          store >> pp[0];       // Blue
          store >> pp[1];       // Green
          store >> pp[2];       // Red
          store >> pp[3];       // Alpha
          pp+=4;
          }
        }
      }
    }

  // 10 - Runlength encoded RGB images.
  else if(ImageType==10){

    // Origin in upper left-hand corner
    if((imgdescriptor&0x20)==0x20){
      pp=(FXuchar*)data;
      for(i=0; i<height; i++){
        j=0;
        while(j<width){

          // Read Repetition Count field
          store >> c;

          // Check for Run-length Packet
          if(c&128){
            rc=c-127;
            j+=rc;
            store >> B;
            store >> G;
            store >> R;
            store >> A;
            while(rc--){
              pp[0]=B;          // Blue
              pp[1]=G;          // Green
              pp[2]=R;          // Red
              pp[3]=A;          // Alpha
              pp+=4;
              }
            }

          // Raw Packet
          else{
            rc=c+1;
            j+=rc;
            while(rc--){
              store >> pp[0];   // Blue
              store >> pp[1];   // Green
              store >> pp[2];   // Red
              store >> pp[3];   // Alpha
              pp+=4;
              }
            }
          }
        }
      }

    // Origin in lower left-hand corner
    else{
      for(i=height-1; i>=0; i--){
        j=0;
        pp=(FXuchar*)(data+i*width);
        while(j<width){

          // Read Repetition Count field
          store >> c;

          // Check for Run-length Packet
          if(c&128){
            rc=c-127;
            j+=rc;
            store >> B;
            store >> G;
            store >> R;
            store >> A;
            while(rc--){
              pp[0]=B;          // Blue
              pp[1]=G;          // Green
              pp[2]=R;          // Red
              pp[3]=A;          // Alpha
              pp+=4;
              }
            }

          // Raw Packet
          else{
            rc=c+1;
            j+=rc;
            while(rc--){
              store >> pp[0];   // Blue
              store >> pp[1];   // Green
              store >> pp[2];   // Red
              store >> pp[3];   // Alpha
              pp+=4;
              }
            }
          }
        }
      }
    }
  return true;
  }


static FXbool loadTarga24(FXStream& store,FXColor* data,FXint width,FXint height,FXuchar imgdescriptor,FXuchar ImageType){
  int i,j,rc;
  FXuchar *pp,R,G,B,c;

  // 2 - Uncompressed, RGB images.
  if(ImageType == 2){

    // Origin in upper left-hand corner
    if((imgdescriptor&0x20)==0x20){
      pp=(FXuchar*)data;
      for(i=0; i<height; i++){
        for(j=0; j<width; j++){
          store >> pp[0];       // Blue
          store >> pp[1];       // Green
          store >> pp[2];       // Red
          pp[3]=255;            // Alpha
          pp+=4;
          }
        }
      }

    // Origin in lower left-hand corner
    else{
      for(i=height-1; i>=0; i--){
        pp=(FXuchar*)(data+i*width);
        for(j=0; j<width; j++){
          store >> pp[0];       // Blue
          store >> pp[1];       // Green
          store >> pp[2];       // Red
          pp[3]=255;            // Alpha
          pp+=4;
          }
        }
      }
    }

  // 10 - Runlength encoded RGB images.
  else if(ImageType==10){

    // Origin in upper left-hand corner
    if((imgdescriptor&0x20)==0x20){
      pp=(FXuchar*)data;
      for(i=0; i<height; i++){
        j=0;
        while(j<width){

          // Read Repetition Count field
          store >> c;

          // Check for Run-length Packet
          if(c&128){
            rc=c-127;
            j+=rc;
            store >> B;
            store >> G;
            store >> R;
            while(rc--){
              pp[0]=B;          // Blue
              pp[1]=G;          // Green
              pp[2]=R;          // Red
              pp[3]=255;        // Alpha
              pp+=4;
              }
            }

          // Raw Packet
          else{
            rc=c+1;
            j+=rc;
            while(rc--){
              store >> pp[0];   // Blue
              store >> pp[1];   // Green
              store >> pp[2];   // Red
              pp[3]=255;        // Alpha
              pp+=4;
              }
            }
          }
        }
      }

    // Origin in lower left-hand corner
    else{
      for(i=height-1; i>=0; i--){
        j=0;
        pp=(FXuchar*)(data+i*width);
        while(j<width){

          // Read Repetition Count field
          store >> c;

          // Check for Run-length Packet
          if(c&128){
            rc=c-127;
            j+=rc;
            store >> B;
            store >> G;
            store >> R;
            while(rc--){
              pp[0]=B;          // Blue
              pp[1]=G;          // Green
              pp[2]=R;          // Red
              pp[3]=255;        // Alpha
              pp+=4;
              }
            }

          // Raw Packet
          else{
            rc = c + 1;
            j += rc;
            while(rc--){
              store >> pp[0];   // Blue
              store >> pp[1];   // Green
              store >> pp[2];   // Red
              pp[3]=255;        // Alpha
              pp+=4;
              }
            }
          }
        }
      }
    }
  return true;
  }


static FXbool loadTarga16(FXStream& store,FXColor* data,FXint width,FXint height,FXuchar imgdescriptor,FXuchar ImageType){
  FXushort rgb16;
  FXint i,j,rc;
  FXuchar *pp,R,G,B,c;

  // 2 - Uncompressed, RGB images.
  if(ImageType==2){

    // Origin in upper left-hand corner
    if((imgdescriptor&0x20)==0x20){
      pp=(FXuchar*)data;
      for(i=0; i<height; i++){
        for(j=0; j<width; j++){
          store >> rgb16;
          pp[0]=((rgb16<<3)&0xf8)+((rgb16>>2)&7);       // Blue
          pp[1]=((rgb16>>2)&0xf8)+((rgb16>>7)&7);       // Green
          pp[2]=((rgb16>>7)&0xf8)+((rgb16>>12)&7);      // Red
          pp[3]=255;                                    // Alpha
          pp+=4;
          }
        }
      }

    // Origin in lower left-hand corner
    else{
      for(i=height-1; i>=0; i--){
        pp=(FXuchar*)(data+i*width);
        for(j=0; j<width; j++){
          store >> rgb16;
          pp[0]=((rgb16<<3)&0xf8)+((rgb16>>2)&7);       // Blue
          pp[1]=((rgb16>>2)&0xf8)+((rgb16>>7)&7);       // Green
          pp[2]=((rgb16>>7)&0xf8)+((rgb16>>12)&7);      // Red
          pp[3]=255;                                    // Alpha
          pp+=4;
          }
        }
      }
    }

  // 10 - Runlength encoded RGB images.
  else if(ImageType==10){

    // Origin in upper left-hand corner
    if((imgdescriptor&0x20)==0x20){
      pp=(FXuchar*)data;
      for(i=0; i<height; i++){
        j=0;
        while(j<width){

          // Read Repetition Count field
          store >> c;

          // Check for Run-length Packet
          if(c&128){
            rc=c-127;
            j+=rc;
            store >> rgb16;
            B=((rgb16<<3)&0xf8)+((rgb16>>2)&7);         // Blue
            G=((rgb16>>2)&0xf8)+((rgb16>>7)&7);         // Green
            R=((rgb16>>7)&0xf8)+((rgb16>>12)&7);        // Red
            while(rc--){
              pp[0]=B;                                  // Blue
              pp[1]=G;                                  // Green
              pp[2]=R;                                  // Red
              pp[3]=255;                                // Alpha
              pp+=4;
              }
            }

          // Raw Packet
          else{
            rc=c+1;
            j+=rc;
            while(rc--){
              store >> rgb16;
              pp[0]=((rgb16<<3)&0xf8)+((rgb16>>2)&7);   // Blue
              pp[1]=((rgb16>>2)&0xf8)+((rgb16>>7)&7);   // Green
              pp[2]=((rgb16>>7)&0xf8)+((rgb16>>12)&7);  // Red
              pp[3]=255;                                // Alpha
              pp+=4;
              }
            }
          }
        }
      }

    // Origin in lower left-hand corner
    else{
      for(i=height-1; i>=0; i--){
        j=0;
        pp=(FXuchar*)(data+i*width);
        while(j<width){

          // Read Repetition Count field
          store >> c;

          // Check for Run-length Packet
          if(c&128){
            rc=c-127;
            j+=rc;
            store >> rgb16;
            B=((rgb16<<3)&0xf8)+((rgb16>>2)&7);         // Blue
            G=((rgb16>>2)&0xf8)+((rgb16>>7)&7);         // Green
            R=((rgb16>>7)&0xf8)+((rgb16>>12)&7);        // Red
            while(rc--){
              pp[0]=B;                                  // Blue
              pp[1]=G;                                  // Green
              pp[2]=R;                                  // Red
              pp[3]=255;                                // Alpha
              pp+=4;
              }
            }

          // Raw Packet
          else{
            rc=c+1;
            j+=rc;
            while(rc--){
              store >> rgb16;
              pp[0]=((rgb16<<3)&0xf8)+((rgb16>>2)&7);   // Blue
              pp[1]=((rgb16>>2)&0xf8)+((rgb16>>7)&7);   // Green
              pp[2]=((rgb16>>7)&0xf8)+((rgb16>>12)&7);  // Red
              pp[3]=255;                                // Alpha
              pp+=4;
              }
            }
          }
        }
      }
    }
  return true;
  }


static FXbool loadTarga8(FXStream& store,FXColor* data,FXint width,FXint height,FXuchar colormap[][4],FXuchar imgdescriptor,FXuchar ImageType){
  FXint i,j,rc;
  FXuchar *pp;
  FXuchar R,G,B,A,c;

  // 1 - Uncompressed, color-mapped images
  if(ImageType==1){

    // Origin in upper left-hand corner
    if((imgdescriptor&0x20)==0x20){
      pp=(FXuchar*)data;
      for(i=0; i<height; i++){
        for(j=0; j<width; j++){
          store >> c;
          pp[0]=colormap[c][0];         // Blue
          pp[1]=colormap[c][1];         // Green
          pp[2]=colormap[c][2];         // Red
          pp[3]=colormap[c][3];         // Alpha
          pp+=4;
          }
        }
      }

    // Origin in lower left-hand corner
    else{
      for(i=height-1; i>=0; i--){
        pp=(FXuchar*)(data+i*width);
        for(j=0; j<width; j++){
          store >> c;
          pp[0]=colormap[c][0];         // Blue
          pp[1]=colormap[c][1];         // Green
          pp[2]=colormap[c][2];         // Red
          pp[3]=colormap[c][3];         // Alpha
          pp+=4;
          }
        }
      }
    }

  // 9 - Runlength encoded color-mapped images
  else if(ImageType==9){

    // Origin in upper left-hand corner
    if((imgdescriptor&0x20)==0x20){
      pp=(FXuchar*)data;
      for(i=0; i<height; i++){
        j=0;
        while(j<width){

          // Read Repetition Count field
          store >> c;

          // Check for Run-length Packet
          if(c&128){
            rc=c-127;
            j+=rc;
            store >> c;
            B=colormap[c][0];
            G=colormap[c][1];
            R=colormap[c][2];
            A=colormap[c][3];
            while(rc--){
              pp[0]=B;                  // Blue
              pp[1]=G;                  // Green
              pp[2]=R;                  // Red
              pp[3]=A;                  // Alpha
              pp+=4;
              }
            }

          // Raw Packet
          else{
            rc=c+1;
            j+=rc;
            while(rc--){
              store >> c;
              pp[0]=colormap[c][0];     // Blue
              pp[1]=colormap[c][1];     // Green
              pp[2]=colormap[c][2];     // Red
              pp[3]=colormap[c][3];     // Alpha
              pp+=4;
              }
            }
          }
        }
      }

    // Origin in lower left-hand corner
    else{
      for(i=height-1; i>=0; i--){
        j=0;
        pp=(FXuchar*)(data+i*width);
        while(j<width){

          // read Repetition Count field
          store >> c;

          // check for Run-length Packet
          if(c&128){
            rc=c-127;
            j+=rc;

            // read Pixel Value field
            store >> c;

            // get R,G,B values
            B=colormap[c][0];
            G=colormap[c][1];
            R=colormap[c][2];
            A=colormap[c][3];
            while(rc--){
              pp[0]=B;                  // Blue
              pp[1]=G;                  // Green
              pp[2]=R;                  // Red
              pp[3]=A;                  // Alpha
              pp+=4;
              }
            }

          // Raw Packet
          else{
            rc=c+1;
            j+=rc;
            while(rc--){
              store >> c;
              pp[0]=colormap[c][0];     // Blue
              pp[1]=colormap[c][1];     // Green
              pp[2]=colormap[c][2];     // Red
              pp[3]=colormap[c][3];     // Alpha
              pp+=4;
              }
            }
          }
        }
      }
    }
  return true;
  }


static FXbool loadTargaGray(FXStream& store,FXColor* data,FXint width,FXint height,FXuchar imgdescriptor,FXuchar ImageType){
  FXint i,j,rc;
  FXuchar *pp;
  FXuchar c;

  // 3 - Uncompressed, black and white images.
  if(ImageType==3){

    // Origin in upper left-hand corner
    if((imgdescriptor&0x20)==0x20){
      pp=(FXuchar*)data;
      for(i=0; i<height; i++){
        for(j=0; j<width; j++){
          store >> c;
          *pp++=c;
          *pp++=c;
          *pp++=c;
          *pp++=255;
          }
        }
      }

    // Origin in lower left-hand corner
    else{
      for(i=height-1; i>=0; i--){
        pp=(FXuchar*)(data+i*width);
        for(j=0; j<width; j++){
          store >> c;
          *pp++=c;
          *pp++=c;
          *pp++=c;
          *pp++=255;
          }
        }
      }
    }

  // 11 - Compressed, black and white images.
  else if(ImageType==11){

    // Origin in upper left-hand corner
    if((imgdescriptor&0x20)==0x20){
      pp=(FXuchar*)data;
      for(i=0; i<height; i++){
        j=0;
        while(j<width){

          // read Repetition Count field
          store >> c;

          // check for Run-length Packet
          if(c&128){
            rc=c-127;
            j+=rc;

            // read Pixel Value field
            store >> c;
            while(rc--){
              *pp++=c;
              *pp++=c;
              *pp++=c;
              *pp++=255;
              }
            }

          // Raw Packet
          else{
            rc=c+1;
            j+=rc;
            while(rc--){
              store >> c;
              *pp++=c;
              *pp++=c;
              *pp++=c;
              *pp++=255;
              }
            }
          }
        }
      }

    // Origin in lower left-hand corner
    else{
      for(i=height-1; i>=0; i--){
        j = 0;
        pp=(FXuchar*)(data+i*width);
        while(j<width){

          // read Repetition Count field
          store >> c;

          // check for Run-length Packet
          if(c&128){
            rc=c-127;
            j+=rc;

            // read Pixel Value field
            store >> c;
            while(rc--){
              *pp++=c;
              *pp++=c;
              *pp++=c;
              *pp++=255;
              }
            }

          // Raw Packet
          else{
            rc=c+1;
            j+=rc;
            while(rc--){
              store >> c;
              *pp++=c;
              *pp++=c;
              *pp++=c;
              *pp++=255;
              }
            }
          }
        }
      }
    }
  return true;
  }


// Check if stream contains a TARGA
FXbool fxcheckTGA(FXStream& store){
  FXuchar signature[3];
  store.load(signature,3);
  store.position(-3,FXFromCurrent);
  return (signature[1]==0 || signature[1]==1) && (signature[2]==1 || signature[2]==2 || signature[2]==3 || signature[2]==9 || signature[2]==10 || signature[2]==11 || signature[2]==32 || signature[2]==33);
  }


// Load Targa image from stream
FXbool fxloadTGA(FXStream& store,FXColor*& data,FXint& width,FXint& height){
  FXuchar  IDLength;
  FXuchar  ColorMapType;
  FXuchar  ImageType;
  FXshort  ColorMapOrigin;
  FXshort  ColorMapLength;
  FXuchar  ColorMapEntrySize;
  FXshort  XOrg;
  FXshort  YOrg;
  FXshort  Width;
  FXshort  Height;
  FXuchar  PixelDepth;
  FXuchar  ImageDescriptor;
  FXushort rgb16;
  FXuchar  colormap[256][4];
  FXbool   swap;
  FXbool   ok=false;
  FXint    i;

  // Null out
  data=nullptr;
  width=0;
  height=0;

  // Length of Image ID Field
  store >> IDLength;

  // Type of color map (if any) included with the image
  // 0 - indicates that no color-map data is included with this image
  // 1 - indicates that a color-map is included with this image
  store >> ColorMapType;

  // Image Type
  //  0 - No image data included.
  //  1 - Uncompressed, color-mapped images.
  //  2 - Uncompressed, RGB images.
  //  3 - Uncompressed, black and white images.
  //  9 - Runlength encoded color-mapped images.
  // 10 - Runlength encoded RGB images.
  // 11 - Compressed, black and white images.
  // 32 - Compressed color-mapped data, using Huffman, Delta, and runlength encoding.
  // 33 - Compressed color-mapped data, using Huffman, Delta, and runlength encoding.
  //      4-pass quadtree-type process.
  store >> ImageType;

  FXTRACE((100,"fxloadTGA IDLength=%d ColorMapType=%d ImageType=%d\n",IDLength,ColorMapType,ImageType));

  // Check for supported image type
  if(ImageType==1 || ImageType==2 || ImageType==3 || ImageType==9 || ImageType==10 || ImageType==11 || ImageType==32 || ImageType==33){

    // Switch to little-endian
    swap=store.swapBytes();
    store.setBigEndian(false);

    // First color map entry
    store >> ColorMapOrigin;

    // Color map length
    store >> ColorMapLength;

    // Don't load too many colors
    if(ColorMapLength>256) goto x;

    // Color map Entry Size
    // Establishes the number of bits per entry.
    // Typically 15, 16, 24 or 32-bit values are used.
    store >> ColorMapEntrySize;

    // X-origin of image and Y-origin of image
    store >> XOrg;
    store >> YOrg;

    // This field specifies the width of the image in pixels
    store >> Width;

    // This field specifies the height of the image in pixels
    store >> Height;

    // This field indicates the number of bits per pixel. This number includes
    // the Attribute or Alpha channel bits. Common values are 8, 16, 24 and 32
    // but other pixel depths could be used.
    store >> PixelDepth;

    FXTRACE((100,"fxloadTGA PixelDepth=%d ColorMapLength=%d ColorMapEntrySize=%d Width=%d Height=%d\n",PixelDepth,ColorMapLength,ColorMapEntrySize,Width,Height));

    // Sanity check
    if(PixelDepth!=1 && PixelDepth!=8 && PixelDepth!=15 && PixelDepth!=16 && PixelDepth!=24 && PixelDepth!=32) goto x;

    // Bits 3-0 - number of attribute bits associated with each pixel
    // Bit 4    - reserved.  Must be set to 0
    // Bit 5    - screen origin bit:
    //            0 = Origin in lower left-hand corner
    //            1 = Origin in upper left-hand corner
    //            Must be 0 for Truevision images
    // Bits 7-6 - Data storage interleaving flag:
    //            00 = non-interleaved
    //            01 = two-way (even/odd) interleaving
    //            10 = four way interleaving
    //            11 = reserved
    store >> ImageDescriptor;

    // Skip over Image Identification Field; its length is IDLength
    store.position(IDLength,FXFromCurrent);

    // Allocate memory
    if(allocElms(data,Width*Height)){

      // Set return size
      width=Width;
      height=Height;

      // Read color map
      if(0<ColorMapLength){
        switch(ColorMapEntrySize){
          case 15:          // 15- or 16-bit RGB
          case 16:
            for(i=0; i<ColorMapLength; i++){
              store >> rgb16;
              colormap[i][0]=((rgb16<<3)&0xf8)+((rgb16>>2)&7);      // Blue
              colormap[i][1]=((rgb16>>2)&0xf8)+((rgb16>>7)&7);      // Green
              colormap[i][2]=((rgb16>>7)&0xf8)+((rgb16>>12)&7);     // Red
              colormap[i][3]=255;                                   // Alpha
              }
            break;
          case 24:           // 24-bit RGB
            for(i=0; i<ColorMapLength; i++){
              store >> colormap[i][0];                              // Red
              store >> colormap[i][1];                              // Green
              store >> colormap[i][2];                              // Blue
              colormap[i][3]=255;                                   // Alpha
              }
            break;
          case 32:          // 32-bit RGBA
            for(i=0; i<ColorMapLength; i++){
              store >> colormap[i][0];                              // Red
              store >> colormap[i][1];                              // Green
              store >> colormap[i][2];                              // Blue
              store >> colormap[i][3];                              // Alpha
              }
            break;
          default:          // Unexpected depth
            goto x;
          }
        }

      FXTRACE((100,"fxloadTARGA: Width=%d Height=%d IDLength=%d ColorMapType=%d ColorMapLength=%d ColorMapEntrySize=%d ImageType=%d PixelDepth=%d ImageDescriptor=%02x\n",Width,Height,IDLength,ColorMapType,ColorMapLength,ColorMapEntrySize,ImageType,PixelDepth,ImageDescriptor));

      // Load up the image
      if(PixelDepth==32 && (ImageType==2 || ImageType==10)){
        ok=loadTarga32(store,data,Width,Height,ImageDescriptor,ImageType);
        }
      else if(PixelDepth==24 && (ImageType==2 || ImageType==10)){
        ok=loadTarga24(store,data,Width,Height,ImageDescriptor,ImageType);
        }
      else if(PixelDepth==16 && (ImageType==2 || ImageType==10)){
        ok=loadTarga16(store,data,Width,Height,ImageDescriptor,ImageType);
        }
      else if(PixelDepth==15 && (ImageType==2 || ImageType==10)){
        ok=loadTarga16(store,data,Width,Height,ImageDescriptor,ImageType);
        }
      else if(PixelDepth==8 && (ImageType==1 || ImageType==9)){
        ok=loadTarga8(store,data,Width,Height,colormap,ImageDescriptor,ImageType);
        }
      else if(ImageType==3 || ImageType==11){
        ok=loadTargaGray(store,data,Width,Height,ImageDescriptor,ImageType);
        }
      }

    // Reset byte order
x:  store.swapBytes(swap);
    }
  return ok;
  }

/*******************************************************************************/


// Save a Targa file to a stream
FXbool fxsaveTGA(FXStream& store,const FXColor *data,FXint width,FXint height){
  FXuchar IDLength=0;
  FXuchar ColorMapType=0;
  FXuchar ImageType=2;
  FXshort ColorMapOrigin=0;
  FXshort ColorMapLength=0;
  FXuchar ColorMapEntrySize=0;
  FXshort XOrg=0;
  FXshort YOrg=0;
  FXshort Width=width;
  FXshort Height=height;
  FXuchar PixelDepth=32;
  FXuchar ImageDescriptor=8;
  FXbool  swap;
  FXint   i,j;

  // Must make sense
  if(!data || width<=0 || height<=0) return false;

  // Switch to little-endian
  swap=store.swapBytes();
  store.setBigEndian(false);

  // Length of Image ID Field
  store << IDLength;

  // Type of color map
  store << ColorMapType;

  // Image Type
  store << ImageType;

  // Index of the first color map entry
  store << ColorMapOrigin;

  // Color map length
  store << ColorMapLength;

  // Color map entry size
  store << ColorMapEntrySize;

  // X-origin of image and Y-origin of image
  store << XOrg;
  store << YOrg;

  // Width of the image in pixels
  store << Width;

  // Height of the image in pixels
  store << Height;

  // This field indicates the number of bits per pixel
  store << PixelDepth;

  // Bits 3-0 - number of attribute bits associated with each pixel
  // Bit 4    - reserved.  Must be set to 0
  // Bit 5    - screen origin bit:
  //            0 = Origin in lower left-hand corner
  //            1 = Origin in upper left-hand corner
  //            Must be 0 for Truevision images
  // Bits 7-6 - Data storage interleaving flag:
  //            00 = non-interleaved
  //            01 = two-way (even/odd) interleaving
  //            10 = four way interleaving
  //            11 = reserved
  store << ImageDescriptor;

  // Write image
  for(i=height-1; i>=0; i--){
    for(j=0; j<width; j++){
      store << ((const FXuchar*)(&data[i*width+j]))[0];
      store << ((const FXuchar*)(&data[i*width+j]))[1];
      store << ((const FXuchar*)(&data[i*width+j]))[2];
      store << ((const FXuchar*)(&data[i*width+j]))[3];
      }
    }

  // Reset byte order
  store.swapBytes(swap);
  return true;
  }

}

