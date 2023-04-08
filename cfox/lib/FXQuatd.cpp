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
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "fxmath.h"
#include "FXArray.h"
#include "FXHash.h"
#include "FXStream.h"
#include "FXObject.h"
#include "FXVec2d.h"
#include "FXVec3d.h"
#include "FXVec4d.h"
#include "FXQuatd.h"
#include "FXMat3d.h"
#include "FXMat4d.h"

/*
  Notes:

  - Quaternion represents a rotation as follows:

                   phi       axis            phi
     Q =  ( sin ( ----- ) * ------  , cos ( ----- ) )
                    2       |axis|            2

  - Typically, |Q| == 1.  But this is not always a given.
  - Repeated operations should periodically fix Q to maintain |Q| == 1, using
    the adjust() API.
  - FIXME maybe refine exp() and log() as non-members.
*/

using namespace FX;

/*******************************************************************************/

namespace FX {

// Construct from rotation axis and angle in radians
FXQuatd::FXQuatd(const FXVec3d& axis,FXdouble phi){
  setAxisAngle(axis,phi);
  }


// Construct quaternion from arc between two unit vectors fm and to
FXQuatd::FXQuatd(const FXVec3d& fr,const FXVec3d& to){
  set(arc(fr,to));
  }


// Construct from euler angles yaw (z), pitch (y), and roll (x)
FXQuatd::FXQuatd(FXdouble roll,FXdouble pitch,FXdouble yaw){
  setRollPitchYaw(roll,pitch,yaw);
  }


// Construct quaternion from three orthogonal unit vectors
FXQuatd::FXQuatd(const FXVec3d& ex,const FXVec3d& ey,const FXVec3d& ez){
  setAxes(ex,ey,ez);
  }


// Construct quaternion from rotation vector rot, representing a rotation
// by |rot| radians about a unit vector rot/|rot|.
FXQuatd::FXQuatd(const FXVec3d& rot){
  setRotation(rot);
  }


// Set axis and angle
void FXQuatd::setAxisAngle(const FXVec3d& axis,FXdouble phi){
  FXdouble mag2(axis.length2());
  if(__likely(0.0<mag2)){
    FXdouble arg(0.5*phi);
    FXdouble mag(Math::sqrt(mag2));
    FXdouble s(Math::sin(arg)/mag);
    FXdouble c(Math::cos(arg));
    x=axis.x*s;
    y=axis.y*s;
    z=axis.z*s;
    w=c;
    }
  else{
    x=0.0;
    y=0.0;
    z=0.0;
    w=1.0;
    }
  }


// Obtain axis and angle
// Remeber that: q = sin(A/2)*(x*i+y*j+z*k)+cos(A/2)
void FXQuatd::getAxisAngle(FXVec3d& axis,FXdouble& phi) const {
  FXdouble mag2(x*x+y*y+z*z);
  if(0.0<mag2){
    FXdouble mag(Math::sqrt(mag2));
    axis.x=x/mag;
    axis.y=y/mag;
    axis.z=z/mag;
    phi=2.0*Math::atan2(mag,w);
    }
  else{
    axis.x=1.0;
    axis.y=0.0;
    axis.z=0.0;
    phi=0.0;
    }
  }


// Set quaternion from rotation vector rot
//
//                 |rot|         rot              |rot|
//   Q =  ( sin ( ------- ) *  -------  , cos (  ------- ) )
//                   2          |rot|               2
//
void FXQuatd::setRotation(const FXVec3d& rot){
  FXdouble mag2(rot.length2());
  if(0.0<mag2){
    FXdouble mag(Math::sqrt(mag2));
    FXdouble arg(mag*0.5);
    FXdouble s(Math::sin(arg)/mag);
    FXdouble c(Math::cos(arg));
    x=rot.x*s;
    y=rot.y*s;
    z=rot.z*s;
    w=c;
    }
  else{
    x=0.0;
    y=0.0;
    z=0.0;
    w=1.0;
    }
  }


// Get rotation vector from quaternion
FXVec3d FXQuatd::getRotation() const {
  FXVec3d rot(0.0,0.0,0.0);
  FXdouble mag2(x*x+y*y+z*z);
  if(0.0<mag2){
    FXdouble mag(Math::sqrt(mag2));
    FXdouble phi(2.0*Math::atan2(mag,w)/mag);
    rot.x=x*phi*mag;
    rot.y=y*phi*mag;
    rot.z=z*phi*mag;
    }
  return rot;
  }


// Set unit quaternion to modified rodrigues parameters.
// Modified Rodriques parameters are defined as MRP = tan(theta/4)*E,
// where theta is rotation angle (radians), and E is unit axis of rotation.
// Reference: "A survey of Attitude Representations", Malcolm D. Shuster,
// Journal of Astronautical sciences, Vol. 41, No. 4, Oct-Dec. 1993, pp. 476,
// Equations (253).
void FXQuatd::setMRP(const FXVec3d& m){
  FXdouble mm=m[0]*m[0]+m[1]*m[1]+m[2]*m[2];
  FXdouble D=1.0/(1.0+mm);
  x=m[0]*2.0*D;
  y=m[1]*2.0*D;
  z=m[2]*2.0*D;
  w=(1.0-mm)*D;
  }


// Return modified rodrigues parameters from unit quaternion.
// Reference: "A survey of Attitude Representations", Malcolm D. Shuster,
// Journal of Astronautical sciences, Vol. 41, No. 4, Oct-Dec. 1993, pp. 475,
// Equations (249). (250).
FXVec3d FXQuatd::getMRP() const {
  FXdouble m=(0.0<w)?1.0/(1.0+w):-1.0/(1.0-w);
  return FXVec3d(x*m,y*m,z*m);
  }


// Set quaternion from roll (x), pitch (y), yaw (z)
void FXQuatd::setRollPitchYaw(FXdouble roll,FXdouble pitch,FXdouble yaw){
  FXdouble sr,cr,sp,cp,sy,cy;
  FXdouble rr=0.5*roll;
  FXdouble pp=0.5*pitch;
  FXdouble yy=0.5*yaw;
  sr=Math::sin(rr); cr=Math::cos(rr);
  sp=Math::sin(pp); cp=Math::cos(pp);
  sy=Math::sin(yy); cy=Math::cos(yy);
  x=sr*cp*cy-cr*sp*sy;
  y=cr*sp*cy+sr*cp*sy;
  z=cr*cp*sy-sr*sp*cy;
  w=cr*cp*cy+sr*sp*sy;
  }


// Set quaternion from yaw (z), pitch (y), roll (x)
void FXQuatd::setYawPitchRoll(FXdouble yaw,FXdouble pitch,FXdouble roll){
  FXdouble sr,cr,sp,cp,sy,cy;
  FXdouble rr=0.5*roll;
  FXdouble pp=0.5*pitch;
  FXdouble yy=0.5*yaw;
  sr=Math::sin(rr); cr=Math::cos(rr);
  sp=Math::sin(pp); cp=Math::cos(pp);
  sy=Math::sin(yy); cy=Math::cos(yy);
  x=sr*cp*cy+cr*sp*sy;
  y=cr*sp*cy-sr*cp*sy;
  z=cr*cp*sy+sr*sp*cy;
  w=cr*cp*cy-sr*sp*sy;
  }


// Set quaternion from roll (x), yaw (z), pitch (y)
void FXQuatd::setRollYawPitch(FXdouble roll,FXdouble yaw,FXdouble pitch){
  FXdouble sr,cr,sp,cp,sy,cy;
  FXdouble rr=0.5*roll;
  FXdouble pp=0.5*pitch;
  FXdouble yy=0.5*yaw;
  sr=Math::sin(rr); cr=Math::cos(rr);
  sp=Math::sin(pp); cp=Math::cos(pp);
  sy=Math::sin(yy); cy=Math::cos(yy);
  x=cp*cy*sr+sp*sy*cr;
  y=sp*cy*cr+cp*sy*sr;
  z=cp*sy*cr-sp*cy*sr;
  w=cp*cy*cr-sp*sy*sr;
  }


// Set quaternion from pitch (y), roll (x),yaw (z)
void FXQuatd::setPitchRollYaw(FXdouble pitch,FXdouble roll,FXdouble yaw){
  FXdouble sr,cr,sp,cp,sy,cy;
  FXdouble rr=0.5*roll;
  FXdouble pp=0.5*pitch;
  FXdouble yy=0.5*yaw;
  sr=Math::sin(rr); cr=Math::cos(rr);
  sp=Math::sin(pp); cp=Math::cos(pp);
  sy=Math::sin(yy); cy=Math::cos(yy);
  x=cy*sr*cp-sy*cr*sp;
  y=cy*cr*sp+sy*sr*cp;
  z=cy*sr*sp+sy*cr*cp;
  w=cy*cr*cp-sy*sr*sp;
  }


// Set quaternion from pitch (y), yaw (z), roll (x)
void FXQuatd::setPitchYawRoll(FXdouble pitch,FXdouble yaw,FXdouble roll){
  FXdouble sr,cr,sp,cp,sy,cy;
  FXdouble rr=0.5*roll;
  FXdouble pp=0.5*pitch;
  FXdouble yy=0.5*yaw;
  sr=Math::sin(rr); cr=Math::cos(rr);
  sp=Math::sin(pp); cp=Math::cos(pp);
  sy=Math::sin(yy); cy=Math::cos(yy);
  x=sr*cy*cp-cr*sy*sp;
  y=cr*cy*sp-sr*sy*cp;
  z=sr*cy*sp+cr*sy*cp;
  w=cr*cy*cp+sr*sy*sp;
  }


// Set quaternion from yaw (z), roll (x), pitch (y)
void FXQuatd::setYawRollPitch(FXdouble yaw,FXdouble roll,FXdouble pitch){
  FXdouble sr,cr,sp,cp,sy,cy;
  FXdouble rr=0.5*roll;
  FXdouble pp=0.5*pitch;
  FXdouble yy=0.5*yaw;
  sr=Math::sin(rr); cr=Math::cos(rr);
  sp=Math::sin(pp); cp=Math::cos(pp);
  sy=Math::sin(yy); cy=Math::cos(yy);
  x=cp*sr*cy+sp*cr*sy;
  y=sp*cr*cy-cp*sr*sy;
  z=cp*cr*sy-sp*sr*cy;
  w=cp*cr*cy+sp*sr*sy;
  }


// Obtain roll, pitch, yaw
void FXQuatd::getRollPitchYaw(FXdouble& roll,FXdouble& pitch,FXdouble& yaw) const {
  roll=Math::atan2(2.0*(y*z+w*x),1.0-2.0*(x*x+y*y));
  pitch=Math::asin(Math::fclamp(-1.0,-2.0*(x*z-w*y),1.0));
  yaw=Math::atan2(2.0*(x*y+w*z),1.0-2.0*(y*y+z*z));
  }


// Obtain yaw, pitch, and roll
void FXQuatd::getYawPitchRoll(FXdouble& yaw,FXdouble& pitch,FXdouble& roll) const {
  yaw=Math::atan2(-2.0*(x*y-w*z),1.0-2.0*(y*y+z*z));
  pitch=Math::asin(Math::fclamp(-1.0,2.0*(x*z+w*y),1.0));
  roll=Math::atan2(-2.0*(y*z-w*x),1.0-2.0*(x*x+y*y));
  }


// Obtain roll, yaw, pitch
void FXQuatd::getRollYawPitch(FXdouble& roll,FXdouble& yaw,FXdouble& pitch) const {
  roll=Math::atan2(-2.0*(y*z-w*x),1.0-2.0*(x*x+z*z));
  yaw=Math::asin(Math::fclamp(-1.0,2.0*(x*y+w*z),1.0));
  pitch=Math::atan2(-2.0*(x*z-w*y),1.0-2.0*(y*y+z*z));
  }


// Obtain pitch, roll, yaw
void FXQuatd::getPitchRollYaw(FXdouble& pitch,FXdouble& roll,FXdouble& yaw) const {
  pitch=Math::atan2(-2.0*(x*z-w*y),1.0-2.0*(x*x+y*y));
  roll=Math::asin(Math::fclamp(-1.0,2.0*(y*z+w*x),1.0));
  yaw=Math::atan2(-2.0*(x*y-w*z),1.0-2.0*(x*x+z*z));
  }


// Obtain pitch, yaw, roll
void FXQuatd::getPitchYawRoll(FXdouble& pitch,FXdouble& yaw,FXdouble& roll) const {
  pitch=Math::atan2(2.0*(x*z+w*y),1.0-2.0*(y*y+z*z));
  yaw=Math::asin(Math::fclamp(-1.0,-2.0*(x*y-w*z),1.0));
  roll=Math::atan2(2.0*(y*z+w*x),1.0-2.0*(x*x+z*z));
  }


// Obtain yaw, roll, pitch
void FXQuatd::getYawRollPitch(FXdouble& yaw,FXdouble& roll,FXdouble& pitch) const {
  yaw=Math::atan2(2.0*(x*y+w*z),1.0-2.0*(x*x+z*z));
  roll=Math::asin(Math::fclamp(-1.0,-2.0*(y*z-w*x),1.0));
  pitch=Math::atan2(2.0*(x*z+w*y),1.0-2.0*(x*x+y*y));
  }


// Set quaternion from axes
// "Converting a Rotation Matrix to a Quaternion," Mike Day, Insomniac Games.
void FXQuatd::setAxes(const FXVec3d& ex,const FXVec3d& ey,const FXVec3d& ez){
  FXdouble t;
  if(ez.z<0.0){
    if(ex.x>ey.y){
      t=1.0+ex.x-ey.y-ez.z;
      x=t;
      y=ex.y+ey.x;
      z=ez.x+ex.z;
      w=ey.z-ez.y;
      }
    else{
      t=1.0-ex.x+ey.y-ez.z;
      x=ex.y+ey.x;
      y=t;
      z=ey.z+ez.y;
      w=ez.x-ex.z;
      }
    }
  else{
    if(ex.x<-ey.y){
      t=1.0-ex.x-ey.y+ez.z;
      x=ez.x+ex.z;
      y=ey.z+ez.y;
      z=t;
      w=ex.y-ey.x;
      }
    else{
      t=1.0+ex.x+ey.y+ez.z;
      x=ey.z-ez.y;
      y=ez.x-ex.z;
      z=ex.y-ey.x;
      w=t;
      }
    }
  FXASSERT(t>0.0);
  t=0.5/Math::sqrt(t);
  x*=t;
  y*=t;
  z*=t;
  w*=t;
  }


// Get quaternion axes
void FXQuatd::getAxes(FXVec3d& ex,FXVec3d& ey,FXVec3d& ez) const {
  FXdouble tx=2.0*x;
  FXdouble ty=2.0*y;
  FXdouble tz=2.0*z;
  FXdouble twx=tx*w;
  FXdouble twy=ty*w;
  FXdouble twz=tz*w;
  FXdouble txx=tx*x;
  FXdouble txy=ty*x;
  FXdouble txz=tz*x;
  FXdouble tyy=ty*y;
  FXdouble tyz=tz*y;
  FXdouble tzz=tz*z;
  ex.x=1.0-tyy-tzz;
  ex.y=txy+twz;
  ex.z=txz-twy;
  ey.x=txy-twz;
  ey.y=1.0-txx-tzz;
  ey.z=tyz+twx;
  ez.x=txz+twy;
  ez.y=tyz-twx;
  ez.z=1.0-txx-tyy;
  }


// Obtain local x axis
FXVec3d FXQuatd::getXAxis() const {
  FXdouble ty=2.0*y;
  FXdouble tz=2.0*z;
  return FXVec3d(1.0-ty*y-tz*z,ty*x+tz*w,tz*x-ty*w);
  }


// Obtain local y axis
FXVec3d FXQuatd::getYAxis() const {
  FXdouble tx=2.0*x;
  FXdouble tz=2.0*z;
  return FXVec3d(tx*y-tz*w,1.0-tx*x-tz*z,tz*y+tx*w);
  }


// Obtain local z axis
FXVec3d FXQuatd::getZAxis() const {
  FXdouble tx=2.0*x;
  FXdouble ty=2.0*y;
  return FXVec3d(tx*z+ty*w,ty*z-tx*w,1.0-tx*x-ty*y);
  }


// Exponentiate unit quaternion
// Given q = theta*(x*i+y*j+z*k), where length of (x,y,z) is 1,
// then exp(q) = sin(theta)*(x*i+y*j+z*k)+cos(theta).
FXQuatd FXQuatd::exp() const {
  FXQuatd result(0.0,0.0,0.0,1.0);
  FXdouble mag2(x*x+y*y+z*z);
  if(0.0<mag2){
    FXdouble mag(Math::sqrt(mag2));
    FXdouble s(Math::sin(mag)/mag);
    FXdouble c(Math::cos(mag));
    result.x=x*s;
    result.y=y*s;
    result.z=z*s;
    result.w=c;
    }
  return result;
  }


// Take logarithm of unit quaternion
// Given q = sin(theta)*(x*i+y*j+z*k)+cos(theta), length of (x,y,z) is 1,
// then log(q) = theta*(x*i+y*j+z*k).
FXQuatd FXQuatd::log() const {
  FXQuatd result(0.0,0.0,0.0,0.0);
  FXdouble mag2(x*x+y*y+z*z);
  if(0.0<mag2){
    FXdouble mag(Math::sqrt(mag2));
    FXdouble phi(Math::atan2(mag,w)/mag);
    result.x=x*phi;
    result.y=y*phi;
    result.z=z*phi;
    }
  return result;
  }


// Power of quaternion
FXQuatd FXQuatd::pow(FXdouble t) const {
  return (log()*t).exp();
  }


// Rotation unit-quaternion and vector v . q = (q . v . q*) where q* is
// the conjugate of q.
//
// The Rodriques Formula for rotating a vector V over angle A about a unit-vector K:
//
//    V' = K (K . V) + (K x V) sin(A) - K x (K x V) cos(A)
//
// Given UNIT quaternion q=(S,c), i.e. |q|=1, defined as a imaginary part S with
// |S|=K sin(A/2), where K is a unit vector, and a real part c=cos(A/2), we can obtain,
// after some (well, a LOT of) manipulation:
//
//    V' = S (S . V) + c (c V + S x V) + S x (c V + S x V)
//
// Using:
//
//    A x (B x C) = B (A . C) - C (A . B)
//
// We can further simplify:
//
//    V' = V + 2 S x (S x V + c V)
//
FXVec3d operator*(const FXVec3d& v,const FXQuatd& q){
  FXVec3d s(q.x,q.y,q.z);
  return v+(s^((s^v)+(v*q.w)))*2.0;
  }


// Rotation unit-quaternion and vector q . v = (q* . v . q)
FXVec3d operator*(const FXQuatd& q,const FXVec3d& v){
  FXVec3d s(q.x,q.y,q.z);
  return v+(((v^s)+(v*q.w))^s)*2.0;     // Yes, -a^b is b^a!
  }

/*******************************************************************************/

// Adjust quaternion length
FXQuatd& FXQuatd::adjust(){
  FXdouble s(length());
  if(__likely(s)){
    return *this /= s;
    }
  return *this;
  }


// Normalize quaternion such that |Q|==1
FXQuatd normalize(const FXQuatd& q){
  FXdouble s(q.length());
  if(__likely(s)){
    return q/s;
    }
  return q;
  }


// Normalize quaternion incrementally; assume |Q| approximately 1 already
FXQuatd fastnormalize(const FXQuatd& q){
  FXdouble s((3.0-q.w*q.w-q.z*q.z-q.y*q.y-q.x*q.x)*0.5);
  return q*s;
  }

/*******************************************************************************/

// Construct quaternion from arc f->t, described by two vectors f and t on
// a unit sphere.
//
// A quaternion which rotates by an angle theta about a unit axis A is specified as:
//
//   q = (A * sin(theta/2), cos(theta/2)).
//
// Assuming is f and t are unit length, construct half-way vector:
//
//   h = (f + t)
//
// Then:
//                        f . h
//  cos(theta/2)     =   -------
//                       |f|*|h|
//
// and:
//
//                        f x h
//  A * sin(theta/2) =   -------
//                       |f|*|h|
//
// So generate normalized quaternion as follows:
//
//         f x h     f . h        (f x h, f . h)     (f x h, f . h)
//  Q = ( ------- , ------- )  = ---------------- = ----------------
//        |f|*|h|   |f|*|h|          |f|*|h|        |(f x h, f . h)|
//
// NOTE1: Technically, input vectors f and t do not actually have to
// be unit length in this formulation.  However, they do need to be
// the same lengths.
//
// NOTE2: A problem exists when |h|=0.  This only happens when rotating
// 180 degrees, i.e. f = -t.  In this case, the dot-product (f . h) will
// be zero.  Pick a vector v orthogonal to f, then set Q:
//
//  Q = (v, 0)
//
FXQuatd arc(const FXVec3d& f,const FXVec3d& t){
  FXQuatd result;
  FXVec3d h(f+t);
  FXdouble w(f.x*h.x+f.y*h.y+f.z*h.z);
  if(Math::fabs(w)<0.00000000000000001){ // |f.h| is small
    FXdouble ax=Math::fabs(f.x);
    FXdouble ay=Math::fabs(f.y);
    FXdouble az=Math::fabs(f.z);
    if(ax<ay){
      if(ax<az){                        // |f.x| smallest
        result.x=-f.y*f.y-f.z*f.z;
        result.y= f.x*f.y;
        result.z= f.x*f.z;
        result.w= 0.0;
        }
      else{                             // |f.z| smallest
        result.x= f.x*f.z;
        result.y= f.y*f.z;
        result.z=-f.x*f.x-f.y*f.y;
        result.w= 0.0;
        }
      }
    else{
      if(ay<az){                        // |f.y| smallest
        result.x= f.y*f.x;
        result.y=-f.x*f.x-f.z*f.z;
        result.z= f.y*f.z;
        result.w= 0.0;
        }
      else{                             // |f.z| smallest
        result.x= f.x*f.z;
        result.y= f.y*f.z;
        result.z=-f.y*f.y-f.x*f.x;
        result.w= 0.0;
        }
      }
    }
  else{
    result.x=f.y*h.z-f.z*h.y;           // fxh
    result.y=f.z*h.x-f.x*h.z;
    result.z=f.x*h.y-f.y*h.x;
    result.w=w;                         // f.h
    }
  result*=(1.0/result.length());
  return result;
  }

/*******************************************************************************/

// Spherical lerp of unit quaternions u,v
// This is equivalent to: u * (u.unitinvert()*v).pow(f)
FXQuatd lerp(const FXQuatd& u,const FXQuatd& v,FXdouble f){
  FXdouble dot=u.x*v.x+u.y*v.y+u.z*v.z+u.w*v.w;
  FXdouble to=Math::fblend(dot,0.0,-f,f);
  FXdouble fr=1.0-f;
  FXdouble cost=Math::fabs(dot);
  FXdouble sint;
  FXdouble theta;
  FXQuatd result;
  if(__likely(cost<0.999999999999999)){
    sint=Math::sqrt(1.0-cost*cost);
    theta=Math::atan2(sint,cost);
    fr=Math::sin(fr*theta)/sint;
    to=Math::sin(to*theta)/sint;
    }
  result.x=fr*u.x+to*v.x;
  result.y=fr*u.y+to*v.y;
  result.z=fr*u.z+to*v.z;
  result.w=fr*u.w+to*v.w;
  return result;
  }


// Derivative of spherical lerp of unit quaternions u,v
// This is equivalent to: u * (u.unitinvert()*v).pow(f) * (u.unitinvert()*v).log(),
// which is itself equivalent to: lerp(u,v,f) * (u.unitinvert()*v).log()
FXQuatd lerpdot(const FXQuatd& u,const FXQuatd& v,FXdouble f){
  FXdouble dot=u.x*v.x+u.y*v.y+u.z*v.z+u.w*v.w;
  FXdouble cost=Math::fabs(dot);
  FXdouble sint;
  FXdouble fr=1.0-f;
  FXdouble to=f;
  FXdouble theta;
  FXQuatd result;
  if(__likely(cost<0.999999999999999)){
    sint=Math::sqrt(1.0-cost*cost);
    theta=Math::atan2(sint,cost);
    fr=-theta*Math::cos(fr*theta)/sint;
    to=theta*Math::cos(to*theta)/sint;
    }
  result.x=fr*u.x+to*v.x;
  result.y=fr*u.y+to*v.y;
  result.z=fr*u.z+to*v.z;
  result.w=fr*u.w+to*v.w;
  return result;
  }


/*******************************************************************************/

// 1/(i*(2*i+1)) for i>=1
const FXdouble u8_0=0.333333333333333333333333;
const FXdouble u8_1=0.1;
const FXdouble u8_2=0.047619047619047619047619;
const FXdouble u8_3=0.027777777777777777777778;
const FXdouble u8_4=0.018181818181818181818182;
const FXdouble u8_5=0.012820512820512820512820;
const FXdouble u8_6=0.009523809523809523809524;
const FXdouble u8_7=0.00735294117647058823529412*1.85298109240830;

// i/(2*i+1) for i>=1
const FXdouble v8_0=0.333333333333333333333333;
const FXdouble v8_1=0.4;
const FXdouble v8_2=0.428571428571428571428571;
const FXdouble v8_3=0.444444444444444444444444;
const FXdouble v8_4=0.454545454545454545454545;
const FXdouble v8_5=0.461538461538461538461538;
const FXdouble v8_6=0.466666666666666666666667;
const FXdouble v8_7=0.470588235294117647058824*1.85298109240830;


// It is assumed that the angle between q0 and q1 is acute, i.e. angle between
// q0 and q1 is less than pi/2.
//
// Based on "A Fast and Accurate Algorithm for Computing SLERP", by David Eberly.
FXQuatd fastlerp8(const FXQuatd& q0,const FXQuatd& q1,FXdouble t){
  FXdouble xm1=q0.x*q1.x+q0.y*q1.y+q0.z*q1.z+q0.w*q1.w-1.0;      // x-1 = cos(theta)-1
  FXdouble d=1.0-t;
  FXdouble sqrT=t*t;
  FXdouble sqrD=d*d;
  FXdouble bD0=(u8_0*sqrD-v8_0)*xm1;
  FXdouble bD1=(u8_1*sqrD-v8_1)*xm1;
  FXdouble bD2=(u8_2*sqrD-v8_2)*xm1;
  FXdouble bD3=(u8_3*sqrD-v8_3)*xm1;
  FXdouble bD4=(u8_4*sqrD-v8_4)*xm1;
  FXdouble bD5=(u8_5*sqrD-v8_5)*xm1;
  FXdouble bD6=(u8_6*sqrD-v8_6)*xm1;
  FXdouble bD7=(u8_7*sqrD-v8_7)*xm1;
  FXdouble bT0=(u8_0*sqrT-v8_0)*xm1;
  FXdouble bT1=(u8_1*sqrT-v8_1)*xm1;
  FXdouble bT2=(u8_2*sqrT-v8_2)*xm1;
  FXdouble bT3=(u8_3*sqrT-v8_3)*xm1;
  FXdouble bT4=(u8_4*sqrT-v8_4)*xm1;
  FXdouble bT5=(u8_5*sqrT-v8_5)*xm1;
  FXdouble bT6=(u8_6*sqrT-v8_6)*xm1;
  FXdouble bT7=(u8_7*sqrT-v8_7)*xm1;
  FXdouble f0=d*(1.0+bD0*(1.0+bD1*(1.0+bD2*(1.0+bD3*(1.0+bD4*(1.0+bD5*(1.0+bD6*(1.0+bD7))))))));
  FXdouble f1=t*(1.0+bT0*(1.0+bT1*(1.0+bT2*(1.0+bT3*(1.0+bT4*(1.0+bT5*(1.0+bT6*(1.0+bT7))))))));
  FXQuatd result(f0*q0+f1*q1);
  return result;
  }

// 1/(i*(2*i+1)) for i>=1
const FXdouble u10_0=0.333333333333333333333333;
const FXdouble u10_1=0.1;
const FXdouble u10_2=0.047619047619047619047619;
const FXdouble u10_3=0.027777777777777777777778;
const FXdouble u10_4=0.018181818181818181818182;
const FXdouble u10_5=0.012820512820512820512820;
const FXdouble u10_6=0.009523809523809523809524;
const FXdouble u10_7=0.007352941176470588235294;
const FXdouble u10_8=0.005847953216374269005848;
const FXdouble u10_9=0.004761904761904761904762*1.87666328810155;

// i/(2*i+1) for i>=1
const FXdouble v10_0=0.333333333333333333333333;
const FXdouble v10_1=0.4;
const FXdouble v10_2=0.428571428571428571428571;
const FXdouble v10_3=0.444444444444444444444444;
const FXdouble v10_4=0.454545454545454545454545;
const FXdouble v10_5=0.461538461538461538461538;
const FXdouble v10_6=0.466666666666666666666667;
const FXdouble v10_7=0.470588235294117647058824;
const FXdouble v10_8=0.473684210526315789473684;
const FXdouble v10_9=0.476190476190476190476190*1.87666328810155;


FXQuatd fastlerp10(const FXQuatd& q0,const FXQuatd& q1,FXdouble t){
  FXdouble xm1=q0.x*q1.x+q0.y*q1.y+q0.z*q1.z+q0.w*q1.w-1.0;      // x-1 = cos(theta)-1
  FXdouble d=1.0-t;
  FXdouble sqrT=t*t;
  FXdouble sqrD=d*d;
  FXdouble bD0=(u10_0*sqrD-v10_0)*xm1;
  FXdouble bD1=(u10_1*sqrD-v10_1)*xm1;
  FXdouble bD2=(u10_2*sqrD-v10_2)*xm1;
  FXdouble bD3=(u10_3*sqrD-v10_3)*xm1;
  FXdouble bD4=(u10_4*sqrD-v10_4)*xm1;
  FXdouble bD5=(u10_5*sqrD-v10_5)*xm1;
  FXdouble bD6=(u10_6*sqrD-v10_6)*xm1;
  FXdouble bD7=(u10_7*sqrD-v10_7)*xm1;
  FXdouble bD8=(u10_8*sqrD-v10_8)*xm1;
  FXdouble bD9=(u10_9*sqrD-v10_9)*xm1;
  FXdouble bT0=(u10_0*sqrT-v10_0)*xm1;
  FXdouble bT1=(u10_1*sqrT-v10_1)*xm1;
  FXdouble bT2=(u10_2*sqrT-v10_2)*xm1;
  FXdouble bT3=(u10_3*sqrT-v10_3)*xm1;
  FXdouble bT4=(u10_4*sqrT-v10_4)*xm1;
  FXdouble bT5=(u10_5*sqrT-v10_5)*xm1;
  FXdouble bT6=(u10_6*sqrT-v10_6)*xm1;
  FXdouble bT7=(u10_7*sqrT-v10_7)*xm1;
  FXdouble bT8=(u10_8*sqrT-v10_8)*xm1;
  FXdouble bT9=(u10_9*sqrT-v10_9)*xm1;
  FXdouble f0=d*(1.0+bD0*(1.0+bD1*(1.0+bD2*(1.0+bD3*(1.0+bD4*(1.0+bD5*(1.0+bD6*(1.0+bD7*(1.0+bD8*(1.0+bD9))))))))));
  FXdouble f1=t*(1.0+bT0*(1.0+bT1*(1.0+bT2*(1.0+bT3*(1.0+bT4*(1.0+bT5*(1.0+bT6*(1.0+bT7*(1.0+bT8*(1.0+bT9))))))))));
  FXQuatd result(f0*q0+f1*q1);
  return result;
  }


// 1/(i*(2*i+1)) for i>=1
const FXdouble u12_0=0.333333333333333333333333;
const FXdouble u12_1=0.1;
const FXdouble u12_2=0.047619047619047619047619;
const FXdouble u12_3=0.027777777777777777777778;
const FXdouble u12_4=0.018181818181818181818182;
const FXdouble u12_5=0.012820512820512820512820;
const FXdouble u12_6=0.009523809523809523809524;
const FXdouble u12_7=0.007352941176470588235294;
const FXdouble u12_8=0.005847953216374269005848;
const FXdouble u12_9=0.004761904761904761904762;
const FXdouble u12_10=0.003952569169960474308300;
const FXdouble u12_11=0.00333333333333333333333333*1.89371240325272;

// i/(2*i+1) for i>=1
const FXdouble v12_0=0.333333333333333333333333;
const FXdouble v12_1=0.4;
const FXdouble v12_2=0.428571428571428571428571;
const FXdouble v12_3=0.444444444444444444444444;
const FXdouble v12_4=0.454545454545454545454545;
const FXdouble v12_5=0.461538461538461538461538;
const FXdouble v12_6=0.466666666666666666666667;
const FXdouble v12_7=0.470588235294117647058824;
const FXdouble v12_8=0.473684210526315789473684;
const FXdouble v12_9=0.476190476190476190476190;
const FXdouble v12_10=0.478260869565217391304348;
const FXdouble v12_11=0.48*1.89371240325272;


// About 26 clocks, err = ~1E-6
FXQuatd fastlerp12(const FXQuatd& q0,const FXQuatd& q1,FXdouble t){
  FXdouble xm1=q0.x*q1.x+q0.y*q1.y+q0.z*q1.z+q0.w*q1.w-1.0;      // x-1 = cos(theta)-1
  FXdouble d=1.0-t;
  FXdouble sqrT=t*t;
  FXdouble sqrD=d*d;
  FXdouble bD0=(u12_0*sqrD-v12_0)*xm1;
  FXdouble bD1=(u12_1*sqrD-v12_1)*xm1;
  FXdouble bD2=(u12_2*sqrD-v12_2)*xm1;
  FXdouble bD3=(u12_3*sqrD-v12_3)*xm1;
  FXdouble bD4=(u12_4*sqrD-v12_4)*xm1;
  FXdouble bD5=(u12_5*sqrD-v12_5)*xm1;
  FXdouble bD6=(u12_6*sqrD-v12_6)*xm1;
  FXdouble bD7=(u12_7*sqrD-v12_7)*xm1;
  FXdouble bD8=(u12_8*sqrD-v12_8)*xm1;
  FXdouble bD9=(u12_9*sqrD-v12_9)*xm1;
  FXdouble bD10=(u12_10*sqrD-v12_10)*xm1;
  FXdouble bD11=(u12_11*sqrD-v12_11)*xm1;
  FXdouble bT0=(u12_0*sqrT-v12_0)*xm1;
  FXdouble bT1=(u12_1*sqrT-v12_1)*xm1;
  FXdouble bT2=(u12_2*sqrT-v12_2)*xm1;
  FXdouble bT3=(u12_3*sqrT-v12_3)*xm1;
  FXdouble bT4=(u12_4*sqrT-v12_4)*xm1;
  FXdouble bT5=(u12_5*sqrT-v12_5)*xm1;
  FXdouble bT6=(u12_6*sqrT-v12_6)*xm1;
  FXdouble bT7=(u12_7*sqrT-v12_7)*xm1;
  FXdouble bT8=(u12_8*sqrT-v12_8)*xm1;
  FXdouble bT9=(u12_9*sqrT-v12_9)*xm1;
  FXdouble bT10=(u12_10*sqrT-v12_10)*xm1;
  FXdouble bT11=(u12_11*sqrT-v12_11)*xm1;
  FXdouble f0=d*(1.0+bD0*(1.0+bD1*(1.0+bD2*(1.0+bD3*(1.0+bD4*(1.0+bD5*(1.0+bD6*(1.0+bD7*(1.0+bD8*(1.0+bD9*(1.0+bD10*(1.0+bD11))))))))))));
  FXdouble f1=t*(1.0+bT0*(1.0+bT1*(1.0+bT2*(1.0+bT3*(1.0+bT4*(1.0+bT5*(1.0+bT6*(1.0+bT7*(1.0+bT8*(1.0+bT9*(1.0+bT10*(1.0+bT11))))))))))));
  FXQuatd result(f0*q0+f1*q1);
  return result;
  }


// 1/(i*(2*i+1)) for i>=1
const FXdouble u16_0=0.333333333333333333333333;
const FXdouble u16_1=0.1;
const FXdouble u16_2=0.047619047619047619047619;
const FXdouble u16_3=0.027777777777777777777778;
const FXdouble u16_4=0.018181818181818181818182;
const FXdouble u16_5=0.012820512820512820512820;
const FXdouble u16_6=0.009523809523809523809524;
const FXdouble u16_7=0.007352941176470588235294;
const FXdouble u16_8=0.005847953216374269005848;
const FXdouble u16_9=0.004761904761904761904762;
const FXdouble u16_10=0.003952569169960474308300;
const FXdouble u16_11=0.00333333333333333333333333;
const FXdouble u16_12=0.002849002849002849002849;
const FXdouble u16_13=0.00246305418719211822660099;
const FXdouble u16_14=0.00215053763440860215053763;
const FXdouble u16_15=0.00189393939393939393939394*1.91666919924319;     // Best value if angle <pi/2
//const FXdouble u16_15=0.00189393939393939393939394*1.06647791713476;   // Best value if angle <pi/4

// i/(2*i+1) for i>=1
const FXdouble v16_0=0.333333333333333333333333;
const FXdouble v16_1=0.4;
const FXdouble v16_2=0.428571428571428571428571;
const FXdouble v16_3=0.444444444444444444444444;
const FXdouble v16_4=0.454545454545454545454545;
const FXdouble v16_5=0.461538461538461538461538;
const FXdouble v16_6=0.466666666666666666666667;
const FXdouble v16_7=0.470588235294117647058824;
const FXdouble v16_8=0.473684210526315789473684;
const FXdouble v16_9=0.476190476190476190476190;
const FXdouble v16_10=0.478260869565217391304348;
const FXdouble v16_11=0.48;
const FXdouble v16_12=0.481481481481481481481481;
const FXdouble v16_13=0.482758620689655172413793;
const FXdouble v16_14=0.483870967741935483870968;
const FXdouble v16_15=0.484848484848484848484848*1.91666919924319;       // Best value if angle <pi/2
//const FXdouble v16_15=0.484848484848484848484848*1.06647791713476;     // Best value if angle <pi/4


// About 39 clocks, err = ~3.5E-8
FXQuatd fastlerp16(const FXQuatd& q0,const FXQuatd& q1,FXdouble t){
  FXdouble xm1=q0.x*q1.x+q0.y*q1.y+q0.z*q1.z+q0.w*q1.w-1.0;      // x-1 = cos(theta)-1
  FXdouble d=1.0-t;
  FXdouble sqrT=t*t;
  FXdouble sqrD=d*d;
  FXdouble bD0=(u16_0*sqrD-v16_0)*xm1;
  FXdouble bT0=(u16_0*sqrT-v16_0)*xm1;
  FXdouble bD1=(u16_1*sqrD-v16_1)*xm1;
  FXdouble bT1=(u16_1*sqrT-v16_1)*xm1;
  FXdouble bD2=(u16_2*sqrD-v16_2)*xm1;
  FXdouble bT2=(u16_2*sqrT-v16_2)*xm1;
  FXdouble bD3=(u16_3*sqrD-v16_3)*xm1;
  FXdouble bT3=(u16_3*sqrT-v16_3)*xm1;
  FXdouble bD4=(u16_4*sqrD-v16_4)*xm1;
  FXdouble bT4=(u16_4*sqrT-v16_4)*xm1;
  FXdouble bD5=(u16_5*sqrD-v16_5)*xm1;
  FXdouble bT5=(u16_5*sqrT-v16_5)*xm1;
  FXdouble bD6=(u16_6*sqrD-v16_6)*xm1;
  FXdouble bT6=(u16_6*sqrT-v16_6)*xm1;
  FXdouble bD7=(u16_7*sqrD-v16_7)*xm1;
  FXdouble bT7=(u16_7*sqrT-v16_7)*xm1;
  FXdouble bD8=(u16_8*sqrD-v16_8)*xm1;
  FXdouble bT8=(u16_8*sqrT-v16_8)*xm1;
  FXdouble bD9=(u16_9*sqrD-v16_9)*xm1;
  FXdouble bT9=(u16_9*sqrT-v16_9)*xm1;
  FXdouble bD10=(u16_10*sqrD-v16_10)*xm1;
  FXdouble bT10=(u16_10*sqrT-v16_10)*xm1;
  FXdouble bD11=(u16_11*sqrD-v16_11)*xm1;
  FXdouble bT11=(u16_11*sqrT-v16_11)*xm1;
  FXdouble bD12=(u16_12*sqrD-v16_12)*xm1;
  FXdouble bT12=(u16_12*sqrT-v16_12)*xm1;
  FXdouble bD13=(u16_13*sqrD-v16_13)*xm1;
  FXdouble bT13=(u16_13*sqrT-v16_13)*xm1;
  FXdouble bD14=(u16_14*sqrD-v16_14)*xm1;
  FXdouble bT14=(u16_14*sqrT-v16_14)*xm1;
  FXdouble bD15=(u16_15*sqrD-v16_15)*xm1;
  FXdouble bT15=(u16_15*sqrT-v16_15)*xm1;
  FXdouble f0=d*(1.0+bD0*(1.0+bD1*(1.0+bD2*(1.0+bD3*(1.0+bD4*(1.0+bD5*(1.0+bD6*(1.0+bD7*(1.0+bD8*(1.0+bD9*(1.0+bD10*(1.0+bD11*(1.0+bD12*(1.0+bD13*(1.0+bD14*(1.0+bD15))))))))))))))));
  FXdouble f1=t*(1.0+bT0*(1.0+bT1*(1.0+bT2*(1.0+bT3*(1.0+bT4*(1.0+bT5*(1.0+bT6*(1.0+bT7*(1.0+bT8*(1.0+bT9*(1.0+bT10*(1.0+bT11*(1.0+bT12*(1.0+bT13*(1.0+bT14*(1.0+bT15))))))))))))))));
  FXQuatd result(f0*q0+f1*q1);
  return result;
  }

#if 0

FXQuatd lerp1(const FXQuatd& u,const FXQuatd& v,TDouble f){
  return lerp(u,v,f);
  }


FXQuatd lerp2(const FXQuatd& u,const FXQuatd& v,TDouble f){
  return fastlerp(u,v,f);
  }


// Test fast slerp() vs slerp()
void fastSlerpTest(){
  FXRandom rng(FXThread::ticks());
  FXQuat q1,q2,qf,qs,qd,qworst1,qworst2;
  FXdouble t,dot,err;
  FXdouble eworst=0.0;
  FXdouble tworst=0.0;
  fxmessage("fastSlerpTest:\n");

  for(FXival i=0; i<1000000000L; ++i){
    t=rng.randDouble();
    q1.x=2.0*rng.randDouble()-1.0;
    q1.y=2.0*rng.randDouble()-1.0;
    q1.z=2.0*rng.randDouble()-1.0;
    q1.w=2.0*rng.randDouble()-1.0;
    q2.x=2.0*rng.randDouble()-1.0;
    q2.y=2.0*rng.randDouble()-1.0;
    q2.z=2.0*rng.randDouble()-1.0;
    q2.w=2.0*rng.randDouble()-1.0;
    q1=normalize(q1);
    q2=normalize(q2);
    dot=q1.x*q2.x+q1.y*q2.y+q1.z*q2.z+q1.w*q2.w;
//  if(0.667457216028384<=dot){
    if(0.0<=dot){
      qs=lerp1(q1,q2,t);
      qf=lerp2(q1,q2,t);
      qd=qs-qf;
      err=qd.length2();
      if(err>eworst){
        qworst1=q1;
        qworst2=q2;
        tworst=t;
        eworst=err;
        }
      }
    }
  if(0.0<=eworst){
    qs=lerp(qworst1,qworst2,t);
    qf=fastlerp(qworst1,qworst2,t);
    qd=qs-qf;
    dot=qworst1.x*qworst2.x+qworst1.y*qworst2.y+qworst1.z*qworst2.z+qworst1.w*qworst2.w;
    fxmessage("q1  = (%12.8lf,%12.8lf,%12.8lf,%12.8lf)\n",qworst1.x,qworst1.y,qworst1.z,qworst1.w);
    fxmessage("q2  = (%12.8lf,%12.8lf,%12.8lf,%12.8lf)\n",qworst2.x,qworst2.y,qworst2.z,qworst2.w);
    fxmessage("qs  = (%12.8lf,%12.8lf,%12.8lf,%12.8lf)\n",qs.x,qs.y,qs.z,qs.w);
    fxmessage("qf  = (%12.8lf,%12.8lf,%12.8lf,%12.8lf)\n",qf.x,qf.y,qf.z,qf.w);
    fxmessage("qd  = (%12.8le,%12.8le,%12.8le,%12.8le)\n",qd.x,qd.y,qd.z,qd.w);
    fxmessage("|qs|= %.16lE\n",qs.length());
    fxmessage("|qf|= %.16lE\n",qf.length());
    fxmessage("err = %.16lE\n",Math::sqrt(eworst));
    fxmessage("dot = %.16lE\n",dot);
    fxmessage("arg = %.16lE (%.16lg deg)\n",Math::acos(dot),Math::RTOD*Math::acos(dot));
    fxmessage("t   = %.16lE\n",tworst);
    }
  }
#endif

/*******************************************************************************/

// Cubic hermite quaternion interpolation
FXQuatd hermite(const FXQuatd& q0,const FXVec3d& r0,const FXQuatd& q1,const FXVec3d& r1,FXdouble t){
  FXQuatd w1(r0[0]*0.333333333333333333,r0[1]*0.333333333333333333,r0[2]*0.333333333333333333,0.0);
  FXQuatd w3(r1[0]*0.333333333333333333,r1[1]*0.333333333333333333,r1[2]*0.333333333333333333,0.0);
  FXQuatd w2((w1.exp().unitinvert()*q0.unitinvert()*q1*w3.exp().unitinvert()).log());
  FXdouble t2=t*t;
  FXdouble beta3=t2*t;
  FXdouble beta1=3.0*t-3.0*t2+beta3;
  FXdouble beta2=3.0*t2-2.0*beta3;
  return q0*(w1*beta1).exp()*(w2*beta2).exp()*(w3*beta3).exp();
  }

/*******************************************************************************/

// Estimate angular body rates omega from unit quaternions Q0 and Q1 separated by time dt
//
//                      -1
//          2 * log ( Q0   * Q1 )
//   w   =  ---------------------
//    b              dt
//
// Be aware that we don't know how many full revolutions happened between q0 and q1;
// there may be aliasing!
FXVec3d omegaBody(const FXQuatd& q0,const FXQuatd& q1,FXdouble dt){
  FXVec3d omega(0.0,0.0,0.0);
  FXQuatd diff(q0.unitinvert()*q1);
  FXdouble mag2(diff.x*diff.x+diff.y*diff.y+diff.z*diff.z);
  if(0.0<mag2){
    FXdouble mag(Math::sqrt(mag2));
    FXdouble phi(2.0*Math::atan2(mag,diff.w));
    FXdouble s(Math::wrap(phi)/(mag*dt));       // Wrap angle -PI*2...PI*2 to -PI...PI range
    omega.x=diff.x*s;
    omega.y=diff.y*s;
    omega.z=diff.z*s;
    }
  return omega;
  }

/*******************************************************************************/

// Derivative q' of orientation quaternion q with angular body rates omega (rad/s)
//
//   .
//   Q = 0.5 * Q * w
//
FXQuatd quatDot(const FXQuatd& q,const FXVec3d& omega){
  return FXQuatd( 0.5*(omega.x*q.w-omega.y*q.z+omega.z*q.y),
                  0.5*(omega.x*q.z+omega.y*q.w-omega.z*q.x),
                  0.5*(omega.y*q.x+omega.z*q.w-omega.x*q.y),
                 -0.5*(omega.x*q.x+omega.y*q.y+omega.z*q.z));
  }

/*******************************************************************************/

// Calculate angular acceleration of a body with inertial moments tensor M
// Rotationg about its axes with angular rates omega, under a torque torq.
// Formula is:
//
//   .         -1
//   w    =   M   * ( T   -   w  x  ( M * w )
//    b                        b           b
//
// Where M is inertia tensor:
//
//      ( Ixx   Ixy   Ixz )                                                              T
//  M = ( Iyx   Iyy   Iyz )     , with Ixy == Iyz,  Ixz == Izx,  Iyz == Izy, i.e. M == M
//      ( Izx   Izy   Izz )
//
// In the unlikely case that M is singular, return angular acceleration of 0.
FXVec3d omegaDot(const FXMat3d& M,const FXVec3d& omega,const FXVec3d& torq){
  FXVec3d result(0.0,0.0,0.0);
  FXdouble Ixx=M[0][0];
  FXdouble Ixy=M[0][1];
  FXdouble Ixz=M[0][2];
  FXdouble Iyy=M[1][1];
  FXdouble Iyz=M[1][2];
  FXdouble Izz=M[2][2];
  FXdouble m00=Iyy*Izz-Iyz*Iyz;                 // Cofactors of M
  FXdouble m01=Ixz*Iyz-Ixy*Izz;
  FXdouble m02=Ixy*Iyz-Ixz*Iyy;
  FXdouble m11=Ixx*Izz-Ixz*Ixz;
  FXdouble m12=Ixy*Ixz-Ixx*Iyz;
  FXdouble m22=Ixx*Iyy-Ixy*Ixy;
  FXdouble det=Ixx*m00+Ixy*m01+Ixz*m02;
  FXASSERT(M[0][1]==M[1][0]);
  FXASSERT(M[0][2]==M[2][0]);
  FXASSERT(M[1][2]==M[2][1]);
  if(__likely(det!=0.0)){                       // Check if M is singular
    FXdouble ox=omega.x;
    FXdouble oy=omega.y;
    FXdouble oz=omega.z;
    FXdouble mox=Ixx*ox+Ixy*oy+Ixz*oz;          // M * w
    FXdouble moy=Ixy*ox+Iyy*oy+Iyz*oz;
    FXdouble moz=Ixz*ox+Iyz*oy+Izz*oz;
    FXdouble rhx=torq.x-(oy*moz-oz*moy);        // R = T - w x (M * w)
    FXdouble rhy=torq.y-(oz*mox-ox*moz);
    FXdouble rhz=torq.z-(ox*moy-oy*mox);
    result.x=(m00*rhx+m01*rhy+m02*rhz)/det;     //  -1
    result.y=(m01*rhx+m11*rhy+m12*rhz)/det;     // M   * R
    result.z=(m02*rhx+m12*rhy+m22*rhz)/det;     //
    }
  return result;
  }


// Save vector to stream
FXStream& operator<<(FXStream& store,const FXQuatd& v){
  store << v.x << v.y << v.z << v.w;
  return store;
  }

// Load vector from stream
FXStream& operator>>(FXStream& store,FXQuatd& v){
  store >> v.x >> v.y >> v.z >> v.w;
  return store;
  }

}
