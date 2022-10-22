/********************************************************************************
*                                                                               *
*           D o u b l e - P r e c i s i o n    E x t e n t    C l a s s         *
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
#include "FXVec2d.h"
#include "FXExtentd.h"

/*
  Notes:
*/


using namespace FX;

/**************************  E x t e n t   C l a s s   *************************/

namespace FX {

// Longest side
FXdouble FXExtentd::longest() const {
  FXdouble x=upper.x-lower.x;
  FXdouble y=upper.y-lower.y;
  return Math::fmax(x,y);
  }


// Shortest side
FXdouble FXExtentd::shortest() const {
  FXdouble x=upper.x-lower.x;
  FXdouble y=upper.y-lower.y;
  return Math::fmin(x,y);
  }


// Length of diagonal
FXdouble FXExtentd::diameter() const {
  FXdouble x=upper.x-lower.x;
  FXdouble y=upper.y-lower.y;
  return Math::sqrt(x*x+y*y);
  }


// Get radius of box
FXdouble FXExtentd::radius() const {
  return diameter()*0.5;
  }


// Get diagonal of box
FXVec2d FXExtentd::diagonal() const {
  return upper-lower;
  }


// Get center of box
FXVec2d FXExtentd::center() const {
  return 0.5*(upper+lower);
  }


// Test if empty
FXbool FXExtentd::empty() const {
  return upper.x<lower.x || upper.y<lower.y;
  }

// Test if box contains point
FXbool FXExtentd::contains(FXdouble x,FXdouble y) const {
  return lower.x<=x && x<=upper.x && lower.y<=y && y<=upper.y;
  }


// Test if box contains point p
FXbool FXExtentd::contains(const FXVec2d& p) const {
  return lower.x<=p.x && p.x<=upper.x && lower.y<=p.y && p.y<=upper.y;
  }


// Test if box contains another box
FXbool FXExtentd::contains(const FXExtentd& ext) const {
  return lower.x<=ext.lower.x && ext.upper.x<=upper.x && lower.y<=ext.lower.y && ext.upper.y<=upper.y;
  }


// Include point into range
FXExtentd& FXExtentd::include(FXdouble x,FXdouble y){
  lower.x=Math::fmin(x,lower.x);
  lower.y=Math::fmin(y,lower.y);
  upper.x=Math::fmax(x,upper.x);
  upper.y=Math::fmax(y,upper.y);
  return *this;
  }


// Include point into range
FXExtentd& FXExtentd::include(const FXVec2d& v){
  return include(v.x,v.y);
  }


// Include given box into box's range
FXExtentd& FXExtentd::include(const FXExtentd& ext){
  lower.x=Math::fmin(ext.lower.x,lower.x);
  lower.y=Math::fmin(ext.lower.y,lower.y);
  upper.x=Math::fmax(ext.upper.x,upper.x);
  upper.y=Math::fmax(ext.upper.y,upper.y);
  return *this;
  }


// Intersect box with ray u-v
FXbool FXExtentd::intersect(const FXVec2d& u,const FXVec2d& v) const {
  FXdouble hits[2];
  return intersect(u,v-u,hits) && 0.0<=hits[1] && hits[0]<=1.0;
  }


// Intersect box with ray p+lambda*d, returning true if hit
FXbool FXExtentd::intersect(const FXVec2d& pos,const FXVec2d& dir,FXdouble hit[]) const {
  FXdouble f= DBL_MAX;
  FXdouble n=-DBL_MAX;
  FXdouble ni,fi;
  if(__likely(dir.x!=0.0)){
    if(0.0<dir.x){
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
  if(__likely(dir.y!=0.0)){
    if(0.0<dir.y){
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
  hit[0]=n;
  hit[1]=f;
  return true;
  }


// Test if overlap
FXbool overlap(const FXExtentd& a,const FXExtentd& b){
  return a.upper.x>=b.lower.x && a.lower.x<=b.upper.x && a.upper.y>=b.lower.y && a.lower.y<=b.upper.y;
  }


// Union of two boxes
FXExtentd unite(const FXExtentd& a,const FXExtentd& b){
  return FXExtentd(lo(a.lower,b.lower),hi(a.upper,b.upper));
  }


// Intersection of two boxes
FXExtentd intersect(const FXExtentd& a,const FXExtentd& b){
  return FXExtentd(hi(a.lower,b.lower),lo(a.upper,b.upper));
  }


// Saving
FXStream& operator<<(FXStream& store,const FXExtentd& ext){
  store << ext.lower.x << ext.upper.x;
  store << ext.lower.y << ext.upper.y;
  return store;
  }


// Loading
FXStream& operator>>(FXStream& store,FXExtentd& ext){
  store >> ext.lower.x >> ext.upper.x;
  store >> ext.lower.y >> ext.upper.y;
  return store;
  }

}

