/********************************************************************************
*                                                                               *
*                              C P U I D   S u p p o r t                        *
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
#include "fxmath.h"
#include "fxcpuid.h"


/*
  Notes:
  - Obtain processor capabilities at runtime.
  - Utility API's to discover CPU vendor and CPU instruction-set extensions.
  - Only supported on x86 and x86-64 cpus.
  - Consult AMD and Intel Programming Manuals for details.
*/


using namespace FX;

/*******************************************************************************/

namespace FX {


// Return number of levels of CPUID feature-requests supported
FXuint fxCPUCaps(FXuint level){
#if defined(WIN32) && (defined(_M_IX86) || defined(_M_X64)) && (_MSC_VER >= 1500)
  FXint features[4];
  level&=0x80000000;
  __cpuid(features,level);
  return features[0]+1;
#elif ((defined(__GNUC__) || defined(__INTEL_COMPILER)) && defined(__i686__))
  FXuint eax,ebx,ecx,edx;
  level&=0x80000000;
  __asm__ __volatile__("xchgl %%ebx, %1 \n\t"  \
                       "cpuid           \n\t"  \
                       "xchgl %%ebx, %1 \n\t"  : "=a"(eax), "=r"(ebx), "=c"(ecx), "=d"(edx) : "0" (level) : "cc");
  return eax+1;
#elif ((defined(__GNUC__) || defined(__INTEL_COMPILER)) && defined(__x86_64__))
  FXuint eax,ebx,ecx,edx;
  level&=0x80000000;
  __asm__ __volatile__("cpuid \n\t" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "0" (level) : "cc");
  return eax+1;
#endif
  return 0;
  }


// Get CPU info
FXbool fxCPUGetCaps(FXuint level,FXuint features[]){
#if defined(WIN32) && (defined(_M_IX86) || defined(_M_X64)) && (_MSC_VER >= 1500)
  if(level<fxCPUCaps(level)){
    __cpuid((int*)features,level);
    return true;
    }
#elif ((defined(__GNUC__) || defined(__INTEL_COMPILER)) && defined(__i686__))
  if(level<fxCPUCaps(level)){
  __asm__ __volatile__("xchgl %%ebx, %1 \n\t"  \
                       "cpuid           \n\t"  \
                       "xchgl %%ebx, %1 \n\t"  : "=a"(features[0]), "=r"(features[1]), "=c"(features[2]), "=d"(features[3]) : "0" (level) : "cc");
    return true;
    }
#elif ((defined(__GNUC__) || defined(__INTEL_COMPILER)) && defined(__x86_64__))
  if(level<fxCPUCaps(level)){
    __asm__ __volatile__("cpuid \n\t" : "=a"(features[0]), "=b"(features[1]), "=c"(features[2]), "=d"(features[3]) : "0" (level) : "cc");
    return true;
    }
#endif
  return false;
  }


// Get CPU info
FXbool fxCPUGetXCaps(FXuint level,FXuint count,FXuint features[]){
#if defined(WIN32) && (defined(_M_IX86) || defined(_M_X64)) && (_MSC_VER >= 1500)
  if(level<fxCPUCaps(level)){
   __cpuidex((int*)features,level,count);
    return true;
    }
#elif ((defined(__GNUC__) || defined(__INTEL_COMPILER)) && defined(__i686__))
  if(level<fxCPUCaps(level)){
  __asm__ __volatile__("xchgl %%ebx, %1 \n\t"   \
                       "cpuid           \n\t"   \
                       "xchgl %%ebx, %1 \n\t" : "=a"(features[0]), "=r"(features[1]), "=c"(features[2]), "=d"(features[3]) : "0"(level), "2"(count) : "cc");
    return true;
    }
#elif ((defined(__GNUC__) || defined(__INTEL_COMPILER)) && defined(__x86_64__))
  if(level<fxCPUCaps(level)){
    __asm__ __volatile__("cpuid \n\t" : "=a"(features[0]), "=b"(features[1]), "=c"(features[2]), "=d"(features[3]) : "0"(level), "2"(count) : "cc");
    return true;
    }
#endif
  return false;
  }


// Return exciting features
FXuint fxCPUFeatures(){
  FXuint features[4];
  if(fxCPUGetCaps(1,features)){
    FXuint blank=(CPU_HAS_AVX|CPU_HAS_AVX2|CPU_HAS_FMA|CPU_HAS_FMA4|CPU_HAS_XOP);
    FXuint caps=0;
    if(FXBIT(features[2],0)) caps|=CPU_HAS_SSE3;
    if(FXBIT(features[3],8)) caps|=CPU_HAS_CX8;
    if(FXBIT(features[2],9)) caps|=CPU_HAS_SSSE3;
    if(FXBIT(features[2],12)) caps|=CPU_HAS_FMA;
    if(FXBIT(features[2],13)) caps|=CPU_HAS_CX16;
    if(FXBIT(features[2],19)) caps|=CPU_HAS_SSE41;
    if(FXBIT(features[2],20)) caps|=CPU_HAS_SSE42;
    if(FXBIT(features[2],23)) caps|=CPU_HAS_POPCNT;
    if(FXBIT(features[2],25)) caps|=CPU_HAS_AES;
    if(FXBIT(features[2],28)) caps|=CPU_HAS_AVX;
    if(FXBIT(features[2],29)) caps|=CPU_HAS_F16;        // Half-floats
    if(FXBIT(features[2],30)) caps|=CPU_HAS_RAND;
    if(FXBIT(features[3],25)) caps|=CPU_HAS_SSE;
    if(FXBIT(features[3],26)) caps|=CPU_HAS_SSE2;
    if(FXBIT(features[2],27)){                          // OSXSAVE
#if ((defined(__GNUC__) || defined(__INTEL_COMPILER)) && (defined(__i686__) || defined(__x86_64__)))
      FXuint lo,hi;
      __asm__ __volatile__(".byte 0x0f,0x01,0xd0" : "=a" (lo), "=d" (hi) : "c" (0));    // XGETBV ecx=0
      if((lo&6)==6) blank=0;                            // Don't blank out AVX, AVX2, FMA, FMA4, XOP later
#endif
// _xgetbv(0); // For _MSC_VER
      }
    if(fxCPUGetXCaps(7,0,features)){
      if(FXBIT(features[1],3)) caps|=CPU_HAS_BMI1;
      if(FXBIT(features[1],5)) caps|=CPU_HAS_AVX2;
      if(FXBIT(features[1],8)) caps|=CPU_HAS_BMI2;
      }
    if(fxCPUGetCaps(0,features) && (features[1]==0x68747541) && (features[2]==0x444d4163) && (features[3]==0x69746e65)){
      if(fxCPUGetCaps(0x80000001,features)){
        if(FXBIT(features[2],6)) caps|=CPU_HAS_SSE4A;
        if(FXBIT(features[2],5)) caps|=CPU_HAS_ABM;
        if(FXBIT(features[2],11)) caps|=CPU_HAS_XOP;
        if(FXBIT(features[2],16)) caps|=CPU_HAS_FMA4;
        if(FXBIT(features[2],21)) caps|=CPU_HAS_TBM;
        }
      }
    caps&=~blank;
    return caps;
    }
  return 0;
  }


// Return CPU Identification.
FXbool fxCPUName(FXchar name[]){
  FXuint features[4];
  if(fxCPUGetCaps(0,features)){
    name[0]=((char*)features)[4];
    name[1]=((char*)features)[5];
    name[2]=((char*)features)[6];
    name[3]=((char*)features)[7];
    name[4]=((char*)features)[12];
    name[5]=((char*)features)[13];
    name[6]=((char*)features)[14];
    name[7]=((char*)features)[15];
    name[8]=((char*)features)[8];
    name[9]=((char*)features)[9];
    name[10]=((char*)features)[10];
    name[11]=((char*)features)[11];
    name[12]='\0';
    return true;
    }
  name[0]='\0';
  return false;
  }

}
