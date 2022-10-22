/********************************************************************************
*                                                                               *
*                              M u t e x   C l a s s                            *
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
#ifndef FXMUTEX_H
#define FXMUTEX_H

namespace FX {


/**
* FXMutex provides a mutex which can be used to enforce critical
* sections around updates of data shared by multiple threads.
*/
class FXAPI FXMutex {
  friend class FXCondition;
private:
  FXuval data[24];
private:
  FXMutex(const FXMutex&);
  FXMutex &operator=(const FXMutex&);
public:

  /**
  * Initialize the mutex; if the parameter recursive is true,
  * the mutex is reentrant, i.e. counts the number of locks and unlocks.
  */
  FXMutex(FXbool recursive=false);

  /// Lock the mutex
  void lock();

  /// Return true if succeeded locking the mutex
  FXbool trylock();

  /// Return true if mutex is already locked
  FXbool locked();

  /// Unlock mutex
  void unlock();

  /// Delete the mutex
  ~FXMutex();
  };


/**
* Establish a correspondence between a C++ scope and a FXMutex,
* so that entering and leaving the scope in which the scoped lock
* is defined will automatically lock and unlock the associated
* mutex.
* This will typically result in much less coding, and in addition
* will make the code safe from exceptions.
*/
class FXAPI FXScopedMutex {
private:
  FXMutex& mtx;
private:
  FXScopedMutex();
  FXScopedMutex(const FXScopedMutex&);
  FXScopedMutex& operator=(const FXScopedMutex&);
public:

  /// Construct and lock associated mutex
  FXScopedMutex(FXMutex& m):mtx(m){ lock(); }

  /// Return reference to associated mutex
  FXMutex& mutex(){ return mtx; }

  /// Lock mutex
  void lock(){ mtx.lock(); }

  /// Return true if succeeded locking the mutex
  FXbool trylock(){ return mtx.trylock(); }

  /// Return true if mutex is already locked
  FXbool locked(){ return mtx.locked(); }

  /// Unlock mutex
  void unlock(){ mtx.unlock(); }

  /// Destroy and unlock associated mutex
  ~FXScopedMutex(){ unlock(); }
  };


/**
* The Reverse Mutex unlocks its associated FXMutex when entering the
* scope, and automatically relocks it upon exiting the scope.
* Exceptions raised while in this region will automatically relock
* the mutex upon leaving the enclosing scope.
*/
class FXAPI FXReverseMutex {
private:
  FXMutex& mtx;
private:
  FXReverseMutex();
  FXReverseMutex(const FXReverseMutex&);
  FXReverseMutex& operator=(const FXReverseMutex&);
public:

  /// Construct and unlock associated mutex
  FXReverseMutex(FXMutex& m):mtx(m){ unlock(); }

  /// Return reference to associated mutex
  FXMutex& mutex(){ return mtx; }

  /// Lock mutex
  void lock(){ mtx.lock(); }

  /// Return true if succeeded locking the mutex
  FXbool trylock(){ return mtx.trylock(); }

  /// Return true if mutex is already locked
  FXbool locked(){ return mtx.locked(); }

  /// Unlock mutex
  void unlock(){ mtx.unlock(); }

  /// Destroy and relock associated mutex
  ~FXReverseMutex(){ lock(); }
  };


}

#endif

