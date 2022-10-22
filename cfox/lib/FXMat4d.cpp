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
#include "FXMat3d.h"
#include "FXMat4d.h"


/*
  Notes:
  - Transformations pre-multiply.
  - Goal is same effect as OpenGL.
  - Some operations assume last column is (0,0,0,1).
*/


using namespace FX;

/*******************************************************************************/

namespace FX {

// Mask bottom 3 elements
#define MMM _mm256_set_epi64x(0,~0,~0,~0)


// Initialize matrix from scalar
FXMat4d::FXMat4d(FXdouble s){
#if defined(FOX_HAS_AVX)
  _mm256_storeu_pd(&m[0][0],_mm256_set1_pd(s));
  _mm256_storeu_pd(&m[1][0],_mm256_set1_pd(s));
  _mm256_storeu_pd(&m[2][0],_mm256_set1_pd(s));
  _mm256_storeu_pd(&m[3][0],_mm256_set1_pd(s));
#elif defined(FOX_HAS_SSE2)
  _mm_storeu_pd(&m[0][0],_mm_set1_pd(s));
  _mm_storeu_pd(&m[0][2],_mm_set1_pd(s));
  _mm_storeu_pd(&m[1][0],_mm_set1_pd(s));
  _mm_storeu_pd(&m[1][2],_mm_set1_pd(s));
  _mm_storeu_pd(&m[2][0],_mm_set1_pd(s));
  _mm_storeu_pd(&m[2][2],_mm_set1_pd(s));
  _mm_storeu_pd(&m[3][0],_mm_set1_pd(s));
  _mm_storeu_pd(&m[3][2],_mm_set1_pd(s));
#else
  m[0][0]=s; m[0][1]=s; m[0][2]=s; m[0][3]=s;
  m[1][0]=s; m[1][1]=s; m[1][2]=s; m[1][3]=s;
  m[2][0]=s; m[2][1]=s; m[2][2]=s; m[2][3]=s;
  m[3][0]=s; m[3][1]=s; m[3][2]=s; m[3][3]=s;
#endif
  }


// Initialize with 3x3 rotation and scaling matrix
FXMat4d::FXMat4d(const FXMat3d& s){
#if defined(FOX_HAS_AVX)
  _mm256_storeu_pd(&m[0][0],_mm256_maskload_pd(&s[0][0],MMM));
  _mm256_storeu_pd(&m[1][0],_mm256_maskload_pd(&s[1][0],MMM));
  _mm256_storeu_pd(&m[2][0],_mm256_maskload_pd(&s[2][0],MMM));
  _mm256_storeu_pd(&m[3][0],_mm256_set_pd(1.0,0.0,0.0,0.0));
#elif defined(FOX_HAS_SSE2)
  _mm_storeu_pd(&m[0][0],_mm_loadu_pd(&s[0][0]));
  _mm_storeu_pd(&m[0][2],_mm_set_pd(0.0,s[0][2]));
  _mm_storeu_pd(&m[1][0],_mm_loadu_pd(&s[1][0]));
  _mm_storeu_pd(&m[1][2],_mm_set_pd(0.0,s[1][2]));
  _mm_storeu_pd(&m[2][0],_mm_loadu_pd(&s[2][0]));
  _mm_storeu_pd(&m[2][2],_mm_set_pd(0.0,s[2][2]));
  _mm_storeu_pd(&m[3][0],_mm_set_pd(0.0,0.0));
  _mm_storeu_pd(&m[3][2],_mm_set_pd(1.0,0.0));
#else
  m[0][0]=s[0][0]; m[0][1]=s[0][1]; m[0][2]=s[0][2]; m[0][3]=0.0;
  m[1][0]=s[1][0]; m[1][1]=s[1][1]; m[1][2]=s[1][2]; m[1][3]=0.0;
  m[2][0]=s[2][0]; m[2][1]=s[2][1]; m[2][2]=s[2][2]; m[2][3]=0.0;
  m[3][0]=0.0;     m[3][1]=0.0;     m[3][2]=0.0;     m[3][3]=1.0;
#endif
  }


// Initialize matrix from another matrix
FXMat4d::FXMat4d(const FXMat4d& s){
#if defined(FOX_HAS_AVX)
  _mm256_storeu_pd(&m[0][0],_mm256_loadu_pd(&s[0][0]));
  _mm256_storeu_pd(&m[1][0],_mm256_loadu_pd(&s[1][0]));
  _mm256_storeu_pd(&m[2][0],_mm256_loadu_pd(&s[2][0]));
  _mm256_storeu_pd(&m[3][0],_mm256_loadu_pd(&s[3][0]));
#elif defined(FOX_HAS_SSE2)
  _mm_storeu_pd(&m[0][0],_mm_loadu_pd(&s[0][0]));
  _mm_storeu_pd(&m[0][2],_mm_loadu_pd(&s[0][2]));
  _mm_storeu_pd(&m[1][0],_mm_loadu_pd(&s[1][0]));
  _mm_storeu_pd(&m[1][2],_mm_loadu_pd(&s[1][2]));
  _mm_storeu_pd(&m[2][0],_mm_loadu_pd(&s[2][0]));
  _mm_storeu_pd(&m[2][2],_mm_loadu_pd(&s[2][2]));
  _mm_storeu_pd(&m[3][0],_mm_loadu_pd(&s[3][0]));
  _mm_storeu_pd(&m[3][2],_mm_loadu_pd(&s[3][2]));
#else
  m[0]=s[0];
  m[1]=s[1];
  m[2]=s[2];
  m[3]=s[3];
#endif
  }


// Initialize matrix from array
FXMat4d::FXMat4d(const FXdouble s[]){
#if defined(FOX_HAS_AVX)
  _mm256_storeu_pd(&m[0][0],_mm256_loadu_pd(s+0));
  _mm256_storeu_pd(&m[1][0],_mm256_loadu_pd(s+4));
  _mm256_storeu_pd(&m[2][0],_mm256_loadu_pd(s+8));
  _mm256_storeu_pd(&m[3][0],_mm256_loadu_pd(s+12));
#elif defined(FOX_HAS_SSE2)
  _mm_storeu_pd(&m[0][0],_mm_loadu_pd(s+0));
  _mm_storeu_pd(&m[0][2],_mm_loadu_pd(s+2));
  _mm_storeu_pd(&m[1][0],_mm_loadu_pd(s+4));
  _mm_storeu_pd(&m[1][2],_mm_loadu_pd(s+6));
  _mm_storeu_pd(&m[2][0],_mm_loadu_pd(s+8));
  _mm_storeu_pd(&m[2][2],_mm_loadu_pd(s+10));
  _mm_storeu_pd(&m[3][0],_mm_loadu_pd(s+12));
  _mm_storeu_pd(&m[3][2],_mm_loadu_pd(s+14));
#else
  m[0][0]=s[0];  m[0][1]=s[1];  m[0][2]=s[2];  m[0][3]=s[3];
  m[1][0]=s[4];  m[1][1]=s[5];  m[1][2]=s[6];  m[1][3]=s[7];
  m[2][0]=s[8];  m[2][1]=s[9];  m[2][2]=s[10]; m[2][3]=s[11];
  m[3][0]=s[12]; m[3][1]=s[13]; m[3][2]=s[14]; m[3][3]=s[15];
#endif
  }


// Initialize diagonal matrix
FXMat4d::FXMat4d(FXdouble a,FXdouble b,FXdouble c,FXdouble d){
#if defined(FOX_HAS_AVX)
  _mm256_storeu_pd(&m[0][0],_mm256_set_pd(0.0,0.0,0.0,a));
  _mm256_storeu_pd(&m[1][0],_mm256_set_pd(0.0,0.0,b,0.0));
  _mm256_storeu_pd(&m[2][0],_mm256_set_pd(0.0,c,0.0,0.0));
  _mm256_storeu_pd(&m[3][0],_mm256_set_pd(d,0.0,0.0,0.0));
#elif defined(FOX_HAS_SSE2)
  _mm_storeu_pd(&m[0][0],_mm_set_pd(0.0,a));
  _mm_storeu_pd(&m[0][2],_mm_set_pd(0.0,0.0));
  _mm_storeu_pd(&m[1][0],_mm_set_pd(b,0.0));
  _mm_storeu_pd(&m[1][2],_mm_set_pd(0.0,0.0));
  _mm_storeu_pd(&m[2][0],_mm_set_pd(0.0,0.0));
  _mm_storeu_pd(&m[2][2],_mm_set_pd(0.0,c));
  _mm_storeu_pd(&m[3][0],_mm_set_pd(0.0,0.0));
  _mm_storeu_pd(&m[3][2],_mm_set_pd(d,0.0));
#else
  m[0][0]=a;   m[0][1]=0.0; m[0][2]=0.0; m[0][3]=0.0;
  m[1][0]=0.0; m[1][1]=b;   m[1][2]=0.0; m[1][3]=0.0;
  m[2][0]=0.0; m[2][1]=0.0; m[2][2]=c;   m[2][3]=0.0;
  m[3][0]=0.0; m[3][1]=0.0; m[3][2]=0.0; m[3][3]=d;
#endif
  }


// Initialize matrix from components
FXMat4d::FXMat4d(FXdouble a00,FXdouble a01,FXdouble a02,FXdouble a03,FXdouble a10,FXdouble a11,FXdouble a12,FXdouble a13,FXdouble a20,FXdouble a21,FXdouble a22,FXdouble a23,FXdouble a30,FXdouble a31,FXdouble a32,FXdouble a33){
  m[0][0]=a00; m[0][1]=a01; m[0][2]=a02; m[0][3]=a03;
  m[1][0]=a10; m[1][1]=a11; m[1][2]=a12; m[1][3]=a13;
  m[2][0]=a20; m[2][1]=a21; m[2][2]=a22; m[2][3]=a23;
  m[3][0]=a30; m[3][1]=a31; m[3][2]=a32; m[3][3]=a33;
  }


// Initialize matrix from four vectors
FXMat4d::FXMat4d(const FXVec4d& a,const FXVec4d& b,const FXVec4d& c,const FXVec4d& d){
#if defined(FOX_HAS_AVX)
  _mm256_storeu_pd(&m[0][0],_mm256_loadu_pd(a));
  _mm256_storeu_pd(&m[1][0],_mm256_loadu_pd(b));
  _mm256_storeu_pd(&m[2][0],_mm256_loadu_pd(c));
  _mm256_storeu_pd(&m[3][0],_mm256_loadu_pd(d));
#elif defined(FOX_HAS_SSE2)
  _mm_storeu_pd(&m[0][0],_mm_loadu_pd(&a[0]));
  _mm_storeu_pd(&m[0][2],_mm_loadu_pd(&a[2]));
  _mm_storeu_pd(&m[1][0],_mm_loadu_pd(&b[0]));
  _mm_storeu_pd(&m[1][2],_mm_loadu_pd(&b[2]));
  _mm_storeu_pd(&m[2][0],_mm_loadu_pd(&c[0]));
  _mm_storeu_pd(&m[2][2],_mm_loadu_pd(&c[2]));
  _mm_storeu_pd(&m[3][0],_mm_loadu_pd(&d[0]));
  _mm_storeu_pd(&m[3][2],_mm_loadu_pd(&d[2]));
#else
  m[0][0]=a[0]; m[0][1]=a[1]; m[0][2]=a[2]; m[0][3]=a[3];
  m[1][0]=b[0]; m[1][1]=b[1]; m[1][2]=b[2]; m[1][3]=b[3];
  m[2][0]=c[0]; m[2][1]=c[1]; m[2][2]=c[2]; m[2][3]=c[3];
  m[3][0]=d[0]; m[3][1]=d[1]; m[3][2]=d[2]; m[3][3]=d[3];
#endif
  }


// Initialize matrix from quaternion
FXMat4d::FXMat4d(const FXQuatd& quat){
  set(FXMat3d(quat));
  }


// Assign from scalar
FXMat4d& FXMat4d::operator=(FXdouble s){
#if defined(FOX_HAS_AVX)
  _mm256_storeu_pd(&m[0][0],_mm256_set1_pd(s));
  _mm256_storeu_pd(&m[1][0],_mm256_set1_pd(s));
  _mm256_storeu_pd(&m[2][0],_mm256_set1_pd(s));
  _mm256_storeu_pd(&m[3][0],_mm256_set1_pd(s));
#elif defined(FOX_HAS_SSE2)
  _mm_storeu_pd(&m[0][0],_mm_set1_pd(s));
  _mm_storeu_pd(&m[0][2],_mm_set1_pd(s));
  _mm_storeu_pd(&m[1][0],_mm_set1_pd(s));
  _mm_storeu_pd(&m[1][2],_mm_set1_pd(s));
  _mm_storeu_pd(&m[2][0],_mm_set1_pd(s));
  _mm_storeu_pd(&m[2][2],_mm_set1_pd(s));
  _mm_storeu_pd(&m[3][0],_mm_set1_pd(s));
  _mm_storeu_pd(&m[3][2],_mm_set1_pd(s));
#else
  m[0][0]=s; m[0][1]=s; m[0][2]=s; m[0][3]=s;
  m[1][0]=s; m[1][1]=s; m[1][2]=s; m[1][3]=s;
  m[2][0]=s; m[2][1]=s; m[2][2]=s; m[2][3]=s;
  m[3][0]=s; m[3][1]=s; m[3][2]=s; m[3][3]=s;
#endif
  return *this;
  }


// Assign from 3x3 rotation and scaling matrix
FXMat4d& FXMat4d::operator=(const FXMat3d& s){
#if defined(FOX_HAS_AVX)
  _mm256_storeu_pd(&m[0][0],_mm256_maskload_pd(s[0],MMM));
  _mm256_storeu_pd(&m[1][0],_mm256_maskload_pd(s[1],MMM));
  _mm256_storeu_pd(&m[2][0],_mm256_maskload_pd(s[2],MMM));
  _mm256_storeu_pd(&m[3][0],_mm256_set_pd(1.0,0.0,0.0,0.0));
#elif defined(FOX_HAS_SSE2)
  _mm_storeu_pd(&m[0][0],_mm_loadu_pd(&s[0][0]));
  _mm_storeu_pd(&m[0][2],_mm_set_pd(0.0,s[0][2]));
  _mm_storeu_pd(&m[1][0],_mm_loadu_pd(&s[1][0]));
  _mm_storeu_pd(&m[1][2],_mm_set_pd(0.0,s[1][2]));
  _mm_storeu_pd(&m[2][0],_mm_loadu_pd(&s[2][0]));
  _mm_storeu_pd(&m[2][2],_mm_set_pd(0.0,s[2][2]));
  _mm_storeu_pd(&m[3][0],_mm_set_pd(0.0,0.0));
  _mm_storeu_pd(&m[3][2],_mm_set_pd(0.0,1.0));
#else
  m[0][0]=s[0][0]; m[0][1]=s[0][1]; m[0][2]=s[0][2]; m[0][3]=0.0;
  m[1][0]=s[1][0]; m[1][1]=s[1][1]; m[1][2]=s[1][2]; m[1][3]=0.0;
  m[2][0]=s[2][0]; m[2][1]=s[2][1]; m[2][2]=s[2][2]; m[2][3]=0.0;
  m[3][0]=0.0;     m[3][1]=0.0;     m[3][2]=0.0;     m[3][3]=1.0;
#endif
  return *this;
  }


// Assignment operator
FXMat4d& FXMat4d::operator=(const FXMat4d& s){
#if defined(FOX_HAS_AVX)
  _mm256_storeu_pd(&m[0][0],_mm256_loadu_pd(&s[0][0]));
  _mm256_storeu_pd(&m[1][0],_mm256_loadu_pd(&s[1][0]));
  _mm256_storeu_pd(&m[2][0],_mm256_loadu_pd(&s[2][0]));
  _mm256_storeu_pd(&m[3][0],_mm256_loadu_pd(&s[3][0]));
#elif defined(FOX_HAS_SSE2)
  _mm_storeu_pd(&m[0][0],_mm_loadu_pd(&s[0][0]));
  _mm_storeu_pd(&m[0][2],_mm_loadu_pd(&s[0][2]));
  _mm_storeu_pd(&m[1][0],_mm_loadu_pd(&s[1][0]));
  _mm_storeu_pd(&m[1][2],_mm_loadu_pd(&s[1][2]));
  _mm_storeu_pd(&m[2][0],_mm_loadu_pd(&s[2][0]));
  _mm_storeu_pd(&m[2][2],_mm_loadu_pd(&s[2][2]));
  _mm_storeu_pd(&m[3][0],_mm_loadu_pd(&s[3][0]));
  _mm_storeu_pd(&m[3][2],_mm_loadu_pd(&s[3][2]));
#else
  m[0]=s[0];
  m[1]=s[1];
  m[2]=s[2];
  m[3]=s[3];
#endif
  return *this;
  }


// Assignment from quaternion
FXMat4d& FXMat4d::operator=(const FXQuatd& quat){
  return set(FXMat3d(quat));
  }


// Assignment from array
FXMat4d& FXMat4d::operator=(const FXdouble s[]){
#if defined(FOX_HAS_AVX)
  _mm256_storeu_pd(&m[0][0],_mm256_loadu_pd(s+0));
  _mm256_storeu_pd(&m[1][0],_mm256_loadu_pd(s+4));
  _mm256_storeu_pd(&m[2][0],_mm256_loadu_pd(s+8));
  _mm256_storeu_pd(&m[3][0],_mm256_loadu_pd(s+12));
#elif defined(FOX_HAS_SSE2)
  _mm_storeu_pd(&m[0][0],_mm_loadu_pd(s+0));
  _mm_storeu_pd(&m[0][2],_mm_loadu_pd(s+2));
  _mm_storeu_pd(&m[1][0],_mm_loadu_pd(s+4));
  _mm_storeu_pd(&m[1][2],_mm_loadu_pd(s+6));
  _mm_storeu_pd(&m[2][0],_mm_loadu_pd(s+8));
  _mm_storeu_pd(&m[2][2],_mm_loadu_pd(s+10));
  _mm_storeu_pd(&m[3][0],_mm_loadu_pd(s+12));
  _mm_storeu_pd(&m[3][2],_mm_loadu_pd(s+14));
#else
  m[0][0]=s[0];  m[0][1]=s[1];  m[0][2]=s[2];  m[0][3]=s[3];
  m[1][0]=s[4];  m[1][1]=s[5];  m[1][2]=s[6];  m[1][3]=s[7];
  m[2][0]=s[8];  m[2][1]=s[9];  m[2][2]=s[10]; m[2][3]=s[11];
  m[3][0]=s[12]; m[3][1]=s[13]; m[3][2]=s[14]; m[3][3]=s[15];
#endif
  return *this;
  }


// Set value from scalar
FXMat4d& FXMat4d::set(FXdouble s){
#if defined(FOX_HAS_AVX)
  _mm256_storeu_pd(&m[0][0],_mm256_set1_pd(s));
  _mm256_storeu_pd(&m[1][0],_mm256_set1_pd(s));
  _mm256_storeu_pd(&m[2][0],_mm256_set1_pd(s));
  _mm256_storeu_pd(&m[3][0],_mm256_set1_pd(s));
#elif defined(FOX_HAS_SSE2)
  _mm_storeu_pd(&m[0][0],_mm_set1_pd(s));
  _mm_storeu_pd(&m[0][2],_mm_set1_pd(s));
  _mm_storeu_pd(&m[1][0],_mm_set1_pd(s));
  _mm_storeu_pd(&m[1][2],_mm_set1_pd(s));
  _mm_storeu_pd(&m[2][0],_mm_set1_pd(s));
  _mm_storeu_pd(&m[2][2],_mm_set1_pd(s));
  _mm_storeu_pd(&m[3][0],_mm_set1_pd(s));
  _mm_storeu_pd(&m[3][2],_mm_set1_pd(s));
#else
  m[0][0]=s; m[0][1]=s; m[0][2]=s; m[0][3]=s;
  m[1][0]=s; m[1][1]=s; m[1][2]=s; m[1][3]=s;
  m[2][0]=s; m[2][1]=s; m[2][2]=s; m[2][3]=s;
  m[3][0]=s; m[3][1]=s; m[3][2]=s; m[3][3]=s;
#endif
  return *this;
  }


// Set from 3x3 rotation and scaling matrix
FXMat4d& FXMat4d::set(const FXMat3d& s){
#if defined(FOX_HAS_AVX)
  _mm256_storeu_pd(&m[0][0],_mm256_maskload_pd(&s[0][0],MMM));
  _mm256_storeu_pd(&m[1][0],_mm256_maskload_pd(&s[1][0],MMM));
  _mm256_storeu_pd(&m[2][0],_mm256_maskload_pd(&s[2][0],MMM));
  _mm256_storeu_pd(&m[3][0],_mm256_set_pd(1.0,0.0,0.0,0.0));
#elif defined(FOX_HAS_SSE2)
  _mm_storeu_pd(&m[0][0],_mm_loadu_pd(&s[0][0]));
  _mm_storeu_pd(&m[0][2],_mm_set_pd(0.0,s[0][2]));
  _mm_storeu_pd(&m[1][0],_mm_loadu_pd(&s[1][0]));
  _mm_storeu_pd(&m[1][2],_mm_set_pd(0.0,s[1][2]));
  _mm_storeu_pd(&m[2][0],_mm_loadu_pd(&s[2][0]));
  _mm_storeu_pd(&m[2][2],_mm_set_pd(0.0,s[2][2]));
  _mm_storeu_pd(&m[3][0],_mm_set_pd(0.0,0.0));
  _mm_storeu_pd(&m[3][2],_mm_set_pd(0.0,1.0));
#else
  m[0][0]=s[0][0]; m[0][1]=s[0][1]; m[0][2]=s[0][2]; m[0][3]=0.0;
  m[1][0]=s[1][0]; m[1][1]=s[1][1]; m[1][2]=s[1][2]; m[1][3]=0.0;
  m[2][0]=s[2][0]; m[2][1]=s[2][1]; m[2][2]=s[2][2]; m[2][3]=0.0;
  m[3][0]=0.0;     m[3][1]=0.0;     m[3][2]=0.0;     m[3][3]=1.0;
#endif
  return *this;
  }


// Set value from another matrix
FXMat4d& FXMat4d::set(const FXMat4d& s){
#if defined(FOX_HAS_AVX)
  _mm256_storeu_pd(&m[0][0],_mm256_loadu_pd(&s[0][0]));
  _mm256_storeu_pd(&m[1][0],_mm256_loadu_pd(&s[1][0]));
  _mm256_storeu_pd(&m[2][0],_mm256_loadu_pd(&s[2][0]));
  _mm256_storeu_pd(&m[3][0],_mm256_loadu_pd(&s[3][0]));
#elif defined(FOX_HAS_SSE2)
  _mm_storeu_pd(&m[0][0],_mm_loadu_pd(&s[0][0]));
  _mm_storeu_pd(&m[0][2],_mm_loadu_pd(&s[0][2]));
  _mm_storeu_pd(&m[1][0],_mm_loadu_pd(&s[1][0]));
  _mm_storeu_pd(&m[1][2],_mm_loadu_pd(&s[1][2]));
  _mm_storeu_pd(&m[2][0],_mm_loadu_pd(&s[2][0]));
  _mm_storeu_pd(&m[2][2],_mm_loadu_pd(&s[2][2]));
  _mm_storeu_pd(&m[3][0],_mm_loadu_pd(&s[3][0]));
  _mm_storeu_pd(&m[3][2],_mm_loadu_pd(&s[3][2]));
#else
  m[0]=s[0];
  m[1]=s[1];
  m[2]=s[2];
  m[3]=s[3];
#endif
  return *this;
  }


// Set value from array
FXMat4d& FXMat4d::set(const FXdouble s[]){
#if defined(FOX_HAS_AVX)
  _mm256_storeu_pd(&m[0][0],_mm256_loadu_pd(s+0));
  _mm256_storeu_pd(&m[1][0],_mm256_loadu_pd(s+4));
  _mm256_storeu_pd(&m[2][0],_mm256_loadu_pd(s+8));
  _mm256_storeu_pd(&m[3][0],_mm256_loadu_pd(s+12));
#elif defined(FOX_HAS_SSE2)
  _mm_storeu_pd(&m[0][0],_mm_loadu_pd(s+0));
  _mm_storeu_pd(&m[0][2],_mm_loadu_pd(s+2));
  _mm_storeu_pd(&m[1][0],_mm_loadu_pd(s+4));
  _mm_storeu_pd(&m[1][2],_mm_loadu_pd(s+6));
  _mm_storeu_pd(&m[2][0],_mm_loadu_pd(s+8));
  _mm_storeu_pd(&m[2][2],_mm_loadu_pd(s+10));
  _mm_storeu_pd(&m[3][0],_mm_loadu_pd(s+12));
  _mm_storeu_pd(&m[3][2],_mm_loadu_pd(s+14));
#else
  m[0][0]=s[0];  m[0][1]=s[1];  m[0][2]=s[2];  m[0][3]=s[3];
  m[1][0]=s[4];  m[1][1]=s[5];  m[1][2]=s[6];  m[1][3]=s[7];
  m[2][0]=s[8];  m[2][1]=s[9];  m[2][2]=s[10]; m[2][3]=s[11];
  m[3][0]=s[12]; m[3][1]=s[13]; m[3][2]=s[14]; m[3][3]=s[15];
#endif
  return *this;
  }


// Set diagonal matrix
FXMat4d& FXMat4d::set(FXdouble a,FXdouble b,FXdouble c,FXdouble d){
#if defined(FOX_HAS_AVX)
  _mm256_storeu_pd(&m[0][0],_mm256_set_pd(0.0,0.0,0.0,a));
  _mm256_storeu_pd(&m[1][0],_mm256_set_pd(0.0,0.0,b,0.0));
  _mm256_storeu_pd(&m[2][0],_mm256_set_pd(0.0,c,0.0,0.0));
  _mm256_storeu_pd(&m[3][0],_mm256_set_pd(d,0.0,0.0,0.0));
#elif defined(FOX_HAS_SSE2)
  _mm_storeu_pd(&m[0][0],_mm_set_pd(0.0,a));
  _mm_storeu_pd(&m[0][2],_mm_set_pd(0.0,0.0));
  _mm_storeu_pd(&m[1][0],_mm_set_pd(b,0.0));
  _mm_storeu_pd(&m[1][2],_mm_set_pd(0.0,0.0));
  _mm_storeu_pd(&m[2][0],_mm_set_pd(0.0,0.0));
  _mm_storeu_pd(&m[2][2],_mm_set_pd(0.0,c));
  _mm_storeu_pd(&m[3][0],_mm_set_pd(0.0,0.0));
  _mm_storeu_pd(&m[3][2],_mm_set_pd(d,0.0));
#else
  m[0][0]=a;   m[0][1]=0.0; m[0][2]=0.0; m[0][3]=0.0;
  m[1][0]=0.0; m[1][1]=b;   m[1][2]=0.0; m[1][3]=0.0;
  m[2][0]=0.0; m[2][1]=0.0; m[2][2]=c;   m[2][3]=0.0;
  m[3][0]=0.0; m[3][1]=0.0; m[3][2]=0.0; m[3][3]=d;
#endif
  return *this;
  }


// Set value from components
FXMat4d& FXMat4d::set(FXdouble a00,FXdouble a01,FXdouble a02,FXdouble a03,FXdouble a10,FXdouble a11,FXdouble a12,FXdouble a13,FXdouble a20,FXdouble a21,FXdouble a22,FXdouble a23,FXdouble a30,FXdouble a31,FXdouble a32,FXdouble a33){
  m[0][0]=a00; m[0][1]=a01; m[0][2]=a02; m[0][3]=a03;
  m[1][0]=a10; m[1][1]=a11; m[1][2]=a12; m[1][3]=a13;
  m[2][0]=a20; m[2][1]=a21; m[2][2]=a22; m[2][3]=a23;
  m[3][0]=a30; m[3][1]=a31; m[3][2]=a32; m[3][3]=a33;
  return *this;
  }


// Set value from four vectors
FXMat4d& FXMat4d::set(const FXVec4d& a,const FXVec4d& b,const FXVec4d& c,const FXVec4d& d){
#if defined(FOX_HAS_AVX)
  _mm256_storeu_pd(&m[0][0],_mm256_loadu_pd(a));
  _mm256_storeu_pd(&m[1][0],_mm256_loadu_pd(b));
  _mm256_storeu_pd(&m[2][0],_mm256_loadu_pd(c));
  _mm256_storeu_pd(&m[3][0],_mm256_loadu_pd(d));
#elif defined(FOX_HAS_SSE2)
  _mm_storeu_pd(&m[0][0],_mm_loadu_pd(&a[0]));
  _mm_storeu_pd(&m[0][2],_mm_loadu_pd(&a[2]));
  _mm_storeu_pd(&m[1][0],_mm_loadu_pd(&b[0]));
  _mm_storeu_pd(&m[1][2],_mm_loadu_pd(&b[2]));
  _mm_storeu_pd(&m[2][0],_mm_loadu_pd(&c[0]));
  _mm_storeu_pd(&m[2][2],_mm_loadu_pd(&c[2]));
  _mm_storeu_pd(&m[3][0],_mm_loadu_pd(&d[0]));
  _mm_storeu_pd(&m[3][2],_mm_loadu_pd(&d[2]));
#else
  m[0][0]=a[0]; m[0][1]=a[1]; m[0][2]=a[2]; m[0][3]=a[3];
  m[1][0]=b[0]; m[1][1]=b[1]; m[1][2]=b[2]; m[1][3]=b[3];
  m[2][0]=c[0]; m[2][1]=c[1]; m[2][2]=c[2]; m[2][3]=c[3];
  m[3][0]=d[0]; m[3][1]=d[1]; m[3][2]=d[2]; m[3][3]=d[3];
#endif
  return *this;
  }


// Assignment from quaternion
FXMat4d& FXMat4d::set(const FXQuatd& quat){
  return set(FXMat3d(quat));
  }


// Add matrices
FXMat4d& FXMat4d::operator+=(const FXMat4d& s){
#if defined(FOX_HAS_AVX)
  _mm256_storeu_pd(&m[0][0],_mm256_add_pd(_mm256_loadu_pd(&m[0][0]),_mm256_loadu_pd(&s[0][0])));
  _mm256_storeu_pd(&m[1][0],_mm256_add_pd(_mm256_loadu_pd(&m[1][0]),_mm256_loadu_pd(&s[1][0])));
  _mm256_storeu_pd(&m[2][0],_mm256_add_pd(_mm256_loadu_pd(&m[2][0]),_mm256_loadu_pd(&s[2][0])));
  _mm256_storeu_pd(&m[3][0],_mm256_add_pd(_mm256_loadu_pd(&m[3][0]),_mm256_loadu_pd(&s[3][0])));
#elif defined(FOX_HAS_SSE2)
  _mm_storeu_pd(&m[0][0],_mm_add_pd(_mm_loadu_pd(&m[0][0]),_mm_loadu_pd(&s[0][0])));
  _mm_storeu_pd(&m[0][2],_mm_add_pd(_mm_loadu_pd(&m[0][2]),_mm_loadu_pd(&s[0][2])));
  _mm_storeu_pd(&m[1][0],_mm_add_pd(_mm_loadu_pd(&m[1][0]),_mm_loadu_pd(&s[1][0])));
  _mm_storeu_pd(&m[1][2],_mm_add_pd(_mm_loadu_pd(&m[1][2]),_mm_loadu_pd(&s[1][2])));
  _mm_storeu_pd(&m[2][0],_mm_add_pd(_mm_loadu_pd(&m[2][0]),_mm_loadu_pd(&s[2][0])));
  _mm_storeu_pd(&m[2][2],_mm_add_pd(_mm_loadu_pd(&m[2][2]),_mm_loadu_pd(&s[2][2])));
  _mm_storeu_pd(&m[3][0],_mm_add_pd(_mm_loadu_pd(&m[3][0]),_mm_loadu_pd(&s[3][0])));
  _mm_storeu_pd(&m[3][2],_mm_add_pd(_mm_loadu_pd(&m[3][2]),_mm_loadu_pd(&s[3][2])));
#else
  m[0][0]+=s[0][0]; m[0][1]+=s[0][1]; m[0][2]+=s[0][2]; m[0][3]+=s[0][3];
  m[1][0]+=s[1][0]; m[1][1]+=s[1][1]; m[1][2]+=s[1][2]; m[1][3]+=s[1][3];
  m[2][0]+=s[2][0]; m[2][1]+=s[2][1]; m[2][2]+=s[2][2]; m[2][3]+=s[2][3];
  m[3][0]+=s[3][0]; m[3][1]+=s[3][1]; m[3][2]+=s[3][2]; m[3][3]+=s[3][3];
#endif
  return *this;
  }


// Subtract matrices
FXMat4d& FXMat4d::operator-=(const FXMat4d& s){
#if defined(FOX_HAS_AVX)
  _mm256_storeu_pd(&m[0][0],_mm256_sub_pd(_mm256_loadu_pd(&m[0][0]),_mm256_loadu_pd(&s[0][0])));
  _mm256_storeu_pd(&m[1][0],_mm256_sub_pd(_mm256_loadu_pd(&m[1][0]),_mm256_loadu_pd(&s[1][0])));
  _mm256_storeu_pd(&m[2][0],_mm256_sub_pd(_mm256_loadu_pd(&m[2][0]),_mm256_loadu_pd(&s[2][0])));
  _mm256_storeu_pd(&m[3][0],_mm256_sub_pd(_mm256_loadu_pd(&m[3][0]),_mm256_loadu_pd(&s[3][0])));
#elif defined(FOX_HAS_SSE2)
  _mm_storeu_pd(&m[0][0],_mm_sub_pd(_mm_loadu_pd(&m[0][0]),_mm_loadu_pd(&s[0][0])));
  _mm_storeu_pd(&m[0][2],_mm_sub_pd(_mm_loadu_pd(&m[0][2]),_mm_loadu_pd(&s[0][2])));
  _mm_storeu_pd(&m[1][0],_mm_sub_pd(_mm_loadu_pd(&m[1][0]),_mm_loadu_pd(&s[1][0])));
  _mm_storeu_pd(&m[1][2],_mm_sub_pd(_mm_loadu_pd(&m[1][2]),_mm_loadu_pd(&s[1][2])));
  _mm_storeu_pd(&m[2][0],_mm_sub_pd(_mm_loadu_pd(&m[2][0]),_mm_loadu_pd(&s[2][0])));
  _mm_storeu_pd(&m[2][2],_mm_sub_pd(_mm_loadu_pd(&m[2][2]),_mm_loadu_pd(&s[2][2])));
  _mm_storeu_pd(&m[3][0],_mm_sub_pd(_mm_loadu_pd(&m[3][0]),_mm_loadu_pd(&s[3][0])));
  _mm_storeu_pd(&m[3][2],_mm_sub_pd(_mm_loadu_pd(&m[3][2]),_mm_loadu_pd(&s[3][2])));
#else
  m[0][0]-=s[0][0]; m[0][1]-=s[0][1]; m[0][2]-=s[0][2]; m[0][3]-=s[0][3];
  m[1][0]-=s[1][0]; m[1][1]-=s[1][1]; m[1][2]-=s[1][2]; m[1][3]-=s[1][3];
  m[2][0]-=s[2][0]; m[2][1]-=s[2][1]; m[2][2]-=s[2][2]; m[2][3]-=s[2][3];
  m[3][0]-=s[3][0]; m[3][1]-=s[3][1]; m[3][2]-=s[3][2]; m[3][3]-=s[3][3];
#endif
  return *this;
  }


// Multiply matrix by scalar
FXMat4d& FXMat4d::operator*=(FXdouble s){
#if defined(FOX_HAS_AVX)
  _mm256_storeu_pd(&m[0][0],_mm256_mul_pd(_mm256_loadu_pd(&m[0][0]),_mm256_set1_pd(s)));
  _mm256_storeu_pd(&m[1][0],_mm256_mul_pd(_mm256_loadu_pd(&m[1][0]),_mm256_set1_pd(s)));
  _mm256_storeu_pd(&m[2][0],_mm256_mul_pd(_mm256_loadu_pd(&m[2][0]),_mm256_set1_pd(s)));
  _mm256_storeu_pd(&m[3][0],_mm256_mul_pd(_mm256_loadu_pd(&m[3][0]),_mm256_set1_pd(s)));
#elif defined(FOX_HAS_SSE2)
  _mm_storeu_pd(&m[0][0],_mm_mul_pd(_mm_loadu_pd(&m[0][0]),_mm_set1_pd(s)));
  _mm_storeu_pd(&m[0][2],_mm_mul_pd(_mm_loadu_pd(&m[0][2]),_mm_set1_pd(s)));
  _mm_storeu_pd(&m[1][0],_mm_mul_pd(_mm_loadu_pd(&m[1][0]),_mm_set1_pd(s)));
  _mm_storeu_pd(&m[1][2],_mm_mul_pd(_mm_loadu_pd(&m[1][2]),_mm_set1_pd(s)));
  _mm_storeu_pd(&m[2][0],_mm_mul_pd(_mm_loadu_pd(&m[2][0]),_mm_set1_pd(s)));
  _mm_storeu_pd(&m[2][2],_mm_mul_pd(_mm_loadu_pd(&m[2][2]),_mm_set1_pd(s)));
  _mm_storeu_pd(&m[3][0],_mm_mul_pd(_mm_loadu_pd(&m[3][0]),_mm_set1_pd(s)));
  _mm_storeu_pd(&m[3][2],_mm_mul_pd(_mm_loadu_pd(&m[3][2]),_mm_set1_pd(s)));
#else
  m[0][0]*=s; m[0][1]*=s; m[0][2]*=s; m[0][3]*=s;
  m[1][0]*=s; m[1][1]*=s; m[1][2]*=s; m[2][3]*=s;
  m[2][0]*=s; m[2][1]*=s; m[2][2]*=s; m[3][3]*=s;
  m[3][0]*=s; m[3][1]*=s; m[3][2]*=s; m[3][3]*=s;
#endif
  return *this;
  }


// Multiply matrix by matrix
FXMat4d& FXMat4d::operator*=(const FXMat4d& s){
#if defined(FOX_HAS_AVX)
  __m256d b0=_mm256_loadu_pd(&s[0][0]);
  __m256d b1=_mm256_loadu_pd(&s[1][0]);
  __m256d b2=_mm256_loadu_pd(&s[2][0]);
  __m256d b3=_mm256_loadu_pd(&s[3][0]);
  __m256d xx,yy,zz,ww;
  xx=_mm256_set1_pd(m[0][0]);
  yy=_mm256_set1_pd(m[0][1]);
  zz=_mm256_set1_pd(m[0][2]);
  ww=_mm256_set1_pd(m[0][3]);
  _mm256_storeu_pd(m[0],_mm256_add_pd(_mm256_add_pd(_mm256_mul_pd(b0,xx),_mm256_mul_pd(b1,yy)),_mm256_add_pd(_mm256_mul_pd(b2,zz),_mm256_mul_pd(b3,ww))));
  xx=_mm256_set1_pd(m[1][0]);
  yy=_mm256_set1_pd(m[1][1]);
  zz=_mm256_set1_pd(m[1][2]);
  ww=_mm256_set1_pd(m[1][3]);
  _mm256_storeu_pd(m[1],_mm256_add_pd(_mm256_add_pd(_mm256_mul_pd(b0,xx),_mm256_mul_pd(b1,yy)),_mm256_add_pd(_mm256_mul_pd(b2,zz),_mm256_mul_pd(b3,ww))));
  xx=_mm256_set1_pd(m[2][0]);
  yy=_mm256_set1_pd(m[2][1]);
  zz=_mm256_set1_pd(m[2][2]);
  ww=_mm256_set1_pd(m[2][3]);
  _mm256_storeu_pd(m[2],_mm256_add_pd(_mm256_add_pd(_mm256_mul_pd(b0,xx),_mm256_mul_pd(b1,yy)),_mm256_add_pd(_mm256_mul_pd(b2,zz),_mm256_mul_pd(b3,ww))));
  xx=_mm256_set1_pd(m[3][0]);
  yy=_mm256_set1_pd(m[3][1]);
  zz=_mm256_set1_pd(m[3][2]);
  ww=_mm256_set1_pd(m[3][3]);
  _mm256_storeu_pd(m[3],_mm256_add_pd(_mm256_add_pd(_mm256_mul_pd(b0,xx),_mm256_mul_pd(b1,yy)),_mm256_add_pd(_mm256_mul_pd(b2,zz),_mm256_mul_pd(b3,ww))));
#elif defined(FOX_HAS_SSE2)
  __m128d b00=_mm_loadu_pd(&s[0][0]);
  __m128d b02=_mm_loadu_pd(&s[0][2]);
  __m128d b10=_mm_loadu_pd(&s[1][0]);
  __m128d b12=_mm_loadu_pd(&s[1][2]);
  __m128d b20=_mm_loadu_pd(&s[2][0]);
  __m128d b22=_mm_loadu_pd(&s[2][2]);
  __m128d b30=_mm_loadu_pd(&s[3][0]);
  __m128d b32=_mm_loadu_pd(&s[3][2]);
  __m128d xx,yy,zz,ww;
  xx=_mm_set1_pd(m[0][0]);
  yy=_mm_set1_pd(m[0][1]);
  zz=_mm_set1_pd(m[0][2]);
  ww=_mm_set1_pd(m[0][3]);
  _mm_storeu_pd(&m[0][0],_mm_add_pd(_mm_add_pd(_mm_add_pd(_mm_mul_pd(b00,xx),_mm_mul_pd(b10,yy)),_mm_mul_pd(b20,zz)),_mm_mul_pd(b30,ww)));
  _mm_storeu_pd(&m[0][2],_mm_add_pd(_mm_add_pd(_mm_add_pd(_mm_mul_pd(b02,xx),_mm_mul_pd(b12,yy)),_mm_mul_pd(b22,zz)),_mm_mul_pd(b32,ww)));
  xx=_mm_set1_pd(m[1][0]);
  yy=_mm_set1_pd(m[1][1]);
  zz=_mm_set1_pd(m[1][2]);
  ww=_mm_set1_pd(m[1][3]);
  _mm_storeu_pd(&m[1][0],_mm_add_pd(_mm_add_pd(_mm_add_pd(_mm_mul_pd(b00,xx),_mm_mul_pd(b10,yy)),_mm_mul_pd(b20,zz)),_mm_mul_pd(b30,ww)));
  _mm_storeu_pd(&m[1][2],_mm_add_pd(_mm_add_pd(_mm_add_pd(_mm_mul_pd(b02,xx),_mm_mul_pd(b12,yy)),_mm_mul_pd(b22,zz)),_mm_mul_pd(b32,ww)));
  xx=_mm_set1_pd(m[2][0]);
  yy=_mm_set1_pd(m[2][1]);
  zz=_mm_set1_pd(m[2][2]);
  ww=_mm_set1_pd(m[2][3]);
  _mm_storeu_pd(&m[2][0],_mm_add_pd(_mm_add_pd(_mm_add_pd(_mm_mul_pd(b00,xx),_mm_mul_pd(b10,yy)),_mm_mul_pd(b20,zz)),_mm_mul_pd(b30,ww)));
  _mm_storeu_pd(&m[2][2],_mm_add_pd(_mm_add_pd(_mm_add_pd(_mm_mul_pd(b02,xx),_mm_mul_pd(b12,yy)),_mm_mul_pd(b22,zz)),_mm_mul_pd(b32,ww)));
  xx=_mm_set1_pd(m[3][0]);
  yy=_mm_set1_pd(m[3][1]);
  zz=_mm_set1_pd(m[3][2]);
  ww=_mm_set1_pd(m[3][3]);
  _mm_storeu_pd(&m[3][0],_mm_add_pd(_mm_add_pd(_mm_add_pd(_mm_mul_pd(b00,xx),_mm_mul_pd(b10,yy)),_mm_mul_pd(b20,zz)),_mm_mul_pd(b30,ww)));
  _mm_storeu_pd(&m[3][2],_mm_add_pd(_mm_add_pd(_mm_add_pd(_mm_mul_pd(b02,xx),_mm_mul_pd(b12,yy)),_mm_mul_pd(b22,zz)),_mm_mul_pd(b32,ww)));
#else
  FXdouble x,y,z,w;
  x=m[0][0]; y=m[0][1]; z=m[0][2]; w=m[0][3];
  m[0][0]=x*s[0][0]+y*s[1][0]+z*s[2][0]+w*s[3][0];
  m[0][1]=x*s[0][1]+y*s[1][1]+z*s[2][1]+w*s[3][1];
  m[0][2]=x*s[0][2]+y*s[1][2]+z*s[2][2]+w*s[3][2];
  m[0][3]=x*s[0][3]+y*s[1][3]+z*s[2][3]+w*s[3][3];
  x=m[1][0]; y=m[1][1]; z=m[1][2]; w=m[1][3];
  m[1][0]=x*s[0][0]+y*s[1][0]+z*s[2][0]+w*s[3][0];
  m[1][1]=x*s[0][1]+y*s[1][1]+z*s[2][1]+w*s[3][1];
  m[1][2]=x*s[0][2]+y*s[1][2]+z*s[2][2]+w*s[3][2];
  m[1][3]=x*s[0][3]+y*s[1][3]+z*s[2][3]+w*s[3][3];
  x=m[2][0]; y=m[2][1]; z=m[2][2]; w=m[2][3];
  m[2][0]=x*s[0][0]+y*s[1][0]+z*s[2][0]+w*s[3][0];
  m[2][1]=x*s[0][1]+y*s[1][1]+z*s[2][1]+w*s[3][1];
  m[2][2]=x*s[0][2]+y*s[1][2]+z*s[2][2]+w*s[3][2];
  m[2][3]=x*s[0][3]+y*s[1][3]+z*s[2][3]+w*s[3][3];
  x=m[3][0]; y=m[3][1]; z=m[3][2]; w=m[3][3];
  m[3][0]=x*s[0][0]+y*s[1][0]+z*s[2][0]+w*s[3][0];
  m[3][1]=x*s[0][1]+y*s[1][1]+z*s[2][1]+w*s[3][1];
  m[3][2]=x*s[0][2]+y*s[1][2]+z*s[2][2]+w*s[3][2];
  m[3][3]=x*s[0][3]+y*s[1][3]+z*s[2][3]+w*s[3][3];
#endif
  return *this;
  }


// Divide matrix by scalar
FXMat4d& FXMat4d::operator/=(FXdouble s){
#if defined(FOX_HAS_AVX)
  _mm256_storeu_pd(&m[0][0],_mm256_div_pd(_mm256_loadu_pd(&m[0][0]),_mm256_set1_pd(s)));
  _mm256_storeu_pd(&m[1][0],_mm256_div_pd(_mm256_loadu_pd(&m[1][0]),_mm256_set1_pd(s)));
  _mm256_storeu_pd(&m[2][0],_mm256_div_pd(_mm256_loadu_pd(&m[2][0]),_mm256_set1_pd(s)));
  _mm256_storeu_pd(&m[3][0],_mm256_div_pd(_mm256_loadu_pd(&m[3][0]),_mm256_set1_pd(s)));
#elif defined(FOX_HAS_SSE2)
  _mm_storeu_pd(&m[0][0],_mm_div_pd(_mm_loadu_pd(&m[0][0]),_mm_set1_pd(s)));
  _mm_storeu_pd(&m[0][2],_mm_div_pd(_mm_loadu_pd(&m[0][2]),_mm_set1_pd(s)));
  _mm_storeu_pd(&m[1][0],_mm_div_pd(_mm_loadu_pd(&m[1][0]),_mm_set1_pd(s)));
  _mm_storeu_pd(&m[1][2],_mm_div_pd(_mm_loadu_pd(&m[1][2]),_mm_set1_pd(s)));
  _mm_storeu_pd(&m[2][0],_mm_div_pd(_mm_loadu_pd(&m[2][0]),_mm_set1_pd(s)));
  _mm_storeu_pd(&m[2][2],_mm_div_pd(_mm_loadu_pd(&m[2][2]),_mm_set1_pd(s)));
  _mm_storeu_pd(&m[3][0],_mm_div_pd(_mm_loadu_pd(&m[3][0]),_mm_set1_pd(s)));
  _mm_storeu_pd(&m[3][2],_mm_div_pd(_mm_loadu_pd(&m[3][2]),_mm_set1_pd(s)));
#else
  m[0][0]/=s; m[0][1]/=s; m[0][2]/=s; m[0][3]/=s;
  m[1][0]/=s; m[1][1]/=s; m[1][2]/=s; m[1][3]/=s;
  m[2][0]/=s; m[2][1]/=s; m[2][2]/=s; m[2][3]/=s;
  m[3][0]/=s; m[3][1]/=s; m[3][2]/=s; m[3][3]/=s;
#endif
  return *this;
  }


// Unary minus
FXMat4d FXMat4d::operator-() const {
#if defined(FOX_HAS_AVX)
  FXMat4d r;
  _mm256_storeu_pd(&r[0][0],_mm256_sub_pd(_mm256_set_pd(0.0,0.0,0.0,0.0),_mm256_loadu_pd(&m[0][0])));
  _mm256_storeu_pd(&r[1][0],_mm256_sub_pd(_mm256_set_pd(0.0,0.0,0.0,0.0),_mm256_loadu_pd(&m[1][0])));
  _mm256_storeu_pd(&r[2][0],_mm256_sub_pd(_mm256_set_pd(0.0,0.0,0.0,0.0),_mm256_loadu_pd(&m[2][0])));
  _mm256_storeu_pd(&r[3][0],_mm256_sub_pd(_mm256_set_pd(0.0,0.0,0.0,0.0),_mm256_loadu_pd(&m[3][0])));
  return r;
#elif defined(FOX_HAS_SSE2)
  FXMat4d r;
  _mm_storeu_pd(&r[0][0],_mm_sub_pd(_mm_set1_pd(0.0),_mm_loadu_pd(&m[0][0])));
  _mm_storeu_pd(&r[0][2],_mm_sub_pd(_mm_set1_pd(0.0),_mm_loadu_pd(&m[0][2])));
  _mm_storeu_pd(&r[1][0],_mm_sub_pd(_mm_set1_pd(0.0),_mm_loadu_pd(&m[1][0])));
  _mm_storeu_pd(&r[1][2],_mm_sub_pd(_mm_set1_pd(0.0),_mm_loadu_pd(&m[1][2])));
  _mm_storeu_pd(&r[2][0],_mm_sub_pd(_mm_set1_pd(0.0),_mm_loadu_pd(&m[2][0])));
  _mm_storeu_pd(&r[2][2],_mm_sub_pd(_mm_set1_pd(0.0),_mm_loadu_pd(&m[2][2])));
  _mm_storeu_pd(&r[3][0],_mm_sub_pd(_mm_set1_pd(0.0),_mm_loadu_pd(&m[3][0])));
  _mm_storeu_pd(&r[3][2],_mm_sub_pd(_mm_set1_pd(0.0),_mm_loadu_pd(&m[3][2])));
  return r;
#else
  return FXMat4d(-m[0][0],-m[0][1],-m[0][2],-m[0][3],
                 -m[1][0],-m[1][1],-m[1][2],-m[1][3],
                 -m[2][0],-m[2][1],-m[2][2],-m[2][3],
                 -m[3][0],-m[3][1],-m[3][2],-m[3][3]);
#endif
  }


// Set to identity matrix
FXMat4d& FXMat4d::identity(){
#if defined(FOX_HAS_AVX)
  _mm256_storeu_pd(&m[0][0],_mm256_set_pd(0.0,0.0,0.0,1.0));
  _mm256_storeu_pd(&m[1][0],_mm256_set_pd(0.0,0.0,1.0,0.0));
  _mm256_storeu_pd(&m[2][0],_mm256_set_pd(0.0,1.0,0.0,0.0));
  _mm256_storeu_pd(&m[3][0],_mm256_set_pd(1.0,0.0,0.0,0.0));
#elif defined(FOX_HAS_SSE2)
  _mm_storeu_pd(&m[0][0],_mm_set_pd(0.0,1.0));
  _mm_storeu_pd(&m[0][2],_mm_set_pd(0.0,0.0));
  _mm_storeu_pd(&m[1][0],_mm_set_pd(1.0,0.0));
  _mm_storeu_pd(&m[1][2],_mm_set_pd(0.0,0.0));
  _mm_storeu_pd(&m[2][0],_mm_set_pd(0.0,0.0));
  _mm_storeu_pd(&m[2][2],_mm_set_pd(0.0,1.0));
  _mm_storeu_pd(&m[3][0],_mm_set_pd(0.0,0.0));
  _mm_storeu_pd(&m[3][2],_mm_set_pd(1.0,0.0));
#else
  m[0][0]=1.0; m[0][1]=0.0; m[0][2]=0.0; m[0][3]=0.0;
  m[1][0]=0.0; m[1][1]=1.0; m[1][2]=0.0; m[1][3]=0.0;
  m[2][0]=0.0; m[2][1]=0.0; m[2][2]=1.0; m[2][3]=0.0;
  m[3][0]=0.0; m[3][1]=0.0; m[3][2]=0.0; m[3][3]=1.0;
#endif
  return *this;
  }


// Return true if identity matrix
FXbool FXMat4d::isIdentity() const {
  return m[0][0]==1.0 && m[0][1]==0.0 && m[0][2]==0.0 && m[0][3]==0.0 &&
         m[1][0]==0.0 && m[1][1]==1.0 && m[1][2]==0.0 && m[1][3]==0.0 &&
         m[2][0]==0.0 && m[2][1]==0.0 && m[2][2]==1.0 && m[2][3]==0.0 &&
         m[3][0]==0.0 && m[3][1]==0.0 && m[3][2]==0.0 && m[3][3]==1.0;
  }


// Set orthographic projection from view volume
FXMat4d& FXMat4d::setOrtho(FXdouble xlo,FXdouble xhi,FXdouble ylo,FXdouble yhi,FXdouble zlo,FXdouble zhi){
  FXdouble rl=xhi-xlo;
  FXdouble tb=yhi-ylo;
  FXdouble yh=zhi-zlo;
  return set(2.0/rl,0.0,0.0,0.0,0.0,2.0/tb,0.0,0.0,0.0,0.0,-2.0/yh,0.0,-(xhi+xlo)/rl,-(yhi+ylo)/tb,-(zhi+zlo)/yh,1.0);
  }


// Get view volume from orthographic projection
void FXMat4d::getOrtho(FXdouble& xlo,FXdouble& xhi,FXdouble& ylo,FXdouble& yhi,FXdouble& zlo,FXdouble& zhi) const {
  zlo= (m[3][2]+1.0)/m[2][2];
  zhi= (m[3][2]-1.0)/m[2][2];
  xlo=-(1.0+m[3][0])/m[0][0];
  xhi= (1.0-m[3][0])/m[0][0];
  ylo=-(1.0+m[3][1])/m[1][1];
  yhi= (1.0-m[3][1])/m[1][1];
  }


// Set to inverse orthographic projection
FXMat4d& FXMat4d::setInverseOrtho(FXdouble xlo,FXdouble xhi,FXdouble ylo,FXdouble yhi,FXdouble zlo,FXdouble zhi){
  FXdouble rl=xhi-xlo;
  FXdouble tb=yhi-ylo;
  FXdouble yh=zhi-zlo;
  return set(0.5*rl,0.0,0.0,0.0,0.0,0.5*tb,0.0,0.0,0.0,0.0,-0.5*yh,0.0,0.5*(xhi+xlo),0.5*(yhi+ylo),0.5*(zhi+zlo),1.0);
  }


// Set to perspective projection from view volume
FXMat4d& FXMat4d::setFrustum(FXdouble xlo,FXdouble xhi,FXdouble ylo,FXdouble yhi,FXdouble zlo,FXdouble zhi){
  FXdouble rl=xhi-xlo;
  FXdouble tb=yhi-ylo;
  FXdouble yh=zhi-zlo;
  return set(2.0*zlo/rl,0.0,0.0,0.0,0.0,2.0*zlo/tb,0.0,0.0,(xhi+xlo)/rl,(yhi+ylo)/tb,-(zhi+zlo)/yh,-1.0,0.0,0.0,-2.0*zhi*zlo/yh,0.0);
  }


// Get view volume from perspective projection
void FXMat4d::getFrustum(FXdouble& xlo,FXdouble& xhi,FXdouble& ylo,FXdouble& yhi,FXdouble& zlo,FXdouble& zhi) const {
  zlo=m[3][2]/(m[2][2]-1.0);
  zhi=m[3][2]/(m[2][2]+1.0);
  xlo=zlo*(m[2][0]-1.0)/m[0][0];
  xhi=zlo*(m[2][0]+1.0)/m[0][0];
  yhi=zlo*(m[2][1]+1.0)/m[1][1];
  ylo=zlo*(m[2][1]-1.0)/m[1][1];
  }


// Set to inverse perspective projection from view volume
FXMat4d& FXMat4d::setInverseFrustum(FXdouble xlo,FXdouble xhi,FXdouble ylo,FXdouble yhi,FXdouble zlo,FXdouble zhi){
  FXdouble rl=xhi-xlo;
  FXdouble tb=yhi-ylo;
  FXdouble yh=zhi-zlo;
  return set(0.5*rl/zlo,0.0,0.0,0.0,0.0,0.5*tb/zlo,0.0,0.0,0.0,0.0,0.0,-0.5*yh/(zhi*zlo),0.5*(xhi+xlo)/zlo,0.5*(yhi+ylo)/zlo,-1.0,0.5*(zhi+zlo)/(zhi*zlo));
  }


// Make left hand matrix
FXMat4d& FXMat4d::left(){
  m[2][0]= -m[2][0];
  m[2][1]= -m[2][1];
  m[2][2]= -m[2][2];
  m[2][3]= -m[2][3];
  return *this;
  }


// Multiply by rotation matrix
FXMat4d& FXMat4d::rot(const FXMat3d& r){
  FXdouble x,y,z;
  x=m[0][0]; y=m[1][0]; z=m[2][0];
  m[0][0]=x*r[0][0]+y*r[0][1]+z*r[0][2];
  m[1][0]=x*r[1][0]+y*r[1][1]+z*r[1][2];
  m[2][0]=x*r[2][0]+y*r[2][1]+z*r[2][2];
  x=m[0][1]; y=m[1][1]; z=m[2][1];
  m[0][1]=x*r[0][0]+y*r[0][1]+z*r[0][2];
  m[1][1]=x*r[1][0]+y*r[1][1]+z*r[1][2];
  m[2][1]=x*r[2][0]+y*r[2][1]+z*r[2][2];
  x=m[0][2]; y=m[1][2]; z=m[2][2];
  m[0][2]=x*r[0][0]+y*r[0][1]+z*r[0][2];
  m[1][2]=x*r[1][0]+y*r[1][1]+z*r[1][2];
  m[2][2]=x*r[2][0]+y*r[2][1]+z*r[2][2];
  x=m[0][3]; y=m[1][3]; z=m[2][3];
  m[0][3]=x*r[0][0]+y*r[0][1]+z*r[0][2];
  m[1][3]=x*r[1][0]+y*r[1][1]+z*r[1][2];
  m[2][3]=x*r[2][0]+y*r[2][1]+z*r[2][2];
  return *this;
  }


// Rotate using quaternion
FXMat4d& FXMat4d::rot(const FXQuatd& q){
  return rot(FXMat3d(q));
  }


// Multiply by rotation c,s about unit axis
FXMat4d& FXMat4d::rot(const FXVec3d& v,FXdouble c,FXdouble s){
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
  return rot(FXMat3d(t*xx+c,t*xy+zs,t*zx-ys,t*xy-zs,t*yy+c,t*yz+xs,t*zx+ys,t*yz-xs,t*zz+c));
  }


// Multiply by rotation of phi about unit axis
FXMat4d& FXMat4d::rot(const FXVec3d& v,FXdouble phi){
  return rot(v,Math::cos(phi),Math::sin(phi));
  }


// Rotate about x-axis
FXMat4d& FXMat4d::xrot(FXdouble c,FXdouble s){
#if defined(FOX_HAS_AVX)
  __m256d cc=_mm256_set1_pd(c);
  __m256d ss=_mm256_set1_pd(s);
  __m256d uu=_mm256_loadu_pd(m[1]);
  __m256d vv=_mm256_loadu_pd(m[2]);
  _mm256_storeu_pd(m[1],_mm256_add_pd(_mm256_mul_pd(cc,uu),_mm256_mul_pd(ss,vv)));
  _mm256_storeu_pd(m[2],_mm256_sub_pd(_mm256_mul_pd(cc,vv),_mm256_mul_pd(ss,uu)));
#elif defined(FOX_HAS_SSE2)
  __m128d cc=_mm_set1_pd(c);
  __m128d ss=_mm_set1_pd(s);
  __m128d uu0=_mm_loadu_pd(&m[1][0]);
  __m128d uu2=_mm_loadu_pd(&m[1][2]);
  __m128d vv0=_mm_loadu_pd(&m[2][0]);
  __m128d vv2=_mm_loadu_pd(&m[2][2]);
  _mm_storeu_pd(&m[1][0],_mm_add_pd(_mm_mul_pd(cc,uu0),_mm_mul_pd(ss,vv0)));
  _mm_storeu_pd(&m[1][2],_mm_add_pd(_mm_mul_pd(cc,uu2),_mm_mul_pd(ss,vv2)));
  _mm_storeu_pd(&m[2][0],_mm_sub_pd(_mm_mul_pd(cc,vv0),_mm_mul_pd(ss,uu0)));
  _mm_storeu_pd(&m[2][2],_mm_sub_pd(_mm_mul_pd(cc,vv2),_mm_mul_pd(ss,uu2)));
#else
  FXdouble u,v;
  u=m[1][0]; v=m[2][0]; m[1][0]=c*u+s*v; m[2][0]=c*v-s*u;
  u=m[1][1]; v=m[2][1]; m[1][1]=c*u+s*v; m[2][1]=c*v-s*u;
  u=m[1][2]; v=m[2][2]; m[1][2]=c*u+s*v; m[2][2]=c*v-s*u;
  u=m[1][3]; v=m[2][3]; m[1][3]=c*u+s*v; m[2][3]=c*v-s*u;
#endif
  return *this;
  }


// Rotate by angle about x-axis
FXMat4d& FXMat4d::xrot(FXdouble phi){
  return xrot(Math::cos(phi),Math::sin(phi));
  }


// Rotate about y-axis
FXMat4d& FXMat4d::yrot(FXdouble c,FXdouble s){
#if defined(FOX_HAS_AVX)
  __m256d cc=_mm256_set1_pd(c);
  __m256d ss=_mm256_set1_pd(s);
  __m256d uu=_mm256_loadu_pd(m[0]);
  __m256d vv=_mm256_loadu_pd(m[2]);
  _mm256_storeu_pd(m[0],_mm256_sub_pd(_mm256_mul_pd(cc,uu),_mm256_mul_pd(ss,vv)));
  _mm256_storeu_pd(m[2],_mm256_add_pd(_mm256_mul_pd(cc,vv),_mm256_mul_pd(ss,uu)));
#elif defined(FOX_HAS_SSE2)
  __m128d cc=_mm_set1_pd(c);
  __m128d ss=_mm_set1_pd(s);
  __m128d uu0=_mm_loadu_pd(&m[0][0]);
  __m128d uu2=_mm_loadu_pd(&m[0][2]);
  __m128d vv0=_mm_loadu_pd(&m[2][0]);
  __m128d vv2=_mm_loadu_pd(&m[2][2]);
  _mm_storeu_pd(&m[0][0],_mm_sub_pd(_mm_mul_pd(cc,uu0),_mm_mul_pd(ss,vv0)));
  _mm_storeu_pd(&m[0][2],_mm_sub_pd(_mm_mul_pd(cc,uu2),_mm_mul_pd(ss,vv2)));
  _mm_storeu_pd(&m[2][0],_mm_add_pd(_mm_mul_pd(cc,vv0),_mm_mul_pd(ss,uu0)));
  _mm_storeu_pd(&m[2][2],_mm_add_pd(_mm_mul_pd(cc,vv2),_mm_mul_pd(ss,uu2)));
#else
  FXdouble u,v;
  u=m[0][0]; v=m[2][0]; m[0][0]=c*u-s*v; m[2][0]=c*v+s*u;
  u=m[0][1]; v=m[2][1]; m[0][1]=c*u-s*v; m[2][1]=c*v+s*u;
  u=m[0][2]; v=m[2][2]; m[0][2]=c*u-s*v; m[2][2]=c*v+s*u;
  u=m[0][3]; v=m[2][3]; m[0][3]=c*u-s*v; m[2][3]=c*v+s*u;
#endif
  return *this;
  }


// Rotate by angle about y-axis
FXMat4d& FXMat4d::yrot(FXdouble phi){
  return yrot(Math::cos(phi),Math::sin(phi));
  }


// Rotate about z-axis
FXMat4d& FXMat4d::zrot(FXdouble c,FXdouble s){
#if defined(FOX_HAS_AVX)
  __m256d cc=_mm256_set1_pd(c);
  __m256d ss=_mm256_set1_pd(s);
  __m256d uu=_mm256_loadu_pd(m[0]);
  __m256d vv=_mm256_loadu_pd(m[1]);
  _mm256_storeu_pd(m[0],_mm256_add_pd(_mm256_mul_pd(cc,uu),_mm256_mul_pd(ss,vv)));
  _mm256_storeu_pd(m[1],_mm256_sub_pd(_mm256_mul_pd(cc,vv),_mm256_mul_pd(ss,uu)));
#elif defined(FOX_HAS_SSE2)
  __m128d cc=_mm_set1_pd(c);
  __m128d ss=_mm_set1_pd(s);
  __m128d uu0=_mm_loadu_pd(&m[0][0]);
  __m128d uu2=_mm_loadu_pd(&m[0][2]);
  __m128d vv0=_mm_loadu_pd(&m[1][0]);
  __m128d vv2=_mm_loadu_pd(&m[1][2]);
  _mm_storeu_pd(&m[0][0],_mm_add_pd(_mm_mul_pd(cc,uu0),_mm_mul_pd(ss,vv0)));
  _mm_storeu_pd(&m[0][2],_mm_add_pd(_mm_mul_pd(cc,uu2),_mm_mul_pd(ss,vv2)));
  _mm_storeu_pd(&m[1][0],_mm_sub_pd(_mm_mul_pd(cc,vv0),_mm_mul_pd(ss,uu0)));
  _mm_storeu_pd(&m[1][2],_mm_sub_pd(_mm_mul_pd(cc,vv2),_mm_mul_pd(ss,uu2)));
#else
  FXdouble u,v;
  u=m[0][0]; v=m[1][0]; m[0][0]=c*u+s*v; m[1][0]=c*v-s*u;
  u=m[0][1]; v=m[1][1]; m[0][1]=c*u+s*v; m[1][1]=c*v-s*u;
  u=m[0][2]; v=m[1][2]; m[0][2]=c*u+s*v; m[1][2]=c*v-s*u;
  u=m[0][3]; v=m[1][3]; m[0][3]=c*u+s*v; m[1][3]=c*v-s*u;
#endif
  return *this;
  }


// Rotate by angle about z-axis
FXMat4d& FXMat4d::zrot(FXdouble phi){
  return zrot(Math::cos(phi),Math::sin(phi));
  }


// Look at
FXMat4d& FXMat4d::look(const FXVec3d& from,const FXVec3d& to,const FXVec3d& up){
  FXdouble x0,x1,x2,tx,ty,tz;
  FXVec3d rz,rx,ry;
  rz=normalize(from-to);
  rx=normalize(up^rz);
  ry=normalize(rz^rx);
  tx= -from[0]*rx[0]-from[1]*rx[1]-from[2]*rx[2];
  ty= -from[0]*ry[0]-from[1]*ry[1]-from[2]*ry[2];
  tz= -from[0]*rz[0]-from[1]*rz[1]-from[2]*rz[2];
  x0=m[0][0]; x1=m[0][1]; x2=m[0][2];
  m[0][0]=rx[0]*x0+rx[1]*x1+rx[2]*x2+tx*m[0][3];
  m[0][1]=ry[0]*x0+ry[1]*x1+ry[2]*x2+ty*m[0][3];
  m[0][2]=rz[0]*x0+rz[1]*x1+rz[2]*x2+tz*m[0][3];
  x0=m[1][0]; x1=m[1][1]; x2=m[1][2];
  m[1][0]=rx[0]*x0+rx[1]*x1+rx[2]*x2+tx*m[1][3];
  m[1][1]=ry[0]*x0+ry[1]*x1+ry[2]*x2+ty*m[1][3];
  m[1][2]=rz[0]*x0+rz[1]*x1+rz[2]*x2+tz*m[1][3];
  x0=m[2][0]; x1=m[2][1]; x2=m[2][2];
  m[2][0]=rx[0]*x0+rx[1]*x1+rx[2]*x2+tx*m[2][3];
  m[2][1]=ry[0]*x0+ry[1]*x1+ry[2]*x2+ty*m[2][3];
  m[2][2]=rz[0]*x0+rz[1]*x1+rz[2]*x2+tz*m[2][3];
  x0=m[3][0]; x1=m[3][1]; x2=m[3][2];
  m[3][0]=rx[0]*x0+rx[1]*x1+rx[2]*x2+tx*m[3][3];
  m[3][1]=ry[0]*x0+ry[1]*x1+ry[2]*x2+ty*m[3][3];
  m[3][2]=rz[0]*x0+rz[1]*x1+rz[2]*x2+tz*m[3][3];
  return *this;
  }


// Translate
FXMat4d& FXMat4d::trans(FXdouble tx,FXdouble ty,FXdouble tz){
#if defined(FOX_HAS_AVX)
  __m256d ttx=_mm256_set1_pd(tx);
  __m256d tty=_mm256_set1_pd(ty);
  __m256d ttz=_mm256_set1_pd(tz);
  __m256d r0=_mm256_mul_pd(_mm256_loadu_pd(m[0]),ttx);
  __m256d r1=_mm256_mul_pd(_mm256_loadu_pd(m[1]),tty);
  __m256d r2=_mm256_mul_pd(_mm256_loadu_pd(m[2]),ttz);
  __m256d r3=_mm256_loadu_pd(m[3]);
  _mm256_storeu_pd(m[3],_mm256_add_pd(_mm256_add_pd(r0,r1),_mm256_add_pd(r2,r3)));
#elif defined(FOX_HAS_SSE2)
  __m128d ttx=_mm_set1_pd(tx);
  __m128d tty=_mm_set1_pd(ty);
  __m128d ttz=_mm_set1_pd(tz);
  __m128d r00=_mm_mul_pd(_mm_loadu_pd(&m[0][0]),ttx);
  __m128d r02=_mm_mul_pd(_mm_loadu_pd(&m[0][2]),ttx);
  __m128d r10=_mm_mul_pd(_mm_loadu_pd(&m[1][0]),tty);
  __m128d r12=_mm_mul_pd(_mm_loadu_pd(&m[1][2]),tty);
  __m128d r20=_mm_mul_pd(_mm_loadu_pd(&m[2][0]),ttz);
  __m128d r22=_mm_mul_pd(_mm_loadu_pd(&m[2][2]),ttz);
  __m128d r30=_mm_loadu_pd(&m[3][0]);
  __m128d r32=_mm_loadu_pd(&m[3][2]);
  _mm_storeu_pd(&m[3][0],_mm_add_pd(_mm_add_pd(r00,r10),_mm_add_pd(r20,r30)));
  _mm_storeu_pd(&m[3][2],_mm_add_pd(_mm_add_pd(r02,r12),_mm_add_pd(r22,r32)));
#else
  m[3][0]=m[3][0]+tx*m[0][0]+ty*m[1][0]+tz*m[2][0];
  m[3][1]=m[3][1]+tx*m[0][1]+ty*m[1][1]+tz*m[2][1];
  m[3][2]=m[3][2]+tx*m[0][2]+ty*m[1][2]+tz*m[2][2];
  m[3][3]=m[3][3]+tx*m[0][3]+ty*m[1][3]+tz*m[2][3];
#endif
  return *this;
  }


// Translate over vector
FXMat4d& FXMat4d::trans(const FXVec3d& v){
  return trans(v[0],v[1],v[2]);
  }


// Scale unqual
FXMat4d& FXMat4d::scale(FXdouble sx,FXdouble sy,FXdouble sz){
#if defined(FOX_HAS_AVX)
  __m256d ssx=_mm256_set1_pd(sx);
  __m256d ssy=_mm256_set1_pd(sy);
  __m256d ssz=_mm256_set1_pd(sz);
  _mm256_storeu_pd(m[0],_mm256_mul_pd(_mm256_loadu_pd(m[0]),ssx));
  _mm256_storeu_pd(m[1],_mm256_mul_pd(_mm256_loadu_pd(m[1]),ssy));
  _mm256_storeu_pd(m[2],_mm256_mul_pd(_mm256_loadu_pd(m[2]),ssz));
#elif defined(FOX_HAS_SSE2)
  __m128d ssx=_mm_set1_pd(sx);
  __m128d ssy=_mm_set1_pd(sy);
  __m128d ssz=_mm_set1_pd(sz);
  _mm_storeu_pd(&m[0][0],_mm_mul_pd(_mm_loadu_pd(&m[0][0]),ssx));
  _mm_storeu_pd(&m[0][2],_mm_mul_pd(_mm_loadu_pd(&m[0][2]),ssx));
  _mm_storeu_pd(&m[1][0],_mm_mul_pd(_mm_loadu_pd(&m[1][0]),ssy));
  _mm_storeu_pd(&m[1][2],_mm_mul_pd(_mm_loadu_pd(&m[1][2]),ssy));
  _mm_storeu_pd(&m[2][0],_mm_mul_pd(_mm_loadu_pd(&m[2][0]),ssz));
  _mm_storeu_pd(&m[2][2],_mm_mul_pd(_mm_loadu_pd(&m[2][2]),ssz));
#else
  m[0][0]*=sx; m[0][1]*=sx; m[0][2]*=sx; m[0][3]*=sx;
  m[1][0]*=sy; m[1][1]*=sy; m[1][2]*=sy; m[1][3]*=sy;
  m[2][0]*=sz; m[2][1]*=sz; m[2][2]*=sz; m[2][3]*=sz;
#endif
  return *this;
  }


// Scale unqual
FXMat4d& FXMat4d::scale(const FXVec3d& v){
  return scale(v[0],v[1],v[2]);
  }


// Scale uniform
FXMat4d& FXMat4d::scale(FXdouble s){
  return scale(s,s,s);
  }


// Calculate determinant
FXdouble FXMat4d::det() const {
  return (m[0][0]*m[1][1]-m[0][1]*m[1][0]) * (m[2][2]*m[3][3]-m[2][3]*m[3][2]) -
         (m[0][0]*m[1][2]-m[0][2]*m[1][0]) * (m[2][1]*m[3][3]-m[2][3]*m[3][1]) +
         (m[0][0]*m[1][3]-m[0][3]*m[1][0]) * (m[2][1]*m[3][2]-m[2][2]*m[3][1]) +
         (m[0][1]*m[1][2]-m[0][2]*m[1][1]) * (m[2][0]*m[3][3]-m[2][3]*m[3][0]) -
         (m[0][1]*m[1][3]-m[0][3]*m[1][1]) * (m[2][0]*m[3][2]-m[2][2]*m[3][0]) +
         (m[0][2]*m[1][3]-m[0][3]*m[1][2]) * (m[2][0]*m[3][1]-m[2][1]*m[3][0]);
  }


// Transpose matrix
FXMat4d FXMat4d::transpose() const {
#if defined(FOX_HAS_AVX)
  __m256d m0=_mm256_loadu_pd(m[0]);    // a3 a2 a1 a0
  __m256d m1=_mm256_loadu_pd(m[1]);    // b3 b2 b1 b0
  __m256d m2=_mm256_loadu_pd(m[2]);    // c3 c2 c1 c0
  __m256d m3=_mm256_loadu_pd(m[3]);    // d3 d2 d1 d0
  __m256d s0=_mm256_unpackhi_pd(m0,m1);        // b3 a3 b1 a1
  __m256d s1=_mm256_unpacklo_pd(m0,m1);        // b2 a2 b0 a0
  __m256d s2=_mm256_unpackhi_pd(m2,m3);        // d3 c3 d1 c1
  __m256d s3=_mm256_unpacklo_pd(m2,m3);        // d2 c2 d0 c0
  FXMat4d r;
  _mm256_storeu_pd(r[0],_mm256_permute2f128_pd(s1,s3,0x20));    // d0 c0 b0 a0
  _mm256_storeu_pd(r[1],_mm256_permute2f128_pd(s0,s2,0x20));    // d1 c1 b1 a1
  _mm256_storeu_pd(r[2],_mm256_permute2f128_pd(s1,s3,0x31));    // d2 c2 b2 a2
  _mm256_storeu_pd(r[3],_mm256_permute2f128_pd(s0,s2,0x31));    // d3 c3 b3 a3
  return r;
#elif defined(FOX_HAS_SSE2)
  __m128d m00=_mm_loadu_pd(&m[0][0]);
  __m128d m02=_mm_loadu_pd(&m[0][2]);
  __m128d m10=_mm_loadu_pd(&m[1][0]);
  __m128d m12=_mm_loadu_pd(&m[1][2]);
  __m128d m20=_mm_loadu_pd(&m[2][0]);
  __m128d m22=_mm_loadu_pd(&m[2][2]);
  __m128d m30=_mm_loadu_pd(&m[3][0]);
  __m128d m32=_mm_loadu_pd(&m[3][2]);
  FXMat4d r;
  _mm_storeu_pd(&r[0][0],_mm_unpacklo_pd(m00,m10));
  _mm_storeu_pd(&r[0][2],_mm_unpacklo_pd(m20,m30));
  _mm_storeu_pd(&r[1][0],_mm_unpackhi_pd(m00,m10));
  _mm_storeu_pd(&r[1][2],_mm_unpackhi_pd(m20,m30));
  _mm_storeu_pd(&r[2][0],_mm_unpacklo_pd(m02,m12));
  _mm_storeu_pd(&r[2][2],_mm_unpacklo_pd(m22,m32));
  _mm_storeu_pd(&r[3][0],_mm_unpackhi_pd(m02,m12));
  _mm_storeu_pd(&r[3][2],_mm_unpackhi_pd(m22,m32));
  return r;
#else
  return FXMat4d(m[0][0],m[1][0],m[2][0],m[3][0],
                 m[0][1],m[1][1],m[2][1],m[3][1],
                 m[0][2],m[1][2],m[2][2],m[3][2],
                 m[0][3],m[1][3],m[2][3],m[3][3]);
#endif
  }



// Invert matrix
FXMat4d FXMat4d::invert() const {
  FXMat4d r;
  FXdouble a0=m[0][0]*m[1][1]-m[0][1]*m[1][0];
  FXdouble a1=m[0][0]*m[1][2]-m[0][2]*m[1][0];
  FXdouble a2=m[0][0]*m[1][3]-m[0][3]*m[1][0];
  FXdouble a3=m[0][1]*m[1][2]-m[0][2]*m[1][1];
  FXdouble a4=m[0][1]*m[1][3]-m[0][3]*m[1][1];
  FXdouble a5=m[0][2]*m[1][3]-m[0][3]*m[1][2];
  FXdouble b0=m[2][0]*m[3][1]-m[2][1]*m[3][0];
  FXdouble b1=m[2][0]*m[3][2]-m[2][2]*m[3][0];
  FXdouble b2=m[2][0]*m[3][3]-m[2][3]*m[3][0];
  FXdouble b3=m[2][1]*m[3][2]-m[2][2]*m[3][1];
  FXdouble b4=m[2][1]*m[3][3]-m[2][3]*m[3][1];
  FXdouble b5=m[2][2]*m[3][3]-m[2][3]*m[3][2];
  FXdouble dd=a0*b5-a1*b4+a2*b3+a3*b2-a4*b1+a5*b0;
  FXASSERT(dd!=0.0);
  dd=1.0/dd;
  r[0][0]=(m[1][1]*b5-m[1][2]*b4+m[1][3]*b3)*dd;
  r[1][0]=(m[1][2]*b2-m[1][0]*b5-m[1][3]*b1)*dd;
  r[2][0]=(m[1][0]*b4-m[1][1]*b2+m[1][3]*b0)*dd;
  r[3][0]=(m[1][1]*b1-m[1][0]*b3-m[1][2]*b0)*dd;
  r[0][1]=(m[0][2]*b4-m[0][1]*b5-m[0][3]*b3)*dd;
  r[1][1]=(m[0][0]*b5-m[0][2]*b2+m[0][3]*b1)*dd;
  r[2][1]=(m[0][1]*b2-m[0][0]*b4-m[0][3]*b0)*dd;
  r[3][1]=(m[0][0]*b3-m[0][1]*b1+m[0][2]*b0)*dd;
  r[0][2]=(m[3][1]*a5-m[3][2]*a4+m[3][3]*a3)*dd;
  r[1][2]=(m[3][2]*a2-m[3][0]*a5-m[3][3]*a1)*dd;
  r[2][2]=(m[3][0]*a4-m[3][1]*a2+m[3][3]*a0)*dd;
  r[3][2]=(m[3][1]*a1-m[3][0]*a3-m[3][2]*a0)*dd;
  r[0][3]=(m[2][2]*a4-m[2][1]*a5-m[2][3]*a3)*dd;
  r[1][3]=(m[2][0]*a5-m[2][2]*a2+m[2][3]*a1)*dd;
  r[2][3]=(m[2][1]*a2-m[2][0]*a4-m[2][3]*a0)*dd;
  r[3][3]=(m[2][0]*a3-m[2][1]*a1+m[2][2]*a0)*dd;
  return r;
  }


// Invert affine matrix
FXMat4d FXMat4d::affineInvert() const {
  FXdouble dd;
  FXMat4d r;
  r[0][0]=m[1][1]*m[2][2]-m[1][2]*m[2][1];
  r[0][1]=m[0][2]*m[2][1]-m[0][1]*m[2][2];
  r[0][2]=m[0][1]*m[1][2]-m[0][2]*m[1][1];
  r[0][3]=0.0;
  r[1][0]=m[1][2]*m[2][0]-m[1][0]*m[2][2];
  r[1][1]=m[0][0]*m[2][2]-m[0][2]*m[2][0];
  r[1][2]=m[0][2]*m[1][0]-m[0][0]*m[1][2];
  r[1][3]=0.0;
  r[2][0]=m[1][0]*m[2][1]-m[1][1]*m[2][0];
  r[2][1]=m[0][1]*m[2][0]-m[0][0]*m[2][1];
  r[2][2]=m[0][0]*m[1][1]-m[0][1]*m[1][0];
  r[2][3]=0.0;
  dd=m[0][0]*r[0][0]+m[0][1]*r[1][0]+m[0][2]*r[2][0];
  FXASSERT(dd!=0.0);
  dd=1.0/dd;
  r[0][0]*=dd;
  r[0][1]*=dd;
  r[0][2]*=dd;
  r[1][0]*=dd;
  r[1][1]*=dd;
  r[1][2]*=dd;
  r[2][0]*=dd;
  r[2][1]*=dd;
  r[2][2]*=dd;
  r[3][0]=-(r[0][0]*m[3][0]+r[1][0]*m[3][1]+r[2][0]*m[3][2]);
  r[3][1]=-(r[0][1]*m[3][0]+r[1][1]*m[3][1]+r[2][1]*m[3][2]);
  r[3][2]=-(r[0][2]*m[3][0]+r[1][2]*m[3][1]+r[2][2]*m[3][2]);
  r[3][3]=1.0;
  return r;
  }


// Invert rigid body transform matrix
FXMat4d FXMat4d::rigidInvert() const {
  FXdouble ss;
  FXMat4d r;
  ss=1.0/(m[0][0]*m[0][0]+m[0][1]*m[0][1]+m[0][2]*m[0][2]);
  r[0][0]=m[0][0]*ss;
  r[0][1]=m[1][0]*ss;
  r[0][2]=m[2][0]*ss;
  r[0][3]=0.0;
  r[1][0]=m[0][1]*ss;
  r[1][1]=m[1][1]*ss;
  r[1][2]=m[2][1]*ss;
  r[1][3]=0.0;
  r[2][0]=m[0][2]*ss;
  r[2][1]=m[1][2]*ss;
  r[2][2]=m[2][2]*ss;
  r[2][3]=0.0;
  r[3][0]=-(r[0][0]*m[3][0]+r[1][0]*m[3][1]+r[2][0]*m[3][2]);
  r[3][1]=-(r[0][1]*m[3][0]+r[1][1]*m[3][1]+r[2][1]*m[3][2]);
  r[3][2]=-(r[0][2]*m[3][0]+r[1][2]*m[3][1]+r[2][2]*m[3][2]);
  r[3][3]=1.0;
  return r;
  }


// Return normal-transformation matrix (inverse transpose of upper 3x3 block)
FXMat3d FXMat4d::normalMatrix() const {
  FXdouble dd;
  FXMat3d res;
  res[0][0]=m[1][1]*m[2][2]-m[1][2]*m[2][1];
  res[0][1]=m[1][2]*m[2][0]-m[1][0]*m[2][2];
  res[0][2]=m[1][0]*m[2][1]-m[1][1]*m[2][0];
  res[1][0]=m[0][2]*m[2][1]-m[0][1]*m[2][2];
  res[1][1]=m[0][0]*m[2][2]-m[0][2]*m[2][0];
  res[1][2]=m[0][1]*m[2][0]-m[0][0]*m[2][1];
  res[2][0]=m[0][1]*m[1][2]-m[0][2]*m[1][1];
  res[2][1]=m[0][2]*m[1][0]-m[0][0]*m[1][2];
  res[2][2]=m[0][0]*m[1][1]-m[0][1]*m[1][0];
  dd=m[0][0]*res[0][0]+m[0][1]*res[0][1]+m[0][2]*res[0][2];
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
FXMat4d orthogonalize(const FXMat4d& m){
  FXMat4d result(m);
  result[0]/=result[0].length();
  result[1]-=result[0]*(result[1]*result[0]);
  result[1]/=result[1].length();
  result[2]-=result[0]*(result[2]*result[0]);
  result[2]-=result[1]*(result[2]*result[1]);
  result[2]/=result[2].length();
  result[3]-=result[0]*(result[3]*result[0]);
  result[3]-=result[1]*(result[3]*result[1]);
  result[3]-=result[2]*(result[3]*result[2]);
  result[3]/=result[3].length();
  return result;
  }


// Matrix times vector
FXVec3d operator*(const FXMat4d& m,const FXVec3d& v){
#if defined(FOX_HAS_AVX)
  __m256d vv=_mm256_set_pd(1.0,v[2],v[1],v[0]);
  __m256d r0=_mm256_mul_pd(_mm256_loadu_pd(&m[0][0]),vv);      // m03  m02*v2  m01*v1  m00*v0
  __m256d r1=_mm256_mul_pd(_mm256_loadu_pd(&m[1][0]),vv);      // m13  m12*v2  m11*v1  m10*v0
  __m256d r2=_mm256_mul_pd(_mm256_loadu_pd(&m[2][0]),vv);      // m23  m22*v2  m21*v1  m20*v0
  __m256d r3=_mm256_mul_pd(_mm256_loadu_pd(&m[3][0]),vv);      // m33  m32*v2  m31*v1  m30*v0
  __m256d t0=_mm256_unpackhi_pd(r0,r1);      // m13     m03     m11*v1  m01*v1
  __m256d t1=_mm256_unpacklo_pd(r0,r1);      // m12*v2  m02*v2  m10*v0  m00*v0
  __m256d t2=_mm256_unpackhi_pd(r2,r3);      // m33     m23     m31*v1  m21*v1
  __m256d t3=_mm256_unpacklo_pd(r2,r3);      // m32*v2  m22*v2  m30*v0  m20*v0
  FXVec3d r;
  r0=_mm256_permute2f128_pd(t1,t3,0x20);    // m30*v0 m20*v0 m10*v0 m00*v0
  r1=_mm256_permute2f128_pd(t0,t2,0x20);    // m31*v1 m21*v1 m11*v1 m01*v1
  r2=_mm256_permute2f128_pd(t1,t3,0x31);    // m32*v2 m22*v2 m12*v2 m02*v2
  r3=_mm256_permute2f128_pd(t0,t2,0x31);    // m33    m23    m13    m03
  _mm256_maskstore_pd(&r[0],MMM,_mm256_add_pd(_mm256_add_pd(r0,r1),_mm256_add_pd(r2,r3)));
  return r;
#elif defined(FOX_HAS_SSE3)
  __m128d v0=_mm_loadu_pd(&v[0]);
  __m128d v1=_mm_load_sd(&v[2]);
  __m128d r00=_mm_mul_pd(_mm_loadu_pd(&m[0][0]),v0);   // m01*v1  m00*v0
  __m128d r02=_mm_mul_pd(_mm_loadu_pd(&m[0][2]),v1);   // m03     m02*v2
  __m128d r10=_mm_mul_pd(_mm_loadu_pd(&m[1][0]),v0);   // m11*v1  m10*v0
  __m128d r12=_mm_mul_pd(_mm_loadu_pd(&m[1][2]),v1);   // m13     m12*v2
  __m128d r20=_mm_mul_pd(_mm_loadu_pd(&m[2][0]),v0);   // m21*v1  m20*v0
  __m128d r22=_mm_mul_pd(_mm_loadu_pd(&m[2][2]),v1);   // m23     m22*v2
  FXVec3d r;
  r00=_mm_hadd_pd(r00,r02);     // m03+m02*v2  m01*v1+m00*v0
  r10=_mm_hadd_pd(r10,r12);     // m13+m12*v2  m11*v1+m10*v0
  r20=_mm_hadd_pd(r20,r22);     // m23+m22*v2  m21*v1+m20*v0
  r00=_mm_hadd_pd(r00,r10);     // m13+m12*v2+m11*v1+m10*v0  m03+m02*v2+m01*v1+m00*v0
  r20=_mm_hadd_pd(r20,r20);     // m23+m22*v2+m21*v1+m20*v0  m23+m22*v2+m21*v1+m20*v0
  _mm_storeu_pd(&r[0],r00);
  _mm_store_sd (&r[2],r20);
  return r;
#else
  return FXVec3d(m[0][0]*v[0]+m[0][1]*v[1]+m[0][2]*v[2]+m[0][3], m[1][0]*v[0]+m[1][1]*v[1]+m[1][2]*v[2]+m[1][3], m[2][0]*v[0]+m[2][1]*v[1]+m[2][2]*v[2]+m[2][3]);
#endif
  }


// Matrix times vector
FXVec4d operator*(const FXMat4d& m,const FXVec4d& v){
#if defined(FOX_HAS_AVX)
  __m256d vv=_mm256_loadu_pd(v);
  __m256d r0=_mm256_mul_pd(_mm256_loadu_pd(&m[0][0]),vv);      // m03*v3  m02*v2  m01*v1  m00*v0
  __m256d r1=_mm256_mul_pd(_mm256_loadu_pd(&m[1][0]),vv);      // m13*v3  m12*v2  m11*v1  m10*v0
  __m256d r2=_mm256_mul_pd(_mm256_loadu_pd(&m[2][0]),vv);      // m23*v3  m22*v2  m21*v1  m20*v0
  __m256d r3=_mm256_mul_pd(_mm256_loadu_pd(&m[3][0]),vv);      // m33*v3  m32*v2  m31*v1  m30*v0
  __m256d t0=_mm256_unpackhi_pd(r0,r1);      // m13*v3  m03*v3  m11*v1  m01*v1
  __m256d t1=_mm256_unpacklo_pd(r0,r1);      // m12*v2  m02*v2  m10*v0  m00*v0
  __m256d t2=_mm256_unpackhi_pd(r2,r3);      // m33*v3  m23*v3  m31*v1  m21*v1
  __m256d t3=_mm256_unpacklo_pd(r2,r3);      // m32*v2  m22*v2  m30*v0  m20*v0
  FXVec4d r;
  r0=_mm256_permute2f128_pd(t1,t3,0x20);    // m30*v0 m20*v0 m10*v0 m00*v0
  r1=_mm256_permute2f128_pd(t0,t2,0x20);    // m31*v1 m21*v1 m11*v1 m01*v1
  r2=_mm256_permute2f128_pd(t1,t3,0x31);    // m32*v2 m22*v2 m12*v2 m02*v2
  r3=_mm256_permute2f128_pd(t0,t2,0x31);    // m33*v3 m23*v3 m13*v3 m03*v3
  _mm256_storeu_pd(&r[0],_mm256_add_pd(_mm256_add_pd(r0,r1),_mm256_add_pd(r2,r3)));
  return r;
#elif defined(FOX_HAS_SSE3)
  __m128d v0=_mm_loadu_pd(&v[0]);
  __m128d v1=_mm_loadu_pd(&v[2]);
  __m128d r00=_mm_mul_pd(_mm_loadu_pd(&m[0][0]),v0);
  __m128d r02=_mm_mul_pd(_mm_loadu_pd(&m[0][2]),v1);
  __m128d r10=_mm_mul_pd(_mm_loadu_pd(&m[1][0]),v0);
  __m128d r12=_mm_mul_pd(_mm_loadu_pd(&m[1][2]),v1);
  __m128d r20=_mm_mul_pd(_mm_loadu_pd(&m[2][0]),v0);
  __m128d r22=_mm_mul_pd(_mm_loadu_pd(&m[2][2]),v1);
  __m128d r30=_mm_mul_pd(_mm_loadu_pd(&m[3][0]),v0);
  __m128d r32=_mm_mul_pd(_mm_loadu_pd(&m[3][2]),v1);
  FXVec4d r;
  r00=_mm_hadd_pd(r00,r02);     // m03*v3+m02*v2  m01*v1+m00*v0
  r10=_mm_hadd_pd(r10,r12);     // m13*v3+m12*v2  m11*v1+m10*v0
  r20=_mm_hadd_pd(r20,r22);     // m23*v3+m22*v2  m21*v1+m20*v0
  r30=_mm_hadd_pd(r30,r32);     // m33*v3+m32*v2  m31*v1+m30*v0
  r00=_mm_hadd_pd(r00,r10);     // (m13*v3+m12*v2)+(m11*v1+m10*v0) (m03*v3+m02*v2)+(m01*v1+m00*v0)
  r20=_mm_hadd_pd(r20,r30);     // (m33*v3+m32*v2)+(m31*v1+m30*v0) (m23*v3+m22*v2)+(m21*v1+m20*v0)
  _mm_storeu_pd(&r[0],r00);
  _mm_storeu_pd(&r[2],r20);
  return r;
#else
  return FXVec4d(m[0][0]*v[0]+m[0][1]*v[1]+m[0][2]*v[2]+m[0][3]*v[3], m[1][0]*v[0]+m[1][1]*v[1]+m[1][2]*v[2]+m[1][3]*v[3], m[2][0]*v[0]+m[2][1]*v[1]+m[2][2]*v[2]+m[2][3]*v[3], m[3][0]*v[0]+m[3][1]*v[1]+m[3][2]*v[2]+m[3][3]*v[3]);
#endif
  }


// Vector times matrix
FXVec3d operator*(const FXVec3d& v,const FXMat4d& m){
#if defined(FOX_HAS_AVX)
  __m256d m0=_mm256_loadu_pd(&m[0][0]);
  __m256d m1=_mm256_loadu_pd(&m[1][0]);
  __m256d m2=_mm256_loadu_pd(&m[2][0]);
  __m256d m3=_mm256_loadu_pd(&m[3][0]);
  __m256d v0=_mm256_set1_pd(v[0]);
  __m256d v1=_mm256_set1_pd(v[1]);
  __m256d v2=_mm256_set1_pd(v[2]);
  FXVec3d r;
  _mm256_maskstore_pd(&r[0],MMM,_mm256_add_pd(_mm256_add_pd(_mm256_mul_pd(v0,m0),_mm256_mul_pd(v1,m1)),_mm256_add_pd(_mm256_mul_pd(v2,m2),m3)));
  return r;
#elif defined(FOX_HAS_SSE2)
  __m128d m00=_mm_loadu_pd(&m[0][0]);
  __m128d m02=_mm_loadu_pd(&m[0][2]);
  __m128d m10=_mm_loadu_pd(&m[1][0]);
  __m128d m12=_mm_loadu_pd(&m[1][2]);
  __m128d m20=_mm_loadu_pd(&m[2][0]);
  __m128d m22=_mm_loadu_pd(&m[2][2]);
  __m128d m30=_mm_loadu_pd(&m[3][0]);
  __m128d m32=_mm_loadu_pd(&m[3][2]);
  __m128d v0=_mm_set1_pd(v[0]);
  __m128d v1=_mm_set1_pd(v[1]);
  __m128d v2=_mm_set1_pd(v[2]);
  FXVec3d r;
  _mm_storeu_pd(&r[0],_mm_add_pd(_mm_add_pd(_mm_mul_pd(m00,v0),_mm_mul_pd(m10,v1)),_mm_add_pd(_mm_mul_pd(m20,v2),m30)));
  _mm_store_sd(&r[2],_mm_add_sd(_mm_add_sd(_mm_mul_sd(m02,v0),_mm_mul_sd(m12,v1)),_mm_add_sd(_mm_mul_sd(m22,v2),m32)));
  return r;
#else
  return FXVec3d(v[0]*m[0][0]+v[1]*m[1][0]+v[2]*m[2][0]+m[3][0], v[0]*m[0][1]+v[1]*m[1][1]+v[2]*m[2][1]+m[3][1], v[0]*m[0][2]+v[1]*m[1][2]+v[2]*m[2][2]+m[3][2]);
#endif
  }


// Vector times matrix
FXVec4d operator*(const FXVec4d& v,const FXMat4d& m){
#if defined(FOX_HAS_AVX)
  __m256d m0=_mm256_loadu_pd(&m[0][0]);
  __m256d m1=_mm256_loadu_pd(&m[1][0]);
  __m256d m2=_mm256_loadu_pd(&m[2][0]);
  __m256d m3=_mm256_loadu_pd(&m[3][0]);
  __m256d v0=_mm256_set1_pd(v[0]);
  __m256d v1=_mm256_set1_pd(v[1]);
  __m256d v2=_mm256_set1_pd(v[2]);
  __m256d v3=_mm256_set1_pd(v[3]);
  FXVec4d r;
  _mm256_storeu_pd(&r[0],_mm256_add_pd(_mm256_add_pd(_mm256_mul_pd(v0,m0),_mm256_mul_pd(v1,m1)),_mm256_add_pd(_mm256_mul_pd(v2,m2),_mm256_mul_pd(v3,m3))));
  return r;
#elif defined(FOX_HAS_SSE2)
  __m128d m00=_mm_loadu_pd(&m[0][0]);
  __m128d m02=_mm_loadu_pd(&m[0][2]);
  __m128d m10=_mm_loadu_pd(&m[1][0]);
  __m128d m12=_mm_loadu_pd(&m[1][2]);
  __m128d m20=_mm_loadu_pd(&m[2][0]);
  __m128d m22=_mm_loadu_pd(&m[2][2]);
  __m128d m30=_mm_loadu_pd(&m[3][0]);
  __m128d m32=_mm_loadu_pd(&m[3][2]);
  __m128d v0=_mm_set1_pd(v[0]);
  __m128d v1=_mm_set1_pd(v[1]);
  __m128d v2=_mm_set1_pd(v[2]);
  __m128d v3=_mm_set1_pd(v[3]);
  FXVec4d r;
  _mm_storeu_pd(&r[0],_mm_add_pd(_mm_add_pd(_mm_mul_pd(m00,v0),_mm_mul_pd(m10,v1)),_mm_add_pd(_mm_mul_pd(m20,v2),_mm_mul_pd(m30,v3))));
  _mm_storeu_pd(&r[2],_mm_add_pd(_mm_add_pd(_mm_mul_pd(m02,v0),_mm_mul_pd(m12,v1)),_mm_add_pd(_mm_mul_pd(m22,v2),_mm_mul_pd(m32,v3))));
  return r;
#else
  return FXVec4d(v[0]*m[0][0]+v[1]*m[1][0]+v[2]*m[2][0]+v[3]*m[3][0], v[0]*m[0][1]+v[1]*m[1][1]+v[2]*m[2][1]+v[3]*m[3][1], v[0]*m[0][2]+v[1]*m[1][2]+v[2]*m[2][2]+v[3]*m[3][2], v[0]*m[0][3]+v[1]*m[1][3]+v[2]*m[2][3]+v[3]*m[3][3]);
#endif
  }


// Matrix and matrix add
FXMat4d operator+(const FXMat4d& a,const FXMat4d& b){
#if defined(FOX_HAS_AVX)
  FXMat4d r;
  _mm256_storeu_pd(&r[0][0],_mm256_add_pd(_mm256_loadu_pd(&a[0][0]),_mm256_loadu_pd(&b[0][0])));
  _mm256_storeu_pd(&r[1][0],_mm256_add_pd(_mm256_loadu_pd(&a[1][0]),_mm256_loadu_pd(&b[1][0])));
  _mm256_storeu_pd(&r[2][0],_mm256_add_pd(_mm256_loadu_pd(&a[2][0]),_mm256_loadu_pd(&b[2][0])));
  _mm256_storeu_pd(&r[3][0],_mm256_add_pd(_mm256_loadu_pd(&a[3][0]),_mm256_loadu_pd(&b[3][0])));
  return r;
#elif defined(FOX_HAS_SSE2)
  FXMat4d r;
  _mm_storeu_pd(&r[0][0],_mm_add_pd(_mm_loadu_pd(&a[0][0]),_mm_loadu_pd(&b[0][0])));
  _mm_storeu_pd(&r[0][2],_mm_add_pd(_mm_loadu_pd(&a[0][2]),_mm_loadu_pd(&b[0][2])));
  _mm_storeu_pd(&r[1][0],_mm_add_pd(_mm_loadu_pd(&a[1][0]),_mm_loadu_pd(&b[1][0])));
  _mm_storeu_pd(&r[1][2],_mm_add_pd(_mm_loadu_pd(&a[1][2]),_mm_loadu_pd(&b[1][2])));
  _mm_storeu_pd(&r[2][0],_mm_add_pd(_mm_loadu_pd(&a[2][0]),_mm_loadu_pd(&b[2][0])));
  _mm_storeu_pd(&r[2][2],_mm_add_pd(_mm_loadu_pd(&a[2][2]),_mm_loadu_pd(&b[2][2])));
  _mm_storeu_pd(&r[3][0],_mm_add_pd(_mm_loadu_pd(&a[3][0]),_mm_loadu_pd(&b[3][0])));
  _mm_storeu_pd(&r[3][2],_mm_add_pd(_mm_loadu_pd(&a[3][2]),_mm_loadu_pd(&b[3][2])));
  return r;
#else
  return FXMat4d(a[0][0]+b[0][0],a[0][1]+b[0][1],a[0][2]+b[0][2],a[0][3]+b[0][3],
                 a[1][0]+b[1][0],a[1][1]+b[1][1],a[1][2]+b[1][2],a[1][3]+b[1][3],
                 a[2][0]+b[2][0],a[2][1]+b[2][1],a[2][2]+b[2][2],a[2][3]+b[2][3],
                 a[3][0]+b[3][0],a[3][1]+b[3][1],a[3][2]+b[3][2],a[3][3]+b[3][3]);
#endif
  }


// Matrix and matrix subtract
FXMat4d operator-(const FXMat4d& a,const FXMat4d& b){
#if defined(FOX_HAS_AVX)
  FXMat4d r;
  _mm256_storeu_pd(&r[0][0],_mm256_sub_pd(_mm256_loadu_pd(&a[0][0]),_mm256_loadu_pd(&b[0][0])));
  _mm256_storeu_pd(&r[1][0],_mm256_sub_pd(_mm256_loadu_pd(&a[1][0]),_mm256_loadu_pd(&b[1][0])));
  _mm256_storeu_pd(&r[2][0],_mm256_sub_pd(_mm256_loadu_pd(&a[2][0]),_mm256_loadu_pd(&b[2][0])));
  _mm256_storeu_pd(&r[3][0],_mm256_sub_pd(_mm256_loadu_pd(&a[3][0]),_mm256_loadu_pd(&b[3][0])));
  return r;
#elif defined(FOX_HAS_SSE2)
  FXMat4d r;
  _mm_storeu_pd(&r[0][0],_mm_sub_pd(_mm_loadu_pd(&a[0][0]),_mm_loadu_pd(&b[0][0])));
  _mm_storeu_pd(&r[0][2],_mm_sub_pd(_mm_loadu_pd(&a[0][2]),_mm_loadu_pd(&b[0][2])));
  _mm_storeu_pd(&r[1][0],_mm_sub_pd(_mm_loadu_pd(&a[1][0]),_mm_loadu_pd(&b[1][0])));
  _mm_storeu_pd(&r[1][2],_mm_sub_pd(_mm_loadu_pd(&a[1][2]),_mm_loadu_pd(&b[1][2])));
  _mm_storeu_pd(&r[2][0],_mm_sub_pd(_mm_loadu_pd(&a[2][0]),_mm_loadu_pd(&b[2][0])));
  _mm_storeu_pd(&r[2][2],_mm_sub_pd(_mm_loadu_pd(&a[2][2]),_mm_loadu_pd(&b[2][2])));
  _mm_storeu_pd(&r[3][0],_mm_sub_pd(_mm_loadu_pd(&a[3][0]),_mm_loadu_pd(&b[3][0])));
  _mm_storeu_pd(&r[3][2],_mm_sub_pd(_mm_loadu_pd(&a[3][2]),_mm_loadu_pd(&b[3][2])));
  return r;
#else
  return FXMat4d(a[0][0]-b[0][0],a[0][1]-b[0][1],a[0][2]-b[0][2],a[0][3]-b[0][3],
                 a[1][0]-b[1][0],a[1][1]-b[1][1],a[1][2]-b[1][2],a[1][3]-b[1][3],
                 a[2][0]-b[2][0],a[2][1]-b[2][1],a[2][2]-b[2][2],a[2][3]-b[2][3],
                 a[3][0]-b[3][0],a[3][1]-b[3][1],a[3][2]-b[3][2],a[3][3]-b[3][3]);
#endif
  }


// Matrix and matrix multiply
FXMat4d operator*(const FXMat4d& a,const FXMat4d& b){
#if defined(FOX_HAS_AVX)
  __m256d b0=_mm256_loadu_pd(&b[0][0]);
  __m256d b1=_mm256_loadu_pd(&b[1][0]);
  __m256d b2=_mm256_loadu_pd(&b[2][0]);
  __m256d b3=_mm256_loadu_pd(&b[3][0]);
  __m256d xx,yy,zz,ww;
  FXMat4d r;
  xx=_mm256_set1_pd(a[0][0]);
  yy=_mm256_set1_pd(a[0][1]);
  zz=_mm256_set1_pd(a[0][2]);
  ww=_mm256_set1_pd(a[0][3]);
  _mm256_storeu_pd(r[0],_mm256_add_pd(_mm256_add_pd(_mm256_mul_pd(b0,xx),_mm256_mul_pd(b1,yy)),_mm256_add_pd(_mm256_mul_pd(b2,zz),_mm256_mul_pd(b3,ww))));
  xx=_mm256_set1_pd(a[1][0]);
  yy=_mm256_set1_pd(a[1][1]);
  zz=_mm256_set1_pd(a[1][2]);
  ww=_mm256_set1_pd(a[1][3]);
  _mm256_storeu_pd(r[1],_mm256_add_pd(_mm256_add_pd(_mm256_mul_pd(b0,xx),_mm256_mul_pd(b1,yy)),_mm256_add_pd(_mm256_mul_pd(b2,zz),_mm256_mul_pd(b3,ww))));
  xx=_mm256_set1_pd(a[2][0]);
  yy=_mm256_set1_pd(a[2][1]);
  zz=_mm256_set1_pd(a[2][2]);
  ww=_mm256_set1_pd(a[2][3]);
  _mm256_storeu_pd(r[2],_mm256_add_pd(_mm256_add_pd(_mm256_mul_pd(b0,xx),_mm256_mul_pd(b1,yy)),_mm256_add_pd(_mm256_mul_pd(b2,zz),_mm256_mul_pd(b3,ww))));
  xx=_mm256_set1_pd(a[3][0]);
  yy=_mm256_set1_pd(a[3][1]);
  zz=_mm256_set1_pd(a[3][2]);
  ww=_mm256_set1_pd(a[3][3]);
  _mm256_storeu_pd(r[3],_mm256_add_pd(_mm256_add_pd(_mm256_mul_pd(b0,xx),_mm256_mul_pd(b1,yy)),_mm256_add_pd(_mm256_mul_pd(b2,zz),_mm256_mul_pd(b3,ww))));
  return r;
#elif defined(FOX_HAS_SSE2)
  __m128d b00=_mm_loadu_pd(&b[0][0]);
  __m128d b02=_mm_loadu_pd(&b[0][2]);
  __m128d b10=_mm_loadu_pd(&b[1][0]);
  __m128d b12=_mm_loadu_pd(&b[1][2]);
  __m128d b20=_mm_loadu_pd(&b[2][0]);
  __m128d b22=_mm_loadu_pd(&b[2][2]);
  __m128d b30=_mm_loadu_pd(&b[3][0]);
  __m128d b32=_mm_loadu_pd(&b[3][2]);
  __m128d xx,yy,zz,ww;
  FXMat4d r;
  xx=_mm_set1_pd(a[0][0]);
  yy=_mm_set1_pd(a[0][1]);
  zz=_mm_set1_pd(a[0][2]);
  ww=_mm_set1_pd(a[0][3]);
  _mm_storeu_pd(&r[0][0],_mm_add_pd(_mm_add_pd(_mm_add_pd(_mm_mul_pd(b00,xx),_mm_mul_pd(b10,yy)),_mm_mul_pd(b20,zz)),_mm_mul_pd(b30,ww)));
  _mm_storeu_pd(&r[0][2],_mm_add_pd(_mm_add_pd(_mm_add_pd(_mm_mul_pd(b02,xx),_mm_mul_pd(b12,yy)),_mm_mul_pd(b22,zz)),_mm_mul_pd(b32,ww)));
  xx=_mm_set1_pd(a[1][0]);
  yy=_mm_set1_pd(a[1][1]);
  zz=_mm_set1_pd(a[1][2]);
  ww=_mm_set1_pd(a[1][3]);
  _mm_storeu_pd(&r[1][0],_mm_add_pd(_mm_add_pd(_mm_add_pd(_mm_mul_pd(b00,xx),_mm_mul_pd(b10,yy)),_mm_mul_pd(b20,zz)),_mm_mul_pd(b30,ww)));
  _mm_storeu_pd(&r[1][2],_mm_add_pd(_mm_add_pd(_mm_add_pd(_mm_mul_pd(b02,xx),_mm_mul_pd(b12,yy)),_mm_mul_pd(b22,zz)),_mm_mul_pd(b32,ww)));
  xx=_mm_set1_pd(a[2][0]);
  yy=_mm_set1_pd(a[2][1]);
  zz=_mm_set1_pd(a[2][2]);
  ww=_mm_set1_pd(a[2][3]);
  _mm_storeu_pd(&r[2][0],_mm_add_pd(_mm_add_pd(_mm_add_pd(_mm_mul_pd(b00,xx),_mm_mul_pd(b10,yy)),_mm_mul_pd(b20,zz)),_mm_mul_pd(b30,ww)));
  _mm_storeu_pd(&r[2][2],_mm_add_pd(_mm_add_pd(_mm_add_pd(_mm_mul_pd(b02,xx),_mm_mul_pd(b12,yy)),_mm_mul_pd(b22,zz)),_mm_mul_pd(b32,ww)));
  xx=_mm_set1_pd(a[3][0]);
  yy=_mm_set1_pd(a[3][1]);
  zz=_mm_set1_pd(a[3][2]);
  ww=_mm_set1_pd(a[3][3]);
  _mm_storeu_pd(&r[3][0],_mm_add_pd(_mm_add_pd(_mm_add_pd(_mm_mul_pd(b00,xx),_mm_mul_pd(b10,yy)),_mm_mul_pd(b20,zz)),_mm_mul_pd(b30,ww)));
  _mm_storeu_pd(&r[3][2],_mm_add_pd(_mm_add_pd(_mm_add_pd(_mm_mul_pd(b02,xx),_mm_mul_pd(b12,yy)),_mm_mul_pd(b22,zz)),_mm_mul_pd(b32,ww)));
  return r;
#else
  FXdouble x,y,z,w;
  FXMat4d r;
  x=a[0][0]; y=a[0][1]; z=a[0][2]; w=a[0][3];
  r[0][0]=x*b[0][0]+y*b[1][0]+z*b[2][0]+w*b[3][0];
  r[0][1]=x*b[0][1]+y*b[1][1]+z*b[2][1]+w*b[3][1];
  r[0][2]=x*b[0][2]+y*b[1][2]+z*b[2][2]+w*b[3][2];
  r[0][3]=x*b[0][3]+y*b[1][3]+z*b[2][3]+w*b[3][3];
  x=a[1][0]; y=a[1][1]; z=a[1][2]; w=a[1][3];
  r[1][0]=x*b[0][0]+y*b[1][0]+z*b[2][0]+w*b[3][0];
  r[1][1]=x*b[0][1]+y*b[1][1]+z*b[2][1]+w*b[3][1];
  r[1][2]=x*b[0][2]+y*b[1][2]+z*b[2][2]+w*b[3][2];
  r[1][3]=x*b[0][3]+y*b[1][3]+z*b[2][3]+w*b[3][3];
  x=a[2][0]; y=a[2][1]; z=a[2][2]; w=a[2][3];
  r[2][0]=x*b[0][0]+y*b[1][0]+z*b[2][0]+w*b[3][0];
  r[2][1]=x*b[0][1]+y*b[1][1]+z*b[2][1]+w*b[3][1];
  r[2][2]=x*b[0][2]+y*b[1][2]+z*b[2][2]+w*b[3][2];
  r[2][3]=x*b[0][3]+y*b[1][3]+z*b[2][3]+w*b[3][3];
  x=a[3][0]; y=a[3][1]; z=a[3][2]; w=a[3][3];
  r[3][0]=x*b[0][0]+y*b[1][0]+z*b[2][0]+w*b[3][0];
  r[3][1]=x*b[0][1]+y*b[1][1]+z*b[2][1]+w*b[3][1];
  r[3][2]=x*b[0][2]+y*b[1][2]+z*b[2][2]+w*b[3][2];
  r[3][3]=x*b[0][3]+y*b[1][3]+z*b[2][3]+w*b[3][3];
  return r;
#endif
  }


// Multiply scalar by matrix
FXMat4d operator*(FXdouble x,const FXMat4d& a){
#if defined(FOX_HAS_AVX)
  FXMat4d r;
  _mm256_storeu_pd(&r[0][0],_mm256_mul_pd(_mm256_set1_pd(x),_mm256_loadu_pd(&a[0][0])));
  _mm256_storeu_pd(&r[1][0],_mm256_mul_pd(_mm256_set1_pd(x),_mm256_loadu_pd(&a[1][0])));
  _mm256_storeu_pd(&r[2][0],_mm256_mul_pd(_mm256_set1_pd(x),_mm256_loadu_pd(&a[2][0])));
  _mm256_storeu_pd(&r[3][0],_mm256_mul_pd(_mm256_set1_pd(x),_mm256_loadu_pd(&a[3][0])));
  return r;
#elif defined(FOX_HAS_SSE2)
  FXMat4d r;
  _mm_storeu_pd(&r[0][0],_mm_mul_pd(_mm_set1_pd(x),_mm_loadu_pd(&a[0][0])));
  _mm_storeu_pd(&r[0][2],_mm_mul_pd(_mm_set1_pd(x),_mm_loadu_pd(&a[0][2])));
  _mm_storeu_pd(&r[1][0],_mm_mul_pd(_mm_set1_pd(x),_mm_loadu_pd(&a[1][0])));
  _mm_storeu_pd(&r[1][2],_mm_mul_pd(_mm_set1_pd(x),_mm_loadu_pd(&a[1][2])));
  _mm_storeu_pd(&r[2][0],_mm_mul_pd(_mm_set1_pd(x),_mm_loadu_pd(&a[2][0])));
  _mm_storeu_pd(&r[2][2],_mm_mul_pd(_mm_set1_pd(x),_mm_loadu_pd(&a[2][2])));
  _mm_storeu_pd(&r[3][0],_mm_mul_pd(_mm_set1_pd(x),_mm_loadu_pd(&a[3][0])));
  _mm_storeu_pd(&r[3][2],_mm_mul_pd(_mm_set1_pd(x),_mm_loadu_pd(&a[3][2])));
  return r;
#else
  return FXMat4d(x*a[0][0],x*a[0][1],x*a[0][2],a[0][3],
                 x*a[1][0],x*a[1][1],x*a[1][2],a[1][3],
                 x*a[2][0],x*a[2][1],x*a[2][2],a[2][3],
                 x*a[3][0],x*a[3][1],x*a[3][2],a[3][3]);
#endif
  }


// Multiply matrix by scalar
FXMat4d operator*(const FXMat4d& a,FXdouble x){
#if defined(FOX_HAS_AVX)
  FXMat4d r;
  _mm256_storeu_pd(&r[0][0],_mm256_mul_pd(_mm256_loadu_pd(&a[0][0]),_mm256_set1_pd(x)));
  _mm256_storeu_pd(&r[1][0],_mm256_mul_pd(_mm256_loadu_pd(&a[1][0]),_mm256_set1_pd(x)));
  _mm256_storeu_pd(&r[2][0],_mm256_mul_pd(_mm256_loadu_pd(&a[2][0]),_mm256_set1_pd(x)));
  _mm256_storeu_pd(&r[3][0],_mm256_mul_pd(_mm256_loadu_pd(&a[3][0]),_mm256_set1_pd(x)));
  return r;
#elif defined(FOX_HAS_SSE2)
  FXMat4d r;
  _mm_storeu_pd(&r[0][0],_mm_mul_pd(_mm_loadu_pd(&a[0][0]),_mm_set1_pd(x)));
  _mm_storeu_pd(&r[0][2],_mm_mul_pd(_mm_loadu_pd(&a[0][2]),_mm_set1_pd(x)));
  _mm_storeu_pd(&r[1][0],_mm_mul_pd(_mm_loadu_pd(&a[1][0]),_mm_set1_pd(x)));
  _mm_storeu_pd(&r[1][2],_mm_mul_pd(_mm_loadu_pd(&a[1][2]),_mm_set1_pd(x)));
  _mm_storeu_pd(&r[2][0],_mm_mul_pd(_mm_loadu_pd(&a[2][0]),_mm_set1_pd(x)));
  _mm_storeu_pd(&r[2][2],_mm_mul_pd(_mm_loadu_pd(&a[2][2]),_mm_set1_pd(x)));
  _mm_storeu_pd(&r[3][0],_mm_mul_pd(_mm_loadu_pd(&a[3][0]),_mm_set1_pd(x)));
  _mm_storeu_pd(&r[3][2],_mm_mul_pd(_mm_loadu_pd(&a[3][2]),_mm_set1_pd(x)));
  return r;
#else
  return FXMat4d(a[0][0]*x,a[0][1]*x,a[0][2]*x,a[0][3],
                 a[1][0]*x,a[1][1]*x,a[1][2]*x,a[1][3],
                 a[2][0]*x,a[2][1]*x,a[2][2]*x,a[2][3],
                 a[3][0]*x,a[3][1]*x,a[3][2]*x,a[3][3]);
#endif
  }


// Divide scalar by matrix
FXMat4d operator/(FXdouble x,const FXMat4d& a){
#if defined(FOX_HAS_AVX)
  FXMat4d r;
  _mm256_storeu_pd(&r[0][0],_mm256_div_pd(_mm256_set1_pd(x),_mm256_loadu_pd(&a[0][0])));
  _mm256_storeu_pd(&r[1][0],_mm256_div_pd(_mm256_set1_pd(x),_mm256_loadu_pd(&a[1][0])));
  _mm256_storeu_pd(&r[2][0],_mm256_div_pd(_mm256_set1_pd(x),_mm256_loadu_pd(&a[2][0])));
  _mm256_storeu_pd(&r[3][0],_mm256_div_pd(_mm256_set1_pd(x),_mm256_loadu_pd(&a[3][0])));
  return r;
#elif defined(FOX_HAS_SSE2)
  FXMat4d r;
  _mm_storeu_pd(&r[0][0],_mm_div_pd(_mm_set1_pd(x),_mm_loadu_pd(&a[0][0])));
  _mm_storeu_pd(&r[0][2],_mm_div_pd(_mm_set1_pd(x),_mm_loadu_pd(&a[0][2])));
  _mm_storeu_pd(&r[1][0],_mm_div_pd(_mm_set1_pd(x),_mm_loadu_pd(&a[1][0])));
  _mm_storeu_pd(&r[1][2],_mm_div_pd(_mm_set1_pd(x),_mm_loadu_pd(&a[1][2])));
  _mm_storeu_pd(&r[2][0],_mm_div_pd(_mm_set1_pd(x),_mm_loadu_pd(&a[2][0])));
  _mm_storeu_pd(&r[2][2],_mm_div_pd(_mm_set1_pd(x),_mm_loadu_pd(&a[2][2])));
  _mm_storeu_pd(&r[3][0],_mm_div_pd(_mm_set1_pd(x),_mm_loadu_pd(&a[3][0])));
  _mm_storeu_pd(&r[3][2],_mm_div_pd(_mm_set1_pd(x),_mm_loadu_pd(&a[3][2])));
  return r;
#else
  return FXMat4d(x/a[0][0],x/a[0][1],x/a[0][2],a[0][3],
                 x/a[1][0],x/a[1][1],x/a[1][2],a[1][3],
                 x/a[2][0],x/a[2][1],x/a[2][2],a[2][3],
                 x/a[3][0],x/a[3][1],x/a[3][2],a[3][3]);
#endif
  }


// Divide matrix by scalar
FXMat4d operator/(const FXMat4d& a,FXdouble x){
#if defined(FOX_HAS_AVX)
  FXMat4d r;
  _mm256_storeu_pd(&r[0][0],_mm256_div_pd(_mm256_loadu_pd(&a[0][0]),_mm256_set1_pd(x)));
  _mm256_storeu_pd(&r[1][0],_mm256_div_pd(_mm256_loadu_pd(&a[1][0]),_mm256_set1_pd(x)));
  _mm256_storeu_pd(&r[2][0],_mm256_div_pd(_mm256_loadu_pd(&a[2][0]),_mm256_set1_pd(x)));
  _mm256_storeu_pd(&r[3][0],_mm256_div_pd(_mm256_loadu_pd(&a[3][0]),_mm256_set1_pd(x)));
  return r;
#elif defined(FOX_HAS_SSE2)
  FXMat4d r;
  _mm_storeu_pd(&r[0][0],_mm_div_pd(_mm_loadu_pd(&a[0][0]),_mm_set1_pd(x)));
  _mm_storeu_pd(&r[0][2],_mm_div_pd(_mm_loadu_pd(&a[0][2]),_mm_set1_pd(x)));
  _mm_storeu_pd(&r[1][0],_mm_div_pd(_mm_loadu_pd(&a[1][0]),_mm_set1_pd(x)));
  _mm_storeu_pd(&r[1][2],_mm_div_pd(_mm_loadu_pd(&a[1][2]),_mm_set1_pd(x)));
  _mm_storeu_pd(&r[2][0],_mm_div_pd(_mm_loadu_pd(&a[2][0]),_mm_set1_pd(x)));
  _mm_storeu_pd(&r[2][2],_mm_div_pd(_mm_loadu_pd(&a[2][2]),_mm_set1_pd(x)));
  _mm_storeu_pd(&r[3][0],_mm_div_pd(_mm_loadu_pd(&a[3][0]),_mm_set1_pd(x)));
  _mm_storeu_pd(&r[3][2],_mm_div_pd(_mm_loadu_pd(&a[3][2]),_mm_set1_pd(x)));
  return r;
#else
  return FXMat4d(a[0][0]/x,a[0][1]/x,a[0][2]/x,a[0][3],
                 a[1][0]/x,a[1][1]/x,a[1][2]/x,a[1][3],
                 a[2][0]/x,a[2][1]/x,a[2][2]/x,a[2][3],
                 a[3][0]/x,a[3][1]/x,a[3][2]/x,a[3][3]);
#endif
  }


// Matrix and matrix equality
FXbool operator==(const FXMat4d& a,const FXMat4d& b){
  return a[0]==b[0] && a[1]==b[1] && a[2]==b[2] && a[3]==b[3];
  }


// Matrix and matrix inequality
FXbool operator!=(const FXMat4d& a,const FXMat4d& b){
  return a[0]!=b[0] || a[1]!=b[1] || a[2]!=b[2] || a[3]!=b[3];
  }


// Matrix and scalar equality
FXbool operator==(const FXMat4d& a,FXdouble n){
  return a[0]==n && a[1]==n && a[2]==n && a[3]==n;
  }


// Scalar and matrix equality
FXbool operator==(FXdouble n,const FXMat4d& a){
  return n==a[0] && n==a[1] && n==a[2] && n==a[3];
  }


// Matrix and scalar inequality
FXbool operator!=(const FXMat4d& a,FXdouble n){
  return a[0]!=n || a[1]!=n || a[2]!=n || a[3]!=n;
  }


// Scalar and matrix inequality
FXbool operator!=(FXdouble n,const FXMat4d& a){
  return n!=a[0] || n!=a[1] || n!=a[2] || n!=a[3];
  }


// Save to archive
FXStream& operator<<(FXStream& store,const FXMat4d& m){
  store << m[0] << m[1] << m[2] << m[3];
  return store;
  }


// Load from archive
FXStream& operator>>(FXStream& store,FXMat4d& m){
  store >> m[0] >> m[1] >> m[2] >> m[3];
  return store;
  }

}
