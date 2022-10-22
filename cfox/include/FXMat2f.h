/********************************************************************************
*                                                                               *
*            S i n g l e - P r e c i s i o n   2 x 2   M a t r i x              *
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
#ifndef FXMAT2F_H
#define FXMAT2F_H

namespace FX {


class FXMat3f;


/// Single-precision 2x2 matrix
class FXAPI FXMat2f {
protected:
  FXVec2f m[2];
public:

  /// Default constructor; value is not initialized
  FXMat2f(){}

  /// Initialize matrix from scalar
  FXMat2f(FXfloat s);

  /// Initialize matrix from another matrix
  FXMat2f(const FXMat2f& s);

  /// Initialize from rotation and scaling part of 3x3 matrix
  FXMat2f(const FXMat3f& s);

  /// Initialize matrix from array
  FXMat2f(const FXfloat s[]);

  /// Initialize diagonal matrix
  FXMat2f(FXfloat a,FXfloat b);

  /// Initialize matrix from components
  FXMat2f(FXfloat a00,FXfloat a01,FXfloat a10,FXfloat a11);

  /// Initialize matrix from three vectors
  FXMat2f(const FXVec2f& a,const FXVec2f& b);

  /// Assignment from scalar
  FXMat2f& operator=(FXfloat s);

  /// Assignment
  FXMat2f& operator=(const FXMat2f& s);
  FXMat2f& operator=(const FXMat3f& s);

  /// Assignment from array
  FXMat2f& operator=(const FXfloat s[]);

  /// Set value from scalar
  FXMat2f& set(FXfloat s);

  /// Set value from another matrix
  FXMat2f& set(const FXMat2f& s);

  /// Set from rotation and scaling part of 3x3 matrix
  FXMat2f& set(const FXMat3f& s);

  /// Set value from array
  FXMat2f& set(const FXfloat s[]);

  /// Set diagonal matrix
  FXMat2f& set(FXfloat a,FXfloat b);

  /// Set value from components
  FXMat2f& set(FXfloat a00,FXfloat a01,FXfloat a10,FXfloat a11);

  /// Set value from two vectors
  FXMat2f& set(const FXVec2f& a,const FXVec2f& b);

  /// Assignment operators
  FXMat2f& operator+=(const FXMat2f& s);
  FXMat2f& operator-=(const FXMat2f& s);
  FXMat2f& operator*=(const FXMat2f& s);
  FXMat2f& operator*=(FXfloat s);
  FXMat2f& operator/=(FXfloat s);

  /// Indexing
  FXVec2f& operator[](FXint i){return m[i];}
  const FXVec2f& operator[](FXint i) const {return m[i];}

  /// Conversion
  operator FXfloat*(){return m[0];}
  operator const FXfloat*() const {return m[0];}

  /// Unary minus
  FXMat2f operator-() const;

  /// Set to identity matrix
  FXMat2f& identity();

  /// Return true if identity matrix
  FXbool isIdentity() const;

  /// Multiply by rotation of phi
  FXMat2f& rot(FXfloat c,FXfloat s);
  FXMat2f& rot(FXfloat phi);

  /// Multiply by scaling
  FXMat2f& scale(FXfloat sx,FXfloat sy);
  FXMat2f& scale(FXfloat s);

  /// Determinant
  FXfloat det() const;

  /// Transpose
  FXMat2f transpose() const;

  /// Invert
  FXMat2f invert() const;

  /// Destructor
 ~FXMat2f(){}
  };


/// Matrix times vector
extern FXAPI FXVec2f operator*(const FXMat2f& m,const FXVec2f& v);

/// Vector times matrix
extern FXAPI FXVec2f operator*(const FXVec2f& v,const FXMat2f& m);

/// Matrix and matrix addition
extern FXAPI FXMat2f operator+(const FXMat2f& a,const FXMat2f& b);
extern FXAPI FXMat2f operator-(const FXMat2f& a,const FXMat2f& b);

/// Matrix and matrix multiply
extern FXAPI FXMat2f operator*(const FXMat2f& a,const FXMat2f& b);

/// Scaling
extern FXAPI FXMat2f operator*(FXfloat x,const FXMat2f& a);
extern FXAPI FXMat2f operator*(const FXMat2f& a,FXfloat x);
extern FXAPI FXMat2f operator/(const FXMat2f& a,FXfloat x);
extern FXAPI FXMat2f operator/(FXfloat x,const FXMat2f& a);

/// Equality tests
extern FXAPI FXbool operator==(const FXMat2f& a,const FXMat2f& b);
extern FXAPI FXbool operator!=(const FXMat2f& a,const FXMat2f& b);
extern FXAPI FXbool operator==(const FXMat2f& a,FXfloat n);
extern FXAPI FXbool operator!=(const FXMat2f& a,FXfloat n);
extern FXAPI FXbool operator==(FXfloat n,const FXMat2f& a);
extern FXAPI FXbool operator!=(FXfloat n,const FXMat2f& a);

/// Orthogonalize matrix
extern FXAPI FXMat2f orthogonalize(const FXMat2f& m);

/// Save matrix to a stream
extern FXAPI FXStream& operator<<(FXStream& store,const FXMat2f& m);

/// Load matrix from a stream
extern FXAPI FXStream& operator>>(FXStream& store,FXMat2f& m);

}

#endif
