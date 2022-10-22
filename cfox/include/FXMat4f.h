/********************************************************************************
*                                                                               *
*            S i n g l e - P r e c i s i o n   4 x 4   M a t r i x              *
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
#ifndef FXMAT4F_H
#define FXMAT4F_H

namespace FX {


class FXQuatf;
class FXMat3f;


/// Single-precision 4x4 matrix
class FXAPI FXMat4f {
protected:
  FXVec4f m[4];
public:

  /// Default constructor; value is not initialized
  FXMat4f(){}

  /// Initialize matrix from scalar
  FXMat4f(FXfloat s);

  /// Initialize with 3x3 rotation and scaling matrix
  FXMat4f(const FXMat3f& s);

  /// Initialize matrix from another matrix
  FXMat4f(const FXMat4f& s);

  /// Initialize matrix from array
  FXMat4f(const FXfloat s[]);

  /// Initialize diagonal matrix
  FXMat4f(FXfloat a,FXfloat b,FXfloat c,FXfloat d);

  /// Initialize matrix from components
  FXMat4f(FXfloat a00,FXfloat a01,FXfloat a02,FXfloat a03,
          FXfloat a10,FXfloat a11,FXfloat a12,FXfloat a13,
          FXfloat a20,FXfloat a21,FXfloat a22,FXfloat a23,
          FXfloat a30,FXfloat a31,FXfloat a32,FXfloat a33);

  /// Initialize matrix from four vectors
  FXMat4f(const FXVec4f& a,const FXVec4f& b,const FXVec4f& c,const FXVec4f& d);

  /// Initialize matrix from quaternion
  FXMat4f(const FXQuatf& quat);

  /// Assignment from scalar
  FXMat4f& operator=(FXfloat s);

  /// Assignment
  FXMat4f& operator=(const FXMat3f& s);
  FXMat4f& operator=(const FXMat4f& s);

  /// Assignment from quaternion
  FXMat4f& operator=(const FXQuatf& quat);

  /// Assignment from array
  FXMat4f& operator=(const FXfloat s[]);

  /// Set value from scalar
  FXMat4f& set(FXfloat s);

  /// Set value from 3x3 rotation and scaling matrix
  FXMat4f& set(const FXMat3f& s);

  /// Set value from another matrix
  FXMat4f& set(const FXMat4f& s);

  /// Set value from array
  FXMat4f& set(const FXfloat s[]);

  /// Set diagonal matrix
  FXMat4f& set(FXfloat a,FXfloat b,FXfloat c,FXfloat d);

  /// Set value from components
  FXMat4f& set(FXfloat a00,FXfloat a01,FXfloat a02,FXfloat a03,
               FXfloat a10,FXfloat a11,FXfloat a12,FXfloat a13,
               FXfloat a20,FXfloat a21,FXfloat a22,FXfloat a23,
               FXfloat a30,FXfloat a31,FXfloat a32,FXfloat a33);

  /// Set value from four vectors
  FXMat4f& set(const FXVec4f& a,const FXVec4f& b,const FXVec4f& c,const FXVec4f& d);

  /// Set value from quaternion
  FXMat4f& set(const FXQuatf& quat);

  /// Assignment operators
  FXMat4f& operator+=(const FXMat4f& s);
  FXMat4f& operator-=(const FXMat4f& s);
  FXMat4f& operator*=(const FXMat4f& s);
  FXMat4f& operator*=(FXfloat s);
  FXMat4f& operator/=(FXfloat s);

  /// Indexing
  FXVec4f& operator[](FXint i){return m[i];}
  const FXVec4f& operator[](FXint i) const {return m[i];}

  /// Conversion
  operator FXfloat*(){return m[0];}
  operator const FXfloat*() const {return m[0];}

  /// Unary minus
  FXMat4f operator-() const;

  /// Set to identity matrix
  FXMat4f& identity();

  /// Return true if identity matrix
  FXbool isIdentity() const;

  /// Set orthographic projection from view volume
  FXMat4f& setOrtho(FXfloat xlo,FXfloat xhi,FXfloat ylo,FXfloat yhi,FXfloat zlo,FXfloat zhi);

  /// Get view volume from orthographic projection
  void getOrtho(FXfloat& xlo,FXfloat& xhi,FXfloat& ylo,FXfloat& yhi,FXfloat& zlo,FXfloat& zhi) const;

  /// Set to inverse orthographic projection
  FXMat4f& setInverseOrtho(FXfloat xlo,FXfloat xhi,FXfloat ylo,FXfloat yhi,FXfloat zlo,FXfloat zhi);

  /// Set to perspective projection from view volume
  FXMat4f& setFrustum(FXfloat xlo,FXfloat xhi,FXfloat ylo,FXfloat yhi,FXfloat zlo,FXfloat zhi);

  /// Get view volume from perspective projection
  void getFrustum(FXfloat& xlo,FXfloat& xhi,FXfloat& ylo,FXfloat& yhi,FXfloat& zlo,FXfloat& zhi) const;

  /// Set to inverse perspective projection from view volume
  FXMat4f& setInverseFrustum(FXfloat xlo,FXfloat xhi,FXfloat ylo,FXfloat yhi,FXfloat zlo,FXfloat zhi);

  /// Multiply by left-hand matrix
  FXMat4f& left();

  /// Multiply by rotation matrix
  FXMat4f& rot(const FXMat3f& r);

  /// Multiply by rotation about unit-quaternion
  FXMat4f& rot(const FXQuatf& q);

  /// Multiply by rotation c,s about unit axis
  FXMat4f& rot(const FXVec3f& v,FXfloat c,FXfloat s);

  /// Multiply by rotation of phi about unit axis
  FXMat4f& rot(const FXVec3f& v,FXfloat phi);

  /// Multiply by x-rotation
  FXMat4f& xrot(FXfloat c,FXfloat s);
  FXMat4f& xrot(FXfloat phi);

  /// Multiply by y-rotation
  FXMat4f& yrot(FXfloat c,FXfloat s);
  FXMat4f& yrot(FXfloat phi);

  /// Multiply by z-rotation
  FXMat4f& zrot(FXfloat c,FXfloat s);
  FXMat4f& zrot(FXfloat phi);

  /// Look at
  FXMat4f& look(const FXVec3f& from,const FXVec3f& to,const FXVec3f& up);

  /// Multiply by translation
  FXMat4f& trans(FXfloat tx,FXfloat ty,FXfloat tz);
  FXMat4f& trans(const FXVec3f& v);

  /// Multiply by scaling
  FXMat4f& scale(FXfloat sx,FXfloat sy,FXfloat sz);
  FXMat4f& scale(const FXVec3f& v);
  FXMat4f& scale(FXfloat s);

  /// Determinant
  FXfloat det() const;

  /// Transpose
  FXMat4f transpose() const;

  /// Invert
  FXMat4f invert() const;

  /// Invert affine matrix
  FXMat4f affineInvert() const;

  /// Invert rigid body transform matrix
  FXMat4f rigidInvert() const;

  /// Return normal-transformation matrix
  FXMat3f normalMatrix() const;

  /// Destructor
 ~FXMat4f(){}
  };


/// Matrix times vector
extern FXAPI FXVec3f operator*(const FXMat4f& m,const FXVec3f& v);
extern FXAPI FXVec4f operator*(const FXMat4f& m,const FXVec4f& v);

/// Vector times matrix
extern FXAPI FXVec3f operator*(const FXVec3f& v,const FXMat4f& m);
extern FXAPI FXVec4f operator*(const FXVec4f& v,const FXMat4f& m);

/// Matrix and matrix addition
extern FXAPI FXMat4f operator+(const FXMat4f& a,const FXMat4f& b);
extern FXAPI FXMat4f operator-(const FXMat4f& a,const FXMat4f& b);

/// Matrix and matrix multiply
extern FXAPI FXMat4f operator*(const FXMat4f& a,const FXMat4f& b);

/// Scaling
extern FXAPI FXMat4f operator*(FXfloat x,const FXMat4f& a);
extern FXAPI FXMat4f operator*(const FXMat4f& a,FXfloat x);
extern FXAPI FXMat4f operator/(const FXMat4f& a,FXfloat x);
extern FXAPI FXMat4f operator/(FXfloat x,const FXMat4f& a);

/// Equality tests
extern FXAPI FXbool operator==(const FXMat4f& a,const FXMat4f& b);
extern FXAPI FXbool operator!=(const FXMat4f& a,const FXMat4f& b);
extern FXAPI FXbool operator==(const FXMat4f& a,FXfloat n);
extern FXAPI FXbool operator!=(const FXMat4f& a,FXfloat n);
extern FXAPI FXbool operator==(FXfloat n,const FXMat4f& a);
extern FXAPI FXbool operator!=(FXfloat n,const FXMat4f& a);

/// Orthogonalize matrix
extern FXAPI FXMat4f orthogonalize(const FXMat4f& m);

/// Save matrix to a stream
extern FXAPI FXStream& operator<<(FXStream& store,const FXMat4f& m);

/// Load matrix from a stream
extern FXAPI FXStream& operator>>(FXStream& store,FXMat4f& m);

}

#endif
