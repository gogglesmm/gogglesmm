/********************************************************************************
*                                                                               *
*       D o u b l e - P r e c i s i o n   4 - E l e m e n t   V e c t o r       *
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
#ifndef FXVEC4D_H
#define FXVEC4D_H

namespace FX {


/// Double-precision 4-element vector
class FXAPI FXVec4d {
public:
  FXdouble x;
  FXdouble y;
  FXdouble z;
  FXdouble w;
public:

  /// Default constructor; value is not initialized
  FXVec4d(){}

  /// Construct with 3-vector
  FXVec4d(const FXVec3d& v,FXdouble s=0.0):x(v.x),y(v.y),z(v.z),w(s){}

  /// Initialize from another vector
  FXVec4d(const FXVec4d& v):x(v.x),y(v.y),z(v.z),w(v.w){}

  /// Initialize from array of doubles
  FXVec4d(const FXdouble v[]):x(v[0]),y(v[1]),z(v[2]),w(v[3]){}

  /// Initialize with components
  FXVec4d(FXdouble xx,FXdouble yy,FXdouble zz,FXdouble ww):x(xx),y(yy),z(zz),w(ww){}

  /// Return a non-const reference to the ith element
  FXdouble& operator[](FXint i){return (&x)[i];}

  /// Return a const reference to the ith element
  const FXdouble& operator[](FXint i) const {return (&x)[i];}

  /// Assignment
  FXVec4d& operator=(const FXVec4d& v){x=v.x;y=v.y;z=v.z;w=v.w;return *this;}

  /// Assignment from array of doubles
  FXVec4d& operator=(const FXdouble v[]){x=v[0];y=v[1];z=v[2];w=v[3];return *this;}

  /// Set value from another vector
  FXVec4d& set(const FXVec4d& v){x=v.x;y=v.y;z=v.z;w=v.w;return *this;}

  /// Set value from array of doubles
  FXVec4d& set(const FXdouble v[]){x=v[0];y=v[1];z=v[2];w=v[3];return *this;}

  /// Set value from components
  FXVec4d& set(FXdouble xx,FXdouble yy,FXdouble zz,FXdouble ww){x=xx;y=yy;z=zz;w=ww;return *this;}

  /// Assigning operators
  FXVec4d& operator*=(FXdouble n){ return set(x*n,y*n,z*n,w*n); }
  FXVec4d& operator/=(FXdouble n){ return set(x/n,y/n,z/n,w/n); }

  /// Element-wise assigning operators
  FXVec4d& operator+=(const FXVec4d& v){ return set(x+v.x,y+v.y,z+v.z,w+v.w); }
  FXVec4d& operator-=(const FXVec4d& v){ return set(x-v.x,y-v.y,z-v.z,w-v.w); }
  FXVec4d& operator%=(const FXVec4d& v){ return set(x*v.x,y*v.y,z*v.z,w*v.w); }
  FXVec4d& operator/=(const FXVec4d& v){ return set(x/v.x,y/v.y,z/v.z,w/v.w); }

  /// Conversion
  operator FXdouble*(){return &x;}
  operator const FXdouble*() const {return &x;}
  operator FXVec3d&(){return *reinterpret_cast<FXVec3d*>(this);}
  operator const FXVec3d&() const {return *reinterpret_cast<const FXVec3d*>(this);}

  /// Test if zero
  FXbool operator!() const {return x==0.0 && y==0.0 && z==0.0 && w==0.0; }

  /// Unary
  FXVec4d operator+() const { return *this; }
  FXVec4d operator-() const { return FXVec4d(-x,-y,-z,-w); }

  /// Length and square of length
  FXdouble length2() const { return w*w+z*z+y*y+x*x; }
  FXdouble length() const { return Math::sqrt(length2()); }

  /// Signed distance normalized plane and point
  FXdouble distance(const FXVec3d& p) const;

  /// Return true if edge a-b crosses plane
  FXbool crosses(const FXVec3d& a,const FXVec3d& b) const;

  /// Destructor
 ~FXVec4d(){}
  };


/// Dot product
inline FXdouble operator*(const FXVec4d& a,const FXVec4d& b){ return a.x*b.x+a.y*b.y+a.z*b.z+a.w*b.w; }

/// Scaling
inline FXVec4d operator*(const FXVec4d& a,FXdouble n){return FXVec4d(a.x*n,a.y*n,a.z*n,a.w*n);}
inline FXVec4d operator*(FXdouble n,const FXVec4d& a){return FXVec4d(n*a.x,n*a.y,n*a.z,n*a.w);}
inline FXVec4d operator/(const FXVec4d& a,FXdouble n){return FXVec4d(a.x/n,a.y/n,a.z/n,a.w/n);}
inline FXVec4d operator/(FXdouble n,const FXVec4d& a){return FXVec4d(n/a.x,n/a.y,n/a.z,n/a.w);}

/// Vector and vector addition
inline FXVec4d operator+(const FXVec4d& a,const FXVec4d& b){ return FXVec4d(a.x+b.x,a.y+b.y,a.z+b.z,a.w+b.w); }
inline FXVec4d operator-(const FXVec4d& a,const FXVec4d& b){ return FXVec4d(a.x-b.x,a.y-b.y,a.z-b.z,a.w-b.w); }

/// Element-wise multiply and divide
inline FXVec4d operator%(const FXVec4d& a,const FXVec4d& b){ return FXVec4d(a.x*b.x,a.y*b.y,a.z*b.z,a.w*b.w); }
inline FXVec4d operator/(const FXVec4d& a,const FXVec4d& b){ return FXVec4d(a.x/b.x,a.y/b.y,a.z/b.z,a.w/b.w); }

/// Equality tests
inline FXbool operator==(const FXVec4d& a,FXdouble n){return a.x==n && a.y==n && a.z==n && a.w==n;}
inline FXbool operator!=(const FXVec4d& a,FXdouble n){return a.x!=n || a.y!=n || a.z!=n || a.w!=n;}
inline FXbool operator==(FXdouble n,const FXVec4d& a){return n==a.x && n==a.y && n==a.z && n==a.w;}
inline FXbool operator!=(FXdouble n,const FXVec4d& a){return n!=a.x || n!=a.y || n!=a.z || n!=a.w;}

/// Equality tests
inline FXbool operator==(const FXVec4d& a,const FXVec4d& b){ return a.x==b.x && a.y==b.y && a.z==b.z && a.w==b.w; }
inline FXbool operator!=(const FXVec4d& a,const FXVec4d& b){ return a.x!=b.x || a.y!=b.y || a.z!=b.z || a.w!=b.w; }

/// Inequality tests
inline FXbool operator<(const FXVec4d& a,FXdouble n){return a.x<n && a.y<n && a.z<n && a.w<n;}
inline FXbool operator<=(const FXVec4d& a,FXdouble n){return a.x<=n && a.y<=n && a.z<=n && a.w<=n;}
inline FXbool operator>(const FXVec4d& a,FXdouble n){return a.x>n && a.y>n && a.z>n && a.w>n;}
inline FXbool operator>=(const FXVec4d& a,FXdouble n){return a.x>=n && a.y>=n && a.z>=n && a.w>=n;}

/// Inequality tests
inline FXbool operator<(FXdouble n,const FXVec4d& a){return n<a.x && n<a.y && n<a.z && n<a.w;}
inline FXbool operator<=(FXdouble n,const FXVec4d& a){return n<=a.x && n<=a.y && n<=a.z && n<=a.w;}
inline FXbool operator>(FXdouble n,const FXVec4d& a){return n>a.x && n>a.y && n>a.z && n>a.w;}
inline FXbool operator>=(FXdouble n,const FXVec4d& a){return n>=a.x && n>=a.y && n>=a.z && n>=a.w;}

/// Inequality tests
inline FXbool operator<(const FXVec4d& a,const FXVec4d& b){ return a.x<b.x && a.y<b.y && a.z<b.z && a.w<b.w; }
inline FXbool operator<=(const FXVec4d& a,const FXVec4d& b){ return a.x<=b.x && a.y<=b.y && a.z<=b.z && a.w<=b.w; }
inline FXbool operator>(const FXVec4d& a,const FXVec4d& b){ return a.x>b.x && a.y>b.y && a.z>b.z && a.w>b.w; }
inline FXbool operator>=(const FXVec4d& a,const FXVec4d& b){ return a.x>=b.x && a.y>=b.y && a.z>=b.z && a.w>=b.w; }

/// Lowest components
inline FXVec4d lo(const FXVec4d& a,const FXVec4d& b){return FXVec4d(Math::fmin(a.x,b.x),Math::fmin(a.y,b.y),Math::fmin(a.z,b.z),Math::fmin(a.w,b.w));}
inline FXVec4d lo(const FXVec4d& a,FXdouble n){return FXVec4d(Math::fmin(a.x,n),Math::fmin(a.y,n),Math::fmin(a.z,n),Math::fmin(a.w,n));}
inline FXVec4d lo(FXdouble n,const FXVec4d& b){return FXVec4d(Math::fmin(n,b.x),Math::fmin(n,b.y),Math::fmin(n,b.z),Math::fmin(n,b.w));}

/// Highest components
inline FXVec4d hi(const FXVec4d& a,const FXVec4d& b){return FXVec4d(Math::fmax(a.x,b.x),Math::fmax(a.y,b.y),Math::fmax(a.z,b.z),Math::fmax(a.w,b.w));}
inline FXVec4d hi(const FXVec4d& a,FXdouble n){return FXVec4d(Math::fmax(a.x,n),Math::fmax(a.y,n),Math::fmax(a.z,n),Math::fmax(a.w,n));}
inline FXVec4d hi(FXdouble n,const FXVec4d& b){return FXVec4d(Math::fmax(n,b.x),Math::fmax(n,b.y),Math::fmax(n,b.z),Math::fmax(n,b.w));}

/// Clamp components of vector between lower and upper limits
inline FXVec4d clamp(FXdouble lower,const FXVec4d& x,FXdouble upper){return hi(lo(x,upper),lower);}

/// Clamp components of vector between lower corner and upper corner
inline FXVec4d clamp(const FXVec4d& lower,const FXVec4d& x,const FXVec4d& upper){return hi(lo(x,upper),lower);}

/// Return vector of absolute value of each element
inline FXVec4d abs(const FXVec4d& a){return FXVec4d(Math::fabs(a.x),Math::fabs(a.y),Math::fabs(a.z),Math::fabs(a.w));}

/// Return maximum component of vector
inline FXdouble max(const FXVec4d& a){ return Math::fmax(Math::fmax(a.x,a.y),Math::fmax(a.z,a.w)); }

/// Return minimum component of vector
inline FXdouble min(const FXVec4d& a){ return Math::fmin(Math::fmin(a.x,a.y),Math::fmin(a.z,a.w)); }

/// Linearly interpolate
inline FXVec4d lerp(const FXVec4d& u,const FXVec4d& v,FXdouble f){return (v-u)*f+u;}

/// Compute normalized plane equation ax+by+cz+d=0
extern FXAPI FXVec4d plane(const FXVec4d& vec);

/// Compute plane equation from vector and distance
extern FXAPI FXVec4d plane(const FXVec3d& vec,FXdouble dist);

/// Compute plane equation from vector and point on plane
extern FXAPI FXVec4d plane(const FXVec3d& vec,const FXVec3d& p);

/// Compute plane equation from 3 points a,b,c
extern FXAPI FXVec4d plane(const FXVec3d& a,const FXVec3d& b,const FXVec3d& c);

/// Convert vector to color
extern FXAPI FXColor colorFromVec4d(const FXVec4d& vec);

/// Convert color to vector
extern FXAPI FXVec4d colorToVec4d(FXColor clr);

/// Normalize vector
extern FXAPI FXVec4d normalize(const FXVec4d& v);

/// Save vector to a stream
extern FXAPI FXStream& operator<<(FXStream& store,const FXVec4d& v);

/// Load vector from a stream
extern FXAPI FXStream& operator>>(FXStream& store,FXVec4d& v);

}

#endif
