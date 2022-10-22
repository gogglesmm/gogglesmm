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
#ifndef FXCPUID_H
#define FXCPUID_H


namespace FX {


// Runtime CPU features check (x86/x86-64 only)
enum {
  CPU_HAS_SSE       = 0x0000001,
  CPU_HAS_SSE2      = 0x0000002,
  CPU_HAS_SSE3      = 0x0000004,
  CPU_HAS_SSSE3     = 0x0000008,
  CPU_HAS_SSE41     = 0x0000010,
  CPU_HAS_SSE42     = 0x0000020,
  CPU_HAS_AVX       = 0x0000040,
  CPU_HAS_AVX2      = 0x0000080,
  CPU_HAS_FMA       = 0x0000100,
  CPU_HAS_AES       = 0x0000200,
  CPU_HAS_POPCNT    = 0x0000400,
  CPU_HAS_CX8       = 0x0000800,
  CPU_HAS_CX16      = 0x0001000,
  CPU_HAS_F16       = 0x0002000,
  CPU_HAS_BMI1      = 0x0004000,
  CPU_HAS_BMI2      = 0x0008000,
  CPU_HAS_SSE4A     = 0x0010000,
  CPU_HAS_ABM       = 0x0020000,
  CPU_HAS_XOP       = 0x0040000,
  CPU_HAS_FMA4      = 0x0080000,
  CPU_HAS_TBM       = 0x0100000,
  CPU_HAS_RAND      = 0x0200000
  };


/**
* On x86 or x86-64 processors, check the number of feature-requests
* available on the current processor.  If available, the largest feature
* request index+1 is returned, otherwise, this function returns 0.
* Extended caps levels may be obtained by passing 0x80000000 instead of
* zero.
*/
extern FXAPI FXuint fxCPUCaps(FXuint level=0);

/**
* Perform processor feature request of given level.
* The resulting output values are written into the array features,
* in the order EAX,EBX,ECX,EDX (for x86/x86-64).
* The function returns true if successful, and false if the processor
* does not support feature requests, or if the feature request level
* exceeds the number of levels reported by fxCPUCaps().
*/
extern FXAPI FXbool fxCPUGetCaps(FXuint level,FXuint features[]);

/**
* Perform processor extended feature request of given level, and given count
* parameter.
* The resulting output values are written into the array features,
* in the order EAX,EBX,ECX,EDX (for x86/x86-64).
* The function returns true if successful, and false if the processor
* does not support feature requests, or if the feature request level
* exceeds the number of levels reported by fxCPUCaps().
*/
extern FXAPI FXbool fxCPUGetXCaps(FXuint level,FXuint count,FXuint features[]);

/**
* Return CPU features.  For example, CPU_HAS_SSE2 means the CPU has
* integer vector math support.
*/
extern FXAPI FXuint fxCPUFeatures();

/**
* Return CPU Identification.  For example, on AMD processors this returns
* "AuthenticAMD", while on Intel processors it returns "GenuineIntel".
* Name should be at least 16 bytes, plus 1 for end-of-string.
*/
extern FXAPI FXbool fxCPUName(FXchar name[]);

}

#endif
