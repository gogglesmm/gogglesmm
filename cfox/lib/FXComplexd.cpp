/********************************************************************************
*                                                                               *
*          D o u b l e - P r e c i s i o n   C o m p l e x   N u m b e r        *
*                                                                               *
*********************************************************************************
* Copyright (C) 2006,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "FXStream.h"
#include "FXObject.h"
#include "FXComplexd.h"


/*
  Notes:

  - The cpow() function return complex power function value

      z = (x) + j(y) cpow(x,y) = cexp(y*clog(x))

  - The cproj() function computes a projection of z onto the Riemann sphere: Argument projects to argument, except that all complex infinities (even those with one infinite part and one NaN part) project to positive infinity on the real axis. If argument has an infinite part, then cproj(argument) shall be equivalent to: INFINITY + I * copysign(0.0, cimag(argument))

      z = (x) + j(y) cproj(z) = (2.0 * z) / (~z*z + 1.0)

  - The csin() function returns the complex sine value.

      z = (x) + j(y) csin(z) = cosh(y) * sin(x) + j * sinh(y) * cos(x)

  - The csinh() function returnes complex hyperbolic sine value

      z = (x) + j(y) csinh(z) = sinh(x) * cos(y) + j * cosh(x) * sin(y)

  - The sqrt() function returns the complex square root value, in the range of the right half-plane (including the imaginary axis)

      z = (x) + j(y) csqrt(z) = sqrt(0.5*(cabs(z)+x)) + j*(y/abs(y))*sqrt(0.5*(cabs(z)-x))

      r = sqrt(0.5 * (abs(x) + cabs(z)) q = (0.5 * y) / r where(x >= 0) csqrt(z) = r + j*q where(x < 0 && y >= 0) csqrt(z) = q + j*r where(x < 0 && y < 0) csqrt(z) = -q + j*r

  -The ctan() function returns the complex tangent value.

      z = (x) + j(y) ctan(z) = (sin(2*x) + j*sinh(2*y)) / (cos(2*x) + cosh(2*y)) when (CCOS(z) = 0 + j0) ctan(z)= MAX_POS_DBLF + j0 else ctan (z) = CSIN(z) / CCOS(z)

  - The ctanh() function returnes complex hyperbolic tangent value

      z = (x) + j(y) ctanh(z) = (sinh(2*x) + j * sin(2*y)) / (cosh(2*x) + cos(2*y))

  - The cacos() function returns the complex arc cosine value, in the range of a strip mathematically unbounded along the imaginary axis and in the interval [0,pi] along the real axis.

      z = (x) + j(y) cacos(z) = (PI/2, 0) - casin(z) (old version) cacos(z) = (+/-) j*cacosh(z)


  - The cacosh() function returns the complex arc hyperbolic cosine value, in the range of a half-strip of non-negative values along the real axis and in the interval [-i pi, +i pi] along the imaginary axis.

      z = (x) + j(y) cacosh(z) = clog(z+csqrt(~z*z-1.0))

  - The carg() function returns the value of the argument in the interval [-pi, +pi].

      z = (x) + j(y) carg(z) = atan2(y,x)

  - The casin() function returns the complex arc sine value, in the range of a strip mathematically unbounded along the imaginary axis and in the interval [-pi/2, +pi/2] along the real axis.

      z = (x) + j(y) casin(z) = -j*casinh(j*z)

  - The casinh() function returns the complex arc hyperbolic cosine value, in the range of a half-strip of non-negative values along the real axis and in the interval [-i pi, +i pi] along the imaginary axis.

      z = (x) + j(y) casinh(z) = clog(z+csqrt(~z*z+1))

  - The catan() function returns the complex arc tangent value, in the range of a strip mathematically unbounded along the imaginary axis and in the interval [-pi/2, +pi/2] along the real axis.

      z = (x) + j(y) catan(z) = -j/2 * log( (j+z) / (j-z) ) catan(z) = 0.5 * atan(2*x,1-(x^2)-(y^2)) + j*0.25*(log((x^2)+(y+1)^2) - log((x^2)+(y-1)^2))

  - The catanh() function returns the complex arc hyperbolic tangent value, in the range of a strip mathematically unbounded along the real axis and in the interval [-i pi/2, +i pi/2] along the imaginary axis.

      z = (x) + j(y) catanh(z) = 0.5 * clog((1+z)/(1-z)) catanh(z) = 0.25 * (log( (y^2)+(1+x)^2 ) - log( (y^2)+(1-x)^2 )) + j * 0.5 * atan(2*y,(1-(x^2)-(y^2))

  - The ccos() function returns the complex cosine value.

      z =(x) + j(y) ccos(z) = ccosh(j*z)

  - The ccosh() function returnes complex hyperbolic cosine value

      z = (x) + j(y) ccosh(z) = cosh(x) * cos(y) + j * sinh(x) * sin(y)

*/

using namespace FX;


/*******************************************************************************/

namespace FX {


// Complex square root
FXComplexd csqrt(const FXComplexd& c){
  FXdouble mag=abs(c);
  FXdouble rr=Math::sqrt((mag+c.re)*0.5);
  FXdouble ii=Math::sqrt((mag-c.re)*0.5);
  return FXComplexd(rr,Math::copysign(ii,c.im));
  }


// Complex sine
FXComplexd csin(const FXComplexd& c){
  return FXComplexd(Math::sin(c.re)*Math::cosh(c.im),Math::cos(c.re)*Math::sinh(c.im));
  }


// Complex cosine
FXComplexd ccos(const FXComplexd& c){
  return FXComplexd(Math::cos(c.re)*Math::cosh(c.im),-Math::sin(c.re)*Math::sinh(c.im));
  }


// Complex tangent
FXComplexd ctan(const FXComplexd& c){
  FXComplexd ep=exp(FXComplexd(-c.im,c.re));
  FXComplexd em=exp(FXComplexd(c.im,-c.re));
  FXComplexd t=(em-ep)/(em+ep);
  return FXComplexd(-t.im,t.re);
  }


// Complex hyperbolic sine
FXComplexd csinh(const FXComplexd& c){
  return FXComplexd(Math::cos(c.im)*Math::sinh(c.re),Math::sin(c.im)*Math::cosh(c.re));
  }


// Complex hyperbolic cosine
FXComplexd ccosh(const FXComplexd& c){
  return FXComplexd(Math::cos(c.im)*Math::cosh(c.re),Math::sin(c.im)*Math::sinh(c.re));
  }


// Complex hyperbolic tangent
FXComplexd ctanh(const FXComplexd& c){
  return csinh(c)/ccosh(c);
  }


FXStream& operator<<(FXStream& store,const FXComplexd& c){
  store << c.re << c.im;
  return store;
  }


FXStream& operator>>(FXStream& store,FXComplexd& c){
  store >> c.re >> c.im;
  return store;
  }

}
