/********************************************************************************
*                                                                               *
*                    S c o p e d   T h r e a d   S u p p o r t                  *
*                                                                               *
*********************************************************************************
* Copyright (C) 2013,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "FXRunnable.h"
#include "FXAutoThreadStorageKey.h"
#include "FXThread.h"
#include "FXScopedThread.h"

/*
  Notes:

  - Destructor automatically waits until the thread is done.
  - A ScopedThread should probably not be detached!
  - The ScopedThread destructor does not cause the thread to exit;
    some other mechanism should be provided for this purpose.

*/

using namespace FX;


namespace FX {

/*******************************************************************************/

// Initialize thread object
FXScopedThread::FXScopedThread(){
  }


// Wait until the thread is done
FXScopedThread::~FXScopedThread(){
  join();
  }

}
