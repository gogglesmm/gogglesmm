/********************************************************************************
*                                                                               *
*                      B y t e   S w a p p i n g   S u p p o r t                *
*                                                                               *
*********************************************************************************
* Copyright (C) 2010,2018 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#elif (defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__)))
  __asm__ __volatile__("rorw $8,%0\n\t" : "=r"(x) : "0"(x) : "cc");
  return x;
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
#elif (defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__)))
  __asm__ __volatile__("bswapl %0\n\t" : "=r"(x): "0"(x));
  return x;
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
#elif (defined(__GNUC__) && defined(__i386__))
  union { struct { FXuint l; FXuint h; } s; FXulong x; } n;
  n.x=x;
  __asm__ __volatile__("bswapl %0\n\t"
                       "bswapl %1\n\t"
                       "xchgl %0,%1\n\t" : "=r"(n.s.l), "=r" (n.s.h) : "0"(n.s.l), "1"(n.s.h));
  return n.x;
#elif (defined(__GNUC__) && defined(__x86_64__))
  __asm__ __volatile__("bswapq %0\n\t" : "=r"(x) : "0"(x));
  return x;
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


// Count one-bits in non-zero integer
static inline FXuint pop32(FXuint x){
#if ((__GNUC__ > 3) || ((__GNUC__ == 3) && (__GNUC_MINOR__ >= 4)))
  return __builtin_popcount(x);
#else
  x=x-((x>>1)&0x55555555);
  x=(x&0x33333333)+((x>>2)&0x33333333);
  x=(x+(x>>4))&0x0F0F0F0F;
  x=x+(x>>8);
  x=x+(x>>16);
  return (x&0x3F);
#endif
  }


// Count leading zeros in non-zero integer
static inline FXuint clz32(FXuint x){
#if ((__GNUC__ > 3) || ((__GNUC__ == 3) && (__GNUC_MINOR__ >= 4)))
  return __builtin_clz(x);
#else
  FXuint f,e,d,c,b;
  f=!(x&0xffff0000)<<4; x<<=f;
  e=!(x&0xff000000)<<3; x<<=e;
  d=!(x&0xf0000000)<<2; x<<=d;
  c=!(x&0xC0000000)<<1; x<<=c;
  b=!(x&0x80000000);
  return f+e+d+c+b;
#endif
  }


// Count trailing zeros in non-zero integer
static inline FXuint ctz32(FXuint x){
#if ((__GNUC__ > 3) || ((__GNUC__ == 3) && (__GNUC_MINOR__ >= 4)))
  return __builtin_ctz(x);
#else
  FXuint f,e,d,c,b;
  f=!(x&0x0000ffff)<<4; x>>=f;
  e=!(x&0x000000ff)<<3; x>>=e;
  d=!(x&0x0000000f)<<2; x>>=d;
  c=!(x&0x00000003)<<1; x>>=c;
  b=!(x&0x00000001);
  return f+e+d+c+b;
#endif
  }


}

#endif
