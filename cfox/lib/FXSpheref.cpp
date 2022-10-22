/********************************************************************************
*                                                                               *
*           S i n g l e - P r e c i s i o n    S p h e r e    C l a s s         *
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
#include "FXSpheref.h"
#include "FXRangef.h"
#include "FXMat3f.h"
#include "FXMat4f.h"

/*
  Notes:
  - Negative radius represents empty bounding sphere.
*/


using namespace FX;

/**************************  S p h e r e   C l a s s   *************************/

namespace FX {


// Initialize from bounding box
FXSpheref::FXSpheref(const FXRangef& bounds):center(bounds.center()),radius(bounds.diameter()*0.5f){
  }


// Test if sphere contains point x,y,z
FXbool FXSpheref::contains(FXfloat x,FXfloat y,FXfloat z) const {
  return 0.0f<=radius && Math::sqr(center.x-x)+Math::sqr(center.y-y)+Math::sqr(center.z-z)<=Math::sqr(radius);
  }


// Test if sphere contains point p
FXbool FXSpheref::contains(const FXVec3f& p) const {
  return contains(p.x,p.y,p.z);
  }


// Test if sphere contains another box
FXbool FXSpheref::contains(const FXRangef& box) const {
  if(box.lower.x<=box.upper.x && box.lower.y<=box.upper.y && box.lower.z<=box.upper.z){
    return contains(box.corner(0)) && contains(box.corner(1)) && contains(box.corner(2)) && contains(box.corner(3)) && contains(box.corner(4)) && contains(box.corner(5)) && contains(box.corner(6)) && contains(box.corner(7));
    }
  return false;
  }


// Test if sphere properly contains another sphere
FXbool FXSpheref::contains(const FXSpheref& sphere) const {
  if(0.0f<=sphere.radius && sphere.radius<=radius){
    FXfloat dx=center.x-sphere.center.x;
    FXfloat dy=center.y-sphere.center.y;
    FXfloat dz=center.z-sphere.center.z;
    return sphere.radius+Math::sqrt(dx*dx+dy*dy+dz*dz)<=radius;
    }
  return false;
  }


// Include point
FXSpheref& FXSpheref::include(FXfloat x,FXfloat y,FXfloat z){
  if(0.0f<=radius){
    FXfloat dx=x-center.x;
    FXfloat dy=y-center.y;
    FXfloat dz=z-center.z;
    FXfloat dist=Math::sqrt(dx*dx+dy*dy+dz*dz);
    if(radius<dist){
      FXfloat newradius=0.5f*(radius+dist);
      FXfloat delta=(newradius-radius);
      center.x+=delta*dx/dist;
      center.y+=delta*dy/dist;
      center.z+=delta*dz/dist;
      radius=newradius;
      }
    return *this;
    }
  center.x=x;
  center.y=y;
  center.z=z;
  radius=0.0f;
  return *this;
  }


// Include point
FXSpheref& FXSpheref::include(const FXVec3f& p){
  return include(p.x,p.y,p.z);
  }


// Expand radius to include point
FXSpheref& FXSpheref::includeInRadius(FXfloat x,FXfloat y,FXfloat z){
  if(0.0f<=radius){
    FXfloat dx=x-center.x;
    FXfloat dy=y-center.y;
    FXfloat dz=z-center.z;
    FXfloat dist=Math::sqrt(dx*dx+dy*dy+dz*dz);
    if(radius<dist) radius=dist;
    return *this;
    }
  center.x=x;
  center.y=y;
  center.z=z;
  radius=0.0f;
  return *this;
  }


// Expand radius to include point
FXSpheref& FXSpheref::includeInRadius(const FXVec3f& p){
  return includeInRadius(p.x,p.y,p.z);
  }


// Include given range into this one
FXSpheref& FXSpheref::include(const FXRangef& box){
  if(box.lower.x<=box.upper.x && box.lower.y<=box.upper.y && box.lower.z<=box.upper.z){
    if(0.0f<=radius){
      include(box.corner(0));
      include(box.corner(7));
      include(box.corner(1));
      include(box.corner(6));
      include(box.corner(2));
      include(box.corner(5));
      include(box.corner(3));
      include(box.corner(4));
      return *this;
      }
    center=box.center();
    radius=box.radius();
    }
  return *this;
  }


// Expand radius to include box
FXSpheref& FXSpheref::includeInRadius(const FXRangef& box){
  if(box.lower.x<=box.upper.x && box.lower.y<=box.upper.y && box.lower.z<=box.upper.z){
    if(0.0f<=radius){
      includeInRadius(box.corner(0));
      includeInRadius(box.corner(7));
      includeInRadius(box.corner(1));
      includeInRadius(box.corner(6));
      includeInRadius(box.corner(2));
      includeInRadius(box.corner(5));
      includeInRadius(box.corner(3));
      includeInRadius(box.corner(4));
      return *this;
      }
    center=box.center();
    radius=box.radius();
    }
  return *this;
  }


// Include given sphere into this one
FXSpheref& FXSpheref::include(const FXSpheref& sphere){
  if(0.0f<=sphere.radius){
    if(0.0f<=radius){
      FXfloat dx=sphere.center.x-center.x;
      FXfloat dy=sphere.center.y-center.y;
      FXfloat dz=sphere.center.z-center.z;
      FXfloat dist=Math::sqrt(dx*dx+dy*dy+dz*dz);
      if(sphere.radius<dist+radius){
        if(radius<dist+sphere.radius){
          FXfloat newradius=0.5f*(radius+dist+sphere.radius);
          FXfloat delta=(newradius-radius);
          center.x+=delta*dx/dist;
          center.y+=delta*dy/dist;
          center.z+=delta*dz/dist;
          radius=newradius;
          }
        return *this;
        }
      }
    center=sphere.center;
    radius=sphere.radius;
    }
  return *this;
  }


// Expand radius to include sphere
FXSpheref& FXSpheref::includeInRadius(const FXSpheref& sphere){
  if(0.0f<=sphere.radius){
    if(0.0f<=radius){
      FXfloat dx=sphere.center.x-center.x;
      FXfloat dy=sphere.center.y-center.y;
      FXfloat dz=sphere.center.z-center.z;
      FXfloat dist=Math::sqrt(dx*dx+dy*dy+dz*dz)+sphere.radius;
      if(radius<dist) radius=dist;
      return *this;
      }
    center=sphere.center;
    radius=sphere.radius;
    }
  return *this;
  }


// Intersect sphere with normalized plane ax+by+cz+w; returns -1,0,+1
FXint FXSpheref::intersect(const FXVec4f& plane) const {
  FXfloat dist=plane.distance(center);

  // Upper point on negative side of plane
  if(dist<=-radius) return -1;

  // Lower point on positive side of plane
  if(dist>=radius) return 1;

  // Overlap
  return 0;
  }


// Intersect sphere with ray u-v
FXbool FXSpheref::intersect(const FXVec3f& u,const FXVec3f& v) const {
  FXfloat hits[2];
  return intersect(u,v-u,hits) && 0.0f<=hits[1] && hits[0]<=1.0f;
  }


// Intersect box with ray pos+lambda*dir, returning true if hit
FXbool FXSpheref::intersect(const FXVec3f& pos,const FXVec3f& dir,FXfloat hit[]) const {
  if(0.0f<=radius){
    FXVec3f m=center-pos;
    FXfloat m2=m.length2();
    FXfloat d2=dir.length2();
    FXfloat r2=radius*radius;
    FXfloat b=dir*m;
    FXfloat disc=b*b-d2*(m2-r2);
    if(0.0f<=disc){
      disc=Math::sqrt(disc);
      hit[0]=(-b-disc)/d2;
      hit[1]=(-b+disc)/d2;
      return true;
      }
    }
  return false;
  }


// Test if box overlaps with sphere (QRI Larsson, Moeller, Lengyel)
FXbool overlap(const FXSpheref& a,const FXRangef& b){
  if(0.0f<=a.radius){
    FXfloat e1,e2,e3;
    if((e1=Math::fmax(b.lower.x-a.center.x,0.0f)+Math::fmax(a.center.x-b.upper.x,0.0f))>a.radius) return false;
    if((e2=Math::fmax(b.lower.y-a.center.y,0.0f)+Math::fmax(a.center.y-b.upper.y,0.0f))>a.radius) return false;
    if((e3=Math::fmax(b.lower.z-a.center.z,0.0f)+Math::fmax(a.center.z-b.upper.z,0.0f))>a.radius) return false;
    return e1*e1+e2*e2+e3*e3<=a.radius*a.radius;
    }
  return false;
  }


// Test if box overlaps with sphere; algorithm due to Arvo (GEMS I)
FXbool overlap(const FXRangef& a,const FXSpheref& b){
  return overlap(b,a);
  }


// Test if spheres overlap
FXbool overlap(const FXSpheref& a,const FXSpheref& b){
  if(0.0f<=a.radius && 0.0f<=b.radius){
    FXfloat dx=a.center.x-b.center.x;
    FXfloat dy=a.center.y-b.center.y;
    FXfloat dz=a.center.z-b.center.z;
    return (dx*dx+dy*dy+dz*dz)<Math::sqr(a.radius+b.radius);
    }
  return false;
  }


// Transform sphere by 4x4 matrix
FXSpheref FXSpheref::transform(const FXMat4f& mat) const {
  if(!empty()){
    FXfloat xd=mat[0][0]*mat[0][0]+mat[0][1]*mat[0][1]+mat[0][2]*mat[0][2];
    FXfloat yd=mat[1][0]*mat[1][0]+mat[1][1]*mat[1][1]+mat[1][2]*mat[1][2];
    FXfloat zd=mat[2][0]*mat[2][0]+mat[2][1]*mat[2][1]+mat[2][2]*mat[2][2];
    return FXSpheref(center*mat,radius*Math::sqrt(Math::fmax(Math::fmax(xd,yd),zd)));
    }
  return FXSpheref(center*mat,radius);
  }


// Saving
FXStream& operator<<(FXStream& store,const FXSpheref& sphere){
  store << sphere.center << sphere.radius;
  return store;
  }


// Loading
FXStream& operator>>(FXStream& store,FXSpheref& sphere){
  store >> sphere.center >> sphere.radius;
  return store;
  }

}
