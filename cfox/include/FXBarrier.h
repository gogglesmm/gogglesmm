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
#ifndef FXBARRIER_H
#define FXBARRIER_H


namespace FX {


/**
* Barrier with changeable threshold.
* A Barrier holds back threads arriving at the barrier, until the
* number of threads exceeds some threshold (quorum), after which all
* threads can progress.
* The Barrier can be broken in two additional ways. First, the threshold
* value could be changed to a number lower than the currently waiting
* number of threads.  Second, the waiting threads could be released
* unconditionally, without reaching quorum.
* Since there must be at least one thread, the theshold value should
* be at least 1.
* Deleting the barrier while threads are blocked in not allowed.
*/
class FXAPI FXBarrier {
private:
  FXCondition     condition;
  FXMutex         mutex;
  volatile FXuint generation;
  volatile FXuint thresh;
  volatile FXuint count;
private:
  FXBarrier(const FXBarrier&);
  FXBarrier &operator=(const FXBarrier&);
public:

  /// Initialize the barrier with initial threshold thr
  FXBarrier(FXuint thr=1);

  /**
  * Change threshold to thr, possibly releasing all waiting threads.
  * Return true if current count now exceeds the threshold.
  */
  FXbool threshold(FXuint thr);

  /**
  * Return the threshold, i.e. number at which the barrier
  * breaks through.
  */
  FXuint threshold() const { return thresh; }

  /**
  * Wait for all threads to hit the barrier.
  * Returns true for one thread, false for all other threads.
  */
  FXbool wait();

  /**
  * Release all waiting threads unconditionally; all waiting threads
  * will break through the barrier and proceed, even if the threshold
  * was not exceeded.
  * Return true if any threads were waiting.
  */
  FXbool release();

  /// Delete the barrier
 ~FXBarrier();
  };


}

#endif

