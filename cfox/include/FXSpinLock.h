/********************************************************************************
*                                                                               *
*                           S p i n l o c k   C l a s s                         *
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
#ifndef FXSPINLOCK_H
#define FXSPINLOCK_H

namespace FX {


/**
* FXSpinLock can be used to provide safe access to very small critical sections.
*
* Similar to FXMutex, a FXSpinLock provides safe access to a critical section
* shared by multiple threads.  Unlike FXMutex, however, a thread which is unable
* to obtain the lock will not block, but spin in a tight loop until the lock can
* be obtained.  The advantage of FXSpinLock over FXMutex is that no operating
* system calls are performed suspending and resuming the calling thread.
*/
class FXAPI FXSpinLock {
private:
  volatile FXuval data[4];
private:
  FXSpinLock(const FXSpinLock&);
  FXSpinLock &operator=(const FXSpinLock&);
public:

  /// Initialize the spinlock
  FXSpinLock();

  /// Lock the spinlock
  void lock();

  /// Return true if succeeded locking the spinlock
  FXbool trylock();

  /// Return true if spinlock is already locked
  FXbool locked();

  /// Unlock spinlock
  void unlock();

  /// Delete the spinlock
  ~FXSpinLock();
  };


/**
* Establish a correspondence between a C++ scope and an FXSpinLock,
* so that entering and leaving the scope in which the scoped lock
* is defined will automatically lock and unlock the associated
* spin lock.
* This will typically result in much less coding, and in addition
* will make the code safe from exceptions.
*/
class FXAPI FXScopedSpinLock {
private:
  FXSpinLock& spn;
private:
  FXScopedSpinLock();
  FXScopedSpinLock(const FXScopedSpinLock&);
  FXScopedSpinLock& operator=(const FXScopedSpinLock&);
public:

  /// Construct & lock associated spinlock
  FXScopedSpinLock(FXSpinLock& s):spn(s){ lock(); }

  /// Return reference to associated spinlock
  FXSpinLock& spinlock(){ return spn; }

  /// Lock spinlock
  void lock(){ spn.lock(); }

  /// Return true if succeeded locking the spinlock
  FXbool trylock(){ return spn.trylock(); }

  /// Return true if spinlock is already locked
  FXbool locked(){ return spn.locked(); }

  /// Unlock spin lock
  void unlock(){ spn.unlock(); }

  /// Destroy and unlock associated spinlock
  ~FXScopedSpinLock(){ unlock(); }
  };


/**
* The reverse spin lock unlocks its associated FXSpinLock when entering
* the scope, and automatically relocks it upon exiting the scope.
* Exceptions raised while in this region will automatically relock
* the spin lock upon leaving the enclosing scope.
*/
class FXAPI FXReverseSpinLock {
private:
  FXSpinLock& spn;
private:
  FXReverseSpinLock();
  FXReverseSpinLock(const FXReverseSpinLock&);
  FXReverseSpinLock& operator=(const FXReverseSpinLock&);
public:

  /// Construct and unlock associated spin lock
  FXReverseSpinLock(FXSpinLock& s):spn(s){ unlock(); }

  /// Return reference to associated spin lock
  FXSpinLock& spinlock(){ return spn; }

  /// Lock spin lock
  void lock(){ spn.lock(); }

  /// Return true if succeeded locking the spin lock
  FXbool trylock(){ return spn.trylock(); }

  /// Return true if spin lock is already locked
  FXbool locked(){ return spn.locked(); }

  /// Unlock spin lock
  void unlock(){ spn.unlock(); }

  /// Destroy and relock associated spin lock
  ~FXReverseSpinLock(){ lock(); }
  };

}

#endif

