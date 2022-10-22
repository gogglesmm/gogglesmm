/********************************************************************************
*                                                                               *
*                          B M P   I n p u t / O u t p u t                      *
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
#include "fxendian.h"
#include "FXArray.h"
#include "FXHash.h"
#include "FXElement.h"
#include "FXStream.h"

/*
  Notes:
  - Writer should use fxezquantize() and if the number of colors is less than
    256, use 8bpp RLE compressed output; if less that 4, use 4bpp RLE compressed
    output, else if less than 2, use monochrome.
  - Writer should do this only when no loss of fidelity occurs.
  - Find documentation on 32-bpp bitmap.
  - Need to have checks in RLE decoder for out-of-bounds checking.
  - To map from 5-bit to 8-bit, we use value*8+floor(value/4) which
    is almost the same as the correct value*8.225806.
*/

// Bitmap compression values
#define BIH_RGB            0    // RGB mode
#define BIH_RLE8           1    // 8-bit/pixel rle mode
#define BIH_RLE4           2    // 4-bit/pixel rle mode
#define BIH_BITFIELDS      3    // Bit field mode
#define BIH_JPEG           4    // Not supported
#define BIH_PNG            5    // Not supported
#define BIH_ALPHABITFIELDS 6    // RGBA bit field masks
#define BIH_CMYK           11   // none
#define BIH_CMYKRLE8       12   // RLE-8
#define BIH_CMYKRLE4       13   // RLE-4

#define RLE_ESC       0         // RLE escape sequence
#define RLE_LINE      0         // RLE end of line
#define RLE_END       1         // RLE end of bitmap
#define RLE_DELTA     2         // RLE delta

#define OS2_OLD       12        // Bitmap Info Header sizes
#define OS2_NEW       64
#define WIN_NEW       40

#define IDH_ICO       1         // ICO
#define IDH_CUR       2         // CUR


using namespace FX;

/*******************************************************************************/

