/********************************************************************************
*                                                                               *
*               S t r i n g   t o   D o u b l e   C o n v e r s i o n           *
*                                                                               *
*********************************************************************************
* Copyright (C) 2005,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "fxascii.h"


/*
  Notes:
  - Thread-safe conversion of strings to double and float, with extra parameter
    for conversion success.
  - Checks both overflow and underflow. Note that characters are consumed in case of
    overflow or underflow, but OK flag is still false when the condition is raised.
  - Accuracy is in the order of 16 digits; this is the best one can do with doubles.
  - We only work with the first 20 significants digits; processing more will not
    increase accuracy and may cause trouble.
  - When compiled with FTZ/DAZ (-ffast-math) or equivalent, results from conversions
    will be less accurate!
*/


using namespace FX;

/*******************************************************************************/

namespace FX {

extern FXAPI FXdouble __strtod(const FXchar *beg,const FXchar** end=nullptr,FXbool* ok=nullptr);
extern FXAPI FXfloat __strtof(const FXchar *beg,const FXchar** end=nullptr,FXbool* ok=nullptr);


// Some magick
static const union{ FXulong u; FXdouble f; } dblinf={FXULONG(0x7ff0000000000000)};
static const union{ FXulong u; FXdouble f; } dblnan={FXULONG(0x7fffffffffffffff)};


// Maximum number of significant digits
const FXint MAXDIGS=20;


// Convert string to double
FXdouble __strtod(const FXchar *beg,const FXchar** end,FXbool* ok){
  const FXchar *s=beg;
  FXdouble value=0.0;
  FXdouble mult=1.0;
  FXint exponent=-1;
  FXint digits=0;
  FXint expo=0;
  FXint neg;
  FXint nex;

  // Assume the worst
  if(ok) *ok=false;

  // No characters consumed
  if(end) *end=s;

  // Skip whitespace
  while(Ascii::isSpace(s[0])){
    ++s;
    }

  // Handle sign
  if((neg=(s[0]=='-')) || (s[0]=='+')){
    ++s;
    }

  // We see start of a number
  if(s[0]=='.' || ('0'<=s[0] && s[0]<='9')){

    // Leading zeros
    while(s[0]=='0'){
      ++s;
      }

    // Parse significant digits
    while('0'<=s[0] && s[0]<='9'){
      if(digits<MAXDIGS){
        value+=(s[0]-'0')*mult;
        mult*=0.1;
        }
      ++exponent;
      ++digits;
      ++s;
      }

    // Decimal point
    if(s[0]=='.'){
      ++s;

      // Leading zeros; adjust exponent
      if(digits==0){
        while(s[0]=='0'){
          --exponent;
          ++s;
          }
        }

      // Parse significant digits
      while('0'<=s[0] && s[0]<='9'){
        if(digits<MAXDIGS){
          value+=(s[0]-'0')*mult;
          mult*=0.1;
          }
        ++digits;
        ++s;
        }
      }

    // Characters consumed so far
    if(end) *end=s;

    // Try exponent
    if((s[0]|0x20)=='e'){
      ++s;

      // Handle optional exponent sign
      if((nex=(s[0]=='-')) || (s[0]=='+')){
        ++s;
        }

      // Got an exponent
      if('0'<=s[0] && s[0]<='9'){

        // Eat exponent digits
        while('0'<=s[0] && s[0]<='9'){
          expo=expo*10+(s[0]-'0');
          ++s;
          }

        // Update exponent
        if(nex){
          exponent-=expo;
          }
        else{
          exponent+=expo;
          }

        // Consumed exponent as well
        if(end) *end=s;
        }
      }

    // Elaborate checks only if non-zero
    if(value!=0.0){

      // Check for overflow
      if(308<=exponent){

        // Sensible value, but not OK
        if((308<exponent) || (1.79769313486231570815<=value)){
          return neg ? -1.79769313486231570815E+308 : 1.79769313486231570815E+308;
          }
        }

      // Check for denormal or underflow
      if(exponent<-308){

        // Flush to zero, and OK
        if((exponent<-324) || ((exponent==-324) && (value<=4.94065645841246544177))){
          if(ok) *ok=true;
          return 0.0;
          }

        // Fix number
        value*=1.0E-16;
        exponent+=16;
        }

      // Exponent in range
      value*=Math::pow10i(exponent);

      // Adjust sign
      if(neg){
        value=-value;
        }
      }
    if(ok) *ok=true;
    }

  // Infinite
  else if((s[0]|0x20)=='i'){
    ++s;
    if((s[0]|0x20)=='n'){
      ++s;
      if((s[0]|0x20)=='f'){
        value=neg?-dblinf.f:dblinf.f;
        if(end){                // Return end of recognized number
          if((s[1]|0x20)=='i' && (s[2]|0x20)=='n' && (s[3]|0x20)=='i' && (s[4]|0x20)=='t' && (s[5]|0x20)=='y') s+=5;
          *end=++s;
          }
        if(ok) *ok=true;
        }
      }
    }

  // NaN
  else if((s[0]|0x20)=='n'){
    ++s;
    if((s[0]|0x20)=='a'){
      ++s;
      if((s[0]|0x20)=='n'){
        value=neg?-dblnan.f:dblnan.f;
        if(end){                // Return end of recognized number
          *end=++s;
          }
        if(ok) *ok=true;
        }
      }
    }
  return value;
  }


// Convert string to float
FXfloat __strtof(const FXchar* beg,const FXchar** end,FXbool* ok){
  const FXdouble DBL_MINIMUM=(FXdouble)-FLT_MAX;
  const FXdouble DBL_MAXIMUM=(FXdouble) FLT_MAX;
  FXdouble value=__strtod(beg,end,ok);
  if(__unlikely(value<DBL_MINIMUM)){
    if(ok) *ok=false;
    return -FLT_MAX;
    }
  if(__unlikely(value>DBL_MAXIMUM)){
    if(ok) *ok=false;
    return FLT_MAX;
    }
  return (FXfloat)value;
  }

}
