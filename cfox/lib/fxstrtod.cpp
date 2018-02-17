/********************************************************************************
*                                                                               *
*               S t r i n g   t o   D o u b l e   C o n v e r s i o n           *
*                                                                               *
*********************************************************************************
* Copyright (C) 2005,2017 by Jeroen van der Zijp.   All Rights Reserved.        *
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

#define MAXDIGS  20     // Maximum number of significant digits

/*******************************************************************************/

using namespace FX;

namespace FX {


extern FXAPI FXdouble __strtod(const FXchar *beg,const FXchar** end=NULL,FXbool* ok=NULL);
extern FXAPI FXfloat __strtof(const FXchar *beg,const FXchar** end=NULL,FXbool* ok=NULL);

// Some magick
const union{ FXulong u; FXdouble f; } inf={FXULONG(0x7ff0000000000000)};
const union{ FXulong u; FXdouble f; } nan={FXULONG(0x7fffffffffffffff)};

// Convert string to double
FXdouble __strtod(const FXchar *beg,const FXchar** end,FXbool* ok){
  register const FXchar *s=beg;
  register FXdouble value=0.0;
  register FXdouble mult=1.0;
  register FXint exponent=-1;
  register FXint digits=0;
  register FXint expo=0;
  register FXint neg;
  register FXint nex;

  // Assume the worst
  if(ok) *ok=false;

  // No characters consumed
  if(end) *end=s;

  // Skip whitespace
  while(Ascii::isSpace(s[0])){
    ++s;
    }

  // Handle sign
  if((neg=(*s=='-')) || (*s=='+')){
    ++s;
    }

  // We see start of a number
  if(*s=='.' || ('0'<=*s && *s<='9')){

    // Leading zeros
    while(*s=='0'){
      ++s;
      }

    // Read the mantissa
    while('0'<=*s && *s<='9'){
      if(digits<MAXDIGS){
        value+=(*s-'0')*mult;
        mult*=0.1;
        }
      ++exponent;
      ++digits;
      ++s;
      }
    if(*s=='.'){
      ++s;
      while('0'<=*s && *s<='9'){
        if(digits<MAXDIGS){
          value+=(*s-'0')*mult;
          mult*=0.1;
          }
        digits++;
        ++s;
        }
      }

    // Characters consumed so far
    if(end) *end=s;

    // Try exponent
    if((*s|0x20)=='e'){
      ++s;

      // Handle optional exponent sign
      if((nex=(*s=='-')) || (*s=='+')){
        ++s;
        }

      // Got an exponent
      if('0'<=*s && *s<='9'){

        // Eat exponent digits
        while('0'<=*s && *s<='9'){
          expo=expo*10+(*s-'0');
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
        if((308<exponent) || (1.79769313486231570815<=value)){
          return neg ? -1.79769313486231570815E+308 : 1.79769313486231570815E+308;      // Sensible value, but not OK
          }
        }

      // Check for denormal or underflow
      if(exponent<-308){
        if((exponent<-324) || ((exponent==-324) && (value<=4.94065645841246544177))){
          if(ok) *ok=true;
          return 0.0;                                                                   // Flush to zero, and OK
          }
        value*=1.0E-16;         // Fix number
        exponent+=16;
        }

      // In range
      value*=Math::ipow(10.0,exponent);

      // Adjust sign
      if(neg){
        value=-value;
        }
      }
    if(ok) *ok=true;            // OK
    }

  // Infinite
  else if(*s=='i' || *s=='I'){
    s++;
    if(*s=='n' || *s=='N'){
      s++;
      if(*s=='f' || *s=='F'){
        s++;
        value=neg?-inf.f:inf.f;
        if(end) *end=s;
        if(ok) *ok=true;        // OK
        }
      }
    }

  // NaN
  else if(*s=='n' || *s=='N'){
    s++;
    if(*s=='a' || *s=='A'){
      s++;
      if(*s=='n' || *s=='N'){
        s++;
        value=neg?-nan.f:nan.f;
        if(end) *end=s;
        if(ok) *ok=true;        // OK
        }
      }
    }
  return value;
  }


// Convert string to unsigned int
FXfloat __strtof(const FXchar* beg,const FXchar** end,FXbool* ok){
  register FXdouble value=__strtod(beg,end,ok);
  if(__unlikely(value<-FLT_MAX)){
    if(ok) *ok=false;
    return -FLT_MAX;
    }
  if(__unlikely(value>FLT_MAX)){
    if(ok) *ok=false;
    return FLT_MAX;
    }
  return (FXfloat)value;
  }

}
