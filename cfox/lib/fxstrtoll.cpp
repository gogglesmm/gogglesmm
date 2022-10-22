/********************************************************************************
*                                                                               *
*        S t r i n g   t o   S i g n e d   L o n g   C o n v e r s i o n        *
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
#include "FXString.h"


/*
  Notes:
  - Thread-safe conversion of strings to signed long and int, with extra parameter
    'ok' for conversion success.
*/


// Largest and smallest unsigned long value
#ifndef LLONG_MAX
#define LLONG_MAX  FXLONG(9223372036854775807)
#endif
#ifndef LLONG_MIN
#define LLONG_MIN  (-LLONG_MAX-FXLONG(1))
#endif


using namespace FX;

/*******************************************************************************/

namespace FX {

extern FXAPI FXlong __strtoll(const FXchar *beg,const FXchar** end=nullptr,FXint base=0,FXbool* ok=nullptr);
extern FXAPI FXint __strtol(const FXchar *beg,const FXchar** end=nullptr,FXint base=0,FXbool* ok=nullptr);


// Convert string to signed long
FXlong __strtoll(const FXchar *beg,const FXchar** end,FXint base,FXbool* ok){
  const FXchar *s=beg;
  FXulong cutoff=LLONG_MAX;
  FXulong value=0;
  FXint cutlim;
  FXint digits=0;
  FXint neg=0;
  FXint ovf=0;
  FXint v;

  // Assume the worst
  if(ok) *ok=false;

  // No characters consumed
  if(end) *end=s;

  // Keep it reasonable
  if(base<=36){

    // Skip whitespace
    while(Ascii::isSpace(*s)){
      s++;
      }

    // Handle sign
    if(*s=='-'){
      cutoff++;           // One higher:- 2's complement
      neg=1;
      s++;
      }
    else if(*s=='+'){
      s++;
      }

    // Hex, binary, or octal start with 0x, 0b, or plain 0
    if(s[0]=='0'){
      if((base<=1 || base==16) && ((s[1]|0x20)=='x')){ base=16; s+=2; }
      else if((base<=1 || base==2) && ((s[1]|0x20)=='b')){ base=2; s+=2; }
      else if(base<=1){ base=8; }
      }

    // Default to decimal base
    else if(base<=1){
      base=10;
      }

    // Overflow check values
    cutlim=cutoff%base;
    cutoff=cutoff/base;

    // Scan digits and aggregate number
    while(0<=(v=Ascii::digitValue(*s)) && v<base){
      if(!ovf){
        if(value>cutoff || (value==cutoff && v>cutlim)) ovf=1;
        value=value*base+v;
        }
      digits++;
      s++;
      }

    // At least one digit was found
    if(0<digits){
      if(end) *end=s;
      if(ovf){
        value=neg?LLONG_MIN:LLONG_MAX;
        }
      else{
        if(neg) value=-(FXlong)value;
        if(ok) *ok=true;
        }
      }
    }
  return (FXlong)value;
  }


// Convert string to signed int
FXint __strtol(const FXchar *beg,const FXchar** end,FXint base,FXbool* ok){
  FXlong value=__strtoll(beg,end,base,ok);
  if(__unlikely(value<INT_MIN)){
    if(ok) *ok=false;
    return INT_MIN;
    }
  if(__unlikely(value>INT_MAX)){
    if(ok) *ok=false;
    return INT_MAX;
    }
  return (FXint)value;
  }

}
