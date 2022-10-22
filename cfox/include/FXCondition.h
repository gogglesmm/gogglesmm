/********************************************************************************
*                                                                               *
*                         C o n d i t i o n   C l a s s                         *
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
#ifndef FXCONDITION_H
#define FXCONDITION_H


namespace FX {


/**
* A condition allows one or more threads to synchronize
* to an event.  When a thread calls wait, the associated
* mutex is unlocked while the thread is blocked.  When the
* condition becomes signaled, the associated mutex is
* locked and the thread(s) are reawakened.
*/
class FXAPI FXCondition {
private:
  FXuval data[12];
private:
  FXCondition(const FXCondition&);
  FXCondition& operator=(const FXCondition&);
public:

  /// Initialize the condition
  FXCondition();

  /**
  * Wake or unblock a single blocked thread
  */
  void signal();

  /**
  * Wake or unblock all blocked threads
  */
  void broadcast();

  /**
  * Wait until condition becomes signalled, using given mutex,
  * which must already have been locked prior to this call.
  * Return true if the wait ended due to the condition being
  * signalled through signal() or broadcast(), and false if the
  * wait was interrupted or some error occurred.
  */
  FXbool wait(FXMutex& mtx);
  FXbool wait(FXScopedMutex& smx){ return wait(smx.mutex()); }
  FXbool wait(FXReverseMutex& rmx){ return wait(rmx.mutex()); }

  /**
  * Wait until condition becomes signalled, using given mutex,
  * which must already have been locked prior to this call.
  * Return true if the wait ended due to the condition being
  * signalled through signal() or broadcast(), and false if the
  * wait timed out, was interrupted, or some other error occurred.
  * The relative timeout nsec is specified in nanoseconds; if the
  * special value 'forever' is passed, wait indefinitely.
  */
  FXbool wait(FXMutex& mtx,FXTime nsec);
  FXbool wait(FXScopedMutex& smx,FXTime nsec){ return wait(smx.mutex(),nsec); }
  FXbool wait(FXReverseMutex& rmx,FXTime nsec){ return wait(rmx.mutex(),nsec); }

  /// Delete the condition
  ~FXCondition();
  };

}

#endif

