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
#ifndef FXCOMPLEXD_H
#define FXCOMPLEXD_H


namespace FX {


/// Double-precision complex
class FXAPI FXComplexd {
public:
  FXdouble re;
  FXdouble im;
public:

  /// Default constructor; value is not initialized
  FXComplexd(){ }

  /// Construct from real
  FXComplexd(FXdouble r):re(r),im(0.0){ }

  /// Construct from components
  FXComplexd(FXdouble r,FXdouble i):re(r),im(i){ }

  /// Initialize from another complex
  FXComplexd(const FXComplexd& c):re(c.re),im(c.im){ }

  /// Set value from real
  FXComplexd& set(FXdouble r){ re=r; im=0.0; return *this; }

  /// Set value from components
  FXComplexd& set(FXdouble r,FXdouble i){ re=r; im=i; return *this;}

  /// Set value from another complex
  FXComplexd& set(const FXComplexd& c){ re=c.re; im=c.im; return *this;}

  /// Test if zero
  FXbool operator!() const { return (re==0.0) && (im==0.0); }

  /// Access real part
  FXdouble& real(){ return re; }
  const FXdouble& real() const { return re; }

  /// Access imaginary part
  FXdouble& imag(){ return im; }
  const FXdouble& imag() const { return im; }

  /// Squared modulus
  FXdouble modulus2() const { return re*re+im*im; }

  /// Modulus or absolute value of complex
  FXdouble modulus() const { return Math::sqrt(modulus2()); }

  /// Argument of complex
  FXdouble argument() const { return Math::atan2(im,re); }

  /// Return a non-const reference to the ith element
  FXdouble& operator[](FXint i){ return (&re)[i]; }

  /// Return a const reference to the ith element
  const FXdouble& operator[](FXint i) const { return (&re)[i]; }

  /// Unary
  FXComplexd operator+() const { return *this; }
  FXComplexd operator-() const { return FXComplexd(-re,-im); }

  /// Assignment from real
  FXComplexd& operator=(const FXdouble r){ return set(r); }

  /// Assignment from another complex
  FXComplexd& operator=(const FXComplexd& c){ return set(c); }

  /// Assigning operators with real
  FXComplexd& operator+=(FXdouble r){ re+=r; return *this; }
  FXComplexd& operator-=(FXdouble r){ re-=r; return *this; }
  FXComplexd& operator*=(FXdouble r){ re*=r; im*=r; return *this; }
  FXComplexd& operator/=(FXdouble r){ re/=r; im/=r; return *this; }

  /// Assigning operators with another complex
  FXComplexd& operator+=(const FXComplexd& c){ return set(re+c.re,im+c.im); }
  FXComplexd& operator-=(const FXComplexd& c){ return set(re-c.re,im-c.im); }
  FXComplexd& operator*=(const FXComplexd& c){ return set(re*c.re-im*c.im,re*c.im+im*c.re); }
  FXComplexd& operator/=(const FXComplexd& c){ FXdouble m=c.re*c.re+c.im*c.im; return set((re*c.re+im*c.im)/m,(im*c.re-re*c.im)/m); }

