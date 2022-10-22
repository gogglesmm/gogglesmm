/********************************************************************************
*                                                                               *
*                          S e m a p h o r e   C l a s s                        *
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
#ifndef FXSEMAPHORE_H
#define FXSEMAPHORE_H

namespace FX {


/**
* A semaphore allows for protection of a resource that can
* be accessed by a fixed number of simultaneous threads.
*
* A typical example of the use of semaphores is for a buffer containing N items.
* A producer thread may freely append N items before blocking for space to become
* available; a consumer thread can remove items and block only when no items are
* left.  Thus, two counting semaphores could be used to manage such a buffer, one
* counting empty slots and one counting filled slots.  As long as production and
* consumption proceed at comparable rates, no thread needs to be suspended.
*/
class FXAPI FXSemaphore {
private:
  FXuval data[21];
private:
  FXSemaphore(const FXSemaphore&);
  FXSemaphore& operator=(const FXSemaphore&);
public:

  /// Initialize semaphore with given count
  FXSemaphore(FXint count=1);

  /// Decrement semaphore by 1, waiting if count is zero
  void wait();

  /// Try decrement semaphore; return false if timed out
  FXbool wait(FXTime nsec);

  /// Try decrement semaphore, and return false if count was zero
  FXbool trywait();

  /// Increment semaphore by 1
  void post();

  /// Delete semaphore
  ~FXSemaphore();
  };

}

#endif

