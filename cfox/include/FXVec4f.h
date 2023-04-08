/********************************************************************************
*                                                                               *
*       S i n g l e - P r e c i s i o n   4 - E l e m e n t   V e c t o r       *
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
#ifndef FXVEC4F_H
#define FXVEC4F_H

namespace FX {


/// Single-precision 4-element vector
class FXAPI FXVec4f {
public:
  FXfloat x;
  FXfloat y;
  FXfloat z;
  FXfloat w;
public:

  /// Default constructor; value is not initialized
  FXVec4f(){}

  /// Construct with 3-vector
  FXVec4f(const FXVec3f& v,FXfloat s=0.0f):x(v.x),y(v.y),z(v.z),w(s){}

  /// Initialize from another vector
  FXVec4f(const FXVec4f& v):x(v.x),y(v.y),z(v.z),w(v.w){}

  /// Construct from array of floats
  FXVec4f(const FXfloat v[]):x(v[0]),y(v[1]),z(v[2]),w(v[3]){}

  /// Construct from components
  FXVec4f(FXfloat xx,FXfloat yy,FXfloat zz,FXfloat ww):x(xx),y(yy),z(zz),w(ww){}

  /// Return a non-const reference to the ith element
  FXfloat& operator[](FXint i){return (&x)[i];}

  /// Return a const reference to the ith element
  const FXfloat& operator[](FXint i) const {return (&x)[i];}

  /// Assignment
  FXVec4f& operator=(const FXVec4f& v){x=v.x;y=v.y;z=v.z;w=v.w;return *this;}

  /// Assignment from array of floats
  FXVec4f& operator=(const FXfloat v[]){x=v[0];y=v[1];z=v[2];w=v[3];return *this;}

  /// Set value from another vector
  FXVec4f& set(const FXVec4f& v){x=v.x;y=v.y;z=v.z;w=v.w;return *this;}

  /// Set value from array of floats
  FXVec4f& set(const FXfloat v[]){x=v[0];y=v[1];z=v[2];w=v[3];return *this;}

  /// Set value from components
  FXVec4f& set(FXfloat xx,FXfloat yy,FXfloat zz,FXfloat ww){x=xx;y=yy;z=zz;w=ww;return *this;}

  /// Assigning operators
  FXVec4f& operator*=(FXfloat n){ return set(x*n,y*n,z*n,w*n); }
  FXVec4f& operator/=(FXfloat n){ return set(x/n,y/n,z/n,w/n); }

  /// Element-wise assigning operators
  FXVec4f& operator+=(const FXVec4f& v){ return set(x+v.x,y+v.y,z+v.z,w+v.w); }
  FXVec4f& operator-=(const FXVec4f& v){ return set(x-v.x,y-v.y,z-v.z,w-v.w); }
  FXVec4f& operator%=(const FXVec4f& v){ return set(x*v.x,y*v.y,z*v.z,w*v.w); }
  FXVec4f& operator/=(const FXVec4f& v){ return set(x/v.x,y/v.y,z/v.z,w/v.w); }

  /// Conversion
  operator FXfloat*(){return &x;}
  operator const FXfloat*() const {return &x;}
  operator FXVec3f&(){return *reinterpret_cast<FXVec3f*>(this);}
  operator const FXVec3f&() const {return *reinterpret_cast<const FXVec3f*>(this);}

  /// Test if zero
  FXbool operator!() const { return x==0.0f && y==0.0f && z==0.0f && w==0.0f; }

  /// Unary
  FXVec4f operator+() const { return *this; }
  FXVec4f operator-() const { return FXVec4f(-x,-y,-z,-w); }

  /// Length and square of length
  FXfloat length2() const { return w*w+z*z+y*y+x*x; }
  FXfloat length() const { return Math::sqrt(length2()); }

  /// Signed distance normalized plane and point
  FXfloat distance(const FXVec3f& p) const;

  /// Return true if edge a-b crosses plane
  FXbool crosses(const FXVec3f& a,const FXVec3f& b) const;

  /// Destructor
 ~FXVec4f(){}
  };


/// Dot product
inline FXfloat operator*(const FXVec4f& a,const FXVec4f& b){ return a.x*b.x+a.y*b.y+a.z*b.z+a.w*b.w; }

/// Scaling
inline FXVec4f operator*(const FXVec4f& a,FXfloat n){return FXVec4f(a.x*n,a.y*n,a.z*n,a.w*n);}
inline FXVec4f operator*(FXfloat n,const FXVec4f& a){return FXVec4f(n*a.x,n*a.y,n*a.z,n*a.w);}
inline FXVec4f operator/(const FXVec4f& a,FXfloat n){return FXVec4f(a.x/n,a.y/n,a.z/n,a.w/n);}
inline FXVec4f operator/(FXfloat n,const FXVec4f& a){return FXVec4f(n/a.x,n/a.y,n/a.z,n/a.w);}

/// Vector and vector addition
inline FXVec4f operator+(const FXVec4f& a,const FXVec4f& b){ return FXVec4f(a.x+b.x,a.y+b.y,a.z+b.z,a.w+b.w); }
inline FXVec4f operator-(const FXVec4f& a,const FXVec4f& b){ return FXVec4f(a.x-b.x,a.y-b.y,a.z-b.z,a.w-b.w); }

/// Element-wise multiply and divide
inline FXVec4f operator%(const FXVec4f& a,const FXVec4f& b){ return FXVec4f(a.x*b.x,a.y*b.y,a.z*b.z,a.w*b.w); }
inline FXVec4f operator/(const FXVec4f& a,const FXVec4f& b){ return FXVec4f(a.x/b.x,a.y/b.y,a.z/b.z,a.w/b.w); }

/// Equality tests
inline FXbool operator==(const FXVec4f& a,FXfloat n){return a.x==n && a.y==n && a.z==n && a.w==n;}
inline FXbool operator!=(const FXVec4f& a,FXfloat n){return a.x!=n || a.y!=n || a.z!=n || a.w!=n;}
inline FXbool operator==(FXfloat n,const FXVec4f& a){return n==a.x && n==a.y && n==a.z && n==a.w;}
inline FXbool operator!=(FXfloat n,const FXVec4f& a){return n!=a.x || n!=a.y || n!=a.z || n!=a.w;}

/// Equality tests
inline FXbool operator==(const FXVec4f& a,const FXVec4f& b){ return a.x==b.x && a.y==b.y && a.z==b.z && a.w==b.w; }
inline FXbool operator!=(const FXVec4f& a,const FXVec4f& b){ return a.x!=b.x || a.y!=b.y || a.z!=b.z || a.w!=b.w; }

/// Inequality tests
inline FXbool operator<(const FXVec4f& a,FXfloat n){return a.x<n && a.y<n && a.z<n && a.w<n;}
inline FXbool operator<=(const FXVec4f& a,FXfloat n){return a.x<=n && a.y<=n && a.z<=n && a.w<=n;}
inline FXbool operator>(const FXVec4f& a,FXfloat n){return a.x>n && a.y>n && a.z>n && a.w>n;}
inline FXbool operator>=(const FXVec4f& a,FXfloat n){return a.x>=n && a.y>=n && a.z>=n && a.w>=n;}

/// Inequality tests
inline FXbool operator<(FXfloat n,const FXVec4f& a){return n<a.x && n<a.y && n<a.z && n<a.w;}
inline FXbool operator<=(FXfloat n,const FXVec4f& a){return n<=a.x && n<=a.y && n<=a.z && n<=a.w;}
inline FXbool operator>(FXfloat n,const FXVec4f& a){return n>a.x && n>a.y && n>a.z && n>a.w;}
inline FXbool operator>=(FXfloat n,const FXVec4f& a){return n>=a.x && n>=a.y && n>=a.z && n>=a.w;}

/// Inequality tests
inline FXbool operator<(const FXVec4f& a,const FXVec4f& b){ return a.x<b.x && a.y<b.y && a.z<b.z && a.w<b.w; }
inline FXbool operator<=(const FXVec4f& a,const FXVec4f& b){ return a.x<=b.x && a.y<=b.y && a.z<=b.z && a.w<=b.w; }
inline FXbool operator>(const FXVec4f& a,const FXVec4f& b){ return a.x>b.x && a.y>b.y && a.z>b.z && a.w>b.w; }
inline FXbool operator>=(const FXVec4f& a,const FXVec4f& b){ return a.x>=b.x && a.y>=b.y && a.z>=b.z && a.w>=b.w; }

/// Lowest components
inline FXVec4f lo(const FXVec4f& a,const FXVec4f& b){return FXVec4f(Math::fmin(a.x,b.x),Math::fmin(a.y,b.y),Math::fmin(a.z,b.z),Math::fmin(a.w,b.w));}
inline FXVec4f lo(const FXVec4f& a,FXfloat n){return FXVec4f(Math::fmin(a.x,n),Math::fmin(a.y,n),Math::fmin(a.z,n),Math::fmin(a.w,n));}
inline FXVec4f lo(FXfloat n,const FXVec4f& b){return FXVec4f(Math::fmin(n,b.x),Math::fmin(n,b.y),Math::fmin(n,b.z),Math::fmin(n,b.w));}

/// Highest components
inline FXVec4f hi(const FXVec4f& a,const FXVec4f& b){return FXVec4f(Math::fmax(a.x,b.x),Math::fmax(a.y,b.y),Math::fmax(a.z,b.z),Math::fmax(a.w,b.w));}
inline FXVec4f hi(const FXVec4f& a,FXfloat n){return FXVec4f(Math::fmax(a.x,n),Math::fmax(a.y,n),Math::fmax(a.z,n),Math::fmax(a.w,n));}
inline FXVec4f hi(FXfloat n,const FXVec4f& b){return FXVec4f(Math::fmax(n,b.x),Math::fmax(n,b.y),Math::fmax(n,b.z),Math::fmax(n,b.w));}

/// Clamp components of vector between lower and upper limits
inline FXVec4f clamp(FXfloat lower,const FXVec4f& x,FXfloat upper){return hi(lo(x,upper),lower);}

/// Clamp components of vector between lower corner and upper corner
inline FXVec4f clamp(const FXVec4f& lower,const FXVec4f& x,const FXVec4f& upper){return hi(lo(x,upper),lower);}

/// Return vector of absolute value of each element
inline FXVec4f abs(const FXVec4f& a){return FXVec4f(Math::fabs(a.x),Math::fabs(a.y),Math::fabs(a.z),Math::fabs(a.w));}

/// Return maximum component of vector
inline FXfloat max(const FXVec4f& a){ return Math::fmax(Math::fmax(a.x,a.y),Math::fmax(a.z,a.w)); }

/// Return minimum component of vector
inline FXfloat min(const FXVec4f& a){ return Math::fmin(Math::fmin(a.x,a.y),Math::fmin(a.z,a.w)); }

/// Linearly interpolate
inline FXVec4f lerp(const FXVec4f& u,const FXVec4f& v,FXfloat f){return (v-u)*f+u;}

/// Compute normalized plane equation ax+by+cz+d=0
extern FXAPI FXVec4f plane(const FXVec4f& vec);

/// Compute plane equation from vector and distance
extern FXAPI FXVec4f plane(const FXVec3f& vec,FXfloat dist);

/// Compute plane equation from vector and point on plane
extern FXAPI FXVec4f plane(const FXVec3f& vec,const FXVec3f& p);

/// Compute plane equation from 3 points a,b,c
extern FXAPI FXVec4f plane(const FXVec3f& a,const FXVec3f& b,const FXVec3f& c);

/// Convert vector to color
extern FXAPI FXColor colorFromVec4f(const FXVec4f& vec);

/// Convert color to vector
extern FXAPI FXVec4f colorToVec4f(FXColor clr);

/// Normalize vector
extern FXAPI FXVec4f normalize(const FXVec4f& v);

/// Save vector to a stream
extern FXAPI FXStream& operator<<(FXStream& store,const FXVec4f& v);

/// Load vector from a stream
extern FXAPI FXStream& operator>>(FXStream& store,FXVec4f& v);

}

#endif
