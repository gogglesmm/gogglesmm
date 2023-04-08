/********************************************************************************
*                                                                               *
*              D o u b l e - P r e c i s i o n  Q u a t e r n i o n             *
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
#ifndef FXQUATD_H
#define FXQUATD_H

namespace FX {


// Forward reference
class FXMat3d;


/// Double-precision quaternion
class FXAPI FXQuatd {
public:
  double x;
  double y;
  double z;
  double w;
public:

  /**
  * Default constructor; value is not initialized.
  */
  FXQuatd(){}

  /**
  * Copy constructor.
  */
  FXQuatd(const FXQuatd& q):x(q.x),y(q.y),z(q.z),w(q.w){}

  /**
  * Construct from array of four doubles.
  */
  FXQuatd(const FXdouble v[]):x(v[0]),y(v[1]),z(v[2]),w(v[3]){}

  /**
  * Construct from four components.
  */
  FXQuatd(FXdouble xx,FXdouble yy,FXdouble zz,FXdouble ww):x(xx),y(yy),z(zz),w(ww){}

  /**
  * Construct from rotation axis and angle in radians.
  */
  FXQuatd(const FXVec3d& axis,FXdouble phi);

  /**
  * Construct quaternion from arc between two unit vectors fm and to.
  */
  FXQuatd(const FXVec3d& fr,const FXVec3d& to);

  /**
  * Construct from euler angles yaw (z), pitch (y), and roll (x).
  */
  FXQuatd(FXdouble roll,FXdouble pitch,FXdouble yaw);

  /**
  * Construct quaternion from three orthogonal unit vectors.
  */
  FXQuatd(const FXVec3d& ex,const FXVec3d& ey,const FXVec3d& ez);

  /**
  * Construct quaternion from rotation vector rot, representing a rotation
  * by |rot| radians about a unit vector rot/|rot|.
  */
  FXQuatd(const FXVec3d& rot);

  /**
  * Return a non-const reference to the ith element.
  */
  FXdouble& operator[](FXint i){return (&x)[i];}

  /**
  * Return a const reference to the ith element.
  */
  const FXdouble& operator[](FXint i) const {return (&x)[i];}

  /**
  * Assignment from other quaternion.
  */
  FXQuatd& operator=(const FXQuatd& v){x=v.x;y=v.y;z=v.z;w=v.w;return *this;}

  /**
  * Assignment from array of four doubles.
  */
  FXQuatd& operator=(const FXdouble v[]){x=v[0];y=v[1];z=v[2];w=v[3];return *this;}

  /**
  * Set value from another quaternion.
  */
  FXQuatd& set(const FXQuatd& v){x=v.x;y=v.y;z=v.z;w=v.w;return *this;}

  /**
  * Set value from array of four doubles.
  */
  FXQuatd& set(const FXdouble v[]){x=v[0];y=v[1];z=v[2];w=v[3];return *this;}

  /**
  * Set value from four components.
  */
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

  /// Conversion to 3-vector, axis of rotation
  operator FXVec3d&(){return *reinterpret_cast<FXVec3d*>(this);}
  operator const FXVec3d&() const {return *reinterpret_cast<const FXVec3d*>(this);}

  /// Test if zero
  FXbool operator!() const {return x==0.0 && y==0.0 && z==0.0 && w==0.0; }

  /// Unary
  FXQuatd operator+() const { return *this; }
  FXQuatd operator-() const { return FXQuatd(-x,-y,-z,-w); }

  /// Length and square of length
  FXdouble length2() const { return w*w+z*z+y*y+x*x; }
  FXdouble length() const { return Math::sqrt(length2()); }

  /// Adjust quaternion length
  FXQuatd& adjust();

  /**
  * Set quaternion from axis and angle.
  * Quaternion represents a rotation of phi radians about unit vector axis.
  */
  void setAxisAngle(const FXVec3d& axis,FXdouble phi);

  /**
  * Obtain axis and angle from quaternion.
  * Return unit vector and angle of rotation phi, in radians.
  * If identity quaternion (0,0,0,1), return axis as (1,0.0).
  */
  void getAxisAngle(FXVec3d& axis,FXdouble& phi) const;

  /**
  * Set quaternion from rotation vector rot, representing a rotation by |rot| radians
  * about a unit vector rot/|rot|.  Set to the identity quaternion (0,0,0,1) if the rotation
  * vector is equal to (0,0,0).
  */
  void setRotation(const FXVec3d& rot);

  /**
  * Get rotation vector from quaternion, representing a rotation of |rot| radians
  * about an axis rot/|rot|.  If quaternion is identity quaternion (0,0,0,1), return (0,0,0).
  */
  FXVec3d getRotation() const;

  /**
  * Set unit quaternion to modified rodrigues parameters.
  * Modified Rodriques parameters are defined as MRP = tan(theta/4)*E,
  * where theta is rotation angle (radians), and E is unit axis of rotation.
  */
  void setMRP(const FXVec3d& m);

  /**
  * Return modified rodrigues parameters from unit quaternion.
  */
  FXVec3d getMRP() const;

  /// Set quaternion from roll (x), pitch (y), yaw (z), in that order
  void setRollPitchYaw(FXdouble roll,FXdouble pitch,FXdouble yaw);

  /// Return the roll (x), pitch (y), yaw (z) angles represented by the quaternion
  void getRollPitchYaw(FXdouble& roll,FXdouble& pitch,FXdouble& yaw) const;

  /// Set quaternion from yaw (z), pitch (y), roll (x), in that order
  void setYawPitchRoll(FXdouble yaw,FXdouble pitch,FXdouble roll);

  /// Return the yaw (z), pitch (y), roll (x) angles represented by the quaternion
  void getYawPitchRoll(FXdouble& yaw,FXdouble& pitch,FXdouble& roll) const;

  /// Set quaternion from roll (x), yaw (z), pitch (y), in that order
  void setRollYawPitch(FXdouble roll,FXdouble yaw,FXdouble pitch);

  /// Return the roll (x), yaw (z), pitch (y) angles represented by the quaternion
  void getRollYawPitch(FXdouble& roll,FXdouble& yaw,FXdouble& pitch) const;

  /// Set quaternion from pitch (y), roll (x),yaw (z), in that order
  void setPitchRollYaw(FXdouble pitch,FXdouble roll,FXdouble yaw);

  /// Return the pitch (y), roll (x),yaw (z) angles represented by the quaternion
  void getPitchRollYaw(FXdouble& pitch,FXdouble& roll,FXdouble& yaw) const;

  /// Set quaternion from pitch (y), yaw (z), roll (x), in that order
  void setPitchYawRoll(FXdouble pitch,FXdouble yaw,FXdouble roll);

  /// Return the pitch (y), yaw (z), roll (x) angles represented by the quaternion
  void getPitchYawRoll(FXdouble& pitch,FXdouble& yaw,FXdouble& roll) const;

  /// Set quaternion from yaw (z), roll (x), pitch (y), in that order
  void setYawRollPitch(FXdouble yaw,FXdouble roll,FXdouble pitch);

  /// Return the yaw (z), roll (x), pitch (y) angles represented by the quaternion
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

  /// Conjugate quaternion
  FXQuatd conj() const { return FXQuatd(-x,-y,-z,w); }

  /// Invert unit quaternion
  FXQuatd unitinvert() const { return FXQuatd(-x,-y,-z,w); }

  /// Invert quaternion
  FXQuatd invert() const { FXdouble m(length2()); return FXQuatd(-x/m,-y/m,-z/m,w/m); }

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