namespace FX {


// Check BMP, ICO/CUR file based on contents
extern FXAPI FXbool fxcheckBMP(FXStream& store);
extern FXAPI FXbool fxcheckICO(FXStream& store);

// Load / save BMP
extern FXAPI FXbool fxloadBMP(FXStream& store,FXColor*& data,FXint& width,FXint& height);
extern FXAPI FXbool fxsaveBMP(FXStream& store,const FXColor *data,FXint width,FXint height);

// Load / save DIB
extern FXAPI FXbool fxloadDIB(FXStream& store,FXColor*& data,FXint& width,FXint& height);
extern FXAPI FXbool fxsaveDIB(FXStream& store,const FXColor *data,FXint width,FXint height);

// Load / save ICO or CUR
extern FXAPI FXbool fxloadICO(FXStream& store,FXColor*& data,FXint& width,FXint& height,FXint& xspot,FXint& yspot);
extern FXAPI FXbool fxsaveICO(FXStream& store,const FXColor *data,FXint width,FXint height,FXint xspot=-1,FXint yspot=-1);

extern FXAPI FXbool fxloadICOStream(FXStream& store,FXColor*& data,FXint& width,FXint& height);


// Bitmap Info Header
struct BitmapInfoHeader {
  FXuint   biSize;
  FXint    biWidth;
  FXint    biHeight;
  FXushort biPlanes;
  FXushort biBitCount;
  FXuint   biCompression;
  FXuint   biSizeImage;
  FXint    biXPelsPerMeter;
  FXint    biYPelsPerMeter;
  FXuint   biClrUsed;
  FXuint   biClrImportant;
  };


// Bitmap File Header
struct BitmapFileHeader {
  FXushort bfType;              // BM
  FXuint   bfSize;
  FXushort bfReserved1;
  FXushort bfReserved2;
  FXuint   bfOffBits;
  };


// Icon Directory
struct IconDirectory {
  FXushort idReserved;          // Must be 0
  FXushort idType;              // ICO=1, CUR=2
  FXushort idCount;             // Number of images
  };


// Icon Directory Entry
struct IconDirectoryEntry {
  FXuchar  bWidth;
  FXuchar  bHeight;
  FXuchar  bColorCount;
  FXuchar  bReserved;
  FXushort wXHotspot;           // X hotspot if cursor, #planes if icon
  FXushort wYHotspot;           // Y hotspot if cursor, #bits/pixel if icon
  FXuint   dwBytesInRes;
  FXuint   dwImageOffset;
  };

/*******************************************************************************/

// Check if stream contains a BMP
FXbool fxcheckBMP(FXStream& store){
  FXuchar signature[2];
  store.load(signature,2);
  store.position(-2,FXFromCurrent);
  return signature[0]=='B' && signature[1]=='M';
  }


// Check if stream contains ICO or CUR
FXbool fxcheckICO(FXStream& store){
  FXbool swap=store.swapBytes();
  FXshort signature[3];
  store.setBigEndian(false);
  store.load(signature,3);
  store.position(-6,FXFromCurrent);
  store.swapBytes(swap);
  return signature[0]==0 && (signature[1]==IDH_ICO || signature[1]==IDH_CUR) && signature[2]>=1;
  }

/*******************************************************************************/

// Load bitmap bits
static FXbool fxloadBMPBits(FXStream& store,FXColor*& data,FXint width,FXint height,FXint bpp,FXint enc,FXint clrs,FXint fmt){
  if(allocElms(data,width*height)){
    FXColor  colormap[256],c1,c2;
    FXuchar  padding[4],r,g,b,a;
    FXint    pad,i,x,y;
    FXushort rgb16;

    // Otherwise, maybe a map
    if(bpp<=8){

      // OS2 has 3-byte colormaps
      if(fmt==3){
        for(i=0; i<clrs; i++){
          store >> b;                           // Blue
          store >> g;                           // Green
          store >> r;                           // Red
          colormap[i]=FXRGB(r,g,b);
          }
        }

      // Microsoft bitmaps have 4-byte colormaps
      else{
        for(i=0; i<clrs; i++){
          store >> b;                           // Blue
          store >> g;                           // Green
          store >> r;                           // Red
          store >> a;
          colormap[i]=FXRGB(r,g,b);
          }
        }
      }

    // Handle various depths
    switch(bpp){
      case 1:                                   // 1-bit/pixel
        pad=(width+31)&~31;                     // Padded to DWORD
        for(y=height-1; y>=0; y--){
          for(x=0; x<pad; x++){
            if((x&7)==0){ store >> b; }
            if(__unlikely(x>=width)) continue;
            data[y*width+x]=colormap[(b>>7)&1];
            b<<=1;
            }
          }
        return true;
      case 4:                                   // 4-bit/pixel
        if(enc==BIH_RLE4){                      // Read RLE4 compressed data
          x=0;
          y=height-1;
          while(!store.eof()){
            store >> a;
            store >> b;
            if(a==RLE_ESC){                     // Escape code
              if(b==RLE_END){                   // End of data
                break;
                }
              if(b==RLE_LINE){                  // End of line
                x=0;
                y--;
                continue;
                }
              if(b==RLE_DELTA){                 // Delta
                store >> a; x+=a;
                store >> a; y-=a;
                continue;
                }
              if(__unlikely(y<0)) break;        // Safety check
              for(i=0; i<b; ++i){               // Absolute mode
                if(i&1){
                  c1=colormap[a&15];
                  }
                else{
                  store >> a;
                  c1=colormap[a>>4];
                  }
                if(__unlikely(x>=width)) continue;
                data[y*width+x++]=c1;
                }
              if(((b&3)==1) || ((b&3)==2)) store >> a;          // Read pad byte
              }
            else{                               // Repeat mode
              if(__unlikely(y<0)) break;        // Safety check
              c1=colormap[b>>4];
              c2=colormap[b&15];
              for(i=0; i<a && x<width; ++i){
                data[y*width+x++]=(i&1)?c2:c1;
                }
              }
            }
          }
        else{                                   // Read uncompressed data
          pad=(width+7)&~7;                     // Padded to DWORD
          for(y=height-1; y>=0; y--){
            for(x=0; x<pad; x+=2){
              store >> a;
              if(__unlikely(x>=width)) continue;
              data[y*width+x]=colormap[a>>4];
              if(__unlikely(x+1>=width)) continue;
              data[y*width+x+1]=colormap[a&15];
              }
            }
          }
        return true;
      case 8:                                   // 8-bit/pixel
        if(enc==BIH_RLE8){                      // Read RLE8 compressed data
          x=0;
          y=height-1;
          while(!store.eof()){
            store >> a;
            store >> b;
            if(a==RLE_ESC){                     // Escape code
              if(b==RLE_END){                   // End of data
                break;
                }
              if(b==RLE_LINE){                  // End of line
                x=0;
                y--;
                continue;
                }
              if(b==RLE_DELTA){                 // Delta
                store >> a; x+=a;
                store >> a; y-=a;
                continue;
                }
              if(__unlikely(y<0)) break;        // Safety check
              for(i=0; i<b && x<width; ++i){    // Absolute mode
                store >> a;
                data[y*width+x++]=colormap[a];
                }
              if(b&1) store >> a;               // Odd length run: read an extra pad byte
              }
            else{                               // Repeat mode
              if(__unlikely(y<0)) break;        // Safety check
              c1=colormap[b];
              for(i=0; i<a && x<width; ++i){
                data[y*width+x++]=c1;
                }
              }
            }
          }
        else{                                   // Read uncompressed data
          pad=(4-(width&3))&3;                  // Padded to DWORD
          for(y=height-1; y>=0; y--){
            for(x=0; x<width; x++){
              store >> a;
              data[y*width+x]=colormap[a];
              }
            store.load(padding,pad);
            }
          }
        return true;
      case 16:                                  // 16-bit/pixel
        pad=(4-((width*2)&3))&3;                // Padded to DWORD
        for(y=height-1; y>=0; y--){
          for(x=0; x<width; x++){
            store >> rgb16;
            r=((rgb16<<3)&0xf8)+((rgb16>> 2)&7);
            g=((rgb16>>2)&0xf8)+((rgb16>> 7)&7);
            b=((rgb16>>7)&0xf8)+((rgb16>>12)&7);
            data[y*width+x]=FXRGB(r,g,b);
            }
          store.load(padding,pad);
          }
        return true;
      case 24:                                  // 24-bit/pixel
        pad=(4-((width*3)&3))&3;                // Padded to DWORD
        for(y=height-1; y>=0; y--){
          for(x=0; x<width; x++){
            store >> b;
            store >> g;
            store >> r;
            data[y*width+x]=FXRGB(r,g,b);
            }
          store.load(padding,pad);
          }
        return true;
      case 32:                                  // 32-bit/pixel
        for(y=height-1; y>=0; y--){
          for(x=0; x<width; x++){
            store >> b;
            store >> g;
            store >> r;
            store >> a;
            data[y*width+x]=FXRGBA(r,g,b,a);
            }
          }
        return true;
      }
    }
  return false;
  }


// Save bitmap bits
static FXbool fxsaveBMPBits(FXStream& store,const FXColor* data,FXint width,FXint height,FXint bpp){
  const FXuchar padding[3]={0,0,0};
  FXuchar pad,r,g,b,a;
  FXint x,y;

  // Handle various depths
  switch(bpp){
    case 24:                                    // 24-bit/pixel
      pad=(4-((width*3)&3))&3;                  // Padded to DWORD
      for(y=height-1; y>=0; y--){
        for(x=0; x<width; x++){
          r=FXREDVAL(data[y*width+x]);
          g=FXGREENVAL(data[y*width+x]);
          b=FXBLUEVAL(data[y*width+x]);
          store << b;
          store << g;
          store << r;
          }
        store.save(padding,pad);
        }
      return true;
    case 32:                                    // 32-bit/pixel
      for(y=height-1; y>=0; y--){
        for(x=0; x<width; x++){
          r=FXREDVAL(data[y*width+x]);
          g=FXGREENVAL(data[y*width+x]);
          b=FXBLUEVAL(data[y*width+x]);
          a=FXALPHAVAL(data[y*width+x]);
          store << b;
          store << g;
          store << r;
          store << a;
          }
        }
      return true;
    }
  return false;
  }

/*******************************************************************************/

// Load icon bits
static FXbool fxloadICOBits(FXStream& store,FXColor*& data,FXint width,FXint height,FXint bpp,FXint enc,FXint clrs,FXint fmt){
  FXint x,y,pad; FXuchar c;

  // Load pixels (XOR bytes)
  if(fxloadBMPBits(store,data,width,height,bpp,enc,clrs,fmt)){

    // Use AND bytes to set alpha channel
    if(bpp<32){
      pad=(4-((width+7)>>3))&3;         // Padded to DWORD
      for(y=height-1; y>=0; y--){
        for(x=0; x<width; x++){
          if((x&7)==0){ store >> c; }
          if(c&0x80) data[y*width+x]&=FXRGBA(255,255,255,0);
          c<<=1;
          }
        store.position(pad,FXFromCurrent);
        }
      }

    // Got alpha, so skip over AND bytes
    else{
      pad=((width+31)>>5)<<2;           // Width rounded up to DWORD
      store.position(height*pad,FXFromCurrent);
      }
    return true;
    }
  return false;
  }


// Save icon bits
static FXbool fxsaveICOBits(FXStream& store,const FXColor* data,FXint width,FXint height,FXint bpp){
  const FXuchar padding[3]={0,0,0};
  FXint x,y,pad; FXuchar c,bit;

  // Save pixels (XOR bytes)
  if(fxsaveBMPBits(store,data,width,height,bpp)){

    // Write AND bytes from alpha channel
    pad=(4-((width+7)>>3))&3;           // Padded to DWORD
    for(y=height-1; y>=0; y--){
      for(x=c=0,bit=0x80; x<width; x++){
        if((data[y*width+x]&FXRGBA(0,0,0,255))==0) c|=bit;
        bit>>=1;
        if(bit==0){
          store << c;
          bit=0x80;
          c=0;
          }
        }
      store.save(padding,pad);
      }
    return true;
    }
  return false;
  }


// 32 npp if alpha, 24 bpp otherwise
static FXushort checkBPP(const FXColor *data,FXint width,FXint height){
  for(FXint i=0; i<width*height; ++i){
    if((data[i]&FXRGBA(0,0,0,255))<FXRGBA(0,0,0,255)){ return 32; }
    }
  return 24;
  }

/*******************************************************************************/

// Load BMP image from stream
FXbool fxloadBMP(FXStream& store,FXColor*& data,FXint& width,FXint& height){
  FXbool swap=store.swapBytes();
  FXbool result=false;
  FXint colors;
  FXint format;
  FXushort ss;

  // Null out
  data=nullptr;
  width=0;
  height=0;

  // Make little-endian
  store.setBigEndian(false);

  // Get size and offset
  BitmapFileHeader bfh;
  store >> bfh.bfType;
  store >> bfh.bfSize;
  store >> bfh.bfReserved1;
  store >> bfh.bfReserved2;
  store >> bfh.bfOffBits;

  // Check signature
  if(bfh.bfType==0x4d42){

    // Read bitmap info header
    BitmapInfoHeader bmi;
    store >> bmi.biSize;

    // Old OS/2 format header
    if(bmi.biSize==OS2_OLD){
      store >> ss; bmi.biWidth=ss;
      store >> ss; bmi.biHeight=ss;
      store >> bmi.biPlanes;
      store >> bmi.biBitCount;
      bmi.biCompression=BIH_RGB;
      bmi.biSizeImage=(((bmi.biPlanes*bmi.biBitCount*bmi.biWidth)+31)>>5)*4*bmi.biHeight;
      bmi.biXPelsPerMeter=0;
      bmi.biYPelsPerMeter=0;
      bmi.biClrUsed=0;
      bmi.biClrImportant=0;
      }

    // New Windows header
    else{
      store >> bmi.biWidth;
      store >> bmi.biHeight;
      store >> bmi.biPlanes;
      store >> bmi.biBitCount;
      store >> bmi.biCompression;
      store >> bmi.biSizeImage;
      store >> bmi.biXPelsPerMeter;
      store >> bmi.biYPelsPerMeter;
      store >> bmi.biClrUsed;
      store >> bmi.biClrImportant;
      store.position(bmi.biSize-WIN_NEW,FXFromCurrent);
      }

    FXTRACE((100,"fxloadBMP: biSize=%d biWidth=%d biHeight=%d biPlanes=%d biBitCount=%d biCompression=%d biSizeImage=%u biClrUsed=%u biClrImportant=%u\n",bmi.biSize,bmi.biWidth,bmi.biHeight,bmi.biPlanes,bmi.biBitCount,bmi.biCompression,bmi.biSizeImage,bmi.biClrUsed,bmi.biClrImportant));

    // Check for sensible inputs
    if(bmi.biPlanes==1 && 0<bmi.biWidth && 0<bmi.biHeight && bmi.biClrUsed<=256 && bmi.biCompression<=BIH_RLE4){

      // Width and height
      width=bmi.biWidth;
      height=FXABS(bmi.biHeight);
      colors=bmi.biClrUsed?bmi.biClrUsed:1<<bmi.biBitCount;
      format=(bmi.biSize==OS2_OLD||bmi.biSize==OS2_NEW)?3:4;

      // Load the bits
      result=fxloadBMPBits(store,data,width,height,bmi.biBitCount,bmi.biCompression,colors,format);
      }
    }

  // Restore byte order
  store.swapBytes(swap);
  return result;
  }

/*******************************************************************************/

// Load DIB image from stream
FXbool fxloadDIB(FXStream& store,FXColor*& data,FXint& width,FXint& height){
  FXbool swap=store.swapBytes();
  FXbool result=false;
  FXint colors;

  // Null out
  data=nullptr;
  width=0;
  height=0;

  // Make little-endian
  store.setBigEndian(false);

  // Read bitmap info header
  BitmapInfoHeader bmi;
  store >> bmi.biSize;
  store >> bmi.biWidth;
  store >> bmi.biHeight;
  store >> bmi.biPlanes;
  store >> bmi.biBitCount;
  store >> bmi.biCompression;
  store >> bmi.biSizeImage;
  store >> bmi.biXPelsPerMeter;
  store >> bmi.biYPelsPerMeter;
  store >> bmi.biClrUsed;
  store >> bmi.biClrImportant;

  FXTRACE((100,"fxloadBMPStream: biSize=%d biWidth=%d biHeight=%d biPlanes=%d biBitCount=%d biCompression=%d biSizeImage=%u biClrUsed=%u biClrImportant=%u\n",bmi.biSize,bmi.biWidth,bmi.biHeight,bmi.biPlanes,bmi.biBitCount,bmi.biCompression,bmi.biSizeImage,bmi.biClrUsed,bmi.biClrImportant));

  // Check for sensible inputs
  if(bmi.biPlanes==1 && 0<bmi.biWidth && 0<bmi.biHeight && bmi.biClrUsed<=256 && bmi.biCompression<=BIH_RLE4){

    // Width and height
    width=bmi.biWidth;
    height=FXABS(bmi.biHeight);
    colors=bmi.biClrUsed?bmi.biClrUsed:1<<bmi.biBitCount;

    // Skip rest of header
    store.position(bmi.biSize-sizeof(BitmapInfoHeader),FXFromCurrent);

    // Load the bits
    result=fxloadBMPBits(store,data,width,height,bmi.biBitCount,bmi.biCompression,colors,4);
    }

  // Restore byte order
  store.swapBytes(swap);
  return result;
  }

/*******************************************************************************/

// Save BMP image to file stream
FXbool fxsaveBMP(FXStream& store,const FXColor *data,FXint width,FXint height){
  FXbool result=false;

  // Must make sense
  if(data && 0<width && 0<height){

    // Save byte order
    FXbool swap=store.swapBytes();

    // Use alpha channel if image not opaque
    FXushort bpp=checkBPP(data,width,height);

    // Make little-endian
    store.setBigEndian(false);

    // BitmapFileHeader
    BitmapFileHeader bfh={0x4d42,FXuint(14+WIN_NEW+height*(((width*bpp+31)>>5)<<2)),0,0,14+WIN_NEW};

    // Initialize bitmap info header
    BitmapInfoHeader bmi={WIN_NEW,width,height,1,bpp,BIH_RGB,FXuint(height*(((width*bpp+31)>>5)<<2)),75*39,75*39,0,0};

    // BitmapFileHeader
    store << bfh.bfType;        // Magic number
    store << bfh.bfSize;        // File size
    store << bfh.bfReserved1;   // bfReserved1
    store << bfh.bfReserved2;   // bfReserved2
    store << bfh.bfOffBits;     // bfOffBits

    // Bitmap Info Header
    store << bmi.biSize;
    store << bmi.biWidth;
    store << bmi.biHeight;
    store << bmi.biPlanes;
    store << bmi.biBitCount;            // biBitCount (1,4,8,24, or 32)
    store << bmi.biCompression;         // biCompression:  BIH_RGB, BIH_RLE8, BIH_RLE4, or BIH_BITFIELDS
    store << bmi.biSizeImage;
    store << bmi.biXPelsPerMeter;       // biXPelsPerMeter: (75dpi * 39" per meter)
    store << bmi.biYPelsPerMeter;       // biYPelsPerMeter: (75dpi * 39" per meter)
    store << bmi.biClrUsed;
    store << bmi.biClrImportant;

    // Save pixels
    result=fxsaveBMPBits(store,data,width,height,bpp);

    // Restore byte order
    store.swapBytes(swap);
    }
  return result;
  }

/*******************************************************************************/

// Save DIB image to stream
FXbool fxsaveDIB(FXStream& store,const FXColor *data,FXint width,FXint height){
  FXbool result=false;

  // Must make sense
  if(data && 0<width && 0<height){

    // Save byte order
    FXbool swap=store.swapBytes();

    // Use alpha channel if image not opaque
    FXushort bpp=checkBPP(data,width,height);

    // Make little-endian
    store.setBigEndian(false);

    // Initialize bitmap info header
    BitmapInfoHeader bmi={WIN_NEW,width,height,1,bpp,BIH_RGB,FXuint(height*(((width*bpp+31)>>5)<<2)),75*39,75*39,0,0};

    // Bitmap Info Header
    store << bmi.biSize;
    store << bmi.biWidth;
    store << bmi.biHeight;
    store << bmi.biPlanes;
    store << bmi.biBitCount;            // biBitCount (1,4,8,24, or 32)
    store << bmi.biCompression;         // biCompression:  BIH_RGB, BIH_RLE8, BIH_RLE4, or BIH_BITFIELDS
    store << bmi.biSizeImage;           // Image size in bytes
    store << bmi.biXPelsPerMeter;       // biXPelsPerMeter: (75dpi * 39" per meter)
    store << bmi.biYPelsPerMeter;       // biYPelsPerMeter: (75dpi * 39" per meter)
    store << bmi.biClrUsed;
    store << bmi.biClrImportant;

    // Save pixels
    result=fxsaveBMPBits(store,data,width,height,bpp);

    // Restore byte order
    store.swapBytes(swap);
    }
  return result;
  }

/*******************************************************************************/

// Load ICO image from stream
FXbool fxloadICO(FXStream& store,FXColor*& data,FXint& width,FXint& height,FXint& xspot,FXint& yspot){
  FXbool swap=store.swapBytes();
  FXbool result=false;
  FXint colors;

  // Null out
  data=nullptr;
  width=0;
  height=0;
  xspot=-1;
  yspot=-1;

  // Make little-endian
  store.setBigEndian(false);

  // Icon Directory Header
  IconDirectory icd;
  store >> icd.idReserved;      // Must be zero
  store >> icd.idType;          // Must be 1 (icon) or 2 (cursor)
  store >> icd.idCount;         // Only one icon

  // Validity of icon directory header
  if(icd.idReserved==0 && 0<icd.idCount && (icd.idType==IDH_ICO || icd.idType==IDH_CUR)){

    // Icon Directory Entry
    IconDirectoryEntry ice;
    store >> ice.bWidth;
    store >> ice.bHeight;
    store >> ice.bColorCount;     // 0 for > 8bit/pixel
    store >> ice.bReserved;       // 0
    store >> ice.wXHotspot;       // X hotspot if cursor, #planes if icon
    store >> ice.wYHotspot;       // Y hotspot if cursor, #bits/pixel if icon
    store >> ice.dwBytesInRes;    // Total number of bytes in images (including palette data)
    store >> ice.dwImageOffset;   // Location of image from the beginning of file

    // Skip to bitmap info header
    store.position(ice.dwImageOffset-22,FXFromCurrent);

    // Initialize bitmap info header
    BitmapInfoHeader bmi;
    store >> bmi.biSize;
    store >> bmi.biWidth;
    store >> bmi.biHeight;
    store >> bmi.biPlanes;
    store >> bmi.biBitCount;
    store >> bmi.biCompression;
    store >> bmi.biSizeImage;
    store >> bmi.biXPelsPerMeter;
    store >> bmi.biYPelsPerMeter;
    store >> bmi.biClrUsed;
    store >> bmi.biClrImportant;

    // Skip rest of header
    store.position(bmi.biSize-WIN_NEW,FXFromCurrent);

    FXTRACE((100,"fxloadICO: idCount=%d idType=%d wXHotspot=%d wYHotspot=%d\n",icd.idCount,icd.idType,ice.wXHotspot,ice.wYHotspot));
    FXTRACE((100,"fxloadICO: biSize=%d biWidth=%d biHeight=%d biPlanes=%d biBitCount=%d biCompression=%d biSizeImage=%u biClrUsed=%u biClrImportant=%u\n",bmi.biSize,bmi.biWidth,bmi.biHeight,bmi.biPlanes,bmi.biBitCount,bmi.biCompression,bmi.biSizeImage,bmi.biClrUsed,bmi.biClrImportant));

    // Check for sensible inputs
    if(bmi.biPlanes==1 && 0<bmi.biWidth && 0<bmi.biHeight && bmi.biClrUsed<=256){

      // Width and height
      width=bmi.biWidth;
      height=FXABS(bmi.biHeight)/2;
      colors=bmi.biClrUsed?bmi.biClrUsed:1<<bmi.biBitCount;

      // Copy hotspot location if cursor
      if(icd.idType==IDH_CUR){
        xspot=ice.wXHotspot;
        yspot=ice.wYHotspot;
        }

      // Load the bits
      result=fxloadICOBits(store,data,width,height,bmi.biBitCount,bmi.biCompression,colors,4);
      }
    }

  // Restore byte order
  store.swapBytes(swap);
  return result;
  }


// Load ICO Image from stream
FXbool fxloadICOStream(FXStream& store,FXColor*& data,FXint& width,FXint& height){
  FXbool swap=store.swapBytes();
  FXbool result=false;
  FXint colors;

  // Null out
  data=nullptr;
  width=0;
  height=0;

  // Bitmaps are little-endian
  store.setBigEndian(false);

  // Read bitmap info header
  BitmapInfoHeader bmi;
  store >> bmi.biSize;
  store >> bmi.biWidth;
  store >> bmi.biHeight;
  store >> bmi.biPlanes;
  store >> bmi.biBitCount;
  store >> bmi.biCompression;
  store >> bmi.biSizeImage;
  store >> bmi.biXPelsPerMeter;
  store >> bmi.biYPelsPerMeter;
  store >> bmi.biClrUsed;
  store >> bmi.biClrImportant;

  // Skip rest of header
  store.position(bmi.biSize-WIN_NEW,FXFromCurrent);

  FXTRACE((100,"fxloadICOStream: biSize=%d biWidth=%d biHeight=%d biBitCount=%d biCompression=%d biSizeImage=%d biClrUsed=%d\n",bmi.biSize,bmi.biWidth,bmi.biHeight,bmi.biBitCount,bmi.biCompression,bmi.biSizeImage,bmi.biClrUsed));

  // Check for sensible inputs
  if(bmi.biPlanes==1 && 0<bmi.biWidth && 0<bmi.biHeight && bmi.biClrUsed<=256 && bmi.biCompression<=BIH_RLE4){

    // Width and height
    width=bmi.biWidth;
    height=FXABS(bmi.biHeight)/2;         // Topsy turvy possibility; adjust height also
    colors=bmi.biClrUsed?bmi.biClrUsed:1<<bmi.biBitCount;

    // Load the bits
    result=fxloadICOBits(store,data,width,height,bmi.biBitCount,bmi.biCompression,colors,4);
    }

  // Restore byte order
  store.swapBytes(swap);
  return result;
  }

/*******************************************************************************/

// Save a ICO file to a stream
FXbool fxsaveICO(FXStream& store,const FXColor *data,FXint width,FXint height,FXint xspot,FXint yspot){
  FXbool result=false;

  // Must make sense
  if(data && 0<width && 0<height && width<256 && height<256){

    // Save byte order
    FXbool swap=store.swapBytes();

    // Use alpha channel if image not opaque
    FXushort bpp=checkBPP(data,width,height);

    // Bitmaps are little-endian
    store.setBigEndian(false);

    // Initialize icon directory header
    IconDirectory icd={0,IDH_CUR,1};

    // Initialize icon directory entry
    IconDirectoryEntry ice={(FXuchar)width,(FXuchar)height,0,0,(FXushort)xspot,(FXushort)yspot,FXuint(WIN_NEW+height*((((width*bpp+31)>>5)<<2)+(((width+31)>>5)<<2))),22};

    // Initialize bitmap info header
    BitmapInfoHeader bmi={WIN_NEW,width,height*2,1,bpp,BIH_RGB,FXuint(height*(((width*bpp+31)>>5)<<2)),75*39,75*39,0,0};

    // Save as ico if no hotspot
    if(xspot<0 || yspot<0){
      icd.idType=IDH_ICO;
      ice.wXHotspot=1;
      ice.wYHotspot=bpp;
      }

    // Icon Directory Header
    store << icd.idReserved;      // Must be zero
    store << icd.idType;          // Must be 1 (icon) or 2 (cursor)
    store << icd.idCount;         // Only one icon

    // Icon Directory Entry
    store << ice.bWidth;
    store << ice.bHeight;
    store << ice.bColorCount;     // 0 for > 8bit/pixel
    store << ice.bReserved;       // 0
    store << ice.wXHotspot;       // X hotspot if cursor, #planes if icon
    store << ice.wYHotspot;       // Y hotspot if cursor, #bits/pixel if icon
    store << ice.dwBytesInRes;    // Total number of bytes in images (including palette data)
    store << ice.dwImageOffset;   // Location of image from the beginning of file

    // Bitmap Info Header
    store << bmi.biSize;
    store << bmi.biWidth;
    store << bmi.biHeight;
    store << bmi.biPlanes;
    store << bmi.biBitCount;
    store << bmi.biCompression;
    store << bmi.biSizeImage;
    store << bmi.biXPelsPerMeter;
    store << bmi.biYPelsPerMeter;
    store << bmi.biClrUsed;
    store << bmi.biClrImportant;

    // Save pixels
    result=fxsaveICOBits(store,data,width,height,bpp);

    // Restore byte order
    store.swapBytes(swap);
    }
  return result;
  }

}
