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
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "FXReadWriteLock.h"

/*
  Notes:

  - Read/write lock variable.
*/

using namespace FX;

/*******************************************************************************/

namespace FX {

#if defined(WIN32)
struct RWLOCK {
  CRITICAL_SECTION mutex[1];
  CRITICAL_SECTION access[1];
  DWORD            readers;
  };
#endif


// Initialize read/write lock
FXReadWriteLock::FXReadWriteLock(){
#if defined(WIN32) && (_WIN32_WINNT >= 0x0600)    // Vista or newer
  // If this fails on your machine, determine what value
  // of sizeof(RWLOCK) is supposed to be on your
  // machine and mail it to: jeroen@fox-toolkit.net!!
  //FXTRACE((150,"sizeof(SRWLOCK)=%d\n",sizeof(SRWLOCK)));
  FXASSERT_STATIC(sizeof(data)>=sizeof(SRWLOCK));
  InitializeSRWLock((SRWLOCK*)data);
#elif defined(WIN32)
  // If this fails on your machine, determine what value
  // of sizeof(RWLOCK) is supposed to be on your
  // machine and mail it to: jeroen@fox-toolkit.net!!
  //FXTRACE((150,"sizeof(RWLOCK)=%d\n",sizeof(RWLOCK)));
  FXASSERT_STATIC(sizeof(data)>=sizeof(RWLOCK));
  InitializeCriticalSection(((RWLOCK*)data)->mutex);
  InitializeCriticalSection(((RWLOCK*)data)->access);
  ((RWLOCK*)data)->readers=0;
#elif (_XOPEN_SOURCE >= 500) || (_POSIX_C_SOURCE >= 200809L)
  // If this fails on your machine, determine what value
  // of sizeof(pthread_rwlock_t) is supposed to be on your
  // machine and mail it to: jeroen@fox-toolkit.net!!
  //FXTRACE((150,"sizeof(pthread_rwlock_t)=%d\n",sizeof(pthread_rwlock_t)));
  FXASSERT_STATIC(sizeof(data)>=sizeof(pthread_rwlock_t));
  pthread_rwlockattr_t rwlockatt;
  pthread_rwlockattr_init(&rwlockatt);
  pthread_rwlockattr_setkind_np(&rwlockatt,PTHREAD_RWLOCK_PREFER_WRITER_NP);
  pthread_rwlock_init((pthread_rwlock_t*)data,&rwlockatt);
  pthread_rwlockattr_destroy(&rwlockatt);
#else
  // If this fails on your machine, determine what value
  // of sizeof(pthread_rwlock_t) is supposed to be on your
  // machine and mail it to: jeroen@fox-toolkit.net!!
  //FXTRACE((150,"sizeof(pthread_rwlock_t)=%d\n",sizeof(pthread_rwlock_t)));
  FXASSERT_STATIC(sizeof(data)>=sizeof(pthread_rwlock_t));
  pthread_rwlock_init((pthread_rwlock_t*)data,nullptr);
#endif
  }


// Acquire read lock for read/write lock
void FXReadWriteLock::readLock(){
#if defined(WIN32) && (_WIN32_WINNT >= 0x0600)    // Vista or newer
  AcquireSRWLockShared((SRWLOCK*)data);
#elif defined(WIN32)
  EnterCriticalSection(((RWLOCK*)data)->mutex);
  if(++((RWLOCK*)data)->readers==1){
    EnterCriticalSection(((RWLOCK*)data)->access);
    }
  LeaveCriticalSection(((RWLOCK*)data)->mutex);
#else
  pthread_rwlock_rdlock((pthread_rwlock_t*)data);
#endif
  }


// Try to acquire read lock for read/write lock
FXbool FXReadWriteLock::tryReadLock(){
#if defined(WIN32) && (_WIN32_WINNT >= 0x0600)    // Vista or newer
  return TryAcquireSRWLockShared((SRWLOCK*)data)!=0;
#elif defined(WIN32)
  if(TryEnterCriticalSection(((RWLOCK*)data)->mutex)){
    if(++((RWLOCK*)data)->readers==1 && !TryEnterCriticalSection(((RWLOCK*)data)->access)){
      --((RWLOCK*)data)->readers;
      LeaveCriticalSection(((RWLOCK*)data)->mutex);
      return false;
      }
    LeaveCriticalSection(((RWLOCK*)data)->mutex);
    return true;
    }
  return false;
#else
  return pthread_rwlock_tryrdlock((pthread_rwlock_t*)data)==0;
#endif
  }


// Unlock read lock
void FXReadWriteLock::readUnlock(){
#if defined(WIN32) && (_WIN32_WINNT >= 0x0600)    // Vista or newer
  ReleaseSRWLockShared((SRWLOCK*)data);
#elif defined(WIN32)
  EnterCriticalSection(((RWLOCK*)data)->mutex);
  if(--((RWLOCK*)data)->readers==0){
    LeaveCriticalSection(((RWLOCK*)data)->access);
    }
  LeaveCriticalSection(((RWLOCK*)data)->mutex);
#else
  pthread_rwlock_unlock((pthread_rwlock_t*)data);
#endif
  }


// Test if read locked
FXbool FXReadWriteLock::readLocked(){
  if(tryReadLock()){
    readUnlock();
    return false;
    }
  return true;
  }


// Acquire write lock for read/write lock
void FXReadWriteLock::writeLock(){
#if defined(WIN32) && (_WIN32_WINNT >= 0x0600)    // Vista or newer
  AcquireSRWLockExclusive((SRWLOCK*)data);
#elif defined(WIN32)
  EnterCriticalSection(((RWLOCK*)data)->access);
#else
  pthread_rwlock_wrlock((pthread_rwlock_t*)data);
#endif
  }


// Try to acquire write lock for read/write lock
FXbool FXReadWriteLock::tryWriteLock(){
#if defined(WIN32) && (_WIN32_WINNT >= 0x0600)    // Vista or newer
  return TryAcquireSRWLockExclusive((SRWLOCK*)data)!=0;
#elif defined(WIN32) && (_WIN32_WINNT >= 0x0400)
  return TryEnterCriticalSection(((RWLOCK*)data)->access)!=0;
#elif defined(WIN32)
  return false;
#else
  return pthread_rwlock_trywrlock((pthread_rwlock_t*)data)==0;
#endif
  }


// Unlock write lock
void FXReadWriteLock::writeUnlock(){
#if defined(WIN32) && (_WIN32_WINNT >= 0x0600)    // Vista or newer
  ReleaseSRWLockExclusive((SRWLOCK*)data);
#elif defined(WIN32)
  LeaveCriticalSection(((RWLOCK*)data)->access);
#else
  pthread_rwlock_unlock((pthread_rwlock_t*)data);
#endif
  }


// Test if write locked
FXbool FXReadWriteLock::writeLocked(){
  if(tryWriteLock()){
    writeUnlock();
    return false;
    }
  return true;
  }


// Delete read/write lock
FXReadWriteLock::~FXReadWriteLock(){
#if defined(WIN32) && (_WIN32_WINNT >= 0x0600)    // Vista or newer
  // NOP //
#elif defined(WIN32)
  DeleteCriticalSection(((RWLOCK*)data)->mutex);
  DeleteCriticalSection(((RWLOCK*)data)->access);
#else
  pthread_rwlock_destroy((pthread_rwlock_t*)data);
#endif
  }

}
