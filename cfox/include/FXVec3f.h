/********************************************************************************
*                                                                               *
*       S i n g l e - P r e c i s i o n   3 - E l e m e n t   V e c t o r       *
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
#ifndef FXVEC3F_H
#define FXVEC3F_H

namespace FX {


/// Single-precision 3-element vector
class FXAPI FXVec3f {
public:
  FXfloat x;
  FXfloat y;
  FXfloat z;
public:

  /// Default constructor; value is not initialized
  FXVec3f(){}

  /// Initialize from 2-vector
  FXVec3f(const FXVec2f& v,FXfloat s=0.0f):x(v.x),y(v.y),z(s){}

  /// Initialize from another vector
  FXVec3f(const FXVec3f& v):x(v.x),y(v.y),z(v.z){}

  /// Initialize from array of floats
  FXVec3f(const FXfloat v[]):x(v[0]),y(v[1]),z(v[2]){}

  /// Initialize from components
  FXVec3f(FXfloat xx,FXfloat yy,FXfloat zz):x(xx),y(yy),z(zz){}

  /// Return a non-const reference to the ith element
  FXfloat& operator[](FXint i){return (&x)[i];}

  /// Return a const reference to the ith element
  const FXfloat& operator[](FXint i) const {return (&x)[i];}

  /// Assignment
  FXVec3f& operator=(const FXVec3f& v){x=v.x;y=v.y;z=v.z;return *this;}

  /// Assignment from array of floats
  FXVec3f& operator=(const FXfloat v[]){x=v[0];y=v[1];z=v[2];return *this;}

  /// Set value from another vector
  FXVec3f& set(const FXVec3f& v){x=v.x;y=v.y;z=v.z;return *this;}

  /// Set value from array of floats
  FXVec3f& set(const FXfloat v[]){x=v[0];y=v[1];z=v[2];return *this;}

  /// Set value from components
  FXVec3f& set(FXfloat xx,FXfloat yy,FXfloat zz){x=xx;y=yy;z=zz;return *this;}

  /// Assigning operators
  FXVec3f& operator*=(FXfloat n){ return set(x*n,y*n,z*n); }
  FXVec3f& operator/=(FXfloat n){ return set(x/n,y/n,z/n); }

  /// Element-wise assigning operators
  FXVec3f& operator+=(const FXVec3f& v){ return set(x+v.x,y+v.y,z+v.z); }
  FXVec3f& operator-=(const FXVec3f& v){ return set(x-v.x,y-v.y,z-v.z); }
  FXVec3f& operator%=(const FXVec3f& v){ return set(x*v.x,y*v.y,z*v.z); }
  FXVec3f& operator/=(const FXVec3f& v){ return set(x/v.x,y/v.y,z/v.z); }

  /// Cross product assigning operator
  FXVec3f& operator^=(const FXVec3f& v){ return set(y*v.z-z*v.y,z*v.x-x*v.z,x*v.y-y*v.x); }

  /// Conversions
  operator FXfloat*(){return &x;}
  operator const FXfloat*() const {return &x;}
  operator FXVec2f&(){return *reinterpret_cast<FXVec2f*>(this);}
  operator const FXVec2f&() const {return *reinterpret_cast<const FXVec2f*>(this);}

  /// Test if zero
  FXbool operator!() const { return x==0.0f && y==0.0f && z==0.0f; }

  /// Unary
  FXVec3f operator+() const { return *this; }
  FXVec3f operator-() const { return FXVec3f(-x,-y,-z); }

  /// Length and square of length
  FXfloat length2() const { return z*z+y*y+x*x; }
  FXfloat length() const { return Math::sqrt(length2()); }

  /// Destructor
 ~FXVec3f(){}
  };


/// Dot product
inline FXfloat operator*(const FXVec3f& a,const FXVec3f& b){ return a.x*b.x+a.y*b.y+a.z*b.z; }

/// Cross product
inline FXVec3f operator^(const FXVec3f& a,const FXVec3f& b){ return FXVec3f(a.y*b.z-a.z*b.y, a.z*b.x-a.x*b.z, a.x*b.y-a.y*b.x); }

/// Scaling
inline FXVec3f operator*(const FXVec3f& a,FXfloat n){return FXVec3f(a.x*n,a.y*n,a.z*n);}
inline FXVec3f operator*(FXfloat n,const FXVec3f& a){return FXVec3f(n*a.x,n*a.y,n*a.z);}
inline FXVec3f operator/(const FXVec3f& a,FXfloat n){return FXVec3f(a.x/n,a.y/n,a.z/n);}
inline FXVec3f operator/(FXfloat n,const FXVec3f& a){return FXVec3f(n/a.x,n/a.y,n/a.z);}

/// Vector and vector addition
inline FXVec3f operator+(const FXVec3f& a,const FXVec3f& b){ return FXVec3f(a.x+b.x,a.y+b.y,a.z+b.z); }
inline FXVec3f operator-(const FXVec3f& a,const FXVec3f& b){ return FXVec3f(a.x-b.x,a.y-b.y,a.z-b.z); }

/// Element-wise multiply and divide
inline FXVec3f operator%(const FXVec3f& a,const FXVec3f& b){ return FXVec3f(a.x*b.x,a.y*b.y,a.z*b.z); }
inline FXVec3f operator/(const FXVec3f& a,const FXVec3f& b){ return FXVec3f(a.x/b.x,a.y/b.y,a.z/b.z); }

/// Equality tests
inline FXbool operator==(const FXVec3f& a,FXfloat n){return a.x==n && a.y==n && a.z==n;}
inline FXbool operator!=(const FXVec3f& a,FXfloat n){return a.x!=n || a.y!=n || a.z!=n;}
inline FXbool operator==(FXfloat n,const FXVec3f& a){return n==a.x && n==a.y && n==a.z;}
inline FXbool operator!=(FXfloat n,const FXVec3f& a){return n!=a.x || n!=a.y || n!=a.z;}

/// Equality tests
inline FXbool operator==(const FXVec3f& a,const FXVec3f& b){ return a.x==b.x && a.y==b.y && a.z==b.z; }
inline FXbool operator!=(const FXVec3f& a,const FXVec3f& b){ return a.x!=b.x || a.y!=b.y || a.z!=b.z; }

/// Inequality tests
inline FXbool operator<(const FXVec3f& a,FXfloat n){return a.x<n && a.y<n && a.z<n;}
inline FXbool operator<=(const FXVec3f& a,FXfloat n){return a.x<=n && a.y<=n && a.z<=n;}
inline FXbool operator>(const FXVec3f& a,FXfloat n){return a.x>n && a.y>n && a.z>n;}
inline FXbool operator>=(const FXVec3f& a,FXfloat n){return a.x>=n && a.y>=n && a.z>=n;}

/// Inequality tests
inline FXbool operator<(FXfloat n,const FXVec3f& a){return n<a.x && n<a.y && n<a.z;}
inline FXbool operator<=(FXfloat n,const FXVec3f& a){return n<=a.x && n<=a.y && n<=a.z;}
inline FXbool operator>(FXfloat n,const FXVec3f& a){return n>a.x && n>a.y && n>a.z;}
inline FXbool operator>=(FXfloat n,const FXVec3f& a){return n>=a.x && n>=a.y && n>=a.z;}

/// Inequality tests
inline FXbool operator<(const FXVec3f& a,const FXVec3f& b){ return a.x<b.x && a.y<b.y && a.z<b.z; }
inline FXbool operator<=(const FXVec3f& a,const FXVec3f& b){ return a.x<=b.x && a.y<=b.y && a.z<=b.z; }
inline FXbool operator>(const FXVec3f& a,const FXVec3f& b){ return a.x>b.x && a.y>b.y && a.z>b.z; }
inline FXbool operator>=(const FXVec3f& a,const FXVec3f& b){ return a.x>=b.x && a.y>=b.y && a.z>=b.z; }

/// Lowest components
inline FXVec3f lo(const FXVec3f& a,const FXVec3f& b){return FXVec3f(Math::fmin(a.x,b.x),Math::fmin(a.y,b.y),Math::fmin(a.z,b.z));}
inline FXVec3f lo(const FXVec3f& a,FXfloat n){return FXVec3f(Math::fmin(a.x,n),Math::fmin(a.y,n),Math::fmin(a.z,n));}
inline FXVec3f lo(FXfloat n,const FXVec3f& b){return FXVec3f(Math::fmin(n,b.x),Math::fmin(n,b.y),Math::fmin(n,b.z));}

/// Highest components
inline FXVec3f hi(const FXVec3f& a,const FXVec3f& b){return FXVec3f(Math::fmax(a.x,b.x),Math::fmax(a.y,b.y),Math::fmax(a.z,b.z));}
inline FXVec3f hi(const FXVec3f& a,FXfloat n){return FXVec3f(Math::fmax(a.x,n),Math::fmax(a.y,n),Math::fmax(a.z,n));}
inline FXVec3f hi(FXfloat n,const FXVec3f& b){return FXVec3f(Math::fmax(n,b.x),Math::fmax(n,b.y),Math::fmax(n,b.z));}

/// Clamp components of vector between lower and upper limits
inline FXVec3f clamp(FXfloat lower,const FXVec3f& x,FXfloat upper){return hi(lo(x,upper),lower);}

/// Clamp components of vector between lower corner and upper corner
inline FXVec3f clamp(const FXVec3f& lower,const FXVec3f& x,const FXVec3f& upper){return hi(lo(x,upper),lower);}

/// Return vector of absolute value of each element
inline FXVec3f abs(const FXVec3f& a){return FXVec3f(Math::fabs(a.x),Math::fabs(a.y),Math::fabs(a.z));}

/// Return maximum component of vector
inline FXfloat max(const FXVec3f& a){ return Math::fmax(Math::fmax(a.x,a.y),a.z); }

/// Return minimum component of vector
inline FXfloat min(const FXVec3f& a){ return Math::fmin(Math::fmin(a.x,a.y),a.z); }

/// Linearly interpolate
inline FXVec3f lerp(const FXVec3f& u,const FXVec3f& v,FXfloat f){return (v-u)*f+u;}

/// Convert vector to color
extern FXAPI FXColor colorFromVec3f(const FXVec3f& vec);

/// Convert color to vector
extern FXAPI FXVec3f colorToVec3f(FXColor clr);

/// Compute normal from three points a,b,c
extern FXAPI FXVec3f normal(const FXVec3f& a,const FXVec3f& b,const FXVec3f& c);

/// Compute approximate normal from four points a,b,c,d
extern FXAPI FXVec3f normal(const FXVec3f& a,const FXVec3f& b,const FXVec3f& c,const FXVec3f& d);

/// Normalize vector
extern FXAPI FXVec3f normalize(const FXVec3f& v);

/// Return vector orthogonal to v
extern FXAPI FXVec3f orthogonal(const FXVec3f& v);

/// Rotate vector vec by unit-length axis about angle specified as (ca,sa)
extern FXAPI FXVec3f rotate(const FXVec3f& vec,const FXVec3f& axis,FXfloat ca,FXfloat sa);

/// Rotate vector by unit-length axis about angle ang
extern FXAPI FXVec3f rotate(const FXVec3f& vector,const FXVec3f& axis,FXfloat ang);

/// Save vector to a stream
extern FXAPI FXStream& operator<<(FXStream& store,const FXVec3f& v);

/// Load vector from a stream
extern FXAPI FXStream& operator>>(FXStream& store,FXVec3f& v);

}

#endif
