/********************************************************************************
*                                                                               *
*            D o u b l e - P r e c i s i o n   4 x 4   M a t r i x              *
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
#ifndef FXMAT4D_H
#define FXMAT4D_H

namespace FX {


class FXQuatd;
class FXMat3d;


/// Double-precision 4x4 matrix
class FXAPI FXMat4d {
protected:
  FXVec4d m[4];
public:

  /// Default constructor; value is not initialized
  FXMat4d(){}

  /// Initialize matrix from scalar
  FXMat4d(FXdouble s);

  /// Initialize with 3x3 rotation and scaling matrix
  FXMat4d(const FXMat3d& s);

  /// Initialize matrix from another matrix
  FXMat4d(const FXMat4d& s);

  /// Initialize matrix from array
  FXMat4d(const FXdouble s[]);

  /// Initialize diagonal matrix
  FXMat4d(FXdouble a,FXdouble b,FXdouble c,FXdouble d);

  /// Initialize matrix from components
  FXMat4d(FXdouble a00,FXdouble a01,FXdouble a02,FXdouble a03,
          FXdouble a10,FXdouble a11,FXdouble a12,FXdouble a13,
          FXdouble a20,FXdouble a21,FXdouble a22,FXdouble a23,
          FXdouble a30,FXdouble a31,FXdouble a32,FXdouble a33);

  /// Initialize matrix from four vectors
  FXMat4d(const FXVec4d& a,const FXVec4d& b,const FXVec4d& c,const FXVec4d& d);

  /// Initialize matrix from quaternion
  FXMat4d(const FXQuatd& quat);

  /// Assignment from scalar
  FXMat4d& operator=(FXdouble s);

  /// Assignment
  FXMat4d& operator=(const FXMat3d& s);
  FXMat4d& operator=(const FXMat4d& s);

  /// Assignment from quaternion
  FXMat4d& operator=(const FXQuatd& quat);

  /// Assignment from array
  FXMat4d& operator=(const FXdouble s[]);

  /// Set value from scalar
  FXMat4d& set(FXdouble s);

  /// Set value from 3x3 rotation and scaling matrix
  FXMat4d& set(const FXMat3d& s);

  /// Set value from another matrix
  FXMat4d& set(const FXMat4d& s);

  /// Set value from array
  FXMat4d& set(const FXdouble s[]);

  /// Set diagonal matrix
  FXMat4d& set(FXdouble a,FXdouble b,FXdouble c,FXdouble d);

  /// Set value from components
  FXMat4d& set(FXdouble a00,FXdouble a01,FXdouble a02,FXdouble a03,
               FXdouble a10,FXdouble a11,FXdouble a12,FXdouble a13,
               FXdouble a20,FXdouble a21,FXdouble a22,FXdouble a23,
               FXdouble a30,FXdouble a31,FXdouble a32,FXdouble a33);

  /// Set value from four vectors
  FXMat4d& set(const FXVec4d& a,const FXVec4d& b,const FXVec4d& c,const FXVec4d& d);

  /// Set value from quaternion
  FXMat4d& set(const FXQuatd& quat);

  /// Assignment operators
  FXMat4d& operator+=(const FXMat4d& w);
  FXMat4d& operator-=(const FXMat4d& w);
  FXMat4d& operator*=(const FXMat4d& w);
  FXMat4d& operator*=(FXdouble w);
  FXMat4d& operator/=(FXdouble w);

  /// Indexing
  FXVec4d& operator[](FXint i){return m[i];}
  const FXVec4d& operator[](FXint i) const {return m[i];}

  /// Conversion
  operator FXdouble*(){return m[0];}
  operator const FXdouble*() const {return m[0];}

  /// Unary minus
  FXMat4d operator-() const;

  /// Set to identity matrix
  FXMat4d& identity();

  /// Return true if identity matrix
  FXbool isIdentity() const;

  /// Set orthographic projection from view volume
  FXMat4d& setOrtho(FXdouble xlo,FXdouble xhi,FXdouble ylo,FXdouble yhi,FXdouble zlo,FXdouble zhi);

  /// Get view volume from orthographic projection
  void getOrtho(FXdouble& xlo,FXdouble& xhi,FXdouble& ylo,FXdouble& yhi,FXdouble& zlo,FXdouble& zhi) const;

  /// Set to inverse orthographic projection
  FXMat4d& setInverseOrtho(FXdouble xlo,FXdouble xhi,FXdouble ylo,FXdouble yhi,FXdouble zlo,FXdouble zhi);

  /// Set to perspective projection from view volume
  FXMat4d& setFrustum(FXdouble xlo,FXdouble xhi,FXdouble ylo,FXdouble yhi,FXdouble zlo,FXdouble zhi);

  /// Get view volume from perspective projection
  void getFrustum(FXdouble& xlo,FXdouble& xhi,FXdouble& ylo,FXdouble& yhi,FXdouble& zlo,FXdouble& zhi) const;

  /// Set to inverse perspective projection from view volume
  FXMat4d& setInverseFrustum(FXdouble xlo,FXdouble xhi,FXdouble ylo,FXdouble yhi,FXdouble zlo,FXdouble zhi);

  /// Multiply by left-hand matrix
  FXMat4d& left();

  /// Multiply by rotation matrix
  FXMat4d& rot(const FXMat3d& r);

  /// Multiply by rotation about unit-quaternion
  FXMat4d& rot(const FXQuatd& q);

  /// Multiply by rotation c,s about unit axis
  FXMat4d& rot(const FXVec3d& v,FXdouble c,FXdouble s);

  /// Multiply by rotation of phi about unit axis
  FXMat4d& rot(const FXVec3d& v,FXdouble phi);

  /// Multiply by x-rotation
  FXMat4d& xrot(FXdouble c,FXdouble s);
  FXMat4d& xrot(FXdouble phi);

  /// Multiply by y-rotation
  FXMat4d& yrot(FXdouble c,FXdouble s);
  FXMat4d& yrot(FXdouble phi);

  /// Multiply by z-rotation
  FXMat4d& zrot(FXdouble c,FXdouble s);
  FXMat4d& zrot(FXdouble phi);

  /// Look at
  FXMat4d& look(const FXVec3d& from,const FXVec3d& to,const FXVec3d& up);

  /// Multiply by translation
  FXMat4d& trans(FXdouble tx,FXdouble ty,FXdouble tz);
  FXMat4d& trans(const FXVec3d& v);

  /// Multiply by scaling
  FXMat4d& scale(FXdouble sx,FXdouble sy,FXdouble sz);
  FXMat4d& scale(const FXVec3d& v);
  FXMat4d& scale(FXdouble s);

  /// Determinant
  FXdouble det() const;

  /// Transpose
  FXMat4d transpose() const;

  /// Invert
  FXMat4d invert() const;

  /// Invert affine matrix
  FXMat4d affineInvert() const;

  /// Invert rigid body transform matrix
  FXMat4d rigidInvert() const;

  /// Return normal-transformation matrix
  FXMat3d normalMatrix() const;

  /// Destructor
 ~FXMat4d(){}
  };


/// Matrix times vector
extern FXAPI FXVec3d operator*(const FXMat4d& m,const FXVec3d& v);
extern FXAPI FXVec4d operator*(const FXMat4d& m,const FXVec4d& v);

/// Vector times matrix
extern FXAPI FXVec3d operator*(const FXVec3d& v,const FXMat4d& m);
extern FXAPI FXVec4d operator*(const FXVec4d& v,const FXMat4d& m);

/// Matrix and matrix addition
extern FXAPI FXMat4d operator+(const FXMat4d& a,const FXMat4d& b);
extern FXAPI FXMat4d operator-(const FXMat4d& a,const FXMat4d& b);

/// Matrix and matrix multiply
extern FXAPI FXMat4d operator*(const FXMat4d& a,const FXMat4d& b);

/// Scaling
extern FXAPI FXMat4d operator*(FXdouble x,const FXMat4d& a);
extern FXAPI FXMat4d operator*(const FXMat4d& a,FXdouble x);
extern FXAPI FXMat4d operator/(const FXMat4d& a,FXdouble x);
extern FXAPI FXMat4d operator/(FXdouble x,const FXMat4d& a);

/// Equality tests
extern FXAPI FXbool operator==(const FXMat4d& a,const FXMat4d& b);
extern FXAPI FXbool operator!=(const FXMat4d& a,const FXMat4d& b);
extern FXAPI FXbool operator==(const FXMat4d& a,FXdouble n);
extern FXAPI FXbool operator!=(const FXMat4d& a,FXdouble n);
extern FXAPI FXbool operator==(FXdouble n,const FXMat4d& a);
extern FXAPI FXbool operator!=(FXdouble n,const FXMat4d& a);

/// Orthogonalize matrix
extern FXAPI FXMat4d orthogonalize(const FXMat4d& m);

/// Save matrix to a stream
extern FXAPI FXStream& operator<<(FXStream& store,const FXMat4d& m);

/// Load matrix from a stream
extern FXAPI FXStream& operator>>(FXStream& store,FXMat4d& m);

}

#endif
