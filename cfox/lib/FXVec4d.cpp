/********************************************************************************
*                                                                               *
*       D o u b l e - P r e c i s i o n   4 - E l e m e n t   V e c t o r       *
*                                                                               *
*********************************************************************************
* Copyright (C) 1994,2017 by Jeroen van der Zijp.   All Rights Reserved.        *
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


using namespace FX;

/*******************************************************************************/

namespace FX {


// Convert from vector to color
FXColor colorFromVec4d(const FXVec4d& vec){
  return FXRGBA((vec.x*255.0+0.5),(vec.y*255.0+0.5),(vec.z*255.0+0.5),(vec.w*255.0+0.5));
  }


// Convert from color to vector
FXVec4d colorToVec4d(FXColor clr){
  return FXVec4d(0.003921568627*FXREDVAL(clr),0.003921568627*FXGREENVAL(clr),0.003921568627*FXBLUEVAL(clr),0.003921568627*FXALPHAVAL(clr));
  }


// Normalize vector
FXVec4d normalize(const FXVec4d& v){
  FXdouble m=v.length2();
  FXVec4d result(v);
  if(__likely(0.0<m)){ result/=Math::sqrt(m); }
  return result;
  }


// Compute plane equation from 3 points a,b,c
FXVec4d plane(const FXVec3d& a,const FXVec3d& b,const FXVec3d& c){
  FXVec3d nm(normal(a,b,c));
  return FXVec4d(nm,-(nm.x*a.x+nm.y*a.y+nm.z*a.z));
  }


// Compute plane equation from vector and distance
FXVec4d plane(const FXVec3d& vec,FXdouble dist){
  FXVec3d nm(normalize(vec));
  return FXVec4d(nm,-dist);
  }


// Compute plane equation from vector and point on plane
FXVec4d plane(const FXVec3d& vec,const FXVec3d& p){
  FXVec3d nm(normalize(vec));
  return FXVec4d(nm,-(nm.x*p.x+nm.y*p.y+nm.z*p.z));
  }


// Compute plane equation from 4 vector
FXVec4d plane(const FXVec4d& vec){
  register FXdouble t=Math::sqrt(vec.x*vec.x+vec.y*vec.y+vec.z*vec.z);
  return FXVec4d(vec.x/t,vec.y/t,vec.z/t,vec.w/t);
  }


// Signed distance normalized plane and point
FXdouble FXVec4d::distance(const FXVec3d& p) const {
  return x*p.x+y*p.y+z*p.z+w;
  }


// Return true if edge a-b crosses plane
FXbool FXVec4d::crosses(const FXVec3d& a,const FXVec3d& b) const {
  return (distance(a)>=0.0) ^ (distance(b)>=0.0);
  }


// Linearly interpolate
FXVec4d lerp(const FXVec4d& u,const FXVec4d& v,FXdouble f){
#if defined(FOX_HAS_AVX2) && defined(FOX_HAS_FMA)
  register __m256d u0=_mm256_loadu_pd(&u[0]);
  register __m256d v0=_mm256_loadu_pd(&v[0]);
  register __m256d ff=_mm256_set1_pd(f);
  FXVec4d r;
  _mm256_storeu_pd(&r[0],_mm256_fmadd_pd(ff,v0,_mm256_fnmadd_pd(ff,u0,u0)));       // Lerp in two instructions!
  return r;
#elif defined(FOX_HAS_AVX)
  register __m256d u0=_mm256_loadu_pd(&u[0]);
  register __m256d v0=_mm256_loadu_pd(&v[0]);
  register __m256d ff=_mm256_set1_pd(f);
  FXVec4d r;
  _mm256_storeu_pd(&r[0],_mm256_add_pd(u0,_mm256_mul_pd(_mm256_sub_pd(v0,u0),ff)));
  return r;
#elif defined(FOX_HAS_SSE2)
  register __m128d u0=_mm_loadu_pd(&u[0]);
  register __m128d u1=_mm_loadu_pd(&u[2]);
  register __m128d v0=_mm_loadu_pd(&v[0]);
  register __m128d v1=_mm_loadu_pd(&v[2]);
  register __m128d ff=_mm_set1_pd(f);
  FXVec4d r;
  _mm_storeu_pd(&r[0],_mm_add_pd(u0,_mm_mul_pd(_mm_sub_pd(v0,u0),ff)));
  _mm_storeu_pd(&r[2],_mm_add_pd(u1,_mm_mul_pd(_mm_sub_pd(v1,u1),ff)));
  return r;
#else
  return FXVec4d(u.x+(v.x-u.x)*f,u.y+(v.y-u.y)*f,u.z+(v.z-u.z)*f,u.w+(v.w-u.w)*f);
#endif
  }


// Save vector to stream
FXStream& operator<<(FXStream& store,const FXVec4d& v){
  store << v.x << v.y << v.z << v.w;
  return store;
  }


// Load vector from stream
FXStream& operator>>(FXStream& store,FXVec4d& v){
  store >> v.x >> v.y >> v.z >> v.w;
  return store;
  }

}
