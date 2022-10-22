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
#include "FXMat2d.h"
#include "FXMat3d.h"


/*
  Notes:
*/


using namespace FX;

/*******************************************************************************/

namespace FX {


// Initialize matrix from scalar
FXMat2d::FXMat2d(FXdouble s){
#if defined(FOX_HAS_AVX)
  _mm256_storeu_pd(&m[0][0],_mm256_set1_pd(s));
#elif defined(FOX_HAS_SSE2)
  _mm_storeu_pd(&m[0][0],_mm_set1_pd(s));
  _mm_storeu_pd(&m[1][0],_mm_set1_pd(s));
#else
  m[0][0]=s; m[0][1]=s;
  m[1][0]=s; m[1][1]=s;
#endif
  }


// Initialize matrix from another matrix
FXMat2d::FXMat2d(const FXMat2d& s){
#if defined(FOX_HAS_AVX)
  _mm256_storeu_pd(&m[0][0],_mm256_loadu_pd(&s[0][0]));
#elif defined(FOX_HAS_SSE2)
  _mm_storeu_pd(&m[0][0],_mm_loadu_pd(&s[0][0]));
  _mm_storeu_pd(&m[1][0],_mm_loadu_pd(&s[1][0]));
#else
  m[0][0]=s[0][0]; m[0][1]=s[0][1];
  m[1][0]=s[1][0]; m[1][1]=s[1][1];
#endif
  }


// Initialize from rotation and scaling part of 3x3 matrix
FXMat2d::FXMat2d(const FXMat3d& s){
#if defined(FOX_HAS_SSE2)
  _mm_storeu_pd(&m[0][0],_mm_loadu_pd(&s[0][0]));
  _mm_storeu_pd(&m[1][0],_mm_loadu_pd(&s[1][0]));
#else
  m[0][0]=s[0][0]; m[0][1]=s[0][1];
  m[1][0]=s[1][0]; m[1][1]=s[1][1];
#endif
  }


// Initialize matrix from array
FXMat2d::FXMat2d(const FXdouble s[]){
#if defined(FOX_HAS_AVX)
  _mm256_storeu_pd(&m[0][0],_mm256_loadu_pd(s));
#elif defined(FOX_HAS_SSE2)
  _mm_storeu_pd(&m[0][0],_mm_loadu_pd(s+0));
  _mm_storeu_pd(&m[1][0],_mm_loadu_pd(s+2));
#else
  m[0][0]=s[0]; m[0][1]=s[1];
  m[1][0]=s[2]; m[1][1]=s[3];
#endif
  }


// Initialize diagonal matrix
FXMat2d::FXMat2d(FXdouble a,FXdouble b){
  m[0][0]=a;   m[0][1]=0.0;
  m[1][0]=0.0; m[1][1]=b;
  }


// Initialize matrix from components
FXMat2d::FXMat2d(FXdouble a00,FXdouble a01,FXdouble a10,FXdouble a11){
  m[0][0]=a00; m[0][1]=a01;
  m[1][0]=a10; m[1][1]=a11;
  }


// Initialize matrix from two vectors
FXMat2d::FXMat2d(const FXVec2d& a,const FXVec2d& b){
#if defined(FOX_HAS_SSE2)
  _mm_storeu_pd(&m[0][0],_mm_loadu_pd(a));
  _mm_storeu_pd(&m[1][0],_mm_loadu_pd(b));
#else
  m[0]=a;
  m[1]=b;
#endif
  }


// Assign from scalar
FXMat2d& FXMat2d::operator=(FXdouble s){
#if defined(FOX_HAS_AVX)
  _mm256_storeu_pd(&m[0][0],_mm256_set1_pd(s));
#elif defined(FOX_HAS_SSE2)
  _mm_storeu_pd(&m[0][0],_mm_set1_pd(s));
  _mm_storeu_pd(&m[1][0],_mm_set1_pd(s));
#else
  m[0][0]=s; m[0][1]=s;
  m[1][0]=s; m[1][1]=s;
#endif
  return *this;
  }


// Assignment operator
FXMat2d& FXMat2d::operator=(const FXMat2d& s){
#if defined(FOX_HAS_AVX)
  _mm256_storeu_pd(&m[0][0],_mm256_loadu_pd(&s[0][0]));
#elif defined(FOX_HAS_SSE2)
  _mm_storeu_pd(&m[0][0],_mm_loadu_pd(&s[0][0]));
  _mm_storeu_pd(&m[1][0],_mm_loadu_pd(&s[1][0]));
#else
  m[0][0]=s[0][0]; m[0][1]=s[0][1];
  m[1][0]=s[1][0]; m[1][1]=s[1][1];
#endif
  return *this;
  }


// Assign from rotation and scaling part of 3x3 matrix
FXMat2d& FXMat2d::operator=(const FXMat3d& s){
#if defined(FOX_HAS_SSE2)
  _mm_storeu_pd(&m[0][0],_mm_loadu_pd(&s[0][0]));
  _mm_storeu_pd(&m[1][0],_mm_loadu_pd(&s[1][0]));
#else
  m[0][0]=s[0][0]; m[0][1]=s[0][1];
  m[1][0]=s[1][0]; m[1][1]=s[1][1];
#endif
  return *this;
  }


// Assignment from array
FXMat2d& FXMat2d::operator=(const FXdouble s[]){
#if defined(FOX_HAS_AVX)
  _mm256_storeu_pd(&m[0][0],_mm256_loadu_pd(s));
#elif defined(FOX_HAS_SSE2)
  _mm_storeu_pd(&m[0][0],_mm_loadu_pd(s+0));
  _mm_storeu_pd(&m[1][0],_mm_loadu_pd(s+2));
#else
  m[0][0]=s[0]; m[0][1]=s[1];
  m[1][0]=s[2]; m[1][1]=s[3];
#endif
  return *this;
  }


// Set value from scalar
FXMat2d& FXMat2d::set(FXdouble s){
#if defined(FOX_HAS_AVX)
  _mm256_storeu_pd(&m[0][0],_mm256_set1_pd(s));
#elif defined(FOX_HAS_SSE2)
  _mm_storeu_pd(&m[0][0],_mm_set1_pd(s));
  _mm_storeu_pd(&m[1][0],_mm_set1_pd(s));
#else
  m[0][0]=s; m[0][1]=s;
  m[1][0]=s; m[1][1]=s;
#endif
  return *this;
  }


// Set value from another matrix
FXMat2d& FXMat2d::set(const FXMat2d& s){
#if defined(FOX_HAS_AVX)
  _mm256_storeu_pd(&m[0][0],_mm256_loadu_pd(&s[0][0]));
#elif defined(FOX_HAS_SSE2)
  _mm_storeu_pd(&m[0][0],_mm_loadu_pd(&s[0][0]));
  _mm_storeu_pd(&m[1][0],_mm_loadu_pd(&s[1][0]));
#else
  m[0][0]=s[0][0]; m[0][1]=s[0][1];
  m[1][0]=s[1][0]; m[1][1]=s[1][1];
#endif
  return *this;
  }


// Set from rotation and scaling part of 3x3 matrix
FXMat2d& FXMat2d::set(const FXMat3d& s){
#if defined(FOX_HAS_SSE2)
  _mm_storeu_pd(&m[0][0],_mm_loadu_pd(&s[0][0]));
  _mm_storeu_pd(&m[1][0],_mm_loadu_pd(&s[1][0]));
#else
  m[0][0]=s[0][0]; m[0][1]=s[0][1];
  m[1][0]=s[1][0]; m[1][1]=s[1][1];
#endif
  return *this;
  }


// Set value from array
FXMat2d& FXMat2d::set(const FXdouble s[]){
#if defined(FOX_HAS_AVX)
  _mm256_storeu_pd(&m[0][0],_mm256_loadu_pd(s));
#elif defined(FOX_HAS_SSE2)
  _mm_storeu_pd(&m[0][0],_mm_loadu_pd(s+0));
  _mm_storeu_pd(&m[1][0],_mm_loadu_pd(s+2));
#else
  m[0][0]=s[0]; m[0][1]=s[1];
  m[1][0]=s[2]; m[1][1]=s[3];
#endif
  return *this;
  }


// Set diagonal matrix
FXMat2d& FXMat2d::set(FXdouble a,FXdouble b){
  m[0][0]=a;   m[0][1]=0.0;
  m[1][0]=0.0; m[1][1]=b;
  return *this;
  }


// Set value from components
FXMat2d& FXMat2d::set(FXdouble a00,FXdouble a01,FXdouble a10,FXdouble a11){
  m[0][0]=a00; m[0][1]=a01;
  m[1][0]=a10; m[1][1]=a11;
  return *this;
  }


// Set value from two vectors
FXMat2d& FXMat2d::set(const FXVec2d& a,const FXVec2d& b){
#if defined(FOX_HAS_SSE2)
  _mm_storeu_pd(&m[0][0],_mm_loadu_pd(a));
  _mm_storeu_pd(&m[1][0],_mm_loadu_pd(b));
#else
  m[0]=a;
  m[1]=b;
#endif
  return *this;
  }


// Add matrices
FXMat2d& FXMat2d::operator+=(const FXMat2d& s){
#if defined(FOX_HAS_AVX)
  _mm256_storeu_pd(&m[0][0],_mm256_add_pd(_mm256_loadu_pd(&m[0][0]),_mm256_loadu_pd(&s[0][0])));
#elif defined(FOX_HAS_SSE2)
  _mm_storeu_pd(&m[0][0],_mm_add_pd(_mm_loadu_pd(&m[0][0]),_mm_loadu_pd(&s[0][0])));
  _mm_storeu_pd(&m[1][0],_mm_add_pd(_mm_loadu_pd(&m[1][0]),_mm_loadu_pd(&s[1][0])));
#else
  m[0][0]+=s[0][0]; m[0][1]+=s[0][1];
  m[1][0]+=s[1][0]; m[1][1]+=s[1][1];
#endif
  return *this;
  }


// Subtract matrices
FXMat2d& FXMat2d::operator-=(const FXMat2d& s){
#if defined(FOX_HAS_AVX)
  _mm256_storeu_pd(&m[0][0],_mm256_sub_pd(_mm256_loadu_pd(&m[0][0]),_mm256_loadu_pd(&s[0][0])));
#elif defined(FOX_HAS_SSE2)
  _mm_storeu_pd(&m[0][0],_mm_sub_pd(_mm_loadu_pd(&m[0][0]),_mm_loadu_pd(&s[0][0])));
  _mm_storeu_pd(&m[1][0],_mm_sub_pd(_mm_loadu_pd(&m[1][0]),_mm_loadu_pd(&s[1][0])));
#else
  m[0][0]-=s[0][0]; m[0][1]-=s[0][1];
  m[1][0]-=s[1][0]; m[1][1]-=s[1][1];
#endif
  return *this;
  }


// Multiply matrix by scalar
FXMat2d& FXMat2d::operator*=(FXdouble s){
#if defined(FOX_HAS_AVX)
  _mm256_storeu_pd(&m[0][0],_mm256_mul_pd(_mm256_loadu_pd(&m[0][0]),_mm256_set1_pd(s)));
#elif defined(FOX_HAS_SSE2)
  _mm_storeu_pd(&m[0][0],_mm_mul_pd(_mm_loadu_pd(&m[0][0]),_mm_set1_pd(s)));
  _mm_storeu_pd(&m[1][0],_mm_mul_pd(_mm_loadu_pd(&m[1][0]),_mm_set1_pd(s)));
#else
  m[0][0]*=s; m[0][1]*=s;
  m[1][0]*=s; m[1][1]*=s;
#endif
  return *this;
  }


// Multiply matrix by matrix
FXMat2d& FXMat2d::operator*=(const FXMat2d& s){
#if defined(FOX_HAS_SSE2)
  __m128d s0=_mm_loadu_pd(&s[0][0]);
  __m128d s1=_mm_loadu_pd(&s[1][0]);
  _mm_storeu_pd(&m[0][0],_mm_add_pd(_mm_mul_pd(_mm_set1_pd(m[0][0]),s0),_mm_mul_pd(_mm_set1_pd(m[0][1]),s1)));
  _mm_storeu_pd(&m[1][0],_mm_add_pd(_mm_mul_pd(_mm_set1_pd(m[1][0]),s0),_mm_mul_pd(_mm_set1_pd(m[1][1]),s1)));
#else
  FXdouble m00=m[0][0],m01=m[0][1],m10=m[1][0],m11=m[1][1];
  m[0][0]=m00*s[0][0]+m01*s[1][0];
  m[0][1]=m00*s[0][1]+m01*s[1][1];
  m[1][0]=m10*s[0][0]+m11*s[1][0];
  m[1][1]=m10*s[0][1]+m11*s[1][1];
#endif
  return *this;
  }


// Divide matrix by scalar
FXMat2d& FXMat2d::operator/=(FXdouble s){
#if defined(FOX_HAS_AVX)
  _mm256_storeu_pd(&m[0][0],_mm256_div_pd(_mm256_loadu_pd(&m[0][0]),_mm256_set1_pd(s)));
#elif defined(FOX_HAS_SSE2)
  _mm_storeu_pd(&m[0][0],_mm_div_pd(_mm_loadu_pd(&m[0][0]),_mm_set1_pd(s)));
  _mm_storeu_pd(&m[1][0],_mm_div_pd(_mm_loadu_pd(&m[1][0]),_mm_set1_pd(s)));
#else
  m[0][0]/=s; m[0][1]/=s;
  m[1][0]/=s; m[1][1]/=s;
#endif
  return *this;
  }


// Negate matrix
FXMat2d FXMat2d::operator-() const {
#if defined(FOX_HAS_AVX)
  FXMat2d r;
  _mm256_storeu_pd(&r[0][0],_mm256_sub_pd(_mm256_set1_pd(0.0),_mm256_loadu_pd(&m[0][0])));
  return r;
#elif defined(FOX_HAS_SSE2)
  FXMat2d r;
  _mm_storeu_pd(&r[0][0],_mm_sub_pd(_mm_set1_pd(0.0),_mm_loadu_pd(&m[0][0])));
  _mm_storeu_pd(&r[1][0],_mm_sub_pd(_mm_set1_pd(0.0),_mm_loadu_pd(&m[1][0])));
  return r;
#else
  return FXMat2d(-m[0][0],-m[0][1],-m[1][0],-m[1][1]);
#endif
  }


// Set to identity matrix
FXMat2d& FXMat2d::identity(){
#if defined(FOX_HAS_AVX)
  _mm256_storeu_pd(&m[0][0],_mm256_set_pd(1.0,0.0,0.0,1.0));
#elif defined(FOX_HAS_SSE2)
  _mm_storeu_pd(&m[0][0],_mm_set_pd(0.0,1.0));
  _mm_storeu_pd(&m[1][0],_mm_set_pd(1.0,0.0));
#else
  m[0][0]=1.0; m[0][1]=0.0;
  m[1][0]=0.0; m[1][1]=1.0;
#endif
  return *this;
  }


// Return true if identity matrix
FXbool FXMat2d::isIdentity() const {
  return m[0][0]==1.0 && m[0][1]==0.0 && m[1][0]==0.0 && m[1][1]==1.0;
  }


// Rotate by cosine, sine
FXMat2d& FXMat2d::rot(FXdouble c,FXdouble s){
#if defined(FOX_HAS_SSE2)
  __m128d cc=_mm_set1_pd(c);
  __m128d ss=_mm_set1_pd(s);
  __m128d uu=_mm_loadu_pd(&m[0][0]);
  __m128d vv=_mm_loadu_pd(&m[1][0]);
  _mm_storeu_pd(&m[0][0],_mm_add_pd(_mm_mul_pd(cc,uu),_mm_mul_pd(ss,vv)));
  _mm_storeu_pd(&m[1][0],_mm_sub_pd(_mm_mul_pd(cc,vv),_mm_mul_pd(ss,uu)));
#else
  FXdouble u,v;
  u=m[0][0]; v=m[1][0]; m[0][0]=c*u+s*v; m[1][0]=c*v-s*u;
  u=m[0][1]; v=m[1][1]; m[0][1]=c*u+s*v; m[1][1]=c*v-s*u;
#endif
  return *this;
  }


// Rotate by angle
FXMat2d& FXMat2d::rot(FXdouble phi){
  return rot(Math::cos(phi),Math::sin(phi));
  }


// Scale unqual
FXMat2d& FXMat2d::scale(FXdouble sx,FXdouble sy){
#if defined(FOX_HAS_AVX)
  _mm256_storeu_pd(&m[0][0],_mm256_mul_pd(_mm256_loadu_pd(&m[0][0]),_mm256_set_pd(sy,sy,sx,sx)));
#elif defined(FOX_HAS_SSE2)
  _mm_storeu_pd(&m[0][0],_mm_mul_pd(_mm_loadu_pd(&m[0][0]),_mm_set_pd(sx,sx)));
  _mm_storeu_pd(&m[1][0],_mm_mul_pd(_mm_loadu_pd(&m[1][0]),_mm_set_pd(sy,sy)));
#else
  m[0][0]*=sx; m[0][1]*=sx;
  m[1][0]*=sy; m[1][1]*=sy;
#endif
  return *this;
  }


// Scale uniform
FXMat2d& FXMat2d::scale(FXdouble s){
#if defined(FOX_HAS_AVX)
  _mm256_storeu_pd(&m[0][0],_mm256_mul_pd(_mm256_loadu_pd(&m[0][0]),_mm256_set1_pd(s)));
#elif defined(FOX_HAS_SSE2)
  _mm_storeu_pd(&m[0][0],_mm_mul_pd(_mm_loadu_pd(&m[0][0]),_mm_set1_pd(s)));
  _mm_storeu_pd(&m[1][0],_mm_mul_pd(_mm_loadu_pd(&m[1][0]),_mm_set1_pd(s)));
#else
  m[0][0]*=s; m[0][1]*=s;
  m[1][0]*=s; m[1][1]*=s;
#endif
  return *this;
  }


// Calculate determinant
FXdouble FXMat2d::det() const {
  return m[0][0]*m[1][1]-m[0][1]*m[1][0];
  }


// Transpose matrix
FXMat2d FXMat2d::transpose() const {
  return FXMat2d(m[0][0],m[1][0],m[0][1],m[1][1]);
  }


// Invert matrix
FXMat2d FXMat2d::invert() const {
  FXdouble dd=m[0][0]*m[1][1]-m[0][1]*m[1][0];
  return FXMat2d(m[1][1]/dd,-m[0][1]/dd,-m[1][0]/dd,m[0][0]/dd);
  }


// Orthogonalize matrix
// Uses Gram-Schmidt orthogonalization on a row-by-row basis
FXMat2d orthogonalize(const FXMat2d& m){
  FXMat2d result(m);
  result[0]/=result[0].length();
  result[1]-=result[0]*(result[1]*result[0]);
  result[1]/=result[1].length();
  return result;
  }


// Matrix times vector
FXVec2d operator*(const FXMat2d& m,const FXVec2d& v){
#if defined(FOX_HAS_SSE3)
  __m128d vv=_mm_loadu_pd(v);
  __m128d r0=_mm_mul_pd(_mm_loadu_pd(&m[0][0]),vv);
  __m128d r1=_mm_mul_pd(_mm_loadu_pd(&m[1][0]),vv);
  FXVec2d r;
  _mm_storeu_pd(&r[0],_mm_hadd_pd(r0,r1));
  return r;
#else
  return FXVec2d(m[0][0]*v[0]+m[0][1]*v[1],m[1][0]*v[0]+m[1][1]*v[1]);
#endif
  }


// Vector times matrix
FXVec2d operator*(const FXVec2d& v,const FXMat2d& m){
#if defined(FOX_HAS_SSE2)
  FXVec2d r;
  __m128d r0=_mm_mul_pd(_mm_set1_pd(v[0]),_mm_loadu_pd(&m[0][0]));
  __m128d r1=_mm_mul_pd(_mm_set1_pd(v[1]),_mm_loadu_pd(&m[1][0]));
  _mm_storeu_pd(&r[0],_mm_add_pd(r0,r1));
  return r;
#else
  return FXVec2d(v[0]*m[0][0]+v[1]*m[1][0],v[0]*m[0][1]+v[1]*m[1][1]);
#endif
  }


// Matrix and matrix add
FXMat2d operator+(const FXMat2d& a,const FXMat2d& b){
#if defined(FOX_HAS_AVX)
  FXMat2d r;
  _mm256_storeu_pd(&r[0][0],_mm256_add_pd(_mm256_loadu_pd(&a[0][0]),_mm256_loadu_pd(&b[0][0])));
  return r;
#elif defined(FOX_HAS_SSE2)
  FXMat2d r;
  _mm_storeu_pd(&r[0][0],_mm_add_pd(_mm_loadu_pd(&a[0][0]),_mm_loadu_pd(&b[0][0])));
  _mm_storeu_pd(&r[1][0],_mm_add_pd(_mm_loadu_pd(&a[1][0]),_mm_loadu_pd(&b[1][0])));
  return r;
#else
  return FXMat2d(a[0][0]+b[0][0],a[0][1]+b[0][1],a[1][0]+b[1][0],a[1][1]+b[1][1]);
#endif
  }


// Matrix and matrix subtract
FXMat2d operator-(const FXMat2d& a,const FXMat2d& b){
#if defined(FOX_HAS_AVX)
  FXMat2d r;
  _mm256_storeu_pd(&r[0][0],_mm256_sub_pd(_mm256_loadu_pd(&a[0][0]),_mm256_loadu_pd(&b[0][0])));
  return r;
#elif defined(FOX_HAS_SSE2)
  FXMat2d r;
  _mm_storeu_pd(&r[0][0],_mm_sub_pd(_mm_loadu_pd(&a[0][0]),_mm_loadu_pd(&b[0][0])));
  _mm_storeu_pd(&r[1][0],_mm_sub_pd(_mm_loadu_pd(&a[1][0]),_mm_loadu_pd(&b[1][0])));
  return r;
#else
  return FXMat2d(a[0][0]-b[0][0],a[0][1]-b[0][1],a[1][0]-b[1][0],a[1][1]-b[1][1]);
#endif
  }


// Matrix and matrix multiply
FXMat2d operator*(const FXMat2d& a,const FXMat2d& b){
#if defined(FOX_HAS_SSE2)
  __m128d b0=_mm_loadu_pd(&b[0][0]);
  __m128d b1=_mm_loadu_pd(&b[1][0]);
  FXMat2d r;
  _mm_storeu_pd(&r[0][0],_mm_add_pd(_mm_mul_pd(_mm_set1_pd(a[0][0]),b0),_mm_mul_pd(_mm_set1_pd(a[0][1]),b1)));
  _mm_storeu_pd(&r[1][0],_mm_add_pd(_mm_mul_pd(_mm_set1_pd(a[1][0]),b0),_mm_mul_pd(_mm_set1_pd(a[1][1]),b1)));
  return r;
#else
  return FXMat2d(a[0][0]*b[0][0]+a[0][1]*b[1][0],a[0][0]*b[0][1]+a[0][1]*b[1][1],a[1][0]*b[0][0]+a[1][1]*b[1][0],a[1][0]*b[0][1]+a[1][1]*b[1][1]);
#endif
  }


// Multiply scalar by matrix
FXMat2d operator*(FXdouble x,const FXMat2d& m){
#if defined(FOX_HAS_AVX)
  FXMat2d r;
  _mm256_storeu_pd(&r[0][0],_mm256_mul_pd(_mm256_set1_pd(x),_mm256_loadu_pd(&m[0][0])));
  return r;
#elif defined(FOX_HAS_SSE2)
  FXMat2d r;
  _mm_storeu_pd(&r[0][0],_mm_mul_pd(_mm_set1_pd(x),_mm_loadu_pd(&m[0][0])));
  _mm_storeu_pd(&r[1][0],_mm_mul_pd(_mm_set1_pd(x),_mm_loadu_pd(&m[1][0])));
  return r;
#else
  return FXMat2d(x*m[0][0],x*m[0][1],x*m[1][0],x*m[1][1]);
#endif
  }


// Multiply matrix by scalar
FXMat2d operator*(const FXMat2d& m,FXdouble x){
#if defined(FOX_HAS_AVX)
  FXMat2d r;
  _mm256_storeu_pd(&r[0][0],_mm256_mul_pd(_mm256_loadu_pd(&m[0][0]),_mm256_set1_pd(x)));
  return r;
#elif defined(FOX_HAS_SSE2)
  FXMat2d r;
  _mm_storeu_pd(&r[0][0],_mm_mul_pd(_mm_loadu_pd(&m[0][0]),_mm_set1_pd(x)));
  _mm_storeu_pd(&r[1][0],_mm_mul_pd(_mm_loadu_pd(&m[1][0]),_mm_set1_pd(x)));
  return r;
#else
  return FXMat2d(m[0][0]*x,m[0][1]*x,m[1][0]*x,m[1][1]*x);
#endif
  }


// Divide scalar by matrix
FXMat2d operator/(FXdouble x,const FXMat2d& m){
#if defined(FOX_HAS_AVX)
  FXMat2d r;
  _mm256_storeu_pd(&r[0][0],_mm256_div_pd(_mm256_set1_pd(x),_mm256_loadu_pd(m[0])));
  return r;
#elif defined(FOX_HAS_SSE2)
  FXMat2d r;
  _mm_storeu_pd(&r[0][0],_mm_div_pd(_mm_set1_pd(x),_mm_loadu_pd(&m[0][0])));
  _mm_storeu_pd(&r[1][0],_mm_div_pd(_mm_set1_pd(x),_mm_loadu_pd(&m[1][0])));
  return r;
#else
  return FXMat2d(x/m[0][0],x/m[0][1],x/m[1][0],x/m[1][1]);
#endif
  }


// Divide matrix by scalar
FXMat2d operator/(const FXMat2d& m,FXdouble x){
#if defined(FOX_HAS_AVX)
  FXMat2d r;
  _mm256_storeu_pd(&r[0][0],_mm256_div_pd(_mm256_loadu_pd(&m[0][0]),_mm256_set1_pd(x)));
  return r;
#elif defined(FOX_HAS_SSE2)
  FXMat2d r;
  _mm_storeu_pd(&r[0][0],_mm_div_pd(_mm_loadu_pd(&m[0][0]),_mm_set1_pd(x)));
  _mm_storeu_pd(&r[1][0],_mm_div_pd(_mm_loadu_pd(&m[1][0]),_mm_set1_pd(x)));
  return r;
#else
  return FXMat2d(m[0][0]/x,m[0][1]/x,m[1][0]/x,m[1][1]/x);
#endif
  }


// Matrix and matrix equality
FXbool operator==(const FXMat2d& a,const FXMat2d& b){
  return a[0]==b[0] && a[1]==b[1];
  }


// Matrix and matrix inequality
FXbool operator!=(const FXMat2d& a,const FXMat2d& b){
  return a[0]!=b[0] || a[1]!=b[1];
  }


// Matrix and scalar equality
FXbool operator==(const FXMat2d& a,FXdouble n){
  return a[0]==n && a[1]==n;
  }


// Matrix and scalar inequality
FXbool operator!=(const FXMat2d& a,FXdouble n){
  return a[0]!=n || a[1]!=n;
  }


// Scalar and matrix equality
FXbool operator==(FXdouble n,const FXMat2d& a){
  return n==a[0] && n==a[1];
  }


// Scalar and matrix inequality
FXbool operator!=(FXdouble n,const FXMat2d& a){
  return n!=a[0] || n!=a[1];
  }


// Save to archive
FXStream& operator<<(FXStream& store,const FXMat2d& m){
  store << m[0] << m[1]; return store;
  }


// Load from archive
FXStream& operator>>(FXStream& store,FXMat2d& m){
  store >> m[0] >> m[1]; return store;
  }

}
