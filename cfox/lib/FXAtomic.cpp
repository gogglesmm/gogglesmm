/********************************************************************************
*                                                                               *
*                         A t o m i c   O p e r a t i o n s                     *
*                                                                               *
*********************************************************************************
* Copyright (C) 2006,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "FXAtomic.h"

/*
  Notes:

  - THIS LIBRARY REQUIRES LOCKING PRIMITIVES NOT PRESENT ON OLDER MACHINES.
    In the x86 world, you're good on PentiumPro or newer.

  - Code intended to run on ancient hardware CAN NOT use features only present
    on modern processors; you should write your software to use operating-system
    provided locking features such as mutexes and semaphores instead!

  - You can test using atomicsAvailable() to see if these primitives are atomic.

  - The API's are function calls rather than inlines in the header files because
    naive programmers don't specify "submodel" options and may not get the inten-
    ded code generation compared to what FOX was compiled with.  The problem is
    compilers tend to generate code for trailing-edge processor targets unless
    told to do otherwise.

  - We generate locking primitives using in-line assembly in preference to the
    GCC builtins, whenever possible.  Just because the version of GCC has the
    builtins doesn't mean it'll generate them since that depends on submodel
    options being set properly!

  - For optimal performance, keep your atomic variables in their own cache-line;
    threads monitoring atomic variables (e.g. spinlocks) will cause bus traffic
    whenever shared cachelines are updated, even if its not the variable itself
    but something close to it in the same cacheline.
*/


// New __atomic_XXX() builtins are available
#if ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ >= 7)))
#define HAVE_BUILTIN_ATOMIC 1
#endif

// Older __sync_XXX() builtins are available
#if ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ >= 1)))
#define HAVE_BUILTIN_SYNC 1
#endif

// Can we use inline-assembly
#if (defined(__GNUC__) || defined(__INTEL_COMPILER))
#define HAVE_INLINE_ASSEMBLY 1
#endif

using namespace FX;