/// Rotation unit-quaternion and vector q . v = (q* . v . q)
extern FXAPI FXVec3d operator*(const FXQuatd& q,const FXVec3d& v);

/// Rotation a vector and unit-quaternion v . q = (q . v . q*)
extern FXAPI FXVec3d operator*(const FXVec3d& v,const FXQuatd& q);

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

/// Normalize quaternion such that |Q|==1
extern FXAPI FXQuatd normalize(const FXQuatd& q);

/// Normalize quaternion incrementally; assume |Q| approximately 1 already
extern FXAPI FXQuatd fastnormalize(const FXQuatd& q);

/// Cubic hermite quaternion interpolation
extern FXAPI FXQuatd hermite(const FXQuatd& q0,const FXVec3d& r0,const FXQuatd& q1,const FXVec3d& r1,FXdouble t);

/// Estimate angular body rates omega from unit quaternions Q0 and Q1 separated by time dt
extern FXAPI FXVec3d omegaBody(const FXQuatd& q0,const FXQuatd& q1,FXdouble dt);

/// Derivative of unit quaternion q with body angular rates omega (rad/s)
extern FXAPI FXQuatd quatDot(const FXQuatd& q,const FXVec3d& omega);

/// Calculate angular acceleration of a body with inertial moments tensor M
/// Rotation about its axes with angular rates omega, under a torque torq
extern FXAPI FXVec3d omegaDot(const FXMat3d& M,const FXVec3d& omega,const FXVec3d& torq);

/// Save quaternion to a stream
extern FXAPI FXStream& operator<<(FXStream& store,const FXQuatd& v);

/// Load quaternion from a stream
extern FXAPI FXStream& operator>>(FXStream& store,FXQuatd& v);

}

#endif
