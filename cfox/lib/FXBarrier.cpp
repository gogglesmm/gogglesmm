/********************************************************************************
*                                                                               *
*                             B a r r i e r   C l a s s                         *
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
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "FXMutex.h"
#include "FXCondition.h"
#include "FXBarrier.h"

/*
  Notes:

  - Barrier synchronization primitive.
  - Not using POSIX barriers, since they lack a few important features:

      o Ability to adjust threshold on the fly.
      o Obtain current threshold value.
      o Unconditionally release all threads.

  - Ability to change threshold is necessary to adjust number of threads based on
    working conditions. For example, on busy machine one could reduce number of
    threads working on a parallel problem until system load drops below some level.

  - To release all blocked threads in case a program can't wait for all threads
    to clock.

  - Many scenarios are possible, and often necessary.

*/

using namespace FX;


namespace FX {


/*******************************************************************************/


// Initialize the barrier with initial threshold thr
FXBarrier::FXBarrier(FXuint thr):generation(0),thresh(FXMAX(thr,1)),count(0){
  }


// Wait for all threads to hit the barrier
FXbool FXBarrier::wait(){
  FXScopedMutex locker(mutex);
  FXuint gen=generation;
  if(++count>=thresh){
    count=0;
    generation++;
    condition.broadcast();
    return true;
    }
  while(gen==generation){
    condition.wait(mutex);
    }
  return false;
  }


// Change threshold, possibly releasing all waiting threads
FXbool FXBarrier::threshold(FXuint thr){
  FXScopedMutex locker(mutex);
  thresh=FXMAX(thr,1);
  if(count>=thresh){
    count=0;
    generation++;
    condition.broadcast();
    return true;
    }
  return false;
  }


// Release all waiting threads unconditionally
FXbool FXBarrier::release(){
  FXScopedMutex locker(mutex);
  if(count){
    count=0;
    generation++;
    condition.broadcast();
    return true;
    }
  return false;
  }


// Delete the barrier
FXBarrier::~FXBarrier(){
  }

}
