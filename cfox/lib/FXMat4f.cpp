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
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "fxmath.h"
#include "FXArray.h"
#include "FXHash.h"
#include "FXStream.h"
#include "FXObject.h"
#include "FXVec2f.h"
#include "FXVec3f.h"
#include "FXVec4f.h"
#include "FXQuatf.h"
#include "FXMat3f.h"
#include "FXMat4f.h"


/*
  Notes:
  - Transformations pre-multiply.
  - Goal is same effect as OpenGL.
  - Some operations assume last column is (0,0,0,1).
*/


using namespace FX;

/*******************************************************************************/

namespace FX {


// Initialize matrix from scalar
FXMat4f::FXMat4f(FXfloat s){
#if defined(FOX_HAS_SSE)
  _mm_storeu_ps(&m[0][0],_mm_set1_ps(s));
  _mm_storeu_ps(&m[1][0],_mm_set1_ps(s));
  _mm_storeu_ps(&m[2][0],_mm_set1_ps(s));
  _mm_storeu_ps(&m[3][0],_mm_set1_ps(s));
#else
  m[0][0]=s; m[0][1]=s; m[0][2]=s; m[0][3]=s;
  m[1][0]=s; m[1][1]=s; m[1][2]=s; m[1][3]=s;
  m[2][0]=s; m[2][1]=s; m[2][2]=s; m[2][3]=s;
  m[3][0]=s; m[3][1]=s; m[3][2]=s; m[3][3]=s;
#endif
  }


// Initialize with 3x3 rotation and scaling matrix
FXMat4f::FXMat4f(const FXMat3f& s){
#if defined(FOX_HAS_SSE)
  _mm_storeu_ps(&m[0][0],_mm_set_ps(0.0f,s[0][2],s[0][1],s[0][0]));
  _mm_storeu_ps(&m[1][0],_mm_set_ps(0.0f,s[1][2],s[1][1],s[1][0]));
  _mm_storeu_ps(&m[2][0],_mm_set_ps(0.0f,s[2][2],s[2][1],s[2][0]));
  _mm_storeu_ps(&m[3][0],_mm_set_ps(1.0f,0.0f,0.0f,0.0f));
#else
  m[0][0]=s[0][0]; m[0][1]=s[0][1]; m[0][2]=s[0][2]; m[0][3]=0.0f;
  m[1][0]=s[1][0]; m[1][1]=s[1][1]; m[1][2]=s[1][2]; m[1][3]=0.0f;
  m[2][0]=s[2][0]; m[2][1]=s[2][1]; m[2][2]=s[2][2]; m[2][3]=0.0f;
  m[3][0]=0.0f;    m[3][1]=0.0f;    m[3][2]=0.0f;    m[3][3]=1.0f;
#endif
  }


// Initialize matrix from another matrix
FXMat4f::FXMat4f(const FXMat4f& s){
#if defined(FOX_HAS_SSE)
  _mm_storeu_ps(&m[0][0],_mm_loadu_ps(&s[0][0]));
  _mm_storeu_ps(&m[1][0],_mm_loadu_ps(&s[1][0]));
  _mm_storeu_ps(&m[2][0],_mm_loadu_ps(&s[2][0]));
  _mm_storeu_ps(&m[3][0],_mm_loadu_ps(&s[3][0]));
#else
  m[0]=s[0];
  m[1]=s[1];
  m[2]=s[2];
  m[3]=s[3];
#endif
  }


// Initialize matrix from array
FXMat4f::FXMat4f(const FXfloat s[]){
#if defined(FOX_HAS_SSE)
  _mm_storeu_ps(&m[0][0],_mm_loadu_ps(s+0));
  _mm_storeu_ps(&m[1][0],_mm_loadu_ps(s+4));
  _mm_storeu_ps(&m[2][0],_mm_loadu_ps(s+8));
  _mm_storeu_ps(&m[3][0],_mm_loadu_ps(s+12));
#else
  m[0][0]=s[0];  m[0][1]=s[1];  m[0][2]=s[2];  m[0][3]=s[3];
  m[1][0]=s[4];  m[1][1]=s[5];  m[1][2]=s[6];  m[1][3]=s[7];
  m[2][0]=s[8];  m[2][1]=s[9];  m[2][2]=s[10]; m[2][3]=s[11];
  m[3][0]=s[12]; m[3][1]=s[13]; m[3][2]=s[14]; m[3][3]=s[15];
#endif
  }


// Initialize diagonal matrix
FXMat4f::FXMat4f(FXfloat a,FXfloat b,FXfloat c,FXfloat d){
#if defined(FOX_HAS_SSE)
  _mm_storeu_ps(&m[0][0],_mm_set_ps(0.0f,0.0f,0.0f,a));
  _mm_storeu_ps(&m[1][0],_mm_set_ps(0.0f,0.0f,b,0.0f));
  _mm_storeu_ps(&m[2][0],_mm_set_ps(0.0f,c,0.0f,0.0f));
  _mm_storeu_ps(&m[3][0],_mm_set_ps(d,0.0f,0.0f,0.0f));
#else
  m[0][0]=a;    m[0][1]=0.0f; m[0][2]=0.0f; m[0][3]=0.0f;
  m[1][0]=0.0f; m[1][1]=b;    m[1][2]=0.0f; m[1][3]=0.0f;
  m[2][0]=0.0f; m[2][1]=0.0f; m[2][2]=c;    m[2][3]=0.0f;
  m[3][0]=0.0f; m[3][1]=0.0f; m[3][2]=0.0f; m[3][3]=d;
#endif
  }


// Initialize matrix from components
FXMat4f::FXMat4f(FXfloat a00,FXfloat a01,FXfloat a02,FXfloat a03,FXfloat a10,FXfloat a11,FXfloat a12,FXfloat a13,FXfloat a20,FXfloat a21,FXfloat a22,FXfloat a23,FXfloat a30,FXfloat a31,FXfloat a32,FXfloat a33){
  m[0][0]=a00; m[0][1]=a01; m[0][2]=a02; m[0][3]=a03;
  m[1][0]=a10; m[1][1]=a11; m[1][2]=a12; m[1][3]=a13;
  m[2][0]=a20; m[2][1]=a21; m[2][2]=a22; m[2][3]=a23;
  m[3][0]=a30; m[3][1]=a31; m[3][2]=a32; m[3][3]=a33;
  }


// Initialize matrix from four vectors
FXMat4f::FXMat4f(const FXVec4f& a,const FXVec4f& b,const FXVec4f& c,const FXVec4f& d){
#if defined(FOX_HAS_SSE)
  _mm_storeu_ps(&m[0][0],_mm_loadu_ps(a));
  _mm_storeu_ps(&m[1][0],_mm_loadu_ps(b));
  _mm_storeu_ps(&m[2][0],_mm_loadu_ps(c));
  _mm_storeu_ps(&m[3][0],_mm_loadu_ps(d));
#else
  m[0][0]=a[0]; m[0][1]=a[1]; m[0][2]=a[2]; m[0][3]=a[3];
  m[1][0]=b[0]; m[1][1]=b[1]; m[1][2]=b[2]; m[1][3]=b[3];
  m[2][0]=c[0]; m[2][1]=c[1]; m[2][2]=c[2]; m[2][3]=c[3];
  m[3][0]=d[0]; m[3][1]=d[1]; m[3][2]=d[2]; m[3][3]=d[3];
#endif
  }


// Initialize matrix from quaternion
FXMat4f::FXMat4f(const FXQuatf& quat){
  set(FXMat3f(quat));
  }


// Assign from scalar
FXMat4f& FXMat4f::operator=(FXfloat s){
#if defined(FOX_HAS_SSE)
  _mm_storeu_ps(&m[0][0],_mm_set1_ps(s));
  _mm_storeu_ps(&m[1][0],_mm_set1_ps(s));
  _mm_storeu_ps(&m[2][0],_mm_set1_ps(s));
  _mm_storeu_ps(&m[3][0],_mm_set1_ps(s));
#else
  m[0][0]=s; m[0][1]=s; m[0][2]=s; m[0][3]=s;
  m[1][0]=s; m[1][1]=s; m[1][2]=s; m[1][3]=s;
  m[2][0]=s; m[2][1]=s; m[2][2]=s; m[2][3]=s;
  m[3][0]=s; m[3][1]=s; m[3][2]=s; m[3][3]=s;
#endif
  return *this;
  }


// Assign from 3x3 rotation and scaling matrix
FXMat4f& FXMat4f::operator=(const FXMat3f& s){
#if defined(FOX_HAS_SSE)
  _mm_storeu_ps(&m[0][0],_mm_set_ps(0.0f,s[0][2],s[0][1],s[0][0]));
  _mm_storeu_ps(&m[1][0],_mm_set_ps(0.0f,s[1][2],s[1][1],s[1][0]));
  _mm_storeu_ps(&m[2][0],_mm_set_ps(0.0f,s[2][2],s[2][1],s[2][0]));
  _mm_storeu_ps(&m[3][0],_mm_set_ps(1.0f,0.0f,0.0f,0.0f));
#else
  m[0][0]=s[0][0]; m[0][1]=s[0][1]; m[0][2]=s[0][2]; m[0][3]=0.0f;
  m[1][0]=s[1][0]; m[1][1]=s[1][1]; m[1][2]=s[1][2]; m[1][3]=0.0f;
  m[2][0]=s[2][0]; m[2][1]=s[2][1]; m[2][2]=s[2][2]; m[2][3]=0.0f;
  m[3][0]=0.0f;    m[3][1]=0.0f;    m[3][2]=0.0f;    m[3][3]=1.0f;
#endif
  return *this;
  }


// Assignment operator
FXMat4f& FXMat4f::operator=(const FXMat4f& s){
#if defined(FOX_HAS_SSE)
  _mm_storeu_ps(&m[0][0],_mm_loadu_ps(&s[0][0]));
  _mm_storeu_ps(&m[1][0],_mm_loadu_ps(&s[1][0]));
  _mm_storeu_ps(&m[2][0],_mm_loadu_ps(&s[2][0]));
  _mm_storeu_ps(&m[3][0],_mm_loadu_ps(&s[3][0]));
#else
  m[0]=s[0];
  m[1]=s[1];
  m[2]=s[2];
  m[3]=s[3];
#endif
  return *this;
  }


// Assignment from quaternion
FXMat4f& FXMat4f::operator=(const FXQuatf& quat){
  return set(FXMat3f(quat));
  }


// Assignment from array
FXMat4f& FXMat4f::operator=(const FXfloat s[]){
#if defined(FOX_HAS_SSE)
  _mm_storeu_ps(&m[0][0],_mm_loadu_ps(s+0));
  _mm_storeu_ps(&m[1][0],_mm_loadu_ps(s+4));
  _mm_storeu_ps(&m[2][0],_mm_loadu_ps(s+8));
  _mm_storeu_ps(&m[3][0],_mm_loadu_ps(s+12));
#else
  m[0][0]=s[0];  m[0][1]=s[1];  m[0][2]=s[2];  m[0][3]=s[3];
  m[1][0]=s[4];  m[1][1]=s[5];  m[1][2]=s[6];  m[1][3]=s[7];
  m[2][0]=s[8];  m[2][1]=s[9];  m[2][2]=s[10]; m[2][3]=s[11];
  m[3][0]=s[12]; m[3][1]=s[13]; m[3][2]=s[14]; m[3][3]=s[15];
#endif
  return *this;
  }


// Set value from scalar
FXMat4f& FXMat4f::set(FXfloat s){
#if defined(FOX_HAS_SSE)
  _mm_storeu_ps(&m[0][0],_mm_set1_ps(s));
  _mm_storeu_ps(&m[1][0],_mm_set1_ps(s));
  _mm_storeu_ps(&m[2][0],_mm_set1_ps(s));
  _mm_storeu_ps(&m[3][0],_mm_set1_ps(s));
#else
  m[0][0]=s; m[0][1]=s; m[0][2]=s; m[0][3]=s;
  m[1][0]=s; m[1][1]=s; m[1][2]=s; m[1][3]=s;
  m[2][0]=s; m[2][1]=s; m[2][2]=s; m[2][3]=s;
  m[3][0]=s; m[3][1]=s; m[3][2]=s; m[3][3]=s;
#endif
  return *this;
  }


// Set from 3x3 rotation and scaling matrix
FXMat4f& FXMat4f::set(const FXMat3f& s){
#if defined(FOX_HAS_SSE)
  _mm_storeu_ps(&m[0][0],_mm_set_ps(0.0f,s[0][2],s[0][1],s[0][0]));
  _mm_storeu_ps(&m[1][0],_mm_set_ps(0.0f,s[1][2],s[1][1],s[1][0]));
  _mm_storeu_ps(&m[2][0],_mm_set_ps(0.0f,s[2][2],s[2][1],s[2][0]));
  _mm_storeu_ps(&m[3][0],_mm_set_ps(1.0f,0.0f,0.0f,0.0f));
#else
  m[0][0]=s[0][0]; m[0][1]=s[0][1]; m[0][2]=s[0][2]; m[0][3]=0.0f;
  m[1][0]=s[1][0]; m[1][1]=s[1][1]; m[1][2]=s[1][2]; m[1][3]=0.0f;
  m[2][0]=s[2][0]; m[2][1]=s[2][1]; m[2][2]=s[2][2]; m[2][3]=0.0f;
  m[3][0]=0.0f;    m[3][1]=0.0f;    m[3][2]=0.0f;    m[3][3]=1.0f;
#endif
  return *this;
  }


// Set value from another matrix
FXMat4f& FXMat4f::set(const FXMat4f& s){
#if defined(FOX_HAS_SSE)
  _mm_storeu_ps(&m[0][0],_mm_loadu_ps(&s[0][0]));
  _mm_storeu_ps(&m[1][0],_mm_loadu_ps(&s[1][0]));
  _mm_storeu_ps(&m[2][0],_mm_loadu_ps(&s[2][0]));
  _mm_storeu_ps(&m[3][0],_mm_loadu_ps(&s[3][0]));
#else
  m[0]=s[0];
  m[1]=s[1];
  m[2]=s[2];
  m[3]=s[3];
#endif
  return *this;
  }


// Set value from array
FXMat4f& FXMat4f::set(const FXfloat s[]){
#if defined(FOX_HAS_SSE)
  _mm_storeu_ps(&m[0][0],_mm_loadu_ps(s+0));
  _mm_storeu_ps(&m[1][0],_mm_loadu_ps(s+4));
  _mm_storeu_ps(&m[2][0],_mm_loadu_ps(s+8));
  _mm_storeu_ps(&m[3][0],_mm_loadu_ps(s+12));
#else
  m[0][0]=s[0];  m[0][1]=s[1];  m[0][2]=s[2];  m[0][3]=s[3];
  m[1][0]=s[4];  m[1][1]=s[5];  m[1][2]=s[6];  m[1][3]=s[7];
  m[2][0]=s[8];  m[2][1]=s[9];  m[2][2]=s[10]; m[2][3]=s[11];
  m[3][0]=s[12]; m[3][1]=s[13]; m[3][2]=s[14]; m[3][3]=s[15];
#endif
  return *this;
  }


// Set diagonal matrix
FXMat4f& FXMat4f::set(FXfloat a,FXfloat b,FXfloat c,FXfloat d){
#if defined(FOX_HAS_SSE)
  _mm_storeu_ps(&m[0][0],_mm_set_ps(0.0f,0.0f,0.0f,a));
  _mm_storeu_ps(&m[1][0],_mm_set_ps(0.0f,0.0f,b,0.0f));
  _mm_storeu_ps(&m[2][0],_mm_set_ps(0.0f,c,0.0f,0.0f));
  _mm_storeu_ps(&m[3][0],_mm_set_ps(d,0.0f,0.0f,0.0f));
#else
  m[0][0]=a;    m[0][1]=0.0f; m[0][2]=0.0f; m[0][3]=0.0f;
  m[1][0]=0.0f; m[1][1]=b;    m[1][2]=0.0f; m[1][3]=0.0f;
  m[2][0]=0.0f; m[2][1]=0.0f; m[2][2]=c;    m[2][3]=0.0f;
  m[3][0]=0.0f; m[3][1]=0.0f; m[3][2]=0.0f; m[3][3]=d;
#endif
  return *this;
  }


// Set value from components
FXMat4f& FXMat4f::set(FXfloat a00,FXfloat a01,FXfloat a02,FXfloat a03,FXfloat a10,FXfloat a11,FXfloat a12,FXfloat a13,FXfloat a20,FXfloat a21,FXfloat a22,FXfloat a23,FXfloat a30,FXfloat a31,FXfloat a32,FXfloat a33){
  m[0][0]=a00; m[0][1]=a01; m[0][2]=a02; m[0][3]=a03;
  m[1][0]=a10; m[1][1]=a11; m[1][2]=a12; m[1][3]=a13;
  m[2][0]=a20; m[2][1]=a21; m[2][2]=a22; m[2][3]=a23;
  m[3][0]=a30; m[3][1]=a31; m[3][2]=a32; m[3][3]=a33;
  return *this;
  }


// Set value from four vectors
FXMat4f& FXMat4f::set(const FXVec4f& a,const FXVec4f& b,const FXVec4f& c,const FXVec4f& d){
#if defined(FOX_HAS_SSE)
  _mm_storeu_ps(&m[0][0],_mm_loadu_ps(a));
  _mm_storeu_ps(&m[1][0],_mm_loadu_ps(b));
  _mm_storeu_ps(&m[2][0],_mm_loadu_ps(c));
  _mm_storeu_ps(&m[3][0],_mm_loadu_ps(d));
#else
  m[0][0]=a[0]; m[0][1]=a[1]; m[0][2]=a[2]; m[0][3]=a[3];
  m[1][0]=b[0]; m[1][1]=b[1]; m[1][2]=b[2]; m[1][3]=b[3];
  m[2][0]=c[0]; m[2][1]=c[1]; m[2][2]=c[2]; m[2][3]=c[3];
  m[3][0]=d[0]; m[3][1]=d[1]; m[3][2]=d[2]; m[3][3]=d[3];
#endif
  return *this;
  }


// Set value from quaternion
FXMat4f& FXMat4f::set(const FXQuatf& quat){
  return set(FXMat3f(quat));
  }


// Add matrices
FXMat4f& FXMat4f::operator+=(const FXMat4f& s){
#if defined(FOX_HAS_SSE)
  _mm_storeu_ps(&m[0][0],_mm_add_ps(_mm_loadu_ps(&m[0][0]),_mm_loadu_ps(&s[0][0])));
  _mm_storeu_ps(&m[1][0],_mm_add_ps(_mm_loadu_ps(&m[1][0]),_mm_loadu_ps(&s[1][0])));
  _mm_storeu_ps(&m[2][0],_mm_add_ps(_mm_loadu_ps(&m[2][0]),_mm_loadu_ps(&s[2][0])));
  _mm_storeu_ps(&m[3][0],_mm_add_ps(_mm_loadu_ps(&m[3][0]),_mm_loadu_ps(&s[3][0])));
#else
  m[0][0]+=s[0][0]; m[0][1]+=s[0][1]; m[0][2]+=s[0][2]; m[0][3]+=s[0][3];
  m[1][0]+=s[1][0]; m[1][1]+=s[1][1]; m[1][2]+=s[1][2]; m[1][3]+=s[1][3];
  m[2][0]+=s[2][0]; m[2][1]+=s[2][1]; m[2][2]+=s[2][2]; m[2][3]+=s[2][3];
  m[3][0]+=s[3][0]; m[3][1]+=s[3][1]; m[3][2]+=s[3][2]; m[3][3]+=s[3][3];
#endif
  return *this;
  }


// Subtract matrices
FXMat4f& FXMat4f::operator-=(const FXMat4f& s){
#if defined(FOX_HAS_SSE)
  _mm_storeu_ps(&m[0][0],_mm_sub_ps(_mm_loadu_ps(&m[0][0]),_mm_loadu_ps(&s[0][0])));
  _mm_storeu_ps(&m[1][0],_mm_sub_ps(_mm_loadu_ps(&m[1][0]),_mm_loadu_ps(&s[1][0])));
  _mm_storeu_ps(&m[2][0],_mm_sub_ps(_mm_loadu_ps(&m[2][0]),_mm_loadu_ps(&s[2][0])));
  _mm_storeu_ps(&m[3][0],_mm_sub_ps(_mm_loadu_ps(&m[3][0]),_mm_loadu_ps(&s[3][0])));
#else
  m[0][0]-=s[0][0]; m[0][1]-=s[0][1]; m[0][2]-=s[0][2]; m[0][3]-=s[0][3];
  m[1][0]-=s[1][0]; m[1][1]-=s[1][1]; m[1][2]-=s[1][2]; m[1][3]-=s[1][3];
  m[2][0]-=s[2][0]; m[2][1]-=s[2][1]; m[2][2]-=s[2][2]; m[2][3]-=s[2][3];
  m[3][0]-=s[3][0]; m[3][1]-=s[3][1]; m[3][2]-=s[3][2]; m[3][3]-=s[3][3];
#endif
  return *this;
  }


// Multiply matrix by scalar
FXMat4f& FXMat4f::operator*=(FXfloat s){
#if defined(FOX_HAS_SSE)
  _mm_storeu_ps(&m[0][0],_mm_mul_ps(_mm_loadu_ps(&m[0][0]),_mm_set1_ps(s)));
  _mm_storeu_ps(&m[1][0],_mm_mul_ps(_mm_loadu_ps(&m[1][0]),_mm_set1_ps(s)));
  _mm_storeu_ps(&m[2][0],_mm_mul_ps(_mm_loadu_ps(&m[2][0]),_mm_set1_ps(s)));
  _mm_storeu_ps(&m[3][0],_mm_mul_ps(_mm_loadu_ps(&m[3][0]),_mm_set1_ps(s)));
#else
  m[0][0]*=s; m[0][1]*=s; m[0][2]*=s; m[0][3]*=s;
  m[1][0]*=s; m[1][1]*=s; m[1][2]*=s; m[2][3]*=s;
  m[2][0]*=s; m[2][1]*=s; m[2][2]*=s; m[3][3]*=s;
  m[3][0]*=s; m[3][1]*=s; m[3][2]*=s; m[3][3]*=s;
#endif
  return *this;
  }


// Multiply matrix by matrix
FXMat4f& FXMat4f::operator*=(const FXMat4f& s){
#if defined(FOX_HAS_SSE)
  __m128 b0=_mm_loadu_ps(&s[0][0]);
  __m128 b1=_mm_loadu_ps(&s[1][0]);
  __m128 b2=_mm_loadu_ps(&s[2][0]);
  __m128 b3=_mm_loadu_ps(&s[3][0]);
  __m128 xx,yy,zz,ww;
  xx=_mm_set1_ps(m[0][0]);
  yy=_mm_set1_ps(m[0][1]);
  zz=_mm_set1_ps(m[0][2]);
  ww=_mm_set1_ps(m[0][3]);
  _mm_storeu_ps(m[0],_mm_add_ps(_mm_add_ps(_mm_mul_ps(b0,xx),_mm_mul_ps(b1,yy)),_mm_add_ps(_mm_mul_ps(b2,zz),_mm_mul_ps(b3,ww))));
  xx=_mm_set1_ps(m[1][0]);
  yy=_mm_set1_ps(m[1][1]);
  zz=_mm_set1_ps(m[1][2]);
  ww=_mm_set1_ps(m[1][3]);
  _mm_storeu_ps(m[1],_mm_add_ps(_mm_add_ps(_mm_mul_ps(b0,xx),_mm_mul_ps(b1,yy)),_mm_add_ps(_mm_mul_ps(b2,zz),_mm_mul_ps(b3,ww))));
  xx=_mm_set1_ps(m[2][0]);
  yy=_mm_set1_ps(m[2][1]);
  zz=_mm_set1_ps(m[2][2]);
  ww=_mm_set1_ps(m[2][3]);
  _mm_storeu_ps(m[2],_mm_add_ps(_mm_add_ps(_mm_mul_ps(b0,xx),_mm_mul_ps(b1,yy)),_mm_add_ps(_mm_mul_ps(b2,zz),_mm_mul_ps(b3,ww))));
  xx=_mm_set1_ps(m[3][0]);
  yy=_mm_set1_ps(m[3][1]);
  zz=_mm_set1_ps(m[3][2]);
  ww=_mm_set1_ps(m[3][3]);
  _mm_storeu_ps(m[3],_mm_add_ps(_mm_add_ps(_mm_mul_ps(b0,xx),_mm_mul_ps(b1,yy)),_mm_add_ps(_mm_mul_ps(b2,zz),_mm_mul_ps(b3,ww))));
#else
  FXfloat x,y,z,w;
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


// Divide matric by scalar
FXMat4f& FXMat4f::operator/=(FXfloat s){
#if defined(FOX_HAS_SSE)
  _mm_storeu_ps(&m[0][0],_mm_div_ps(_mm_loadu_ps(&m[0][0]),_mm_set1_ps(s)));
  _mm_storeu_ps(&m[1][0],_mm_div_ps(_mm_loadu_ps(&m[1][0]),_mm_set1_ps(s)));
  _mm_storeu_ps(&m[2][0],_mm_div_ps(_mm_loadu_ps(&m[2][0]),_mm_set1_ps(s)));
  _mm_storeu_ps(&m[3][0],_mm_div_ps(_mm_loadu_ps(&m[3][0]),_mm_set1_ps(s)));
#else
  m[0][0]/=s; m[0][1]/=s; m[0][2]/=s; m[0][3]/=s;
  m[1][0]/=s; m[1][1]/=s; m[1][2]/=s; m[1][3]/=s;
  m[2][0]/=s; m[2][1]/=s; m[2][2]/=s; m[2][3]/=s;
  m[3][0]/=s; m[3][1]/=s; m[3][2]/=s; m[3][3]/=s;
#endif
  return *this;
  }


// Unary minus
FXMat4f FXMat4f::operator-() const {
#if defined(FOX_HAS_SSE)
  FXMat4f r;
  _mm_storeu_ps(r[0],_mm_sub_ps(_mm_set_ps(0.0f,0.0f,0.0f,0.0f),_mm_loadu_ps(m[0])));
  _mm_storeu_ps(r[1],_mm_sub_ps(_mm_set_ps(0.0f,0.0f,0.0f,0.0f),_mm_loadu_ps(m[1])));
  _mm_storeu_ps(r[2],_mm_sub_ps(_mm_set_ps(0.0f,0.0f,0.0f,0.0f),_mm_loadu_ps(m[2])));
  _mm_storeu_ps(r[3],_mm_sub_ps(_mm_set_ps(0.0f,0.0f,0.0f,0.0f),_mm_loadu_ps(m[3])));
  return r;
#else
  return FXMat4f(-m[0][0],-m[0][1],-m[0][2],-m[0][3],
                 -m[1][0],-m[1][1],-m[1][2],-m[1][3],
                 -m[2][0],-m[2][1],-m[2][2],-m[2][3],
                 -m[3][0],-m[3][1],-m[3][2],-m[3][3]);
#endif
  }


// Set to identity matrix
FXMat4f& FXMat4f::identity(){
#if defined(FOX_HAS_SSE)
  _mm_storeu_ps(&m[0][0],_mm_set_ps(0.0f,0.0f,0.0f,1.0f));
  _mm_storeu_ps(&m[1][0],_mm_set_ps(0.0f,0.0f,1.0f,0.0f));
  _mm_storeu_ps(&m[2][0],_mm_set_ps(0.0f,1.0f,0.0f,0.0f));
  _mm_storeu_ps(&m[3][0],_mm_set_ps(1.0f,0.0f,0.0f,0.0f));
#else
  m[0][0]=1.0f; m[0][1]=0.0f; m[0][2]=0.0f; m[0][3]=0.0f;
  m[1][0]=0.0f; m[1][1]=1.0f; m[1][2]=0.0f; m[1][3]=0.0f;
  m[2][0]=0.0f; m[2][1]=0.0f; m[2][2]=1.0f; m[2][3]=0.0f;
  m[3][0]=0.0f; m[3][1]=0.0f; m[3][2]=0.0f; m[3][3]=1.0f;
#endif
  return *this;
  }


// Return true if identity matrix
FXbool FXMat4f::isIdentity() const {
  return m[0][0]==1.0f && m[0][1]==0.0f && m[0][2]==0.0f && m[0][3]==0.0f &&
         m[1][0]==0.0f && m[1][1]==1.0f && m[1][2]==0.0f && m[1][3]==0.0f &&
         m[2][0]==0.0f && m[2][1]==0.0f && m[2][2]==1.0f && m[2][3]==0.0f &&
         m[3][0]==0.0f && m[3][1]==0.0f && m[3][2]==0.0f && m[3][3]==1.0f;
  }


// Set orthographic projection from view volume
FXMat4f& FXMat4f::setOrtho(FXfloat xlo,FXfloat xhi,FXfloat ylo,FXfloat yhi,FXfloat zlo,FXfloat zhi){
  FXfloat rl=xhi-xlo;
  FXfloat tb=yhi-ylo;
  FXfloat yh=zhi-zlo;
  return set(2.0f/rl,0.0f,0.0f,0.0f,0.0f,2.0f/tb,0.0f,0.0f,0.0f,0.0f,-2.0f/yh,0.0f,-(xhi+xlo)/rl,-(yhi+ylo)/tb,-(zhi+zlo)/yh,1.0f);
  }


// Get view volume from orthographic projection
void FXMat4f::getOrtho(FXfloat& xlo,FXfloat& xhi,FXfloat& ylo,FXfloat& yhi,FXfloat& zlo,FXfloat& zhi) const {
  zlo= (m[3][2]+1.0f)/m[2][2];
  zhi= (m[3][2]-1.0f)/m[2][2];
  xlo=-(1.0f+m[3][0])/m[0][0];
  xhi= (1.0f-m[3][0])/m[0][0];
  ylo=-(1.0f+m[3][1])/m[1][1];
  yhi= (1.0f-m[3][1])/m[1][1];
  }


// Set to inverse orthographic projection
FXMat4f& FXMat4f::setInverseOrtho(FXfloat xlo,FXfloat xhi,FXfloat ylo,FXfloat yhi,FXfloat zlo,FXfloat zhi){
  FXfloat rl=xhi-xlo;
  FXfloat tb=yhi-ylo;
  FXfloat yh=zhi-zlo;
  return set(0.5f*rl,0.0f,0.0f,0.0f,0.0f,0.5f*tb,0.0f,0.0f,0.0f,0.0f,-0.5f*yh,0.0f,0.5f*(xhi+xlo),0.5f*(yhi+ylo),0.5f*(zhi+zlo),1.0f);
  }


// Set to perspective projection from view volume
FXMat4f& FXMat4f::setFrustum(FXfloat xlo,FXfloat xhi,FXfloat ylo,FXfloat yhi,FXfloat zlo,FXfloat zhi){
  FXfloat rl=xhi-xlo;
  FXfloat tb=yhi-ylo;
  FXfloat yh=zhi-zlo;
  return set(2.0f*zlo/rl,0.0f,0.0f,0.0f,0.0f,2.0f*zlo/tb,0.0f,0.0f,(xhi+xlo)/rl,(yhi+ylo)/tb,-(zhi+zlo)/yh,-1.0f,0.0f,0.0f,-2.0f*zhi*zlo/yh,0.0f);
  }


// Get view volume from perspective projection
void FXMat4f::getFrustum(FXfloat& xlo,FXfloat& xhi,FXfloat& ylo,FXfloat& yhi,FXfloat& zlo,FXfloat& zhi) const {
  zlo=m[3][2]/(m[2][2]-1.0f);
  zhi=m[3][2]/(m[2][2]+1.0f);
  xlo=zlo*(m[2][0]-1.0f)/m[0][0];
  xhi=zlo*(m[2][0]+1.0f)/m[0][0];
  yhi=zlo*(m[2][1]+1.0f)/m[1][1];
  ylo=zlo*(m[2][1]-1.0f)/m[1][1];
  }


// Set to inverse perspective projection from view volume
FXMat4f& FXMat4f::setInverseFrustum(FXfloat xlo,FXfloat xhi,FXfloat ylo,FXfloat yhi,FXfloat zlo,FXfloat zhi){
  FXfloat rl=xhi-xlo;
  FXfloat tb=yhi-ylo;
  FXfloat yh=zhi-zlo;
  return set(0.5f*rl/zlo,0.0f,0.0f,0.0f,0.0f,0.5f*tb/zlo,0.0f,0.0f,0.0f,0.0f,0.0f,-0.5f*yh/(zhi*zlo),0.5f*(xhi+xlo)/zlo,0.5f*(yhi+ylo)/zlo,-1.0f,0.5f*(zhi+zlo)/(zhi*zlo));
  }


// Make left hand matrix
FXMat4f& FXMat4f::left(){
  m[2][0]= -m[2][0];
  m[2][1]= -m[2][1];
  m[2][2]= -m[2][2];
  m[2][3]= -m[2][3];
  return *this;
  }


// Multiply by rotation matrix
FXMat4f& FXMat4f::rot(const FXMat3f& r){
  FXfloat x,y,z;
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
FXMat4f& FXMat4f::rot(const FXQuatf& q){
  return rot(FXMat3f(q));
  }


// Multiply by rotation c,s about unit axis
FXMat4f& FXMat4f::rot(const FXVec3f& v,FXfloat c,FXfloat s){
  FXfloat xx=v.x*v.x;
  FXfloat yy=v.y*v.y;
  FXfloat zz=v.z*v.z;
  FXfloat xy=v.x*v.y;
  FXfloat yz=v.y*v.z;
  FXfloat zx=v.z*v.x;
  FXfloat xs=v.x*s;
  FXfloat ys=v.y*s;
  FXfloat zs=v.z*s;
  FXfloat t=1.0f-c;
  return rot(FXMat3f(t*xx+c,t*xy+zs,t*zx-ys,t*xy-zs,t*yy+c,t*yz+xs,t*zx+ys,t*yz-xs,t*zz+c));
  }


// Multiply by rotation of phi about unit axis
FXMat4f& FXMat4f::rot(const FXVec3f& v,FXfloat phi){
  return rot(v,Math::cos(phi),Math::sin(phi));
  }


// Rotate about x-axis
FXMat4f& FXMat4f::xrot(FXfloat c,FXfloat s){
#if defined(FOX_HAS_SSE)
  __m128 cc=_mm_set1_ps(c);
  __m128 ss=_mm_set1_ps(s);
  __m128 uu=_mm_loadu_ps(&m[1][0]);
  __m128 vv=_mm_loadu_ps(&m[2][0]);
  _mm_storeu_ps(m[1],_mm_add_ps(_mm_mul_ps(cc,uu),_mm_mul_ps(ss,vv)));
  _mm_storeu_ps(m[2],_mm_sub_ps(_mm_mul_ps(cc,vv),_mm_mul_ps(ss,uu)));
#else
  FXfloat u,v;
  u=m[1][0]; v=m[2][0]; m[1][0]=c*u+s*v; m[2][0]=c*v-s*u;
  u=m[1][1]; v=m[2][1]; m[1][1]=c*u+s*v; m[2][1]=c*v-s*u;
  u=m[1][2]; v=m[2][2]; m[1][2]=c*u+s*v; m[2][2]=c*v-s*u;
  u=m[1][3]; v=m[2][3]; m[1][3]=c*u+s*v; m[2][3]=c*v-s*u;
#endif
  return *this;
  }


// Rotate by angle about x-axis
FXMat4f& FXMat4f::xrot(FXfloat phi){
  return xrot(Math::cos(phi),Math::sin(phi));
  }


// Rotate about y-axis
FXMat4f& FXMat4f::yrot(FXfloat c,FXfloat s){
#if defined(FOX_HAS_SSE)
  __m128 cc=_mm_set1_ps(c);
  __m128 ss=_mm_set1_ps(s);
  __m128 uu=_mm_loadu_ps(&m[0][0]);
  __m128 vv=_mm_loadu_ps(&m[2][0]);
  _mm_storeu_ps(m[0],_mm_sub_ps(_mm_mul_ps(cc,uu),_mm_mul_ps(ss,vv)));
  _mm_storeu_ps(m[2],_mm_add_ps(_mm_mul_ps(cc,vv),_mm_mul_ps(ss,uu)));
#else
  FXfloat u,v;
  u=m[0][0]; v=m[2][0]; m[0][0]=c*u-s*v; m[2][0]=c*v+s*u;
  u=m[0][1]; v=m[2][1]; m[0][1]=c*u-s*v; m[2][1]=c*v+s*u;
  u=m[0][2]; v=m[2][2]; m[0][2]=c*u-s*v; m[2][2]=c*v+s*u;
  u=m[0][3]; v=m[2][3]; m[0][3]=c*u-s*v; m[2][3]=c*v+s*u;
#endif
  return *this;
  }


// Rotate by angle about y-axis
FXMat4f& FXMat4f::yrot(FXfloat phi){
  return yrot(Math::cos(phi),Math::sin(phi));
  }


// Rotate about z-axis
FXMat4f& FXMat4f::zrot(FXfloat c,FXfloat s){
#if defined(FOX_HAS_SSE)
  __m128 cc=_mm_set1_ps(c);
  __m128 ss=_mm_set1_ps(s);
  __m128 uu=_mm_loadu_ps(&m[0][0]);
  __m128 vv=_mm_loadu_ps(&m[1][0]);
  _mm_storeu_ps(m[0],_mm_add_ps(_mm_mul_ps(cc,uu),_mm_mul_ps(ss,vv)));
  _mm_storeu_ps(m[1],_mm_sub_ps(_mm_mul_ps(cc,vv),_mm_mul_ps(ss,uu)));
#else
  FXfloat u,v;
  u=m[0][0]; v=m[1][0]; m[0][0]=c*u+s*v; m[1][0]=c*v-s*u;
  u=m[0][1]; v=m[1][1]; m[0][1]=c*u+s*v; m[1][1]=c*v-s*u;
  u=m[0][2]; v=m[1][2]; m[0][2]=c*u+s*v; m[1][2]=c*v-s*u;
  u=m[0][3]; v=m[1][3]; m[0][3]=c*u+s*v; m[1][3]=c*v-s*u;
#endif
  return *this;
  }


// Rotate by angle about z-axis
FXMat4f& FXMat4f::zrot(FXfloat phi){
  return zrot(Math::cos(phi),Math::sin(phi));
  }


// Look at
FXMat4f& FXMat4f::look(const FXVec3f& from,const FXVec3f& to,const FXVec3f& up){
  FXfloat x0,x1,x2,tx,ty,tz;
  FXVec3f rz,rx,ry;
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
FXMat4f& FXMat4f::trans(FXfloat tx,FXfloat ty,FXfloat tz){
#if defined(FOX_HAS_SSE)
  __m128 ttx=_mm_set1_ps(tx);
  __m128 tty=_mm_set1_ps(ty);
  __m128 ttz=_mm_set1_ps(tz);
  __m128 r0=_mm_mul_ps(_mm_loadu_ps(&m[0][0]),ttx);
  __m128 r1=_mm_mul_ps(_mm_loadu_ps(&m[1][0]),tty);
  __m128 r2=_mm_mul_ps(_mm_loadu_ps(&m[2][0]),ttz);
  __m128 r3=_mm_loadu_ps(&m[3][0]);
  _mm_storeu_ps(&m[3][0],_mm_add_ps(_mm_add_ps(r0,r1),_mm_add_ps(r2,r3)));
#else
  m[3][0]=m[3][0]+tx*m[0][0]+ty*m[1][0]+tz*m[2][0];
  m[3][1]=m[3][1]+tx*m[0][1]+ty*m[1][1]+tz*m[2][1];
  m[3][2]=m[3][2]+tx*m[0][2]+ty*m[1][2]+tz*m[2][2];
  m[3][3]=m[3][3]+tx*m[0][3]+ty*m[1][3]+tz*m[2][3];
#endif
  return *this;
  }


// Translate over vector
FXMat4f& FXMat4f::trans(const FXVec3f& v){
  return trans(v[0],v[1],v[2]);
  }


// Scale unqual
FXMat4f& FXMat4f::scale(FXfloat sx,FXfloat sy,FXfloat sz){
#if defined(FOX_HAS_SSE)
  __m128 ssx=_mm_set1_ps(sx);
  __m128 ssy=_mm_set1_ps(sy);
  __m128 ssz=_mm_set1_ps(sz);
  _mm_storeu_ps(&m[0][0],_mm_mul_ps(_mm_loadu_ps(&m[0][0]),ssx));
  _mm_storeu_ps(&m[1][0],_mm_mul_ps(_mm_loadu_ps(&m[1][0]),ssy));
  _mm_storeu_ps(&m[2][0],_mm_mul_ps(_mm_loadu_ps(&m[2][0]),ssz));
#else
  m[0][0]*=sx; m[0][1]*=sx; m[0][2]*=sx; m[0][3]*=sx;
  m[1][0]*=sy; m[1][1]*=sy; m[1][2]*=sy; m[1][3]*=sy;
  m[2][0]*=sz; m[2][1]*=sz; m[2][2]*=sz; m[2][3]*=sz;
#endif
  return *this;
  }


// Scale unqual
FXMat4f& FXMat4f::scale(const FXVec3f& v){
  return scale(v[0],v[1],v[2]);
  }


// Scale uniform
FXMat4f& FXMat4f::scale(FXfloat s){
  return scale(s,s,s);
  }


// Calculate determinant
FXfloat FXMat4f::det() const {
  return (m[0][0]*m[1][1]-m[0][1]*m[1][0]) * (m[2][2]*m[3][3]-m[2][3]*m[3][2]) -
         (m[0][0]*m[1][2]-m[0][2]*m[1][0]) * (m[2][1]*m[3][3]-m[2][3]*m[3][1]) +
         (m[0][0]*m[1][3]-m[0][3]*m[1][0]) * (m[2][1]*m[3][2]-m[2][2]*m[3][1]) +
         (m[0][1]*m[1][2]-m[0][2]*m[1][1]) * (m[2][0]*m[3][3]-m[2][3]*m[3][0]) -
         (m[0][1]*m[1][3]-m[0][3]*m[1][1]) * (m[2][0]*m[3][2]-m[2][2]*m[3][0]) +
         (m[0][2]*m[1][3]-m[0][3]*m[1][2]) * (m[2][0]*m[3][1]-m[2][1]*m[3][0]);
  }



// Transpose matrix
FXMat4f FXMat4f::transpose() const {
#if defined(FOX_HAS_SSE)
  FXMat4f r;
  __m128 m0=_mm_loadu_ps(&m[0][0]);
  __m128 m1=_mm_loadu_ps(&m[1][0]);
  __m128 m2=_mm_loadu_ps(&m[2][0]);
  __m128 m3=_mm_loadu_ps(&m[3][0]);
  __m128 t0=_mm_unpacklo_ps(m0,m1);    // m11 m01 m10 m00
  __m128 t1=_mm_unpacklo_ps(m2,m3);    // m31 m21 m30 m20
  __m128 t2=_mm_unpackhi_ps(m0,m1);    // m13 m03 m12 m02
  __m128 t3=_mm_unpackhi_ps(m2,m3);    // m33 m23 m32 m22
  _mm_storeu_ps(r[0],_mm_movelh_ps(t0,t1));     // m30 m20 m10 m00
  _mm_storeu_ps(r[1],_mm_movehl_ps(t1,t0));     // m31 m21 m11 m01
  _mm_storeu_ps(r[2],_mm_movelh_ps(t2,t3));     // m32 m22 m12 m02
  _mm_storeu_ps(r[3],_mm_movehl_ps(t3,t2));     // m33 m23 m13 m03
  return r;
#else
  return FXMat4f(m[0][0],m[1][0],m[2][0],m[3][0],
                 m[0][1],m[1][1],m[2][1],m[3][1],
                 m[0][2],m[1][2],m[2][2],m[3][2],
                 m[0][3],m[1][3],m[2][3],m[3][3]);
#endif
  }


// Invert matrix
FXMat4f FXMat4f::invert() const {
  FXMat4f r;
  FXfloat a0=m[0][0]*m[1][1]-m[0][1]*m[1][0];
  FXfloat a1=m[0][0]*m[1][2]-m[0][2]*m[1][0];
  FXfloat a2=m[0][0]*m[1][3]-m[0][3]*m[1][0];
  FXfloat a3=m[0][1]*m[1][2]-m[0][2]*m[1][1];
  FXfloat a4=m[0][1]*m[1][3]-m[0][3]*m[1][1];
  FXfloat a5=m[0][2]*m[1][3]-m[0][3]*m[1][2];
  FXfloat b0=m[2][0]*m[3][1]-m[2][1]*m[3][0];
  FXfloat b1=m[2][0]*m[3][2]-m[2][2]*m[3][0];
  FXfloat b2=m[2][0]*m[3][3]-m[2][3]*m[3][0];
  FXfloat b3=m[2][1]*m[3][2]-m[2][2]*m[3][1];
  FXfloat b4=m[2][1]*m[3][3]-m[2][3]*m[3][1];
  FXfloat b5=m[2][2]*m[3][3]-m[2][3]*m[3][2];
  FXfloat dd=a0*b5-a1*b4+a2*b3+a3*b2-a4*b1+a5*b0;
  FXASSERT(dd!=0.0f);
  dd=1.0f/dd;
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
FXMat4f FXMat4f::affineInvert() const {
  FXfloat dd;
  FXMat4f r;
  r[0][0]=m[1][1]*m[2][2]-m[1][2]*m[2][1];
  r[0][1]=m[0][2]*m[2][1]-m[0][1]*m[2][2];
  r[0][2]=m[0][1]*m[1][2]-m[0][2]*m[1][1];
  r[0][3]=0.0f;
  r[1][0]=m[1][2]*m[2][0]-m[1][0]*m[2][2];
  r[1][1]=m[0][0]*m[2][2]-m[0][2]*m[2][0];
  r[1][2]=m[0][2]*m[1][0]-m[0][0]*m[1][2];
  r[1][3]=0.0f;
  r[2][0]=m[1][0]*m[2][1]-m[1][1]*m[2][0];
  r[2][1]=m[0][1]*m[2][0]-m[0][0]*m[2][1];
  r[2][2]=m[0][0]*m[1][1]-m[0][1]*m[1][0];
  r[2][3]=0.0f;
  dd=m[0][0]*r[0][0]+m[0][1]*r[1][0]+m[0][2]*r[2][0];
  FXASSERT(dd!=0.0f);
  dd=1.0f/dd;
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
  r[3][3]=1.0f;
  return r;
  }


// Invert rigid body transform matrix
FXMat4f FXMat4f::rigidInvert() const {
  FXfloat ss;
  FXMat4f r;
  ss=1.0f/(m[0][0]*m[0][0]+m[0][1]*m[0][1]+m[0][2]*m[0][2]);
  r[0][0]=m[0][0]*ss;
  r[0][1]=m[1][0]*ss;
  r[0][2]=m[2][0]*ss;
  r[0][3]=0.0f;
  r[1][0]=m[0][1]*ss;
  r[1][1]=m[1][1]*ss;
  r[1][2]=m[2][1]*ss;
  r[1][3]=0.0f;
  r[2][0]=m[0][2]*ss;
  r[2][1]=m[1][2]*ss;
  r[2][2]=m[2][2]*ss;
  r[2][3]=0.0f;
  r[3][0]=-(r[0][0]*m[3][0]+r[1][0]*m[3][1]+r[2][0]*m[3][2]);
  r[3][1]=-(r[0][1]*m[3][0]+r[1][1]*m[3][1]+r[2][1]*m[3][2]);
  r[3][2]=-(r[0][2]*m[3][0]+r[1][2]*m[3][1]+r[2][2]*m[3][2]);
  r[3][3]=1.0f;
  return r;
  }


// Return normal-transformation matrix (inverse transpose of upper 3x3 block)
FXMat3f FXMat4f::normalMatrix() const {
  FXfloat dd;
  FXMat3f res;
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
  FXASSERT(dd!=0.0f);
  dd=1.0f/dd;
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
FXMat4f orthogonalize(const FXMat4f& m){
  FXMat4f result(m);
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
FXVec3f operator*(const FXMat4f& m,const FXVec3f& v){
#if defined(FOX_HAS_SSE3)
  __m128 m0=_mm_loadu_ps(&m[0][0]);
  __m128 m1=_mm_loadu_ps(&m[1][0]);
  __m128 m2=_mm_loadu_ps(&m[2][0]);
  __m128 vv=_mm_set_ps(1.0f,v[2],v[1],v[0]);
  __m128 r0=_mm_mul_ps(m0,vv);         // m03 m02*v2 m01*v1 m00*v0
  __m128 r1=_mm_mul_ps(m1,vv);         // m13 m12*v2 m11*v1 m10*v0
  __m128 r2=_mm_mul_ps(m2,vv);         // m23 m22*v2 m21*v1 m20*v0
  FXVec3f r;
  r0=_mm_hadd_ps(r0,r1);        // m13+m12*v2  m11*v1+m10*v0  m03+m02*v2  m01*v1+m00*v0
  r2=_mm_hadd_ps(r2,m0);        // **********  *************  m23+m22*v2  m21*v1+m20*v0
  r0=_mm_hadd_ps(r0,r2);        // ************************  m23+m22*v2+m21*v1+m20*v0  m13+m12*v2+m11*v1+m10*v0  m03+m02*v2+m01*v1+m00*v0
  _mm_storel_pi((__m64*)&r[0],r0);
  _mm_store_ss(&r[2],_mm_movehl_ps(r0,r0));
  return r;
#else
  return FXVec3f(m[0][0]*v[0]+m[0][1]*v[1]+m[0][2]*v[2]+m[0][3], m[1][0]*v[0]+m[1][1]*v[1]+m[1][2]*v[2]+m[1][3], m[2][0]*v[0]+m[2][1]*v[1]+m[2][2]*v[2]+m[2][3]);
#endif
  }


// Matrix times vector
FXVec4f operator*(const FXMat4f& m,const FXVec4f& v){
#if defined(FOX_HAS_SSE3)
  __m128 vv=_mm_loadu_ps(v);
  __m128 r0=_mm_mul_ps(_mm_loadu_ps(&m[0][0]),vv);     // m03*v3 m02*v2 m01*v1 m00*v0
  __m128 r1=_mm_mul_ps(_mm_loadu_ps(&m[1][0]),vv);     // m13*v3 m12*v2 m11*v1 m10*v0
  __m128 r2=_mm_mul_ps(_mm_loadu_ps(&m[2][0]),vv);     // m23*v3 m22*v2 m21*v1 m20*v0
  __m128 r3=_mm_mul_ps(_mm_loadu_ps(&m[3][0]),vv);     // m33*v3 m32*v2 m31*v1 m30*v0
  FXVec4f r;
  r0=_mm_hadd_ps(r0,r1);        // m13*v3+m12*v2  m11*v1+m10*v0  m03*v3+m02*v2  m01*v1+m00*v0
  r2=_mm_hadd_ps(r2,r3);        // m33*v3+m32*v2  m31*v1+m30*v0  m23*v3+m22*v2  m21*v1+m20*v0
  r0=_mm_hadd_ps(r0,r2);        // m33*v3+m32*v2+m31*v1+m30*v0  m23*v3+m22*v2+m21*v1+m20*v0  m13*v3+m12*v2+m11*v1+m10*v0  m03*v3+m02*v2+m01*v1+m00*v0
  _mm_storeu_ps(&r[0],r0);
  return r;
#else
  return FXVec4f(m[0][0]*v[0]+m[0][1]*v[1]+m[0][2]*v[2]+m[0][3]*v[3], m[1][0]*v[0]+m[1][1]*v[1]+m[1][2]*v[2]+m[1][3]*v[3], m[2][0]*v[0]+m[2][1]*v[1]+m[2][2]*v[2]+m[2][3]*v[3], m[3][0]*v[0]+m[3][1]*v[1]+m[3][2]*v[2]+m[3][3]*v[3]);
#endif
  }


// Vector times matrix
//
//  v[0]*m[0][0] + v[1]*m[1][0] + v[2]*m[2][0] + m[3][0]
//  v[0]*m[0][1] + v[1]*m[1][1] + v[2]*m[2][1] + m[3][1]
//  v[0]*m[0][2] + v[1]*m[1][2] + v[2]*m[2][2] + m[3][2]
//
FXVec3f operator*(const FXVec3f& v,const FXMat4f& m){
#if defined(FOX_HAS_SSE)
  __m128 m0=_mm_loadu_ps(&m[0][0]);
  __m128 m1=_mm_loadu_ps(&m[1][0]);
  __m128 m2=_mm_loadu_ps(&m[2][0]);
  __m128 m3=_mm_loadu_ps(&m[3][0]);
  __m128 v0=_mm_set1_ps(v[0]);
  __m128 v1=_mm_set1_ps(v[1]);
  __m128 v2=_mm_set1_ps(v[2]);
  __m128 rr=_mm_add_ps(_mm_add_ps(_mm_mul_ps(v0,m0),_mm_mul_ps(v1,m1)),_mm_add_ps(_mm_mul_ps(v2,m2),m3));
  FXVec3f r;
  _mm_storel_pi((__m64*)&r[0],rr);
  _mm_store_ss(&r[2],_mm_movehl_ps(rr,rr));
  return r;
#else
  return FXVec3f(v[0]*m[0][0]+v[1]*m[1][0]+v[2]*m[2][0]+m[3][0], v[0]*m[0][1]+v[1]*m[1][1]+v[2]*m[2][1]+m[3][1], v[0]*m[0][2]+v[1]*m[1][2]+v[2]*m[2][2]+m[3][2]);
#endif
  }


// Vector times matrix
//
//  v[0]*m[0][0] + v[1]*m[1][0] + v[2]*m[2][0] + v[3]*m[3][0]
//  v[0]*m[0][1] + v[1]*m[1][1] + v[2]*m[2][1] + v[3]*m[3][1]
//  v[0]*m[0][2] + v[1]*m[1][2] + v[2]*m[2][2] + v[3]*m[3][2]
//  v[0]*m[0][3] + v[1]*m[1][3] + v[2]*m[2][3] + v[3]*m[3][3]
//
FXVec4f operator*(const FXVec4f& v,const FXMat4f& m){
#if defined(FOX_HAS_SSE)
  __m128 m0=_mm_loadu_ps(&m[0][0]);
  __m128 m1=_mm_loadu_ps(&m[1][0]);
  __m128 m2=_mm_loadu_ps(&m[2][0]);
  __m128 m3=_mm_loadu_ps(&m[3][0]);
  __m128 v0=_mm_set1_ps(v[0]);
  __m128 v1=_mm_set1_ps(v[1]);
  __m128 v2=_mm_set1_ps(v[2]);
  __m128 v3=_mm_set1_ps(v[3]);
  FXVec4f r;
  _mm_storeu_ps(&r[0],_mm_add_ps(_mm_add_ps(_mm_mul_ps(v0,m0),_mm_mul_ps(v1,m1)),_mm_add_ps(_mm_mul_ps(v2,m2),_mm_mul_ps(v3,m3))));
  return r;
#else
  return FXVec4f(v[0]*m[0][0]+v[1]*m[1][0]+v[2]*m[2][0]+v[3]*m[3][0], v[0]*m[0][1]+v[1]*m[1][1]+v[2]*m[2][1]+v[3]*m[3][1], v[0]*m[0][2]+v[1]*m[1][2]+v[2]*m[2][2]+v[3]*m[3][2], v[0]*m[0][3]+v[1]*m[1][3]+v[2]*m[2][3]+v[3]*m[3][3]);
#endif
  }


// Matrix and matrix add
FXMat4f operator+(const FXMat4f& a,const FXMat4f& b){
#if defined(FOX_HAS_SSE)
  FXMat4f r;
  _mm_storeu_ps(&r[0][0],_mm_add_ps(_mm_loadu_ps(&a[0][0]),_mm_loadu_ps(&b[0][0])));
  _mm_storeu_ps(&r[1][0],_mm_add_ps(_mm_loadu_ps(&a[1][0]),_mm_loadu_ps(&b[1][0])));
  _mm_storeu_ps(&r[2][0],_mm_add_ps(_mm_loadu_ps(&a[2][0]),_mm_loadu_ps(&b[2][0])));
  _mm_storeu_ps(&r[3][0],_mm_add_ps(_mm_loadu_ps(&a[3][0]),_mm_loadu_ps(&b[3][0])));
  return r;
#else
  return FXMat4f(a[0][0]+b[0][0],a[0][1]+b[0][1],a[0][2]+b[0][2],a[0][3]+b[0][3],
                 a[1][0]+b[1][0],a[1][1]+b[1][1],a[1][2]+b[1][2],a[1][3]+b[1][3],
                 a[2][0]+b[2][0],a[2][1]+b[2][1],a[2][2]+b[2][2],a[2][3]+b[2][3],
                 a[3][0]+b[3][0],a[3][1]+b[3][1],a[3][2]+b[3][2],a[3][3]+b[3][3]);
#endif
  }


// Matrix and matrix subtract
FXMat4f operator-(const FXMat4f& a,const FXMat4f& b){
#if defined(FOX_HAS_SSE)
  FXMat4f r;
  _mm_storeu_ps(&r[0][0],_mm_sub_ps(_mm_loadu_ps(&a[0][0]),_mm_loadu_ps(&b[0][0])));
  _mm_storeu_ps(&r[1][0],_mm_sub_ps(_mm_loadu_ps(&a[1][0]),_mm_loadu_ps(&b[1][0])));
  _mm_storeu_ps(&r[2][0],_mm_sub_ps(_mm_loadu_ps(&a[2][0]),_mm_loadu_ps(&b[2][0])));
  _mm_storeu_ps(&r[3][0],_mm_sub_ps(_mm_loadu_ps(&a[3][0]),_mm_loadu_ps(&b[3][0])));
  return r;
#else
  return FXMat4f(a[0][0]-b[0][0],a[0][1]-b[0][1],a[0][2]-b[0][2],a[0][3]-b[0][3],
                 a[1][0]-b[1][0],a[1][1]-b[1][1],a[1][2]-b[1][2],a[1][3]-b[1][3],
                 a[2][0]-b[2][0],a[2][1]-b[2][1],a[2][2]-b[2][2],a[2][3]-b[2][3],
                 a[3][0]-b[3][0],a[3][1]-b[3][1],a[3][2]-b[3][2],a[3][3]-b[3][3]);
#endif
  }


// Matrix and matrix multiply
FXMat4f operator*(const FXMat4f& a,const FXMat4f& b){
#if defined(FOX_HAS_SSE)
  __m128 b0=_mm_loadu_ps(&b[0][0]);
  __m128 b1=_mm_loadu_ps(&b[1][0]);
  __m128 b2=_mm_loadu_ps(&b[2][0]);
  __m128 b3=_mm_loadu_ps(&b[3][0]);
  __m128 xx,yy,zz,ww;
  FXMat4f r;
  xx=_mm_set1_ps(a[0][0]);
  yy=_mm_set1_ps(a[0][1]);
  zz=_mm_set1_ps(a[0][2]);
  ww=_mm_set1_ps(a[0][3]);
  _mm_storeu_ps(r[0],_mm_add_ps(_mm_add_ps(_mm_mul_ps(b0,xx),_mm_mul_ps(b1,yy)),_mm_add_ps(_mm_mul_ps(b2,zz),_mm_mul_ps(b3,ww))));
  xx=_mm_set1_ps(a[1][0]);
  yy=_mm_set1_ps(a[1][1]);
  zz=_mm_set1_ps(a[1][2]);
  ww=_mm_set1_ps(a[1][3]);
  _mm_storeu_ps(r[1],_mm_add_ps(_mm_add_ps(_mm_mul_ps(b0,xx),_mm_mul_ps(b1,yy)),_mm_add_ps(_mm_mul_ps(b2,zz),_mm_mul_ps(b3,ww))));
  xx=_mm_set1_ps(a[2][0]);
  yy=_mm_set1_ps(a[2][1]);
  zz=_mm_set1_ps(a[2][2]);
  ww=_mm_set1_ps(a[2][3]);
  _mm_storeu_ps(r[2],_mm_add_ps(_mm_add_ps(_mm_mul_ps(b0,xx),_mm_mul_ps(b1,yy)),_mm_add_ps(_mm_mul_ps(b2,zz),_mm_mul_ps(b3,ww))));
  xx=_mm_set1_ps(a[3][0]);
  yy=_mm_set1_ps(a[3][1]);
  zz=_mm_set1_ps(a[3][2]);
  ww=_mm_set1_ps(a[3][3]);
  _mm_storeu_ps(r[3],_mm_add_ps(_mm_add_ps(_mm_mul_ps(b0,xx),_mm_mul_ps(b1,yy)),_mm_add_ps(_mm_mul_ps(b2,zz),_mm_mul_ps(b3,ww))));
  return r;
#else
  FXfloat x,y,z,w;
  FXMat4f r;
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
FXMat4f operator*(FXfloat x,const FXMat4f& a){
#if defined(FOX_HAS_SSE)
  FXMat4f r;
  _mm_storeu_ps(&r[0][0],_mm_mul_ps(_mm_set1_ps(x),_mm_loadu_ps(&a[0][0])));
  _mm_storeu_ps(&r[1][0],_mm_mul_ps(_mm_set1_ps(x),_mm_loadu_ps(&a[1][0])));
  _mm_storeu_ps(&r[2][0],_mm_mul_ps(_mm_set1_ps(x),_mm_loadu_ps(&a[2][0])));
  _mm_storeu_ps(&r[3][0],_mm_mul_ps(_mm_set1_ps(x),_mm_loadu_ps(&a[3][0])));
  return r;
#else
  return FXMat4f(x*a[0][0],x*a[0][1],x*a[0][2],a[0][3],
                 x*a[1][0],x*a[1][1],x*a[1][2],a[1][3],
                 x*a[2][0],x*a[2][1],x*a[2][2],a[2][3],
                 x*a[3][0],x*a[3][1],x*a[3][2],a[3][3]);
#endif
  }


// Multiply matrix by scalar
FXMat4f operator*(const FXMat4f& a,FXfloat x){
#if defined(FOX_HAS_SSE)
  FXMat4f r;
  _mm_storeu_ps(&r[0][0],_mm_mul_ps(_mm_loadu_ps(&a[0][0]),_mm_set1_ps(x)));
  _mm_storeu_ps(&r[1][0],_mm_mul_ps(_mm_loadu_ps(&a[1][0]),_mm_set1_ps(x)));
  _mm_storeu_ps(&r[2][0],_mm_mul_ps(_mm_loadu_ps(&a[2][0]),_mm_set1_ps(x)));
  _mm_storeu_ps(&r[3][0],_mm_mul_ps(_mm_loadu_ps(&a[3][0]),_mm_set1_ps(x)));
  return r;
#else
  return FXMat4f(a[0][0]*x,a[0][1]*x,a[0][2]*x,a[0][3],
                 a[1][0]*x,a[1][1]*x,a[1][2]*x,a[1][3],
                 a[2][0]*x,a[2][1]*x,a[2][2]*x,a[2][3],
                 a[3][0]*x,a[3][1]*x,a[3][2]*x,a[3][3]);
#endif
  }


// Divide scalar by matrix
FXMat4f operator/(FXfloat x,const FXMat4f& a){
#if defined(FOX_HAS_SSE)
  FXMat4f r;
  _mm_storeu_ps(&r[0][0],_mm_div_ps(_mm_set1_ps(x),_mm_loadu_ps(&a[0][0])));
  _mm_storeu_ps(&r[1][0],_mm_div_ps(_mm_set1_ps(x),_mm_loadu_ps(&a[1][0])));
  _mm_storeu_ps(&r[2][0],_mm_div_ps(_mm_set1_ps(x),_mm_loadu_ps(&a[2][0])));
  _mm_storeu_ps(&r[3][0],_mm_div_ps(_mm_set1_ps(x),_mm_loadu_ps(&a[3][0])));
  return r;
#else
  return FXMat4f(x/a[0][0],x/a[0][1],x/a[0][2],a[0][3],
                 x/a[1][0],x/a[1][1],x/a[1][2],a[1][3],
                 x/a[2][0],x/a[2][1],x/a[2][2],a[2][3],
                 x/a[3][0],x/a[3][1],x/a[3][2],a[3][3]);
#endif
  }


// Divide matrix by scalar
FXMat4f operator/(const FXMat4f& a,FXfloat x){
#if defined(FOX_HAS_SSE)
  FXMat4f r;
  _mm_storeu_ps(&r[0][0],_mm_div_ps(_mm_loadu_ps(&a[0][0]),_mm_set1_ps(x)));
  _mm_storeu_ps(&r[1][0],_mm_div_ps(_mm_loadu_ps(&a[1][0]),_mm_set1_ps(x)));
  _mm_storeu_ps(&r[2][0],_mm_div_ps(_mm_loadu_ps(&a[2][0]),_mm_set1_ps(x)));
  _mm_storeu_ps(&r[3][0],_mm_div_ps(_mm_loadu_ps(&a[3][0]),_mm_set1_ps(x)));
  return r;
#else
  return FXMat4f(a[0][0]/x,a[0][1]/x,a[0][2]/x,a[0][3],
                 a[1][0]/x,a[1][1]/x,a[1][2]/x,a[1][3],
                 a[2][0]/x,a[2][1]/x,a[2][2]/x,a[2][3],
                 a[3][0]/x,a[3][1]/x,a[3][2]/x,a[3][3]);
#endif
  }


// Matrix and matrix equality
FXbool operator==(const FXMat4f& a,const FXMat4f& b){
  return a[0]==b[0] && a[1]==b[1] && a[2]==b[2] && a[3]==b[3];
  }


// Matrix and matrix inequality
FXbool operator!=(const FXMat4f& a,const FXMat4f& b){
  return a[0]!=b[0] || a[1]!=b[1] || a[2]!=b[2] || a[3]!=b[3];
  }


// Matrix and scalar equality
FXbool operator==(const FXMat4f& a,FXfloat n){
  return a[0]==n && a[1]==n && a[2]==n && a[3]==n;
  }


// Scalar and matrix equality
FXbool operator==(FXfloat n,const FXMat4f& a){
  return n==a[0] && n==a[1] && n==a[2] && n==a[3];
  }


// Matrix and scalar inequality
FXbool operator!=(const FXMat4f& a,FXfloat n){
  return a[0]!=n || a[1]!=n || a[2]!=n || a[3]!=n;
  }


// Scalar and matrix inequality
FXbool operator!=(FXfloat n,const FXMat4f& a){
  return n!=a[0] || n!=a[1] || n!=a[2] || n!=a[3];
  }


// Save to archive
FXStream& operator<<(FXStream& store,const FXMat4f& m){
  store << m[0] << m[1] << m[2] << m[3];
  return store;
  }


// Load from archive
FXStream& operator>>(FXStream& store,FXMat4f& m){
  store >> m[0] >> m[1] >> m[2] >> m[3];
  return store;
  }

}
