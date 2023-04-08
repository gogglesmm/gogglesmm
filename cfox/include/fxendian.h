/********************************************************************************
*                                                                               *
*                      B y t e   S w a p p i n g   S u p p o r t                *
*                                                                               *
*********************************************************************************
* Copyright (C) 2010,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#ifndef FXENDIAN_H
#define FXENDIAN_H

namespace FX {


// Bit reverse in a byte
static inline FXuchar reverse8(FXuchar x){
  x=((x<<1)&0xAA) | ((x>>1)&0x55);
  x=((x<<2)&0xCC) | ((x>>2)&0x33);
  return (x<<4) | (x>>4);
  }


// Bit reverse in a unsigned short
static inline FXushort reverse16(FXushort x){
  x=((x<<1)&0xAAAA) | ((x>>1)&0x5555);
  x=((x<<2)&0xCCCC) | ((x>>2)&0x3333);
  x=((x<<4)&0xF0F0) | ((x>>4)&0x0F0F);
  return (x<<8) | (x>>8);
  }


// Bit reverse in an unsigned integer
static inline FXuint reverse32(FXuint x){
  x=((x<<1)&0xAAAAAAAA) | ((x>>1)&0x55555555);
  x=((x<<2)&0xCCCCCCCC) | ((x>>2)&0x33333333);
  x=((x<<4)&0xF0F0F0F0) | ((x>>4)&0x0F0F0F0F);
  x=((x<<8)&0xFF00FF00) | ((x>>8)&0x00FF00FF);
  return (x<<16) | (x>>16);
  }


// Bit reverse in an unsigned long
static inline FXulong reverse64(FXulong x){
  x=((x<< 1)&FXULONG(0xAAAAAAAAAAAAAAAA)) | ((x>> 1)&FXULONG(0x5555555555555555));
  x=((x<< 2)&FXULONG(0xCCCCCCCCCCCCCCCC)) | ((x>> 2)&FXULONG(0x3333333333333333));
  x=((x<< 4)&FXULONG(0xF0F0F0F0F0F0F0F0)) | ((x>> 4)&FXULONG(0x0F0F0F0F0F0F0F0F));
  x=((x<< 8)&FXULONG(0xFF00FF00FF00FF00)) | ((x>> 8)&FXULONG(0x00FF00FF00FF00FF));
  x=((x<<16)&FXULONG(0xFFFF0000FFFF0000)) | ((x>>16)&FXULONG(0x0000FFFF0000FFFF));
  return (x<<32) | (x>>32);
  }


// Byte swap unsigned short
static inline FXushort swap16(FXushort x){
#if ((__GNUC__ >= 5) || ((__GNUC__ == 4) && (__GNUC_MINOR__ >= 8)))
  return __builtin_bswap16(x);
#elif (_MSC_VER >= 1500)
  return _byteswap_ushort(x);
#else
  return (x>>8) | (x<<8);
#endif
  }


// Byte swap unsiged int
static inline FXuint swap32(FXuint x){
#if ((__GNUC__ >= 5) || ((__GNUC__ == 4) && (__GNUC_MINOR__ >= 8)))
  return __builtin_bswap32(x);
#elif (_MSC_VER >= 1500)
  return _byteswap_ulong(x);
#else
  x=((x<<8)&0xFF00FF00)|((x>>8)&0x00FF00FF);
  return (x>>16)|(x<<16);
#endif
  }


// Byte swap unsigned long
static inline FXulong swap64(FXulong x){
#if ((__GNUC__ >= 5) || ((__GNUC__ == 4) && (__GNUC_MINOR__ >= 8)))
  return __builtin_bswap64(x);
#elif (_MSC_VER >= 1500)
  return _byteswap_uint64(x);
#else
  x=((x<< 8)&FXULONG(0xFF00FF00FF00FF00))|((x>> 8)&FXULONG(0x00FF00FF00FF00FF));
  x=((x<<16)&FXULONG(0xFFFF0000FFFF0000))|((x>>16)&FXULONG(0x0000FFFF0000FFFF));
  return (x>>32)|(x<<32);
#endif
  }


// Isolate least significant bit set
static inline FXuint lsb32(FXuint x){
  return FXuint(x&(-FXint(x)));
  }


// Isolate least significant bit set
static inline FXulong lsb64(FXulong x){
  return FXulong(x&(-FXlong(x)));
  }


// Isolate most significant bit set
static inline FXuint msb32(FXuint x){
  x|=(x>>1);
  x|=(x>>2);
  x|=(x>>4);
  x|=(x>>8);
  x|=(x>>16);
  return x-(x>>1);
  }


// Isolate most significant bit set
static inline FXulong msb64(FXulong x){
  x|=(x>>1);
  x|=(x>>2);
  x|=(x>>4);
  x|=(x>>8);
  x|=(x>>16);
  x|=(x>>32);
  return x-(x>>1);
  }


// Count one-bits in integer
static inline FXuint pop32(FXuint x){
#if ((__GNUC__ > 3) || ((__GNUC__ == 3) && (__GNUC_MINOR__ >= 4)))
  return __builtin_popcount(x);
#elif defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
  return __popcnt(x);
#else
  x=x-((x>>1)&0x55555555);
  x=(x&0x33333333)+((x>>2)&0x33333333);
  return (((x+(x>>4))&0x0F0F0F0F)*0x01010101)>>24;
#endif
  }


// Count one-bits in long
static inline FXulong pop64(FXulong x){
#if ((__GNUC__ > 3) || ((__GNUC__ == 3) && (__GNUC_MINOR__ >= 4)))
#if defined(__LP64__) || defined(_LP64) || (__WORDSIZE == 64)
  return __builtin_popcountl(x);
#else
  return __builtin_popcountll(x);
#endif
#elif defined(_MSC_VER) && (defined(_M_X64))
  return __popcnt64(x);
#else
  x=x-((x>>1)&FXULONG(0x5555555555555555));
  x=(x&FXULONG(0x3333333333333333))+((x>>2)&FXULONG(0x3333333333333333));
  return (((x+(x>>4))&FXULONG(0xf0f0f0f0f0f0f0f))*FXULONG(0x101010101010101))>>56;
#endif
  }


// Count leading zeros in non-zero integer
static inline FXuint clz32(FXuint x){
#if ((__GNUC__ > 3) || ((__GNUC__ == 3) && (__GNUC_MINOR__ >= 4)))
  return __builtin_clz(x);
#elif defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
  unsigned long result;
  _BitScanReverse(&result,x);
  return 31-result;
#else
  FXuint f,e,d,c,b;
  f=(((FXint)(x-0x00010000))>>31)&16; x<<=f;
  e=(((FXint)(x-0x01000000))>>31)&8;  x<<=e;
  d=(((FXint)(x-0x10000000))>>31)&4;  x<<=d;
  c=(((FXint)(x-0x40000000))>>31)&2;  x<<=c;
  b=(((FXint)(x-0x80000000))>>31)&1;
  return f+e+d+c+b;
#endif
  }


// Count leading zeros in non-zero long
static inline FXulong clz64(FXulong x){
#if ((__GNUC__ > 3) || ((__GNUC__ == 3) && (__GNUC_MINOR__ >= 4)))
#if defined(__LP64__) || defined(_LP64) || (__WORDSIZE == 64)
  return __builtin_clzl(x);
#else
  return __builtin_clzll(x);
#endif
#elif defined(_MSC_VER) && defined(_M_X64)
  unsigned long result;
  _BitScanReverse64(&result,x);
  return 63-result;
#else
  FXulong g,f,e,d,c,b;
  g=(((FXlong)(x-FXULONG(0x0000000100000000)))>>63)&32; x<<=g;
  f=(((FXlong)(x-FXULONG(0x0001000000000000)))>>63)&16; x<<=f;
  e=(((FXlong)(x-FXULONG(0x0100000000000000)))>>63)&8;  x<<=e;
  d=(((FXlong)(x-FXULONG(0x1000000000000000)))>>63)&4;  x<<=d;
  c=(((FXlong)(x-FXULONG(0x4000000000000000)))>>63)&2;  x<<=c;
  b=(((FXlong)(x-FXULONG(0x8000000000000000)))>>63)&1;
  return g+f+e+d+c+b;
#endif
  }


// Count trailing zeros in non-zero integer
static inline FXuint ctz32(FXuint x){
#if ((__GNUC__ > 3) || ((__GNUC__ == 3) && (__GNUC_MINOR__ >= 4)))
  return __builtin_ctz(x);
#elif defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
  unsigned long result;
  _BitScanForward(&result,x);
  return result;
#else
  return 31-clz32(x&-x);
#endif
  }


// Count trailing zeros in non-zero long
static inline FXulong ctz64(FXulong x){
#if ((__GNUC__ > 3) || ((__GNUC__ == 3) && (__GNUC_MINOR__ >= 4)))
#if defined(__LP64__) || defined(_LP64) || (__WORDSIZE == 64)
  return __builtin_ctzl(x);
#else
  return __builtin_ctzll(x);
#endif
#elif defined(_MSC_VER) && defined(_M_X64)
  unsigned long result;
  _BitScanForward64(&result,x);
  return result;
#else
  return FXULONG(63)-clz64(x&-x);
#endif
  }


// Roll bits left, 32-bit flavor (count<32)
static inline FXuint rol32(FXuint value,FXuint count){
  return (value<<count) | (value>>(32-count));
  }

// Roll bits right, 32-bit flavor (count<32)
static inline FXuint ror32(FXuint value,FXuint count){
  return (value>>count) | (value<<(32-count));
  }


// Roll bits left, 64-bit flavor (count<64)
static inline FXulong rol64(FXulong value,FXulong count){
  return (value<<count) | (value>>(FXULONG(64)-count));
  }

// Roll bits right, 64-bit flavor (count<64)
static inline FXulong ror64(FXulong value,FXulong count){
  return (value>>count) | (value<<(FXULONG(64)-count));
  }


// Shift bits left, 32-bit flavor (count<32)
static inline FXuint shl32(FXuint value,FXuint count){
  return (value<<count);
  }

// Shift bits right, 32-bit flavor (count<32)
static inline FXuint shr32(FXuint value,FXuint count){
  return (value>>count);
  }


// Shift bits left, 64-bit flavor (count<64)
static inline FXulong shl64(FXulong value,FXulong count){
  return (value<<count);
  }

// Shift bits right, 64-bit flavor (count<64)
static inline FXulong shr64(FXulong value,FXulong count){
  return (value>>count);
  }

}

#endif
