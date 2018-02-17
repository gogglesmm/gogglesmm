/********************************************************************************
*                                                                               *
*              D o u b l e - P r e c i s i o n  Q u a t e r n i o n             *
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
#ifndef FXQUATD_H
#define FXQUATD_H

namespace FX {


/// Double-precision quaternion
class FXAPI FXQuatd {
public:
  double x;
  double y;
  double z;
  double w;
public:

  /// Default constructor; value is not initialized
  FXQuatd(){}

  /// Copy constructor
  FXQuatd(const FXQuatd& q):x(q.x),y(q.y),z(q.z),w(q.w){}

  /// Construct from array of doubles
  FXQuatd(const FXdouble v[]):x(v[0]),y(v[1]),z(v[2]),w(v[3]){}

  /// Construct from components
  FXQuatd(FXdouble xx,FXdouble yy,FXdouble zz,FXdouble ww):x(xx),y(yy),z(zz),w(ww){}

  /// Construct quaternion from two unit vectors
  FXQuatd(const FXVec3d& fr,const FXVec3d& to);

  /// Construct from axis and angle
  FXQuatd(const FXVec3d& axis,FXdouble phi=0.0);

  /// Construct from euler angles yaw (z), pitch (y), and roll (x)
  FXQuatd(FXdouble roll,FXdouble pitch,FXdouble yaw);

  /// Construct quaternion from three axes
  FXQuatd(const FXVec3d& ex,const FXVec3d& ey,const FXVec3d& ez);

  /// Return a non-const reference to the ith element
  FXdouble& operator[](FXint i){return (&x)[i];}

  /// Return a const reference to the ith element
  const FXdouble& operator[](FXint i) const {return (&x)[i];}

  /// Assignment
  FXQuatd& operator=(const FXQuatd& v){x=v.x;y=v.y;z=v.z;w=v.w;return *this;}

  /// Assignment from array of doubles
  FXQuatd& operator=(const FXdouble v[]){x=v[0];y=v[1];z=v[2];w=v[3];return *this;}

  /// Set value from another vector
  FXQuatd& set(const FXQuatd& v){x=v.x;y=v.y;z=v.z;w=v.w;return *this;}

  /// Set value from array of floats
  FXQuatd& set(const FXdouble v[]){x=v[0];y=v[1];z=v[2];w=v[3];return *this;}

  /// Set value from components
  FXQuatd& set(FXdouble xx,FXdouble yy,FXdouble zz,FXdouble ww){x=xx;y=yy;z=zz;w=ww;return *this;}

  /// Assigning operators
  FXQuatd& operator*=(FXdouble n){ return set(x*n,y*n,z*n,w*n); }
  FXQuatd& operator/=(FXdouble n){ return set(x/n,y/n,z/n,w/n); }
  FXQuatd& operator+=(const FXQuatd& v){ return set(x+v.x,y+v.y,z+v.z,w+v.w); }
  FXQuatd& operator-=(const FXQuatd& v){ return set(x-v.x,y-v.y,z-v.z,w-v.w); }
  FXQuatd& operator*=(const FXQuatd& b){ return set(w*b.x+x*b.w+y*b.z-z*b.y, w*b.y+y*b.w+z*b.x-x*b.z, w*b.z+z*b.w+x*b.y-y*b.x, w*b.w-x*b.x-y*b.y-z*b.z); }
  FXQuatd& operator/=(const FXQuatd& b){ return *this*=b.invert(); }

  /// Conversion
  operator FXdouble*(){return &x;}
  operator const FXdouble*() const {return &x;}

  /// Test if zero
  FXbool operator!() const {return x==0.0 && y==0.0 && z==0.0 && w==0.0; }

  /// Unary
  FXQuatd operator+() const { return *this; }
  FXQuatd operator-() const { return FXQuatd(-x,-y,-z,-w); }

  /// Rotation of a vector by a quaternion
  FXVec3d operator*(const FXVec3d& v) const;

  /// Length and square of length
  FXdouble length2() const { return x*x+y*y+z*z+w*w; }
  FXdouble length() const { return Math::sqrt(length2()); }

  /// Adjust quaternion length
  FXQuatd& adjust();

  /// Set quaternion from axis and angle
  void setAxisAngle(const FXVec3d& axis,FXdouble phi=0.0);

  /// Obtain axis and angle from quaternion
  void getAxisAngle(FXVec3d& axis,FXdouble& phi) const;

  /// Set quaternion from roll (x), pitch (y), yaw (z)
  void setRollPitchYaw(FXdouble roll,FXdouble pitch,FXdouble yaw);
  void getRollPitchYaw(FXdouble& roll,FXdouble& pitch,FXdouble& yaw) const;

  /// Set quaternion from yaw (z), pitch (y), roll (x)
  void setYawPitchRoll(FXdouble yaw,FXdouble pitch,FXdouble roll);
  void getYawPitchRoll(FXdouble& yaw,FXdouble& pitch,FXdouble& roll) const;

  /// Set quaternion from roll (x), yaw (z), pitch (y)
  void setRollYawPitch(FXdouble roll,FXdouble yaw,FXdouble pitch);
  void getRollYawPitch(FXdouble& roll,FXdouble& yaw,FXdouble& pitch) const;

  /// Set quaternion from pitch (y), roll (x),yaw (z)
  void setPitchRollYaw(FXdouble pitch,FXdouble roll,FXdouble yaw);
  void getPitchRollYaw(FXdouble& pitch,FXdouble& roll,FXdouble& yaw) const;

  /// Set quaternion from pitch (y), yaw (z), roll (x)
  void setPitchYawRoll(FXdouble pitch,FXdouble yaw,FXdouble roll);
  void getPitchYawRoll(FXdouble& pitch,FXdouble& yaw,FXdouble& roll) const;

  /// Set quaternion from yaw (z), roll (x), pitch (y)
  void setYawRollPitch(FXdouble yaw,FXdouble roll,FXdouble pitch);
  void getYawRollPitch(FXdouble& yaw,FXdouble& roll,FXdouble& pitch) const;

  /// Set quaternion from axes
  void setAxes(const FXVec3d& ex,const FXVec3d& ey,const FXVec3d& ez);

  /// Get quaternion axes
  void getAxes(FXVec3d& ex,FXVec3d& ey,FXVec3d& ez) const;

  /// Obtain local x axis
  FXVec3d getXAxis() const;

  /// Obtain local y axis
  FXVec3d getYAxis() const;

  /// Obtain local z axis
  FXVec3d getZAxis() const;

  /// Exponentiate quaternion
  FXQuatd exp() const;

  /// Take logarithm of quaternion
  FXQuatd log() const;

  /// Power of quaternion
  FXQuatd pow(FXdouble t) const;

  /// Invert quaternion
  FXQuatd invert() const;

  /// Invert unit quaternion
  FXQuatd unitinvert() const;

  /// Conjugate quaternion
  FXQuatd conj() const;

  /// Destructor
 ~FXQuatd(){}
  };

/// Scaling
inline FXQuatd operator*(const FXQuatd& a,FXdouble n){return FXQuatd(a.x*n,a.y*n,a.z*n,a.w*n);}
inline FXQuatd operator*(FXdouble n,const FXQuatd& a){return FXQuatd(n*a.x,n*a.y,n*a.z,n*a.w);}
inline FXQuatd operator/(const FXQuatd& a,FXdouble n){return FXQuatd(a.x/n,a.y/n,a.z/n,a.w/n);}
inline FXQuatd operator/(FXdouble n,const FXQuatd& a){return FXQuatd(n/a.x,n/a.y,n/a.z,n/a.w);}

/// Quaternion and quaternion multiply
inline FXQuatd operator*(const FXQuatd& a,const FXQuatd& b){ return FXQuatd(a.w*b.x+a.x*b.w+a.y*b.z-a.z*b.y, a.w*b.y+a.y*b.w+a.z*b.x-a.x*b.z, a.w*b.z+a.z*b.w+a.x*b.y-a.y*b.x, a.w*b.w-a.x*b.x-a.y*b.y-a.z*b.z); }
inline FXQuatd operator/(const FXQuatd& a,const FXQuatd& b){ return a*b.invert(); }

/// Quaternion and quaternion addition
inline FXQuatd operator+(const FXQuatd& a,const FXQuatd& b){ return FXQuatd(a.x+b.x,a.y+b.y,a.z+b.z,a.w+b.w); }
inline FXQuatd operator-(const FXQuatd& a,const FXQuatd& b){ return FXQuatd(a.x-b.x,a.y-b.y,a.z-b.z,a.w-b.w); }

/// Equality tests
inline FXbool operator==(const FXQuatd& a,FXdouble n){return a.x==n && a.y==n && a.z==n && a.w==n;}
inline FXbool operator!=(const FXQuatd& a,FXdouble n){return a.x!=n || a.y!=n || a.z!=n || a.w!=n;}
inline FXbool operator==(FXdouble n,const FXQuatd& a){return n==a.x && n==a.y && n==a.z && n==a.w;}
inline FXbool operator!=(FXdouble n,const FXQuatd& a){return n!=a.x || n!=a.y || n!=a.z || n!=a.w;}

/// Equality tests
inline FXbool operator==(const FXQuatd& a,const FXQuatd& b){ return a.x==b.x && a.y==b.y && a.z==b.z && a.w==b.w; }
inline FXbool operator!=(const FXQuatd& a,const FXQuatd& b){ return a.x!=b.x || a.y!=b.y || a.z!=b.z || a.w!=b.w; }


/// Construct quaternion from arc a->b on unit sphere
extern FXAPI FXQuatd arc(const FXVec3d& a,const FXVec3d& b);

/// Spherical lerp of unit quaternions u,v
extern FXAPI FXQuatd lerp(const FXQuatd& u,const FXQuatd& v,FXdouble f);

/// Derivative of spherical lerp of unit quaternions u,v
extern FXAPI FXQuatd lerpdot(const FXQuatd& u,const FXQuatd& v,FXdouble f);

/// Save quaternion to a stream
extern FXAPI FXStream& operator<<(FXStream& store,const FXQuatd& v);

/// Load quaternion from a stream
extern FXAPI FXStream& operator>>(FXStream& store,FXQuatd& v);

}

#endif
