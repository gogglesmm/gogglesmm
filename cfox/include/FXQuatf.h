/********************************************************************************
*                                                                               *
*              S i n g l e - P r e c i s i o n  Q u a t e r n i o n             *
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
#ifndef FXQUATF_H
#define FXQUATF_H

namespace FX {


// Forward reference
class FXMat3f;


/// Single-precision quaternion
class FXAPI FXQuatf {
public:
  FXfloat x;
  FXfloat y;
  FXfloat z;
  FXfloat w;
public:

  /**
  * Default constructor; value is not initialized.
  */
  FXQuatf(){}

  /**
  * Copy constructor.
  */
  FXQuatf(const FXQuatf& q):x(q.x),y(q.y),z(q.z),w(q.w){}

  /**
  * Construct from array of four floats.
  */
  FXQuatf(const FXfloat v[]):x(v[0]),y(v[1]),z(v[2]),w(v[3]){}

  /**
  * Construct from four components.
  */
  FXQuatf(FXfloat xx,FXfloat yy,FXfloat zz,FXfloat ww):x(xx),y(yy),z(zz),w(ww){}

  /**
  * Construct from rotation axis and angle in radians.
  */
  FXQuatf(const FXVec3f& axis,FXfloat phi);

  /**
  * Construct quaternion from arc between two unit vectors fm and to.
  */
  FXQuatf(const FXVec3f& fr,const FXVec3f& to);

  /**
  * Construct from euler angles yaw (z), pitch (y), and roll (x).
  */
  FXQuatf(FXfloat roll,FXfloat pitch,FXfloat yaw);

  /**
  * Construct quaternion from three orthogonal unit vectors.
  */
  FXQuatf(const FXVec3f& ex,const FXVec3f& ey,const FXVec3f& ez);

  /**
  * Construct quaternion from rotation vector rot, representing a rotation
  * by |rot| radians about a unit vector rot/|rot|.
  */
  FXQuatf(const FXVec3f& rot);

  /**
  * Return a non-const reference to the ith element.
  */
  FXfloat& operator[](FXint i){return (&x)[i];}

  /**
  * Return a const reference to the ith element.
  */
  const FXfloat& operator[](FXint i) const {return (&x)[i];}

  /**
  * Assignment from other quaternion.
  */
  FXQuatf& operator=(const FXQuatf& v){x=v.x;y=v.y;z=v.z;w=v.w;return *this;}

  /**
  * Assignment from array of four floats.
  */
  FXQuatf& operator=(const FXfloat v[]){x=v[0];y=v[1];z=v[2];w=v[3];return *this;}

  /**
  * Set value from another quaternion.
  */
  FXQuatf& set(const FXQuatf& v){x=v.x;y=v.y;z=v.z;w=v.w;return *this;}

  /**
  * Set value from array of four floats.
  */
  FXQuatf& set(const FXfloat v[]){x=v[0];y=v[1];z=v[2];w=v[3];return *this;}

  /**
  * Set value from four components.
  */
  FXQuatf& set(FXfloat xx,FXfloat yy,FXfloat zz,FXfloat ww){x=xx;y=yy;z=zz;w=ww;return *this;}

  /// Assigning operators
  FXQuatf& operator*=(FXfloat n){ return set(x*n,y*n,z*n,w*n); }
  FXQuatf& operator/=(FXfloat n){ return set(x/n,y/n,z/n,w/n); }
  FXQuatf& operator+=(const FXQuatf& v){ return set(x+v.x,y+v.y,z+v.z,w+v.w); }
  FXQuatf& operator-=(const FXQuatf& v){ return set(x-v.x,y-v.y,z-v.z,w-v.w); }
  FXQuatf& operator*=(const FXQuatf& b){ return set(w*b.x+x*b.w+y*b.z-z*b.y, w*b.y+y*b.w+z*b.x-x*b.z, w*b.z+z*b.w+x*b.y-y*b.x, w*b.w-x*b.x-y*b.y-z*b.z); }
  FXQuatf& operator/=(const FXQuatf& b){ return *this*=b.invert(); }

  /// Conversion
  operator FXfloat*(){return &x;}
  operator const FXfloat*() const {return &x;}

  /// Conversion to 3-vector, axis of rotation
  operator FXVec3f&(){return *reinterpret_cast<FXVec3f*>(this);}
  operator const FXVec3f&() const {return *reinterpret_cast<const FXVec3f*>(this);}

  /// Test if zero
  FXbool operator!() const {return x==0.0f && y==0.0f && z==0.0f && w==0.0f; }

  /// Unary
  FXQuatf operator+() const { return *this; }
  FXQuatf operator-() const { return FXQuatf(-x,-y,-z,-w); }

  /// Length and square of length
  FXfloat length2() const { return w*w+z*z+y*y+x*x; }
  FXfloat length() const { return Math::sqrt(length2()); }

  /// Adjust quaternion length
  FXQuatf& adjust();

  /**
  * Set quaternion from axis and angle.
  * Quaternion represents a rotation of phi radians about unit vector axis.
  */
  void setAxisAngle(const FXVec3f& axis,FXfloat phi);

  /**
  * Obtain axis and angle from quaternion.
  * Return unit vector and angle of rotation phi, in radians.
  * If identity quaternion (0,0,0,1), return axis as (1,0.0).
  */
  void getAxisAngle(FXVec3f& axis,FXfloat& phi) const;

  /**
  * Set quaternion from rotation vector rot, representing a rotation by |rot| radians
  * about a unit vector rot/|rot|.  Set to the identity quaternion (0,0,0,1) if the rotation
  * vector is equal to (0,0,0).
  */
  void setRotation(const FXVec3f& rot);

  /**
  * Get rotation vector from quaternion, representing a rotation of |rot| radians
  * about an axis rot/|rot|.  If quaternion is identity quaternion (0,0,0,1), return (0,0,0).
  */
  FXVec3f getRotation() const;

  /**
  * Set unit quaternion to modified rodrigues parameters.
  * Modified Rodriques parameters are defined as MRP = tan(theta/4)*E,
  * where theta is rotation angle (radians), and E is unit axis of rotation.
  */
  void setMRP(const FXVec3f& m);

  /**
  * Return modified rodrigues parameters from unit quaternion.
  */
  FXVec3f getMRP() const;

  /// Set quaternion from roll (x), pitch (y), yaw (z), in that order
  void setRollPitchYaw(FXfloat roll,FXfloat pitch,FXfloat yaw);

  /// Return the roll (x), pitch (y), yaw (z) angles represented by the quaternion
  void getRollPitchYaw(FXfloat& roll,FXfloat& pitch,FXfloat& yaw) const;

  /// Set quaternion from yaw (z), pitch (y), roll (x), in that order
  void setYawPitchRoll(FXfloat yaw,FXfloat pitch,FXfloat roll);

  /// Return the yaw (z), pitch (y), roll (x) angles represented by the quaternion
  void getYawPitchRoll(FXfloat& yaw,FXfloat& pitch,FXfloat& roll) const;

  /// Set quaternion from roll (x), yaw (z), pitch (y), in that order
  void setRollYawPitch(FXfloat roll,FXfloat yaw,FXfloat pitch);

  /// Return the roll (x), yaw (z), pitch (y) angles represented by the quaternion
  void getRollYawPitch(FXfloat& roll,FXfloat& yaw,FXfloat& pitch) const;

  /// Set quaternion from pitch (y), roll (x),yaw (z), in that order
  void setPitchRollYaw(FXfloat pitch,FXfloat roll,FXfloat yaw);

  /// Return the pitch (y), roll (x),yaw (z) angles represented by the quaternion
  void getPitchRollYaw(FXfloat& pitch,FXfloat& roll,FXfloat& yaw) const;

  /// Set quaternion from pitch (y), yaw (z), roll (x), in that order
  void setPitchYawRoll(FXfloat pitch,FXfloat yaw,FXfloat roll);

  /// Return the pitch (y), yaw (z), roll (x) angles represented by the quaternion
  void getPitchYawRoll(FXfloat& pitch,FXfloat& yaw,FXfloat& roll) const;

  /// Set quaternion from yaw (z), roll (x), pitch (y), in that order
  void setYawRollPitch(FXfloat yaw,FXfloat roll,FXfloat pitch);

  /// Return the yaw (z), roll (x), pitch (y) angles represented by the quaternion
  void getYawRollPitch(FXfloat& yaw,FXfloat& roll,FXfloat& pitch) const;

  /// Set quaternion from axes
  void setAxes(const FXVec3f& ex,const FXVec3f& ey,const FXVec3f& ez);

  /// Get quaternion axes
  void getAxes(FXVec3f& ex,FXVec3f& ey,FXVec3f& ez) const;

  /// Obtain local x axis
  FXVec3f getXAxis() const;

  /// Obtain local y axis
  FXVec3f getYAxis() const;

  /// Obtain local z axis
  FXVec3f getZAxis() const;

  /// Exponentiate quaternion
  FXQuatf exp() const;

  /// Take logarithm of quaternion
  FXQuatf log() const;

  /// Power of quaternion
  FXQuatf pow(FXfloat t) const;

  /// Conjugate quaternion
  FXQuatf conj() const { return FXQuatf(-x,-y,-z,w); }

  /// Invert unit quaternion
  FXQuatf unitinvert() const { return FXQuatf(-x,-y,-z,w); }

  /// Invert quaternion
  FXQuatf invert() const { FXfloat m(length2()); return FXQuatf(-x/m,-y/m,-z/m,w/m); }

  /// Destructor
 ~FXQuatf(){}
  };


