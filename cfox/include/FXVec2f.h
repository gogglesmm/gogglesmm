/********************************************************************************
*                                                                               *
*       S i n g l e - P r e c i s i o n   2 - E l e m e n t   V e c t o r       *
*                                                                               *
*********************************************************************************
* Copyright (C) 1994,2017 by Jeroen van der Zijp.   All Rights Reserved.        *
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
  FXVec2f& operator+=(const FXVec2f& v){ return set(x+v.x,y+v.y); }
  FXVec2f& operator-=(const FXVec2f& v){ return set(x-v.x,y-v.y); }

  /// Conversions
  operator FXfloat*(){return &x;}
  operator const FXfloat*() const {return &x;}

  /// Test if zero
  FXbool operator!() const { return x==0.0f && y==0.0f; }

  /// Unary
  FXVec2f operator+() const { return *this; }
  FXVec2f operator-() const { return FXVec2f(-x,-y); }

  /// Length and square of length
  FXfloat length2() const { return x*x+y*y; }
  FXfloat length() const { return Math::sqrt(length2()); }

  /// Clamp values of vector between limits
  FXVec2f& clamp(FXfloat lo,FXfloat hi){ return set(Math::fclamp(lo,x,hi),Math::fclamp(lo,y,hi)); }

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

/// Lowest or highest components
inline FXVec2f lo(const FXVec2f& a,const FXVec2f& b){return FXVec2f(Math::fmin(a.x,b.x),Math::fmin(a.y,b.y));}
inline FXVec2f hi(const FXVec2f& a,const FXVec2f& b){return FXVec2f(Math::fmax(a.x,b.x),Math::fmax(a.y,b.y));}

/// Normalize vector
extern FXAPI FXVec2f normalize(const FXVec2f& v);

/// Linearly interpolate
extern FXAPI FXVec2f lerp(const FXVec2f& u,const FXVec2f& v,FXfloat f);

/// Save vector to a stream
extern FXAPI FXStream& operator<<(FXStream& store,const FXVec2f& v);

/// Load vector from a stream
extern FXAPI FXStream& operator>>(FXStream& store,FXVec2f& v);

}

#endif
