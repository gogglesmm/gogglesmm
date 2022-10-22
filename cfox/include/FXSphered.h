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
#ifndef FXSPHERED_H
#define FXSPHERED_H

namespace FX {


class FXRanged;
class FXMat4d;


/// Spherical bounds
class FXAPI FXSphered {
public:
  FXVec3d  center;
  FXdouble radius;
public:

  /// Default constructor; value is not initialized
  FXSphered(){}

  /// Copy constructor
  FXSphered(const FXSphered& sphere):center(sphere.center),radius(sphere.radius){}

  /// Initialize from center and radius
  FXSphered(const FXVec3d& cen,FXdouble rad=0.0):center(cen),radius(rad){}

  /// Initialize from center and radius
  FXSphered(FXdouble x,FXdouble y,FXdouble z,FXdouble rad=0.0):center(x,y,z),radius(rad){}

  /// Initialize sphere to fully contain the given bounding box
  FXSphered(const FXRanged& bounds);

  /// Assignment
  FXSphered& operator=(const FXSphered& sphere){ center=sphere.center; radius=sphere.radius; return *this; }

  /// Set value from another sphere
  FXSphered& set(const FXSphered& sphere){ center=sphere.center; radius=sphere.radius; return *this; }

  /// Set value from center and radius
  FXSphered& set(const FXVec3d& cen,FXdouble rad=0.0){ center=cen; radius=rad; return *this; }

  /// Set value from center and radius
  FXSphered& set(FXdouble x,FXdouble y,FXdouble z,FXdouble rad=0.0){ center.set(x,y,z); radius=rad; return *this; }

  /// Comparison
  FXbool operator==(const FXSphered& s) const { return center==s.center && radius==s.radius;}
  FXbool operator!=(const FXSphered& s) const { return center!=s.center || radius!=s.radius;}

  /// Diameter of sphere
  FXdouble diameter() const { return radius*2.0; }

  /// Area of sphere
  FXdouble area() const { return radius*radius*12.5663706143591729538505735331; }

  /// Volume of sphere
  FXdouble volume() const { return radius*radius*radius*4.18879020478639098461685784437; }

  /// Test if empty
  FXbool empty() const { return radius<0.0; }

  /// Test if sphere contains point x,y,z
  FXbool contains(FXdouble x,FXdouble y,FXdouble z) const;

  /// Test if sphere contains point p
  FXbool contains(const FXVec3d& p) const;

  /// Test if sphere contains another box
  FXbool contains(const FXRanged& box) const;

  /// Test if sphere contains another sphere
  FXbool contains(const FXSphered& sphere) const;

  /// Include point
  FXSphered& include(FXdouble x,FXdouble y,FXdouble z);

  /// Include point
  FXSphered& include(const FXVec3d& p);

  /// Expand radius to include point
  FXSphered& includeInRadius(FXdouble x,FXdouble y,FXdouble z);

  /// Expand radius to include point
  FXSphered& includeInRadius(const FXVec3d& p);

  /// Include given range into this one
  FXSphered& include(const FXRanged& box);

  /// Expand radius to include box
  FXSphered& includeInRadius(const FXRanged& box);

  /// Include given sphere into this one
  FXSphered& include(const FXSphered& sphere);

  /// Expand radius to include sphere
  FXSphered& includeInRadius(const FXSphered& sphere);

  /// Intersect sphere with normalized plane ax+by+cz+w; returns -1,0,+1
  FXint intersect(const FXVec4d& plane) const;

  /// Intersect sphere with ray u-v
  FXbool intersect(const FXVec3d& u,const FXVec3d& v) const;

  /// Intersect box with ray pos+lambda*dir, returning true if hit
  FXbool intersect(const FXVec3d& pos,const FXVec3d& dir,FXdouble hit[]) const;

  /// Transform sphere by 4x4 matrix
  FXSphered transform(const FXMat4d& mat) const;

  /// Destructor
 ~FXSphered(){}
  };


/// Test if box overlaps with sphere
extern FXAPI FXbool overlap(const FXRanged& a,const FXSphered& b);

/// Test if sphere overlaps with box
extern FXAPI FXbool overlap(const FXSphered& a,const FXRanged& b);

/// Test if spheres overlap
extern FXAPI FXbool overlap(const FXSphered& a,const FXSphered& b);

/// Save object to a stream
extern FXAPI FXStream& operator<<(FXStream& store,const FXSphered& sphere);

/// Load object from a stream
extern FXAPI FXStream& operator>>(FXStream& store,FXSphered& sphere);

}

#endif
