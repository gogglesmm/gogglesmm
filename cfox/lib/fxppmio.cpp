/********************************************************************************
*                                                                               *
*                          P P M   I n p u t / O u t p u t                      *
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
#include "FXArray.h"
#include "FXHash.h"
#include "FXElement.h"
#include "FXStream.h"



/*
  Notes:
  - Definitely a 'no-frills' format.
  - Certainly not optimized for speed; but it works.
  - No support for values greater than 255.
*/

using namespace FX;

/*******************************************************************************/

namespace FX {


#ifndef FXLOADPPM
extern FXAPI FXbool fxcheckPPM(FXStream& store);
extern FXAPI FXbool fxloadPPM(FXStream& store,FXColor*& data,FXint& width,FXint& height);
extern FXAPI FXbool fxsavePPM(FXStream& store,const FXColor *data,FXint width,FXint height);
#endif

// Furnish our own version
extern FXAPI FXint __snprintf(FXchar* string,FXint length,const FXchar* format,...);


// Read one integer
static FXint getint(FXStream& store){
  FXint num=0;
  FXuchar c;
  while(!store.eof()){
    store >> c;
    if('0'<=c && c<='9') break;
    if(c=='#'){
      while(!store.eof()){
        store >> c;
        if(c=='\n') break;
        }
      }
    }
  while(!store.eof()){
    num=num*10+c-'0';
    store >> c;
    if(c<'0' || c>'9') break;
    }
  return num;
  }


// Check if stream contains a PPM
FXbool fxcheckPPM(FXStream& store){
  FXuchar signature[2];
  store.load(signature,2);
  store.position(-2,FXFromCurrent);
  return signature[0]=='P' && '1'<=signature[1] && signature[1]<='6';
  }


// Load image from stream
FXbool fxloadPPM(FXStream& store,FXColor*& data,FXint& width,FXint& height){
  FXint npixels,i,j,maxvalue=1;
  FXuchar *pp;
  FXuchar magic,format,byte;

  // Null out
  data=nullptr;
  width=0;
  height=0;

  // Check magic byte
  store >> magic;
  if(magic=='P'){

    // Check format
    // "P1" = Ascii bitmap,
    // "P2" = Ascii greymap,
    // "P3" = Ascii pixmap,
    // "P4" = Raw bitmap,
    // "P5" = Raw greymap,
    // "P6" = Raw pixmap
    store >> format;
    if('1'<=format && format<='6'){

      // Get size
      width=getint(store);
      height=getint(store);

      // Sanity check
      if(0<width && 0<height){
        npixels=width*height;

        // Get maximum value
        if(format!='1' && format!='4'){
          maxvalue=getint(store);
          if(maxvalue<=0 || maxvalue>=256) return false;
          }

        FXTRACE((100,"fxloadPPM: width=%d height=%d type=%c \n",width,height,format));

        // Allocate buffer
        if(callocElms(data,npixels)){

          // Read it
          pp=(FXuchar*)data;
          switch(format){
            case '1':   // Ascii bitmap
              for(i=0; i<height; i++){
                for(j=0; j<width; j++,pp+=4){
                  byte=getint(store);
                  pp[2] = byte?255:0;
                  pp[1] = pp[2];
                  pp[0] = pp[2];
                  pp[3] = 255;
                  }
                }
              break;
            case '2':   // Ascii greymap
              for(i=0; i<height; i++){
                for(j=0; j<width; j++,pp+=4){
                  pp[2] = getint(store);
                  pp[1] = pp[2];
                  pp[0] = pp[2];
                  pp[3] = 255;
                  }
                }
              break;
            case '3':   // Ascii pixmap
              for(i=0; i<height; i++){
                for(j=0; j<width; j++,pp+=4){
                  pp[2] = getint(store);
                  pp[1] = getint(store);
                  pp[0] = getint(store);
                  pp[3] = 255;
                  }
                }
              break;
            case '4':   // Binary bitmap
              for(i=0; i<height; i++){
                for(j=0; j<width; j++,byte<<=1,pp+=4){
                  if((j&7)==0){ store >> byte; }
                  pp[2] = (byte&0x80)?255:0;
                  pp[1] = pp[2];
                  pp[0] = pp[2];
                  pp[3] = 255;
                  }
                }
              break;
            case '5':   // Binary greymap
              for(i=0; i<height; i++){
                for(j=0; j<width; j++,pp+=4){
                  store >> pp[2];
                  pp[1] =  pp[2];
                  pp[0] =  pp[2];
                  pp[3] =  255;
                  }
                }
              break;
            case '6':   // Binary pixmap
              for(i=0; i<height; i++){
                for(j=0; j<width; j++,pp+=4){
                  store >> pp[2];
                  store >> pp[1];
                  store >> pp[0];
                  pp[3] =  255;
                  }
                }
              break;
            }
          return true;
          }
        }
      }
    }
  return false;
  }


/*******************************************************************************/


// Save a bmp file to a stream
FXbool fxsavePPM(FXStream& store,const FXColor *data,FXint width,FXint height){
  if(data && 0<width && 0<height){
    const FXuchar *pp=(const FXuchar*)data;
    FXint i,j,nsize;
    FXchar size[32];

    // Save header
    nsize=__snprintf(size,sizeof(size),"P6\n%d %d\n255\n",width,height);
    store.save(size,nsize);

    // 24-bit/pixel
    for(i=0; i<height; i++){
      for(j=0; j<width; j++,pp+=4){
        store << pp[2];
        store << pp[1];
        store << pp[0];
        }
      }
    return true;
    }
  return false;
  }

}
