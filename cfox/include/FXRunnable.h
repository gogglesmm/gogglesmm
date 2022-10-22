/********************************************************************************
*                                                                               *
*                          R u n n a b l e   C l a s s                          *
*                                                                               *
*********************************************************************************
* Copyright (C) 2004,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#ifndef FXRUNNABLE_H
#define FXRUNNABLE_H

namespace FX {


/**
* FXRunnable represents a generic runnable thing.  It serves primarily
* as a base class for FXThread and tasks in FXThreadPool.
* FXRunnable must be subclassed to reimplement an overloaded run() function,
* which is typically invoked by some thread.
*/
class FXAPI FXRunnable {
public:

  /// Construct a runnable
  FXRunnable(){}

  /// Subclasses should overload this function to perform actual work
  virtual FXint run() = 0;

  /// Destroy a runnable
  virtual ~FXRunnable(){}
  };

}

#endif

