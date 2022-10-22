/********************************************************************************
*                                                                               *
*         P a r s e   G e o m e t r y   F r o m   C o m m a n d   L i n e       *
*                                                                               *
*********************************************************************************
* Copyright (C) 2002,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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


/*
  Notes:
  - Programmer must call this to parse geometry from command
    line parameter "-g" or "--geometry"; first initialize input/output
    arguments to their default values, the ones parsed will be
    overridden.
*/

using namespace FX;

/*******************************************************************************/

namespace FX {

// Parse string of the form:
//
//      [=][<width>{xX}<height>][{+-}<xoffset>{+-}<yoffset>]
//
// Return or of flags:
//
//      1       x was assigned
//      2       y was assigned
//      4       width was assigned
//      8       height was assigned
FXint fxparsegeometry(const FXchar *string,FXint& x,FXint& y,FXint& w,FXint& h){
  FXint result=0;

  // Got string?
  if(string && *string!='\0'){
    FXint tw=0;
    FXint th=0;
    FXint tx=0;
    FXint ty=0;
    FXint mul;

    // Parse white space
    while(*string==' ' || *string=='\t'){
      string++;
      }

    // Skip leading '=', if any
    if(*string=='=') string++;

    // Start with width
    if(*string!='+' && *string!='-' && *string!='x' && *string!='X'){
      while('0'<=*string && *string<='9'){
        tw=tw*10+(*string++-'0');
        result|=4;                      // At least one digit found
        }
      }

    // Then height
    if(*string=='x' || *string=='X'){
      string++;
      while('0'<=*string && *string<='9'){
        th=th*10+(*string++-'0');
        result|=8;                      // At least one digit found
        }
      }

    // Then x
    if(*string=='+' || *string== '-'){
      if(*string++ == '-') mul=-1; else mul=1;
      while('0'<=*string && *string<='9'){
        tx=tx*10+(*string++-'0');
        result|=1;                      // At least one digit found
        }
      tx*=mul;
      }

    // Then y
    if(*string=='+' || *string == '-'){
      if(*string++ == '-') mul=-1; else mul=1;
      while('0'<=*string && *string<='9'){
        ty=ty*10+(*string++-'0');
        result|=2;                      // At least one digit found
        }
      ty*=mul;
      }

    // Parse white space
    while(*string==' ' || *string=='\t'){
      string++;
      }

    // Got the whole string
    if(*string=='\0'){

      // Return what was found
      if(result&1) x=tx;
      if(result&2) y=ty;
      if(result&4) w=tw;
      if(result&8) h=th;
      }
    }
  return result;
  }

}
