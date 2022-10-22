/********************************************************************************
*                                                                               *
*                          X B M   I n p u t / O u t p u t                      *
*                                                                               *
*********************************************************************************
* Copyright (C) 2003,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "fxascii.h"
#include "FXArray.h"
#include "FXHash.h"
#include "FXElement.h"
#include "FXStream.h"



/*
  Notes:
  - Support for black and white images.
  - When writing image, dither output to bilevel.
  - Hot spot allows using these routines for cursor images.
  - Should we load (or save) image name?
*/

using namespace FX;


/*******************************************************************************/

namespace FX {


// Declarations
#ifndef FXLOADXBM
extern FXAPI FXbool fxcheckXBM(FXStream& store);
extern FXAPI FXbool fxloadXBM(FXColor*& data,const FXuchar *pix,const FXuchar *msk,FXint width,FXint height);
extern FXAPI FXbool fxloadXBM(FXStream& store,FXColor*& data,FXint& width,FXint& height,FXint& hotx,FXint& hoty);
extern FXAPI FXbool fxsaveXBM(FXStream& store,const FXColor *data,FXint width,FXint height,FXint hotx=-1,FXint hoty=-1);
#endif

// Furnish our own version
extern FXAPI FXint __sscanf(const FXchar* string,const FXchar* format,...);
extern FXAPI FXint __snprintf(FXchar* string,FXint length,const FXchar* format,...);


// Little helper
static void readline(FXStream& store,FXchar* buffer,FXuint size){
  FXuint i=0;
  while(!store.eof() && i<size){
    store >> buffer[i];
    if(buffer[i]=='\r') continue;
    if(buffer[i]=='\n') break;
    i++;
    }
  buffer[i]=0;
  }


// Check if stream contains a XBM
FXbool fxcheckXBM(FXStream& store){
  FXuchar signature[4];
  store.load(signature,4);
  store.position(-4,FXFromCurrent);
  return signature[0]=='#' && signature[1]=='d' && signature[2]=='e' && signature[3]=='f';
  }


// Load alpha XBM image from pixels and mask
FXbool fxloadXBM(FXColor*& data,const FXuchar *pixels,const FXuchar *mask,FXint width,FXint height){
  FXint x,y,byt,bit,row;
  data=nullptr;
  if(pixels && mask && 0<width && 0<height){
    if(callocElms(data,width*height)){
      row=(width+7)>>3;
      for(y=0; y<height; y++){
        for(x=0; x<width; x++){
          byt=y*row+(x>>3);
          bit=1<<(x&7);
          if(mask[byt]&bit){
            data[y*width+x]|=FXRGBA(0,0,0,255);                                 // Opaque
            if(!(pixels[byt]&bit)) data[y*width+x]|=FXRGBA(255,255,255,0);      // White
            }
          }
        }
      return true;
      }
    }
  return false;
  }


// Load image from stream
FXbool fxloadXBM(FXStream& store,FXColor*& data,FXint& width,FXint& height,FXint& hotx,FXint& hoty){
  const FXColor colormap[2]={FXRGB(255,255,255),FXRGB(0,0,0)};
  FXchar buffer[1024],name[255],ch;
  FXint  value,i,j;
  FXColor *pp;

  // Null out
  data=nullptr;
  width=0;
  height=0;
  hotx=-1;
  hoty=-1;

  // Read header
  while(!store.eof()){

    // Read buffer
    readline(store,buffer,sizeof(buffer));
    while(strstr(buffer,"/*") && !store.eof()){
      readline(store,buffer,sizeof(buffer));
      }

    // Recognize #define
    if(__sscanf(buffer,"#define %100s %d",name,&value)==2){
      if(strstr(name,"width")) width=value;
      else if(strstr(name,"height")) height=value;
      else if(strstr(name,"x_hot")) hotx=value;
      else if(strstr(name,"y_hot")) hoty=value;
      continue;
      }

    // Recognize declaration
    if(__sscanf(buffer,"static unsigned char %100s = {",name)==1) break;
    if(__sscanf(buffer,"static char %100s = {", name)==1) break;
    }

  // Test sensible width, height
  if(width<=0 || height<=0) return false;

  // Allocate image to return
  if(!allocElms(data,width*height)){
    return false;
    }

  // Load image; note we will read a whole number of bytes
  // for each line, the last byte may not be completely used
  for(j=0,pp=data; j<height; j++){
    for(i=0; i<width; i++){
      if((i&7)==0){
        value=0;
        while(!store.eof()){
          store >> ch;
          if(ch!='0') continue;
          store >> ch;
          if(ch!='x' && ch!='X') continue;
          break;
          }
        while(!store.eof()){
          store >> ch;
          if(!Ascii::isHexDigit(ch)) break;
          value=value*16+Ascii::digitValue(ch);
          }
        }
      *pp++=colormap[value&1];
      value>>=1;
      }
    }

  // We got the image, but we're not done yet; need to read few more bytes
  // the number of bytes read here must match the number of bytes written
  // by fxsaveXBM() so that the stream won't get out of sync
  while(!store.eof()){
    store >> ch;
    if(ch=='\n') break;
    }

  return true;
  }


/*******************************************************************************/



// Save image to a stream
FXbool fxsaveXBM(FXStream& store,const FXColor *data,FXint width,FXint height,FXint hotx,FXint hoty){
  static const FXint dither[4][4]={{0,32768, 8192,40960},{49152,16384,57344,24576},{12288,45056,4096,36864},{61440,28672,53248,20480}};
  const FXuchar *ptr=(const FXuchar*)data;
  FXint bit,code,count,x,y,n;
  const char name[]="image";
  FXchar buffer[128];

  // Write width
  n=__snprintf(buffer,sizeof(buffer),"#define %s_width %d\n",name,width);
  store.save(buffer,n);

  // Write height
  n=__snprintf(buffer,sizeof(buffer),"#define %s_height %d\n",name,height);
  store.save(buffer,n);

  // Write hot spot
  if(0<=hotx && 0<=hoty){
    n=__snprintf(buffer,sizeof(buffer),"#define %s_x_hot %d\n",name,hotx);
    store.save(buffer,n);
    n=__snprintf(buffer,sizeof(buffer),"#define %s_y_hot %d\n",name,hoty);
    store.save(buffer,n);
    }

  // Write declaration
  n=__snprintf(buffer,sizeof(buffer),"static char %s_bits[] = {",name);
  store.save(buffer,n);

  // Write pixels
  for(y=count=0; y<height; y++){
    for(x=code=0,bit=1; x<width; x++){
      if((ptr[0]*29+ptr[1]*151+ptr[2]*77)<dither[y&3][x&3]) code|=bit;
      bit<<=1;
      if(bit==256 || x==width-1){
        if(count){
          store.save(",",1);
          }
        if(count%12==0){
          store.save("\n   ",3);
          }
        n=__snprintf(buffer,sizeof(buffer)," 0x%02x",code);
        store.save(buffer,n);
        bit=1;
        code=0;
        count++;
        }
      ptr+=4;
      }
    }

  // Wrap it up
  store.save("};\n",3);

  return true;
  }

}

