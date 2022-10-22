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
#ifndef FXCOMPLETION_H
#define FXCOMPLETION_H


namespace FX {


class FXThreadPool;

/**
* A completion counter allows a single thread to monitor a number of ongoing
* concurrent activities for completion.  Each activity calls increment() on the
* completion counter when started, and decrement() when finished.
* The monitoring thread can call wait(), wait(nsec), or done() to determine
* the status of the concurrent activity.
* Only one thread is allowed to call any of the wait() functions; but multiple
* threads can call increment() or decrement().
* The completion counter can not be destroyed until the last thread decrements
* the counter back down to zero.
*/
class FXAPI FXCompletion {
private:
  FXSemaphore     semaphore;    // Signalled when last task done
  volatile FXuint counter;      // Task counter
private:
  FXCompletion(const FXCompletion&);
  FXCompletion& operator=(const FXCompletion&);
public:

  /// Initialize completion counter
  FXCompletion();

  /// Return current counter value
  FXuint count() const { return counter; }

  /// Increment counter by cnt
  void increment(FXuint cnt=1);

  /// Decrement counter by cnt
  void decrement(FXuint cnt=1);

  /// Wait till count becomes zero again
  void wait();

  /// Wait till count becomes zero, or timeout; return false if timed out
  FXbool wait(FXTime nsec);

  /// Return true if count is zero
  FXbool done() const { return (counter==0); }

  /// Wait till count becomes zero, then destroy
 ~FXCompletion();
  };

}

#endif

