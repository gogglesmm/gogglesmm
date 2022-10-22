/********************************************************************************
*                                                                               *
*              B a s e - 8 5    E n c o d i n g  /  D e c o d i n g             *
*                                                                               *
*********************************************************************************
* Copyright (C) 2018,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
  - Encode/decode binary data to or from ascii85.
  - Could do with some error checking; encode can't fail, but decode can,
    if input character is not in expected set, what do we do? [suggest we
    skip it, maybe].
  - Which base85 is most useful? Adobe, rfc1924, Z85,
  - 85^5 = 4384852500, which is just larger than 256^4=4294967295; thus, 5
    bytes needed to encode each 4 bytes.
  - Encode 4 bytes of source into 5 bytes of destination, using only 85 printable
    ASCII symbols.

*/

using namespace FX;

/*******************************************************************************/

namespace FX {


// Ascii85 encoding table
static const FXchar encode[86]="0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz!#$%&()*+-;<=>?@^_`{|}~";


// Ascii85 decoding table
static const FXuchar decode[256]={
  128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
  128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
  128,  62, 128,  63,  64,  65,  66, 128,  67,  68,  69,  70, 128,  71, 128, 128,
    0,   1,   2,   3,   4,   5,   6,   7,   8,   9, 128,  72,  73,  74,  75,  76,
   77,  10,  11,  12,  13,  14,  15,  16,  17,  18,  19,  20,  21,  22,  23,  24,
   25,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35, 128, 128, 128,  78,  79,
   80,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,  48,  49,  50,
   51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  81,  82,  83,  84, 128,
  128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
  128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
  128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
  128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
  128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
  128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
  128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
  128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
  };


// Encode src to dst in base85, each 4 bytes is encoded as 5 ascii bytes
FXchar* fxencode85(FXchar* dst,FXchar* dstend,const FXchar* src,const FXchar* srcend){
  FXchar* ptr=dst;
  FXuchar c0,c1,c2,c3,c4;
  FXuint value;
  while(ptr+4<dstend && src+3<srcend){  // Encode whole groups
    c0=src[0];
    c1=src[1];
    c2=src[2];
    c3=src[3];
    value=(c0<<24)|(c1<<16)|(c2<<8)|c3;
    c0=value/52200625;                  // No modulo needed
    c1=(value/614125)%85;
    c2=(value/7225)%85;
    c3=(value/85)%85;
    c4=value%85;
    ptr[0]=encode[c0];
    ptr[1]=encode[c1];
    ptr[2]=encode[c2];
    ptr[3]=encode[c3];
    ptr[4]=encode[c4];
    src+=4;
    ptr+=5;
    }
  if(ptr+4<dstend && src<srcend){       // Encode partial group
    c0=src[0];
    c1=0;
    c2=0;
    if(src+1<srcend){
      c1=src[1];
      if(src+2<srcend){
        c2=src[2];
        }
      }
    value=(c0<<24)|(c1<<16)|(c2<<8);
    c0=value/52200625;                  // No modulo needed
    c1=(value/614125)%85;
    c2=(value/7225)%85;
    c3=(value/85)%85;
    c4=value%85;
    ptr[0]=encode[c0];
    ptr[1]=encode[c1];
    ptr[2]=encode[c2];
    ptr[3]=encode[c3];
    ptr[4]=encode[c4];
    ptr+=5;
    }
  if(ptr<dstend){                       // Terminate string
    ptr[0]='\0';
    }
  return ptr;
  }


// Decode src to dst from base85; assume input is a multiple of 5,
// and output a whole multiple of 4.
FXchar* fxdecode85(FXchar* dst,FXchar* dstend,const FXchar* src,const FXchar* srcend){
  const FXchar* end=src;
  FXchar* ptr=dst;
  FXuchar c0,c1,c2,c3,c4;
  FXuint value;
  while(end<srcend && decode[(FXuchar)end[0]]!=128){
    ++end;
    }
  while(ptr+3<dstend && src+4<end){     // Decode whole group of 4
    c0=src[0];
    c1=src[1];
    c2=src[2];
    c3=src[3];
    c4=src[4];
    c0=decode[c0];
    c1=decode[c1];
    c2=decode[c2];
    c3=decode[c3];
    c4=decode[c4];
    value=((c0*85+c1)*85+c2)*85+c3;
    if(value>=50529027 && (value+c4)>50529027) break;   // Overflow check
    value=value*85+c4;
    c0=value>>24;
    c1=value>>16;
    c2=value>>8;
    c3=value;
    ptr[0]=c0;
    ptr[1]=c1;
    ptr[2]=c2;
    ptr[3]=c3;
    src+=5;
    ptr+=4;
    }
  if(ptr<dstend){                       // Terminate string
    ptr[0]='\0';
    }
  return ptr;
  }

}

