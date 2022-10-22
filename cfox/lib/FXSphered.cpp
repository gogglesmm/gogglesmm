/********************************************************************************
*                                                                               *
*           D o u b l e - P r e c i s i o n    S p h e r e    C l a s s         *
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
#include "FXVec3d.h"
#include "FXVec4d.h"
#include "FXSphered.h"
#include "FXRanged.h"
#include "FXMat3d.h"
#include "FXMat4d.h"

/*
  Notes:
  - Negative radius represents empty bounding sphere.
*/


using namespace FX;

/**************************  S p h e r e   C l a s s   *************************/

namespace FX {


// Initialize from bounding box
FXSphered::FXSphered(const FXRanged& bounds):center(bounds.center()),radius(bounds.diameter()*0.5){
  }


// Test if sphere contains point x,y,z
FXbool FXSphered::contains(FXdouble x,FXdouble y,FXdouble z) const {
  return 0.0<=radius && Math::sqr(center.x-x)+Math::sqr(center.y-y)+Math::sqr(center.z-z)<=Math::sqr(radius);
  }


// Test if sphere contains point p
FXbool FXSphered::contains(const FXVec3d& p) const {
  return contains(p.x,p.y,p.z);
  }


// Test if sphere contains another box
FXbool FXSphered::contains(const FXRanged& box) const {
  if(box.lower.x<=box.upper.x && box.lower.y<=box.upper.y && box.lower.z<=box.upper.z){
    return contains(box.corner(0)) && contains(box.corner(1)) && contains(box.corner(2)) && contains(box.corner(3)) && contains(box.corner(4)) && contains(box.corner(5)) && contains(box.corner(6)) && contains(box.corner(7));
    }
  return false;
  }


// Test if sphere properly contains another sphere
FXbool FXSphered::contains(const FXSphered& sphere) const {
  if(0.0<=sphere.radius && sphere.radius<=radius){
    FXdouble dx=center.x-sphere.center.x;
    FXdouble dy=center.y-sphere.center.y;
    FXdouble dz=center.z-sphere.center.z;
    return sphere.radius+Math::sqrt(dx*dx+dy*dy+dz*dz)<=radius;
    }
  return false;
  }


// Include point
FXSphered& FXSphered::include(FXdouble x,FXdouble y,FXdouble z){
  if(0.0<=radius){
    FXdouble dx=x-center.x;
    FXdouble dy=y-center.y;
    FXdouble dz=z-center.z;
    FXdouble dist=Math::sqrt(dx*dx+dy*dy+dz*dz);
    if(radius<dist){
      FXdouble newradius=0.5*(radius+dist);
      FXdouble delta=(newradius-radius);
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
  radius=0.0;
  return *this;
  }


// Include point
FXSphered& FXSphered::include(const FXVec3d& p){
  return include(p.x,p.y,p.z);
  }


// Expand radius to include point
FXSphered& FXSphered::includeInRadius(FXdouble x,FXdouble y,FXdouble z){
  if(0.0<=radius){
    FXdouble dx=x-center.x;
    FXdouble dy=y-center.y;
    FXdouble dz=z-center.z;
    FXdouble dist=Math::sqrt(dx*dx+dy*dy+dz*dz);
    if(radius<dist) radius=dist;
    return *this;
    }
  center.x=x;
  center.y=y;
  center.z=z;
  radius=0.0;
  return *this;
  }


// Expand radius to include point
FXSphered& FXSphered::includeInRadius(const FXVec3d& p){
  return includeInRadius(p.x,p.y,p.z);
  }


// Include given range into this one
FXSphered& FXSphered::include(const FXRanged& box){
  if(box.lower.x<=box.upper.x && box.lower.y<=box.upper.y && box.lower.z<=box.upper.z){
    if(0.0<=radius){
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
FXSphered& FXSphered::includeInRadius(const FXRanged& box){
  if(box.lower.x<=box.upper.x && box.lower.y<=box.upper.y && box.lower.z<=box.upper.z){
    if(0.0<=radius){
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
FXSphered& FXSphered::include(const FXSphered& sphere){
  if(0.0<=sphere.radius){
    if(0.0<=radius){
      FXdouble dx=sphere.center.x-center.x;
      FXdouble dy=sphere.center.y-center.y;
      FXdouble dz=sphere.center.z-center.z;
      FXdouble dist=Math::sqrt(dx*dx+dy*dy+dz*dz);
      if(sphere.radius<dist+radius){
        if(radius<dist+sphere.radius){
          FXdouble newradius=0.5*(radius+dist+sphere.radius);
          FXdouble delta=(newradius-radius);
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
FXSphered& FXSphered::includeInRadius(const FXSphered& sphere){
  if(0.0<=sphere.radius){
    if(0.0<=radius){
      FXdouble dx=sphere.center.x-center.x;
      FXdouble dy=sphere.center.y-center.y;
      FXdouble dz=sphere.center.z-center.z;
      FXdouble dist=Math::sqrt(dx*dx+dy*dy+dz*dz)+sphere.radius;
      if(radius<dist) radius=dist;
      return *this;
      }
    center=sphere.center;
    radius=sphere.radius;
    }
  return *this;
  }


// Intersect sphere with normalized plane ax+by+cz+w; returns -1,0,+1
FXint FXSphered::intersect(const FXVec4d& plane) const {
  FXdouble dist=plane.distance(center);

  // Upper point on negative side of plane
  if(dist<=-radius) return -1;

  // Lower point on positive side of plane
  if(dist>=radius) return 1;

  // Overlap
  return 0;
  }


// Intersect sphere with ray u-v
FXbool FXSphered::intersect(const FXVec3d& u,const FXVec3d& v) const {
  FXdouble hits[2];
  return intersect(u,v-u,hits) && 0.0<=hits[1] && hits[0]<=1.0;
  }


// Intersect box with ray pos+lambda*dir, returning true if hit
FXbool FXSphered::intersect(const FXVec3d& pos,const FXVec3d& dir,FXdouble hit[]) const {
  if(0.0<=radius){
    FXVec3d m=center-pos;
    FXdouble m2=m.length2();
    FXdouble d2=dir.length2();
    FXdouble r2=radius*radius;
    FXdouble b=dir*m;
    FXdouble disc=b*b-d2*(m2-r2);
    if(0.0<=disc){
      disc=Math::sqrt(disc);
      hit[0]=(-b-disc)/d2;
      hit[1]=(-b+disc)/d2;
      return true;
      }
    }
  return false;
  }


// Test if box overlaps with sphere (QRI Larsson, Moeller, Lengyel)
FXbool overlap(const FXSphered& a,const FXRanged& b){
  if(0.0<=a.radius){
    FXdouble e1,e2,e3;
    if((e1=Math::fmax(b.lower.x-a.center.x,0.0)+Math::fmax(a.center.x-b.upper.x,0.0))>a.radius) return false;
    if((e2=Math::fmax(b.lower.y-a.center.y,0.0)+Math::fmax(a.center.y-b.upper.y,0.0))>a.radius) return false;
    if((e3=Math::fmax(b.lower.z-a.center.z,0.0)+Math::fmax(a.center.z-b.upper.z,0.0))>a.radius) return false;
    return e1*e1+e2*e2+e3*e3<=a.radius*a.radius;
    }
  return false;
  }


// Test if box overlaps with sphere; algorithm due to Arvo (GEMS I)
FXbool overlap(const FXRanged& a,const FXSphered& b){
  return overlap(b,a);
  }


// Test if spheres overlap
FXbool overlap(const FXSphered& a,const FXSphered& b){
  if(0.0<=a.radius && 0.0<=b.radius){
    FXdouble dx=a.center.x-b.center.x;
    FXdouble dy=a.center.y-b.center.y;
    FXdouble dz=a.center.z-b.center.z;
    return (dx*dx+dy*dy+dz*dz)<Math::sqr(a.radius+b.radius);
    }
  return false;
  }


// Transform sphere by 4x4 matrix
FXSphered FXSphered::transform(const FXMat4d& mat) const {
  if(!empty()){
    FXdouble xd=mat[0][0]*mat[0][0]+mat[0][1]*mat[0][1]+mat[0][2]*mat[0][2];
    FXdouble yd=mat[1][0]*mat[1][0]+mat[1][1]*mat[1][1]+mat[1][2]*mat[1][2];
    FXdouble zd=mat[2][0]*mat[2][0]+mat[2][1]*mat[2][1]+mat[2][2]*mat[2][2];
    return FXSphered(center*mat,radius*Math::sqrt(Math::fmax(Math::fmax(xd,yd),zd)));
    }
  return FXSphered(center*mat,radius);
  }


// Saving
FXStream& operator<<(FXStream& store,const FXSphered& sphere){
  store << sphere.center << sphere.radius;
  return store;
  }


// Loading
FXStream& operator>>(FXStream& store,FXSphered& sphere){
  store >> sphere.center >> sphere.radius;
  return store;
  }

}
