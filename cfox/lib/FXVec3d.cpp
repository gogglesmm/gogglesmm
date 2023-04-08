/********************************************************************************
*                                                                               *
*       D o u b l e - P r e c i s i o n   3 - E l e m e n t   V e c t o r       *
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


using namespace FX;

/*******************************************************************************/

namespace FX {


// Mask bottom 3 elements
#define MMM _mm256_set_epi64x(0,~0,~0,~0)


#if defined(FOX_HAS_AVX2)

// Convert from vector to color
FXColor colorFromVec3d(const FXVec3d& vec){
  FXColor res;

  // Scale and convert to integer:      00000000 000000BB 000000GG 000000RR
  __m128i uuuu=_mm256_cvtpd_epi32(_mm256_mul_pd(_mm256_maskload_pd(&vec[0],MMM),_mm256_set1_pd(255.0)));

  // Shuffle to lower 4 bytes:          RRRRRRRR RRRRRRRR RRRRRRRR 00RRGGBB
  __m128i bbbb=_mm_shuffle_epi8(uuuu,_mm_set_epi8(0,0,0,0, 0,0,0,0, 0,0,0,0, 12,0,4,8));

  // Assign to output
  res=_mm_cvtsi128_si32(bbbb);

  // Set alpha to opaque
  res|=FXRGBA(0,0,0,255);
  return res;
  }


// Convert from color to vector
FXVec3d colorToVec3d(FXColor clr){
  FXVec3d res;

  // Shuffle into place, zero the rest: 000000AA 000000BB 000000GG 000000RR
  __m128i uuuu=_mm_shuffle_epi8(_mm_cvtsi32_si128(clr),_mm_set_epi8(8,8,8,3, 8,8,8,0, 8,8,8,1, 8,8,8,2));

  // Convert to double and scale:       AAAAAAAA BBBBBBBB GGGGGGGG RRRRRRRR
  __m256d dddd=_mm256_mul_pd(_mm256_cvtepi32_pd(uuuu),_mm256_set1_pd(0.003921568627));

  // Assign to output
  _mm256_maskstore_pd(&res[0],MMM,dddd);
  return res;
  }

#else

// Convert from vector to color
FXColor colorFromVec3d(const FXVec3d& vec){
  return FXRGB((vec.x*255.0+0.5),(vec.y*255.0+0.5),(vec.z*255.0+0.5));
  }


// Convert from color to vector
FXVec3d colorToVec3d(FXColor clr){
  return FXVec3d(0.003921568627*FXREDVAL(clr),0.003921568627*FXGREENVAL(clr),0.003921568627*FXBLUEVAL(clr));
  }

#endif


// Normalize vector
FXVec3d normalize(const FXVec3d& v){
  FXdouble m=v.length();
  if(__likely(m)){ return v/m; }
  return v;
  }


// Return vector orthogonal to v
FXVec3d orthogonal(const FXVec3d& v){
  FXVec3d result(0.0,0.0,0.0);
  FXdouble x=Math::fabs(v.x);
  FXdouble y=Math::fabs(v.y);
  FXdouble z=Math::fabs(v.z);
  if(x<y){
    if(x<z){            // Y,Z largest
      result.y= v.z;    // v x X
      result.z=-v.y;
      }
    else{               // Y, X largest
      result.x= v.y;    // v x Z
      result.y=-v.x;
      }
    }
  else{
    if(y<z){            // X, Z largest
      result.x=-v.z;    // v x Y
      result.z= v.x;
      }
    else{               // X, Y largest
      result.x= v.y;    // v x Z
      result.y=-v.x;
      }
    }
  return result;
  }


// Compute normal from three points a,b,c
FXVec3d normal(const FXVec3d& a,const FXVec3d& b,const FXVec3d& c){
  return normalize((b-a)^(c-a));
  }


// Compute approximate normal from four points a,b,c,d
FXVec3d normal(const FXVec3d& a,const FXVec3d& b,const FXVec3d& c,const FXVec3d& d){
  return normalize((c-a)^(d-b));
  }


// Rotate vector vec by unit-length axis about angle specified as (ca,sa)
FXVec3d rotate(const FXVec3d& vec,const FXVec3d& axis,FXdouble ca,FXdouble sa){
  FXVec3d v1((vec*axis)*axis);
  FXVec3d v2(axis^vec);
  FXVec3d v3(vec-v1);
  return v1+v2*sa+v3*ca;
  }


// Rotate vector vec by unit-length axis about angle ang
FXVec3d rotate(const FXVec3d& vec,const FXVec3d& axis,FXdouble ang){
  return rotate(vec,axis,Math::cos(ang),Math::sin(ang));
  }


// Save vector to stream
FXStream& operator<<(FXStream& store,const FXVec3d& v){
  store << v.x << v.y << v.z;
  return store;
  }


// Load vector from stream
FXStream& operator>>(FXStream& store,FXVec3d& v){
  store >> v.x >> v.y >> v.z;
  return store;
  }

}
