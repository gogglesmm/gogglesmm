/********************************************************************************
*                                                                               *
*                          U t i l i t y   F u n c t i o n s                    *
*                                                                               *
*********************************************************************************
* Copyright (C) 1998,2019 by Jeroen van der Zijp.   All Rights Reserved.        *
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

// Return clock ticks from performance counter [WIN32 version].
FXTime fxgetticks(){
  FXTime value;
  QueryPerformanceCounter((LARGE_INTEGER*)&value);
  return value;
  }

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


#elif !defined(__INTEL_COMPILER) && defined(__GNUC__) && defined(__ia64__)

// Return clock ticks from performance counter [GCC IA64 version].
FXTime fxgetticks(){
  FXTime value;
  asm ("mov %0=ar.itc" : "=r" (value));
  return value;
  }

#elif defined(__hpux) && defined(__ia64)
#include <machine/sys/inline.h>

// Return clock ticks from performance counter [HPUX C++ IA64 version].
FXTime fxgetticks(){
  FXTime ret;
  ret = _Asm_mov_from_ar (_AREG_ITC);
  return ret;
  }

#elif defined(__GNUC__) && (defined(__powerpc__) || defined(__ppc__))

// Return clock ticks from performance counter [GCC PPC version].
FXTime fxgetticks(){
  FXuint tbl,tbu0,tbu1;
  do{
    asm ("mftbu %0" : "=r"(tbu0));
    asm ("mftb %0" : "=r"(tbl));
    asm ("mftbu %0" : "=r"(tbu1));
    }
  while(tbu0!=tbu1);
  return (((FXTime)tbu0) << 32) | tbl;
  }

#elif defined(__GNUC__) && (defined(__hppa__) || defined(__hppa))

// Return clock ticks from performance counter [GCC PA-RISC version].
FXTime fxgetticks(){
  FXTime value;
  asm ("mfctl 16, %0": "=r" (value));                   // FIXME not tested!
  return value;
  }

#elif !defined(__GNUC__) && (defined(__hppa__) || defined(__hppa))
#include <machine/inline.h>

// Return clock ticks from performance counter [HP-C++ PA-RISC version].
FXTime fxgetticks(){
  register FXTime ret;
  _MFCTL(16, ret);
  return ret;
  }

#elif defined(__GNUC__) && defined(__alpha__)

// Return clock ticks from performance counter [GCC ALPHA version];
FXTime fxgetticks(){
  FXTime value;
  asm ("rpcc %0" : "=r"(value));                        // Only 32-bits accurate!
  return (value & 0xFFFFFFFF);
  }

#elif (defined(__DECC) || defined(__DECCXX)) && defined(__alpha)
#include <c_asm.h>

// Return clock ticks from performance counter [DEC C++ ALPHA version];
FXTime fxgetticks(){
  FXTime value;
  value = asm("rpcc %v0");
  return (value & 0xFFFFFFFF);                          // Only 32-bits accurate!
  }

#elif (defined(__IRIX__) || defined(_SGI)) && defined(CLOCK_SGI_CYCLE)

// Return clock ticks from performance counter [SGI/IRIX version];
FXTime fxgetticks(){
  const FXTime seconds=1000000000;
  struct timespec tp;
  clock_gettime(CLOCK_SGI_CYCLE,&tp);
  return tp.tv_sec*seconds+tp.tv_nsec;
  }

#elif defined(__GNUC__) && defined(__sparc_v9__)

// Return clock ticks from performance counter [GCC SPARC V9 version].
FXTime fxgetticks(){
  FXTime value;
  asm ("rd %%tick, %0" : "=r" (value));
  return value;
  }

#elif (_POSIX_C_SOURCE >= 199309L)

// Return clock ticks [POSIX fallback version].
FXTime fxgetticks(){
  const FXTime seconds=1000000000;
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC,&ts);
  return ts.tv_sec*seconds+ts.tv_nsec;                  // NOT accurate!
  }

#else

// Return clock ticks [UNIX fallback version].
FXTime fxgetticks(){
  const FXTime seconds=1000000000;
  const FXTime microseconds=1000;
  struct timeval tv;
  gettimeofday(&tv,NULL);
  return tv.tv_sec*seconds+tv.tv_usec*microseconds;     // NOT accurate!
  }

#endif

}

