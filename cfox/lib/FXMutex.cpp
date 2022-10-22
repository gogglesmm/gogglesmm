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
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "FXMutex.h"

/*
  Notes:

  - Simple (optionally recursive) mutex variable.

  - An amorphous blob of memory is reserved in the declaration, without revealing
    implementation details.  This is better than allocating memory dynamically, both
    in terms of speed as well as the possibility that hitting malloc() will involve
    another mutex.

  - Hopefully it is big enough for all supported platforms.  But just in case, there's
    an assert in the constructor to verify that enough space is reserved for the
    implementation.

  - I do therefore recommend running this in debug mode first time around on a
    new platform.

  - If you run into this, try to figure out sizeof(pthread_mutex_t) and
    let me know about it (jeroen@fox-toolkit.net).
*/

using namespace FX;

/*******************************************************************************/

namespace FX {

// Initialize mutex
FXMutex::FXMutex(FXbool recursive){
#if defined(WIN32)
  // If this fails on your machine, determine what value
  // of sizeof(CRITICAL_SECTION) is supposed to be on your
  // machine and mail it to: jeroen@fox-toolkit.net!!
  //FXTRACE((150,"sizeof(CRITICAL_SECTION)=%d\n",sizeof(CRITICAL_SECTION)));
  FXASSERT_STATIC(sizeof(data)>=sizeof(CRITICAL_SECTION));
  InitializeCriticalSection((CRITICAL_SECTION*)data);
#else
  // If this fails on your machine, determine what value
  // of sizeof(pthread_mutex_t) is supposed to be on your
  // machine and mail it to: jeroen@fox-toolkit.net!!
  //FXTRACE((150,"sizeof(pthread_mutex_t)=%d\n",sizeof(pthread_mutex_t)));
  FXASSERT_STATIC(sizeof(data)>=sizeof(pthread_mutex_t));
  pthread_mutexattr_t mutexatt;
  pthread_mutexattr_init(&mutexatt);
  pthread_mutexattr_settype(&mutexatt,recursive?PTHREAD_MUTEX_RECURSIVE:PTHREAD_MUTEX_DEFAULT);
  pthread_mutex_init((pthread_mutex_t*)data,&mutexatt);
  pthread_mutexattr_destroy(&mutexatt);
#endif
  }


// Lock the mutex
void FXMutex::lock(){
#if defined(WIN32)
  EnterCriticalSection((CRITICAL_SECTION*)data);
#else
  pthread_mutex_lock((pthread_mutex_t*)data);
#endif
  }


// Try lock the mutex
FXbool FXMutex::trylock(){
#if defined(WIN32)
  return TryEnterCriticalSection((CRITICAL_SECTION*)data)!=0;
#elif defined(WIN32)
  return false;
#else
  return pthread_mutex_trylock((pthread_mutex_t*)data)==0;
#endif
  }


// Unlock mutex
void FXMutex::unlock(){
#if defined(WIN32)
  LeaveCriticalSection((CRITICAL_SECTION*)data);
#else
  pthread_mutex_unlock((pthread_mutex_t*)data);
#endif
  }


// Test if locked
FXbool FXMutex::locked(){
  if(trylock()){
    unlock();
    return false;
    }
  return true;
  }


// Delete mutex
FXMutex::~FXMutex(){
#if defined(WIN32)
  DeleteCriticalSection((CRITICAL_SECTION*)data);
#else
  pthread_mutex_destroy((pthread_mutex_t*)data);
#endif
  }

}
