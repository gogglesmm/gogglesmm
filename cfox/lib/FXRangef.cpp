/********************************************************************************
*                                                                               *
*           S i n g l e - P r e c i s i o n    R a n g e    C l a s s           *
*                                                                               *
*********************************************************************************
* Copyright (C) 2004,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "FXVec2f.h"
#include "FXVec3f.h"
#include "FXVec4f.h"
#include "FXMat4f.h"
#include "FXSpheref.h"
#include "FXRangef.h"

/*
  Notes:
  - Serializes in the same order old FXRange.
*/


using namespace FX;

/**************************  R a n g e   C l a s s   *************************/

namespace FX {

// Initialize from bounding sphere
FXRangef::FXRangef(const FXSpheref& sphere):
  lower(sphere.center.x-sphere.radius,sphere.center.y-sphere.radius,sphere.center.z-sphere.radius),
  upper(sphere.center.x+sphere.radius,sphere.center.y+sphere.radius,sphere.center.z+sphere.radius){
  }


// Longest side
FXfloat FXRangef::longest() const {
  FXfloat x=upper.x-lower.x;
  FXfloat y=upper.y-lower.y;
  FXfloat z=upper.z-lower.z;
  return Math::fmax(Math::fmax(x,y),z);
  }


// Shortest side
FXfloat FXRangef::shortest() const {
  FXfloat x=upper.x-lower.x;
  FXfloat y=upper.y-lower.y;
  FXfloat z=upper.z-lower.z;
  return Math::fmin(Math::fmin(x,y),z);
  }


// Length of diagonal
FXfloat FXRangef::diameter() const {
  FXfloat x=upper.x-lower.x;
  FXfloat y=upper.y-lower.y;
  FXfloat z=upper.z-lower.z;
  return Math::sqrt(x*x+y*y+z*z);
  }


// Get radius of box
FXfloat FXRangef::radius() const {
  return diameter()*0.5f;
  }


// Get diagonal of box
FXVec3f FXRangef::diagonal() const {
  return upper-lower;
  }


// Get center of box
FXVec3f FXRangef::center() const {
  return 0.5f*(upper+lower);
  }


// Test if empty
FXbool FXRangef::empty() const {
  return upper.x<lower.x || upper.y<lower.y || upper.z<lower.z;
  }


// Test if box contains point x,y,z
FXbool FXRangef::contains(FXfloat x,FXfloat y,FXfloat z) const {
  return lower.x<=x && x<=upper.x && lower.y<=y && y<=upper.y && lower.z<=z && z<=upper.z;
  }


// Test if box contains point p
FXbool FXRangef::contains(const FXVec3f& p) const {
  return lower.x<=p.x && p.x<=upper.x && lower.y<=p.y && p.y<=upper.y && lower.z<=p.z && p.z<=upper.z;
  }


// Test if box contains another box
FXbool FXRangef::contains(const FXRangef& bounds) const {
  return lower.x<=bounds.lower.x && bounds.upper.x<=upper.x && lower.y<=bounds.lower.y && bounds.upper.y<=upper.y && lower.z<=bounds.lower.z && bounds.upper.z<=upper.z;
  }


// Test if box contains sphere
FXbool FXRangef::contains(const FXSpheref& sphere) const {
  return lower.x<=sphere.center.x-sphere.radius && sphere.center.x+sphere.radius<=upper.x && lower.y<=sphere.center.y-sphere.radius && sphere.center.y+sphere.radius<=upper.y && lower.z<=sphere.center.z-sphere.radius && sphere.center.z+sphere.radius<=upper.z;
  }


// Include point into range
FXRangef& FXRangef::include(FXfloat x,FXfloat y,FXfloat z){
  lower.x=Math::fmin(x,lower.x);
  lower.y=Math::fmin(y,lower.y);
  lower.z=Math::fmin(z,lower.z);
  upper.x=Math::fmax(x,upper.x);
  upper.y=Math::fmax(y,upper.y);
  upper.z=Math::fmax(z,upper.z);
  return *this;
  }


// Include point into range
FXRangef& FXRangef::include(const FXVec3f& v){
  return include(v.x,v.y,v.z);
  }


// Include given box into box's range
FXRangef& FXRangef::include(const FXRangef& box){
  lower.x=Math::fmin(box.lower.x,lower.x);
  lower.y=Math::fmin(box.lower.y,lower.y);
  lower.z=Math::fmin(box.lower.z,lower.z);
  upper.x=Math::fmax(box.upper.x,upper.x);
  upper.y=Math::fmax(box.upper.y,upper.y);
  upper.z=Math::fmax(box.upper.z,upper.z);
  return *this;
  }


// Include given sphere into this box
FXRangef& FXRangef::include(const FXSpheref& sphere){
  FXVec3f lo(sphere.center.x-sphere.radius,sphere.center.y-sphere.radius,sphere.center.z-sphere.radius);
  FXVec3f hi(sphere.center.x+sphere.radius,sphere.center.y+sphere.radius,sphere.center.z+sphere.radius);
  lower.x=Math::fmin(lo.x,lower.x);
  lower.y=Math::fmin(lo.y,lower.y);
  lower.z=Math::fmin(lo.z,lower.z);
  upper.x=Math::fmax(hi.x,upper.x);
  upper.y=Math::fmax(hi.y,upper.y);
  upper.z=Math::fmax(hi.z,upper.z);
  return *this;
  }


// Test if overlap
FXbool overlap(const FXRangef& a,const FXRangef& b){
  return a.upper.x>=b.lower.x && a.lower.x<=b.upper.x && a.upper.y>=b.lower.y && a.lower.y<=b.upper.y && a.upper.z>=b.lower.z && a.lower.z<=b.upper.z;
  }


// Union of two boxes
FXRangef unite(const FXRangef& a,const FXRangef& b){
  return FXRangef(lo(a.lower,b.lower),hi(a.upper,b.upper));
  }


// Intersection of two boxes
FXRangef intersect(const FXRangef& a,const FXRangef& b){
  return FXRangef(hi(a.lower,b.lower),lo(a.upper,b.upper));
  }


// Intersect box with normalized plane ax+by+cz+w; returns -1,0,+1
FXint FXRangef::intersect(const FXVec4f& plane) const {
  FXVec3f lo;
  FXVec3f hi;

  // Diagonal
  if(plane.x>0.0f){
    lo.x=lower.x;
    hi.x=upper.x;
    }
  else{
    lo.x=upper.x;
    hi.x=lower.x;
    }

  if(plane.y>0.0f){
    lo.y=lower.y;
    hi.y=upper.y;
    }
  else{
    lo.y=upper.y;
    hi.y=lower.y;
    }

  if(plane.z>0.0f){
    lo.z=lower.z;
    hi.z=upper.z;
    }
  else{
    lo.z=upper.z;
    hi.z=lower.z;
    }

  // Lower point on positive side of plane
  if(plane.x*lo.x+plane.y*lo.y+plane.z*lo.z+plane.w>=0.0f) return 1;

  // Upper point on negative side of plane
  if(plane.x*hi.x+plane.y*hi.y+plane.z*hi.z+plane.w<=0.0f) return -1;

  // Overlap
  return 0;
  }


// Intersect box with ray u-v
FXbool FXRangef::intersect(const FXVec3f& u,const FXVec3f& v) const {
  FXfloat hits[2];
  return intersect(u,v-u,hits) && 0.0f<=hits[1] && hits[0]<=1.0f;
  }


// Intersect box with ray pos+lambda*dir, returning true if hit
FXbool FXRangef::intersect(const FXVec3f& pos,const FXVec3f& dir,FXfloat hit[]) const {
  FXfloat f= FLT_MAX;
  FXfloat n=-FLT_MAX;
  FXfloat ni,fi;
  if(__likely(dir.x!=0.0f)){
    if(0.0f<dir.x){
      ni=(lower.x-pos.x)/dir.x;
      fi=(upper.x-pos.x)/dir.x;
      }
    else{
      ni=(upper.x-pos.x)/dir.x;
      fi=(lower.x-pos.x)/dir.x;
      }
    if(ni>n) n=ni;
    if(fi<f) f=fi;
    }
  else{
    if((pos.x<lower.x) || (pos.x>upper.x)) return false;
    }
  if(__likely(dir.y!=0.0f)){
    if(0.0f<dir.y){
      ni=(lower.y-pos.y)/dir.y;
      fi=(upper.y-pos.y)/dir.y;
      }
    else{
      ni=(upper.y-pos.y)/dir.y;
      fi=(lower.y-pos.y)/dir.y;
      }
    if(ni>n) n=ni;
    if(fi<f) f=fi;
    if(n>f) return false;
    }
  else{
    if((pos.y<lower.y) || (pos.y>upper.y)) return false;
    }
  if(__likely(dir.z!=0.0f)){
    if(0.0f<dir.z){
      ni=(lower.z-pos.z)/dir.z;
      fi=(upper.z-pos.z)/dir.z;
      }
    else{
      ni=(upper.z-pos.z)/dir.z;
      fi=(lower.z-pos.z)/dir.z;
      }
    if(ni>n) n=ni;
    if(fi<f) f=fi;
    if(n>f) return false;
    }
  else{
    if((pos.z<lower.z) || (pos.z>upper.z)) return false;
    }
  hit[0]=n;
  hit[1]=f;
  return true;
  }


// Transform range by 4x4 matrix
FXRangef FXRangef::transform(const FXMat4f& mat) const {
  FXRangef result(corner(0)*mat);
  result.include(corner(1)*mat);
  result.include(corner(2)*mat);
  result.include(corner(3)*mat);
  result.include(corner(4)*mat);
  result.include(corner(5)*mat);
  result.include(corner(6)*mat);
  result.include(corner(7)*mat);
  return result;
  }


// Saving
FXStream& operator<<(FXStream& store,const FXRangef& bounds){
  store << bounds.lower.x << bounds.upper.x;
  store << bounds.lower.y << bounds.upper.y;
  store << bounds.lower.z << bounds.upper.z;
  return store;
  }


// Loading
FXStream& operator>>(FXStream& store,FXRangef& bounds){
  store >> bounds.lower.x >> bounds.upper.x;
  store >> bounds.lower.y >> bounds.upper.y;
  store >> bounds.lower.z >> bounds.upper.z;
  return store;
  }

}

