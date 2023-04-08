/********************************************************************************
*                                                                               *
*       D o u b l e - P r e c i s i o n   4 - E l e m e n t   V e c t o r       *
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


using namespace FX;

/*******************************************************************************/

namespace FX {


#if defined(FOX_HAS_AVX2)

// Convert from vector to color
FXColor colorFromVec4d(const FXVec4d& vec){
  FXColor res;

  // Scale and convert to integer:      00000000 000000BB 000000GG 000000RR
  __m128i uuuu=_mm256_cvtpd_epi32(_mm256_mul_pd(_mm256_loadu_pd(&vec[0]),_mm256_set1_pd(255.0)));

  // Shuffle to lower 4 bytes:          RRRRRRRR RRRRRRRR RRRRRRRR 00RRGGBB
  __m128i bbbb=_mm_shuffle_epi8(uuuu,_mm_set_epi8(0,0,0,0, 0,0,0,0, 0,0,0,0, 12,0,4,8));

  // Assign to output
  res=_mm_cvtsi128_si32(bbbb);

  return res;
  }


// Convert from color to vector
FXVec4d colorToVec4d(FXColor clr){
  FXVec4d res;

  // Shuffle into place, zero the rest: 000000AA 000000BB 000000GG 000000RR
  __m128i uuuu=_mm_shuffle_epi8(_mm_cvtsi32_si128(clr),_mm_set_epi8(8,8,8,3, 8,8,8,0, 8,8,8,1, 8,8,8,2));

  // Convert to double and scale:       AAAAAAAA BBBBBBBB GGGGGGGG RRRRRRRR
  __m256d dddd=_mm256_mul_pd(_mm256_cvtepi32_pd(uuuu),_mm256_set1_pd(0.003921568627));

  // Assign to output
  _mm256_storeu_pd(&res[0],dddd);
  return res;
  }


/*
// Normalize vector
FXVec4d normalizeAVX(const FXVec4d& v){
  __m256d dddd=_mm256_loadu_pd(&v[0]);
  __m256d abcd=_mm256_mul_pd(dddd,dddd);                // 4
  __m256d cdab=_mm256_permute2f128_pd(abcd,abcd,1);     // 3
  __m256d pqrs=_mm256_add_pd(abcd,cdab);                // 4
  __m256d qpsr=_mm256_permute_pd(pqrs,5);               // 1
  __m256d rrrr=_mm256_add_pd(pqrs,qpsr);                // 4
  __m256d ssss=_mm256_div_pd(dddd,_mm256_sqrt_pd(rrrr));// 18+14
  FXVec4d res;
  _mm256_storeu_pd(&res[0],ssss);
  return res;
  }
*/

#else

// Convert from vector to color
FXColor colorFromVec4d(const FXVec4d& vec){
  return FXRGBA((vec.x*255.0+0.5),(vec.y*255.0+0.5),(vec.z*255.0+0.5),(vec.w*255.0+0.5));
  }


// Convert from color to vector
FXVec4d colorToVec4d(FXColor clr){
  return FXVec4d(0.003921568627*FXREDVAL(clr),0.003921568627*FXGREENVAL(clr),0.003921568627*FXBLUEVAL(clr),0.003921568627*FXALPHAVAL(clr));
  }

#endif


// Normalize vector
FXVec4d normalize(const FXVec4d& v){
  FXdouble m=v.length();
//  if(__likely(m)){ return v/m; }
//  return v;
  return v/m;
  }


// Compute normalized plane equation ax+by+cz+d=0
FXVec4d plane(const FXVec4d& vec){
  FXdouble t=Math::sqrt(vec.x*vec.x+vec.y*vec.y+vec.z*vec.z);
  return FXVec4d(vec.x/t,vec.y/t,vec.z/t,vec.w/t);
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


// Compute plane equation from 3 points a,b,c
FXVec4d plane(const FXVec3d& a,const FXVec3d& b,const FXVec3d& c){
  FXVec3d nm(normal(a,b,c));
  return FXVec4d(nm,-(nm.x*a.x+nm.y*a.y+nm.z*a.z));
  }


// Signed distance normalized plane and point
FXdouble FXVec4d::distance(const FXVec3d& p) const {
  return x*p.x+y*p.y+z*p.z+w;
  }


// Return true if edge a-b crosses plane
FXbool FXVec4d::crosses(const FXVec3d& a,const FXVec3d& b) const {
  return (distance(a)>=0.0) ^ (distance(b)>=0.0);
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
