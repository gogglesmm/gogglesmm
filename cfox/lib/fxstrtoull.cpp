/********************************************************************************
*                                                                               *
*      S t r i n g   t o   U n s i g n e d   L o n g   C o n v e r s i o n      *
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
  - Thread-safe conversion of strings to unsigned long and int, with extra parameter
    'ok' for conversion success.
*/


// Largest unsigned long value
#ifndef ULLONG_MAX
#define ULLONG_MAX FXULONG(18446744073709551615)
#endif

using namespace FX;

/*******************************************************************************/

namespace FX {

extern FXAPI FXulong __strtoull(const FXchar* beg,const FXchar** end=nullptr,FXint base=0,FXbool* ok=nullptr);
extern FXAPI FXuint __strtoul(const FXchar* beg,const FXchar** end=nullptr,FXint base=0,FXbool* ok=nullptr);


// Convert string to unsigned long
FXulong __strtoull(const FXchar *beg,const FXchar** end,FXint base,FXbool* ok){
  const FXchar *s=beg;
  FXulong cutoff=ULLONG_MAX;
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
    while(Ascii::isSpace(s[0])){
      s++;
      }

    // Handle sign
    if(*s=='-'){
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
        value=ULLONG_MAX;
        }
      else{
        if(neg) value=-(FXlong)value;
        if(ok) *ok=true;
        }
      }
    }
  return value;
  }


// Convert string to unsigned int
FXuint __strtoul(const FXchar* beg,const FXchar** end,FXint base,FXbool* ok){
  FXulong value=__strtoull(beg,end,base,ok);
  if(__unlikely(value>UINT_MAX)){
    if(ok) *ok=false;
    return UINT_MAX;
    }
  return (FXuint)value;
  }

}
