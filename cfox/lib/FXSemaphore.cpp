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
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "FXSemaphore.h"

/*
  Notes:

  - Semaphore variable.

  - Implementation using Condition and Mutex now used for MacOSX and Minix.
    This may be less performant than a true semaphore, but its a nice and fully functional
    fallback until full posix semaphore implementation is available.
  - Increased reserved for pthread_mutex_t as up to 11 FXuval's in size on MacOSX,
    worst case is on 32-bit versions.
  - Keep in mind we can not do sizeof() in the header file, as we're trying to avoid
    including system-headers from public accessible FOX header files.  This is so
    as to avoid "accidental" incorporation of declarations or unexpected macros,
    that may clash.
  - Also we want to avoid calls to memory allocators as that would incur a big
    performance penalty; besides, a memory allocator may need to call locking
    primitives since memory is a resource shared by all threads.
  - The upshot is that the space allocated for the semaphore may be a bit more
    than strictly necessary on some machines.  It may actually be a good thing
    as this increases the odds of these datastructures living in dedicated
    cache-lines.
*/

using namespace FX;


namespace FX {


/*******************************************************************************/

// Initialize semaphore with given count
FXSemaphore::FXSemaphore(FXint count){
#if defined(WIN32)
  FXASSERT_STATIC(sizeof(data)>=sizeof(HANDLE));
  data[0]=(FXuval)CreateSemaphore(nullptr,count,0x7fffffff,nullptr);
#elif (defined(__APPLE__) || defined(__minix))
  // If this fails on your machine, determine what value of
  // sizeof(pthread_cond_t) and sizeof(pthread_mutex_t) is
  // supposed to be and mail it to: jeroen@fox-toolkit.net!!
  //FXTRACE((150,"sizeof(pthread_cond_t)=%d\n",sizeof(pthread_cond_t)));
  //FXTRACE((150,"sizeof(pthread_mutex_t)=%d\n",sizeof(pthread_mutex_t)));
  FXASSERT_STATIC(sizeof(FXuval)*9 >= sizeof(pthread_cond_t));
  FXASSERT_STATIC(sizeof(FXuval)*11 >= sizeof(pthread_mutex_t));
  data[0]=count;
  pthread_cond_init((pthread_cond_t*)&data[1],nullptr);
  pthread_mutex_init((pthread_mutex_t*)&data[10],nullptr);
#else
  // If this fails on your machine, determine what value
  // of sizeof(sem_t) is supposed to be on your
  // machine and mail it to: jeroen@fox-toolkit.net!!
  //FXTRACE((150,"sizeof(sem_t)=%d\n",sizeof(sem_t)));
  FXASSERT_STATIC(sizeof(data)>=sizeof(sem_t));
  sem_init((sem_t*)data,0,(unsigned int)count);
#endif
  }


// Decrement semaphore, waiting if count is zero
void FXSemaphore::wait(){
#if defined(WIN32)
  WaitForSingleObject((HANDLE)data[0],INFINITE);
#elif (defined(__APPLE__) || defined(__minix))
  pthread_mutex_lock((pthread_mutex_t*)&data[10]);
  while(data[0]==0){
    pthread_cond_wait((pthread_cond_t*)&data[1],(pthread_mutex_t*)&data[10]);
    }
  data[0]-=1;
  pthread_mutex_unlock((pthread_mutex_t*)&data[10]);
#else
  sem_wait((sem_t*)data);
#endif
  }


// Try decrement semaphore; return false if timed out
FXbool FXSemaphore::wait(FXTime nsec){
#if defined(WIN32)
  if(0<nsec){
    if(nsec<forever){
      DWORD delay=(DWORD)(nsec/1000000);
      return WaitForSingleObject((HANDLE)data[0],delay)==WAIT_OBJECT_0;
      }
    return WaitForSingleObject((HANDLE)data[0],INFINITE)==WAIT_OBJECT_0;
    }
  return WaitForSingleObject((HANDLE)data[0],0)==WAIT_OBJECT_0;
#elif (defined(__APPLE__) || defined(__minix))
  pthread_mutex_lock((pthread_mutex_t*)&data[10]);
  while(data[0]==0){
    if(0<nsec){
      if(nsec<forever){
#if (_POSIX_C_SOURCE >= 199309L)
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME,&ts);
        ts.tv_sec=ts.tv_sec+(ts.tv_nsec+nsec)/1000000000;
        ts.tv_nsec=(ts.tv_nsec+nsec)%1000000000;
        if(pthread_cond_timedwait((pthread_cond_t*)&data[1],(pthread_mutex_t*)&data[10],&ts)!=0) goto x;
#else
        struct timespec ts;
        struct timeval tv;
        gettimeofday(&tv,nullptr);
        tv.tv_usec*=1000;
        ts.tv_sec=tv.tv_sec+(tv.tv_usec+nsec)/1000000000;
        ts.tv_nsec=(tv.tv_usec+nsec)%1000000000;
        if(pthread_cond_timedwait((pthread_cond_t*)&data[1],(pthread_mutex_t*)&data[10],&ts)!=0) goto x;
#endif
        continue;
        }
      pthread_cond_wait((pthread_cond_t*)&data[1],(pthread_mutex_t*)&data[10]);
      continue;
      }
x:  pthread_mutex_unlock((pthread_mutex_t*)&data[10]);
    return false;
    }
  --data[0];
  pthread_mutex_unlock((pthread_mutex_t*)&data[10]);
  return true;
#else
  if(0<nsec){
    if(nsec<forever){
#if (_POSIX_C_SOURCE >= 199309L)
      struct timespec ts;
      clock_gettime(CLOCK_REALTIME,&ts);
      ts.tv_sec=ts.tv_sec+(ts.tv_nsec+nsec)/1000000000;
      ts.tv_nsec=(ts.tv_nsec+nsec)%1000000000;
      return sem_timedwait((sem_t*)data,&ts)==0;
#else
      struct timespec ts;
      struct timeval tv;
      gettimeofday(&tv,nullptr);
      tv.tv_usec*=1000;
      ts.tv_sec=tv.tv_sec+(tv.tv_usec+nsec)/1000000000;
      ts.tv_nsec=(tv.tv_usec+nsec)%1000000000;
      return sem_timedwait((sem_t*)data,&ts)==0;
#endif
      }
    return sem_wait((sem_t*)data)==0;
    }
  return sem_trywait((sem_t*)data)==0;
#endif
  }


