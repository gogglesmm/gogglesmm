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
#undef exp
#undef expf
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

#undef fmax
#undef fmaxf
#undef fmin
#undef fminf
#undef copysign
#undef copysignf

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
#endif

// Systems below are missing these functions
#if defined(__sun__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__DragonFly__)
#define NO_EXP10F
#define NO_EXP10
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

/*********************************  Functions  *********************************/

// FOX math functions live here
namespace Math {


/// Sign of single precision float point number
static inline FXuint fpSign(FXfloat x){
  union{ FXfloat f; FXuint u; } z={x}; return (z.u>>31);
  }

/// Sign of double precision float point number
static inline FXulong fpSign(FXdouble x){
  union{ FXdouble f; FXulong u; } z={x}; return (z.u>>63);
  }


/// Mantissa of single precision float point number
static inline FXuint fpMantissa(FXfloat x){
  union{ FXfloat f; FXuint u; } z={x}; return (z.u&0x007fffff);
  }

/// Mantissa of double precision float point number
static inline FXulong fpMantissa(FXdouble x){
  union{ FXdouble f; FXulong u; } z={x}; return (z.u&FXULONG(0x000fffffffffffff));
  }


/// Exponent of single precision float point number
static inline FXuint fpExponent(FXfloat x){
  union{ FXfloat f; FXuint u; } z={x}; return ((z.u>>23)&0xff);
  }

/// Exponent of double precision float point number
static inline FXulong fpExponent(FXdouble x){
  union{ FXdouble f; FXulong u; } z={x}; return ((z.u>>52)&0x7ff);
  }


/// Single precision floating point number is finite
static inline FXbool fpFinite(FXfloat x){
  union{ FXfloat f; FXuint u; } z={x}; return ((z.u&0x7fffffff)<0x7f800000);
  }

/// Double precision floating point number is finite
static inline FXbool fpFinite(FXdouble x){
  union{ FXdouble f; FXulong u; } z={x}; return ((z.u&FXULONG(0x7fffffffffffffff))<FXULONG(0x7ff0000000000000));
  }


/// Single precision floating point number is infinite
static inline FXbool fpInfinite(FXfloat x){
  union{ FXfloat f; FXuint u; } z={x}; return ((z.u&0x7fffffff)==0x7f800000);
  }

/// Double precision floating point number is infinite
static inline FXbool fpInfinite(FXdouble x){
  union{ FXdouble f; FXulong u; } z={x}; return ((z.u&FXULONG(0x7fffffffffffffff))==FXULONG(0x7ff0000000000000));
  }


/// Single precision floating point number is NaN
static inline FXbool fpNan(FXfloat x){
  union{ FXfloat f; FXuint u; } z={x}; return (0x7f800000<(z.u&0x7fffffff));
  }

/// Double precision floating point number is NaN
static inline FXbool fpNan(FXdouble x){
  union{ FXdouble f; FXulong u; } z={x}; return (FXULONG(0x7ff0000000000000)<(z.u&FXULONG(0x7fffffffffffffff)));
  }


/// Single precision floating point number is normalized
static inline FXbool fpNormal(FXfloat x){
  union{ FXfloat f; FXuint u; } z={x};
  return ((z.u&0x7fffffff)==0) || ((0x00800000<=(z.u&0x7fffffff)) && ((z.u&0x7fffffff)<0x7f800000));
  }

/// Double precision floating point number is normalized
static inline FXbool fpNormal(FXdouble x){
  union{ FXdouble f; FXulong u; } z={x};
  return ((z.u&FXULONG(0x7fffffffffffffff))==0) || ((FXULONG(0x0010000000000000)<=(z.u&FXULONG(0x7fffffffffffffff))) && ((z.u&FXULONG(0x7fffffffffffffff))<FXULONG(0x7ff0000000000000)));
  }


/// Single precision absolute value
#if defined(NO_FABSF)
static inline FXfloat fabs(FXfloat x){ return (x<0.0f) ? -x : x; }
#else
static inline FXfloat fabs(FXfloat x){ return ::fabsf(x); }
#endif

/// Double precision absolute value
static inline FXdouble fabs(FXdouble x){ return ::fabs(x); }


/// Single precision floating point remainder
static inline FXfloat fmod(FXfloat x,FXfloat y){ return ::fmodf(x,y); }

/// Double precision floating point remainder
static inline FXdouble fmod(FXdouble x,FXdouble y){ return ::fmod(x,y); }



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

/// Single precision sine
static inline FXfloat sin(FXfloat x){ return ::sinf(x); }

/// Double precision sine
static inline FXdouble sin(FXdouble x){ return ::sin(x); }


/// Single precision cosine
static inline FXfloat cos(FXfloat x){ return ::cosf(x); }

/// Double precision cosine
static inline FXdouble cos(FXdouble x){ return ::cos(x); }


/// Single precision tangent
static inline FXfloat tan(FXfloat x){ return ::tanf(x); }

/// Double precision tangent
static inline FXdouble tan(FXdouble x){ return ::tan(x); }


/// Single precision arc sine
static inline FXfloat asin(FXfloat x){ return ::asinf(x); }

/// Double precision arc sine
static inline FXdouble asin(FXdouble x){ return ::asin(x); }


/// Single precision arc cosine
static inline FXfloat acos(FXfloat x){ return ::acosf(x); }

/// Double precision arc cosine
static inline FXdouble acos(FXdouble x){ return ::acos(x); }


/// Single precision arc tangent
static inline FXfloat atan(FXfloat x){ return ::atanf(x); }

/// Double precision arc tangent
static inline FXdouble atan(FXdouble x){ return ::atan(x); }


/// Single precision arc tangent
static inline FXfloat atan2(FXfloat y,FXfloat x){ return ::atan2f(y,x); }

/// Double precision arc tangent
static inline FXdouble atan2(FXdouble y,FXdouble x){ return ::atan2(y,x); }


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


/// Single precision power x^y
static inline FXfloat pow(FXfloat x,FXfloat y){ return ::powf(x,y); }

/// Double precision power x^y
static inline FXdouble pow(FXdouble x,FXdouble y){ return ::pow(x,y); }


/// Single precision base E exponential
static inline FXfloat exp(FXfloat x){ return ::expf(x); }

/// Double precision base E exponential
static inline FXdouble exp(FXdouble x){ return ::exp(x); }


/// Single precision base E exponential - 1
#if defined(NO_EXPM1F)
static inline FXfloat expm1(FXfloat x){ return ::expf(x)-1.0f; }
#else
static inline FXfloat expm1(FXfloat x){ return ::expm1f(x); }
#endif

/// Double precision base E exponential - 1
#if defined(NO_EXPM1)
static inline FXdouble expm1(FXdouble x){ return ::exp(x)-1.0; }
#else
static inline FXdouble expm1(FXdouble x){ return ::expm1(x); }
#endif


/// Single precision base 2 exponential
#if defined(NO_EXP2F)
static inline FXfloat exp2(FXfloat x){ return Math::pow(2.0f,x); }
#else
static inline FXfloat exp2(FXfloat x){ return ::exp2f(x); }
#endif

/// Double precision base 2 exponential
#if defined(NO_EXP2)
static inline FXdouble exp2(FXdouble x){ return Math::pow(2.0,x); }
#else
static inline FXdouble exp2(FXdouble x){ return ::exp2(x); }
#endif


/// Single precision base 10 exponential
#if defined(NO_EXP10F)
static inline FXfloat exp10(FXfloat x){ return Math::pow(10.0f,x); }
#else
static inline FXfloat exp10(FXfloat x){ return ::exp10f(x); }
#endif

/// Double precision base 10 exponential
#if defined(NO_EXP10)
static inline FXdouble exp10(FXdouble x){ return Math::pow(10.0,x); }
#else
static inline FXdouble exp10(FXdouble x){ return ::exp10(x); }
#endif


/// Single precision natural logarithm
static inline FXfloat log(FXfloat x){ return ::logf(x); }

/// Double precision natural logarithm
static inline FXdouble log(FXdouble x){ return ::log(x); }


/// Single precision logarithm of 1+x
#if defined(NO_LOG1PF)
static inline FXfloat log1p(FXfloat x){ return Math::log(1.0f+x); }
#else
static inline FXfloat log1p(FXfloat x){ return ::log1pf(x); }
#endif

/// Double precision logarithm of 1+x
#if defined(NO_LOG1P)
static inline FXdouble log1p(FXdouble x){ return Math::log(1.0+x); }
#else
static inline FXdouble log1p(FXdouble x){ return ::log1p(x); }
#endif


/// Single precision base 2 logarithm
#if defined(NO_LOG2F)
static inline FXfloat log2(FXfloat x){ return Math::log(x)*1.442695040888963407359924681001892137f; }
#else
static inline FXfloat log2(FXfloat x){ return ::log2f(x); }
#endif

/// Double precision base 2 logarithm
#if defined(NO_LOG2)
static inline FXdouble log2(FXdouble x){ return Math::log(x)*1.442695040888963407359924681001892137; }
#else
static inline FXdouble log2(FXdouble x){ return ::log2(x); }
#endif


/// Single precision base 10 logarithm
static inline FXfloat log10(FXfloat x){ return ::log10f(x); }

/// Double precision base 10 logarithm
static inline FXdouble log10(FXdouble x){ return ::log10(x); }


/// Single precision square root
static inline FXfloat sqrt(FXfloat x){ return ::sqrtf(x); }

/// Double precision square root
static inline FXdouble sqrt(FXdouble x){ return ::sqrt(x); }


/// Single precision cube root
#if defined(NO_CBRTF)
static inline FXfloat cbrt(FXfloat x){ return Math::pow(x,0.333333333333333333333333333333f); }
#else
static inline FXfloat cbrt(FXfloat x){ return ::cbrtf(x); }
#endif

/// Double precision cube root
#if defined(NO_CBRT)
static inline FXdouble cbrt(FXdouble x){ return Math::pow(x,0.333333333333333333333333333333); }
#else
static inline FXdouble cbrt(FXdouble x){ return ::cbrt(x); }
#endif


/// Single precision square
static inline FXfloat sqr(FXfloat x){ return x*x; }

/// Double precision square
static inline FXdouble sqr(FXdouble x){ return x*x; }


/// Single precision cube
static inline FXfloat cub(FXfloat x){ return x*x*x; }

/// Double precision cube
static inline FXdouble cub(FXdouble x){ return x*x*x; }


/// Single precision maximum of two
#if defined(NO_FMAXF)
static inline FXfloat fmax(FXfloat x,FXfloat y){ return (x>y)?x:y; }
#else
static inline FXfloat fmax(FXfloat x,FXfloat y){ return ::fmaxf(x,y); }
#endif

/// Double precision maximum of two
#if defined(NO_FMAX)
static inline FXdouble fmax(FXdouble x,FXdouble y){ return (x>y)?x:y; }
#else
static inline FXdouble fmax(FXdouble x,FXdouble y){ return ::fmax(x,y); }
#endif


/// Single precision minimum of two
#if defined(NO_FMINF)
static inline FXfloat fmin(FXfloat x,FXfloat y){ return (x<y)?x:y; }
#else
static inline FXfloat fmin(FXfloat x,FXfloat y){ return ::fminf(x,y); }
#endif

/// Double precision minimum of two
#if defined(NO_FMINF)
static inline FXdouble fmin(FXdouble x,FXdouble y){ return (x<y)?x:y; }
#else
static inline FXdouble fmin(FXdouble x,FXdouble y){ return ::fmin(x,y); }
#endif


/// Single precision clamp of number x to lie within range [lo..hi]
static inline FXfloat fclamp(FXfloat lo,FXfloat x,FXfloat hi){ return fmin(fmax(x,lo),hi); }

/// Double precision clamp of number x to lie within range [lo..hi]
static inline FXdouble fclamp(FXdouble lo,FXdouble x,FXdouble hi){ return fmin(fmax(x,lo),hi); }


/// Single precision copy sign of y and apply to magnitude of x
#if defined(NO_COPYSIGNF)
static inline FXfloat copysign(FXfloat x,FXfloat y){
  union{ FXfloat f; FXuint u; } xx={x},yy={y}; xx.u&=0x7fffffff; xx.u|=(yy.u&0x80000000); return xx.f;
  }
#else
static inline FXfloat copysign(FXfloat x,FXfloat y){ return ::copysignf(x,y); }
#endif


/// Double precision copy sign of y and apply to magnitude of x
#if defined(NO_COPYSIGN)
static inline FXdouble copysign(FXdouble x,FXdouble y){
  union{ FXdouble f; FXulong u; } xx={x},yy={y}; xx.u&=FXULONG(0x7fffffffffffffff); xx.u|=(yy.u&FXULONG(0x8000000000000000)); return xx.f;
  }
#else
static inline FXdouble copysign(FXdouble x,FXdouble y){ return ::copysign(x,y); }
#endif


/// Single precision integer power
extern FXAPI FXfloat ipow(FXfloat base,FXint ex);

/// Double precision integer power
extern FXAPI FXdouble ipow(FXdouble base,FXint ex);

}

}

#endif
