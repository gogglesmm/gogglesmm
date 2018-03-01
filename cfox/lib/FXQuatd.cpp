/********************************************************************************
*                                                                               *
*              D o u b l e - P r e c i s i o n  Q u a t e r n i o n             *
*                                                                               *
*********************************************************************************
* Copyright (C) 1994,2018 by Jeroen van der Zijp.   All Rights Reserved.        *
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


using namespace FX;

/*******************************************************************************/

namespace FX {

// Construct from angle and axis
FXQuatd::FXQuatd(const FXVec3d& axis,FXdouble phi){
  setAxisAngle(axis,phi);
  }


// Construct from roll, pitch, yaw
FXQuatd::FXQuatd(FXdouble roll,FXdouble pitch,FXdouble yaw){
  setRollPitchYaw(roll,pitch,yaw);
  }


// Construct quaternion from two unit vectors
FXQuatd::FXQuatd(const FXVec3d& fr,const FXVec3d& to){
  set(arc(fr,to));
  }


// Construct quaternion from axes
FXQuatd::FXQuatd(const FXVec3d& ex,const FXVec3d& ey,const FXVec3d& ez){
  setAxes(ex,ey,ez);
  }


// Adjust quaternion length
FXQuatd& FXQuatd::adjust(){
  register FXdouble t=x*x+y*y+z*z+w*w;
  register FXdouble f;
  if(__likely(t>0.0)){
    f=1.0/Math::sqrt(t);
    x*=f;
    y*=f;
    z*=f;
    w*=f;
    }
  return *this;
  }


// Set axis and angle
void FXQuatd::setAxisAngle(const FXVec3d& axis,FXdouble phi){
  register FXdouble mag=axis.length();
  register FXdouble a,m;
  if(__likely(0.0<mag)){
    a=0.5*phi;
    m=Math::sin(a)/mag;
    x=axis.x*m;
    y=axis.y*m;
    z=axis.z*m;
    w=Math::cos(a);
    return;
    }
  x=0.0;
  y=0.0;
  z=0.0;
  w=1.0;
  }


// Obtain axis and angle
// Remeber that: q = sin(A/2)*(x*i+y*j+z*k)+cos(A/2)
void FXQuatd::getAxisAngle(FXVec3d& axis,FXdouble& phi) const {
  register FXdouble mag=x*x+y*y+z*z;
  register FXdouble m;
  if(__likely(0.0<mag)){
    m=Math::sqrt(mag);
    axis.x=x/m;
    axis.y=y/m;
    axis.z=z/m;
    phi=2.0*Math::acos(w/Math::sqrt(mag+w*w));
    return;
    }
  axis.x=1.0;
  axis.y=0.0;
  axis.z=0.0;
  phi=0.0;
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
// Math is from "3D Game Engine Design" by David Eberly pp 19-20.
// However, instead of testing asin(Sy) against -PI/2 and PI/2, I
// test Sy against -1 and 1; this is numerically more stable, as
// asin doesn't like arguments outside [-1,1].
void FXQuatd::getRollPitchYaw(FXdouble& roll,FXdouble& pitch,FXdouble& yaw) const {
  register FXdouble s=-2.0*(x*z-w*y);
  if(__likely(s<1.0)){
    if(__likely(-1.0<s)){
      roll=Math::atan2(2.0*(y*z+w*x),1.0-2.0*(x*x+y*y));
      pitch=Math::asin(s);
      yaw=Math::atan2(2.0*(x*y+w*z),1.0-2.0*(y*y+z*z));
      }
    else{
      roll=0.0;
      pitch=-1.57079632679489661923;
      yaw=-Math::atan2(-2.0*(x*y-w*z),2.0*(x*z+w*y));
      }
    }
  else{
    roll=0.0;
    pitch=1.57079632679489661923;
    yaw=Math::atan2(-2.0*(x*y-w*z),2.0*(x*z+w*y));
    }
  }


// Obtain yaw, pitch, and roll
void FXQuatd::getYawPitchRoll(FXdouble& yaw,FXdouble& pitch,FXdouble& roll) const {
  register FXdouble s=2.0*(x*z+w*y);
  if(__likely(s<1.0)){
    if(__likely(-1.0<s)){
      yaw=Math::atan2(-2.0*(x*y-w*z),1.0-2.0*(y*y+z*z));
      pitch=Math::asin(s);
      roll=Math::atan2(-2.0*(y*z-w*x),1.0-2.0*(x*x+y*y));
      }
    else{
      yaw=0.0;
      pitch=-1.57079632679489661923;
      roll=-Math::atan2(2.0*(x*y+w*z),1.0-2.0*(x*x+z*z));
      }
    }
  else{
    yaw=0.0;
    pitch=1.57079632679489661923;
    roll=Math::atan2(2.0*(x*y+w*z),1.0-2.0*(x*x+z*z));
    }
  }


// Obtain roll, yaw, pitch
void FXQuatd::getRollYawPitch(FXdouble& roll,FXdouble& yaw,FXdouble& pitch) const {
  register FXdouble s=2.0*(x*y+w*z);
  if(__likely(s<1.0)){
    if(__likely(-1.0<s)){
      roll=Math::atan2(-2.0*(y*z-w*x),1.0-2.0*(x*x+z*z));
      yaw=Math::asin(s);
      pitch=Math::atan2(-2.0*(x*z-w*y),1.0-2.0*(y*y+z*z));
      }
    else{
      roll=0.0;
      yaw=-1.57079632679489661923;
      pitch=-Math::atan2(2.0*(y*z+w*x),1.0-2.0*(x*x+y*y));
      }
    }
  else{
    roll=0.0;
    yaw=1.57079632679489661923;
    pitch=Math::atan2(2.0*(y*z+w*x),1.0-2.0*(x*x+y*y));
    }
  }


// Obtain pitch, roll, yaw
void FXQuatd::getPitchRollYaw(FXdouble& pitch,FXdouble& roll,FXdouble& yaw) const {
  register FXdouble s=2.0*(y*z+w*x);
  if(__likely(s<1.0)){
    if(__likely(-1.0<s)){
      pitch=Math::atan2(-2.0*(x*z-w*y),1.0-2.0*(x*x+y*y));
      roll=Math::asin(s);
      yaw=Math::atan2(-2.0*(x*y-w*z),1.0-2.0*(x*x+z*z));
      }
    else{
      pitch=0.0;
      roll=-1.57079632679489661923;
      yaw=-Math::atan2(2.0*(x*z+w*y),1.0-2.0*(y*y+z*z));
      }
    }
  else{
    pitch=0.0;
    roll=1.57079632679489661923;
    yaw=Math::atan2(2.0*(x*z+w*y),1.0-2.0*(y*y+z*z));
    }
  }


// Obtain pitch, yaw, roll
void FXQuatd::getPitchYawRoll(FXdouble& pitch,FXdouble& yaw,FXdouble& roll) const {
  register FXdouble s=-2.0*(x*y-w*z);
  if(__likely(s<1.0)){
    if(__likely(-1.0<s)){
      pitch=Math::atan2(2.0*(x*z+w*y),1.0-2.0*(y*y+z*z));
      yaw=Math::asin(s);
      roll=Math::atan2(2.0*(y*z+w*x),1.0-2.0*(x*x+z*z));
      }
    else{
      pitch=0.0;
      yaw=-1.57079632679489661923;
      roll=-Math::atan2(-2.0*(x*z-w*y),1.0-2.0*(x*x+y*y));
      }
    }
  else{
    pitch=0.0;
    yaw=1.57079632679489661923;
    roll=Math::atan2(-2.0*(x*z-w*y),1.0-2.0*(x*x+y*y));
    }
  }


// Obtain yaw, roll, pitch
void FXQuatd::getYawRollPitch(FXdouble& yaw,FXdouble& roll,FXdouble& pitch) const {
  register FXdouble s=-2.0*(y*z-w*x);
  if(__likely(s<1.0)){
    if(__likely(-1.0<s)){
      yaw=Math::atan2(2.0*(x*y+w*z),1.0-2.0*(x*x+z*z));
      roll=Math::asin(s);
      pitch=Math::atan2(2.0*(x*z+w*y),1.0-2.0*(x*x+y*y));
      }
    else{
      yaw=0.0;
      roll=-1.57079632679489661923;
      pitch=-Math::atan2(-2.0*(x*y-w*z),1.0-2.0*(y*y+z*z));
      }
    }
  else{
    yaw=0.0;
    roll=1.57079632679489661923;
    pitch=Math::atan2(-2.0*(x*y-w*z),1.0-2.0*(y*y+z*z));
    }
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
  register FXdouble theta=Math::sqrt(x*x+y*y+z*z);
  register FXdouble scale;
  FXQuatd result(x,y,z,Math::cos(theta));
  if(__likely(0.000000001<theta)){
    scale=Math::sin(theta)/theta;
    result.x*=scale;
    result.y*=scale;
    result.z*=scale;
    }
  return result;
  }


// Take logarithm of unit quaternion
// Given q = sin(theta)*(x*i+y*j+z*k)+cos(theta), length of (x,y,z) is 1,
// then log(q) = theta*(x*i+y*j+z*k).
FXQuatd FXQuatd::log() const {
  register FXdouble scale=Math::sqrt(x*x+y*y+z*z);
  register FXdouble theta=Math::atan2(scale,w);
  FXQuatd result(x,y,z,0.0);
  if(__likely(0.0<scale)){
    scale=theta/scale;
    result.x*=scale;
    result.y*=scale;
    result.z*=scale;
    }
  return result;
  }


// Power of quaternion
FXQuatd FXQuatd::pow(FXdouble t) const {
  return (log()*t).exp();
  }


// Invert quaternion
FXQuatd FXQuatd::invert() const {
  register FXdouble n=length2();
  return FXQuatd(-x/n,-y/n,-z/n,w/n);
  }


// Invert unit quaternion
FXQuatd FXQuatd::unitinvert() const {
  return FXQuatd(-x,-y,-z,w);
  }


// Conjugate quaternion
FXQuatd FXQuatd::conj() const {
  return FXQuatd(-x,-y,-z,w);
  }


// Rotation of a vector by a quaternion; this is defined as q.v.q*
// where q* is the conjugate of q.
// Alternatively, using ^ to denote cross product, it can be expressed as:
//
//   t = 2 * (q.xyz ^ v)
//   v' = v + q.w * t + (q.xyz ^ t)
//
FXVec3d FXQuatd::operator*(const FXVec3d& v) const {
  return v*FXMat3d(*this);
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