// Try decrement semaphore; and return false if count was zero
FXbool FXSemaphore::trywait(){
#if defined(WIN32)
  return WaitForSingleObject((HANDLE)data[0],0)==WAIT_OBJECT_0;
#elif (defined(__APPLE__) || defined(__minix))
  pthread_mutex_lock((pthread_mutex_t*)&data[10]);
  if(data[0]==0){
    pthread_mutex_unlock((pthread_mutex_t*)&data[10]);
    return false;
    }
  data[0]-=1;
  pthread_mutex_unlock((pthread_mutex_t*)&data[10]);
  return true;
#else
  return sem_trywait((sem_t*)data)==0;
#endif
  }


// Increment semaphore
void FXSemaphore::post(){
#if defined(WIN32)
  ReleaseSemaphore((HANDLE)data[0],1,nullptr);
#elif (defined(__APPLE__) || defined(__minix))
  pthread_mutex_lock((pthread_mutex_t*)&data[10]);
  data[0]+=1;
  pthread_cond_signal((pthread_cond_t*)&data[1]);
  pthread_mutex_unlock((pthread_mutex_t*)&data[10]);
#else
  sem_post((sem_t*)data);
#endif
  }


// Delete semaphore
FXSemaphore::~FXSemaphore(){
#if defined(WIN32)
  CloseHandle((HANDLE)data[0]);
#elif (defined(__APPLE__) || defined(__minix))
  pthread_mutex_destroy((pthread_mutex_t*)&data[10]);
  pthread_cond_destroy((pthread_cond_t*)&data[1]);
#else
  sem_destroy((sem_t*)data);
#endif
  }

}
