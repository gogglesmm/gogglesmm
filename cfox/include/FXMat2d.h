/********************************************************************************
*                                                                               *
*            D o u b l e - P r e c i s i o n   2 x 2   M a t r i x              *
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
#ifndef FXMAT2D_H
#define FXMAT2D_H

namespace FX {


class FXMat3d;


/// Double-precision 2x2 matrix
class FXAPI FXMat2d {
protected:
  FXVec2d m[2];
public:

  /// Default constructor; value is not initialized
  FXMat2d(){}

  /// Initialize matrix from scalar
  FXMat2d(FXdouble s);

  /// Initialize matrix from another matrix
  FXMat2d(const FXMat2d& s);

  /// Initialize from rotation and scaling part of 3x3 matrix
  FXMat2d(const FXMat3d& s);

  /// Initialize matrix from array
  FXMat2d(const FXdouble s[]);

  /// Initialize diagonal matrix
  FXMat2d(FXdouble a,FXdouble b);

  /// Initialize matrix from components
  FXMat2d(FXdouble a00,FXdouble a01,FXdouble a10,FXdouble a11);

  /// Initialize matrix from two vectors
  FXMat2d(const FXVec2d& a,const FXVec2d& b);

  /// Assignment from scalar
  FXMat2d& operator=(FXdouble s);

  /// Assignment
  FXMat2d& operator=(const FXMat2d& s);
  FXMat2d& operator=(const FXMat3d& s);

  /// Assignment from array
  FXMat2d& operator=(const FXdouble s[]);

  /// Set value from scalar
  FXMat2d& set(FXdouble s);

  /// Set value from another matrix
  FXMat2d& set(const FXMat2d& s);

  /// Set from rotation and scaling part of 3x3 matrix
  FXMat2d& set(const FXMat3d& s);

  /// Set value from array
  FXMat2d& set(const FXdouble s[]);

  /// Set diagonal matrix
  FXMat2d& set(FXdouble a,FXdouble b);

  /// Set value from components
  FXMat2d& set(FXdouble a00,FXdouble a01,FXdouble a10,FXdouble a11);

  /// Set value from two vectors
  FXMat2d& set(const FXVec2d& a,const FXVec2d& b);

  /// Assignment operators
  FXMat2d& operator+=(const FXMat2d& s);
  FXMat2d& operator-=(const FXMat2d& s);
  FXMat2d& operator*=(const FXMat2d& s);
  FXMat2d& operator*=(FXdouble s);
  FXMat2d& operator/=(FXdouble s);

  /// Indexing
  FXVec2d& operator[](FXint i){return m[i];}
  const FXVec2d& operator[](FXint i) const {return m[i];}

  /// Conversion
  operator FXdouble*(){return m[0];}
  operator const FXdouble*() const {return m[0];}

  /// Unary minus
  FXMat2d operator-() const;

  /// Set to identity matrix
  FXMat2d& identity();

  /// Return true if identity matrix
  FXbool isIdentity() const;

  /// Multiply by rotation of phi
  FXMat2d& rot(FXdouble c,FXdouble s);
  FXMat2d& rot(FXdouble phi);

  /// Multiply by scaling
  FXMat2d& scale(FXdouble sx,FXdouble sy);
  FXMat2d& scale(FXdouble s);

  /// Determinant
  FXdouble det() const;

  /// Transpose
  FXMat2d transpose() const;

  /// Invert
  FXMat2d invert() const;

  /// Destructor
 ~FXMat2d(){}
  };


/// Matrix times vector
extern FXAPI FXVec2d operator*(const FXMat2d& m,const FXVec2d& v);

/// Vector times matrix
extern FXAPI FXVec2d operator*(const FXVec2d& v,const FXMat2d& m);

/// Matrix and matrix addition
extern FXAPI FXMat2d operator+(const FXMat2d& a,const FXMat2d& b);
extern FXAPI FXMat2d operator-(const FXMat2d& a,const FXMat2d& b);

/// Matrix and matrix multiply
extern FXAPI FXMat2d operator*(const FXMat2d& a,const FXMat2d& b);

/// Scaling
extern FXAPI FXMat2d operator*(FXdouble x,const FXMat2d& a);
extern FXAPI FXMat2d operator*(const FXMat2d& a,FXdouble x);
extern FXAPI FXMat2d operator/(const FXMat2d& a,FXdouble x);
extern FXAPI FXMat2d operator/(FXdouble x,const FXMat2d& a);

/// Equality tests
extern FXAPI FXbool operator==(const FXMat2d& a,const FXMat2d& b);
extern FXAPI FXbool operator!=(const FXMat2d& a,const FXMat2d& b);
extern FXAPI FXbool operator==(const FXMat2d& a,FXdouble n);
extern FXAPI FXbool operator!=(const FXMat2d& a,FXdouble n);
extern FXAPI FXbool operator==(FXdouble n,const FXMat2d& a);
extern FXAPI FXbool operator!=(FXdouble n,const FXMat2d& a);

/// Orthogonalize matrix
extern FXAPI FXMat2d orthogonalize(const FXMat2d& m);

/// Save matrix to a stream
extern FXAPI FXStream& operator<<(FXStream& store,const FXMat2d& m);

/// Load matrix from a stream
extern FXAPI FXStream& operator>>(FXStream& store,FXMat2d& m);

}

#endif
