/********************************************************************************
*                                                                               *
*                  R e a d - W r i t e   L o c k   C l a s s                    *
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
#ifndef FXREADWRITELOCK_H
#define FXREADWRITELOCK_H

namespace FX {


/**
* FXReadWriteLock allows multiple readers but only a single writer.
*
* FXReadWriteLock provides access to a shared region just like FXMutex,
* except that threads that just read from the shared variables are not
* excluding each other.  When a thread tries to write, however, it can
* only proceed when no other writers or readers are present.
* Thus, data structures which are frequently inspected but rarely updated
* can be more effectively accessed with a read/write lock than with a mutex.
*/
class FXAPI FXReadWriteLock {
private:
  FXuval data[32];
private:
  FXReadWriteLock(const FXReadWriteLock&);
  FXReadWriteLock &operator=(const FXReadWriteLock&);
public:

  /// Initialize the read/write lock
  FXReadWriteLock();

  /// Acquire read lock for read/write lock
  void readLock();

  /// Try to acquire read lock for read/write lock
  FXbool tryReadLock();

  /// Unlock read lock
  void readUnlock();

  /// Test if read locked
  FXbool readLocked();

  /// Acquire write lock for read/write mutex
  void writeLock();

  /// Try to acquire write lock for read/write lock
  FXbool tryWriteLock();

  /// Unlock write mutex
  void writeUnlock();

  /// Test if write locked
  FXbool writeLocked();

  /// Delete the read/write lock
 ~FXReadWriteLock();
  };


/// Scoped read lock
class FXAPI FXScopedReadLock {
private:
  FXReadWriteLock& rwlock;
private:
  FXScopedReadLock();
  FXScopedReadLock(const FXScopedReadLock&);
  FXScopedReadLock& operator=(const FXScopedReadLock&);
public:

  /// Construct & lock associated read-write lock
  FXScopedReadLock(FXReadWriteLock& rwl):rwlock(rwl){ lock(); }

  /// Return reference to associated read-write lock
  FXReadWriteLock& readwritelock(){ return rwlock; }

  /// Lock read-write lock
  void lock(){ rwlock.readLock(); }

  /// Return true if succeeded locking the read-write lock
  FXbool trylock(){ return rwlock.tryReadLock(); }

  /// Return true if read-write lock is already locked
  FXbool locked(){ return rwlock.readLocked(); }

  /// Unlock mutex
  void unlock(){ rwlock.readUnlock(); }

  /// Destroy and unlock associated read-write lock
 ~FXScopedReadLock(){ unlock(); }
  };



/// Scoped write lock
class FXAPI FXScopedWriteLock {
private:
  FXReadWriteLock& rwlock;
private:
  FXScopedWriteLock();
  FXScopedWriteLock(const FXScopedWriteLock&);
  FXScopedWriteLock& operator=(const FXScopedWriteLock&);
public:

  /// Construct & lock associated read-write lock
  FXScopedWriteLock(FXReadWriteLock& rwl):rwlock(rwl){ lock(); }

  /// Return reference to associated read-write lock
  FXReadWriteLock& readwritelock(){ return rwlock; }

  /// Lock read-write lock
  void lock(){ rwlock.writeLock(); }

  /// Return true if succeeded locking the read-write lock
  FXbool trylock(){ return rwlock.tryWriteLock(); }

  /// Return true if read-write lock is already locked
  FXbool locked(){ return rwlock.writeLocked(); }

  /// Unlock read-write lock
  void unlock(){ rwlock.writeUnlock(); }

  /// Destroy and unlock associated read-write lock
 ~FXScopedWriteLock(){ unlock(); }
  };

}

#endif

