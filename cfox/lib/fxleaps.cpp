/********************************************************************************
*                                                                               *
*                                T i m e   S t u f f                            *
*                                                                               *
*********************************************************************************
* Copyright (C) 2019,2020 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "FXString.h"
#include "FXSystem.h"


/*
  Notes:
  - Return number of leap seconds since Unix Epoch.
  - Leap seconds date may be obtained from International Earth Rotation
    System (www.iers.org), Bulletin C.  At the time of this writing, 
    https://datacenter.iers.org/data/16/bulletinc-059.txt.
  - List of leap seconds is obtained from IETF,
    https://www.ietf.org/timezones/data/leap-seconds.list.
  - When new leap seconds are announced by IERS, leapsecond table must be
    appended with a new entry; please notify jeroen@fox-toolkit.com if new
    data is available but not yet incorporated into "leapseconds.h".
*/


using namespace FX;


/*******************************************************************************/

namespace FX {

#include "leapseconds.h"

// Return leap seconds since time
FXival FXSystem::leapSeconds(FXTime ns){
  FXival l=0,h=ARRAYNUMBER(leapseconds)-1,m;
  if(leapseconds[h]<=ns) return h+10;
  if(ns<leapseconds[l]) return 0;
  while(l<(m=(h+l)>>1)){
    if(leapseconds[m]<=ns) l=m; else h=m;
    }
  return l+10;
  }

}

