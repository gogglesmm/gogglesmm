/********************************************************************************
*                                                                               *
*       S i n g l e - P r e c i s i o n   3 - E l e m e n t   V e c t o r       *
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


using namespace FX;

/*******************************************************************************/

namespace FX {


// Mask bottom 3 elements
#define MMM     _mm_set_epi32(0,~0,~0,~0)


#if defined(FOX_HAS_AVX2)

// Convert from vector to color
FXColor colorFromVec3f(const FXVec3f& vec){
  FXColor res;

  // Scale and convert to integer:      00000000 000000BB 000000GG 000000RR
  __m128i uuuu=_mm_cvtps_epi32(_mm_mul_ps(_mm_maskload_ps(&vec[0],MMM),_mm_set1_ps(255.0f)));

  // Shuffle to lower 4 bytes:          RRRRRRRR RRRRRRRR RRRRRRRR 00RRGGBB
  __m128i bbbb=_mm_shuffle_epi8(uuuu,_mm_set_epi8(0,0,0,0, 0,0,0,0, 0,0,0,0, 12,0,4,8));

  // Assign to output
  res=_mm_cvtsi128_si32(bbbb);

  // Set alpha to opaque
  res|=FXRGBA(0,0,0,255);
  return res;
  }


// Convert from color to vector
FXVec3f colorToVec3f(FXColor clr){
  FXVec3f res;

  // Shuffle into place, zero the rest: 000000AA 000000BB 000000GG 000000RR
  __m128i uuuu=_mm_shuffle_epi8(_mm_cvtsi32_si128(clr),_mm_set_epi8(8,8,8,3, 8,8,8,0, 8,8,8,1, 8,8,8,2));

  // Convert to float and scale:        AAAAAAAA BBBBBBBB GGGGGGGG RRRRRRRR
  __m128 ffff=_mm_mul_ps(_mm_cvtepi32_ps(uuuu),_mm_set1_ps(0.003921568627f));

  // Assign to output
  _mm_maskstore_ps(&res[0],MMM,ffff);
  return res;
  }

#else

// Convert from vector to color
FXColor colorFromVec3f(const FXVec3f& vec){
  return FXRGB((vec.x*255.0f+0.5f),(vec.y*255.0f+0.5f),(vec.z*255.0f+0.5f));
  }


// Convert from color to vector
FXVec3f colorToVec3f(FXColor clr){
  return FXVec3f(0.003921568627f*FXREDVAL(clr),0.003921568627f*FXGREENVAL(clr),0.003921568627f*FXBLUEVAL(clr));
  }

#endif


// Compute fast cross product with vector code
FXVec3f cross(const FXVec3f& u,const FXVec3f& v){
#if defined(FOX_HAS_AVX)
  __m128 uu=_mm_maskload_ps(&u[0],MMM);
  __m128 vv=_mm_maskload_ps(&v[0],MMM);
  __m128 a0=_mm_shuffle_ps(uu,uu,_MM_SHUFFLE(3,0,2,1));
  __m128 b0=_mm_shuffle_ps(vv,vv,_MM_SHUFFLE(3,1,0,2));
  __m128 a1=_mm_shuffle_ps(uu,uu,_MM_SHUFFLE(3,1,0,2));
  __m128 b1=_mm_shuffle_ps(vv,vv,_MM_SHUFFLE(3,0,2,1));
  FXVec3f r;
  _mm_maskstore_ps(&r[0],MMM,_mm_sub_ps(_mm_mul_ps(a0,b0),_mm_mul_ps(a1,b1)));
  return r;
#else
  FXVec3f r;
  r.x=u.y*v.z - u.z*v.y;
  r.y=u.z*v.x - u.x*v.z;
  r.z=u.x*v.y - u.y*v.x;
  return r;
#endif
  }


// Compute fast dot product with vector code
FXfloat dot(const FXVec3f& u,const FXVec3f& v){
#if defined(FOX_HAS_AVX)
  __m128 uu=_mm_maskload_ps(&u[0],MMM);
  __m128 vv=_mm_maskload_ps(&v[0],MMM);
  return _mm_cvtss_f32(_mm_dp_ps(uu,vv,0x71));
#else
  return u*v;
#endif
  }


// Normalize vector
FXVec3f normalize(const FXVec3f& v){
  FXfloat m=v.length();
  if(__likely(m)){ return v/m; }
  return v;
  }


// Return vector orthogonal to v
FXVec3f orthogonal(const FXVec3f& v){
  FXVec3f result(0.0f,0.0f,0.0f);
  FXfloat x=Math::fabs(v.x);
  FXfloat y=Math::fabs(v.y);
  FXfloat z=Math::fabs(v.z);
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
FXVec3f normal(const FXVec3f& a,const FXVec3f& b,const FXVec3f& c){
  return normalize((b-a)^(c-a));
  }


// Compute approximate normal from four points a,b,c,d
FXVec3f normal(const FXVec3f& a,const FXVec3f& b,const FXVec3f& c,const FXVec3f& d){
  return normalize((c-a)^(d-b));
  }


// Rotate vector vec by unit-length axis about angle specified as (ca,sa)
FXVec3f rotate(const FXVec3f& vec,const FXVec3f& axis,FXfloat ca,FXfloat sa){
  FXVec3f v1((vec*axis)*axis);
  FXVec3f v2(axis^vec);
  FXVec3f v3(vec-v1);
  return v1+v2*sa+v3*ca;
  }


// Rotate vector by unit-length axis about angle ang
FXVec3f rotate(const FXVec3f& vector,const FXVec3f& axis,FXfloat ang){
  return rotate(vector,axis,Math::cos(ang),Math::sin(ang));
  }


// Save vector to stream
FXStream& operator<<(FXStream& store,const FXVec3f& v){
  store << v.x << v.y << v.z;
  return store;
  }


// Load vector from stream
FXStream& operator>>(FXStream& store,FXVec3f& v){
  store >> v.x >> v.y >> v.z;
  return store;
  }

}
