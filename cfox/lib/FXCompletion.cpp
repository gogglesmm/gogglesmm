/********************************************************************************
*                                                                               *
*                     C o m p l e t i o n   C o u n t e r                       *
*                                                                               *
*********************************************************************************
* Copyright (C) 2014,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "FXAtomic.h"
#include "FXSemaphore.h"
#include "FXCompletion.h"

/*
  Notes:

  - Completion allows a single thread to monitor a number of ongoing concurrent
    activities for completion.

  - When an activity is started, the counter is incremented.  If this was the first
    activity, the monitoring semaphore will be decremented to zero.  Starting
    subsequent activities will increment the count but leave the semaphore alone.

  - When an activity is finished, the counter is decremented; if this was the last
    activity, the semaphore will be incremented, causing the thread that called
    wait to unblock, and subsequently reset the semaphore for next time.

  - A semaphore is set to 1 when no activities are in progress; a call to wait()
    will immediately succeed in that case; the semaphore is automatically reset to
    1 after examination by the calling thread.

  - Note that the first activity being started may have to wait until the calling
    thread from a previous activity completion check has returned from the wait()
    call.

  - Don't block in wait() or wait(nsec) if counter was zero already; no need to
    test semaphore in trywait(), since counter tells all.

  - A completion should NOT go out of scope until counter becomes zero again, since
    other activities are still outstanding.
*/

using namespace FX;


namespace FX {


/*******************************************************************************/

// Initialize completion counter
FXCompletion::FXCompletion():semaphore(1),counter(0){
  }


// Increment counter
void FXCompletion::increment(FXuint cnt){
  if(atomicAdd(&counter,cnt)==0 && cnt){semaphore.wait();}
  }


// Decrement counter
void FXCompletion::decrement(FXuint cnt){
  if(atomicAdd(&counter,0-cnt)==cnt && cnt){semaphore.post();}
  }


// Wait till complete
void FXCompletion::wait(){
  if(counter){
    semaphore.wait();
    semaphore.post();
    }
  }


// Wait till complete or timeout; return false if timed out
FXbool FXCompletion::wait(FXTime nsec){
  if(counter){
    if(!semaphore.wait(nsec)) return false;
    semaphore.post();
    }
  return true;
  }


// Delete completion counter
FXCompletion::~FXCompletion(){
  wait();
  }

}
