/********************************************************************************
*                                                                               *
*              B a s e - 6 4    E n c o d i n g  /  D e c o d i n g             *
*                                                                               *
*********************************************************************************
* Copyright (C) 2012,2017 by Jeroen van der Zijp.   All Rights Reserved.        *
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

/*
  Notes:
  - Encode/decode binary data to or from base64 ascii.
  - Maybe add some code to limit number of columns to N.
*/

using namespace FX;

/*******************************************************************************/

namespace FX {


// Encode table
static const FXchar encode[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";


// Encode src to dst in base64
FXint fxencode64(FXchar* dst,const FXchar* src,FXint len){
  register FXchar* ptr=dst;
  while(3<=len){
    *ptr++=encode[(src[0]>>2)&0x3f];
    *ptr++=encode[((src[0]&0x03)<<4)|((src[1]>>4)&0x0f)];
    *ptr++=encode[((src[1]&0x0f)<<2)|((src[2]>>6)&0x03)];
    *ptr++=encode[src[2]&0x3f];
    src+=3;
    len-=3;
    }
  if(0<len){
    *ptr++=encode[(src[0]>>2)&0x3f];
    if(1<len){
      *ptr++=encode[((src[0]&0x03)<<4)|((src[1]>>4)&0x0f)];
      *ptr++=encode[((src[1]&0x0f)<<2)];
      }
    else{
      *ptr++=encode[(src[0]&0x03)<<4];
      *ptr++='=';
      }
    *ptr++='=';
    }
  return ptr-dst;
  }


// Decode table
static const FXschar decode[256]={
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,
  52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,
  -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
  15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
  -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
  41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
  };


// Decode src to dst from base64
FXint fxdecode64(FXchar* dst,const FXchar* src,FXint len){
  register FXuchar c0,c1,c2,c3;
  register FXchar* ptr=dst;
  while(4<=len){
    if(decode[c0=(FXuchar)src[0]]<0) break;
    if(decode[c1=(FXuchar)src[1]]<0) break;
    if(decode[c2=(FXuchar)src[2]]<0) break;
    if(decode[c3=(FXuchar)src[3]]<0) break;
    *ptr++=((decode[c0]<<2)&0xfc)|((decode[c1]>>4)&0x03);
    if(c2!='='){
      *ptr++=((decode[c1]&0x0f)<<4)|((decode[c2]>>2)&0x0f);
      if(c3!='='){
        *ptr++=((decode[c2]&0x03)<<6)|(decode[c3]&0x3f);
        }
      }
    src+=4;
    len-=4;
    }
  return ptr-dst;
  }

}
