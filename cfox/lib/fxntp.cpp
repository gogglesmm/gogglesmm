/********************************************************************************
*                                                                               *
*                                 N T P - T i m e                               *
*                                                                               *
*********************************************************************************
* Copyright (C) 2019,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "FXSystem.h"


/*
  Notes:

  - Conversions between Network Time Protocol (NTP) time and Unix Epoch Time.
*/


using namespace FX;

/*******************************************************************************/

namespace FX {

// Offset from Epoch to 1-Jan-1900 @ 00:00:00 UTC
const FXTime JANUARY1900=-FXLONG(2208988800000000000);

// Offset from Epoch to 7-Feb-2036 @ 06:28:16 UTC
const FXTime FEBRUARY2036=+FXLONG(2085978496000000000);


// Convert NTP format (ssss:ffff) to nanoseconds since Unix Epoch
FXTime FXSystem::timeFromNTPTime(FXulong ntptime){
  FXTime secs=(ntptime>>32)&0xFFFFFFFF;
  FXTime frac=ntptime-(secs<<32);
  FXTime nano=(frac*1000000000+2147483647)>>32;
  return (secs&0x80000000) ? (secs*1000000000+nano+JANUARY1900) : (secs*1000000000+nano+FEBRUARY2036);
  }


// Convert utc in nanoseconds since Unix Epoch to NTP (ssss:ffff)
FXulong FXSystem::ntpTimeFromTime(FXTime utc){
  FXlong base=utc-FEBRUARY2036;
  FXlong secs=(0<=base ? base : base-999999999)/1000000000;
  FXlong nano=base-secs*1000000000;
  FXlong frac=((nano<<32)+500000000)/1000000000;
  return (secs<<32)+frac;
  }

}

