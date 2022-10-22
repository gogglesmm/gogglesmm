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
#include "FXMat2f.h"
#include "FXMat3f.h"


/*
  Notes:
*/

using namespace FX;

/*******************************************************************************/

namespace FX {


// Eliminate move by using single-source shuffle (SSE2)
#define _mm_shufd_ps(xmm,mask) _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(xmm),mask))


// Initialize matrix from scalar
FXMat2f::FXMat2f(FXfloat s){
#if defined(FOX_HAS_SSE)
  _mm_storeu_ps(&m[0][0],_mm_set1_ps(s));
#else
  m[0][0]=s; m[0][1]=s;
  m[1][0]=s; m[1][1]=s;
#endif
  }


// Initialize matrix from another matrix
FXMat2f::FXMat2f(const FXMat2f& s){
#if defined(FOX_HAS_SSE)
  _mm_storeu_ps(&m[0][0],_mm_loadu_ps(&s[0][0]));
#else
  m[0][0]=s[0][0]; m[0][1]=s[0][1];
  m[1][0]=s[1][0]; m[1][1]=s[1][1];
#endif
  }


// Initialize from rotation and scaling part of 3x3 matrix
FXMat2f::FXMat2f(const FXMat3f& s){
#if defined(FOX_HAS_SSE)
  _mm_storeu_ps(&m[0][0],_mm_set_ps(s[1][1],s[1][0],s[0][1],s[0][0]));
#else
  m[0][0]=s[0][0]; m[0][1]=s[0][1];
  m[1][0]=s[1][0]; m[1][1]=s[1][1];
#endif
  }


// Initialize matrix from array
FXMat2f::FXMat2f(const FXfloat s[]){
#if defined(FOX_HAS_SSE)
  _mm_storeu_ps(&m[0][0],_mm_loadu_ps(s));
#else
  m[0][0]=s[0]; m[0][1]=s[1];
  m[1][0]=s[2]; m[1][1]=s[3];
#endif
  }


// Initialize diagonal matrix
FXMat2f::FXMat2f(FXfloat a,FXfloat b){
  m[0][0]=a;    m[0][1]=0.0f;
  m[1][0]=0.0f; m[1][1]=b;
  }


// Initialize matrix from components
FXMat2f::FXMat2f(FXfloat a00,FXfloat a01,FXfloat a10,FXfloat a11){
#if defined(FOX_HAS_SSE)
  _mm_storeu_ps(&m[0][0],_mm_set_ps(a11,a10,a01,a00));
#else
  m[0][0]=a00; m[0][1]=a01;
  m[1][0]=a10; m[1][1]=a11;
#endif
  }


// Initialize matrix from two vectors
FXMat2f::FXMat2f(const FXVec2f& a,const FXVec2f& b){
#if defined(FOX_HAS_SSE)
  _mm_storeu_ps(&m[0][0],_mm_set_ps(b[1],b[0],a[1],a[0]));
#else
  m[0]=a;
  m[1]=b;
#endif
  }


// Assign from scalar
FXMat2f& FXMat2f::operator=(FXfloat s){
#if defined(FOX_HAS_SSE)
  _mm_storeu_ps(&m[0][0],_mm_set1_ps(s));
#else
  m[0][0]=s; m[0][1]=s;
  m[1][0]=s; m[1][1]=s;
#endif
  return *this;
  }


// Assignment operator
FXMat2f& FXMat2f::operator=(const FXMat2f& s){
#if defined(FOX_HAS_SSE)
  _mm_storeu_ps(&m[0][0],_mm_loadu_ps(&s[0][0]));
#else
  m[0][0]=s[0][0]; m[0][1]=s[0][1];
  m[1][0]=s[1][0]; m[1][1]=s[1][1];
#endif
  return *this;
  }


// Assign from rotation and scaling part of 3x3 matrix
FXMat2f& FXMat2f::operator=(const FXMat3f& s){
#if defined(FOX_HAS_SSE)
  _mm_storeu_ps(&m[0][0],_mm_set_ps(s[1][1],s[1][0],s[0][1],s[0][0]));
#else
  m[0][0]=s[0][0]; m[0][1]=s[0][1];
  m[1][0]=s[1][0]; m[1][1]=s[1][1];
#endif
  return *this;
  }


// Assignment from array
FXMat2f& FXMat2f::operator=(const FXfloat s[]){
#if defined(FOX_HAS_SSE)
  _mm_storeu_ps(&m[0][0],_mm_loadu_ps(s));
#else
  m[0][0]=s[0]; m[0][1]=s[1];
  m[1][0]=s[2]; m[1][1]=s[3];
#endif
  return *this;
  }


// Set value from scalar
FXMat2f& FXMat2f::set(FXfloat s){
#if defined(FOX_HAS_SSE)
  _mm_storeu_ps(&m[0][0],_mm_set1_ps(s));
#else
  m[0][0]=s; m[0][1]=s;
  m[1][0]=s; m[1][1]=s;
#endif
  return *this;
  }


// Set value from another matrix
FXMat2f& FXMat2f::set(const FXMat2f& s){
#if defined(FOX_HAS_SSE)
  _mm_storeu_ps(&m[0][0],_mm_loadu_ps(&s[0][0]));
#else
  m[0][0]=s[0][0]; m[0][1]=s[0][1];
  m[1][0]=s[1][0]; m[1][1]=s[1][1];
#endif
  return *this;
  }


// Set from rotation and scaling part of 3x3 matrix
FXMat2f& FXMat2f::set(const FXMat3f& s){
#if defined(FOX_HAS_SSE)
  _mm_storeu_ps(&m[0][0],_mm_set_ps(s[1][1],s[1][0],s[0][1],s[0][0]));
#else
  m[0][0]=s[0][0]; m[0][1]=s[0][1];
  m[1][0]=s[1][0]; m[1][1]=s[1][1];
#endif
  return *this;
  }


// Set value from array
FXMat2f& FXMat2f::set(const FXfloat s[]){
#if defined(FOX_HAS_SSE)
  _mm_storeu_ps(&m[0][0],_mm_loadu_ps(s));
#else
  m[0][0]=s[0]; m[0][1]=s[1];
  m[1][0]=s[2]; m[1][1]=s[3];
#endif
  return *this;
  }


// Set diagonal matrix
FXMat2f& FXMat2f::set(FXfloat a,FXfloat b){
  m[0][0]=a;    m[0][1]=0.0f;
  m[1][0]=0.0f; m[1][1]=b;
  return *this;
  }


// Set value from components
FXMat2f& FXMat2f::set(FXfloat a00,FXfloat a01,FXfloat a10,FXfloat a11){
  m[0][0]=a00; m[0][1]=a01;
  m[1][0]=a10; m[1][1]=a11;
  return *this;
  }


// Set value from two vectors
FXMat2f& FXMat2f::set(const FXVec2f& a,const FXVec2f& b){
#if defined(FOX_HAS_SSE)
  _mm_storeu_ps(&m[0][0],_mm_set_ps(b[1],b[0],a[1],a[0]));
#else
  m[0]=a;
  m[1]=b;
#endif
  return *this;
  }


// Add matrices
FXMat2f& FXMat2f::operator+=(const FXMat2f& s){
#if defined(FOX_HAS_SSE)
  _mm_storeu_ps(&m[0][0],_mm_add_ps(_mm_loadu_ps(&m[0][0]),_mm_loadu_ps(&s[0][0])));
#else
  m[0][0]+=s[0][0]; m[0][1]+=s[0][1];
  m[1][0]+=s[1][0]; m[1][1]+=s[1][1];
#endif
  return *this;
  }


// Subtract matrices
FXMat2f& FXMat2f::operator-=(const FXMat2f& s){
#if defined(FOX_HAS_SSE)
  _mm_storeu_ps(&m[0][0],_mm_sub_ps(_mm_loadu_ps(&m[0][0]),_mm_loadu_ps(&s[0][0])));
#else
  m[0][0]-=s[0][0]; m[0][1]-=s[0][1];
  m[1][0]-=s[1][0]; m[1][1]-=s[1][1];
#endif
  return *this;
  }


// Multiply matrix by scalar
FXMat2f& FXMat2f::operator*=(FXfloat s){
#if defined(FOX_HAS_SSE)
  _mm_storeu_ps(&m[0][0],_mm_mul_ps(_mm_loadu_ps(&m[0][0]),_mm_set1_ps(s)));
#else
  m[0][0]*=s; m[0][1]*=s;
  m[1][0]*=s; m[1][1]*=s;
#endif
  return *this;
  }


// Multiply matrix by matrix
FXMat2f& FXMat2f::operator*=(const FXMat2f& s){
#if defined(FOX_HAS_SSE2)
  __m128 m0=_mm_loadu_ps(&m[0][0]);
  __m128 ss=_mm_loadu_ps(&s[0][0]);
  __m128 m1=_mm_shufd_ps(m0,_MM_SHUFFLE(2,3,0,1));
  __m128 s0=_mm_shufd_ps(ss,_MM_SHUFFLE(3,0,3,0));
  __m128 s1=_mm_shufd_ps(ss,_MM_SHUFFLE(1,2,1,2));
  _mm_storeu_ps(&m[0][0],_mm_add_ps(_mm_mul_ps(m0,s0),_mm_mul_ps(m1,s1)));
#else
  FXfloat m00=m[0][0],m01=m[0][1],m10=m[1][0],m11=m[1][1];
  m[0][0]=m00*s[0][0]+m01*s[1][0];
  m[0][1]=m00*s[0][1]+m01*s[1][1];
  m[1][0]=m10*s[0][0]+m11*s[1][0];
  m[1][1]=m10*s[0][1]+m11*s[1][1];
#endif
  return *this;
  }


// Divide matrix by scalar
FXMat2f& FXMat2f::operator/=(FXfloat s){
#if defined(FOX_HAS_SSE)
  _mm_storeu_ps(&m[0][0],_mm_div_ps(_mm_loadu_ps(&m[0][0]),_mm_set1_ps(s)));
#else
  m[0][0]/=s; m[0][1]/=s;
  m[1][0]/=s; m[1][1]/=s;
#endif
  return *this;
  }


// Negate matrix
FXMat2f FXMat2f::operator-() const {
#if defined(FOX_HAS_SSE)
  FXMat2f r;
  _mm_storeu_ps(&r[0][0],_mm_sub_ps(_mm_set1_ps(0.0f),_mm_loadu_ps(&m[0][0])));
  return r;
#else
  return FXMat2f(-m[0][0],-m[0][1],-m[1][0],-m[1][1]);
#endif
  }


// Set to identity matrix
FXMat2f& FXMat2f::identity(){
#if defined(FOX_HAS_SSE)
  _mm_storeu_ps(&m[0][0],_mm_set_ps(1.0f,0.0f,0.0f,1.0f));
#else
  m[0][0]=1.0f; m[0][1]=0.0f;
  m[1][0]=0.0f; m[1][1]=1.0f;
#endif
  return *this;
  }


// Return true if identity matrix
FXbool FXMat2f::isIdentity() const {
  return m[0][0]==1.0f && m[0][1]==0.0f && m[1][0]==0.0f && m[1][1]==1.0f;
  }


// Rotate by cosine, sine
FXMat2f& FXMat2f::rot(FXfloat c,FXfloat s){
#if defined(FOX_HAS_SSE2)
  __m128 cc=_mm_set1_ps(c);
  __m128 ss=_mm_set1_ps(s);
  __m128 uv=_mm_loadu_ps(&m[0][0]);
  __m128 vu=_mm_shufd_ps(uv,_MM_SHUFFLE(1,0,3,2));
  _mm_storeu_ps(&m[0][0],_mm_add_ps(_mm_mul_ps(cc,uv),_mm_xor_ps(_mm_mul_ps(ss,vu),_mm_castsi128_ps(_mm_set_epi32(0x80000000,0x80000000,0,0)))));
#else
  FXfloat u,v;
  u=m[0][0]; v=m[1][0]; m[0][0]=c*u+s*v; m[1][0]=c*v-s*u;
  u=m[0][1]; v=m[1][1]; m[0][1]=c*u+s*v; m[1][1]=c*v-s*u;
#endif
  return *this;
  }


// Rotate by angle
FXMat2f& FXMat2f::rot(FXfloat phi){
  return rot(Math::cos(phi),Math::sin(phi));
  }


// Scale unqual
FXMat2f& FXMat2f::scale(FXfloat sx,FXfloat sy){
#if defined(FOX_HAS_SSE)
  _mm_storeu_ps(&m[0][0],_mm_mul_ps(_mm_loadu_ps(&m[0][0]),_mm_set_ps(sy,sy,sx,sx)));
#else
  m[0][0]*=sx; m[0][1]*=sx;
  m[1][0]*=sy; m[1][1]*=sy;
#endif
  return *this;
  }


// Scale uniform
FXMat2f& FXMat2f::scale(FXfloat s){
#if defined(FOX_HAS_SSE)
  _mm_storeu_ps(&m[0][0],_mm_mul_ps(_mm_loadu_ps(&m[0][0]),_mm_set1_ps(s)));
#else
  m[0][0]*=s; m[0][1]*=s;
  m[1][0]*=s; m[1][1]*=s;
#endif
  return *this;
  }


// Calculate determinant
FXfloat FXMat2f::det() const {
  return m[0][0]*m[1][1]-m[0][1]*m[1][0];
  }


// Transpose matrix
FXMat2f FXMat2f::transpose() const {
#if defined(FOX_HAS_SSE2)
  FXMat2f r;
  __m128 mm=_mm_loadu_ps(&m[0][0]);
  _mm_storeu_ps(&r[0][0],_mm_shufd_ps(mm,_MM_SHUFFLE(3,1,2,0)));
  return r;
#else
  return FXMat2f(m[0][0],m[1][0],m[0][1],m[1][1]);
#endif
  }


// Invert matrix
FXMat2f FXMat2f::invert() const {
  FXfloat dd=m[0][0]*m[1][1]-m[0][1]*m[1][0];
  return FXMat2f(m[1][1]/dd,-m[0][1]/dd,-m[1][0]/dd,m[0][0]/dd);
  }


// Orthogonalize matrix
// Uses Gram-Schmidt orthogonalization on a row-by-row basis
FXMat2f orthogonalize(const FXMat2f& m){
  FXMat2f result(m);
  result[0]/=result[0].length();
  result[1]-=result[0]*(result[1]*result[0]);
  result[1]/=result[1].length();
  return result;
  }


// Matrix times vector
FXVec2f operator*(const FXMat2f& m,const FXVec2f& v){
#if defined(FOX_HAS_SSE3)
  __m128 mm=_mm_loadu_ps(&m[0][0]);
  __m128 vv=_mm_castsi128_ps(_mm_set1_epi64(*((const __m64*)&v[0])));
  __m128 rr=_mm_mul_ps(mm,vv);
  FXVec2f r;
  _mm_storel_pi((__m64*)&r[0],_mm_hadd_ps(rr,rr));
  return r;
#else
  return FXVec2f(m[0][0]*v[0]+m[0][1]*v[1],m[1][0]*v[0]+m[1][1]*v[1]);
#endif
  }


// Vector times matrix
FXVec2f operator*(const FXVec2f& v,const FXMat2f& m){
#if defined(FOX_HAS_SSE3)
  __m128 mm=_mm_loadu_ps(&m[0][0]);
  __m128 vv=_mm_castsi128_ps(_mm_set1_epi64(*((const __m64*)&v[0])));
  __m128 rr=_mm_mul_ps(_mm_shufd_ps(mm,_MM_SHUFFLE(3,1,2,0)),vv);
  FXVec2f r;
  _mm_storel_pi((__m64*)&r[0],_mm_hadd_ps(rr,rr));
  return r;
#else
  return FXVec2f(v[0]*m[0][0]+v[1]*m[1][0],v[0]*m[0][1]+v[1]*m[1][1]);
#endif
  }


// Matrix and matrix add
FXMat2f operator+(const FXMat2f& a,const FXMat2f& b){
#if defined(FOX_HAS_SSE)
  FXMat2f r;
  _mm_storeu_ps(&r[0][0],_mm_add_ps(_mm_loadu_ps(&a[0][0]),_mm_loadu_ps(&b[0][0])));
  return r;
#else
  return FXMat2f(a[0][0]+b[0][0],a[0][1]+b[0][1],a[1][0]+b[1][0],a[1][1]+b[1][1]);
#endif
  }


// Matrix and matrix subtract
FXMat2f operator-(const FXMat2f& a,const FXMat2f& b){
#if defined(FOX_HAS_SSE)
  FXMat2f r;
  _mm_storeu_ps(&r[0][0],_mm_sub_ps(_mm_loadu_ps(&a[0][0]),_mm_loadu_ps(&b[0][0])));
  return r;
#else
  return FXMat2f(a[0][0]-b[0][0],a[0][1]-b[0][1],a[1][0]-b[1][0],a[1][1]-b[1][1]);
#endif
  }


// Matrix and matrix multiply
FXMat2f operator*(const FXMat2f& a,const FXMat2f& b){
#if defined(FOX_HAS_SSE2)
  __m128 m0=_mm_loadu_ps(&a[0][0]);
  __m128 ww=_mm_loadu_ps(&b[0][0]);
  __m128 m1=_mm_shufd_ps(m0,_MM_SHUFFLE(2,3,0,1));
  __m128 w0=_mm_shufd_ps(ww,_MM_SHUFFLE(3,0,3,0));
  __m128 w1=_mm_shufd_ps(ww,_MM_SHUFFLE(1,2,1,2));
  FXMat2f r;
  _mm_storeu_ps(&r[0][0],_mm_add_ps(_mm_mul_ps(m0,w0),_mm_mul_ps(m1,w1)));
  return r;
#else
  return FXMat2f(a[0][0]*b[0][0]+a[0][1]*b[1][0],a[0][0]*b[0][1]+a[0][1]*b[1][1],a[1][0]*b[0][0]+a[1][1]*b[1][0],a[1][0]*b[0][1]+a[1][1]*b[1][1]);
#endif
  }


// Multiply scalar by matrix
FXMat2f operator*(FXfloat x,const FXMat2f& m){
#if defined(FOX_HAS_SSE)
  FXMat2f r;
  _mm_storeu_ps(&r[0][0],_mm_mul_ps(_mm_set1_ps(x),_mm_loadu_ps(&m[0][0])));
  return r;
#else
  return FXMat2f(x*m[0][0],x*m[0][1],x*m[1][0],x*m[1][1]);
#endif
  }


// Multiply matrix by scalar
FXMat2f operator*(const FXMat2f& m,FXfloat x){
#if defined(FOX_HAS_SSE)
  FXMat2f r;
  _mm_storeu_ps(&r[0][0],_mm_mul_ps(_mm_loadu_ps(&m[0][0]),_mm_set1_ps(x)));
  return r;
#else
  return FXMat2f(m[0][0]*x,m[0][1]*x,m[1][0]*x,m[1][1]*x);
#endif
  }


// Divide scalar by matrix
FXMat2f operator/(FXfloat x,const FXMat2f& m){
#if defined(FOX_HAS_SSE)
  FXMat2f r;
  _mm_storeu_ps(&r[0][0],_mm_div_ps(_mm_set1_ps(x),_mm_loadu_ps(&m[0][0])));
  return r;
#else
  return FXMat2f(x/m[0][0],x/m[0][1],x/m[1][0],x/m[1][1]);
#endif
  }


// Divide matrix by scalar
FXMat2f operator/(const FXMat2f& m,FXfloat x){
#if defined(FOX_HAS_SSE)
  FXMat2f r;
  _mm_storeu_ps(&r[0][0],_mm_div_ps(_mm_loadu_ps(&m[0][0]),_mm_set1_ps(x)));
  return r;
#else
  return FXMat2f(m[0][0]/x,m[0][1]/x,m[1][0]/x,m[1][1]/x);
#endif
  }


// Matrix and matrix equality
FXbool operator==(const FXMat2f& a,const FXMat2f& b){
  return a[0]==b[0] && a[1]==b[1];
  }


// Matrix and matrix inequality
FXbool operator!=(const FXMat2f& a,const FXMat2f& b){
  return a[0]!=b[0] || a[1]!=b[1];
  }


// Matrix and scalar equality
FXbool operator==(const FXMat2f& a,FXfloat n){
  return a[0]==n && a[1]==n;
  }


// Matrix and scalar inequality
FXbool operator!=(const FXMat2f& a,FXfloat n){
  return a[0]!=n || a[1]!=n;
  }


// Scalar and matrix equality
FXbool operator==(FXfloat n,const FXMat2f& a){
  return n==a[0] && n==a[1];
  }


// Scalar and matrix inequality
FXbool operator!=(FXfloat n,const FXMat2f& a){
  return n!=a[0] || n!=a[1];
  }


// Save to archive
FXStream& operator<<(FXStream& store,const FXMat2f& m){
  store << m[0] << m[1]; return store;
  }


// Load from archive
FXStream& operator>>(FXStream& store,FXMat2f& m){
  store >> m[0] >> m[1]; return store;
  }

}
