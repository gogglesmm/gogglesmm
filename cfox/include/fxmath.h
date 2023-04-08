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
#ifndef FXMATH_H
#define FXMATH_H

// Remove macros
#undef fabs
#undef fabsf
#undef fmod
#undef fmodf

#undef ceil
#undef ceilf
#undef floor
#undef floorf
#undef round
#undef roundf
#undef trunc
#undef truncf
#undef nearbyint
#undef nearbyintf
#undef rint
#undef rintf

#undef sin
#undef sinf
#undef cos
#undef cosf
#undef tan
#undef tanf
#undef asin
#undef asinf
#undef acos
#undef acosf
#undef atan
#undef atanf
#undef atan2
#undef atan2f
#undef sincos
#undef sincosf
#undef sinh
#undef sinhf
#undef cosh
#undef coshf
#undef tanh
#undef tanhf
#undef asinh
#undef asinhf
#undef acosh
#undef acoshf
#undef atanh
#undef atanhf

#undef pow
#undef powf
#undef pow10
#undef pow10f
#undef exp
#undef expf
#undef expm1
#undef expm1f
#undef exp2
#undef exp2f
#undef exp10
#undef exp10f
#undef log
#undef logf
#undef log1p
#undef log1pf
#undef log2
#undef log2f
#undef log10
#undef log10f

#undef sqrt
#undef sqrtf
#undef cbrt
#undef cbrtf
#undef sqr
#undef sqrf
#undef cub
#undef cubf

#undef max
#undef min
#undef fmax
#undef fmaxf
#undef fmin
#undef fminf
#undef copysign
#undef copysignf
#undef hypot
#undef hypotf


// Switch on remedial math on Windows with VC++
#if defined(WIN32) && (defined(_MSC_VER) || defined(__MINGW32__))
#define NO_CEILF
#define NO_FLOORF
#define NO_ROUNDF
#define NO_ROUND
#define NO_TRUNCF
#define NO_TRUNC
#define NO_NEARBYINTF
#define NO_NEARBYINT
#define NO_RINTF
#define NO_RINT
#define NO_EXPM1F
#define NO_EXPM1
#define NO_EXP2F
#define NO_EXP2
#define NO_EXP10F
#define NO_EXP10
#define NO_POW10F
#define NO_POW10
#define NO_LOG1PF
#define NO_LOG1P
#define NO_LOG2F
#define NO_LOG2
#define NO_CBRTF
#define NO_CBRT
#define NO_SINHF
#define NO_SINH
#define NO_COSHF
#define NO_COSH
#define NO_TANHF
#define NO_TANH
#define NO_ASINHF
#define NO_ASINH
#define NO_ACOSHF
#define NO_ACOSH
#define NO_ATANHF
#define NO_ATANH
#define NO_FMAXF
#define NO_FMAX
#define NO_FMINF
#define NO_FMIN
#define NO_COPYSIGNF
#define NO_COPYSIGN
#define NO_HYPOTF
#define NO_HYPOT
#define NO_FDIMF
#define NO_FDIM
#define NO_SINCOS
#define NO_SINCOSF
#define NO_LRINT
#define NO_LRINTF
#endif

// Systems below are missing these functions
#if defined(__sun__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__DragonFly__) || defined(__APPLE__)
#define NO_EXP10F
#define NO_EXP10
#endif

// Apple is missing sincos
#if defined(__APPLE__)
#define NO_SINCOS
#define NO_SINCOSF
#endif