namespace FX {

/*******************************************************************************/

// Atomics are available
FXbool atomicsAvailable(){
#if defined(WIN32) && (_MSC_VER >= 1500)
  return true;
#elif (defined(HAVE_INLINE_ASSEMBLY) && defined(__i386__))
  return true;
#elif (defined(HAVE_INLINE_ASSEMBLY) && defined(__x86_64__))
  return true;
#elif defined(HAVE_BUILTIN_ATOMIC)
  return true;
#elif defined(HAVE_BUILTIN_SYNC)
  return true;
#else
  return false;
#endif
  }


// Load/store fence
void atomicThreadFence(){
#if defined(WIN32) && (_MSC_VER >= 1500)
  _ReadWriteBarrier();
#elif defined(HAVE_BUILTIN_ATOMIC)
  __atomic_thread_fence(__ATOMIC_SEQ_CST);
#elif defined(HAVE_BUILTIN_SYNC)
  __sync_synchronize();
#endif
  }


/*******************************************************************************/

// Atomic read; necessary as it serializes reads and writes prior to this one
FXint atomicRead(volatile FXint* ptr){
#if defined(WIN32) && (_MSC_VER >= 1500)
  return _InterlockedCompareExchange((volatile LONG*)ptr,0,0);
#elif defined(HAVE_BUILTIN_ATOMIC)
  return __atomic_load_n(ptr,__ATOMIC_SEQ_CST);
#elif defined(HAVE_BUILTIN_SYNC)
  return __sync_val_compare_and_swap(ptr,0,0);
#elif (defined(HAVE_INLINE_ASSEMBLY) && (defined(__i386__) || defined(__x86_64__)))
  FXint ret=0;
  __asm__ __volatile__("lock\n\t"
                       "cmpxchgl %2, (%1)\n\t" : "=a"(ret) : "r"(ptr), "r"(ret), "a"(ret) : "memory", "cc");
  return ret;
#else
  return *ptr;
#endif
  }


// Atomic write, ensure visibility of written variable
void atomicWrite(volatile FXint* ptr,FXint v){
#if defined(WIN32) && (_MSC_VER >= 1500)
  *ptr=v;
  _ReadWriteBarrier();
#elif defined(HAVE_BUILTIN_ATOMIC)
  __atomic_store_n(ptr,v,__ATOMIC_SEQ_CST);
#elif defined(HAVE_BUILTIN_SYNC)
  *ptr=v;
  __sync_synchronize();
#elif (defined(HAVE_INLINE_ASSEMBLY) && (defined(__i386__) || defined(__x86_64__)))
  *ptr=v;
  __asm__ __volatile__("mfence\n\t" : : : "memory");
#else
  *ptr=v;
#endif
  }


// Atomically set variable at ptr to v, and return its old contents
FXint atomicSet(volatile FXint* ptr,FXint v){
#if defined(WIN32) && (_MSC_VER >= 1500)
  return _InterlockedExchange((volatile LONG*)ptr,v);
#elif (defined(HAVE_INLINE_ASSEMBLY) && (defined(__i386__) || defined(__x86_64__)))
  FXint ret=v;
  __asm__ __volatile__("xchgl %0, (%1)\n\t" : "=r"(ret) : "r"(ptr), "0"(ret) : "memory", "cc");
  return ret;
#elif defined(HAVE_BUILTIN_SYNC)
  return __sync_lock_test_and_set(ptr,v);
#else
  FXint ret=*ptr;
  *ptr=v;
  return ret;
#endif
  }


// Atomically add v to variable at ptr, and return its old contents
FXint atomicAdd(volatile FXint* ptr,FXint v){
#if defined(WIN32) && (_MSC_VER >= 1500)
  return _InterlockedExchangeAdd((volatile LONG*)ptr,v);
#elif (defined(HAVE_INLINE_ASSEMBLY) && (defined(__i386__) || defined(__x86_64__)))
  FXint ret=v;
  __asm__ __volatile__ ("lock\n\t"
                        "xaddl %0, (%1)\n\t" : "=r"(ret) : "r"(ptr), "0"(ret) : "memory", "cc");
  return ret;
#elif defined(HAVE_BUILTIN_SYNC)
  return __sync_fetch_and_add(ptr,v);
#else
  FXint ret=*ptr;
  *ptr+=v;
  return ret;
#endif
  }


// Atomically compare variable at ptr against expect, setting it to v if equal; returns the old value at ptr
FXint atomicCas(volatile FXint* ptr,FXint expect,FXint v){
#if defined(WIN32) && (_MSC_VER >= 1500)
  return _InterlockedCompareExchange((volatile LONG*)ptr,(LONG)v,(LONG)expect);
#elif (defined(HAVE_INLINE_ASSEMBLY) && (defined(__i386__) || defined(__x86_64__)))
  FXint ret;
  __asm__ __volatile__("lock\n\t"
                       "cmpxchgl %2, (%1)\n\t" : "=a"(ret) : "r"(ptr), "r"(v), "a"(expect) : "memory", "cc");
  return ret;
#elif defined(HAVE_BUILTIN_SYNC)
  return __sync_val_compare_and_swap(ptr,expect,v);
#else
  FXint ret=*ptr;
  if(*ptr==expect){
    *ptr=v;
    }
  return ret;
#endif
  }


// Atomically compare variable at ptr against expect, setting it to v if equal and return true, or false otherwise
FXbool atomicBoolCas(volatile FXint* ptr,FXint expect,FXint v){
#if defined(WIN32) && (_MSC_VER >= 1500)
  return (_InterlockedCompareExchange((volatile LONG*)ptr,(LONG)v,(LONG)expect)==(LONG)expect);
#elif (defined(HAVE_INLINE_ASSEMBLY) && (defined(__i386__) || defined(__x86_64__)))
  FXbool ret;
  __asm__ __volatile__ ("lock\n\t"
                        "cmpxchgl %2, (%1)\n\t"
                        "sete   %%al\n\t"
                        "andl   $1, %%eax\n\t" : "=a"(ret) : "r"(ptr), "r"(v), "a"(expect) : "memory", "cc");
  return ret;
#elif defined(HAVE_BUILTIN_SYNC)
  return __sync_bool_compare_and_swap(ptr,expect,v);
#else
  if(*ptr==expect){
    *ptr=v;
    return true;
    }
  return false;
#endif
  }

/*******************************************************************************/

// Atomically set variable at ptr to v, and return its old contents
FXlong atomicSet(volatile FXlong* ptr,FXlong v){
#if defined(WIN32) && (_MSC_VER >= 1800)
  return _InterlockedExchange64((volatile LONG64*)ptr,v);
#elif (defined(HAVE_INLINE_ASSEMBLY) && defined(__i386__) && (defined(__PIC__) || defined(__PIE__)))
  FXlong ret;
  __asm__ __volatile__ ("xchgl %%esi, %%ebx\n\t"
                        "1:\n\t"
                        "lock\n\t"
                        "cmpxchg8b (%1)\n\t"
                        "jnz 1b\n\t"
                        "xchgl %%esi, %%ebx\n\t" : "=A"(ret) : "D"(ptr), "S"((FXint)v), "c"((FXint)(v>>32)), "A"(*ptr) : "memory", "cc");
  return ret;
#elif (defined(HAVE_INLINE_ASSEMBLY) && defined(__i386__))
  FXlong ret;
  __asm__ __volatile__ ("1:\n\t"
                        "lock\n\t"
                        "cmpxchg8b (%1)\n\t"
                        "jnz 1b\n\t" : "=A"(ret) : "D"(ptr), "b"((FXint)v), "c"((FXint)(v>>32)), "A"(*ptr) : "memory", "cc");
  return ret;
#elif (defined(HAVE_INLINE_ASSEMBLY) && defined(__x86_64__))
  FXlong ret;
  __asm__ __volatile__("xchgq %0,(%1)\n\t" : "=r"(ret) : "r"(ptr), "0"(v) : "memory", "cc");
  return ret;
#elif defined(HAVE_BUILTIN_ATOMIC)
  return __atomic_exchange_n(ptr,v,__ATOMIC_SEQ_CST);
#elif defined(HAVE_BUILTIN_SYNC)
  return __sync_lock_test_and_set(ptr,v);
#else
  FXlong ret=*ptr;
  *ptr=v;
  return ret;
#endif
  }


// Atomically add v to variable at ptr, and return its old contents
FXlong atomicAdd(volatile FXlong* ptr,FXlong v){
#if defined(WIN32) && (_MSC_VER >= 1800)
  return _InterlockedExchangeAdd64((volatile LONG64*)ptr,v);
#elif defined(HAVE_BUILTIN_ATOMIC)
  return __atomic_fetch_add(ptr,v,__ATOMIC_SEQ_CST);
#elif defined(HAVE_BUILTIN_SYNC)
  return __sync_fetch_and_add(ptr,v);
#elif (defined(HAVE_INLINE_ASSEMBLY) && defined(__i386__) && (defined(__PIC__) || defined(__PIE__)))
  FXuint inclo=v;
  FXuint inchi=(v>>32);
  FXlong ret;
  __asm __volatile("movl %%ebx, %%esi\n\t"
                   "1:\n\t"
                   "movl %2, %%ebx\n\t"
                   "movl %3, %%ecx\n\t"
                   "addl %%eax, %%ebx\n\t"
                   "addc %%edx, %%ecx\n\t"
                   "lock\n\t"
                   "cmpxchg8b (%1)\n\t"
                   "jnz 1b\n\t"
                   "movl %%esi, %%ebx\n\t" : "=A"(ret) : "D"(ptr), "m"(inclo), "m"(inchi), "A"(*ptr) : "esi", "memory", "cc");
  return ret;
#elif (defined(HAVE_INLINE_ASSEMBLY) && defined(__i386__))
  FXuint inclo=v;
  FXuint inchi=(v>>32);
  FXlong ret;
  __asm __volatile("1:\n\t"
                   "movl %2, %%ebx\n\t"
                   "movl %3, %%ecx\n\t"
                   "addl %%eax, %%ebx\n\t"
                   "addc %%edx, %%ecx\n\t"
                   "lock\n\t"
                   "cmpxchg8b (%1)\n\t"
                   "jnz 1b\n\t" : "=A"(ret) : "D"(ptr), "m"(inclo), "m"(inchi), "A"(*ptr) : "memory", "cc");
  return ret;
#elif (defined(HAVE_INLINE_ASSEMBLY) && defined(__x86_64__))
  FXlong ret;
  __asm__ __volatile__ ("lock\n\t"
                        "xaddq %0,(%1)\n\t" : "=r"(ret) : "r"(ptr), "0"(v) : "memory", "cc");
  return ret;
#else
  FXlong ret=*ptr;
  *ptr+=v;
  return ret;
#endif
  }


// Atomically compare variable at ptr against expect, setting it to v if equal; returns the old value at ptr
FXlong atomicCas(volatile FXlong* ptr,FXlong expect,FXlong v){
#if defined(WIN32) && (_MSC_VER >= 1800)
  return _InterlockedCompareExchange64((volatile LONG64*)ptr,v,expect);
#elif defined(HAVE_BUILTIN_SYNC)
  return __sync_val_compare_and_swap(ptr,expect,v);
#elif (defined(HAVE_INLINE_ASSEMBLY) && defined(__i386__) && (defined(__PIC__) || defined(__PIE__)))
  FXlong ret;
  __asm__ __volatile__ ("xchgl %%esi, %%ebx\n\t"
                        "lock\n\t"
                        "cmpxchg8b (%1)\n\t"
                        "movl %%esi, %%ebx\n\t" : "=A"(ret) : "D"(ptr), "S"((FXuint)v), "c"((FXuint)(v>>32)), "A"(expect) : "memory", "cc");
  return ret;
#elif (defined(HAVE_INLINE_ASSEMBLY) && defined(__i386__))
  FXlong ret;
  __asm__ __volatile__ ("lock\n\t"
                        "cmpxchg8b (%1)\n\t" : "=A"(ret) : "D"(ptr), "b"((FXuint)v), "c"((FXuint)(v>>32)), "A"(expect) : "memory", "cc");
  return ret;
#elif (defined(HAVE_INLINE_ASSEMBLY) && defined(__x86_64__))
  FXlong ret;
  __asm__ __volatile__("lock\n\t"
                       "cmpxchgq %2,(%1)\n\t" : "=a"(ret) : "r"(ptr), "r"(v), "a"(expect) : "memory", "cc");
  return ret;
#else
  FXlong ret=*ptr;
  if(*ptr==expect){
    *ptr=v;
    }
  return ret;
#endif
  }


// Atomically compare variable at ptr against expect, setting it to v if equal and return true, or false otherwise
FXbool atomicBoolCas(volatile FXlong* ptr,FXlong expect,FXlong v){
#if defined(WIN32) && (_MSC_VER >= 1800)
  return (_InterlockedCompareExchange64((volatile LONG64*)ptr,v,expect)==expect);
#elif defined(HAVE_BUILTIN_SYNC)
  return __sync_bool_compare_and_swap(ptr,expect,v);
#elif (defined(HAVE_INLINE_ASSEMBLY) && defined(__i386__) && (defined(__PIC__) || defined(__PIE__)))
  FXbool ret;
  __asm__ __volatile__ ("xchgl %%esi, %%ebx\n\t"
                        "lock\n\t"
                        "cmpxchg8b (%1)\n\t"
                        "setz %%al\n\t"
                        "andl $1, %%eax\n\t"
                        "xchgl %%esi, %%ebx\n\t" : "=A"(ret) : "D"(ptr), "S"((FXuint)v), "c"((FXuint)(v>>32)), "A"(expect) : "memory", "cc");
  return ret;
#elif (defined(HAVE_INLINE_ASSEMBLY) && defined(__i386__))
  FXbool ret;
  __asm__ __volatile__ ("lock\n\t"
                        "cmpxchg8b (%1)\n\t"
                        "setz %%al\n\t"
                        "andl $1, %%eax\n\t" : "=a"(ret) : "D"(ptr), "b"((FXuint)v), "c"((FXuint)(v>>32)), "A"(expect) : "memory", "cc");
  return ret;
#elif (defined(HAVE_INLINE_ASSEMBLY) && defined(__x86_64__))
  FXbool ret;
  __asm__ __volatile__ ("lock\n\t"
                        "cmpxchgq %2,(%1)\n\t"
                        "sete %%al\n\t"
                        "andq $1, %%rax\n\t" : "=a"(ret) : "r"(ptr), "r"(v), "a"(expect) : "memory", "cc");
  return ret;
#else
  if(*ptr==expect){
    *ptr=v;
    return true;
    }
  return false;
#endif
  }


/*******************************************************************************/

// Atomically set variable at ptr to v, and return its old contents
FXulong atomicSet(volatile FXulong* ptr,FXulong v){
#if defined(WIN32) && (_MSC_VER >= 1800)
  return _InterlockedExchange64((volatile LONG64*)ptr,v);
#elif (defined(HAVE_INLINE_ASSEMBLY) && defined(__i386__) && (defined(__PIC__) || defined(__PIE__)))
  FXulong ret;
  __asm__ __volatile__ ("xchgl %%esi, %%ebx\n\t"
                        "1:\n\t"
                        "lock\n\t"
                        "cmpxchg8b (%1)\n\t"
                        "jnz 1b\n\t"
                        "xchgl %%esi, %%ebx\n\t" : "=A"(ret) : "D"(ptr), "S"((FXuint)v), "c"((FXuint)(v>>32)), "A"(*ptr) : "memory", "cc");
  return ret;
#elif (defined(HAVE_INLINE_ASSEMBLY) && defined(__i386__))
  FXulong ret;
  __asm__ __volatile__ ("1:\n\t"
                        "lock\n\t"
                        "cmpxchg8b (%1)\n\t"
                        "jnz 1b\n\t" : "=A"(ret) : "D"(ptr), "b"((FXuint)v), "c"((FXuint)(v>>32)), "A"(*ptr) : "memory", "cc");
  return ret;
#elif (defined(HAVE_INLINE_ASSEMBLY) && defined(__x86_64__))
  FXulong ret;
  __asm__ __volatile__("xchgq %0,(%1)\n\t" : "=r"(ret) : "r"(ptr), "0"(v) : "memory", "cc");
  return ret;
#elif defined(HAVE_BUILTIN_ATOMIC)
  return __atomic_exchange_n(ptr,v,__ATOMIC_SEQ_CST);
#elif defined(HAVE_BUILTIN_SYNC)
  return __sync_lock_test_and_set(ptr,v);
#else
  FXulong ret=*ptr;
  *ptr=v;
  return ret;
#endif
  }


// Atomically add v to variable at ptr, and return its old contents
FXulong atomicAdd(volatile FXulong* ptr,FXulong v){
#if defined(WIN32) && (_MSC_VER >= 1800)
  return _InterlockedExchangeAdd64((volatile LONG64*)ptr,v);
#elif defined(HAVE_BUILTIN_ATOMIC)
  return __atomic_fetch_add(ptr,v,__ATOMIC_SEQ_CST);
#elif defined(HAVE_BUILTIN_SYNC)
  return __sync_fetch_and_add(ptr,v);
#elif (defined(HAVE_INLINE_ASSEMBLY) && defined(__i386__) && (defined(__PIC__) || defined(__PIE__)))
  FXuint inclo=v;
  FXuint inchi=(v>>32);
  FXulong ret;
  __asm __volatile("movl %%ebx, %%esi\n\t"
                   "1:\n\t"
                   "movl %2, %%ebx\n\t"
                   "movl %3, %%ecx\n\t"
                   "addl %%eax, %%ebx\n\t"
                   "addc %%edx, %%ecx\n\t"
                   "lock\n\t"
                   "cmpxchg8b (%1)\n\t"
                   "jnz 1b\n\t"
                   "movl %%esi, %%ebx\n\t" : "=A"(ret) : "D"(ptr), "m"(inclo), "m"(inchi), "A"(*ptr) : "esi", "memory", "cc");
  return ret;
#elif (defined(HAVE_INLINE_ASSEMBLY) && defined(__i386__))
  FXuint inclo=v;
  FXuint inchi=(v>>32);
  FXulong ret;
  __asm __volatile("1:\n\t"
                   "movl %2, %%ebx\n\t"
                   "movl %3, %%ecx\n\t"
                   "addl %%eax, %%ebx\n\t"
                   "addc %%edx, %%ecx\n\t"
                   "lock\n\t"
                   "cmpxchg8b (%1)\n\t"
                   "jnz 1b\n\t" : "=A"(ret) : "D"(ptr), "m"(inclo), "m"(inchi), "A"(*ptr) : "memory", "cc");
  return ret;
#elif (defined(HAVE_INLINE_ASSEMBLY) && defined(__x86_64__))
  FXulong ret;
  __asm__ __volatile__ ("lock\n\t"
                        "xaddq %0,(%1)\n\t" : "=r"(ret) : "r"(ptr), "0"(v) : "memory", "cc");
  return ret;
#else
  FXulong ret=*ptr;
  *ptr+=v;
  return ret;
#endif
  }


// Atomically compare variable at ptr against expect, setting it to v if equal; returns the old value at ptr
FXulong atomicCas(volatile FXulong* ptr,FXulong expect,FXulong v){
#if defined(WIN32) && (_MSC_VER >= 1800)
  return _InterlockedCompareExchange64((volatile LONG64*)ptr,v,expect);
#elif defined(HAVE_BUILTIN_SYNC)
  return __sync_val_compare_and_swap(ptr,expect,v);
#elif (defined(HAVE_INLINE_ASSEMBLY) && defined(__i386__) && (defined(__PIC__) || defined(__PIE__)))
  FXulong ret;
  __asm__ __volatile__ ("xchgl %%esi, %%ebx\n\t"
                        "lock\n\t"
                        "cmpxchg8b (%1)\n\t"
                        "movl %%esi, %%ebx\n\t" : "=A"(ret) : "D"(ptr), "S"((FXuint)v), "c"((FXuint)(v>>32)), "A"(expect) : "memory", "cc");
  return ret;
#elif (defined(HAVE_INLINE_ASSEMBLY) && defined(__i386__))
  FXulong ret;
  __asm__ __volatile__ ("lock\n\t"
                        "cmpxchg8b (%1)\n\t" : "=A"(ret) : "D"(ptr), "b"((FXuint)v), "c"((FXuint)(v>>32)), "A"(expect) : "memory", "cc");
  return ret;
#elif (defined(HAVE_INLINE_ASSEMBLY) && defined(__x86_64__))
  FXulong ret;
  __asm__ __volatile__("lock\n\t"
                       "cmpxchgq %2,(%1)\n\t" : "=a"(ret) : "r"(ptr), "r"(v), "a"(expect) : "memory", "cc");
  return ret;
#else
  FXulong ret=*ptr;
  if(*ptr==expect){
    *ptr=v;
    }
  return ret;
#endif
  }


// Atomically compare variable at ptr against expect, setting it to v if equal and return true, or false otherwise
FXbool atomicBoolCas(volatile FXulong* ptr,FXulong expect,FXulong v){
#if defined(WIN32) && (_MSC_VER >= 1800)
  return (_InterlockedCompareExchange64((volatile LONG64*)ptr,v,expect)==expect);
#elif defined(HAVE_BUILTIN_SYNC)
  return __sync_bool_compare_and_swap(ptr,expect,v);
#elif (defined(HAVE_INLINE_ASSEMBLY) && defined(__i386__) && (defined(__PIC__) || defined(__PIE__)))
  FXbool ret;
  __asm__ __volatile__ ("xchgl %%esi, %%ebx\n\t"
                        "lock\n\t"
                        "cmpxchg8b (%1)\n\t"
                        "setz %%al\n\t"
                        "andl $1, %%eax\n\t"
                        "xchgl %%esi, %%ebx\n\t" : "=A"(ret) : "D"(ptr), "S"((FXuint)v), "c"((FXuint)(v>>32)), "A"(expect) : "memory", "cc");
  return ret;
#elif (defined(HAVE_INLINE_ASSEMBLY) && defined(__i386__))
  FXbool ret;
  __asm__ __volatile__ ("lock\n\t"
                        "cmpxchg8b (%1)\n\t"
                        "setz %%al\n\t"
                        "andl $1, %%eax\n\t" : "=a"(ret) : "D"(ptr), "b"((FXuint)v), "c"((FXuint)(v>>32)), "A"(expect) : "memory", "cc");
  return ret;
#elif (defined(HAVE_INLINE_ASSEMBLY) && defined(__x86_64__))
  FXbool ret;
  __asm__ __volatile__ ("lock\n\t"
                        "cmpxchgq %2,(%1)\n\t"
                        "sete %%al\n\t"
                        "andq $1, %%rax\n\t" : "=a"(ret) : "r"(ptr), "r"(v), "a"(expect) : "memory", "cc");
  return ret;
#else
  if(*ptr==expect){
    *ptr=v;
    return true;
    }
  return false;
#endif
  }

/*******************************************************************************/

// Atomic read; necessary as it serializes reads and writes prior to this one
FXuint atomicRead(volatile FXuint* ptr){
#if defined(WIN32) && (_MSC_VER >= 1500)
  return _InterlockedCompareExchange((volatile LONG*)ptr,0,0);
#elif defined(HAVE_BUILTIN_ATOMIC)
  return __atomic_load_n(ptr,__ATOMIC_SEQ_CST);
#elif defined(HAVE_BUILTIN_SYNC)
  return __sync_val_compare_and_swap(ptr,0,0);
#elif (defined(HAVE_INLINE_ASSEMBLY) && (defined(__i386__) || defined(__x86_64__)))
  FXuint ret=0;
  __asm__ __volatile__("lock\n\t"
                       "cmpxchgl %2, (%1)\n\t" : "=a"(ret) : "r"(ptr), "r"(ret), "a"(ret) : "memory", "cc");
  return ret;
#else
  return *ptr;
#endif
  }


// Atomic write, ensure visibility of written variable
void atomicWrite(volatile FXuint* ptr,FXuint v){
#if defined(WIN32) && (_MSC_VER >= 1500)
  *ptr=v;
  _ReadWriteBarrier();
#elif defined(HAVE_BUILTIN_ATOMIC)
  __atomic_store_n(ptr,v,__ATOMIC_SEQ_CST);
#elif defined(HAVE_BUILTIN_SYNC)
  *ptr=v;
  __sync_synchronize();
#elif (defined(HAVE_INLINE_ASSEMBLY) && (defined(__i386__) || defined(__x86_64__)))
  *ptr=v;
  __asm__ __volatile__("mfence\n\t" : : : "memory");
#else
  *ptr=v;
#endif
  }


// Atomically set variable at ptr to v, and return its old contents
FXuint atomicSet(volatile FXuint* ptr,FXuint v){
#if defined(WIN32) && (_MSC_VER >= 1500)
  return _InterlockedExchange((volatile LONG*)ptr,v);
#elif (defined(HAVE_INLINE_ASSEMBLY) && (defined(__i386__) || defined(__x86_64__)))
  FXuint ret=v;
  __asm__ __volatile__("xchgl %0, (%1)\n\t" : "=r"(ret) : "r"(ptr), "0"(ret) : "memory", "cc");
  return ret;
#elif defined(HAVE_BUILTIN_SYNC)
  return __sync_lock_test_and_set(ptr,v);
#else
  FXuint ret=*ptr;
  *ptr=v;
  return ret;
#endif
  }


// Atomically add v to variable at ptr, and return its old contents
FXuint atomicAdd(volatile FXuint* ptr,FXuint v){
#if defined(WIN32) && (_MSC_VER >= 1500)
  return _InterlockedExchangeAdd((volatile LONG*)ptr,v);
#elif (defined(HAVE_INLINE_ASSEMBLY) && (defined(__i386__) || defined(__x86_64__)))
  FXuint ret=v;
  __asm__ __volatile__ ("lock\n\t"
                        "xaddl %0, (%1)\n\t" : "=r"(ret) : "r"(ptr), "0"(ret) : "memory", "cc");
  return ret;
#elif defined(HAVE_BUILTIN_SYNC)
  return __sync_fetch_and_add(ptr,v);
#else
  FXuint ret=*ptr;
  *ptr+=v;
  return ret;
#endif
  }


// Atomically compare variable at ptr against expect, setting it to v if equal; returns the old value at ptr
FXuint atomicCas(volatile FXuint* ptr,FXuint expect,FXuint v){
#if defined(WIN32) && ((_MSC_VER >= 1500))
  return _InterlockedCompareExchange((volatile LONG*)ptr,(LONG)v,(LONG)expect);
#elif (defined(HAVE_INLINE_ASSEMBLY) && (defined(__i386__) || defined(__x86_64__)))
  FXuint ret;
  __asm__ __volatile__("lock\n\t"
                       "cmpxchgl %2, (%1)\n\t" : "=a"(ret) : "r"(ptr), "r"(v), "a"(expect) : "memory", "cc");
  return ret;
#elif defined(HAVE_BUILTIN_SYNC)
  return __sync_val_compare_and_swap(ptr,expect,v);
#else
  FXuint ret=*ptr;
  if(*ptr==expect){
    *ptr=v;
    }
  return ret;
#endif
  }


// Atomically compare variable at ptr against expect, setting it to v if equal and return true, or false otherwise
FXbool atomicBoolCas(volatile FXuint* ptr,FXuint expect,FXuint v){
#if defined(WIN32) && ((_MSC_VER >= 1500))
  return (_InterlockedCompareExchange((volatile LONG*)ptr,(LONG)v,(LONG)expect)==(LONG)expect);
#elif (defined(HAVE_INLINE_ASSEMBLY) && (defined(__i386__) || defined(__x86_64__)))
  FXbool ret;
  __asm__ __volatile__ ("lock\n\t"
                        "cmpxchgl %2, (%1)\n\t"
                        "sete   %%al\n\t"
                        "andl   $1, %%eax\n\t" : "=a"(ret) : "r"(ptr), "r"(v), "a"(expect) : "memory", "cc");
  return ret;
#elif defined(HAVE_BUILTIN_SYNC)
  return __sync_bool_compare_and_swap(ptr,expect,v);
#else
  if(*ptr==expect){
    *ptr=v;
    return true;
    }
  return false;
#endif
  }

/*******************************************************************************/

// Atomic read; necessary as it serializes reads and writes prior to this one
FXptr atomicRead(volatile FXptr* ptr){
#if defined(WIN32) && defined(_WIN64) && (_MSC_VER >= 1500)
  return (FXptr)_InterlockedCompareExchange64((LONGLONG*)ptr,0,0);
#elif defined(WIN32) && (MSC_VER >=1500)
  return (FXptr)_InterlockedCompareExchange((LONG*)ptr,0,0);
#elif defined(HAVE_BUILTIN_ATOMIC)
  return __atomic_load_n(ptr,__ATOMIC_SEQ_CST);
#elif (defined(HAVE_INLINE_ASSEMBLY) && defined(__i386__))
  FXptr ret=0;
  __asm__ __volatile__("lock\n\t"
                       "cmpxchgl %2, (%1)\n\t" : "=a"(ret) : "r"(ptr), "r"(ret), "a"(ret) : "memory", "cc");
  return ret;
#elif ((defined(__GNUC__) || defined(__INTEL_COMPILER)) && defined(__x86_64__))
  FXptr ret=0;
  __asm__ __volatile__("lock\n\t"
                       "cmpxchgq %2, (%1)\n\t" : "=a"(ret) : "r"(ptr), "r"(ret), "a"(ret) : "memory", "cc");
  return ret;
#elif defined(HAVE_BUILTIN_SYNC)
  return __sync_val_compare_and_swap(ptr,0,0);
#else
  return *ptr;
#endif
  }


// Atomic write, ensure visibility of written variable
void atomicWrite(volatile FXptr* ptr,FXptr v){
#if defined(WIN32) && (_MSC_VER >= 1500)
  *ptr=v;
  _ReadWriteBarrier();
#elif defined(HAVE_BUILTIN_ATOMIC)
  __atomic_store_n(ptr,v,__ATOMIC_SEQ_CST);
#elif defined(HAVE_BUILTIN_SYNC)
  *ptr=v;
  __sync_synchronize();
#elif (defined(HAVE_INLINE_ASSEMBLY) && (defined(__i386__) || defined(__x86_64__)))
  *ptr=v;
  __asm__ __volatile__("mfence\n\t" : : : "memory");
#else
  *ptr=v;
#endif
  }


// Atomically set pointer variable at ptr to v, and return its old contents
FXptr atomicSet(volatile FXptr* ptr,FXptr v){
#if (defined(WIN32) && (_MSC_VER >= 1700))
  return (FXptr)_InterlockedExchangePointer(ptr,v);
#elif (defined(WIN32) && defined(_WIN64) && (_MSC_VER >= 1500))
  FXptr result;
  do{                   // Stop-gap measure: VC++ 2008 doesn't have _InterlockedExchangePointer()
    result=*ptr;        // Only important for 64-bit; on 32-bit we just use _InterlockedExchange()
    }
  while(_InterlockedCompareExchange64((LONGLONG*)ptr,(LONGLONG)v,(LONGLONG)result)!=(LONG)result);
  return result;
#elif (defined(WIN32) && (_MSC_VER >= 1500))
  return (FXptr)_InterlockedExchange((LONG*)ptr,(LONG)v);
#elif (defined(HAVE_INLINE_ASSEMBLY) && defined(__i386__))
  FXptr ret=v;
  __asm__ __volatile__("xchgl %0, (%1)\n\t" : "=r"(ret) : "r"(ptr), "0"(ret) : "memory", "cc");
  return ret;
#elif (defined(HAVE_INLINE_ASSEMBLY) && defined(__x86_64__))
  FXptr ret=v;
  __asm__ __volatile__("xchgq %0, (%1)\n\t" : "=r"(ret) : "r"(ptr), "0"(ret) : "memory", "cc");
  return ret;
#elif defined(HAVE_BUILTIN_SYNC)
  return __sync_lock_test_and_set(ptr,v);
#else
  FXptr ret=*ptr;
  *ptr=v;
  return ret;
#endif
  }

// _M_X64

// Atomically add v to pointer variable at ptr, and return its old contents
FXptr atomicAdd(volatile FXptr* ptr,FXival v){
#if (defined(WIN32) && defined(_WIN64) && (_MSC_VER >= 1500))
  return (FXptr)_InterlockedExchangeAdd64((volatile LONGLONG*)ptr,(LONGLONG)v);
#elif (defined(WIN32) && (_MSC_VER >= 1600))
  return (FXptr)_InterlockedExchangeAdd((volatile LONG*)ptr,(LONG)v);
#elif (defined(HAVE_INLINE_ASSEMBLY) && defined(__i386__))
  FXptr ret=(void*)v;
  __asm__ __volatile__ ("lock\n\t"
                        "xaddl %0, (%1)\n\t" : "=r"(ret) : "r"(ptr), "0"(ret) : "memory", "cc");
  return ret;
#elif (defined(HAVE_INLINE_ASSEMBLY) && defined(__x86_64__))
  FXptr ret=(FXptr)v;
  __asm__ __volatile__ ("lock\n\t"
                        "xaddq %0, (%1)\n\t" : "=r"(ret) : "r"(ptr), "0" (ret) : "memory", "cc");
  return ret;
#elif defined(HAVE_BUILTIN_SYNC)
  return __sync_fetch_and_add(ptr,(FXptr)v);
#else
  FXptr ret=*ptr;
  *((unsigned char**)ptr)+=v;
  return ret;
#endif
  }


// Atomically compare pointer variable at ptr against expect, setting it to v if equal; returns the old value at ptr
FXptr atomicCas(volatile FXptr* ptr,FXptr expect,FXptr v){
#if defined(WIN32) && defined(_WIN64) && (_MSC_VER >= 1500)
  return (FXptr)_InterlockedCompareExchange64((volatile LONGLONG*)ptr,(LONGLONG)v,(LONGLONG)expect);
#elif defined(WIN32) && (MSC_VER >=1500)
  return (FXptr)_InterlockedCompareExchange((volatile LONG*)ptr,(LONG)v,(LONG)expect);
#elif (defined(HAVE_INLINE_ASSEMBLY) && defined(__i386__))
  FXptr ret;
  __asm__ __volatile__("lock\n\t"
                       "cmpxchgl %2, (%1)\n\t" : "=a"(ret) : "r"(ptr), "r"(v), "a"(expect) : "memory", "cc");
  return ret;
#elif (defined(HAVE_INLINE_ASSEMBLY) && defined(__x86_64__))
  FXptr ret;
  __asm__ __volatile__("lock\n\t"
                       "cmpxchgq %2, (%1)\n\t" : "=a"(ret) : "r"(ptr), "r"(v), "a"(expect) : "memory", "cc");
  return ret;
#elif defined(HAVE_BUILTIN_SYNC)
  return __sync_val_compare_and_swap(ptr,expect,v);
#else
  FXptr ret=*ptr;
  if(*ptr==expect){
    *ptr=v;
    }
  return ret;
#endif
  }


// Atomically compare pointer variable at ptr against expect, setting it to v if equal and return true, or false otherwise
FXbool atomicBoolCas(volatile FXptr* ptr,FXptr expect,FXptr v){
#if defined(WIN32) && defined(_WIN64) && (_MSC_VER >= 1500)
  return (_InterlockedCompareExchange64((volatile LONGLONG*)ptr,(LONGLONG)v,(LONGLONG)expect)==(LONGLONG)expect);
#elif defined(WIN32) && (MSC_VER >=1500)
  return (_InterlockedCompareExchange((volatile LONG*)ptr,(LONG)v,(LONG)expect)==(LONG)expect);
#elif (defined(HAVE_INLINE_ASSEMBLY) && defined(__i386__))
  FXbool ret;
  __asm__ __volatile__ ("lock\n\t"
                        "cmpxchgl %2, (%1)\n\t"
                        "sete   %%al\n\t"
                        "andl   $1, %%eax\n\t" : "=a"(ret) : "r"(ptr), "r"(v), "a"(expect) : "memory", "cc");
  return ret;
#elif (defined(HAVE_INLINE_ASSEMBLY) && defined(__x86_64__))
  FXbool ret;
  __asm__ __volatile__ ("lock\n\t"
                        "cmpxchgq %2, (%1)\n\t"
                        "sete   %%al\n\t"
                        "andq   $1, %%rax\n\t" : "=a"(ret) : "r"(ptr), "r"(v), "a"(expect) : "memory", "cc");
  return ret;
#elif defined(HAVE_BUILTIN_SYNC)
  return __sync_bool_compare_and_swap(ptr,expect,v);
#else
  if(*ptr==expect){
    *ptr=v;
    return true;
    }
  return false;
#endif
  }


// Atomically compare pair of variables at ptr against (cmpa,cmpb), setting them to (a,b) if equal and return true, or false otherwise
FXbool atomicBoolDCas(FXptr volatile* ptr,FXptr cmpa,FXptr cmpb,FXptr a,FXptr b){
#if (defined(WIN32) && defined(_WIN64) && (_MSC_VER >= 1500))
  LONGLONG duet[2]={(LONGLONG)a,(LONGLONG)b};
  return !!(_InterlockedCompareExchange128((volatile LONGLONG*)ptr,(LONGLONG)cmpb,(LONGLONG)cmpa,duet));
#elif (defined(WIN32) && (_MSC_VER >= 1500))
  LONGLONG ab=(((LONGLONG)(FXuval)a)|((LONGLONG)(FXuval)b)<<32);
  LONGLONG compab=(((LONGLONG)(FXuval)cmpa)|((LONGLONG)(FXuval)cmpb)<<32);
  return (_InterlockedCompareExchange64((volatile LONGLONG*)ptr,ab,compab)==compab);
#elif (defined(HAVE_INLINE_ASSEMBLY) && defined(__i386__) && (defined(__PIC__) || defined(__PIE__)))
  FXbool ret;
  __asm__ __volatile__ ("xchgl  %%esi, %%ebx\n\t"
                        "lock\n\t"
                        "cmpxchg8b (%1)\n\t"
                        "setz   %%al\n\t"
                        "andl   $1, %%eax\n\t"
                        "xchgl  %%esi, %%ebx\n\t" : "=a"(ret) : "D"(ptr), "a"(cmpa), "d"(cmpb), "S"(a), "c"(b) : "memory", "cc");
  return ret;
#elif (defined(HAVE_INLINE_ASSEMBLY) && defined(__i386__))
  FXbool ret;
  __asm__ __volatile__ ("lock\n\t"
                        "cmpxchg8b (%1)\n\t"
                        "setz   %%al\n\t"
                        "andl   $1, %%eax\n\t" : "=a"(ret) : "D"(ptr), "a"(cmpa), "d"(cmpb), "b"(a), "c"(b) : "memory", "cc");
  return ret;
#elif (defined(HAVE_INLINE_ASSEMBLY) && defined(__x86_64__))
  FXbool ret;
  __asm__ __volatile__ ("lock\n\t"
                        "cmpxchg16b (%1)\n\t"
                        "setz   %%al\n\t"
                        "andq    $1, %%rax\n\t" : "=a"(ret) : "r"(ptr), "a"(cmpa), "d"(cmpb), "b"(a), "c"(b) : "memory", "cc");
  return ret;
/*
#elif (defined(HAVE_BUILTIN_SYNC) && defined(__LP64__))
  __uint128_t expectab=((__uint128_t)(FXuval)cmpa) | (((__uint128_t)(FXuval)cmpb)<<64);
  __uint128_t ab=((__uint128_t)(FXuval)a) | (((__uint128_t)(FXuval)b)<<64);
  return __sync_bool_compare_and_swap((__uint128_t*)ptr,expectab,ab);
#elif (defined(HAVE_BUILTIN_SYNC) && !defined(__LP64__))
  __uint64_t expectab=((__uint64_t)(FXuval)cmpa) | (((__uint64_t)(FXuval)cmpb)<<32);
  __uint64_t ab=((__uint64_t)(FXuval)a) | (((__uint64_t)(FXuval)b)<<32);
  return __sync_bool_compare_and_swap((__uint64_t*)ptr,expectab,ab);
*/
#else
  static __align(64) volatile FXint locks[256]={
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,   // Each spinlock lives in its own cacheline
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0
    };
  FXint which=(((FXuval)ptr)>>2)&0xF0;  // Avoid contention by associating different lock based on address
  while(atomicSet(&locks[which],1)){    // Spinlock to ensure access to variable
    }
  if(ptr[0]==cmpa && ptr[1]==cmpb){     // Change if equal
    ptr[0]=a;
    ptr[1]=b;
    atomicThreadFence();                // Memory fence to ensure visibility prior to releasing lock
    locks[which]=0;                     // Free spinlock
    return true;
    }
  locks[which]=0;                       // Free spinlock
  return false;
#endif
  }


}
