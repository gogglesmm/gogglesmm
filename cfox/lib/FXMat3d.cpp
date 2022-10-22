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
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "fxmath.h"
#include "FXArray.h"
#include "FXHash.h"
#include "FXStream.h"
#include "FXObject.h"
#include "FXVec2d.h"
#include "FXVec3d.h"
#include "FXVec4d.h"
#include "FXQuatd.h"
#include "FXMat2d.h"
#include "FXMat3d.h"
#include "FXMat4d.h"


/*
  Notes:
  - Transformations pre-multiply.
  - Goal is same effect as OpenGL.
*/


using namespace FX;

/*******************************************************************************/

namespace FX {

// Mask bottom 3 elements
#define MMM _mm256_set_epi64x(0,~0,~0,~0)

// More palatable syntax
#define _mm256_storeu_sd(p,x) _mm_storel_pd(p,_mm256_castpd256_pd128(x))
#define _mm256_loadu_sd(p)    _mm256_castpd128_pd256(_mm_load_sd(p))

#define _mm256_store_sd(p,x)  _mm_storel_pd(p,_mm256_castpd256_pd128(x))
#define _mm256_load_sd(p)     _mm256_castpd128_pd256(_mm_load_sd(p))

#define _mm256_div_sd(a,b)    _mm256_castpd128_pd256(_mm_div_sd(_mm256_castpd256_pd128(a),_mm256_castpd256_pd128(b)))
#define _mm256_mul_sd(a,b)    _mm256_castpd128_pd256(_mm_mul_sd(_mm256_castpd256_pd128(a),_mm256_castpd256_pd128(b)))
#define _mm256_add_sd(a,b)    _mm256_castpd128_pd256(_mm_add_sd(_mm256_castpd256_pd128(a),_mm256_castpd256_pd128(b)))
#define _mm256_sub_sd(a,b)    _mm256_castpd128_pd256(_mm_sub_sd(_mm256_castpd256_pd128(a),_mm256_castpd256_pd128(b)))

#define _mm_storeu_sd(p,x)    _mm_store_sd(p,x)
#define _mm_loadu_sd(p)       _mm_load_sd(p)

// Initialize matrix from scalar
FXMat3d::FXMat3d(FXdouble s){
#if defined(FOX_HAS_AVX)
  _mm256_storeu_pd(&m[0][0],_mm256_set1_pd(s));
  _mm256_storeu_pd(&m[1][1],_mm256_set1_pd(s));
  _mm256_storeu_sd(&m[1][1],_mm256_set1_pd(s));
#elif defined(FOX_HAS_SSE2)
  _mm_storeu_pd(&m[0][0],_mm_set1_pd(s));
  _mm_storeu_pd(&m[0][2],_mm_set1_pd(s));
  _mm_storeu_pd(&m[1][1],_mm_set1_pd(s));
  _mm_storeu_pd(&m[2][0],_mm_set1_pd(s));
  _mm_storeu_sd(&m[2][2],_mm_set1_pd(s));
#else
  m[0][0]=s; m[0][1]=s; m[0][2]=s;
  m[1][0]=s; m[1][1]=s; m[1][2]=s;
  m[2][0]=s; m[2][1]=s; m[2][2]=s;
#endif
  }


// Initialize matrix from another matrix
FXMat3d::FXMat3d(const FXMat2d& s){
  m[0][0]=s[0][0]; m[0][1]=s[0][1]; m[0][2]=0.0;
  m[1][0]=s[1][0]; m[1][1]=s[1][1]; m[1][2]=0.0;
  m[2][0]=0.0;     m[2][1]=0.0;     m[2][2]=1.0;
  }


// Initialize matrix from another matrix
FXMat3d::FXMat3d(const FXMat3d& s){
#if defined(FOX_HAS_AVX)
  _mm256_storeu_pd(&m[0][0],_mm256_loadu_pd(&s[0][0]));
  _mm256_storeu_pd(&m[1][1],_mm256_loadu_pd(&s[1][1]));
  _mm256_storeu_sd(&m[2][2],_mm256_loadu_sd(&s[2][2]));
#elif defined(FOX_HAS_SSE2)
  _mm_storeu_pd(&m[0][0],_mm_loadu_pd(&s[0][0]));
  _mm_storeu_pd(&m[0][2],_mm_loadu_pd(&s[0][2]));
  _mm_storeu_pd(&m[1][1],_mm_loadu_pd(&s[1][1]));
  _mm_storeu_pd(&m[2][0],_mm_loadu_pd(&s[2][0]));
  _mm_storeu_sd(&m[2][2],_mm_loadu_sd(&s[2][2]));
#else
  m[0][0]=s[0][0]; m[0][1]=s[0][1]; m[0][2]=s[0][2];
  m[1][0]=s[1][0]; m[1][1]=s[1][1]; m[1][2]=s[1][2];
  m[2][0]=s[2][0]; m[2][1]=s[2][1]; m[2][2]=s[2][2];
#endif
  }


// Initialize from rotation and scaling part of 4x4 matrix
FXMat3d::FXMat3d(const FXMat4d& s){
#if defined(FOX_HAS_SSE2)
  _mm_storeu_pd(&m[0][0],_mm_loadu_pd(&s[0][0])); _mm_store_sd(&m[0][2],_mm_load_sd(&s[0][2]));
  _mm_storeu_pd(&m[1][0],_mm_loadu_pd(&s[1][0])); _mm_store_sd(&m[1][2],_mm_load_sd(&s[1][2]));
  _mm_storeu_pd(&m[2][0],_mm_loadu_pd(&s[2][0])); _mm_store_sd(&m[2][2],_mm_load_sd(&s[2][2]));
#else
  m[0][0]=s[0][0]; m[0][1]=s[0][1]; m[0][2]=s[0][2];
  m[1][0]=s[1][0]; m[1][1]=s[1][1]; m[1][2]=s[1][2];
  m[2][0]=s[2][0]; m[2][1]=s[2][1]; m[2][2]=s[2][2];
#endif
  }


// Initialize matrix from array
FXMat3d::FXMat3d(const FXdouble s[]){
#if defined(FOX_HAS_AVX)
  _mm256_storeu_pd(&m[0][0],_mm256_loadu_pd(&s[0]));
  _mm256_storeu_pd(&m[1][1],_mm256_loadu_pd(&s[4]));
  _mm256_storeu_sd(&m[2][2],_mm256_loadu_sd(&s[8]));
#elif defined(FOX_HAS_SSE2)
  _mm_storeu_pd(&m[0][0],_mm_loadu_pd(&s[0]));
  _mm_storeu_pd(&m[0][2],_mm_loadu_pd(&s[2]));
  _mm_storeu_pd(&m[1][1],_mm_loadu_pd(&s[4]));
  _mm_storeu_pd(&m[2][0],_mm_loadu_pd(&s[6]));
  _mm_storeu_sd(&m[2][2],_mm_loadu_sd(&s[8]));
#else
  m[0][0]=s[0]; m[0][1]=s[1]; m[0][2]=s[2];
  m[1][0]=s[3]; m[1][1]=s[4]; m[1][2]=s[5];
  m[2][0]=s[6]; m[2][1]=s[7]; m[2][2]=s[8];
#endif
  }


// Initialize diagonal matrix
FXMat3d::FXMat3d(FXdouble a,FXdouble b,FXdouble c){
  m[0][0]=a;   m[0][1]=0.0; m[0][2]=0.0;
  m[1][0]=0.0; m[1][1]=b;   m[1][2]=0.0;
  m[2][0]=0.0; m[2][1]=0.0; m[2][2]=c;
  }


// Initialize matrix from components
FXMat3d::FXMat3d(FXdouble a00,FXdouble a01,FXdouble a02,FXdouble a10,FXdouble a11,FXdouble a12,FXdouble a20,FXdouble a21,FXdouble a22){
  m[0][0]=a00; m[0][1]=a01; m[0][2]=a02;
  m[1][0]=a10; m[1][1]=a11; m[1][2]=a12;
  m[2][0]=a20; m[2][1]=a21; m[2][2]=a22;
  }


// Initialize matrix from three vectors
FXMat3d::FXMat3d(const FXVec3d& a,const FXVec3d& b,const FXVec3d& c){
#if defined(FOX_HAS_SSE2)
  _mm_storeu_pd(&m[0][0],_mm_loadu_pd(&a[0])); _mm_store_sd(&m[0][2],_mm_load_sd(&a[2]));
  _mm_storeu_pd(&m[1][0],_mm_loadu_pd(&b[0])); _mm_store_sd(&m[1][2],_mm_load_sd(&b[2]));
  _mm_storeu_pd(&m[2][0],_mm_loadu_pd(&c[0])); _mm_store_sd(&m[2][2],_mm_load_sd(&c[2]));
#else
  m[0]=a;
  m[1]=b;
  m[2]=c;
#endif
  }


// Initialize matrix from quaternion
FXMat3d::FXMat3d(const FXQuatd& quat){
  quat.getAxes(m[0],m[1],m[2]);
  }


// Assign from scalar
FXMat3d& FXMat3d::operator=(FXdouble s){
#if defined(FOX_HAS_AVX)
  _mm256_storeu_pd(&m[0][0],_mm256_set1_pd(s));
  _mm256_storeu_pd(&m[1][1],_mm256_set1_pd(s));
  _mm256_storeu_sd(&m[1][1],_mm256_set1_pd(s));
#elif defined(FOX_HAS_SSE2)
  _mm_storeu_pd(&m[0][0],_mm_set1_pd(s));
  _mm_storeu_pd(&m[0][2],_mm_set1_pd(s));
  _mm_storeu_pd(&m[1][1],_mm_set1_pd(s));
  _mm_storeu_pd(&m[2][0],_mm_set1_pd(s));
  _mm_storeu_sd(&m[2][2],_mm_set1_pd(s));
#else
  m[0][0]=s; m[0][1]=s; m[0][2]=s;
  m[1][0]=s; m[1][1]=s; m[1][2]=s;
  m[2][0]=s; m[2][1]=s; m[2][2]=s;
#endif
  return *this;
  }


// Assign from 2x2 rotation and scale matrix
FXMat3d& FXMat3d::operator=(const FXMat2d& s){
  m[0][0]=s[0][0]; m[0][1]=s[0][1]; m[0][2]=0.0;
  m[1][0]=s[1][0]; m[1][1]=s[1][1]; m[1][2]=0.0;
  m[2][0]=0.0;     m[2][1]=0.0;     m[2][2]=1.0;
  return *this;
  }


// Assignment operator
FXMat3d& FXMat3d::operator=(const FXMat3d& s){
#if defined(FOX_HAS_AVX)
  _mm256_storeu_pd(&m[0][0],_mm256_loadu_pd(&s[0][0]));
  _mm256_storeu_pd(&m[1][1],_mm256_loadu_pd(&s[1][1]));
  _mm256_storeu_sd(&m[2][2],_mm256_loadu_sd(&s[2][2]));
#elif defined(FOX_HAS_SSE2)
  _mm_storeu_pd(&m[0][0],_mm_loadu_pd(&s[0][0]));
  _mm_storeu_pd(&m[0][2],_mm_loadu_pd(&s[0][2]));
  _mm_storeu_pd(&m[1][1],_mm_loadu_pd(&s[1][1]));
  _mm_storeu_pd(&m[2][0],_mm_loadu_pd(&s[2][0]));
  _mm_storeu_sd(&m[2][2],_mm_loadu_sd(&s[2][2]));
#else
  m[0][0]=s[0][0]; m[0][1]=s[0][1]; m[0][2]=s[0][2];
  m[1][0]=s[1][0]; m[1][1]=s[1][1]; m[1][2]=s[1][2];
  m[2][0]=s[2][0]; m[2][1]=s[2][1]; m[2][2]=s[2][2];
#endif
  return *this;
  }


// Assign from rotation and scaling part of 4x4 matrix
FXMat3d& FXMat3d::operator=(const FXMat4d& s){
#if defined(FOX_HAS_SSE2)
  _mm_storeu_pd(&m[0][0],_mm_loadu_pd(&s[0][0])); _mm_store_sd(&m[0][2],_mm_load_sd(&s[0][2]));
  _mm_storeu_pd(&m[1][0],_mm_loadu_pd(&s[1][0])); _mm_store_sd(&m[1][2],_mm_load_sd(&s[1][2]));
  _mm_storeu_pd(&m[2][0],_mm_loadu_pd(&s[2][0])); _mm_store_sd(&m[2][2],_mm_load_sd(&s[2][2]));
#else
  m[0][0]=s[0][0]; m[0][1]=s[0][1]; m[0][2]=s[0][2];
  m[1][0]=s[1][0]; m[1][1]=s[1][1]; m[1][2]=s[1][2];
  m[2][0]=s[2][0]; m[2][1]=s[2][1]; m[2][2]=s[2][2];
#endif
  return *this;
  }


// Assignment from quaternion
FXMat3d& FXMat3d::operator=(const FXQuatd& quat){
  quat.getAxes(m[0],m[1],m[2]);
  return *this;
  }


// Assignment from array
FXMat3d& FXMat3d::operator=(const FXdouble s[]){
#if defined(FOX_HAS_AVX)
  _mm256_storeu_pd(&m[0][0],_mm256_loadu_pd(&s[0]));
  _mm256_storeu_pd(&m[1][1],_mm256_loadu_pd(&s[4]));
  _mm256_storeu_sd(&m[2][2],_mm256_loadu_sd(&s[8]));
#elif defined(FOX_HAS_SSE2)
  _mm_storeu_pd(&m[0][0],_mm_loadu_pd(&s[0]));
  _mm_storeu_pd(&m[0][2],_mm_loadu_pd(&s[2]));
  _mm_storeu_pd(&m[1][1],_mm_loadu_pd(&s[4]));
  _mm_storeu_pd(&m[2][0],_mm_loadu_pd(&s[6]));
  _mm_storeu_sd(&m[2][2],_mm_loadu_sd(&s[8]));
#else
  m[0][0]=s[0]; m[0][1]=s[1]; m[0][2]=s[2];
  m[1][0]=s[3]; m[1][1]=s[4]; m[1][2]=s[5];
  m[2][0]=s[6]; m[2][1]=s[7]; m[2][2]=s[8];
#endif
  return *this;
  }


// Set value from scalar
FXMat3d& FXMat3d::set(FXdouble s){
#if defined(FOX_HAS_AVX)
  _mm256_storeu_pd(&m[0][0],_mm256_set1_pd(s));
  _mm256_storeu_pd(&m[1][1],_mm256_set1_pd(s));
  _mm256_storeu_sd(&m[1][1],_mm256_set1_pd(s));
#elif defined(FOX_HAS_SSE2)
  _mm_storeu_pd(&m[0][0],_mm_set1_pd(s));
  _mm_storeu_pd(&m[0][2],_mm_set1_pd(s));
  _mm_storeu_pd(&m[1][1],_mm_set1_pd(s));
  _mm_storeu_pd(&m[2][0],_mm_set1_pd(s));
  _mm_storeu_sd(&m[2][2],_mm_set1_pd(s));
#else
  m[0][0]=s; m[0][1]=s; m[0][2]=s;
  m[1][0]=s; m[1][1]=s; m[1][2]=s;
  m[2][0]=s; m[2][1]=s; m[2][2]=s;
#endif
  return *this;
  }


// Set value from 2x2 rotation and scale matrix
FXMat3d& FXMat3d::set(const FXMat2d& s){
  m[0][0]=s[0][0]; m[0][1]=s[0][1]; m[0][2]=0.0;
  m[1][0]=s[1][0]; m[1][1]=s[1][1]; m[1][2]=0.0;
  m[2][0]=0.0;     m[2][1]=0.0;     m[2][2]=1.0;
  return *this;
  }


// Set value from another matrix
FXMat3d& FXMat3d::set(const FXMat3d& s){
#if defined(FOX_HAS_AVX)
  _mm256_storeu_pd(&m[0][0],_mm256_loadu_pd(&s[0][0]));
  _mm256_storeu_pd(&m[1][1],_mm256_loadu_pd(&s[1][1]));
  _mm256_storeu_sd(&m[2][2],_mm256_loadu_sd(&s[2][2]));
#elif defined(FOX_HAS_SSE2)
  _mm_storeu_pd(&m[0][0],_mm_loadu_pd(&s[0][0]));
  _mm_storeu_pd(&m[0][2],_mm_loadu_pd(&s[0][2]));
  _mm_storeu_pd(&m[1][1],_mm_loadu_pd(&s[1][1]));
  _mm_storeu_pd(&m[2][0],_mm_loadu_pd(&s[2][0]));
  _mm_storeu_sd(&m[2][2],_mm_loadu_sd(&s[2][2]));
#else
  m[0][0]=s[0][0]; m[0][1]=s[0][1]; m[0][2]=s[0][2];
  m[1][0]=s[1][0]; m[1][1]=s[1][1]; m[1][2]=s[1][2];
  m[2][0]=s[2][0]; m[2][1]=s[2][1]; m[2][2]=s[2][2];
#endif
  return *this;
  }


// Set from rotation and scaling part of 4x4 matrix
FXMat3d& FXMat3d::set(const FXMat4d& s){
#if defined(FOX_HAS_SSE2)
  _mm_storeu_pd(&m[0][0],_mm_loadu_pd(&s[0][0])); _mm_store_sd(&m[0][2],_mm_load_sd(&s[0][2]));
  _mm_storeu_pd(&m[1][0],_mm_loadu_pd(&s[1][0])); _mm_store_sd(&m[1][2],_mm_load_sd(&s[1][2]));
  _mm_storeu_pd(&m[2][0],_mm_loadu_pd(&s[2][0])); _mm_store_sd(&m[2][2],_mm_load_sd(&s[2][2]));
#else
  m[0][0]=s[0][0]; m[0][1]=s[0][1]; m[0][2]=s[0][2];
  m[1][0]=s[1][0]; m[1][1]=s[1][1]; m[1][2]=s[1][2];
  m[2][0]=s[2][0]; m[2][1]=s[2][1]; m[2][2]=s[2][2];
#endif
  return *this;
  }


// Set value from array
FXMat3d& FXMat3d::set(const FXdouble s[]){
#if defined(FOX_HAS_AVX)
  _mm256_storeu_pd(&m[0][0],_mm256_loadu_pd(&s[0]));
  _mm256_storeu_pd(&m[1][1],_mm256_loadu_pd(&s[4]));
  _mm256_storeu_sd(&m[2][2],_mm256_loadu_sd(&s[8]));
#elif defined(FOX_HAS_SSE2)
  _mm_storeu_pd(&m[0][0],_mm_loadu_pd(&s[0]));
  _mm_storeu_pd(&m[0][2],_mm_loadu_pd(&s[2]));
  _mm_storeu_pd(&m[1][1],_mm_loadu_pd(&s[4]));
  _mm_storeu_pd(&m[2][0],_mm_loadu_pd(&s[6]));
  _mm_storeu_sd(&m[2][2],_mm_loadu_sd(&s[8]));
#else
  m[0][0]=s[0]; m[0][1]=s[1]; m[0][2]=s[2];
  m[1][0]=s[3]; m[1][1]=s[4]; m[1][2]=s[5];
  m[2][0]=s[6]; m[2][1]=s[7]; m[2][2]=s[8];
#endif
  return *this;
  }


// Set diagonal matrix
FXMat3d& FXMat3d::set(FXdouble a,FXdouble b,FXdouble c){
  m[0][0]=a;   m[0][1]=0.0; m[0][2]=0.0;
  m[1][0]=0.0; m[1][1]=b;   m[1][2]=0.0;
  m[2][0]=0.0; m[2][1]=0.0; m[2][2]=c;
  return *this;
  }


// Set value from components
FXMat3d& FXMat3d::set(FXdouble a00,FXdouble a01,FXdouble a02,FXdouble a10,FXdouble a11,FXdouble a12,FXdouble a20,FXdouble a21,FXdouble a22){
  m[0][0]=a00; m[0][1]=a01; m[0][2]=a02;
  m[1][0]=a10; m[1][1]=a11; m[1][2]=a12;
  m[2][0]=a20; m[2][1]=a21; m[2][2]=a22;
  return *this;
  }


// Set value from three vectors
FXMat3d& FXMat3d::set(const FXVec3d& a,const FXVec3d& b,const FXVec3d& c){
#if defined(FOX_HAS_SSE2)
  _mm_storeu_pd(&m[0][0],_mm_loadu_pd(&a[0])); _mm_store_sd(&m[0][2],_mm_load_sd(&a[2]));
  _mm_storeu_pd(&m[1][0],_mm_loadu_pd(&b[0])); _mm_store_sd(&m[1][2],_mm_load_sd(&b[2]));
  _mm_storeu_pd(&m[2][0],_mm_loadu_pd(&c[0])); _mm_store_sd(&m[2][2],_mm_load_sd(&c[2]));
#else
  m[0]=a;
  m[1]=b;
  m[2]=c;
#endif
  return *this;
  }


// Set value from quaternion
FXMat3d& FXMat3d::set(const FXQuatd& quat){
  quat.getAxes(m[0],m[1],m[2]);
  return *this;
  }


// Add matrices
FXMat3d& FXMat3d::operator+=(const FXMat3d& w){
#if defined(FOX_HAS_AVX)
  _mm256_storeu_pd(&m[0][0],_mm256_add_pd(_mm256_loadu_pd(&m[0][0]),_mm256_loadu_pd(&w[0][0])));
  _mm256_storeu_pd(&m[1][1],_mm256_add_pd(_mm256_loadu_pd(&m[1][1]),_mm256_loadu_pd(&w[1][1])));
  _mm256_storeu_sd(&m[2][2],_mm256_add_sd(_mm256_loadu_sd(&m[2][2]),_mm256_loadu_sd(&w[2][2])));
#elif defined(FOX_HAS_SSE2)
  _mm_storeu_pd(&m[0][0],_mm_add_pd(_mm_loadu_pd(&m[0][0]),_mm_loadu_pd(&w[0][0])));
  _mm_storeu_pd(&m[0][2],_mm_add_pd(_mm_loadu_pd(&m[0][2]),_mm_loadu_pd(&w[0][2])));
  _mm_storeu_pd(&m[1][1],_mm_add_pd(_mm_loadu_pd(&m[1][1]),_mm_loadu_pd(&w[1][1])));
  _mm_storeu_pd(&m[2][0],_mm_add_pd(_mm_loadu_pd(&m[2][0]),_mm_loadu_pd(&w[2][0])));
  _mm_storeu_sd(&m[2][2],_mm_add_sd(_mm_loadu_sd(&m[2][2]),_mm_loadu_sd(&w[2][2])));
#else
  m[0][0]+=w[0][0]; m[0][1]+=w[0][1]; m[0][2]+=w[0][2];
  m[1][0]+=w[1][0]; m[1][1]+=w[1][1]; m[1][2]+=w[1][2];
  m[2][0]+=w[2][0]; m[2][1]+=w[2][1]; m[2][2]+=w[2][2];
#endif
  return *this;
  }


// Subtract matrices
FXMat3d& FXMat3d::operator-=(const FXMat3d& w){
#if defined(FOX_HAS_AVX)
  _mm256_storeu_pd(&m[0][0],_mm256_sub_pd(_mm256_loadu_pd(&m[0][0]),_mm256_loadu_pd(&w[0][0])));
  _mm256_storeu_pd(&m[1][1],_mm256_sub_pd(_mm256_loadu_pd(&m[1][1]),_mm256_loadu_pd(&w[1][1])));
  _mm256_storeu_sd(&m[2][2],_mm256_sub_sd(_mm256_loadu_sd(&m[2][2]),_mm256_loadu_sd(&w[2][2])));
#elif defined(FOX_HAS_SSE2)
  _mm_storeu_pd(&m[0][0],_mm_sub_pd(_mm_loadu_pd(&m[0][0]),_mm_loadu_pd(&w[0][0])));
  _mm_storeu_pd(&m[0][2],_mm_sub_pd(_mm_loadu_pd(&m[0][2]),_mm_loadu_pd(&w[0][2])));
  _mm_storeu_pd(&m[1][1],_mm_sub_pd(_mm_loadu_pd(&m[1][1]),_mm_loadu_pd(&w[1][1])));
  _mm_storeu_pd(&m[2][0],_mm_sub_pd(_mm_loadu_pd(&m[2][0]),_mm_loadu_pd(&w[2][0])));
  _mm_storeu_sd(&m[2][2],_mm_sub_sd(_mm_loadu_sd(&m[2][2]),_mm_loadu_sd(&w[2][2])));
#else
  m[0][0]-=w[0][0]; m[0][1]-=w[0][1]; m[0][2]-=w[0][2];
  m[1][0]-=w[1][0]; m[1][1]-=w[1][1]; m[1][2]-=w[1][2];
  m[2][0]-=w[2][0]; m[2][1]-=w[2][1]; m[2][2]-=w[2][2];
#endif
  return *this;
  }


// Multiply matrix by scalar
FXMat3d& FXMat3d::operator*=(FXdouble s){
#if defined(FOX_HAS_AVX)
  _mm256_storeu_pd(&m[0][0],_mm256_mul_pd(_mm256_loadu_pd(&m[0][0]),_mm256_set1_pd(s)));
  _mm256_storeu_pd(&m[1][1],_mm256_mul_pd(_mm256_loadu_pd(&m[1][1]),_mm256_set1_pd(s)));
  _mm256_storeu_sd(&m[2][2],_mm256_mul_sd(_mm256_loadu_sd(&m[2][2]),_mm256_set1_pd(s)));
#elif defined(FOX_HAS_SSE2)
  _mm_storeu_pd(&m[0][0],_mm_mul_pd(_mm_loadu_pd(&m[0][0]),_mm_set1_pd(s)));
  _mm_storeu_pd(&m[0][2],_mm_mul_pd(_mm_loadu_pd(&m[0][2]),_mm_set1_pd(s)));
  _mm_storeu_pd(&m[1][1],_mm_mul_pd(_mm_loadu_pd(&m[1][1]),_mm_set1_pd(s)));
  _mm_storeu_pd(&m[2][0],_mm_mul_pd(_mm_loadu_pd(&m[2][0]),_mm_set1_pd(s)));
  _mm_storeu_sd(&m[2][2],_mm_mul_sd(_mm_loadu_sd(&m[2][2]),_mm_set1_pd(s)));
#else
  m[0][0]*=s; m[0][1]*=s; m[0][2]*=s;
  m[1][0]*=s; m[1][1]*=s; m[1][2]*=s;
  m[2][0]*=s; m[2][1]*=s; m[2][2]*=s;
#endif
  return *this;
  }


// Multiply matrix by matrix
FXMat3d& FXMat3d::operator*=(const FXMat3d& s){
#if defined(FOX_HAS_AVX)
  __m256d b0=_mm256_maskload_pd(s[0],MMM);
  __m256d b1=_mm256_maskload_pd(s[1],MMM);
  __m256d b2=_mm256_maskload_pd(s[2],MMM);
  __m256d xx,yy,zz;
  xx=_mm256_set1_pd(m[0][0]);
  yy=_mm256_set1_pd(m[0][1]);
  zz=_mm256_set1_pd(m[0][2]);
  _mm256_maskstore_pd(m[0],MMM,_mm256_add_pd(_mm256_add_pd(_mm256_mul_pd(b0,xx),_mm256_mul_pd(b1,yy)),_mm256_mul_pd(b2,zz)));
  xx=_mm256_set1_pd(m[1][0]);
  yy=_mm256_set1_pd(m[1][1]);
  zz=_mm256_set1_pd(m[1][2]);
  _mm256_maskstore_pd(m[1],MMM,_mm256_add_pd(_mm256_add_pd(_mm256_mul_pd(b0,xx),_mm256_mul_pd(b1,yy)),_mm256_mul_pd(b2,zz)));
  xx=_mm256_set1_pd(m[2][0]);
  yy=_mm256_set1_pd(m[2][1]);
  zz=_mm256_set1_pd(m[2][2]);
  _mm256_maskstore_pd(m[2],MMM,_mm256_add_pd(_mm256_add_pd(_mm256_mul_pd(b0,xx),_mm256_mul_pd(b1,yy)),_mm256_mul_pd(b2,zz)));
  return *this;
#elif defined(FOX_HAS_SSE2)
  __m128d b00=_mm_loadu_pd(&s[0][0]);
  __m128d b02=_mm_load_sd(&s[0][2]);
  __m128d b10=_mm_loadu_pd(&s[1][0]);
  __m128d b12=_mm_load_sd(&s[1][2]);
  __m128d b20=_mm_loadu_pd(&s[2][0]);
  __m128d b22=_mm_load_sd(&s[2][2]);
  __m128d xx,yy,zz;
  xx=_mm_set1_pd(m[0][0]);
  yy=_mm_set1_pd(m[0][1]);
  zz=_mm_set1_pd(m[0][2]);
  _mm_storeu_pd(&m[0][0],_mm_add_pd(_mm_add_pd(_mm_mul_pd(b00,xx),_mm_mul_pd(b10,yy)),_mm_mul_pd(b20,zz)));
  _mm_store_sd(&m[0][2],_mm_add_sd(_mm_add_sd(_mm_mul_sd(b02,xx),_mm_mul_sd(b12,yy)),_mm_mul_sd(b22,zz)));
  xx=_mm_set1_pd(m[1][0]);
  yy=_mm_set1_pd(m[1][1]);
  zz=_mm_set1_pd(m[1][2]);
  _mm_storeu_pd(&m[1][0],_mm_add_pd(_mm_add_pd(_mm_mul_pd(b00,xx),_mm_mul_pd(b10,yy)),_mm_mul_pd(b20,zz)));
  _mm_store_sd(&m[1][2],_mm_add_sd(_mm_add_sd(_mm_mul_sd(b02,xx),_mm_mul_sd(b12,yy)),_mm_mul_sd(b22,zz)));
  xx=_mm_set1_pd(m[2][0]);
  yy=_mm_set1_pd(m[2][1]);
  zz=_mm_set1_pd(m[2][2]);
  _mm_storeu_pd(&m[2][0],_mm_add_pd(_mm_add_pd(_mm_mul_pd(b00,xx),_mm_mul_pd(b10,yy)),_mm_mul_pd(b20,zz)));
  _mm_store_sd(&m[2][2],_mm_add_sd(_mm_add_sd(_mm_mul_sd(b02,xx),_mm_mul_sd(b12,yy)),_mm_mul_sd(b22,zz)));
  return *this;
#else
  FXdouble x,y,z;
  x=m[0][0]; y=m[0][1]; z=m[0][2];
  m[0][0]=x*s[0][0]+y*s[1][0]+z*s[2][0];
  m[0][1]=x*s[0][1]+y*s[1][1]+z*s[2][1];
  m[0][2]=x*s[0][2]+y*s[1][2]+z*s[2][2];
  x=m[1][0]; y=m[1][1]; z=m[1][2];
  m[1][0]=x*s[0][0]+y*s[1][0]+z*s[2][0];
  m[1][1]=x*s[0][1]+y*s[1][1]+z*s[2][1];
  m[1][2]=x*s[0][2]+y*s[1][2]+z*s[2][2];
  x=m[2][0]; y=m[2][1]; z=m[2][2];
  m[2][0]=x*s[0][0]+y*s[1][0]+z*s[2][0];
  m[2][1]=x*s[0][1]+y*s[1][1]+z*s[2][1];
  m[2][2]=x*s[0][2]+y*s[1][2]+z*s[2][2];
  return *this;
#endif
  }


// Divide matrix by scalar
FXMat3d& FXMat3d::operator/=(FXdouble s){
#if defined(FOX_HAS_AVX)
  _mm256_storeu_pd(&m[0][0],_mm256_div_pd(_mm256_loadu_pd(&m[0][0]),_mm256_set1_pd(s)));
  _mm256_storeu_pd(&m[1][1],_mm256_div_pd(_mm256_loadu_pd(&m[1][1]),_mm256_set1_pd(s)));
  _mm256_storeu_sd(&m[2][2],_mm256_div_sd(_mm256_loadu_sd(&m[2][2]),_mm256_set1_pd(s)));
#elif defined(FOX_HAS_SSE2)
  _mm_storeu_pd(&m[0][0],_mm_div_pd(_mm_loadu_pd(&m[0][0]),_mm_set1_pd(s)));
  _mm_storeu_pd(&m[0][2],_mm_div_pd(_mm_loadu_pd(&m[0][2]),_mm_set1_pd(s)));
  _mm_storeu_pd(&m[1][1],_mm_div_pd(_mm_loadu_pd(&m[1][1]),_mm_set1_pd(s)));
  _mm_storeu_pd(&m[2][0],_mm_div_pd(_mm_loadu_pd(&m[2][0]),_mm_set1_pd(s)));
  _mm_storeu_sd(&m[2][2],_mm_div_sd(_mm_loadu_sd(&m[2][2]),_mm_set1_pd(s)));
#else
  m[0][0]/=s; m[0][1]/=s; m[0][2]/=s;
  m[1][0]/=s; m[1][1]/=s; m[1][2]/=s;
  m[2][0]/=s; m[2][1]/=s; m[2][2]/=s;
#endif
  return *this;
  }


// Negate matrix
FXMat3d FXMat3d::operator-() const {
#if defined(FOX_HAS_AVX)
  FXMat3d r;
  _mm256_storeu_pd(&r[0][0],_mm256_sub_pd(_mm256_set1_pd(0.0),_mm256_loadu_pd(&m[0][0])));
  _mm256_storeu_pd(&r[1][1],_mm256_sub_pd(_mm256_set1_pd(0.0),_mm256_loadu_pd(&m[1][1])));
  _mm256_storeu_sd(&r[2][2],_mm256_sub_sd(_mm256_set1_pd(0.0),_mm256_loadu_sd(&m[2][2])));
  return r;
#elif defined(FOX_HAS_SSE2)
  FXMat3d r;
  _mm_storeu_pd(&r[0][0],_mm_sub_pd(_mm_set1_pd(0.0),_mm_loadu_pd(&m[0][0])));
  _mm_storeu_pd(&r[0][2],_mm_sub_pd(_mm_set1_pd(0.0),_mm_loadu_pd(&m[0][2])));
  _mm_storeu_pd(&r[1][1],_mm_sub_pd(_mm_set1_pd(0.0),_mm_loadu_pd(&m[1][1])));
  _mm_storeu_pd(&r[2][0],_mm_sub_pd(_mm_set1_pd(0.0),_mm_loadu_pd(&m[2][0])));
  _mm_storeu_sd(&r[2][2],_mm_sub_sd(_mm_set1_pd(0.0),_mm_loadu_sd(&m[2][2])));
  return r;
#else
  return FXMat3d(-m[0][0],-m[0][1],-m[0][2],
                 -m[1][0],-m[1][1],-m[1][2],
                 -m[2][0],-m[2][1],-m[2][2]);
#endif
  }


// Set to identity matrix
FXMat3d& FXMat3d::identity(){
#if defined(FOX_HAS_AVX)
  _mm256_storeu_pd(&m[0][0],_mm256_set_pd(0.0,0.0,0.0,1.0));
  _mm256_storeu_pd(&m[1][1],_mm256_set_pd(0.0,0.0,0.0,1.0));
  _mm256_storeu_sd(&m[2][2],_mm256_set_pd(0.0,0.0,0.0,1.0));
#elif defined(FOX_HAS_SSE2)
  _mm_storeu_pd(&m[0][0],_mm_set_pd(0.0,1.0));
  _mm_storeu_pd(&m[0][2],_mm_set_pd(0.0,0.0));
  _mm_storeu_pd(&m[1][1],_mm_set_pd(0.0,1.0));
  _mm_storeu_pd(&m[2][0],_mm_set_pd(0.0,0.0));
  _mm_storeu_sd(&m[2][2],_mm_set_pd(0.0,1.0));
#else
  m[0][0]=1.0; m[0][1]=0.0; m[0][2]=0.0;
  m[1][0]=0.0; m[1][1]=1.0; m[1][2]=0.0;
  m[2][0]=0.0; m[2][1]=0.0; m[2][2]=1.0;
#endif
  return *this;
  }


// Return true if identity matrix
FXbool FXMat3d::isIdentity() const {
  return m[0][0]==1.0 && m[0][1]==0.0 && m[0][2]==0.0 &&
         m[1][0]==0.0 && m[1][1]==1.0 && m[1][2]==0.0 &&
         m[2][0]==0.0 && m[2][1]==0.0 && m[2][2]==1.0;
  }


// Rotate using unit quaternion
FXMat3d& FXMat3d::rot(const FXQuatd& q){
  return *this*=FXMat3d(q);
  }


// Multiply by rotation c,s about unit axis
FXMat3d& FXMat3d::rot(const FXVec3d& v,FXdouble c,FXdouble s){
  FXdouble xx=v.x*v.x;
  FXdouble yy=v.y*v.y;
  FXdouble zz=v.z*v.z;
  FXdouble xy=v.x*v.y;
  FXdouble yz=v.y*v.z;
  FXdouble zx=v.z*v.x;
  FXdouble xs=v.x*s;
  FXdouble ys=v.y*s;
  FXdouble zs=v.z*s;
  FXdouble t=1.0-c;
  return *this*=FXMat3d(t*xx+c,t*xy+zs,t*zx-ys,t*xy-zs,t*yy+c,t*yz+xs,t*zx+ys,t*yz-xs,t*zz+c);
  }


// Multiply by rotation of phi about unit axis
FXMat3d& FXMat3d::rot(const FXVec3d& v,FXdouble phi){
  return rot(v,Math::cos(phi),Math::sin(phi));
  }


// Rotate about x-axis
FXMat3d& FXMat3d::xrot(FXdouble c,FXdouble s){
  FXdouble u,v;
  u=m[1][0]; v=m[2][0]; m[1][0]=c*u+s*v; m[2][0]=c*v-s*u;
  u=m[1][1]; v=m[2][1]; m[1][1]=c*u+s*v; m[2][1]=c*v-s*u;
  u=m[1][2]; v=m[2][2]; m[1][2]=c*u+s*v; m[2][2]=c*v-s*u;
  return *this;
  }


// Rotate by angle about x-axis
FXMat3d& FXMat3d::xrot(FXdouble phi){
  return xrot(Math::cos(phi),Math::sin(phi));
  }


// Rotate about y-axis
FXMat3d& FXMat3d::yrot(FXdouble c,FXdouble s){
  FXdouble u,v;
  u=m[0][0]; v=m[2][0]; m[0][0]=c*u-s*v; m[2][0]=c*v+s*u;
  u=m[0][1]; v=m[2][1]; m[0][1]=c*u-s*v; m[2][1]=c*v+s*u;
  u=m[0][2]; v=m[2][2]; m[0][2]=c*u-s*v; m[2][2]=c*v+s*u;
  return *this;
  }


// Rotate by angle about y-axis
FXMat3d& FXMat3d::yrot(FXdouble phi){
  return yrot(Math::cos(phi),Math::sin(phi));
  }


// Rotate about z-axis
FXMat3d& FXMat3d::zrot(FXdouble c,FXdouble s){
  FXdouble u,v;
  u=m[0][0]; v=m[1][0]; m[0][0]=c*u+s*v; m[1][0]=c*v-s*u;
  u=m[0][1]; v=m[1][1]; m[0][1]=c*u+s*v; m[1][1]=c*v-s*u;
  u=m[0][2]; v=m[1][2]; m[0][2]=c*u+s*v; m[1][2]=c*v-s*u;
  return *this;
  }


// Rotate by angle about z-axis
FXMat3d& FXMat3d::zrot(FXdouble phi){
  return zrot(Math::cos(phi),Math::sin(phi));
  }


// Scale unqual
FXMat3d& FXMat3d::scale(FXdouble sx,FXdouble sy,FXdouble sz){
  m[0][0]*=sx; m[0][1]*=sx; m[0][2]*=sx;
  m[1][0]*=sy; m[1][1]*=sy; m[1][2]*=sy;
  m[2][0]*=sz; m[2][1]*=sz; m[2][2]*=sz;
  return *this;
  }


// Scale unqual
FXMat3d& FXMat3d::scale(const FXVec3d& v){
  return scale(v[0],v[1],v[2]);
  }


// Scale uniform
FXMat3d& FXMat3d::scale(FXdouble s){
  return scale(s,s,s);
  }


// Calculate determinant
FXdouble FXMat3d::det() const {
  return m[0][0]*(m[1][1]*m[2][2]-m[2][1]*m[1][2])+
         m[0][1]*(m[2][0]*m[1][2]-m[1][0]*m[2][2])+
         m[0][2]*(m[1][0]*m[2][1]-m[2][0]*m[1][1]);
  }


// Transpose matrix
FXMat3d FXMat3d::transpose() const {
  return FXMat3d(m[0][0],m[1][0],m[2][0],
                 m[0][1],m[1][1],m[2][1],
                 m[0][2],m[1][2],m[2][2]);
  }


// Invert matrix
FXMat3d FXMat3d::invert() const {
  FXdouble dd;
  FXMat3d res;
  res[0][0]=m[1][1]*m[2][2]-m[1][2]*m[2][1];
  res[0][1]=m[0][2]*m[2][1]-m[0][1]*m[2][2];
  res[0][2]=m[0][1]*m[1][2]-m[0][2]*m[1][1];
  res[1][0]=m[1][2]*m[2][0]-m[1][0]*m[2][2];
  res[1][1]=m[0][0]*m[2][2]-m[0][2]*m[2][0];
  res[1][2]=m[0][2]*m[1][0]-m[0][0]*m[1][2];
  res[2][0]=m[1][0]*m[2][1]-m[1][1]*m[2][0];
  res[2][1]=m[0][1]*m[2][0]-m[0][0]*m[2][1];
  res[2][2]=m[0][0]*m[1][1]-m[0][1]*m[1][0];
  dd=m[0][0]*res[0][0]+m[0][1]*res[1][0]+m[0][2]*res[2][0];
  FXASSERT(dd!=0.0);
  dd=1.0/dd;
  res[0][0]*=dd;
  res[0][1]*=dd;
  res[0][2]*=dd;
  res[1][0]*=dd;
  res[1][1]*=dd;
  res[1][2]*=dd;
  res[2][0]*=dd;
  res[2][1]*=dd;
  res[2][2]*=dd;
  return res;
  }


// Orthogonalize matrix
// Uses Gram-Schmidt orthogonalization on a row-by-row basis
FXMat3d orthogonalize(const FXMat3d& m){
  FXMat3d result(m);
  result[0]/=result[0].length();
  result[1]-=result[0]*(result[1]*result[0]);
  result[1]/=result[1].length();
  result[2]-=result[0]*(result[2]*result[0]);
  result[2]-=result[1]*(result[2]*result[1]);
  result[2]/=result[2].length();
  return result;
  }


// Matrix times vector
FXVec2d operator*(const FXMat3d& m,const FXVec2d& v){
#if defined(FOX_HAS_SSE3)
  __m128d vv=_mm_loadu_pd(&v[0]);
  __m128d r00=_mm_mul_pd(_mm_loadu_pd(&m[0][0]),vv);   // m01*v1  m00*v0
  __m128d r10=_mm_mul_pd(_mm_loadu_pd(&m[1][0]),vv);   // m11*v1  m10*v0
  __m128d r02=_mm_load_sd(&m[0][2]);                   // 0       m02
  __m128d r12=_mm_load_sd(&m[1][2]);                   // 0       m12
  FXVec2d r;
  r00=_mm_hadd_pd(r00,r02);     // m02  m01*v1+m00*v0
  r10=_mm_hadd_pd(r10,r12);     // m12  m11*v1+m10*v0
  r00=_mm_hadd_pd(r00,r10);     // m12+m11*v1+m10*v0  m02+m01*v1+m00*v0
  _mm_storeu_pd(&r[0],r00);
  return r;
#else
  return FXVec2d(m[0][0]*v[0]+m[0][1]*v[1]+m[0][2], m[1][0]*v[0]+m[1][1]*v[1]+m[1][2]);
#endif
  }


// Matrix times vector
FXVec3d operator*(const FXMat3d& m,const FXVec3d& v){
#if defined(FOX_HAS_SSE3)
  __m128d v0=_mm_loadu_pd(&v[0]);
  __m128d v1=_mm_load_sd(&v[2]);
  __m128d r00=_mm_mul_pd(_mm_loadu_pd(&m[0][0]),v0);   // m01*v1  m00*v0
  __m128d r10=_mm_mul_pd(_mm_loadu_pd(&m[1][0]),v0);   // m11*v1  m10*v0
  __m128d r20=_mm_mul_pd(_mm_loadu_pd(&m[2][0]),v0);   // m21*v1  m20*v0
  __m128d r02=_mm_mul_pd(_mm_load_sd(&m[0][2]),v1);    // 0       m02*v2
  __m128d r12=_mm_mul_pd(_mm_load_sd(&m[1][2]),v1);    // 0       m12*v2
  __m128d r22=_mm_mul_pd(_mm_load_sd(&m[2][2]),v1);    // 0       m22*v2
  FXVec3d r;
  r00=_mm_hadd_pd(r00,r02);     // m02*v2  m01*v1+m00*v0
  r10=_mm_hadd_pd(r10,r12);     // m12*v2  m11*v1+m10*v0
  r20=_mm_hadd_pd(r20,r22);     // m22*v2  m21*v1+m20*v0
  r00=_mm_hadd_pd(r00,r10);     // m12*v2+m11*v1+m10*v0  m02*v2+m01*v1+m00*v0
  r20=_mm_hadd_pd(r20,r20);     // m22*v2+m21*v1+m20*v0  m22*v2+m21*v1+m20*v0
  _mm_storeu_pd(&r[0],r00); _mm_store_sd(&r[2],r20);
  return r;
#else
  return FXVec3d(m[0][0]*v[0]+m[0][1]*v[1]+m[0][2]*v[2], m[1][0]*v[0]+m[1][1]*v[1]+m[1][2]*v[2], m[2][0]*v[0]+m[2][1]*v[1]+m[2][2]*v[2]);
#endif
  }


// Vector times matrix
FXVec2d operator*(const FXVec2d& v,const FXMat3d& m){
#if defined(FOX_HAS_SSE2)
  __m128d m00=_mm_loadu_pd(&m[0][0]);
  __m128d m10=_mm_loadu_pd(&m[1][0]);
  __m128d m20=_mm_loadu_pd(&m[2][0]);
  __m128d v0=_mm_set1_pd(v[0]);
  __m128d v1=_mm_set1_pd(v[1]);
  FXVec2d r;
  _mm_storeu_pd(&r[0],_mm_add_pd(_mm_add_pd(_mm_mul_pd(m00,v0),_mm_mul_pd(m10,v1)),m20));
  return r;
#else
  return FXVec2d(v[0]*m[0][0]+v[1]*m[1][0]+m[2][0],v[0]*m[0][1]+v[1]*m[1][1]+m[2][1]);
#endif
  }


// Vector times matrix
FXVec3d operator*(const FXVec3d& v,const FXMat3d& m){
#if defined(FOX_HAS_AVX)
  __m256d m0=_mm256_maskload_pd(m[0],MMM);
  __m256d m1=_mm256_maskload_pd(m[1],MMM);
  __m256d m2=_mm256_maskload_pd(m[2],MMM);
  __m256d v0=_mm256_set1_pd(v[0]);
  __m256d v1=_mm256_set1_pd(v[1]);
  __m256d v2=_mm256_set1_pd(v[2]);
  FXVec3d r;
  _mm256_maskstore_pd(&r[0],MMM,_mm256_add_pd(_mm256_add_pd(_mm256_mul_pd(v0,m0),_mm256_mul_pd(v1,m1)),_mm256_mul_pd(v2,m2)));
  return r;
#elif defined(FOX_HAS_SSE2)
  __m128d m00=_mm_loadu_pd(&m[0][0]);
  __m128d m10=_mm_loadu_pd(&m[1][0]);
  __m128d m20=_mm_loadu_pd(&m[2][0]);
  __m128d m02=_mm_load_sd(&m[0][2]);
  __m128d m12=_mm_load_sd(&m[1][2]);
  __m128d m22=_mm_load_sd(&m[2][2]);
  __m128d v0=_mm_set1_pd(v[0]);
  __m128d v1=_mm_set1_pd(v[1]);
  __m128d v2=_mm_set1_pd(v[2]);
  FXVec3d r;
  _mm_storeu_pd(&r[0],_mm_add_pd(_mm_add_pd(_mm_mul_pd(m00,v0),_mm_mul_pd(m10,v1)),_mm_mul_pd(m20,v2)));
  _mm_store_sd(&r[2],_mm_add_sd(_mm_add_sd(_mm_mul_sd(m02,v0),_mm_mul_sd(m12,v1)),_mm_mul_sd(m22,v2)));
  return r;
#else
  return FXVec3d(v[0]*m[0][0]+v[1]*m[1][0]+v[2]*m[2][0], v[0]*m[0][1]+v[1]*m[1][1]+v[2]*m[2][1], v[0]*m[0][2]+v[1]*m[1][2]+v[2]*m[2][2]);
#endif
  }


// Matrix and matrix add
FXMat3d operator+(const FXMat3d& a,const FXMat3d& b){
#if defined(FOX_HAS_AVX)
  FXMat3d r;
  _mm256_maskstore_pd(&r[0][0],MMM,_mm256_add_pd(_mm256_maskload_pd(&a[0][0],MMM),_mm256_maskload_pd(&b[0][0],MMM)));
  _mm256_maskstore_pd(&r[1][0],MMM,_mm256_add_pd(_mm256_maskload_pd(&a[1][0],MMM),_mm256_maskload_pd(&b[1][0],MMM)));
  _mm256_maskstore_pd(&r[2][0],MMM,_mm256_add_pd(_mm256_maskload_pd(&a[2][0],MMM),_mm256_maskload_pd(&b[2][0],MMM)));
  return r;
#elif defined(FOX_HAS_SSE2)
  FXMat3d r;
  _mm_storeu_pd(&r[0][0],_mm_add_pd(_mm_loadu_pd(&a[0][0]),_mm_loadu_pd(&b[0][0])));
  _mm_storeu_pd(&r[0][2],_mm_add_pd(_mm_loadu_pd(&a[0][2]),_mm_loadu_pd(&b[0][2])));
  _mm_storeu_pd(&r[1][1],_mm_add_pd(_mm_loadu_pd(&a[1][1]),_mm_loadu_pd(&b[1][1])));
  _mm_storeu_pd(&r[2][0],_mm_add_pd(_mm_loadu_pd(&a[2][0]),_mm_loadu_pd(&b[2][0])));
  _mm_storeu_sd(&r[2][2],_mm_add_sd(_mm_loadu_sd(&a[2][2]),_mm_loadu_sd(&b[2][2])));
  return r;
#else
  return FXMat3d(a[0][0]+b[0][0],a[0][1]+b[0][1],a[0][2]+b[0][2], a[1][0]+b[1][0],a[1][1]+b[1][1],a[1][2]+b[1][2], a[2][0]+b[2][0],a[2][1]+b[2][1],a[2][2]+b[2][2]);
#endif
  }


// Matrix and matrix subtract
FXMat3d operator-(const FXMat3d& a,const FXMat3d& b){
#if defined(FOX_HAS_AVX)
  FXMat3d r;
  _mm256_maskstore_pd(&r[0][0],MMM,_mm256_sub_pd(_mm256_maskload_pd(&a[0][0],MMM),_mm256_maskload_pd(&b[0][0],MMM)));
  _mm256_maskstore_pd(&r[1][0],MMM,_mm256_sub_pd(_mm256_maskload_pd(&a[1][0],MMM),_mm256_maskload_pd(&b[1][0],MMM)));
  _mm256_maskstore_pd(&r[2][0],MMM,_mm256_sub_pd(_mm256_maskload_pd(&a[2][0],MMM),_mm256_maskload_pd(&b[2][0],MMM)));
  return r;
#elif defined(FOX_HAS_SSE2)
  FXMat3d r;
  _mm_storeu_pd(&r[0][0],_mm_sub_pd(_mm_loadu_pd(&a[0][0]),_mm_loadu_pd(&b[0][0])));
  _mm_storeu_pd(&r[0][2],_mm_sub_pd(_mm_loadu_pd(&a[0][2]),_mm_loadu_pd(&b[0][2])));
  _mm_storeu_pd(&r[1][1],_mm_sub_pd(_mm_loadu_pd(&a[1][1]),_mm_loadu_pd(&b[1][1])));
  _mm_storeu_pd(&r[2][0],_mm_sub_pd(_mm_loadu_pd(&a[2][0]),_mm_loadu_pd(&b[2][0])));
  _mm_storeu_sd(&r[2][2],_mm_sub_sd(_mm_loadu_sd(&a[2][2]),_mm_loadu_sd(&b[2][2])));
  return r;
#else
  return FXMat3d(a[0][0]-b[0][0],a[0][1]-b[0][1],a[0][2]-b[0][2], a[1][0]-b[1][0],a[1][1]-b[1][1],a[1][2]-b[1][2], a[2][0]-b[2][0],a[2][1]-b[2][1],a[2][2]-b[2][2]);
#endif
  }


// Matrix and matrix multiply
FXMat3d operator*(const FXMat3d& a,const FXMat3d& b){
#if defined(FOX_HAS_AVX)
  __m256d b0=_mm256_maskload_pd(b[0],MMM);
  __m256d b1=_mm256_maskload_pd(b[0],MMM);
  __m256d b2=_mm256_maskload_pd(b[0],MMM);
  __m256d xx,yy,zz;
  FXMat3d r;
  xx=_mm256_set1_pd(a[0][0]);
  yy=_mm256_set1_pd(a[0][1]);
  zz=_mm256_set1_pd(a[0][2]);
  _mm256_maskstore_pd(r[0],MMM,_mm256_add_pd(_mm256_add_pd(_mm256_mul_pd(b0,xx),_mm256_mul_pd(b1,yy)),_mm256_mul_pd(b2,zz)));
  xx=_mm256_set1_pd(a[1][0]);
  yy=_mm256_set1_pd(a[1][1]);
  zz=_mm256_set1_pd(a[1][2]);
  _mm256_maskstore_pd(r[1],MMM,_mm256_add_pd(_mm256_add_pd(_mm256_mul_pd(b0,xx),_mm256_mul_pd(b1,yy)),_mm256_mul_pd(b2,zz)));
  xx=_mm256_set1_pd(a[2][0]);
  yy=_mm256_set1_pd(a[2][1]);
  zz=_mm256_set1_pd(a[2][2]);
  _mm256_maskstore_pd(r[2],MMM,_mm256_add_pd(_mm256_add_pd(_mm256_mul_pd(b0,xx),_mm256_mul_pd(b1,yy)),_mm256_mul_pd(b2,zz)));
  return r;
#elif defined(FOX_HAS_SSE2)
  __m128d b00=_mm_loadu_pd(&b[0][0]);
  __m128d b10=_mm_loadu_pd(&b[1][0]);
  __m128d b20=_mm_loadu_pd(&b[2][0]);
  __m128d b02=_mm_load_sd(&b[0][2]);
  __m128d b12=_mm_load_sd(&b[1][2]);
  __m128d b22=_mm_load_sd(&b[2][2]);
  __m128d xx,yy,zz;
  FXMat3d r;
  xx=_mm_set1_pd(a[0][0]);
  yy=_mm_set1_pd(a[0][1]);
  zz=_mm_set1_pd(a[0][2]);
  _mm_storeu_pd(&r[0][0],_mm_add_pd(_mm_add_pd(_mm_mul_pd(b00,xx),_mm_mul_pd(b10,yy)),_mm_mul_pd(b20,zz)));
  _mm_store_sd(&r[0][2],_mm_add_sd(_mm_add_sd(_mm_mul_sd(b02,xx),_mm_mul_sd(b12,yy)),_mm_mul_sd(b22,zz)));
  xx=_mm_set1_pd(a[1][0]);
  yy=_mm_set1_pd(a[1][1]);
  zz=_mm_set1_pd(a[1][2]);
  _mm_storeu_pd(&r[1][0],_mm_add_pd(_mm_add_pd(_mm_mul_pd(b00,xx),_mm_mul_pd(b10,yy)),_mm_mul_pd(b20,zz)));
  _mm_store_sd(&r[1][2],_mm_add_sd(_mm_add_sd(_mm_mul_sd(b02,xx),_mm_mul_sd(b12,yy)),_mm_mul_sd(b22,zz)));
  xx=_mm_set1_pd(a[2][0]);
  yy=_mm_set1_pd(a[2][1]);
  zz=_mm_set1_pd(a[2][2]);
  _mm_storeu_pd(&r[2][0],_mm_add_pd(_mm_add_pd(_mm_mul_pd(b00,xx),_mm_mul_pd(b10,yy)),_mm_mul_pd(b20,zz)));
  _mm_store_sd(&r[2][2],_mm_add_sd(_mm_add_sd(_mm_mul_sd(b02,xx),_mm_mul_sd(b12,yy)),_mm_mul_sd(b22,zz)));
  return r;
#else
  FXdouble x,y,z;
  FXMat3d r;
  x=a[0][0]; y=a[0][1]; z=a[0][2];
  r[0][0]=x*b[0][0]+y*b[1][0]+z*b[2][0];
  r[0][1]=x*b[0][1]+y*b[1][1]+z*b[2][1];
  r[0][2]=x*b[0][2]+y*b[1][2]+z*b[2][2];
  x=a[1][0]; y=a[1][1]; z=a[1][2];
  r[1][0]=x*b[0][0]+y*b[1][0]+z*b[2][0];
  r[1][1]=x*b[0][1]+y*b[1][1]+z*b[2][1];
  r[1][2]=x*b[0][2]+y*b[1][2]+z*b[2][2];
  x=a[2][0]; y=a[2][1]; z=a[2][2];
  r[2][0]=x*b[0][0]+y*b[1][0]+z*b[2][0];
  r[2][1]=x*b[0][1]+y*b[1][1]+z*b[2][1];
  r[2][2]=x*b[0][2]+y*b[1][2]+z*b[2][2];
  return r;
#endif
  }


// Multiply scalar by matrix
FXMat3d operator*(FXdouble x,const FXMat3d& m){
#if defined(FOX_HAS_AVX)
  FXMat3d r;
  _mm256_storeu_pd(&r[0][0],_mm256_mul_pd(_mm256_set1_pd(x),_mm256_loadu_pd(&m[0][0])));
  _mm256_storeu_pd(&r[1][1],_mm256_mul_pd(_mm256_set1_pd(x),_mm256_loadu_pd(&m[1][1])));
  _mm256_storeu_sd(&r[2][2],_mm256_mul_sd(_mm256_set1_pd(x),_mm256_loadu_sd(&m[2][2])));
  return r;
#elif defined(FOX_HAS_SSE2)
  FXMat3d r;
  _mm_storeu_pd(&r[0][0],_mm_mul_pd(_mm_set1_pd(x),_mm_loadu_pd(&m[0][0])));
  _mm_storeu_pd(&r[0][2],_mm_mul_pd(_mm_set1_pd(x),_mm_loadu_pd(&m[0][2])));
  _mm_storeu_pd(&r[1][1],_mm_mul_pd(_mm_set1_pd(x),_mm_loadu_pd(&m[1][1])));
  _mm_storeu_pd(&r[2][0],_mm_mul_pd(_mm_set1_pd(x),_mm_loadu_pd(&m[2][0])));
  _mm_storeu_sd(&r[2][2],_mm_mul_sd(_mm_set1_pd(x),_mm_loadu_sd(&m[2][2])));
  return r;
#else
  return FXMat3d(x*m[0][0],x*m[0][1],x*m[0][2],
                 x*m[1][0],x*m[1][1],x*m[1][2],
                 x*m[2][0],x*m[2][1],x*m[2][2]);
#endif
  }


// Multiply matrix by scalar
FXMat3d operator*(const FXMat3d& m,FXdouble x){
#if defined(FOX_HAS_AVX)
  FXMat3d r;
  _mm256_storeu_pd(&r[0][0],_mm256_mul_pd(_mm256_loadu_pd(&m[0][0]),_mm256_set1_pd(x)));
  _mm256_storeu_pd(&r[1][1],_mm256_mul_pd(_mm256_loadu_pd(&m[1][1]),_mm256_set1_pd(x)));
  _mm256_storeu_sd(&r[2][2],_mm256_mul_sd(_mm256_loadu_sd(&m[2][2]),_mm256_set1_pd(x)));
  return r;
#elif defined(FOX_HAS_SSE2)
  FXMat3d r;
  _mm_storeu_pd(&r[0][0],_mm_mul_pd(_mm_loadu_pd(&m[0][0]),_mm_set1_pd(x)));
  _mm_storeu_pd(&r[0][2],_mm_mul_pd(_mm_loadu_pd(&m[0][2]),_mm_set1_pd(x)));
  _mm_storeu_pd(&r[1][1],_mm_mul_pd(_mm_loadu_pd(&m[1][1]),_mm_set1_pd(x)));
  _mm_storeu_pd(&r[2][0],_mm_mul_pd(_mm_loadu_pd(&m[2][0]),_mm_set1_pd(x)));
  _mm_storeu_sd(&r[2][2],_mm_mul_sd(_mm_loadu_sd(&m[2][2]),_mm_set1_pd(x)));
  return r;
#else
  return FXMat3d(m[0][0]*x,m[0][1]*x,m[0][2]*x,
                 m[1][0]*x,m[1][1]*x,m[1][2]*x,
                 m[2][0]*x,m[2][1]*x,m[2][2]*x);
#endif
  }


// Divide scalar by matrix
FXMat3d operator/(FXdouble x,const FXMat3d& m){
#if defined(FOX_HAS_AVX)
  FXMat3d r;
  _mm256_storeu_pd(&r[0][0],_mm256_div_pd(_mm256_set1_pd(x),_mm256_loadu_pd(&m[0][0])));
  _mm256_storeu_pd(&r[1][1],_mm256_div_pd(_mm256_set1_pd(x),_mm256_loadu_pd(&m[1][1])));
  _mm256_storeu_sd(&r[2][2],_mm256_div_sd(_mm256_set1_pd(x),_mm256_loadu_sd(&m[2][2])));
  return r;
#elif defined(FOX_HAS_SSE2)
  FXMat3d r;
  _mm_storeu_pd(&r[0][0],_mm_div_pd(_mm_set1_pd(x),_mm_loadu_pd(&m[0][0])));
  _mm_storeu_pd(&r[0][2],_mm_div_pd(_mm_set1_pd(x),_mm_loadu_pd(&m[0][2])));
  _mm_storeu_pd(&r[1][1],_mm_div_pd(_mm_set1_pd(x),_mm_loadu_pd(&m[1][1])));
  _mm_storeu_pd(&r[2][0],_mm_div_pd(_mm_set1_pd(x),_mm_loadu_pd(&m[2][0])));
  _mm_storeu_sd(&r[2][2],_mm_div_sd(_mm_set1_pd(x),_mm_loadu_sd(&m[2][2])));
  return r;
#else
  return FXMat3d(x/m[0][0],x/m[0][1],x/m[0][2],
                 x/m[1][0],x/m[1][1],x/m[1][2],
                 x/m[2][0],x/m[2][1],x/m[2][2]);
#endif
  }


// Divide matrix by scalar
FXMat3d operator/(const FXMat3d& m,FXdouble x){
#if defined(FOX_HAS_AVX)
  FXMat3d r;
  _mm256_storeu_pd(&r[0][0],_mm256_div_pd(_mm256_loadu_pd(&m[0][0]),_mm256_set1_pd(x)));
  _mm256_storeu_pd(&r[1][1],_mm256_div_pd(_mm256_loadu_pd(&m[1][1]),_mm256_set1_pd(x)));
  _mm256_storeu_sd(&r[2][2],_mm256_div_sd(_mm256_loadu_sd(&m[2][2]),_mm256_set1_pd(x)));
  return r;
#elif defined(FOX_HAS_SSE2)
  FXMat3d r;
  _mm_storeu_pd(&r[0][0],_mm_div_pd(_mm_loadu_pd(&m[0][0]),_mm_set1_pd(x)));
  _mm_storeu_pd(&r[0][2],_mm_div_pd(_mm_loadu_pd(&m[0][2]),_mm_set1_pd(x)));
  _mm_storeu_pd(&r[1][1],_mm_div_pd(_mm_loadu_pd(&m[1][1]),_mm_set1_pd(x)));
  _mm_storeu_pd(&r[2][0],_mm_div_pd(_mm_loadu_pd(&m[2][0]),_mm_set1_pd(x)));
  _mm_storeu_sd(&r[2][2],_mm_div_sd(_mm_loadu_sd(&m[2][2]),_mm_set1_pd(x)));
  return r;
#else
  return FXMat3d(m[0][0]/x,m[0][1]/x,m[0][2]/x,
                 m[1][0]/x,m[1][1]/x,m[1][2]/x,
                 m[2][0]/x,m[2][1]/x,m[2][2]/x);
#endif
  }


// Matrix and matrix equality
FXbool operator==(const FXMat3d& a,const FXMat3d& b){
  return a[0]==b[0] && a[1]==b[1] && a[2]==b[2];
  }


// Matrix and matrix inequality
FXbool operator!=(const FXMat3d& a,const FXMat3d& b){
  return a[0]!=b[0] || a[1]!=b[1] || a[2]!=b[2];
  }


// Matrix and scalar equality
FXbool operator==(const FXMat3d& a,FXdouble n){
  return a[0]==n && a[1]==n && a[2]==n;
  }


// Matrix and scalar inequality
FXbool operator!=(const FXMat3d& a,FXdouble n){
  return a[0]!=n || a[1]!=n || a[2]!=n;
  }


// Scalar and matrix equality
FXbool operator==(FXdouble n,const FXMat3d& a){
  return n==a[0] && n==a[1] && n==a[2];
  }


// Scalar and matrix inequality
FXbool operator!=(FXdouble n,const FXMat3d& a){
  return n!=a[0] || n!=a[1] || n!=a[2];
  }


// Save to archive
FXStream& operator<<(FXStream& store,const FXMat3d& m){
  store << m[0] << m[1] << m[2];
  return store;
  }


// Load from archive
FXStream& operator>>(FXStream& store,FXMat3d& m){
  store >> m[0] >> m[1] >> m[2];
  return store;
  }

}
