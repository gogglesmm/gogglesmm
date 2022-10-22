/********************************************************************************
*                                                                               *
*              B a s e - 6 4    E n c o d i n g  /  D e c o d i n g             *
*                                                                               *
*********************************************************************************
* Copyright (C) 2012,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
  - New implementation should be both faster and safer.
  - Encode 3 bytes of source into 4 bytes to destination, allowing
    binary data to be safely transmitted using only 64 safe printable letters,
    numbers, and other printable ASCII symbols.
*/

using namespace FX;

/*******************************************************************************/

namespace FX {


// Encode table
static const FXchar encode[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";


// Decode table
static const FXuchar decode[256]={
  128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
  128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
  128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,  62, 128, 128, 128,  63,
   52,  53,  54,  55,  56,  57,  58,  59,  60,  61, 128, 128, 128, 128, 128, 128,
  128,   0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,
   15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25, 128, 128, 128, 128, 128,
  128,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,
   41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  51, 128, 128, 128, 128, 128,
  128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
  128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
  128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
  128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
  128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
  128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
  128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
  128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128
  };



// Encode bytes from source src to destination dst using base64.
// The encoding stops when all source bytes have been encoded or destination buffer
// is full.  A pointer ptr past the last written byte is returned.
// Each 3 bytes from source is encoded as 4 bytes to destination.  If source is not
// a multiple of 3 bytes, last few bytes are encoded with '='.
FXchar* fxencode64(FXchar* dst,FXchar* dstend,const FXchar* src,const FXchar* srcend){
  FXchar* ptr=dst;
  FXuchar c0,c1,c2;
  while(ptr+3<dstend && src+2<srcend){  // Encode whole groups
    c0=src[0];
    c1=src[1];
    c2=src[2];
    ptr[0]=encode[(c0>>2)&0x3f];
    ptr[1]=encode[((c0<<4)|(c1>>4))&0x3f];
    ptr[2]=encode[((c1<<2)|(c2>>6))&0x3f];
    ptr[3]=encode[c2&0x3f];
    src+=3;
    ptr+=4;
    }
  if(ptr+3<dstend && src<srcend){       // Encode partial group
    c0=src[0];
    ptr[0]=encode[(c0>>2)&0x3f];
    if(src+1<srcend){
      c1=src[1];
      ptr[1]=encode[((c0<<4)|(c1>>4))&0x3f];
      ptr[2]=encode[(c1<<2)&0x3f];
      }
    else{
      ptr[1]=encode[(c0<<4)&0x3f];
      ptr[2]='=';
      }
    ptr[3]='=';
    ptr+=4;
    }
  if(ptr<dstend){                       // Terminate string
    ptr[0]='\0';
    }
  return ptr;
  }


// Decode bytes from source src to destination dst using base64.
// The decoding stops when all source bytes have been decoded, a illegal byte is
// encountered, the end of the source is deteced, or when the destination buffer is
// full.  A pointer ptr past the last written byte is returned.
// Each 4 bytes from source is decoded to 3 bytes to destination, except at the end
// where fewer bytes may be generated.
FXchar* fxdecode64(FXchar* dst,FXchar* dstend,const FXchar* src,const FXchar* srcend){
  const FXchar* end=src;
  FXchar* ptr=dst;
  FXuchar c0,c1,c2,c3;
  while(end<srcend && decode[(FXuchar)end[0]]!=128){
    ++end;
    }
  while(ptr+2<dstend && src+3<end){     // Decode whole group of 4
    c0=src[0];
    c1=src[1];
    c2=src[2];
    c3=src[3];
    c0=decode[c0];
    c1=decode[c1];
    c2=decode[c2];
    c3=decode[c3];
    FXASSERT(c0<64 && c1<64 && c2<64 && c3<64);
    ptr[0]=(c0<<2)|(c1>>4);
    ptr[1]=(c1<<4)|(c2>>2);
    ptr[2]=(c2<<6)|c3;
    src+=4;
    ptr+=3;
    }
  if(ptr+1<dstend && src+2<end){        // Decode partial group of 3
    c0=src[0];
    c1=src[1];
    c2=src[2];
    c0=decode[c0];
    c1=decode[c1];
    c2=decode[c2];
    FXASSERT(c0<64 && c1<64 && c2<64);
    ptr[0]=(c0<<2)|(c1>>4);
    ptr[1]=(c1<<4)|(c2>>2);
    ptr+=2;
    }
  else if(ptr<dstend && src+1<end){     // Decode partial group of 2
    c0=src[0];
    c1=src[1];
    c0=decode[c0];
    c1=decode[c1];
    FXASSERT(c0<64 && c1<64);
    ptr[0]=(c0<<2)|(c1>>4);
    ptr+=1;
    }
  if(ptr<dstend){                       // Terminate string
    ptr[0]='\0';
    }
  return ptr;
  }

}
