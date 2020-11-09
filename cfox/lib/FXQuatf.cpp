/********************************************************************************
*                                                                               *
*              S i n g l e - P r e c i s i o n  Q u a t e r n i o n             *
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
#include "FXVec2f.h"
#include "FXVec3f.h"
#include "FXVec4f.h"
#include "FXQuatf.h"
#include "FXMat3f.h"
#include "FXMat4f.h"

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
FXQuatf::FXQuatf(const FXVec3f& axis,FXfloat phi){
  setAxisAngle(axis,phi);
  }


// Construct quaternion from arc between two unit vectors fm and to
FXQuatf::FXQuatf(const FXVec3f& fr,const FXVec3f& to){
  set(arc(fr,to));
  }


// Construct from euler angles yaw (z), pitch (y), and roll (x)
FXQuatf::FXQuatf(FXfloat roll,FXfloat pitch,FXfloat yaw){
  setRollPitchYaw(roll,pitch,yaw);
  }


// Construct quaternion from three orthogonal unit vectors
FXQuatf::FXQuatf(const FXVec3f& ex,const FXVec3f& ey,const FXVec3f& ez){
  setAxes(ex,ey,ez);
  }


// Construct quaternion from rotation vector rot, representing a rotation
// by |rot| radians about a unit vector rot/|rot|.
FXQuatf::FXQuatf(const FXVec3f& rot){
  setRotation(rot);
  }


// Adjust quaternion length
FXQuatf& FXQuatf::adjust(){
  FXfloat mag2(x*x+y*y+z*z+w*w);
  if(__likely(0.0f<mag2)){
    FXfloat s(1.0f/Math::sqrt(mag2));
    x*=s;
    y*=s;
    z*=s;
    w*=s;
    }
  return *this;
  }


// Set axis and angle
void FXQuatf::setAxisAngle(const FXVec3f& axis,FXfloat phi){
  FXfloat mag2(axis.length2());
  if(__likely(0.0f<mag2)){
    FXfloat arg(0.5f*phi);
    FXfloat mag(Math::sqrt(mag2));
    FXfloat s(Math::sin(arg)/mag);
    FXfloat c(Math::cos(arg));
    x=axis.x*s;
    y=axis.y*s;
    z=axis.z*s;
    w=c;
    }
  else{
    x=0.0f;
    y=0.0f;
    z=0.0f;
    w=1.0f;
    }
  }


// Obtain axis and angle
// Remeber that: q = sin(A/2)*(x*i+y*j+z*k)+cos(A/2)
void FXQuatf::getAxisAngle(FXVec3f& axis,FXfloat& phi) const {
  FXfloat mag2(x*x+y*y+z*z);
  if(0.0f<mag2){
    FXfloat mag(Math::sqrt(mag2));
    axis.x=x/mag;
    axis.y=y/mag;
    axis.z=z/mag;
    phi=2.0f*Math::atan2(mag,w);
    }
  else{
    axis.x=1.0f;
    axis.y=0.0f;
    axis.z=0.0f;
    phi=0.0f;
    }
  }


// Set quaternion from rotation vector rot
//
//                 |rot|         rot              |rot|
//   Q =  ( sin ( ------- ) *  -------  , cos (  ------- ) )
//                   2          |rot|               2
//
void FXQuatf::setRotation(const FXVec3f& rot){
  FXfloat mag2(rot.length2());
  if(0.0f<mag2){
    FXfloat mag(Math::sqrt(mag2));
    FXfloat arg(mag*0.5f);
    FXfloat s(Math::sin(arg)/mag);
    FXfloat c(Math::cos(arg));
    x=rot.x*s;
    y=rot.y*s;
    z=rot.z*s;
    w=c;
    }
  else{
    x=0.0f;
    y=0.0f;
    z=0.0f;
    w=1.0f;
    }
  }


// Get rotation vector from quaternion
FXVec3f FXQuatf::getRotation() const {
  FXVec3f rot(0.0f,0.0f,0.0f);
  FXfloat mag2(x*x+y*y+z*z);
  if(0.0f<mag2){
    FXfloat mag(Math::sqrt(mag2));
    FXfloat phi(2.0f*Math::atan2(mag,w)/mag);
    rot.x=x*phi*mag;
    rot.y=y*phi*mag;
    rot.z=z*phi*mag;
    }
  return rot;
  }


// Set quaternion from roll (x), pitch (y), yaw (z)
void FXQuatf::setRollPitchYaw(FXfloat roll,FXfloat pitch,FXfloat yaw){
  register FXfloat sr,cr,sp,cp,sy,cy;
  register FXfloat rr=0.5f*roll;
  register FXfloat pp=0.5f*pitch;
  register FXfloat yy=0.5f*yaw;
  sr=Math::sin(rr); cr=Math::cos(rr);
  sp=Math::sin(pp); cp=Math::cos(pp);
  sy=Math::sin(yy); cy=Math::cos(yy);
  x=sr*cp*cy-cr*sp*sy;
  y=cr*sp*cy+sr*cp*sy;
  z=cr*cp*sy-sr*sp*cy;
  w=cr*cp*cy+sr*sp*sy;
  }


// Set quaternion from yaw (z), pitch (y), roll (x)
void FXQuatf::setYawPitchRoll(FXfloat yaw,FXfloat pitch,FXfloat roll){
  register FXfloat sr,cr,sp,cp,sy,cy;
  register FXfloat rr=0.5f*roll;
  register FXfloat pp=0.5f*pitch;
  register FXfloat yy=0.5f*yaw;
  sr=Math::sin(rr); cr=Math::cos(rr);
  sp=Math::sin(pp); cp=Math::cos(pp);
  sy=Math::sin(yy); cy=Math::cos(yy);
  x=sr*cp*cy+cr*sp*sy;
  y=cr*sp*cy-sr*cp*sy;
  z=cr*cp*sy+sr*sp*cy;
  w=cr*cp*cy-sr*sp*sy;
  }


// Set quaternion from roll (x), yaw (z), pitch (y)
void FXQuatf::setRollYawPitch(FXfloat roll,FXfloat yaw,FXfloat pitch){
  register FXfloat sr,cr,sp,cp,sy,cy;
  register FXfloat rr=0.5f*roll;
  register FXfloat pp=0.5f*pitch;
  register FXfloat yy=0.5f*yaw;
  sr=Math::sin(rr); cr=Math::cos(rr);
  sp=Math::sin(pp); cp=Math::cos(pp);
  sy=Math::sin(yy); cy=Math::cos(yy);
  x=cp*cy*sr+sp*sy*cr;
  y=sp*cy*cr+cp*sy*sr;
  z=cp*sy*cr-sp*cy*sr;
  w=cp*cy*cr-sp*sy*sr;
  }


// Set quaternion from pitch (y), roll (x),yaw (z)
void FXQuatf::setPitchRollYaw(FXfloat pitch,FXfloat roll,FXfloat yaw){
  register FXfloat sr,cr,sp,cp,sy,cy;
  register FXfloat rr=0.5f*roll;
  register FXfloat pp=0.5f*pitch;
  register FXfloat yy=0.5f*yaw;
  sr=Math::sin(rr); cr=Math::cos(rr);
  sp=Math::sin(pp); cp=Math::cos(pp);
  sy=Math::sin(yy); cy=Math::cos(yy);
  x=cy*sr*cp-sy*cr*sp;
  y=cy*cr*sp+sy*sr*cp;
  z=cy*sr*sp+sy*cr*cp;
  w=cy*cr*cp-sy*sr*sp;
  }


// Set quaternion from pitch (y), yaw (z), roll (x)
void FXQuatf::setPitchYawRoll(FXfloat pitch,FXfloat yaw,FXfloat roll){
  register FXfloat sr,cr,sp,cp,sy,cy;
  register FXfloat rr=0.5f*roll;
  register FXfloat pp=0.5f*pitch;
  register FXfloat yy=0.5f*yaw;
  sr=Math::sin(rr); cr=Math::cos(rr);
  sp=Math::sin(pp); cp=Math::cos(pp);
  sy=Math::sin(yy); cy=Math::cos(yy);
  x=sr*cy*cp-cr*sy*sp;
  y=cr*cy*sp-sr*sy*cp;
  z=sr*cy*sp+cr*sy*cp;
  w=cr*cy*cp+sr*sy*sp;
  }


// Set quaternion from yaw (z), roll (x), pitch (y)
void FXQuatf::setYawRollPitch(FXfloat yaw,FXfloat roll,FXfloat pitch){
  register FXfloat sr,cr,sp,cp,sy,cy;
  register FXfloat rr=0.5f*roll;
  register FXfloat pp=0.5f*pitch;
  register FXfloat yy=0.5f*yaw;
  sr=Math::sin(rr); cr=Math::cos(rr);
  sp=Math::sin(pp); cp=Math::cos(pp);
  sy=Math::sin(yy); cy=Math::cos(yy);
  x=cp*sr*cy+sp*cr*sy;
  y=sp*cr*cy-cp*sr*sy;
  z=cp*cr*sy-sp*sr*cy;
  w=cp*cr*cy+sp*sr*sy;
  }


// Obtain roll, pitch, yaw
void FXQuatf::getRollPitchYaw(FXfloat& roll,FXfloat& pitch,FXfloat& yaw) const {
  roll=Math::atan2(2.0f*(y*z+w*x),1.0f-2.0f*(x*x+y*y));
  pitch=Math::asin(Math::fclamp(-1.0f,-2.0f*(x*z-w*y),1.0f));
  yaw=Math::atan2(2.0f*(x*y+w*z),1.0f-2.0f*(y*y+z*z));
  }


// Obtain yaw, pitch, and roll
void FXQuatf::getYawPitchRoll(FXfloat& yaw,FXfloat& pitch,FXfloat& roll) const {
  yaw=Math::atan2(-2.0f*(x*y-w*z),1.0f-2.0f*(y*y+z*z));
  pitch=Math::asin(Math::fclamp(-1.0f,2.0f*(x*z+w*y),1.0f));
  roll=Math::atan2(-2.0f*(y*z-w*x),1.0f-2.0f*(x*x+y*y));
  }


// Obtain roll, yaw, pitch
void FXQuatf::getRollYawPitch(FXfloat& roll,FXfloat& yaw,FXfloat& pitch) const {
  roll=Math::atan2(-2.0f*(y*z-w*x),1.0f-2.0f*(x*x+z*z));
  yaw=Math::asin(Math::fclamp(-1.0f,2.0f*(x*y+w*z),1.0f));
  pitch=Math::atan2(-2.0f*(x*z-w*y),1.0f-2.0f*(y*y+z*z));
  }


// Obtain pitch, roll, yaw
void FXQuatf::getPitchRollYaw(FXfloat& pitch,FXfloat& roll,FXfloat& yaw) const {
  pitch=Math::atan2(-2.0f*(x*z-w*y),1.0f-2.0f*(x*x+y*y));
  roll=Math::asin(Math::fclamp(-1.0f,2.0f*(y*z+w*x),1.0f));
  yaw=Math::atan2(-2.0f*(x*y-w*z),1.0f-2.0f*(x*x+z*z));
  }


// Obtain pitch, yaw, roll
void FXQuatf::getPitchYawRoll(FXfloat& pitch,FXfloat& yaw,FXfloat& roll) const {
  pitch=Math::atan2(2.0f*(x*z+w*y),1.0f-2.0f*(y*y+z*z));
  yaw=Math::asin(Math::fclamp(-1.0f,-2.0f*(x*y-w*z),1.0f));
  roll=Math::atan2(2.0f*(y*z+w*x),1.0f-2.0f*(x*x+z*z));
  }


// Obtain yaw, roll, pitch
void FXQuatf::getYawRollPitch(FXfloat& yaw,FXfloat& roll,FXfloat& pitch) const {
  yaw=Math::atan2(2.0f*(x*y+w*z),1.0f-2.0f*(x*x+z*z));
  roll=Math::asin(Math::fclamp(-1.0f,-2.0f*(y*z-w*x),1.0f));
  pitch=Math::atan2(2.0f*(x*z+w*y),1.0f-2.0f*(x*x+y*y));
  }


// Set quaternion from axes
void FXQuatf::setAxes(const FXVec3f& ex,const FXVec3f& ey,const FXVec3f& ez){
  register FXfloat trace=ex.x+ey.y+ez.z;
  register FXfloat scale;
  if(trace>0.0f){
    scale=Math::sqrt(1.0f+trace);
    w=0.5f*scale;
    scale=0.5f/scale;
    x=(ey.z-ez.y)*scale;
    y=(ez.x-ex.z)*scale;
    z=(ex.y-ey.x)*scale;
    }
  else if(ex.x>ey.y && ex.x>ez.z){
    scale=2.0f*Math::sqrt(1.0f+ex.x-ey.y-ez.z);
    x=0.25f*scale;
    y=(ex.y+ey.x)/scale;
    z=(ex.z+ez.x)/scale;
    w=(ey.z-ez.y)/scale;
    }
  else if(ey.y>ez.z){
    scale=2.0f*Math::sqrt(1.0f+ey.y-ex.x-ez.z);
    y=0.25f*scale;
    x=(ex.y+ey.x)/scale;
    z=(ey.z+ez.y)/scale;
    w=(ez.x-ex.z)/scale;
    }
  else{
    scale=2.0f*Math::sqrt(1.0f+ez.z-ex.x-ey.y);
    z=0.25f*scale;
    x=(ex.z+ez.x)/scale;
    y=(ey.z+ez.y)/scale;
    w=(ex.y-ey.x)/scale;
    }
  }


// Get quaternion axes
void FXQuatf::getAxes(FXVec3f& ex,FXVec3f& ey,FXVec3f& ez) const {
  register FXfloat tx=2.0f*x;
  register FXfloat ty=2.0f*y;
  register FXfloat tz=2.0f*z;
  register FXfloat twx=tx*w;
  register FXfloat twy=ty*w;
  register FXfloat twz=tz*w;
  register FXfloat txx=tx*x;
  register FXfloat txy=ty*x;
  register FXfloat txz=tz*x;
  register FXfloat tyy=ty*y;
  register FXfloat tyz=tz*y;
  register FXfloat tzz=tz*z;
  ex.x=1.0f-tyy-tzz;
  ex.y=txy+twz;
  ex.z=txz-twy;
  ey.x=txy-twz;
  ey.y=1.0f-txx-tzz;
  ey.z=tyz+twx;
  ez.x=txz+twy;
  ez.y=tyz-twx;
  ez.z=1.0f-txx-tyy;
  }


// Obtain local x axis
FXVec3f FXQuatf::getXAxis() const {
  register FXfloat ty=2.0f*y;
  register FXfloat tz=2.0f*z;
  return FXVec3f(1.0f-ty*y-tz*z,ty*x+tz*w,tz*x-ty*w);
  }


// Obtain local y axis
FXVec3f FXQuatf::getYAxis() const {
  register FXfloat tx=2.0f*x;
  register FXfloat tz=2.0f*z;
  return FXVec3f(tx*y-tz*w,1.0f-tx*x-tz*z,tz*y+tx*w);
  }


// Obtain local z axis
FXVec3f FXQuatf::getZAxis() const {
  register FXfloat tx=2.0f*x;
  register FXfloat ty=2.0f*y;
  return FXVec3f(tx*z+ty*w,ty*z-tx*w,1.0f-tx*x-ty*y);
  }


// Exponentiate unit quaternion
// Given q = theta*(x*i+y*j+z*k), where length of (x,y,z) is 1,
// then exp(q) = sin(theta)*(x*i+y*j+z*k)+cos(theta).
FXQuatf FXQuatf::exp() const {
  FXQuatf result(0.0f,0.0f,0.0f,1.0f);
  FXfloat mag2(x*x+y*y+z*z);
  if(0.0f<mag2){
    FXfloat mag(Math::sqrt(mag2));
    FXfloat s(Math::sin(mag)/mag);
    FXfloat c(Math::cos(mag));
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
FXQuatf FXQuatf::log() const {
  FXQuatf result(0.0f,0.0f,0.0f,0.0f);
  FXfloat mag2(x*x+y*y+z*z);
  if(0.0f<mag2){
    FXfloat mag(Math::sqrt(mag2));
    FXfloat phi(Math::atan2(mag,w)/mag);
    result.x=x*phi;
    result.y=y*phi;
    result.z=z*phi;
    }
  return result;
  }


// Power of quaternion
FXQuatf FXQuatf::pow(FXfloat t) const {
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
FXVec3f operator*(const FXVec3f& v,const FXQuatf& q){
  FXVec3f s(q.x,q.y,q.z);
  return v+(s^((s^v)+(v*q.w)))*2.0;
  }


// Rotation quaternion and vector q . v = (q* . v . q)
FXVec3f operator*(const FXQuatf& q,const FXVec3f& v){
  FXVec3f s(q.x,q.y,q.z);
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
FXQuatf arc(const FXVec3f& f,const FXVec3f& t){
  register FXfloat dot=f.x*t.x+f.y*t.y+f.z*t.z,div;
  FXQuatf result;
  if(__unlikely(dot> 0.999999f)){       // Unit quaternion
    result.x=0.0f;
    result.y=0.0f;
    result.z=0.0f;
    result.w=1.0f;
    }
  else if(__unlikely(dot<-0.999999f)){  // 180 quaternion (Stephen Hardy)
    if(Math::fabs(f.z)<Math::fabs(f.x) && Math::fabs(f.z)<Math::fabs(f.y)){ // x, y largest magnitude
      result.x= f.x*f.z-f.z*f.y;
      result.y= f.z*f.x+f.y*f.z;
      result.z=-f.y*f.y-f.x*f.x;
      }
    else if(Math::fabs(f.y)<Math::fabs(f.x)){                     // y, z largest magnitude
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
    result.w=0.0f;
    }
  else{
    div=Math::sqrt((dot+1.0f)*2.0f);
    result.x=(f.y*t.z-f.z*t.y)/div;
    result.y=(f.z*t.x-f.x*t.z)/div;
    result.z=(f.x*t.y-f.y*t.x)/div;
    result.w=div*0.5f;
    }
  return result;
  }


// Spherical lerp of unit quaternions u,v
// This is equivalent to: u * (u.unitinvert()*v).pow(f)
FXQuatf lerp(const FXQuatf& u,const FXQuatf& v,FXfloat f){
  register FXfloat dot=u.x*v.x+u.y*v.y+u.z*v.z+u.w*v.w;
  register FXfloat cost=Math::fabs(dot);
  register FXfloat sint;
  register FXfloat fr=1.0f-f;
  register FXfloat to=f;
  register FXfloat theta;
  FXQuatf result;
  if(__likely(cost<0.999999f)){
    sint=Math::sqrt(1.0f-cost*cost);
    theta=Math::atan2(sint,cost);
    fr=Math::sin(fr*theta)/sint;
    to=Math::sin(to*theta)/sint;
    }
  if(dot<0.0f) to=-to;
  result.x=fr*u.x+to*v.x;
  result.y=fr*u.y+to*v.y;
  result.z=fr*u.z+to*v.z;
  result.w=fr*u.w+to*v.w;
  return result;
  }


// Derivative of spherical lerp of unit quaternions u,v
// This is equivalent to: u * (u.unitinvert()*v).pow(f) * (u.unitinvert()*v).log(),
// which is itself equivalent to: lerp(u,v,f) * (u.unitinvert()*v).log()
FXQuatf lerpdot(const FXQuatf& u,const FXQuatf& v,FXfloat f){
  register FXfloat dot=u.x*v.x+u.y*v.y+u.z*v.z+u.w*v.w;
  register FXfloat cost=Math::fabs(dot);
  register FXfloat sint;
  register FXfloat fr=1.0f-f;
  register FXfloat to=f;
  register FXfloat theta;
  FXQuatf result;
  if(__likely(cost<0.999999f)){
    sint=Math::sqrt(1.0f-cost*cost);
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
FXQuatf hermite(const FXQuatf& q0,const FXVec3f& r0,const FXQuatf& q1,const FXVec3f& r1,FXfloat t){
  FXQuatf w1(r0[0]*0.333333333333333333f,r0[1]*0.333333333333333333f,r0[2]*0.333333333333333333f,0.0f);
  FXQuatf w3(r1[0]*0.333333333333333333f,r1[1]*0.333333333333333333f,r1[2]*0.333333333333333333f,0.0f);
  FXQuatf w2((w1.exp().unitinvert()*q0.unitinvert()*q1*w3.exp().unitinvert()).log());
  FXfloat t2=t*t;
  FXfloat beta3=t2*t;
  FXfloat beta1=3.0f*t-3.0f*t2+beta3;
  FXfloat beta2=3.0f*t2-2.0f*beta3;
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
FXVec3f omegaBody(const FXQuatf& q0,const FXQuatf& q1,FXfloat dt){
  FXVec3f omega(0.0f,0.0f,0.0f);
  FXQuatf diff(q0.unitinvert()*q1);
  FXfloat mag2(diff.x*diff.x+diff.y*diff.y+diff.z*diff.z);
  if(0.0f<mag2){
    FXfloat mag(Math::sqrt(mag2));
    FXfloat phi(2.0f*Math::atan2(mag,diff.w));
    FXfloat s(Math::wrap(phi)/(mag*dt));        // Wrap angle -PI*2...PI*2 to -PI...PI range
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
FXQuatf quatDot(const FXQuatf& q,const FXVec3f& omega){
  return FXQuatf( 0.5f*(omega.x*q.w-omega.y*q.z+omega.z*q.y),
                  0.5f*(omega.x*q.z+omega.y*q.w-omega.z*q.x),
                  0.5f*(omega.y*q.x+omega.z*q.w-omega.x*q.y),
                 -0.5f*(omega.x*q.x+omega.y*q.y+omega.z*q.z));
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
FXVec3f omegaDot(const FXMat3f& M,const FXVec3f& omega,const FXVec3f& torq){
  FXVec3f result(0.0f,0.0f,0.0f);
  FXfloat Ixx=M[0][0];
  FXfloat Ixy=M[0][1];
  FXfloat Ixz=M[0][2];
  FXfloat Iyy=M[1][1];
  FXfloat Iyz=M[1][2];
  FXfloat Izz=M[2][2];
  FXfloat m00=Iyy*Izz-Iyz*Iyz;                  // Cofactors of M
  FXfloat m01=Ixz*Iyz-Ixy*Izz;
  FXfloat m02=Ixy*Iyz-Ixz*Iyy;
  FXfloat m11=Ixx*Izz-Ixz*Ixz;
  FXfloat m12=Ixy*Ixz-Ixx*Iyz;
  FXfloat m22=Ixx*Iyy-Ixy*Ixy;
  FXfloat det=Ixx*m00+Ixy*m01+Ixz*m02;
  FXASSERT(M[0][1]==M[1][0]);
  FXASSERT(M[0][2]==M[2][0]);
  FXASSERT(M[1][2]==M[2][1]);
  if(__likely(det!=0.0f)){                       // Check if M is singular
    FXfloat ox=omega.x;
    FXfloat oy=omega.y;
    FXfloat oz=omega.z;
    FXfloat mox=Ixx*ox+Ixy*oy+Ixz*oz;           // M * w
    FXfloat moy=Ixy*ox+Iyy*oy+Iyz*oz;
    FXfloat moz=Ixz*ox+Iyz*oy+Izz*oz;
    FXfloat rhx=torq.x-(oy*moz-oz*moy);         // R = T - w x (M * w)
    FXfloat rhy=torq.y-(oz*mox-ox*moz);
    FXfloat rhz=torq.z-(ox*moy-oy*mox);
    result.x=(m00*rhx+m01*rhy+m02*rhz)/det;     //  -1
    result.y=(m01*rhx+m11*rhy+m12*rhz)/det;     // M   * R
    result.z=(m02*rhx+m12*rhy+m22*rhz)/det;     //
    }
  return result;
  }


// Save vector to stream
FXStream& operator<<(FXStream& store,const FXQuatf& v){
  store << v.x << v.y << v.z << v.w;
  return store;
  }

// Load vector from stream
FXStream& operator>>(FXStream& store,FXQuatf& v){
  store >> v.x >> v.y >> v.z >> v.w;
  return store;
  }

}

