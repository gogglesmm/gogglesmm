/********************************************************************************
*                                                                               *
*       D o u b l e - P r e c i s i o n   2 - E l e m e n t   V e c t o r       *
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
#ifndef FXVEC2D_H
#define FXVEC2D_H

namespace FX {


/// Double-precision 2-element vector
class FXAPI FXVec2d {
public:
  FXdouble x;
  FXdouble y;
public:

  /// Default constructor; value is not initialized
  FXVec2d(){}

  /// Initialize from another vector
  FXVec2d(const FXVec2d& v):x(v.x),y(v.y){}

  /// Initialize from array of doubles
  FXVec2d(const FXdouble v[]):x(v[0]),y(v[1]){}

  /// Initialize from components
  FXVec2d(FXdouble xx,FXdouble yy):x(xx),y(yy){}

  /// Return a non-const reference to the ith element
  FXdouble& operator[](FXint i){return (&x)[i];}

  /// Return a const reference to the ith element
  const FXdouble& operator[](FXint i) const {return (&x)[i];}

  /// Assignment
  FXVec2d& operator=(const FXVec2d& v){x=v.x;y=v.y;return *this;}

  /// Assignment from array of doubles
  FXVec2d& operator=(const FXdouble v[]){x=v[0];y=v[1];return *this;}

  /// Set value from another vector
  FXVec2d& set(const FXVec2d& v){x=v.x;y=v.y;return *this;}

  /// Set value from array of doubles
  FXVec2d& set(const FXdouble v[]){x=v[0];y=v[1];return *this;}

  /// Set value from components
  FXVec2d& set(FXdouble xx,FXdouble yy){x=xx;y=yy;return *this;}

  /// Assigning operators
  FXVec2d& operator*=(FXdouble n){ return set(x*n,y*n); }
  FXVec2d& operator/=(FXdouble n){ return set(x/n,y/n); }
  FXVec2d& operator+=(const FXVec2d& v){ return set(x+v.x,y+v.y); }
  FXVec2d& operator-=(const FXVec2d& v){ return set(x-v.x,y-v.y); }

  /// Conversions
  operator FXdouble*(){return &x;}
  operator const FXdouble*() const {return &x;}

  /// Test if zero
  FXbool operator!() const { return x==0.0 && y==0.0;}

  /// Unary
  FXVec2d operator+() const { return *this; }
  FXVec2d operator-() const { return FXVec2d(-x,-y); }

  /// Length and square of length
  FXdouble length2() const { return x*x+y*y; }
  FXdouble length() const { return Math::sqrt(length2()); }

  /// Clamp values of vector between limits
  FXVec2d& clamp(FXdouble lo,FXdouble hi){ return set(Math::fclamp(lo,x,hi),Math::fclamp(lo,y,hi)); }

  /// Destructor
 ~FXVec2d(){}
  };


/// Dot product
inline FXdouble operator*(const FXVec2d& a,const FXVec2d& b){ return a.x*b.x+a.y*b.y; }

/// Scaling
inline FXVec2d operator*(const FXVec2d& a,FXdouble n){return FXVec2d(a.x*n,a.y*n);}
inline FXVec2d operator*(FXdouble n,const FXVec2d& a){return FXVec2d(n*a.x,n*a.y);}
inline FXVec2d operator/(const FXVec2d& a,FXdouble n){return FXVec2d(a.x/n,a.y/n);}
inline FXVec2d operator/(FXdouble n,const FXVec2d& a){return FXVec2d(n/a.x,n/a.y);}

/// Vector and vector addition
inline FXVec2d operator+(const FXVec2d& a,const FXVec2d& b){ return FXVec2d(a.x+b.x,a.y+b.y); }
inline FXVec2d operator-(const FXVec2d& a,const FXVec2d& b){ return FXVec2d(a.x-b.x,a.y-b.y); }

/// Equality tests
inline FXbool operator==(const FXVec2d& a,FXdouble n){return a.x==n && a.y==n;}
inline FXbool operator!=(const FXVec2d& a,FXdouble n){return a.x!=n || a.y!=n;}
inline FXbool operator==(FXdouble n,const FXVec2d& a){return n==a.x && n==a.y;}
inline FXbool operator!=(FXdouble n,const FXVec2d& a){return n!=a.x || n!=a.y;}

/// Equality tests
inline FXbool operator==(const FXVec2d& a,const FXVec2d& b){ return a.x==b.x && a.y==b.y; }
inline FXbool operator!=(const FXVec2d& a,const FXVec2d& b){ return a.x!=b.x || a.y!=b.y; }

/// Inequality tests
inline FXbool operator<(const FXVec2d& a,FXdouble n){return a.x<n && a.y<n;}
inline FXbool operator<=(const FXVec2d& a,FXdouble n){return a.x<=n && a.y<=n;}
inline FXbool operator>(const FXVec2d& a,FXdouble n){return a.x>n && a.y>n;}
inline FXbool operator>=(const FXVec2d& a,FXdouble n){return a.x>=n && a.y>=n;}

/// Inequality tests
inline FXbool operator<(FXdouble n,const FXVec2d& a){return n<a.x && n<a.y;}
inline FXbool operator<=(FXdouble n,const FXVec2d& a){return n<=a.x && n<=a.y;}
inline FXbool operator>(FXdouble n,const FXVec2d& a){return n>a.x && n>a.y;}
inline FXbool operator>=(FXdouble n,const FXVec2d& a){return n>=a.x && n>=a.y;}

/// Inequality tests
inline FXbool operator<(const FXVec2d& a,const FXVec2d& b){ return a.x<b.x && a.y<b.y; }
inline FXbool operator<=(const FXVec2d& a,const FXVec2d& b){ return a.x<=b.x && a.y<=b.y; }
inline FXbool operator>(const FXVec2d& a,const FXVec2d& b){ return a.x>b.x && a.y>b.y; }
inline FXbool operator>=(const FXVec2d& a,const FXVec2d& b){ return a.x>=b.x && a.y>=b.y; }

/// Lowest or highest components
inline FXVec2d lo(const FXVec2d& a,const FXVec2d& b){return FXVec2d(Math::fmin(a.x,b.x),Math::fmin(a.y,b.y));}
inline FXVec2d hi(const FXVec2d& a,const FXVec2d& b){return FXVec2d(Math::fmax(a.x,b.x),Math::fmax(a.y,b.y));}

/// Normalize vector
extern FXAPI FXVec2d normalize(const FXVec2d& v);

/// Linearly interpolate
extern FXAPI FXVec2d lerp(const FXVec2d& u,const FXVec2d& v,FXdouble f);

/// Save vector to a stream
extern FXAPI FXStream& operator<<(FXStream& store,const FXVec2d& v);

/// Load vector from a stream
extern FXAPI FXStream& operator>>(FXStream& store,FXVec2d& v);

}

#endif
