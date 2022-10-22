/********************************************************************************
*                                                                               *
*                           D i s p l a y   C l a s s                           *
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
#ifndef FXDISPLAY_H
#define FXDISPLAY_H

namespace FX {


/**
* Display class.
*/
class FXAPI FXDisplay {
private:
  FXptr   display;      // Display
private:
  FXDisplay(const FXDisplay&);
  FXDisplay &operator=(const FXDisplay&);
public:

  /// Construct display
  FXDisplay();

  /// Destroy display
 ~FXDisplay();
  };

}

#endif

