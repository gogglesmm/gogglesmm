/********************************************************************************
*                                                                               *
*                           L o c a l e   C l a s s                             *
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
#include "fxascii.h"
#include "FXArray.h"
#include "FXHash.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXLocale.h"



/*
  Notes:

  - Format of locale:

      language[_territory][.codeset][@modifier]

  - When looking, some parts may be dropped in the following order:

      1 codeset
      2 normalized codeset
      3 territory
      4 modifier

  - Info from "Unicode Common Locale Data Repository," http://cldr.unicode.org,
    and http://www.unicode.org/reports/tr35/.
  - Formatting of dates, times, and time zones.
  - Formatting numbers and currency values.
  - Sorting text
  - Choosing languages or countries by name.
*/


using namespace FX;

/*******************************************************************************/

namespace FX {


FXLocale::FXLocale(){
  }


FXLocale::~FXLocale(){
  }

}

