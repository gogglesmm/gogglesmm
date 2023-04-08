/********************************************************************************
*                                                                               *
*                           M a t h   F u n c t i o n s                         *
*                                                                               *
*********************************************************************************
* Copyright (C) 2015,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "fxendian.h"
#include "fxmath.h"


/*
  Notes:
  - Remedial math functions for systems that are missing some.
  - Changes to implementation tend to favor 64-bit machines, pass branch
    prediction hints to minimize overhead for checking NaN's and try harder
    at making things branch-free.
  - All these functions assume IEEE754 single or double precision standard.
  - We are not making any effort to set flags for overflows, underflows,
    inexact results, or anything like that.
  - No support for long double (128 bit double).  At least until we have H/W
    support for this.
*/


using namespace FX;

/*******************************************************************************/

namespace FX {


// All bits of single precision floating point number
FXuint Math::fpBits(FXfloat x){
  union{ FXfloat f; FXuint u; } z={x};
  return z.u;
  }


// All bits of double precision floating point number
FXulong Math::fpBits(FXdouble x){
  union{ FXdouble f; FXulong u; } z={x};
  return z.u;
  }


// Sign of single precision float point number (0..1)
FXint Math::fpSign(FXfloat x){
  FXint sign=Math::fpBits(x)>>31;
  return sign;
  }


// Sign of double precision float point number (0..1)
FXlong Math::fpSign(FXdouble x){
  FXlong sign=Math::fpBits(x)>>63;
  return sign;
  }


// Signed exponent of single precision float point number (-126..128)
FXint Math::fpExponent(FXfloat x){
  FXint exponent=(Math::fpBits(x)>>23)&0xff;
  FXint bias=126-(-exponent>>31);
  return exponent-bias;
  }


// Signed exponent of double precision float point number (-1022..1024)
FXlong Math::fpExponent(FXdouble x){
  FXlong exponent=(Math::fpBits(x)>>52)&0x7ff;
  FXlong bias=1022-(-exponent>>63);
  return exponent-bias;
  }


// Mantissa of single precision float point number
FXint Math::fpMantissa(FXfloat x){
  FXint mantissa=Math::fpBits(x)&0x007fffff;
  FXint exponent=Math::fpBits(x)&0x7f800000;
  FXint extrabit=-(-exponent>>31);      // 1 if exponent!=0
  return mantissa|(extrabit<<23);
  }


// Mantissa of double precision float point number
FXlong Math::fpMantissa(FXdouble x){
  FXlong mantissa=Math::fpBits(x)&FXLONG(0x000fffffffffffff);
  FXlong exponent=Math::fpBits(x)&FXLONG(0x7ff0000000000000);
  FXlong extrabit=-(-exponent>>63);     // 1 if exponent!=0
  return mantissa|(extrabit<<52);
  }


// Single precision floating point number is finite
FXbool Math::fpFinite(FXfloat x){
  return ((Math::fpBits(x)&0x7fffffff)<0x7f800000);
  }


// Double precision floating point number is finite
FXbool Math::fpFinite(FXdouble x){
  return ((Math::fpBits(x)&FXULONG(0x7fffffffffffffff))<FXULONG(0x7ff0000000000000));
  }


// Single precision floating point number is infinite
FXbool Math::fpInfinite(FXfloat x){
  return ((Math::fpBits(x)&0x7fffffff)==0x7f800000);
  }


// Double precision floating point number is infinite
FXbool Math::fpInfinite(FXdouble x){
  return ((Math::fpBits(x)&FXULONG(0x7fffffffffffffff))==FXULONG(0x7ff0000000000000));
  }


// Single precision floating point number is NaN
FXbool Math::fpNan(FXfloat x){
  return (0x7f800000<(Math::fpBits(x)&0x7fffffff));
  }


// Double precision floating point number is NaN
FXbool Math::fpNan(FXdouble x){
  return (FXULONG(0x7ff0000000000000)<(Math::fpBits(x)&FXULONG(0x7fffffffffffffff)));
  }


// Single precision floating point number is normalized
FXbool Math::fpNormal(FXfloat x){
  FXuint bits=Math::fpBits(x)&0x7fffffff;
  return bits==0 || (0x00800000<=bits && bits<0x7f800000);
  }


// Double precision floating point number is normalized
FXbool Math::fpNormal(FXdouble x){
  FXulong bits=Math::fpBits(x)&FXULONG(0x7fffffffffffffff);
  return (bits==0) || ((FXULONG(0x0010000000000000)<=bits) && (bits<FXULONG(0x7ff0000000000000)));
  }


// Single precision ceiling (round upward to nearest integer)
#if defined(NO_CEILF)
FXfloat Math::ceil(FXfloat x){
  union{ FXfloat f; FXuint u; FXint i; } z={x};
  FXint ex=((z.u>>23)&255)-127;
  if(__likely(ex<23)){                  // |x| < 2^23
    if(__likely(0<=ex)){                // 1 <= |x|
      FXuint mx=0x007fffff>>ex;
      if(__likely(z.u&mx)){
        if(z.i>0) z.u+=mx;              // Round up if positive
        z.u&=~mx;
        return z.f;
        }
      return x;
      }
    if(z.i<0) return -0.0f;             // Round up to -0 if negative
    if(z.u) return 1.0f;                // Round up to 1 if positive and not zero
    return 0.0f;
    }
  return x;
  }
#endif


// Single precision floor (round down to nearest integer)
#if defined(NO_FLOORF)
FXfloat Math::floor(FXfloat x){
  union{ FXfloat f; FXuint u; FXint i; } z={x};
  FXint ex=((z.u>>23)&255)-127;
  if(__likely(ex<23)){                  // |x| < 2^23
    if(__likely(0<=ex)){                // 1 <= |x|
      FXuint mx=0x007fffff>>ex;
      if(__likely(z.u&mx)){
        if(z.i<0) z.u+=mx;              // Round down if negative
        z.u&=~mx;
        }
      return z.f;
      }
    if(z.i>=0) return 0.0f;             // Round down to 0 if positive
    if(z.u&0x7fffffff) return -1.0f;    // Round down to -1 if negative and not zero
    return -0.0f;
    }
  return x;
  }
#endif


// Single precision round to nearest integer (away from zero)
#if defined(NO_ROUNDF)
FXfloat Math::round(FXfloat x){
  if(__likely(Math::fpFinite(x))){
    FXfloat t;
    if(Math::fpSign(x)){
      t=Math::floor(-x);
      t+=(FXfloat)(t+x<=-0.5f);
      return -t;
      }
    else{
      t=Math::floor(x);
      t+=(FXfloat)(t-x<=-0.5f);
      return t;
      }
    }
  return x;
  }
#endif


// Double precision round to nearest integer (away from zero)
#if defined(NO_ROUND)
FXdouble Math::round(FXdouble x){
  if(__likely(Math::fpFinite(x))){
    FXdouble t;
    if(Math::fpSign(x)){
      t=Math::floor(-x);
      t+=(FXdouble)(t+x<=-0.5);
      return -t;
      }
    else{
      t=Math::floor(x);
      t+=(FXdouble)(t-x<=-0.5);
      return t;
      }
    }
  return x;
  }
#endif


// Single precision truncate to nearest integer (toward zero)
// Limitation: not setting inexact flag.
// Assumptions: fast path is |x| < 2^23, i.e. there is actually a fraction
#if defined(NO_TRUNCF)
FXfloat Math::trunc(FXfloat x){
  union{ FXfloat f; FXuint u; } z={x};
  FXint ex=((z.u>>23)&0xff)-127;        // Unbiased exponent
  if(ex<23){                            // Any |x| >= 2^23 has no fraction anyway
    if(ex>=0){                          // Positive exponents may have a fraction
      FXint mm=(0x007fffff)>>ex;
      z.u&=~mm;
      return z.f;
      }
    z.u&=0x80000000;                    // Pure fraction (just keep sign)
    }
  return z.f;
  }
#endif


// Double precision truncate to nearest integer (toward zero)
// Limitation: not setting inexact flag.
// Assumptions: fast path is |x| < 2^52, i.e. there is actually a fraction
#if defined(NO_TRUNC)
FXdouble Math::trunc(FXdouble x){
  union{ FXdouble f; FXulong u; } z={x};
  FXlong ex=((z.u>>52)&0x7ff)-1023;     // Unbiased exponent
  if(ex<52){                            // Any |x| >= 2^52 has no fraction anyway
    if(ex>=0){                          // Positive exponents may have a fraction
      FXlong mm=FXLONG(0x000fffffffffffff)>>ex;
      z.u&=~mm;
      return z.f;
      }
    z.u&=FXLONG(0x8000000000000000);    // Pure fraction (just keep sign)
    }
  return z.f;
  }
#endif


// Single precision round to nearest integer
#if defined(NO_NEARBYINTF)
FXfloat Math::nearbyint(FXfloat x){
  static const FXfloat TWO23[2]={8388608.0f,-8388608.0f};
  union{ FXfloat f; FXuint u; } z={x};
  FXuint ax=z.u&0x7fffffff;
  if(ax<0x4b000000){                    // |x| has fraction
    volatile FXfloat tmp;               // Volatile lest add/subtract is optimized away
    FXuint sx=z.u>>31;
    tmp=x+TWO23[sx];                    // Blow fraction bits, works for values up to 8388608
    x=tmp-TWO23[sx];                    // Values greater than 8388608 are integral anyway
    }
  return x;
  }
#endif


// Double precision round to nearest integer
#if defined(NO_NEARBYINT)
FXdouble Math::nearbyint(FXdouble x){
  static const FXdouble TWO52[2]={4503599627370496.0,-4503599627370496.0};
  union{ FXdouble f; FXulong u; } z={x};
  FXulong ax=z.u&FXULONG(0x7fffffffffffffff);
  if(ax<FXULONG(0x4330000000000000)){   // |x| has fraction
    volatile FXdouble tmp;              // Volatile lest add/subtract is optimized away
    FXulong sx=z.u>>63;
    tmp=x+TWO52[sx];                    // Blow fraction bits, works for values up to 4503599627370496
    x=tmp-TWO52[sx];                    // Values greater than 4503599627370496 are integral anyway
    }
  return x;
  }
#endif


// Single precision round to nearest integer
#if defined(NO_RINTF)
FXfloat Math::rint(FXfloat x){
  return Math::nearbyint(x);
  }
#endif


// Double precision round to nearest integer
#if defined(NO_RINT)
FXdouble Math::rint(FXdouble x){
  return Math::nearbyint(x);
  }
#endif

/*******************************************************************************/

// Single precision hyperbolic sine
#if defined(NO_SINHF)
FXfloat Math::sinh(FXfloat x){
  union{ FXfloat f; FXuint u; FXint i; } z={x};
  FXuint ax=z.u&0x7fffffff;
  if(__likely(ax<0x7f800000)){                          // |x| < Inf
    FXfloat h=(z.i<0)?-0.5f:0.5f;
    if(ax<0x41b00000){                                  // |x| < 22
      if(ax<0x31800000){                                // |x| < 2^-28
        return x;
        }
      FXfloat t=Math::expm1(Math::fabs(x));
      if(ax<0x3f800000){                                // |x| < 1
        return h*(2.0f*t-t*t/(t+1.0f));
        }
      return h*(t+t/(t+1.0f));
      }
    if(ax<0x42b17180){                                  // |x| < log(FLT_MAX)
      return h*Math::exp(Math::fabs(x));
      }
    if(ax<=0x42b2d4fc){                                 // |x| < overflowthreshold
      FXfloat w=Math::exp(0.5f*Math::fabs(x));
      volatile FXfloat t=h*w;                           // FORCE 0.5*w evaluated first
      return t*w;                                       // Otherwise compiler might rearrange
      }
    z.u|=0x7f800000;                                    // Return infinity, with correct sign
    z.u&=0xff800000;
    }
  return z.f;
  }
#endif


// sinh(x) ::= (exp(x) - exp(-x))/2
//
// Cases:
//
// |x| < 2^-28         : x
//
// |x| < 1             : sign(x) * 0.5 * (2 * t - t * t / (t + 1)), where t = expm1(|x|)
//
// 1 < |x| <= 22       : sign(x) * 0.5 * (t + t/(t + 1)), where t = expm1(|x|)
//
// |x| < log(DBL_MAX)  : sign(x) * 0.5 * exp(|x|)
//
// log(DBL_MAX) <= |x| : sign(x) * 0.5 * exp(|x|/2) * exp(|x|/2)
//
// Otherwise           : Inf or NaN
//
#if defined(NO_SINH)
FXdouble Math::sinh(FXdouble x){
  union{ FXdouble f; FXulong u; FXlong i; } z={x};
  FXulong ax=z.u&FXULONG(0x7fffffffffffffff);
  if(__likely(ax<FXULONG(0x7ff0000000000000))){         // |x| < Inf
    FXdouble h=(z.i<0)?-0.5:0.5;
    if(ax<FXULONG(0x4036000000000000)){                 // |x| < 22
      if(ax<FXULONG(0x3e30000000000000)){               // |x| < 2^-28
        return x;
        }
      FXdouble t=Math::expm1(Math::fabs(x));
      if(ax<FXULONG(0x3ff0000000000000)){               // |x| < 1
        return h*(2.0*t-t*t/(t+1.0));
        }
      return h*(t+t/(t+1.0));                           // |x| < 22
      }
//  if(ax<FXULONG(0x40862e42fefa39ef)){
    if(ax<FXULONG(0x40862E4200000000)){
      return h*Math::exp(Math::fabs(x));                // |x| < log(DBL_MAX)
      }
    if(ax<=FXULONG(0x408633ce8fb9f87d)){                // |x| < overflowthreshold
      FXdouble w=Math::exp(0.5*Math::fabs(x));
      volatile FXdouble t=h*w;                          // FORCE 0.5*w evaluated first
      return t*w;                                       // Otherwise compiler might rearrange
      }
    z.u|=FXULONG(0x7ff0000000000000);                   // Return infinity, with correct sign
    z.u&=FXULONG(0xfff0000000000000);
    }
  return z.f;
  }
#endif


// Single precision hyperbolic cosine
#if defined(NO_COSHF)
FXfloat Math::cosh(FXfloat x){
  union{ FXfloat f; FXuint u; FXint i; } z={x};
  FXuint ax=z.u&0x7fffffff;
  if(__likely(ax<=0x7f800000)){                         // |x| <= Inf
    if(ax<0x41b00000){                                  // |x| < 22
      if(ax<0x24000000){                                // |x| < 2.7755575615628913511e-17
        return 1.0f;
        }
      if(ax<0x3eb17218){                                // |x| < 0.5*log(2)
        FXfloat t=Math::expm1(Math::fabs(x));
        return 1.0f+(t*t)/(2.0f+t+t);
        }
      FXfloat t=Math::exp(Math::fabs(x));               // |x| < 22
      return 0.5f*t+0.5f/t;
      }
    if(ax<0x42b17180){                                  // |x| < log(FLT_MAX)
      return 0.5f*Math::exp(Math::fabs(x));
      }
    if(ax<=0x42b2d4fc){                                 // |x| < overflowthreshold
      FXfloat w=Math::exp(0.5f*Math::fabs(x));
      volatile FXfloat t=0.5f*w;                        // FORCE 0.5*w evaluated first
      return t*w;                                       // Otherwise compiler might rearrange
      }
    z.u=0x7f800000;                                     // Return infinity
    }
  return z.f;
  }
#endif


// cosh(x) ::= (exp(x) + exp(-x))/2
//
// Cases:
//
// |x| < 2^-55         : 1
//
// |x| < (1/2) ln(2)   : 1 + t * t / (2 * (1 + t)), where t = expm1(|x|)
//
// |x| < 22            : 0.5 * t + 0.5/t
//
// |x| < log(DBL_MAX)  : 0.5 * exp(|x|)
//
// log(DBL_MAX) <= |x| : 0.5 * exp(|x|/2) * exp(|x|/2)
//
// Otherwise           : Inf or NaN
//
#if defined(NO_COSH)
FXdouble Math::cosh(FXdouble x){
  union{ FXdouble f; FXulong u; FXlong i; } z={x};
  FXulong ax=z.u&FXULONG(0x7fffffffffffffff);
  if(__likely(ax<=FXULONG(0x7ff0000000000000))){        // |x| <= Inf
    if(ax<FXULONG(0x4036000000000000)){                 // |x| < 22
      if(ax<FXULONG(0x3c80000000000000)){               // |x| < 2.7755575615628913511e-17
        return 1.0;
        }
      if(ax<FXULONG(0x3fd62e4200000000)){               // |x| < 0.5*log(2)
        FXdouble t=Math::expm1(Math::fabs(x));
        return 1.0+(t*t)/(2.0+t+t);
        }
      FXdouble t=Math::exp(Math::fabs(x));              // |x| < 22
      return 0.5*t+0.5/t;
      }
    if(ax<FXULONG(0x40862e4200000000)){                 // |x| < log(DBL_MAX)
      return 0.5*Math::exp(Math::fabs(x));
      }
    if(ax<=FXULONG(0x408633ce8fb9f87d)){                // |x| < overflowthreshold
      FXdouble w=Math::exp(0.5*Math::fabs(x));
      volatile FXdouble t=0.5*w;                        // FORCE 0.5*w evaluated first
      return t*w;                                       // Otherwise compiler might rearrange
      }
    z.u=FXULONG(0x7ff0000000000000);                    // Return infinity
    }
  return z.f;
  }
#endif


// Single precision hyperbolic tangent
#if defined(NO_TANHF)
FXfloat Math::tanh(FXfloat x){
  union{ FXfloat f; FXuint u; FXint i; } z={x};
  FXuint ax=z.u&0x7fffffff;
  if(__likely(ax<=0x7f800000)){                         // |x| <= Inf
    if(ax<0x41b00000){                                  // |x| < 22
      if(ax<0x24000000){                                // |x| < 2^-55
        return x*(1.0f+x);
        }
      if(ax<0x3f800000){                                // |x| < 1
        FXfloat t=Math::expm1(-2.0f*Math::fabs(x));
        x=-t/(t+2.0f);
        }
      else{                                             // |x| >= 1
        FXfloat t=Math::expm1(2.0f*Math::fabs(x));
        x=1.0f-2.0f/(t+2.0f);
        }
      }
    else{                                               // |x| > 22
      x=1.0f;
      }
    return (z.i>=0) ? x : -x;
    }
  return z.f;
  }
#endif


// Double precision hyperbolic tangent
#if defined(NO_TANH)
FXdouble Math::tanh(FXdouble x){
  union{ FXdouble f; FXulong u; FXlong i; } z={x};
  FXulong ax=z.u&FXULONG(0x7fffffffffffffff);
  if(__likely(ax<=FXULONG(0x7ff0000000000000))){        // |x| <= Inf
    if(ax<FXULONG(0x4036000000000000)){                 // |x| < 22
      if(ax<FXULONG(0x3c80000000000000)){               // |x| < 2^-55
        return x*(1.0+x);
        }
      if(ax<FXULONG(0x3ff0000000000000)){               // |x| < 1
        FXdouble t=Math::expm1(-2.0*Math::fabs(x));
        x=-t/(t+2.0);
        }
      else{                                             // |x| >= 1
        FXdouble t=Math::expm1(2.0*Math::fabs(x));
        x=1.0-2.0/(t+2.0);
        }
      }
    else{                                               // |x| > 22
      x=1.0;
      }
    return (z.i>=0) ? x : -x;
    }
  return z.f;
  }
#endif

/*******************************************************************************/

// asinh(x) ::= sign(x) * log [ |x| + sqrt(x*x+1) ]
//
// Cases:
// |x| < eps      : x
//
// |x| < 2        : sign(x) * log1p(|x| + x^2 / (1 + sqrt(1 + x^2)))
//
// |x| < big      : sign(x) * log(2 * |x| + 1/(|x| + sqrt(x^2 + 1)))
//
// big <= |x|     : sign(x) * (log(|x|) + ln2)
//
// |x| = Inf, NaN : x
//
#if defined(NO_ASINHF)
FXfloat Math::asinh(FXfloat x){
  union{ FXfloat f; FXuint u; } z={x};
  FXuint ax=z.u&0x7fffffff;
  FXfloat t=Math::fabs(x);
  FXfloat w;
  if(__unlikely(ax>0x47000000)){                        // big <= |x|
    if(__unlikely(ax>=0x7f800000)){ return x; }         // Inf <= |x|
    w=Math::log(t)+0.693147180559945309417232121458176568f;
    }
  else if(__likely(ax<=0x40000000)){                     // |x| < 2
    if(__unlikely(ax<0x38000000)){ return x; }          // |x| < eps
    w=Math::log1p(t+t*t/(1.0f+Math::sqrt(1.0f+t*t)));
    }
  else{                                                 // 2 <= |x| < big
    w=Math::log(2.0f*t+1.0f/(Math::sqrt(t*t+1.0f)+t));
    }
  return Math::copysign(w,x);
  }
#endif


#if defined(NO_ASINH)
FXdouble Math::asinh(FXdouble x){
  union{ FXdouble f; FXulong u; } z={x};
  FXulong ax=z.u&FXULONG(0x7fffffffffffffff);
  FXdouble t=Math::fabs(x);
  FXdouble w;
  if(__unlikely(ax>FXULONG(0x41b0000000000000))){       // big <= |x|
    if(__unlikely(ax>=FXULONG(0x7ff0000000000000))){ return x; }        // Inf <= |x|
    w=Math::log(t)+0.693147180559945309417232121458176568;
    }
  else if(__likely(ax<FXULONG(0x4000000000000000))){    // |x| < 2
    if(__unlikely(ax<FXULONG(0x3e30000000000000))){ return x; }         // |x| < eps
    w=Math::log1p(t+t*t/(1.0+Math::sqrt(1.0+t*t)));
    }
  else{                                                 // 2 <= |x| < big
    w=Math::log(2.0*t+1.0/(Math::sqrt(t*t+1.0)+t));
    }
  return Math::copysign(w,x);
  }
#endif


// acosh(x) ::= log [ x + sqrt(x*x-1) ]
//
// Cases:
// x < 1        : NaN
//
// x == 1       : 0
//
// x < 2        : log1p((x - 1) + sqrt((x - 1) * (x - 1) + 2 * (x - 1)))
//
// x < big      : log(2 * x - 1/(sqrt(x * x - 1) + x))
//
// x < Inf      : log(x) + ln2
//
#if defined(NO_ACOSHF)
FXfloat Math::acosh(FXfloat x){
  union{ FXfloat f; FXuint u; } z={x};
  FXfloat t;
  if(__unlikely(z.u<=0x3f800000)){                      // x <= 1
    if(__likely(z.u==0x3f800000)){ return 0.0f; }       // x == 1
    z.u=0x7fffffff;
    return z.f;                                         // NaN
    }
  if(__unlikely(z.u>=0x4d800000)){                      // x >= big
    if(__likely(z.u<0x7f800000)){ return Math::log(x)+0.693147180559945309417232121458176568f; }
    z.u=0x7fffffff;
    return z.f;                                         // NaN
    }
  if(__unlikely(z.u>0x40000000)){                       // x > 2
    t=x*x;
    return Math::log(x+x-1.0f/(x+Math::sqrt(t-1.0f)));
    }
  t=x-1.0f;
  return Math::log1p(t+Math::sqrt(t+t+t*t));
  }
#endif


#if defined(NO_ACOSH)
FXdouble Math::acosh(FXdouble x){
  union{ FXdouble f; FXulong u; } z={x};
  FXdouble t;
  if(__unlikely(z.u<=FXULONG(0x3ff0000000000000))){     // x <= 1
    if(__likely(z.u==FXULONG(0x3ff0000000000000))){ return 0.0; }       // x == 1
    z.u=FXULONG(0x7fffffffffffffff);
    return z.f;                                         // NaN
    }
  if(__unlikely(z.u>=FXULONG(0x41b0000000000000))){     // x >= big
    if(__likely(z.u<FXULONG(0x7ff0000000000000))){ return Math::log(x)+0.693147180559945309417232121458176568; }
    z.u=FXULONG(0x7fffffffffffffff);
    return z.f;                                         // NaN
    }
  if(__unlikely(z.u>FXULONG(0x4000000000000000))){      // x > 2
    t=x*x;
    return Math::log(x+x-1.0/(x+Math::sqrt(t-1.0)));
    }
  t=x-1.0;
  return Math::log1p(t+Math::sqrt(t+t+t*t));
  }
#endif


// atanh(x) ::= 0.5 * log [(1 + x) / (1 - x)]
//
// Cases:
// |x| < eps    : x     (Small x)
//
// |x| < 0.5    : 0.5 * log1p(2 * x + 2 * x * x / (1 - x))
//
// 0.5 <= |x|   : 0.5 * log1p(2 * (x / (1 - x)))
//
// 1 == |x|     : +/- Inf
//
// 1 < |x|      : NaN
//
#if defined(NO_ATANHF)
FXfloat Math::atanh(FXfloat x){
  union{ FXfloat f; FXuint u; } z={x};
  FXuint sx=z.u&0x80000000;
  z.u&=0x7fffffff;
  if(__likely(z.u<=0x3f800000)){                        // |x| <= 1
    if(__likely(z.u<0x3f800000)){                       // |x| < 1
      if(__likely(0x31800000<=z.u)){                    // eps <= |x|
        if(z.u<0x3f000000){                             // |x| < 0.5
          z.f=0.5f*Math::log1p(2.0f*z.f+2.0f*z.f*z.f/(1.0f-z.f));
          }
        else{                                           // 0.5 <= |x|
	  z.f=0.5f*Math::log1p(2.0f*(z.f/(1.0f-z.f)));
          }
        }
      z.u|=sx;                                          // Add sign back
      return z.f;
      }
    z.u=sx|0x7f800000;                                  // |x| == 1 -> Inf
    return z.f;
    }
  z.u=0x7fffffff;                                       // |x| > 1 -> NaN
  return z.f;
  }
#endif


#if defined(NO_ATANH)
FXdouble Math::atanh(FXdouble x){
  union{ FXdouble f; FXulong u; } z={x};
  FXulong sx=z.u&FXULONG(0x8000000000000000);
  z.u&=FXULONG(0x7fffffffffffffff);
  if(__likely(z.u<=FXULONG(0x3ff0000000000000))){       // |x| <= 1
    if(__likely(z.u<FXULONG(0x3ff0000000000000))){      // |x| < 1
      if(__likely(FXULONG(0x3e30000000000000)<=z.u)){   // eps <= |x|
        if(z.u<FXULONG(0x3fe0000000000000)){            // |x| < 0.5
          z.f=0.5*Math::log1p(z.f+z.f+(z.f+z.f)*z.f/(1.0-z.f));
          }
        else{                                           // 0.5 <= |x|
	  z.f=0.5*Math::log1p(2.0*(z.f/(1.0-z.f)));
          }
        }
      z.u|=sx;                                          // Add sign back
      return z.f;
      }
    z.u=sx|FXULONG(0x7ff0000000000000);                 // |x| == 1 -> Inf
    return z.f;
    }
  z.u=FXULONG(0x7fffffffffffffff);                      // |x| > 1 -> NaN
  return z.f;
  }
#endif

/*******************************************************************************/

/// Single precision error function
FXfloat Math::erf(FXfloat x){
  return ::erff(x);
  }


/// Double precision error function
FXdouble Math::erf(FXdouble x){
  return ::erf(x);
  }


/// Single precision complementary error function
FXfloat Math::erfc(FXfloat x){
  return ::erfcf(x);
  }


/// Double precision complementary error function
FXdouble Math::erfc(FXdouble x){
  return ::erfc(x);
  }


// Single precision inverse error function
FXfloat Math::inverf(FXfloat x){
  return Math::inverfc(1.0f-x);
  }


// Double precision inverse error function
FXdouble Math::inverf(FXdouble x){
  return Math::inverfc(1.0-x);
  }


// Single precision inverse complementary error function
FXfloat Math::inverfc(FXfloat x){
  FXfloat result,sign,err,t;

  // Argument check
  if(1.0f<=x){
    if(__unlikely(x>=2.0f)){ return -FLT_MAX; }
    sign=-1.0f;
    x=2.0f-x;
    }
  else{
    if(__unlikely(x<=0.0f)){ return FLT_MAX; }
    sign=1.0f;
    }

  // Approximation
  t=Math::sqrt(-Math::log(0.5f*x*x));
  result=-0.70711f*((2.30753f+t*0.27061f)/(1.0f+t*(0.99229f+t*0.04481f))-t);

  // Newton-Raphson refinement steps f(x)/f'(x)
  err=Math::erfc(result)-x;
  result+=err/(1.12837916709551257f*Math::exp(-result*result)-result*err);

  // Restore sign
  result*=sign;
  return result;
  }


// Double precision inverse complementary error function
FXdouble Math::inverfc(FXdouble x){
  FXdouble result,sign,err,t;

  // Argument check
  if(1.0<=x){
    if(__unlikely(x>=2.0)){ return -DBL_MAX; }
    sign=-1.0;
    x=2.0-x;
    }
  else{
    if(__unlikely(x<=0.0)){ return DBL_MAX; }
    sign=1.0;
    }

  // Approximation
  t=Math::sqrt(-Math::log(0.5*x*x));
  result=-0.70711*((2.30753+t*0.27061)/(1.0+t*(0.99229+t*0.04481))-t);

  // Newton-Raphson refinement steps f(x)/f'(x)
  err=Math::erfc(result)-x;
  result+=err/(1.12837916709551257*Math::exp(-result*result)-result*err);
  err=Math::erfc(result)-x;
  result+=err/(1.12837916709551257*Math::exp(-result*result)-result*err);

  // Restore sign
  result*=sign;
  return result;
  }

/*******************************************************************************/

// Power of 10 table
static const FXdouble powOfTen[632]={
                                                   1E-323,1E-322,1E-321,
  1E-320,1E-319,1E-318,1E-317,1E-316,1E-315,1E-314,1E-313,1E-312,1E-311,
  1E-310,1E-309,1E-308,1E-307,1E-306,1E-305,1E-304,1E-303,1E-302,1E-301,
  1E-300,1E-299,1E-298,1E-297,1E-296,1E-295,1E-294,1E-293,1E-292,1E-291,
  1E-290,1E-289,1E-288,1E-287,1E-286,1E-285,1E-284,1E-283,1E-282,1E-281,
  1E-280,1E-279,1E-278,1E-277,1E-276,1E-275,1E-274,1E-273,1E-272,1E-271,
  1E-270,1E-269,1E-268,1E-267,1E-266,1E-265,1E-264,1E-263,1E-262,1E-261,
  1E-260,1E-259,1E-258,1E-257,1E-256,1E-255,1E-254,1E-253,1E-252,1E-251,
  1E-250,1E-249,1E-248,1E-247,1E-246,1E-245,1E-244,1E-243,1E-242,1E-241,
  1E-240,1E-239,1E-238,1E-237,1E-236,1E-235,1E-234,1E-233,1E-232,1E-231,
  1E-230,1E-229,1E-228,1E-227,1E-226,1E-225,1E-224,1E-223,1E-222,1E-221,
  1E-220,1E-219,1E-218,1E-217,1E-216,1E-215,1E-214,1E-213,1E-212,1E-211,
  1E-210,1E-209,1E-208,1E-207,1E-206,1E-205,1E-204,1E-203,1E-202,1E-201,
  1E-200,1E-199,1E-198,1E-197,1E-196,1E-195,1E-194,1E-193,1E-192,1E-191,
  1E-190,1E-189,1E-188,1E-187,1E-186,1E-185,1E-184,1E-183,1E-182,1E-181,
  1E-180,1E-179,1E-178,1E-177,1E-176,1E-175,1E-174,1E-173,1E-172,1E-171,
  1E-170,1E-169,1E-168,1E-167,1E-166,1E-165,1E-164,1E-163,1E-162,1E-161,
  1E-160,1E-159,1E-158,1E-157,1E-156,1E-155,1E-154,1E-153,1E-152,1E-151,
  1E-150,1E-149,1E-148,1E-147,1E-146,1E-145,1E-144,1E-143,1E-142,1E-141,
  1E-140,1E-139,1E-138,1E-137,1E-136,1E-135,1E-134,1E-133,1E-132,1E-131,
  1E-130,1E-129,1E-128,1E-127,1E-126,1E-125,1E-124,1E-123,1E-122,1E-121,
  1E-120,1E-119,1E-118,1E-117,1E-116,1E-115,1E-114,1E-113,1E-112,1E-111,
  1E-110,1E-109,1E-108,1E-107,1E-106,1E-105,1E-104,1E-103,1E-102,1E-101,
  1E-100,1E-099,1E-098,1E-097,1E-096,1E-095,1E-094,1E-093,1E-092,1E-091,
  1E-090,1E-089,1E-088,1E-087,1E-086,1E-085,1E-084,1E-083,1E-082,1E-081,
  1E-080,1E-079,1E-078,1E-077,1E-076,1E-075,1E-074,1E-073,1E-072,1E-071,
  1E-070,1E-069,1E-068,1E-067,1E-066,1E-065,1E-064,1E-063,1E-062,1E-061,
  1E-060,1E-059,1E-058,1E-057,1E-056,1E-055,1E-054,1E-053,1E-052,1E-051,
  1E-050,1E-049,1E-048,1E-047,1E-046,1E-045,1E-044,1E-043,1E-042,1E-041,
  1E-040,1E-039,1E-038,1E-037,1E-036,1E-035,1E-034,1E-033,1E-032,1E-031,
  1E-030,1E-029,1E-028,1E-027,1E-026,1E-025,1E-024,1E-023,1E-022,1E-021,
  1E-020,1E-019,1E-018,1E-017,1E-016,1E-015,1E-014,1E-013,1E-012,1E-011,
  1E-010,1E-009,1E-008,1E-007,1E-006,1E-005,1E-004,1E-003,1E-002,1E-001,
  1E+000,1E+001,1E+002,1E+003,1E+004,1E+005,1E+006,1E+007,1E+008,1E+009,
  1E+010,1E+011,1E+012,1E+013,1E+014,1E+015,1E+016,1E+017,1E+018,1E+019,
  1E+020,1E+021,1E+022,1E+023,1E+024,1E+025,1E+026,1E+027,1E+028,1E+029,
  1E+030,1E+031,1E+032,1E+033,1E+034,1E+035,1E+036,1E+037,1E+038,1E+039,
  1E+040,1E+041,1E+042,1E+043,1E+044,1E+045,1E+046,1E+047,1E+048,1E+049,
  1E+050,1E+051,1E+052,1E+053,1E+054,1E+055,1E+056,1E+057,1E+058,1E+059,
  1E+060,1E+061,1E+062,1E+063,1E+064,1E+065,1E+066,1E+067,1E+068,1E+069,
  1E+070,1E+071,1E+072,1E+073,1E+074,1E+075,1E+076,1E+077,1E+078,1E+079,
  1E+080,1E+081,1E+082,1E+083,1E+084,1E+085,1E+086,1E+087,1E+088,1E+089,
  1E+090,1E+091,1E+092,1E+093,1E+094,1E+095,1E+096,1E+097,1E+098,1E+099,
  1E+100,1E+101,1E+102,1E+103,1E+104,1E+105,1E+106,1E+107,1E+108,1E+109,
  1E+110,1E+111,1E+112,1E+113,1E+114,1E+115,1E+116,1E+117,1E+118,1E+119,
  1E+120,1E+121,1E+122,1E+123,1E+124,1E+125,1E+126,1E+127,1E+128,1E+129,
  1E+130,1E+131,1E+132,1E+133,1E+134,1E+135,1E+136,1E+137,1E+138,1E+139,
  1E+140,1E+141,1E+142,1E+143,1E+144,1E+145,1E+146,1E+147,1E+148,1E+149,
  1E+150,1E+151,1E+152,1E+153,1E+154,1E+155,1E+156,1E+157,1E+158,1E+159,
  1E+160,1E+161,1E+162,1E+163,1E+164,1E+165,1E+166,1E+167,1E+168,1E+169,
  1E+170,1E+171,1E+172,1E+173,1E+174,1E+175,1E+176,1E+177,1E+178,1E+179,
  1E+180,1E+181,1E+182,1E+183,1E+184,1E+185,1E+186,1E+187,1E+188,1E+189,
  1E+190,1E+191,1E+192,1E+193,1E+194,1E+195,1E+196,1E+197,1E+198,1E+199,
  1E+200,1E+201,1E+202,1E+203,1E+204,1E+205,1E+206,1E+207,1E+208,1E+209,
  1E+210,1E+211,1E+212,1E+213,1E+214,1E+215,1E+216,1E+217,1E+218,1E+219,
  1E+220,1E+221,1E+222,1E+223,1E+224,1E+225,1E+226,1E+227,1E+228,1E+229,
  1E+230,1E+231,1E+232,1E+233,1E+234,1E+235,1E+236,1E+237,1E+238,1E+239,
  1E+240,1E+241,1E+242,1E+243,1E+244,1E+245,1E+246,1E+247,1E+248,1E+249,
  1E+250,1E+251,1E+252,1E+253,1E+254,1E+255,1E+256,1E+257,1E+258,1E+259,
  1E+260,1E+261,1E+262,1E+263,1E+264,1E+265,1E+266,1E+267,1E+268,1E+269,
  1E+270,1E+271,1E+272,1E+273,1E+274,1E+275,1E+276,1E+277,1E+278,1E+279,
  1E+280,1E+281,1E+282,1E+283,1E+284,1E+285,1E+286,1E+287,1E+288,1E+289,
  1E+290,1E+291,1E+292,1E+293,1E+294,1E+295,1E+296,1E+297,1E+298,1E+299,
  1E+300,1E+301,1E+302,1E+303,1E+304,1E+305,1E+306,1E+307,1E+308
  };


// Fast integer power of 10
FXdouble Math::pow10i(FXint ex){
  FXASSERT(-323<=ex && ex<=308);
  return powOfTen[323+ex];
  }

/*******************************************************************************/

// Single precision integer power
FXfloat Math::powi(FXfloat base,FXint ex){
  FXfloat result=1.0f;
  if(ex){
    if(ex<0){
      base=1.0f/base;
      ex=-ex;
      }
    do{
      if(ex&1){ result*=base; }
      base*=base;
      ex>>=1;
      }
    while(ex);
    }
  return result;
  }


// Double precision integer power
FXdouble Math::powi(FXdouble base,FXint ex){
  FXdouble result=1.0;
  if(ex){
    if(ex<0){
      base=1.0/base;
      ex=-ex;
      }
    do{
      if(ex&1){ result*=base; }
      base*=base;
      ex>>=1;
      }
    while(ex);
    }
  return result;
  }

}