namespace FX {

/*********************************  Constants  *********************************/

/// Pi
const FXdouble PI=3.1415926535897932384626433833;

/// Euler constant
const FXdouble EULER=2.7182818284590452353602874713;

/// Multiplier for degrees to radians
const FXdouble DTOR=0.0174532925199432957692369077;

/// Multiplier for radians to degrees
const FXdouble RTOD=57.295779513082320876798154814;

/// Feigenbaum constant
const FXdouble FEIGENBAUM=4.6692016091029906718532038215;

/*********************************  Functions  *********************************/

// FOX math functions live here
namespace Math {

/// Sign of single precision float point number (0..1)
extern FXAPI FXint fpSign(FXfloat x);

/// Sign of double precision float point number (0..1)
extern FXAPI FXlong fpSign(FXdouble x);

/// Signed exponent of single precision float point number (-126..128)
extern FXAPI FXint fpExponent(FXfloat x);

/// Signed exponent of double precision float point number (-1022..1024)
extern FXAPI FXlong fpExponent(FXdouble x);

/// Mantissa of single precision float point number (including hidden bit)
extern FXAPI FXint fpMantissa(FXfloat x);

/// Mantissa of double precision float point number (including hidden bit)
extern FXAPI FXlong fpMantissa(FXdouble x);

/// Single precision floating point number is finite
extern FXAPI FXbool fpFinite(FXfloat x);

/// Double precision floating point number is finite
extern FXAPI FXbool fpFinite(FXdouble x);

/// Single precision floating point number is infinite
extern FXAPI FXbool fpInfinite(FXfloat x);

/// Double precision floating point number is infinite
extern FXAPI FXbool fpInfinite(FXdouble x);

/// Single precision floating point number is NaN
extern FXAPI FXbool fpNan(FXfloat x);

/// Double precision floating point number is NaN
extern FXAPI FXbool fpNan(FXdouble x);

/// Single precision floating point number is normalized
extern FXAPI FXbool fpNormal(FXfloat x);

/// Double precision floating point number is normalized
extern FXAPI FXbool fpNormal(FXdouble x);

/// All bits of single precision floating point number
extern FXAPI FXuint fpBits(FXfloat x);

/// All bits of double precision floating point number
extern FXAPI FXulong fpBits(FXdouble x);


/// Evaluate integer (a < b) ? x : y
static inline FXint iblend(FXint a,FXint b,FXint x,FXint y){
  return a<b ? x : y;
  }

/// Evaluate long integer (a < b) ? x : y
static inline FXlong iblend(FXlong a,FXlong b,FXlong x,FXlong y){
  return a<b ? x : y;
  }


/// Minimum if two integers
static inline FXint imin(FXint x,FXint y){
  return (x<y)?x:y;
  }

/// Minimum if two longs
static inline FXlong imin(FXlong x,FXlong y){
  return (x<y)?x:y;
  }


/// Minimum of two integers
static inline FXint imax(FXint x,FXint y){
  return (x>y)?x:y;
  }

/// Minimum of two longs
static inline FXlong imax(FXlong x,FXlong y){
  return (x>y)?x:y;
  }


/// Absolute value of integer
static inline FXint iabs(FXint x){
  return 0<x?x:-x;
  }

/// Absolute value of long
static inline FXlong iabs(FXlong x){
  return 0<x?x:-x;
  }


/// Integer clamp of integer x to lie within range [lo..hi]
static inline FXint iclamp(FXint lo,FXint x,FXint hi){
  return Math::imin(Math::imax(x,lo),hi);
  }

/// Long clamp of long x to lie within range [lo..hi]
static inline FXlong iclamp(FXlong lo,FXlong x,FXlong hi){
  return Math::imin(Math::imax(x,lo),hi);
  }


/// Integer clamp of integer x to [-lim..lim], where lim>0
static inline FXint iclamp(FXint x,FXint lim){
  return imin(imax(x,-lim),lim);
  }

/// Long clamp of long x to [-lim..lim], where lim>0
static inline FXlong iclamp(FXlong x,FXlong lim){
  return imin(imax(x,-lim),lim);
  }


/// Sign of integer, return -1 if x is <0, +1 if x>0, and zero otherwise
static inline FXint isign(FXint x){
  return (x>0)-(x<0);
  }

/// Sign of integer, return -1 if x is <0, +1 if x>0, and zero otherwise
static inline FXlong isign(FXlong x){
  return (x>0)-(x<0);
  }


/// Single precision minimum of two
static inline FXfloat fmin(FXfloat x,FXfloat y){
#if defined(NO_FMINF)
  return (x<y)?x:y;
#else
  return ::fminf(x,y);
#endif
  }


/// Double precision minimum of two
static inline FXdouble fmin(FXdouble x,FXdouble y){
#if defined(NO_FMIN)
  return (x<y)?x:y;
#else
  return ::fmin(x,y);
#endif
  }


/// Single precision maximum of two
static inline FXfloat fmax(FXfloat x,FXfloat y){
#if defined(NO_FMAXF)
  return (x>y)?x:y;
#else
  return ::fmaxf(x,y);
#endif
  }

/// Double precision maximum of two
static inline FXdouble fmax(FXdouble x,FXdouble y){
#if defined(NO_FMAX)
  return (x>y)?x:y;
#else
  return ::fmax(x,y);
#endif
  }


/// Single precision absolute value
static inline FXfloat fabs(FXfloat x){
#if defined(NO_FABSF)
  return (x<0.0f) ? -x : x;
#else
  return ::fabsf(x);
#endif
  }

/// Double precision absolute value
static inline FXdouble fabs(FXdouble x){
  return ::fabs(x);
  }


/// Single precision clamp of number x to lie within range [lo..hi]
static inline FXfloat fclamp(FXfloat lo,FXfloat x,FXfloat hi){
  return Math::fmin(Math::fmax(x,lo),hi);
  }

/// Double precision clamp of number x to lie within range [lo..hi]
static inline FXdouble fclamp(FXdouble lo,FXdouble x,FXdouble hi){
  return Math::fmin(Math::fmax(x,lo),hi);
  }


/// Single precision clamp of number x to [-lim..lim], where lim>0
static inline FXfloat fclamp(FXfloat x,FXfloat lim){
  return fmin(fmax(x,-lim),lim);
  }

/// Double precision clamp of number x to [-lim..lim], where lim>0
static inline FXdouble fclamp(FXdouble x,FXdouble lim){
  return fmin(fmax(x,-lim),lim);
  }


/// Single precision positive difference
static inline FXfloat fdim(FXfloat x,FXfloat y){
#if defined(NO_FDIMF)
  return Math::fmax(x-y,0.0f);
#else
  return ::fdimf(x,y);
#endif
  }

/// Double precision positive difference
static inline FXdouble fdim(FXdouble x,FXdouble y){
#if defined(NO_FDIM)
  return Math::fmax(x-y,0.0);
#else
  return ::fdim(x,y);
#endif
  }


/// Single precision floating point remainder
static inline FXfloat fmod(FXfloat x,FXfloat y){
  return ::fmodf(x,y);
  }

/// Double precision floating point remainder
static inline FXdouble fmod(FXdouble x,FXdouble y){
  return ::fmod(x,y);
  }


/// Evaluate single-precision (a < b) ? x : y
static inline FXfloat fblend(FXfloat a,FXfloat b,FXfloat x,FXfloat y){
  return a<b ? x : y;
  }


/// Evaluate double-precision (a < b) ? x : y
static inline FXdouble fblend(FXdouble a,FXdouble b,FXdouble x,FXdouble y){
  return a<b ? x : y;
  }


/// Single precision copy sign of y and apply to magnitude of x
static inline FXfloat copysign(FXfloat x,FXfloat y){
#if defined(NO_COPYSIGNF)
  union{ FXfloat f; FXuint u; } xx={x},yy={y};
  xx.u&=0x7fffffff;
  xx.u|=(yy.u&0x80000000);
  return xx.f;
#else
  return ::copysignf(x,y);
#endif
  }

/// Double precision copy sign of y and apply to magnitude of x
static inline FXdouble copysign(FXdouble x,FXdouble y){
#if defined(NO_COPYSIGN)
  union{ FXdouble f; FXulong u; } xx={x},yy={y};
  xx.u&=FXULONG(0x7fffffffffffffff);
  xx.u|=(yy.u&FXULONG(0x8000000000000000));
  return xx.f;
#else
  return ::copysign(x,y);
#endif
  }


/// Single precision ceiling (round upward to nearest integer)
#if defined(NO_CEILF)
extern FXAPI FXfloat ceil(FXfloat x);
#else
static inline FXfloat ceil(FXfloat x){ return ::ceilf(x); }
#endif

/// Double precision ceiling (round upward to nearest integer)
static inline FXdouble ceil(FXdouble x){ return ::ceil(x); }


/// Single precision floor (round down to nearest integer)
#if defined(NO_FLOORF)
extern FXAPI FXfloat floor(FXfloat x);
#else
static inline FXfloat floor(FXfloat x){ return ::floorf(x); }
#endif

/// Double precision floor (round down to nearest integer)
static inline FXdouble floor(FXdouble x){ return ::floor(x); }

/// Single precision round to nearest integer (away from zero)
#if defined(NO_ROUNDF)
extern FXAPI FXfloat round(FXfloat x);
#else
static inline FXfloat round(FXfloat x){ return ::roundf(x); }
#endif

/// Double precision round to nearest integer (away from zero)
#if defined(NO_ROUND)
extern FXAPI FXdouble round(FXdouble x);
#else
static inline FXdouble round(FXdouble x){ return ::round(x); }
#endif


/// Single precision truncate to nearest integer (toward zero)
#if defined(NO_TRUNCF)
extern FXAPI FXfloat trunc(FXfloat x);
#else
static inline FXfloat trunc(FXfloat x){ return ::truncf(x); }
#endif

/// Double precision truncate to nearest integer (toward zero)
#if defined(NO_TRUNC)
extern FXAPI FXdouble trunc(FXdouble x);
#else
static inline FXdouble trunc(FXdouble x){ return ::trunc(x); }
#endif


/// Single precision round to nearest integer
#if defined(NO_NEARBYINTF)
extern FXAPI FXfloat nearbyint(FXfloat x);
#else
static inline FXfloat nearbyint(FXfloat x){ return ::nearbyintf(x); }
#endif

/// Double precision round to nearest integer
#if defined(NO_NEARBYINT)
extern FXAPI FXdouble nearbyint(FXdouble x);
#else
static inline FXdouble nearbyint(FXdouble x){ return ::nearbyint(x); }
#endif

/// Single precision round to nearest integer
#if defined(NO_RINTF)
extern FXAPI FXfloat rint(FXfloat x);
#else
static inline FXfloat rint(FXfloat x){ return ::rintf(x); }
#endif

/// Double precision round to nearest integer
#if defined(NO_RINT)
extern FXAPI FXdouble rint(FXdouble x);
#else
static inline FXdouble rint(FXdouble x){ return ::rint(x); }
#endif


/// Single precision round to nearest integer
static inline FXint lrint(FXfloat x){
#if defined(NO_LRINTF)
  return (FXint)(x+Math::copysign(0.5f,x));
#else
  return ::lrintf(x);
#endif
 }

/// Double precision round to nearest integer
static inline FXlong lrint(FXdouble x){
#if defined(NO_LRINT)
 return (FXlong)(x+Math::copysign(0.5,x));
#else
 return ::lrint(x);
#endif
 }


/// Wrap single precision phase argument x to -PI...PI range
static inline FXfloat wrap(FXfloat x){
#if defined(__SSE4_1__)
  return x-Math::nearbyint(x*0.159154943091895335768883763373f)*6.28318530717958647692528676656f;
#else
  return x-Math::lrint(x*0.159154943091895335768883763373f)*6.28318530717958647692528676656f;
#endif
  }

/// Wrap double precision phase argument x to -PI...PI range
static inline FXdouble wrap(FXdouble x){
#if defined(__SSE4_1__)
  return x-Math::nearbyint(x*0.159154943091895335768883763373)*6.28318530717958647692528676656;
#else
  return x-Math::lrint(x*0.159154943091895335768883763373)*6.28318530717958647692528676656;
#endif
  }


/// Four quadrant wrap phase argument x to 0...2*PI range
static inline FXfloat wrap4(FXfloat x){
  return x-Math::floor(x*0.159154943091895335768883763373f)*6.28318530717958647692528676656f;
  }


/// Four quadrant wrap phase argument x to 0...2*PI range
static inline FXdouble wrap4(FXdouble x){
  return x-Math::floor(x*0.159154943091895335768883763373)*6.28318530717958647692528676656;
  }


/// Stepify single precision x into multiples of step s (where s>0)
static inline FXfloat stepify(FXfloat x,FXfloat s){
  return Math::nearbyint(x/s)*s;
  }

/// Stepify double precision x into multiples of step s (where s>0)
static inline FXdouble stepify(FXdouble x,FXdouble s){
  return Math::nearbyint(x/s)*s;
  }


/// Single precision zig-zag function, period 1
static inline FXfloat zigzag(FXfloat x){
  return Math::fabs(2.0f*(x-Math::nearbyint(x)));
  }

/// Single precision zig-zag function, period 1
static inline FXdouble zigzag(FXdouble x){
  return Math::fabs(2.0*(x-Math::nearbyint(x)));
  }


/// Single precision sawtooth function, period 1
static inline FXfloat sawtooth(FXfloat x){
  return x-Math::floor(x);
  }

/// Single precision sawtooth function, period 1
static inline FXdouble sawtooth(FXdouble x){
  return x-Math::floor(x);
  }


/// Single precision revserse sawtooth function, period 1
static inline FXfloat rsawtooth(FXfloat x){
  return Math::ceil(x)-x;
  }

/// Single precision revserse sawtooth function, period 1
static inline FXdouble rsawtooth(FXdouble x){
  return Math::ceil(x)-x;
  }


/// Single precision sine
static inline FXfloat sin(FXfloat x){
  return ::sinf(x);
  }

/// Double precision sine
static inline FXdouble sin(FXdouble x){
  return ::sin(x);
  }


/// Single precision cosine
static inline FXfloat cos(FXfloat x){
  return ::cosf(x);
  }

/// Double precision cosine
static inline FXdouble cos(FXdouble x){
  return ::cos(x);
  }


/// Single precision tangent
static inline FXfloat tan(FXfloat x){
  return ::tanf(x);
  }

/// Double precision tangent
static inline FXdouble tan(FXdouble x){
  return ::tan(x);
  }


/// Single precision arc sine
static inline FXfloat asin(FXfloat x){
  return ::asinf(x);
  }

/// Double precision arc sine
static inline FXdouble asin(FXdouble x){
  return ::asin(x);
  }


/// Single precision arc cosine
static inline FXfloat acos(FXfloat x){
  return ::acosf(x);
  }

/// Double precision arc cosine
static inline FXdouble acos(FXdouble x){
  return ::acos(x);
  }


/// Single precision arc tangent
static inline FXfloat atan(FXfloat x){
  return ::atanf(x);
  }

/// Double precision arc tangent
static inline FXdouble atan(FXdouble x){
  return ::atan(x);
  }


/// Single precision arc tangent
static inline FXfloat atan2(FXfloat y,FXfloat x){
  return ::atan2f(y,x);
  }

/// Double precision arc tangent
static inline FXdouble atan2(FXdouble y,FXdouble x){
  return ::atan2(y,x);
  }


/// Single precision sincos
static inline void sincos(FXfloat x,FXfloat& s,FXfloat& c){
#if defined(NO_SINCOSF)
  s=Math::sin(x);
  c=Math::cos(x);
#else
  ::sincosf(x,&s,&c);
#endif
  }


/// Double precision sincos
static inline void sincos(FXdouble x,FXdouble& s,FXdouble& c){
#if defined(NO_SINCOS)
  s=Math::sin(x);
  c=Math::cos(x);
#else
  ::sincos(x,&s,&c);
#endif
  }


/// Single precision hyperbolic sine
#if defined(NO_SINHF)
extern FXAPI FXfloat sinh(FXfloat x);
#else
static inline FXfloat sinh(FXfloat x){ return ::sinhf(x); }
#endif

/// Double precision hyperbolic sine
#if defined(NO_SINH)
extern FXAPI FXdouble sinh(FXdouble x);
#else
static inline FXdouble sinh(FXdouble x){ return ::sinh(x); }
#endif


/// Single precision hyperbolic cosine
#if defined(NO_COSHF)
extern FXAPI FXfloat cosh(FXfloat x);
#else
static inline FXfloat cosh(FXfloat x){ return ::coshf(x); }
#endif

/// Double precision hyperbolic cosine
#if defined(NO_COSH)
extern FXAPI FXdouble cosh(FXdouble x);
#else
static inline FXdouble cosh(FXdouble x){ return ::cosh(x); }
#endif


/// Single precision hyperbolic tangent
#if defined(NO_TANHF)
extern FXAPI FXfloat tanh(FXfloat x);
#else
static inline FXfloat tanh(FXfloat x){ return ::tanhf(x); }
#endif

/// Double precision hyperbolic tangent
#if defined(NO_TANH)
extern FXAPI FXdouble tanh(FXdouble x);
#else
static inline FXdouble tanh(FXdouble x){ return ::tanh(x); }
#endif


/// Single precision hyperbolic arc sine
#if defined(NO_ASINHF)
extern FXAPI FXfloat asinh(FXfloat x);
#else
static inline FXfloat asinh(FXfloat x){ return ::asinhf(x); }
#endif

/// Double precision hyperbolic arc sine
#if defined(NO_ASINH)
extern FXAPI FXdouble asinh(FXdouble x);
#else
static inline FXdouble asinh(FXdouble x){ return ::asinh(x); }
#endif


/// Single precision hyperbolic arc cosine
#if defined(NO_ACOSHF)
extern FXAPI FXfloat acosh(FXfloat x);
#else
static inline FXfloat acosh(FXfloat x){ return ::acoshf(x); }
#endif

/// Double precision  hyperbolic arc cosine
#if defined(NO_ACOSH)
extern FXAPI FXdouble acosh(FXdouble x);
#else
static inline FXdouble acosh(FXdouble x){ return ::acosh(x); }
#endif


/// Single precision hyperbolic arc tangent
#if defined(NO_ATANHF)
extern FXAPI FXfloat atanh(FXfloat x);
#else
static inline FXfloat atanh(FXfloat x){ return ::atanhf(x); }
#endif

/// Double precision hyperbolic arc tangent
#if defined(NO_ATANH)
extern FXAPI FXdouble atanh(FXdouble x);
#else
static inline FXdouble atanh(FXdouble x){ return ::atanh(x); }
#endif


/// Single precision square root
static inline FXfloat sqrt(FXfloat x){
  return ::sqrtf(x);
  }

/// Double precision square root
static inline FXdouble sqrt(FXdouble x){
  return ::sqrt(x);
  }


/// Safe single precision square root
static inline FXfloat safesqrt(FXfloat x){
  return Math::sqrt(Math::fmax(x,0.0f));
  }

/// Safe double precision square root
static inline FXdouble safesqrt(FXdouble x){
  return Math::sqrt(Math::fmax(x,0.0));
  }


/// Single precision cube root
static inline FXfloat cbrt(FXfloat x){
#if defined(NO_CBRTF)
  return ::powf(x,0.333333333333333333333333333333f);
#else
  return ::cbrtf(x);
#endif
  }

/// Double precision cube root
static inline FXdouble cbrt(FXdouble x){
#if defined(NO_CBRT)
  return ::pow(x,0.333333333333333333333333333333);
#else
  return ::cbrt(x);
#endif
  }


/// Single precision square
static inline FXfloat sqr(FXfloat x){
  return x*x;
  }

/// Double precision square
static inline FXdouble sqr(FXdouble x){
  return x*x;
  }


/// Single precision cube
static inline FXfloat cub(FXfloat x){
  return x*x*x;
  }

/// Double precision cube
static inline FXdouble cub(FXdouble x){
  return x*x*x;
  }


/// Single precision calculate hypothenuse sqrt(x^2+y^2)
static inline FXfloat hypot(FXfloat x,FXfloat y){
#if defined(NO_HYPOTF)
  return Math::sqrt(Math::sqr(x)+Math::sqr(y));
#else
  return ::hypotf(x,y);
#endif
  }

/// Double precision calculate hypothenuse sqrt(x^2+y^2)
static inline FXdouble hypot(FXdouble x,FXdouble y){
#if defined(NO_HYPOT)
  return Math::sqrt(Math::sqr(x)+Math::sqr(y));
#else
  return ::hypot(x,y);
#endif
  }


/// Linearly interpolate between u and v as f goes from 0...1
static inline FXfloat lerp(FXfloat u,FXfloat v,FXfloat f){
  return (v-u)*f+u;
  }


/// Linearly interpolate between u and v as f goes from 0...1
static inline FXdouble lerp(FXdouble u,FXdouble v,FXdouble f){
  return (v-u)*f+u;
  }


/// Smooth transition from 0 to 1 as f goes from 0...1
static inline FXfloat smoothstep(FXfloat f){
  return (3.0f-2.0f*f)*f*f;
  }

/// Smooth transition from 0 to 1 as f goes from 0...1
static inline FXdouble smoothstep(FXdouble f){
  return (3.0-2.0*f)*f*f;
  }


/// Single precision base E exponential
static inline FXfloat exp(FXfloat x){
  return ::expf(x);
  }

/// Double precision base E exponential
static inline FXdouble exp(FXdouble x){
  return ::exp(x);
  }


/// Single precision base E exponential - 1
static inline FXfloat expm1(FXfloat x){
#if defined(NO_EXPM1F)
  return ::expf(x)-1.0f;
#else
  return ::expm1f(x);
#endif
  }

/// Double precision base E exponential - 1
static inline FXdouble expm1(FXdouble x){
#if defined(NO_EXPM1)
  return ::exp(x)-1.0;
#else
  return ::expm1(x);
#endif
  }


/// Single precision power x^y
static inline FXfloat pow(FXfloat x,FXfloat y){
  return ::powf(x,y);
  }

/// Double precision power x^y
static inline FXdouble pow(FXdouble x,FXdouble y){
  return ::pow(x,y);
  }


/// Single precision base 2 exponential
static inline FXfloat exp2(FXfloat x){
#if defined(NO_EXP2F)
  return Math::pow(2.0f,x);
#else
  return ::exp2f(x);
#endif
  }


/// Double precision base 2 exponential
static inline FXdouble exp2(FXdouble x){
#if defined(NO_EXP2)
  return Math::pow(2.0,x);
#else
  return ::exp2(x);
#endif
  }


/// Single precision base 10 exponential
static inline FXfloat exp10(FXfloat x){
#if defined(NO_EXP10F)
  return Math::pow(10.0f,x);
#else
  return ::exp10f(x);
#endif
  }


/// Double precision base 10 exponential
static inline FXdouble exp10(FXdouble x){
#if defined(NO_EXP10)
  return Math::pow(10.0,x);
#else
  return ::exp10(x);
#endif
  }


/// Single precision 10^x
static inline FXfloat pow10(FXfloat x){
  return Math::exp10(x);
  }

/// Double precision 10^x
static inline FXdouble pow10(FXdouble x){
  return Math::exp10(x);
  }


/// Double precision integer power of 10
extern FXAPI FXdouble pow10i(FXint ex);


/// Single precision integer power
extern FXAPI FXfloat powi(FXfloat base,FXint ex);


/// Double precision integer power
extern FXAPI FXdouble powi(FXdouble base,FXint ex);


/// Single precision natural logarithm
static inline FXfloat log(FXfloat x){
  return ::logf(x);
  }

/// Double precision natural logarithm
static inline FXdouble log(FXdouble x){
  return ::log(x);
  }


/// Single precision logarithm of 1+x
static inline FXfloat log1p(FXfloat x){
#if defined(NO_LOG1PF)
  return Math::log(1.0f+x);
#else
  return ::log1pf(x);
#endif
  }


/// Double precision logarithm of 1+x
static inline FXdouble log1p(FXdouble x){
#if defined(NO_LOG1P)
  return Math::log(1.0+x);
#else
  return ::log1p(x);
#endif
  }


/// Single precision base 2 logarithm
static inline FXfloat log2(FXfloat x){
#if defined(NO_LOG2F)
  return Math::log(x)*1.442695040888963407359924681001892137f;
#else
  return ::log2f(x);
#endif
  }


/// Double precision base 2 logarithm
static inline FXdouble log2(FXdouble x){
#if defined(NO_LOG2)
  return Math::log(x)*1.442695040888963407359924681001892137;
#else
  return ::log2(x);
#endif
  }


/// Single precision base 10 logarithm
static inline FXfloat log10(FXfloat x){
  return ::log10f(x);
  }

/// Double precision base 10 logarithm
static inline FXdouble log10(FXdouble x){
  return ::log10(x);
  }


/// Single precision error function
extern FXAPI FXfloat erf(FXfloat x);

/// Double precision error function
extern FXAPI FXdouble erf(FXdouble x);


/// Single precision complementary error function
extern FXAPI FXfloat erfc(FXfloat x);

/// Double precision complementary error function
extern FXAPI FXdouble erfc(FXdouble x);


/// Single precision inverse error function
extern FXAPI FXfloat inverf(FXfloat x);

/// Double precision inverse error function
extern FXAPI FXdouble inverf(FXdouble x);


/// Single precision inverse complementary error function
extern FXAPI FXfloat inverfc(FXfloat x);

/// Double precision inverse complementary error function
extern FXAPI FXdouble inverfc(FXdouble x);

}

}

#endif
