/********************************************************************************
*                                                                               *
*            D o u b l e - P r e c i s i o n   3 x 3   M a t r i x              *
*                                                                               *
*********************************************************************************
* Copyright (C) 2003,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#ifndef FXMAT3D_H
#define FXMAT3D_H

namespace FX {


class FXQuatd;
class FXMat2d;
class FXMat4d;


/// Double-precision 3x3 matrix
class FXAPI FXMat3d {
protected:
  FXVec3d m[3];
public:

  /// Default constructor; value is not initialized
  FXMat3d(){}

  /// Initialize matrix from scalar
  FXMat3d(FXdouble s);

  /// Initialize with 2x2 rotation and scale matrix
  FXMat3d(const FXMat2d& s);

  /// Initialize matrix from another matrix
  FXMat3d(const FXMat3d& s);

  /// Initialize from rotation and scaling part of 4x4 matrix
  FXMat3d(const FXMat4d& s);

  /// Initialize matrix from array
  FXMat3d(const FXdouble s[]);

  /// Initialize diagonal matrix
  FXMat3d(FXdouble a,FXdouble b,FXdouble c);

  /// Initialize matrix from components
  FXMat3d(FXdouble a00,FXdouble a01,FXdouble a02,
          FXdouble a10,FXdouble a11,FXdouble a12,
          FXdouble a20,FXdouble a21,FXdouble a22);

  /// Initialize matrix from three vectors
  FXMat3d(const FXVec3d& a,const FXVec3d& b,const FXVec3d& c);

  /// Initialize matrix from quaternion
  FXMat3d(const FXQuatd& quat);

  /// Assignment from scalar
  FXMat3d& operator=(FXdouble s);

  /// Assignment
  FXMat3d& operator=(const FXMat2d& s);
  FXMat3d& operator=(const FXMat3d& s);
  FXMat3d& operator=(const FXMat4d& s);

  /// Assignment from quaternion
  FXMat3d& operator=(const FXQuatd& quat);

  /// Assignment from array
  FXMat3d& operator=(const FXdouble s[]);

  /// Set value from scalar
  FXMat3d& set(FXdouble s);

  /// Set value from 2x2 rotation and scale matrix
  FXMat3d& set(const FXMat2d& s);

  /// Set value from another matrix
  FXMat3d& set(const FXMat3d& s);

  /// Set from rotation and scaling part of 4x4 matrix
  FXMat3d& set(const FXMat4d& s);

  /// Set value from array
  FXMat3d& set(const FXdouble s[]);

  /// Set diagonal matrix
  FXMat3d& set(FXdouble a,FXdouble b,FXdouble c);

  /// Set value from components
  FXMat3d& set(FXdouble a00,FXdouble a01,FXdouble a02,
               FXdouble a10,FXdouble a11,FXdouble a12,
               FXdouble a20,FXdouble a21,FXdouble a22);

  /// Set value from three vectors
  FXMat3d& set(const FXVec3d& a,const FXVec3d& b,const FXVec3d& c);

  /// Set value from quaternion
  FXMat3d& set(const FXQuatd& quat);

  /// Assignment operators
  FXMat3d& operator+=(const FXMat3d& w);
  FXMat3d& operator-=(const FXMat3d& w);
  FXMat3d& operator*=(const FXMat3d& w);
  FXMat3d& operator*=(FXdouble w);
  FXMat3d& operator/=(FXdouble w);

  /// Indexing
  FXVec3d& operator[](FXint i){return m[i];}
  const FXVec3d& operator[](FXint i) const {return m[i];}

  /// Conversion
  operator FXdouble*(){return m[0];}
  operator const FXdouble*() const {return m[0];}

  /// Unary minus
  FXMat3d operator-() const;

  /// Set to identity matrix
  FXMat3d& identity();

  /// Return true if identity matrix
  FXbool isIdentity() const;

  /// Multiply by rotation about unit-quaternion
  FXMat3d& rot(const FXQuatd& q);

  /// Multiply by rotation c,s about unit axis
  FXMat3d& rot(const FXVec3d& v,FXdouble c,FXdouble s);

  /// Multiply by rotation of phi about unit axis
  FXMat3d& rot(const FXVec3d& v,FXdouble phi);

  /// Multiply by x-rotation
  FXMat3d& xrot(FXdouble c,FXdouble s);
  FXMat3d& xrot(FXdouble phi);

  /// Multiply by y-rotation
  FXMat3d& yrot(FXdouble c,FXdouble s);
  FXMat3d& yrot(FXdouble phi);

  /// Multiply by z-rotation
  FXMat3d& zrot(FXdouble c,FXdouble s);
  FXMat3d& zrot(FXdouble phi);

  /// Multiply by scaling
  FXMat3d& scale(FXdouble sx,FXdouble sy,FXdouble sz);
  FXMat3d& scale(const FXVec3d& v);
  FXMat3d& scale(FXdouble s);

  /// Determinant
  FXdouble det() const;

  /// Transpose
  FXMat3d transpose() const;

  /// Invert
  FXMat3d invert() const;

  /// Destructor
 ~FXMat3d(){}
  };


/// Matrix times vector
extern FXAPI FXVec2d operator*(const FXMat3d& m,const FXVec2d& v);
extern FXAPI FXVec3d operator*(const FXMat3d& m,const FXVec3d& v);

/// Vector times matrix
extern FXAPI FXVec2d operator*(const FXVec2d& v,const FXMat3d& m);
extern FXAPI FXVec3d operator*(const FXVec3d& v,const FXMat3d& m);

/// Matrix and matrix addition
extern FXAPI FXMat3d operator+(const FXMat3d& a,const FXMat3d& b);
extern FXAPI FXMat3d operator-(const FXMat3d& a,const FXMat3d& b);

/// Matrix and matrix multiply
extern FXAPI FXMat3d operator*(const FXMat3d& a,const FXMat3d& b);

/// Scaling
extern FXAPI FXMat3d operator*(FXdouble x,const FXMat3d& a);
extern FXAPI FXMat3d operator*(const FXMat3d& a,FXdouble x);
extern FXAPI FXMat3d operator/(const FXMat3d& a,FXdouble x);
extern FXAPI FXMat3d operator/(FXdouble x,const FXMat3d& a);

/// Equality tests
extern FXAPI FXbool operator==(const FXMat3d& a,const FXMat3d& b);
extern FXAPI FXbool operator!=(const FXMat3d& a,const FXMat3d& b);
extern FXAPI FXbool operator==(const FXMat3d& a,FXdouble n);
extern FXAPI FXbool operator!=(const FXMat3d& a,FXdouble n);
extern FXAPI FXbool operator==(FXdouble n,const FXMat3d& a);
extern FXAPI FXbool operator!=(FXdouble n,const FXMat3d& a);

/// Orthogonalize matrix
extern FXAPI FXMat3d orthogonalize(const FXMat3d& m);

/// Save matrix to a stream
extern FXAPI FXStream& operator<<(FXStream& store,const FXMat3d& m);

/// Load matrix from a stream
extern FXAPI FXStream& operator>>(FXStream& store,FXMat3d& m);

}

#endif
