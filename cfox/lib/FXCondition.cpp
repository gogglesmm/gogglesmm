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
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "FXMutex.h"
#include "FXCondition.h"

/*
  Notes:
  - Condition variable.
*/

using namespace FX;

/*******************************************************************************/

namespace FX {

// Initialize condition
FXCondition::FXCondition(){
#if defined(WIN32) && (_WIN32_WINNT >= 0x0600)    // Vista or newer
  // If this fails on your machine, determine what value
  // of sizeof(pthread_cond_t) is supposed to be on your
  // machine and mail it to: jeroen@fox-toolkit.net!!
  FXASSERT_STATIC(sizeof(data)>=sizeof(CONDITION_VARIABLE));
  InitializeConditionVariable((CONDITION_VARIABLE*)data);
#elif defined(WIN32)
  // If this fails on your machine, determine what value
  // of sizeof(pthread_cond_t) is supposed to be on your
  // machine and mail it to: jeroen@fox-toolkit.net!!
  //FXTRACE((150,"sizeof(CRITICAL_SECTION)+sizeof(HANDLE)+sizeof(HANDLE)+sizeof(FXuval)=%d\n",sizeof(CRITICAL_SECTION)+sizeof(HANDLE)+sizeof(HANDLE)+sizeof(FXuval)));
  FXASSERT_STATIC(sizeof(data)>=sizeof(CRITICAL_SECTION)+sizeof(HANDLE)+sizeof(HANDLE)+sizeof(FXuval));
  data[0]=(FXuval)CreateEvent(nullptr,0,0,nullptr);                   // Wakes one, autoreset
  data[1]=(FXuval)CreateEvent(nullptr,1,0,nullptr);                   // Wakes all, manual reset
  data[2]=0;                                                    // Blocked count
  InitializeCriticalSection((CRITICAL_SECTION*)&data[3]);       // Critical section
#else
  // If this fails on your machine, determine what value
  // of sizeof(pthread_cond_t) is supposed to be on your
  // machine and mail it to: jeroen@fox-toolkit.net!!
  //FXTRACE((150,"sizeof(pthread_cond_t)=%d\n",sizeof(pthread_cond_t)));
  FXASSERT_STATIC(sizeof(data)>=sizeof(pthread_cond_t));
  pthread_cond_init((pthread_cond_t*)data,nullptr);
#endif
  }


// Wake up one single waiting thread
void FXCondition::signal(){
#if defined(WIN32) && (_WIN32_WINNT >= 0x0600)    // Vista or newer
  WakeConditionVariable((CONDITION_VARIABLE*)data);
#elif defined(WIN32)
  EnterCriticalSection((CRITICAL_SECTION*)&data[3]);
  int blocked=(data[2]>0);
  LeaveCriticalSection((CRITICAL_SECTION*)&data[3]);
  if(blocked) SetEvent((HANDLE)data[0]);
#else
  pthread_cond_signal((pthread_cond_t*)data);
#endif
  }


// Wake up all waiting threads
void FXCondition::broadcast(){
#if defined(WIN32) && (_WIN32_WINNT >= 0x0600)    // Vista or newer
  WakeAllConditionVariable((CONDITION_VARIABLE*)data);
#elif defined(WIN32)
  EnterCriticalSection((CRITICAL_SECTION*)&data[3]);
  int blocked=(data[2]>0);
  LeaveCriticalSection((CRITICAL_SECTION*)&data[3]);
  if(blocked) SetEvent((HANDLE)data[1]);
#else
  pthread_cond_broadcast((pthread_cond_t*)data);
#endif
  }


// Wait
FXbool FXCondition::wait(FXMutex& mtx){
#if defined(WIN32) && (_WIN32_WINNT >= 0x0600)    // Vista or newer
  return SleepConditionVariableCS((CONDITION_VARIABLE*)data,(CRITICAL_SECTION*)mtx.data,INFINITE)!=0;
#elif defined(WIN32)
  EnterCriticalSection((CRITICAL_SECTION*)&data[3]);
  data[2]++;
  LeaveCriticalSection((CRITICAL_SECTION*)&data[3]);
  mtx.unlock();
  DWORD result=WaitForMultipleObjects(2,(HANDLE*)data,0,INFINITE);
  EnterCriticalSection((CRITICAL_SECTION*)&data[3]);
  data[2]--;
  int last_waiter=(result==WAIT_OBJECT_0+1)&&(data[2]==0);      // Unblocked by broadcast & no other blocked threads
  LeaveCriticalSection((CRITICAL_SECTION*)&data[3]);
  if(last_waiter) ResetEvent((HANDLE)data[1]);                  // Reset signal
  mtx.lock();
  return (WAIT_OBJECT_0+0==result)||(result==WAIT_OBJECT_0+1);
#else
  return pthread_cond_wait((pthread_cond_t*)data,(pthread_mutex_t*)mtx.data)==0;
#endif
  }


// Wait using single global mutex
FXbool FXCondition::wait(FXMutex& mtx,FXTime nsec){
#if defined(WIN32) && (_WIN32_WINNT >= 0x0600)    // Vista or newer
  if(0<nsec){
    DWORD delay=INFINITE;
    if(nsec<forever) delay=nsec/1000000;
    return SleepConditionVariableCS((CONDITION_VARIABLE*)data,(CRITICAL_SECTION*)mtx.data,delay)!=0;
    }
  return false;
#elif defined(WIN32)
  if(0<nsec){
    DWORD delay=INFINITE;
    if(nsec<forever) delay=(DWORD)(nsec/1000000);
    EnterCriticalSection((CRITICAL_SECTION*)&data[3]);
    data[2]++;
    LeaveCriticalSection((CRITICAL_SECTION*)&data[3]);
    mtx.unlock();
    DWORD result=WaitForMultipleObjects(2,(HANDLE*)data,0,delay);
    EnterCriticalSection((CRITICAL_SECTION*)&data[3]);
    data[2]--;
    int last_waiter=(result==WAIT_OBJECT_0+1)&&(data[2]==0);    // Unblocked by broadcast & no other blocked threads
    LeaveCriticalSection((CRITICAL_SECTION*)&data[3]);
    if(last_waiter) ResetEvent((HANDLE)data[1]);                // Reset signal
    mtx.lock();
    return (WAIT_OBJECT_0+0==result)||(result==WAIT_OBJECT_0+1);
    }
  return false;
#else
  if(0<nsec){
    if(nsec<forever){
#if (_POSIX_C_SOURCE >= 199309L)
      struct timespec ts;
      clock_gettime(CLOCK_REALTIME,&ts);
      ts.tv_sec=ts.tv_sec+(ts.tv_nsec+nsec)/1000000000;
      ts.tv_nsec=(ts.tv_nsec+nsec)%1000000000;
      return pthread_cond_timedwait((pthread_cond_t*)data,(pthread_mutex_t*)mtx.data,&ts)==0;
#else
      struct timespec ts;
      struct timeval tv;
      gettimeofday(&tv,nullptr);
      tv.tv_usec*=1000;
      ts.tv_sec=tv.tv_sec+(tv.tv_usec+nsec)/1000000000;
      ts.tv_nsec=(tv.tv_usec+nsec)%1000000000;
      return pthread_cond_timedwait((pthread_cond_t*)data,(pthread_mutex_t*)mtx.data,&ts)==0;
#endif
      }
    return pthread_cond_wait((pthread_cond_t*)data,(pthread_mutex_t*)mtx.data)==0;
    }
  return false;
#endif
  }


// Delete condition
FXCondition::~FXCondition(){
#if defined(WIN32) && (_WIN32_WINNT >= 0x0600)    // Vista or newer
  // NOP //
#elif defined(WIN32)
  CloseHandle((HANDLE)data[0]);
  CloseHandle((HANDLE)data[1]);
  DeleteCriticalSection((CRITICAL_SECTION*)&data[3]);
#else
  pthread_cond_destroy((pthread_cond_t*)data);
#endif
  }

}