  /// Destructor
 ~FXComplexd(){}
  };


/// Return complex complex conjugate
inline FXComplexd conj(const FXComplexd& c){ return FXComplexd(c.real(),-c.imag()); }

/// Return complex number from modulus and argument
inline FXComplexd polar(FXdouble mod,FXdouble arg){ return FXComplexd(Math::cos(arg)*mod,Math::sin(arg)*mod); }

/// Return norm of complex
inline FXdouble norm(const FXComplexd& c){ return c.real()*c.real()+c.imag()*c.imag(); }

/// Return modulus or absolute value of complex
inline FXdouble abs(const FXComplexd& c){ return Math::sqrt(norm(c)); }

/// Return argument of complex
inline FXdouble arg(const FXComplexd& c){ return Math::atan2(c.imag(),c.real()); }

/// Returns the complex base e exponential of c
inline FXComplexd exp(const FXComplexd& c){ return polar(Math::exp(c.real()),c.imag()); }

/// Returns the complex base e logarithm of c
inline FXComplexd log(const FXComplexd& c){ return FXComplexd(Math::log(abs(c)),arg(c)); }


/// Equality between complex and real
inline FXbool operator==(const FXComplexd& c,FXdouble r){ return c.real()==r && c.imag()==0.0; }
inline FXbool operator!=(const FXComplexd& c,FXdouble r){ return c.real()!=r || c.imag()!=0.0; }

/// Equality between real and complex
inline FXbool operator==(FXdouble r,const FXComplexd& c){ return r==c.real() && c.imag()==0.0; }
inline FXbool operator!=(FXdouble r,const FXComplexd& c){ return r!=c.real() || c.imag()!=0.0; }

/// Equality between one complex and another
inline FXbool operator==(const FXComplexd& a,const FXComplexd& b){ return a.real()==b.real() && a.imag()==b.imag(); }
inline FXbool operator!=(const FXComplexd& a,const FXComplexd& b){ return a.real()!=b.real() || a.imag()!=b.imag(); }

/// Operators between complex and real
inline FXComplexd operator+(const FXComplexd& a,FXdouble b){ return FXComplexd(a.real()+b,a.imag()); }
inline FXComplexd operator-(const FXComplexd& a,FXdouble b){ return FXComplexd(a.real()-b,a.imag()); }
inline FXComplexd operator*(const FXComplexd& a,FXdouble b){ return FXComplexd(a.real()*b,a.imag()*b); }
inline FXComplexd operator/(const FXComplexd& a,FXdouble b){ return FXComplexd(a.real()/b,a.imag()/b); }

/// Operators between real and complex
inline FXComplexd operator+(FXdouble a,const FXComplexd& b){ return FXComplexd(a+b.real(),b.imag()); }
inline FXComplexd operator-(FXdouble a,const FXComplexd& b){ return FXComplexd(a-b.real(),-b.imag()); }
inline FXComplexd operator*(FXdouble a,const FXComplexd& b){ return FXComplexd(a*b.real(),a*b.imag()); }
inline FXComplexd operator/(FXdouble a,const FXComplexd& b){ FXdouble m=norm(b); return FXComplexd((a*b.real())/m,(-a*b.imag())/m); }

/// Operators between one complex and another
inline FXComplexd operator+(const FXComplexd& a,const FXComplexd& b){ return FXComplexd(a.real()+b.real(),a.imag()+b.imag()); }
inline FXComplexd operator-(const FXComplexd& a,const FXComplexd& b){ return FXComplexd(a.real()-b.real(),a.imag()-b.imag()); }
inline FXComplexd operator*(const FXComplexd& a,const FXComplexd& b){ return FXComplexd(a.real()*b.real()-a.imag()*b.imag(),a.real()*b.imag()+a.imag()*b.real()); }
inline FXComplexd operator/(const FXComplexd& a,const FXComplexd& b){ FXdouble m=norm(b); return FXComplexd((a.real()*b.real()+a.imag()*b.imag())/m,(a.imag()*b.real()-a.real()*b.imag())/m); }

/// Complex square root
extern FXAPI FXComplexd csqrt(const FXComplexd& c);

/// Complex sine
extern FXAPI FXComplexd csin(const FXComplexd& c);

/// Complex cosine
extern FXAPI FXComplexd ccos(const FXComplexd& c);

/// Complex square root
extern FXAPI FXComplexd ctan(const FXComplexd& c);

/// Complex hyperbolic sine
extern FXAPI FXComplexd csinh(const FXComplexd& c);

/// Complex hyperbolic cosine
extern FXAPI FXComplexd ccosh(const FXComplexd& c);

/// Complex hyperbolic tangent
extern FXAPI FXComplexd ctanh(const FXComplexd& c);

/// Save to a stream
extern FXAPI FXStream& operator<<(FXStream& store,const FXComplexd& c);

/// Load from a stream
extern FXAPI FXStream& operator>>(FXStream& store,FXComplexd& c);

}

#endif
