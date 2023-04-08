/********************************************************************************
*                                                                               *
*                             P o i n t    C l a s s                            *
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
#ifndef FXPOINT_H
#define FXPOINT_H

#ifndef FXSIZE_H
#include "FXSize.h"
#endif

namespace FX {


/// Point
class FXAPI FXPoint {
public:
  FXshort x;
  FXshort y;
public:

  /// Constructors
  FXPoint(){ }
  FXPoint(const FXSize& s):x(s.w),y(s.h){ }
  FXPoint(const FXPoint& p):x(p.x),y(p.y){ }
  FXPoint(FXshort xx,FXshort yy):x(xx),y(yy){ }

  /// Return a non-const reference to the ith element
  FXshort& operator[](FXint i){return (&x)[i];}

  /// Return a const reference to the ith element
  const FXshort& operator[](FXint i) const {return (&x)[i];}

  /// Assignment
  FXPoint& operator=(const FXPoint& p){ x=p.x; y=p.y; return *this; }

  /// Set value from another point
  FXPoint& set(const FXPoint& p){ x=p.x; y=p.y; return *this; }

  /// Set value from components
  FXPoint& set(FXshort xx,FXshort yy){ x=xx; y=yy; return *this; }

  /// Assignment operators
  FXPoint& operator*=(FXshort c){ x*=c; y*=c; return *this; }
  FXPoint& operator/=(FXshort c){ x/=c; y/=c; return *this; }
  FXPoint& operator+=(const FXPoint& p){ x+=p.x; y+=p.y; return *this; }
  FXPoint& operator-=(const FXPoint& p){ x-=p.x; y-=p.y; return *this; }

  /// Test if zero
  FXbool operator!() const { return x==0 && y==0; }

  /// Unary
  FXPoint operator+() const { return *this; }
  FXPoint operator-(){ return FXPoint(-x,-y); }
  };


/// Scale operators
inline FXPoint operator*(const FXPoint& p,FXshort c){ return FXPoint(p.x*c,p.y*c); }
inline FXPoint operator*(FXshort c,const FXPoint& p){ return FXPoint(c*p.x,c*p.y); }
inline FXPoint operator/(const FXPoint& p,FXshort c){ return FXPoint(p.x/c,p.y/c); }
inline FXPoint operator/(FXshort c,const FXPoint& p){ return FXPoint(c/p.x,c/p.y); }

/// Addition operators
inline FXPoint operator+(const FXPoint& a,const FXPoint& b){ return FXPoint(a.x+b.x,a.y+b.y); }
inline FXPoint operator-(const FXPoint& a,const FXPoint& b){ return FXPoint(a.x-b.x,a.y-b.y); }

/// Equality tests
inline FXbool operator==(const FXPoint& a,FXshort n){ return a.x==n && a.y==n; }
inline FXbool operator!=(const FXPoint& a,FXshort n){ return a.x!=n || a.y!=n; }
inline FXbool operator==(FXshort n,const FXPoint& a){ return n==a.x && n==a.y; }
inline FXbool operator!=(FXshort n,const FXPoint& a){ return n!=a.x || n!=a.y; }

/// Equality tests
inline FXbool operator==(const FXPoint& a,const FXPoint& b){ return a.x==b.x && a.y==b.y; }
inline FXbool operator!=(const FXPoint& a,const FXPoint& b){ return a.x!=b.x || a.y!=b.y; }

/// Inequality tests
inline FXbool operator<(const FXPoint& a,FXshort n){ return a.x<n && a.y<n; }
inline FXbool operator<=(const FXPoint& a,FXshort n){ return a.x<=n && a.y<=n; }
inline FXbool operator>(const FXPoint& a,FXshort n){ return a.x>n && a.y>n; }
inline FXbool operator>=(const FXPoint& a,FXshort n){ return a.x>=n && a.y>=n; }

/// Inequality tests
inline FXbool operator<(FXshort n,const FXPoint& a){ return n<a.x && n<a.y; }
inline FXbool operator<=(FXshort n,const FXPoint& a){ return n<=a.x && n<=a.y; }
inline FXbool operator>(FXshort n,const FXPoint& a){ return n>a.x && n>a.y; }
inline FXbool operator>=(FXshort n,const FXPoint& a){ return n>=a.x && n>=a.y; }

/// Inequality tests
inline FXbool operator<(const FXPoint& a,const FXPoint& b){ return a.x<b.x && a.y<b.y; }
inline FXbool operator<=(const FXPoint& a,const FXPoint& b){ return a.x<=b.x && a.y<=b.y; }
inline FXbool operator>(const FXPoint& a,const FXPoint& b){ return a.x>b.x && a.y>b.y; }
inline FXbool operator>=(const FXPoint& a,const FXPoint& b){ return a.x>=b.x && a.y>=b.y; }

/// Lowest or highest components
inline FXPoint lo(const FXPoint& a,const FXPoint& b){ return FXPoint(Math::imin(a.x,b.x),Math::imin(a.y,b.y)); }
inline FXPoint hi(const FXPoint& a,const FXPoint& b){ return FXPoint(Math::imax(a.x,b.x),Math::imax(a.y,b.y)); }

/// Save object to a stream
extern FXAPI FXStream& operator<<(FXStream& store,const FXPoint& p);

/// Load object from a stream
extern FXAPI FXStream& operator>>(FXStream& store,FXPoint& p);

}

#endif
