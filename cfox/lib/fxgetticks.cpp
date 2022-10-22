/********************************************************************************
*                                                                               *
*                          U t i l i t y   F u n c t i o n s                    *
*                                                                               *
*********************************************************************************
* Copyright (C) 1998,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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

/*
  Notes:
  - Access to CPU tick counters.
  - Very accurate time-source used to profile time-critical
    pieces of code.
  - Depends on particular hardware features which unfortunately are
    not always present on all processors [but most recent CPU's have
    something like this].
*/


using namespace FX;

/*******************************************************************************/

namespace FX {


#if defined(WIN32)

#if defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))

FXTime fxgetticks(){
  FXTime value=__rdtsc();
  return value;
  }

#else

// Return clock ticks from performance counter [WIN32 version].
FXTime fxgetticks(){
  FXTime value;
  QueryPerformanceCounter((LARGE_INTEGER*)&value);
  return value;
  }

#endif

#elif (defined(__GNUC__) || defined(__ICC)) && defined(__i386__)

// Return clock ticks from x86 TSC register [GCC/ICC x86 version].
FXTime fxgetticks(){
  FXTime value;
  asm ("rdtsc" : "=A" (value));
  return value;
  }

#elif (defined(__GNUC__) || defined(__ICC)) && defined(__x86_64__)

// Return clock ticks from AMD64 TSC register [GCC AMD64 version].
FXTime fxgetticks(){
  FXTime value;
  __asm__ __volatile__ ( "rdtsc              \n\t"
                         "salq	$32, %%rdx   \n\t"
                         "orq	%%rdx, %%rax \n\t" : "=q" (value));
  return value;
  }

/*
#elif defined(__GNUC__) && defined(__aarch64__)

// Return clock ticks from AMD64 TSC register [GCC AMD64 version].
FXTime fxgetticks(){
  FXTime value;
  __asm__ __volatile__("mrs %0, cntvct_el0" : "=r" (value));
//  __asm__ __volatile__("isb; mrs %0, cntvct_el0" : "=r"(value));
  return value;
  }
*/


#elif !defined(__INTEL_COMPILER) && defined(__GNUC__) && defined(__ia64__)

// Return clock ticks from performance counter [GCC IA64 version].
FXTime fxgetticks(){
  FXTime value;
  asm ("mov %0=ar.itc" : "=r" (value));
  return value;
  }

#elif defined(__GNUC__) && (defined(__powerpc64__) || defined(__ppc64__))

FXTime fxgetticks(){
  FXTime value;
  asm volatile("mfspr %0, 268" : "=r"(value));
  return value;
  }

#elif defined(__GNUC__) && (defined(__powerpc__) || defined(__ppc__))

// Return clock ticks from performance counter [GCC PPC version].
FXTime fxgetticks(){
  FXuint tbl, tbu0, tbu1;
  asm volatile("mftbu %0 \n\t"
               "mftb  %1 \n\t"
               "mftbu %2 \n\t" : "=r"(tbu0), "=r"(tbl), "=r"(tbu1));
  tbl&=-(FXint)(tbu0==tbu1);
  return (static_cast<uint64_t>(tbu1) << 32) | tbl;
  return (((FXTime)tbu1) << 32) | tbl;
  }

#elif (_POSIX_C_SOURCE >= 199309L)

// Return clock ticks [POSIX fallback version].
FXTime fxgetticks(){
  const FXTime seconds=1000000000;
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC,&ts);
  return ts.tv_sec*seconds+ts.tv_nsec;
  }

#else

// Return clock ticks [UNIX fallback version].
FXTime fxgetticks(){
  const FXTime seconds=1000000000;
  const FXTime microseconds=1000;
  struct timeval tv;
  gettimeofday(&tv,nullptr);
  return tv.tv_sec*seconds+tv.tv_usec*microseconds;
  }

#endif

}