/// Scaling
inline FXQuatf operator*(const FXQuatf& a,FXfloat n){return FXQuatf(a.x*n,a.y*n,a.z*n,a.w*n);}
inline FXQuatf operator*(FXfloat n,const FXQuatf& a){return FXQuatf(n*a.x,n*a.y,n*a.z,n*a.w);}
inline FXQuatf operator/(const FXQuatf& a,FXfloat n){return FXQuatf(a.x/n,a.y/n,a.z/n,a.w/n);}
inline FXQuatf operator/(FXfloat n,const FXQuatf& a){return FXQuatf(n/a.x,n/a.y,n/a.z,n/a.w);}

/// Quaternion and quaternion multiply
inline FXQuatf operator*(const FXQuatf& a,const FXQuatf& b){ return FXQuatf(a.w*b.x+a.x*b.w+a.y*b.z-a.z*b.y, a.w*b.y+a.y*b.w+a.z*b.x-a.x*b.z, a.w*b.z+a.z*b.w+a.x*b.y-a.y*b.x, a.w*b.w-a.x*b.x-a.y*b.y-a.z*b.z); }
inline FXQuatf operator/(const FXQuatf& a,const FXQuatf& b){ return a*b.invert(); }

/// Rotation unit-quaternion and vector q . v = (q* . v . q)
extern FXAPI FXVec3f operator*(const FXQuatf& q,const FXVec3f& v);

