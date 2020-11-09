/********************************************************************************
*                                                                               *
*              D o u b l e - P r e c i s i o n  Q u a t e r n i o n             *
*                                                                               *
*********************************************************************************
* Copyright (C) 1994,2020 by Jeroen van der Zijp.   All Rights Reserved.        *
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


// Adjust quaternion length
FXQuatd& FXQuatd::adjust(){
  FXdouble mag2(x*x+y*y+z*z+w*w);
  if(__likely(0.0<mag2)){
    FXdouble s(1.0/Math::sqrt(mag2));
    x*=s;
    y*=s;
    z*=s;
    w*=s;
    }
  return *this;
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


// Set quaternion from roll (x), pitch (y), yaw (z)
void FXQuatd::setRollPitchYaw(FXdouble roll,FXdouble pitch,FXdouble yaw){
  register FXdouble sr,cr,sp,cp,sy,cy;
  register FXdouble rr=0.5*roll;
  register FXdouble pp=0.5*pitch;
  register FXdouble yy=0.5*yaw;
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
  register FXdouble sr,cr,sp,cp,sy,cy;
  register FXdouble rr=0.5*roll;
  register FXdouble pp=0.5*pitch;
  register FXdouble yy=0.5*yaw;
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
  register FXdouble sr,cr,sp,cp,sy,cy;
  register FXdouble rr=0.5*roll;
  register FXdouble pp=0.5*pitch;
  register FXdouble yy=0.5*yaw;
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
  register FXdouble sr,cr,sp,cp,sy,cy;
  register FXdouble rr=0.5*roll;
  register FXdouble pp=0.5*pitch;
  register FXdouble yy=0.5*yaw;
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
  register FXdouble sr,cr,sp,cp,sy,cy;
  register FXdouble rr=0.5*roll;
  register FXdouble pp=0.5*pitch;
  register FXdouble yy=0.5*yaw;
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
  register FXdouble sr,cr,sp,cp,sy,cy;
  register FXdouble rr=0.5*roll;
  register FXdouble pp=0.5*pitch;
  register FXdouble yy=0.5*yaw;
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
void FXQuatd::setAxes(const FXVec3d& ex,const FXVec3d& ey,const FXVec3d& ez){
  register FXdouble trace=ex.x+ey.y+ez.z;
  register FXdouble scale;
  if(trace>0.0){
    scale=Math::sqrt(1.0+trace);
    w=0.5*scale;
    scale=0.5/scale;
    x=(ey.z-ez.y)*scale;
    y=(ez.x-ex.z)*scale;
    z=(ex.y-ey.x)*scale;
    }
  else if(ex.x>ey.y && ex.x>ez.z){
    scale=2.0*Math::sqrt(1.0+ex.x-ey.y-ez.z);
    x=0.25*scale;
    y=(ex.y+ey.x)/scale;
    z=(ex.z+ez.x)/scale;
    w=(ey.z-ez.y)/scale;
    }
  else if(ey.y>ez.z){
    scale=2.0*Math::sqrt(1.0+ey.y-ex.x-ez.z);
    y=0.25*scale;
    x=(ex.y+ey.x)/scale;
    z=(ey.z+ez.y)/scale;
    w=(ez.x-ex.z)/scale;
    }
  else{
    scale=2.0*Math::sqrt(1.0+ez.z-ex.x-ey.y);
    z=0.25*scale;
    x=(ex.z+ez.x)/scale;
    y=(ey.z+ez.y)/scale;
    w=(ex.y-ey.x)/scale;
    }
  }


// Get quaternion axes
void FXQuatd::getAxes(FXVec3d& ex,FXVec3d& ey,FXVec3d& ez) const {
  register FXdouble tx=2.0*x;
  register FXdouble ty=2.0*y;
  register FXdouble tz=2.0*z;
  register FXdouble twx=tx*w;
  register FXdouble twy=ty*w;
  register FXdouble twz=tz*w;
  register FXdouble txx=tx*x;
  register FXdouble txy=ty*x;
  register FXdouble txz=tz*x;
  register FXdouble tyy=ty*y;
  register FXdouble tyz=tz*y;
  register FXdouble tzz=tz*z;
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
  register FXdouble ty=2.0*y;
  register FXdouble tz=2.0*z;
  return FXVec3d(1.0-ty*y-tz*z,ty*x+tz*w,tz*x-ty*w);
  }


// Obtain local y axis
FXVec3d FXQuatd::getYAxis() const {
  register FXdouble tx=2.0*x;
  register FXdouble tz=2.0*z;
  return FXVec3d(tx*y-tz*w,1.0-tx*x-tz*z,tz*y+tx*w);
  }


// Obtain local z axis
FXVec3d FXQuatd::getZAxis() const {
  register FXdouble tx=2.0*x;
  register FXdouble ty=2.0*y;
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


// Rotation quaternion and vector v . q = (q . v . q*) where q* is
// the conjugate of q; assume |q|=1.  Our version uses a faster formulation.
//
// The Rodriques Formula for rotating a vector V about a unit-axis K is:
//
//    V' = K (K . V) + (K x V) sin(A) - K x (K x V) cos(A)
//
// Given UNIT quaternion q=(S,c), i.e. |q|=1, defined as a imaginary part S with
// |S|=K sin(A/2), where K is a unit vector, and a real part c=cos(A/2), we can obtain,
// after some (well, a LOT of) manipilation:
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


// Rotation quaternion and vector q . v = (q* . v . q)
FXVec3d operator*(const FXQuatd& q,const FXVec3d& v){
  FXVec3d s(q.x,q.y,q.z);
  return v+(((v^s)+(v*q.w))^s)*2.0;     // Yes, -a^b is b^a!
  }


// Construct quaternion from arc a->b on unit sphere.
//
// Explanation: a quaternion which rotates by angle theta about unit axis a
// is specified as:
//
//   q = (a * sin(theta/2), cos(theta/2)).
//
// Assuming is f and t are unit length, we have:
//
//  sin(theta) = | f x t |
//
// and
//
//  cos(theta) = f . t
//
// Using sin(2 * x) = 2 * sin(x) * cos(x), we get:
//
//  a * sin(theta/2) = (f x t) * sin(theta/2) / (2 * sin(theta/2) * cos(theta/2))
//
//                   = (f x t) / (2 * cos(theta/2))
//
// Using cos^2(x)=(1 + cos(2 * x)) / 2, we get:
//
//  4 * cos^2(theta/2) = 2 + 2 * cos(theta)
//
//                     = 2 + 2 * (f . t)
// Ergo:
//
//  2 * cos(theta/2)   = sqrt(2 + 2 * (f . t))
//
FXQuatd arc(const FXVec3d& f,const FXVec3d& t){
  register FXdouble dot=f.x*t.x+f.y*t.y+f.z*t.z,div;
  FXQuatd result;
  if(__unlikely(dot>0.999999999999999)){        // Unit quaternion
    result.x=0.0;
    result.y=0.0;
    result.z=0.0;
    result.w=1.0;
    }
  else if(__unlikely(dot<-0.999999999999999)){  // 180 quaternion (Stephen Hardy)
    if(Math::fabs(f.z)<Math::fabs(f.x) && Math::fabs(f.z)<Math::fabs(f.y)){     // x, y largest magnitude
      result.x= f.x*f.z-f.z*f.y;
      result.y= f.z*f.x+f.y*f.z;
      result.z=-f.y*f.y-f.x*f.x;
      }
    else if(Math::fabs(f.y)<Math::fabs(f.x)){               // y, z largest magnitude
      result.x= f.y*f.z-f.x*f.y;
      result.y= f.x*f.x+f.z*f.z;
      result.z=-f.z*f.y-f.y*f.x;
      }
    else{                                               // x, z largest magnitude
      result.x=-f.z*f.z-f.y*f.y;
      result.y= f.y*f.x-f.x*f.z;
      result.z= f.x*f.y+f.z*f.x;
      }
    dot=result.x*result.x+result.y*result.y+result.z*result.z;
    div=Math::sqrt(dot);
    result.x/=div;
    result.y/=div;
    result.z/=div;
    result.w=0.0;
    }
  else{
    div=Math::sqrt((dot+1.0)*2.0);
    result.x=(f.y*t.z-f.z*t.y)/div;
    result.y=(f.z*t.x-f.x*t.z)/div;
    result.z=(f.x*t.y-f.y*t.x)/div;
    result.w=div*0.5;
    }
  return result;
  }


// Spherical lerp of unit quaternions u,v
// This is equivalent to: u * (u.unitinvert()*v).pow(f)
FXQuatd lerp(const FXQuatd& u,const FXQuatd& v,FXdouble f){
  register FXdouble dot=u.x*v.x+u.y*v.y+u.z*v.z+u.w*v.w;
  register FXdouble cost=Math::fabs(dot);
  register FXdouble sint;
  register FXdouble fr=1.0-f;
  register FXdouble to=f;
  register FXdouble theta;
  FXQuatd result;
  if(__likely(cost<0.999999999999999)){
    sint=Math::sqrt(1.0-cost*cost);
    theta=Math::atan2(sint,cost);
    fr=Math::sin(fr*theta)/sint;
    to=Math::sin(to*theta)/sint;
    }
  if(dot<0.0) to=-to;
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
  register FXdouble dot=u.x*v.x+u.y*v.y+u.z*v.z+u.w*v.w;
  register FXdouble cost=Math::fabs(dot);
  register FXdouble sint;
  register FXdouble fr=1.0-f;
  register FXdouble to=f;
  register FXdouble theta;
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
    FXdouble mox=Ixx*ox+Ixy*oy+Ixz*oz;           // M * w
    FXdouble moy=Ixy*ox+Iyy*oy+Iyz*oz;
    FXdouble moz=Ixz*ox+Iyz*oy+Izz*oz;
    FXdouble rhx=torq.x-(oy*moz-oz*moy);         // R = T - w x (M * w)
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

