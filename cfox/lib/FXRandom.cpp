/********************************************************************************
*                                                                               *
*                  R a n d o m   N u m b e r   G e n e r a t o r                *
*                                                                               *
*********************************************************************************
* Copyright (C) 2007,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "FXRandom.h"


/*
  Notes:
  - Algorithm based on Numerical Recipes, 3ed, pp. 351-352.
  - Original algorithm George Marsaglia, "Random number generators",
    Journal of Modern Applied Statistical Methods 2, No. 2, 2003.
  - Different shift-counts are certainly possible, and produce different
    sequences (of the same period), see G. Marsaglia, "Xorshift RNGs".
  - New shift counts based on the paper "An experimental exploration of
    Marsaglia's xorshift generators, scrambled", Sebastiano Vigna, 2014.
*/

#define SHIFTA    12
#define SHIFTB    25
#define SHIFTC    27

using namespace FX;


namespace FX {

/*******************************************************************************/

// Construct random generator with default seed
FXRandom::FXRandom():state(FXULONG(4101842887655102017)){
  }


// Construct random generator with given seed s
FXRandom::FXRandom(FXulong s):state(FXULONG(4101842887655102017)^s){
  }


// Initialize seed
void FXRandom::seed(FXulong s){
  state=s^FXULONG(4101842887655102017);
  }


// Generate next state
FXulong FXRandom::next(){
  state^=state>>SHIFTA;
  state^=state<<SHIFTB;
  state^=state>>SHIFTC;
  return state;
  }


// Get random long
FXulong FXRandom::randLong(){
  return next()*FXULONG(2685821657736338717);
  }


// Get random double
FXfloat FXRandom::randFloat(){
  FXlong num=(FXlong)randLong();
  return Math::fabs(num*1.0842021724855044340074528008699e-19f);
  }


// Get random double
FXdouble FXRandom::randDouble(){
  FXlong num=(FXlong)randLong();
  return Math::fabs(num*1.0842021724855044340074528008699e-19);
  }

}



