/********************************************************************************
*                                                                               *
*       S i n g l e - P r e c i s i o n   2 - E l e m e n t   V e c t o r       *
*                                                                               *
*********************************************************************************
* Copyright (C) 1994,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#ifndef FXVEC2F_H
#define FXVEC2F_H

namespace FX {


/// Single-precision 2-element vector
class FXAPI FXVec2f {
public:
  FXfloat x;
  FXfloat y;
public:

  /// Default constructor; value is not initialized
  FXVec2f(){}

  /// Initialize from another vector
  FXVec2f(const FXVec2f& v):x(v.x),y(v.y){}

  /// Initialize from array of floats
  FXVec2f(const FXfloat v[]):x(v[0]),y(v[1]){}

  /// Initialize from components
  FXVec2f(FXfloat xx,FXfloat yy):x(xx),y(yy){}

  /// Return a non-const reference to the ith element
  FXfloat& operator[](FXint i){return (&x)[i];}

  /// Return a const reference to the ith element
  const FXfloat& operator[](FXint i) const {return (&x)[i];}

  /// Assignment
  FXVec2f& operator=(const FXVec2f& v){x=v.x;y=v.y;return *this;}

  /// Assignment from array of floats
  FXVec2f& operator=(const FXfloat v[]){x=v[0];y=v[1];return *this;}

  /// Set value from another vector
  FXVec2f& set(const FXVec2f& v){x=v.x;y=v.y;return *this;}

  /// Set value from array of floats
  FXVec2f& set(const FXfloat v[]){x=v[0];y=v[1];return *this;}

  /// Set value from components
  FXVec2f& set(FXfloat xx,FXfloat yy){x=xx;y=yy;return *this;}

  /// Assigning operators
  FXVec2f& operator*=(FXfloat n){ return set(x*n,y*n); }
  FXVec2f& operator/=(FXfloat n){ return set(x/n,y/n); }

  /// Element-wise assigning operators
  FXVec2f& operator+=(const FXVec2f& v){ return set(x+v.x,y+v.y); }
  FXVec2f& operator-=(const FXVec2f& v){ return set(x-v.x,y-v.y); }
  FXVec2f& operator%=(const FXVec2f& v){ return set(x*v.x,y*v.y); }
  FXVec2f& operator/=(const FXVec2f& v){ return set(x/v.x,y/v.y); }

  /// Conversions
  operator FXfloat*(){return &x;}
  operator const FXfloat*() const {return &x;}

  /// Test if zero
  FXbool operator!() const { return x==0.0f && y==0.0f; }

  /// Unary
  FXVec2f operator+() const { return *this; }
  FXVec2f operator-() const { return FXVec2f(-x,-y); }

  /// Length and square of length
  FXfloat length2() const { return y*y+x*x; }
  FXfloat length() const { return Math::sqrt(length2()); }

  /// Destructor
 ~FXVec2f(){}
  };


/// Dot product
inline FXfloat operator*(const FXVec2f& a,const FXVec2f& b){ return a.x*b.x+a.y*b.y; }

/// Scaling
inline FXVec2f operator*(const FXVec2f& a,FXfloat n){return FXVec2f(a.x*n,a.y*n);}
inline FXVec2f operator*(FXfloat n,const FXVec2f& a){return FXVec2f(n*a.x,n*a.y);}
inline FXVec2f operator/(const FXVec2f& a,FXfloat n){return FXVec2f(a.x/n,a.y/n);}
inline FXVec2f operator/(FXfloat n,const FXVec2f& a){return FXVec2f(n/a.x,n/a.y);}

/// Vector and vector addition
inline FXVec2f operator+(const FXVec2f& a,const FXVec2f& b){ return FXVec2f(a.x+b.x,a.y+b.y); }
inline FXVec2f operator-(const FXVec2f& a,const FXVec2f& b){ return FXVec2f(a.x-b.x,a.y-b.y); }

/// Element-wise multiply and divide
inline FXVec2f operator%(const FXVec2f& a,const FXVec2f& b){ return FXVec2f(a.x*b.x,a.y*b.y); }
inline FXVec2f operator/(const FXVec2f& a,const FXVec2f& b){ return FXVec2f(a.x/b.x,a.y/b.y); }

/// Equality tests
inline FXbool operator==(const FXVec2f& a,FXfloat n){return a.x==n && a.y==n;}
inline FXbool operator!=(const FXVec2f& a,FXfloat n){return a.x!=n || a.y!=n;}
inline FXbool operator==(FXfloat n,const FXVec2f& a){return n==a.x && n==a.y;}
inline FXbool operator!=(FXfloat n,const FXVec2f& a){return n!=a.x || n!=a.y;}

/// Equality tests
inline FXbool operator==(const FXVec2f& a,const FXVec2f& b){ return a.x==b.x && a.y==b.y; }
inline FXbool operator!=(const FXVec2f& a,const FXVec2f& b){ return a.x!=b.x || a.y!=b.y; }

/// Inequality tests
inline FXbool operator<(const FXVec2f& a,FXfloat n){return a.x<n && a.y<n;}
inline FXbool operator<=(const FXVec2f& a,FXfloat n){return a.x<=n && a.y<=n;}
inline FXbool operator>(const FXVec2f& a,FXfloat n){return a.x>n && a.y>n;}
inline FXbool operator>=(const FXVec2f& a,FXfloat n){return a.x>=n && a.y>=n;}

/// Inequality tests
inline FXbool operator<(FXfloat n,const FXVec2f& a){return n<a.x && n<a.y;}
inline FXbool operator<=(FXfloat n,const FXVec2f& a){return n<=a.x && n<=a.y;}
inline FXbool operator>(FXfloat n,const FXVec2f& a){return n>a.x && n>a.y;}
inline FXbool operator>=(FXfloat n,const FXVec2f& a){return n>=a.x && n>=a.y;}

/// Inequality tests
inline FXbool operator<(const FXVec2f& a,const FXVec2f& b){ return a.x<b.x && a.y<b.y; }
inline FXbool operator<=(const FXVec2f& a,const FXVec2f& b){ return a.x<=b.x && a.y<=b.y; }
inline FXbool operator>(const FXVec2f& a,const FXVec2f& b){ return a.x>b.x && a.y>b.y; }
inline FXbool operator>=(const FXVec2f& a,const FXVec2f& b){ return a.x>=b.x && a.y>=b.y; }

/// Lowest components
inline FXVec2f lo(const FXVec2f& a,const FXVec2f& b){return FXVec2f(Math::fmin(a.x,b.x),Math::fmin(a.y,b.y));}
inline FXVec2f lo(const FXVec2f& a,FXfloat n){return FXVec2f(Math::fmin(a.x,n),Math::fmin(a.y,n));}
inline FXVec2f lo(FXfloat n,const FXVec2f& b){return FXVec2f(Math::fmin(n,b.x),Math::fmin(n,b.y));}

/// Highest components
inline FXVec2f hi(const FXVec2f& a,const FXVec2f& b){return FXVec2f(Math::fmax(a.x,b.x),Math::fmax(a.y,b.y));}
inline FXVec2f hi(const FXVec2f& a,FXfloat n){return FXVec2f(Math::fmax(a.x,n),Math::fmax(a.y,n));}
inline FXVec2f hi(FXfloat n,const FXVec2f& b){return FXVec2f(Math::fmax(n,b.x),Math::fmax(n,b.y));}

/// Clamp components of vector between lower and upper limits
inline FXVec2f clamp(FXfloat lower,const FXVec2f& x,FXfloat upper){return hi(lo(x,upper),lower);}

/// Clamp components of vector between lower corner and upper corner
inline FXVec2f clamp(const FXVec2f& lower,const FXVec2f& x,const FXVec2f& upper){return hi(lo(x,upper),lower);}

/// Return vector of absolute value of each element
inline FXVec2f abs(const FXVec2f& a){return FXVec2f(Math::fabs(a.x),Math::fabs(a.y));}

/// Return maximum component of vector
inline FXfloat max(const FXVec2f& a){ return Math::fmax(a.x,a.y); }

/// Return minimum component of vector
inline FXfloat min(const FXVec2f& a){ return Math::fmin(a.x,a.y); }

/// Linearly interpolate
inline FXVec2f lerp(const FXVec2f& u,const FXVec2f& v,FXfloat f){return (v-u)*f+u;}

/// Normalize vector
extern FXAPI FXVec2f normalize(const FXVec2f& v);

/// Save vector to a stream
extern FXAPI FXStream& operator<<(FXStream& store,const FXVec2f& v);

/// Load vector from a stream
extern FXAPI FXStream& operator>>(FXStream& store,FXVec2f& v);

}

#endif
