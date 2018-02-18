/********************************************************************************
*                                                                               *
*                           M a t h   F u n c t i o n s                         *
*                                                                               *
*********************************************************************************
* Copyright (C) 2015,2017 by Jeroen van der Zijp.   All Rights Reserved.        *
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
    register FXulong sx=z.u>>63;
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

// Single precision integer power
FXfloat Math::ipow(FXfloat base,FXint ex){
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
FXdouble Math::ipow(FXdouble base,FXint ex){
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
