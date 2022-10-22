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
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "FXSpinLock.h"

/*
  Notes:

  - Spin lock variable.
  - Extra fluff data is not useless; it is intentionally there to pad the
    spin-variable to a cache line, thus reducing false-sharing in case other
    "hot" data is nearby.
  - Ticket spinlock flavor does not support more than 256 simultaneous threads.
*/

using namespace FX;

/*******************************************************************************/

namespace FX {

// Initialize spinlock
FXSpinLock::FXSpinLock(){
#if defined(WIN32)
  data[0]=data[1]=data[2]=data[3]=0;
#elif (defined(__GNUC__) || defined(__INTEL_COMPILER)) && (defined(__i386__) || defined(__x86_64__))
  data[0]=data[1]=data[2]=data[3]=0;
#elif defined(__APPLE__)
  // If this fails on your machine, determine what value
  // of sizeof(pthread_mutex_t) is supposed to be on your
  // machine and mail it to: jeroen@fox-toolkit.net!!
  //FXTRACE((150,"sizeof(OSSpinLock)=%d\n",sizeof(OSSpinLock)));
  FXASSERT_STATIC(sizeof(data)>=sizeof(OSSpinLock));
  data[0]=data[1]=data[2]=data[3]=0;
  *((OSSpinLock*)data)=OS_SPINLOCK_INIT;
#else
  // If this fails on your machine, determine what value
  // of sizeof(pthread_spinlock_t) is supposed to be on your
  // machine and mail it to: jeroen@fox-toolkit.net!!
  //FXTRACE((150,"sizeof(pthread_spinlock_t)=%d\n",sizeof(pthread_spinlock_t)));
  FXASSERT_STATIC(sizeof(data)>=sizeof(pthread_spinlock_t));
  data[0]=data[1]=data[2]=data[3]=0;
  pthread_spin_init((pthread_spinlock_t*)(void*)data,PTHREAD_PROCESS_PRIVATE);
#endif
  }


// Lock the spinlock
void FXSpinLock::lock(){
#if defined(WIN32)
  while(_InterlockedExchange((LONG*)data,1L)){
    YieldProcessor();
    }
#elif (defined(__GNUC__) || defined(__INTEL_COMPILER)) && (defined(__i386__) || defined(__x86_64__))
  __asm__ __volatile__ ("movw $0x0100, %%ax \n\t"
                        "lock\n\t"
                        "xaddw %%ax, %0 \n\t"
                        "1: \n\t"
                        "cmpb %%ah, %%al\n\t"
                        "je 2f\n\t"
                        "rep; nop\n\t"
                        "movb %0, %%al\n\t"
                        "jmp 1b\n\t"
                        "2:"  : "+m" (data) : : "memory", "ax", "cc");
#elif defined(__APPLE__)
  OSSpinLockLock((OSSpinLock*)data);
#else
  pthread_spin_lock((pthread_spinlock_t*)(void*)data);
#endif
  }


// Try lock the spinlock
FXbool FXSpinLock::trylock(){
#if defined(WIN32)
  return !_InterlockedExchange((LONG*)data,1L);
#elif (defined(__GNUC__) || defined(__INTEL_COMPILER)) && (defined(__i386__) || defined(__x86_64__))
  FXbool ret;
  __asm__ __volatile__ ("movw %1,%%ax\n\t"
                        "cmpb %%ah, %%al\n\t"
                        "jne 1f\n\t"
                        "movw %%ax,%%cx\n\t"
                        "addw $0x0100,%%cx\n\t"
                        "lock\n\t"
                        "cmpxchgw %%cx,%1\n\t"
                        "1:\n\t"
                        "sete %b0\n\t" :"=a" (ret), "+m" (data) : : "ecx", "memory", "cc");
  return ret;
#elif defined(__APPLE__)
  return OSSpinLockTry((OSSpinLock*)data);
#else
  return pthread_spin_trylock((pthread_spinlock_t*)(void*)data)==0;
#endif
  }


// Unlock spinlock
void FXSpinLock::unlock(){
#if defined(WIN32)
  _InterlockedExchange((LONG*)data,0L);
#elif (defined(__GNUC__) || defined(__INTEL_COMPILER)) && (defined(__i386__) || defined(__x86_64__))
  __asm__ __volatile__ ("lock\n\t"
                        "incb %0\n\t" : "+m" (data) : : "memory", "cc");
#elif defined(__APPLE__)
  OSSpinLockUnlock((OSSpinLock*)data);
#else
  pthread_spin_unlock((pthread_spinlock_t*)(void*)data);
#endif
  }


// Test if locked
FXbool FXSpinLock::locked(){
#if defined(WIN32)
  return (data[0]!=0);
#elif (defined(__GNUC__) || defined(__INTEL_COMPILER)) && (defined(__i386__) || defined(__x86_64__))
  FXbool ret;
  __asm__ __volatile__ ("movw %1,%%ax\n\t"
                        "cmpb %%ah, %%al\n\t"
                        "setne %%al\n\t" :"=a" (ret), "+m" (data) : : "memory", "cc");
  return ret;
#elif defined(__APPLE__)
  if(OSSpinLockTry((OSSpinLock*)data)){
    OSSpinLockUnlock((OSSpinLock*)data);
    return false;
    }
  return true;
#else
  if(pthread_spin_trylock((pthread_spinlock_t*)(void*)data)==0){
    pthread_spin_unlock((pthread_spinlock_t*)(void*)data);
    return false;
    }
  return true;
#endif
  }


// Delete spinlock
FXSpinLock::~FXSpinLock(){
#if defined(WIN32)
  // NOP //
#elif (defined(__GNUC__) || defined(__INTEL_COMPILER)) && (defined(__i386__) || defined(__x86_64__))
  // NOP //
#elif defined(__APPLE__)
  // NOP //
#else
  pthread_spin_destroy((pthread_spinlock_t*)(void*)data);
#endif
  }

}
