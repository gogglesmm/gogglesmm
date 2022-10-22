/********************************************************************************
*                                                                               *
*                     H a l f - F l o a t   S u p p o r t                       *
*                                                                               *
*********************************************************************************
* Copyright (C) 2008,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#ifndef FXHALF_H
#define FXHALF_H


// Some important numbers
#define HALF_MIN        5.9604644775391E-08     // Smallest half number
#define HALF_MAX        65504.0                 // Largest half number
#define HALF_EPSILON    0.00097656              // Smallest number where (1+e) != 1

namespace FX {


/// Half float (16-bit float)
class FXAPI FXhalf {
private:
  FXushort v;
private:
  static const FXushort fhb[512];
  static const FXuchar  fhs[512];
  static const FXuint   hfm[2048];
  static const FXuint   hfe[64];
  static const FXushort hfw[64];
public:
  FXhalf(){}

  // Initialize half with half
  FXhalf(const FXhalf& h):v(h.v){}

  // Initialize half with float
  FXhalf(FXfloat f){
    union { FXfloat f; FXuint u; } r={f};
    v=fhb[(r.u>>23)&0x1ff]+((r.u&0x007fffff)>>fhs[(r.u>>23)&0x1ff]);
    }

  // Convert half to float
  operator FXfloat() const {
    union { FXuint u; FXfloat f; } r={hfm[hfw[v>>10]+(v&0x3ff)]+hfe[v>>10]};
    return r.f;
    }

  // Test for zero
  FXbool operator!() const { return v==0; }

  // Unary
  FXhalf operator+() const { return *this; }
  FXhalf operator-() const { FXhalf h; h.v=v^0x8000; return h; }

  // Equality
  FXbool operator==(FXhalf h) const { return v==h.v; }
  FXbool operator!=(FXhalf h) const { return v!=h.v; }

  // Assignment
  FXhalf& operator=(FXhalf h){ v=h.v; return *this; }
  FXhalf& operator=(FXfloat f){ *this=FXhalf(f); return *this; }

  // Assignment operators
  FXhalf& operator+=(FXhalf h){ *this=FXhalf(FXfloat(*this)+FXfloat(h)); return *this; }
  FXhalf& operator+=(FXfloat f){ *this=FXhalf(FXfloat(*this)+f); return *this; }

  FXhalf& operator-=(FXhalf h){ *this=FXhalf(FXfloat(*this)-FXfloat(h)); return *this; }
  FXhalf& operator-=(FXfloat f){ *this=FXhalf(FXfloat(*this)-f); return *this; }

  FXhalf& operator*=(FXhalf h){ *this=FXhalf(FXfloat(*this)*FXfloat(h)); return *this; }
  FXhalf& operator*=(FXfloat f){ *this=FXhalf(FXfloat(*this)*f); return *this; }

  FXhalf& operator/=(FXhalf h){ *this=FXhalf(FXfloat(*this)/FXfloat(h)); return *this; }
  FXhalf& operator/=(FXfloat f){ *this=FXhalf(FXfloat(*this)/f); return *this; }
  };


}

#endif