/// Rotation a vector and unit-quaternion v . q = (q . v . q*)
extern FXAPI FXVec3f operator*(const FXVec3f& v,const FXQuatf& q);

/// Quaternion and quaternion addition
inline FXQuatf operator+(const FXQuatf& a,const FXQuatf& b){ return FXQuatf(a.x+b.x,a.y+b.y,a.z+b.z,a.w+b.w); }
inline FXQuatf operator-(const FXQuatf& a,const FXQuatf& b){ return FXQuatf(a.x-b.x,a.y-b.y,a.z-b.z,a.w-b.w); }

/// Equality tests
inline FXbool operator==(const FXQuatf& a,FXfloat n){return a.x==n && a.y==n && a.z==n && a.w==n;}
inline FXbool operator!=(const FXQuatf& a,FXfloat n){return a.x!=n || a.y!=n || a.z!=n || a.w!=n;}
inline FXbool operator==(FXfloat n,const FXQuatf& a){return n==a.x && n==a.y && n==a.z && n==a.w;}
inline FXbool operator!=(FXfloat n,const FXQuatf& a){return n!=a.x || n!=a.y || n!=a.z || n!=a.w;}

/// Equality tests
inline FXbool operator==(const FXQuatf& a,const FXQuatf& b){ return a.x==b.x && a.y==b.y && a.z==b.z && a.w==b.w; }
inline FXbool operator!=(const FXQuatf& a,const FXQuatf& b){ return a.x!=b.x || a.y!=b.y || a.z!=b.z || a.w!=b.w; }


/// Construct quaternion from arc a->b on unit sphere
extern FXAPI FXQuatf arc(const FXVec3f& a,const FXVec3f& b);

/// Spherical lerp of unit quaternions u,v
extern FXAPI FXQuatf lerp(const FXQuatf& u,const FXQuatf& v,FXfloat f);

/// Derivative of spherical lerp of unit quaternions u,v
extern FXAPI FXQuatf lerpdot(const FXQuatf& u,const FXQuatf& v,FXfloat f);

/// Normalize quaternion such that |Q|==1
extern FXAPI FXQuatf normalize(const FXQuatf& q);

/// Normalize quaternion incrementally; assume |Q| approximately 1 already
extern FXAPI FXQuatf fastnormalize(const FXQuatf& q);

/// Cubic hermite quaternion interpolation
extern FXAPI FXQuatf hermite(const FXQuatf& q0,const FXVec3f& r0,const FXQuatf& q1,const FXVec3f& r1,FXfloat t);

/// Estimate angular body rates omega from unit quaternions Q0 and Q1 separated by time dt
extern FXAPI FXVec3f omegaBody(const FXQuatf& q0,const FXQuatf& q1,FXfloat dt);

/// Derivative of unit quaternion q with body angular rates omega (rad/s)
extern FXAPI FXQuatf quatDot(const FXQuatf& q,const FXVec3f& omega);

/// Calculate angular acceleration of a body with inertial moments tensor M
/// Rotation about its axes with angular rates omega, under a torque torq
extern FXAPI FXVec3f omegaDot(const FXMat3f& M,const FXVec3f& omega,const FXVec3f& torq);

/// Save quaternion to a stream
extern FXAPI FXStream& operator<<(FXStream& store,const FXQuatf& v);

/// Load quaternion from a stream
extern FXAPI FXStream& operator>>(FXStream& store,FXQuatf& v);

}

#endif
