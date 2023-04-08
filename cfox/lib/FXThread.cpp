/********************************************************************************
*                                                                               *
*                          T h r e a d   S u p p o r t                          *
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
#include "fxchar.h"
#include "fxmath.h"
#include "FXException.h"
#include "FXString.h"
#include "FXRunnable.h"
#include "FXAutoThreadStorageKey.h"
#include "FXThread.h"

/*
  Notes:

  - Note that the FXThreadID is only valid when busy==true, except insofar
    as when its used to harvest thread exit status like e.g. join!

  - The busy flag is set BEFORE actually spawning the thread, and reset
    if we were unable to spawn the thread.
    This is because we don't know how the thread creation is implemented:-
    its possible that the new thread may already be running for some time
    before pthread_create() returns successfully.
    We want to be sure that the flag reflects running state from within
    the newly spawned thread [we need this in several API's].

  - Note that cancel() also resets busy since it kills the thread
    right away; however join() doesn't because then we wait for the
    thread to finish normally.

  - About thread suspend/resume.  This does not work on Linux since there is
    no pthread equivalent for SuspendThread() and ResumeThread().  There is,
    however, an exceptionally inelegant solution in Boehm's GC code (file
    linux_threads.c).  But its so ugly we'd rather live without until a real
    suspend/resume facility is implemented in the linux kernel.

*/

using namespace FX;


namespace FX {

/*******************************************************************************/

// Generate one for the thread itself
FXAutoThreadStorageKey FXThread::selfKey;


// Initialize thread
FXThread::FXThread():tid(0),busy(false){
  }


// Return thread id of this thread object.
// Purposefully NOT inlined, the tid may be changed by another
// thread and therefore we must force the compiler to fetch
// this value fresh each time it is needed!
FXThreadID FXThread::id() const {
  return tid;
  }


// Return true if this thread is running
FXbool FXThread::running() const {
  return busy;
  }


// Change pointer to thread
void FXThread::self(FXThread* t){
  FXThread::selfKey.set(t);
  }


// Return pointer to calling thread
FXThread* FXThread::self(){
  return (FXThread*)FXThread::selfKey.get();
  }


// Start the thread; we associate the FXThread instance with this thread using thread-local
// storage accessed with self_key.
// A special FXThreadException may be used to unroll to this point and return a specific
// error code from the thread; any other FXException is silently caught here, and causes
// FXThread to return error-code of -1.
#if defined(WIN32)
unsigned int CALLBACK FXThread::function(void* ptr){
  FXThread *thread=(FXThread*)ptr;
  FXint code=-1;
  self(thread);
  try{
    code=thread->run();
    }
  catch(const FXThreadException& e){    // Graceful thread exit
    code=e.code();
    }
  catch(const FXException& e){          // Our kind of exceptions
    fxerror("FXThread: caught exception: %s.\n",e.what());
    }
  catch(...){                           // Other exceptions
    fxerror("FXThread: caught unknown exception.\n");
    }
  if(self()){ self()->busy=false; }
  return code;
  }
#else
void* FXThread::function(void* ptr){
  FXThread *thread=(FXThread*)ptr;
  FXint code=-1;
  self(thread);
  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,nullptr);
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,nullptr);
  try{
    code=thread->run();
    }
  catch(const FXThreadException& e){    // Graceful thread exit
    code=e.code();
    }
  catch(const FXException& e){          // Our kind of exceptions
    fxerror("FXThread: caught exception: %s.\n",e.what());
    }
  catch(...){                           // Other exceptions
    fxerror("FXThread: caught unknown exception.\n");
    }
  if(self()){ self()->busy=false; }
  return (void*)(FXival)code;
  }
#endif


// Start thread
FXbool FXThread::start(FXuval stacksize){
#if defined(WIN32)
  DWORD thd;
  if(busy){ fxerror("FXThread::start: thread already running.\n"); }
  if(tid){ fxerror("FXThread::start: thread still attached.\n"); }
  busy=true;
  if((tid=(FXThreadID)CreateThread(nullptr,stacksize,(LPTHREAD_START_ROUTINE)FXThread::function,this,0,&thd))==nullptr) busy=false;
//  if((tid=(FXThreadID)_beginthreadex(nullptr,stacksize,(LPTHREAD_START_ROUTINE)FXThread::function,this,0,&thd))==nullptr) busy=false;
#else
  pthread_attr_t attr;
  if(busy){ fxerror("FXThread::start: thread already running.\n"); }
  if(tid){ fxerror("FXThread::start: thread still attached.\n"); }
  pthread_attr_init(&attr);
  pthread_attr_setinheritsched(&attr,PTHREAD_INHERIT_SCHED);
  if(stacksize){ pthread_attr_setstacksize(&attr,stacksize); }
  busy=true;
#if defined(__USE_POSIX199506) || defined(__USE_UNIX98)
  sigset_t newset,oldset;
  sigfillset(&newset);
  pthread_sigmask(SIG_SETMASK,&newset,&oldset); // No signals except to main thread
  if(pthread_create((pthread_t*)&tid,&attr,FXThread::function,(void*)this)!=0) busy=false;
  pthread_sigmask(SIG_SETMASK,&oldset,nullptr);    // Restore old mask
#else
  if(pthread_create((pthread_t*)&tid,&attr,FXThread::function,(void*)this)!=0) busy=false;
#endif
  pthread_attr_destroy(&attr);
#endif
  return busy;
  }


// Suspend calling thread until thread is done
FXbool FXThread::join(FXint& code){
#if defined(WIN32)
  if(tid && WaitForSingleObject((HANDLE)tid,INFINITE)==WAIT_OBJECT_0){
    GetExitCodeThread((HANDLE)tid,(DWORD*)&code);
    CloseHandle((HANDLE)tid);
    tid=0;
    return true;
    }
  return false;
#else
  void *trc=nullptr;
  if(tid && pthread_join((pthread_t)tid,&trc)==0){
    code=(FXint)(FXival)trc;
    tid=0;
    return true;
    }
  return false;
#endif
  }


// Suspend calling thread until thread is done
FXbool FXThread::join(){
#if defined(WIN32)
  if(tid && WaitForSingleObject((HANDLE)tid,INFINITE)==WAIT_OBJECT_0){
    CloseHandle((HANDLE)tid);
    tid=0;
    return true;
    }
  return false;
#else
  if(tid && pthread_join((pthread_t)tid,nullptr)==0){
    tid=0;
    return true;
    }
  return false;
#endif
  }


// Cancel the thread
FXbool FXThread::cancel(){
#if defined(WIN32)
  if(tid){
    if(busy && TerminateThread((HANDLE)tid,0)) busy=false;
    if(CloseHandle((HANDLE)tid)){
      tid=0;
      return true;
      }
    }
  return false;
#else
  if(tid){
    if(busy && pthread_cancel((pthread_t)tid)==0) busy=false;
    if(pthread_join((pthread_t)tid,nullptr)==0){
      tid=0;
      return true;
      }
    }
  return false;
#endif
  }


// Detach thread
FXbool FXThread::detach(){
#if defined(WIN32)
  if(tid && CloseHandle((HANDLE)tid)){
    tid=0;
    return true;
    }
  return false;
#else
  if(tid && pthread_detach((pthread_t)tid)==0){
    tid=0;
    return true;
    }
  return false;
#endif
  }


// Exit calling thread
void FXThread::exit(FXint code){
#if defined(WIN32)
  if(self()){ self()->busy=false; }
  ExitThread(code);
//  _endthreadex(code);
#else
  if(self()){ self()->busy=false; }
  pthread_exit((void*)(FXival)code);
#endif
  }


// Yield the thread
void FXThread::yield(){
#if defined(WIN32)
  Sleep(0);
#else
  sched_yield();                // More portable than pthread_yield()
#endif
  }


// Processor pause/back-off
void FXThread::pause(){
#if defined(WIN32)
#if defined(_MSC_VER)
  YieldProcessor();
#endif
#elif (defined(__GNUC__) || defined(__INTEL_COMPILER)) && (defined(__i386__) || defined(__x86_64__))
  __asm__ __volatile__("rep; nop\n" : : : "memory" );
//  _mm_pause();
#elif defined(__GNUC__) && defined(__aarch64__)
  __asm__ __volatile__("yield" ::: "memory");
#endif
  }


// Get time in nanoseconds since Epoch
FXTime FXThread::time(){
#if defined(WIN32)
  const FXTime BIAS=FXLONG(116444736000000000);
  const FXTime HIGH=FXLONG(4294967296);
  FILETIME now;
  GetSystemTimeAsFileTime(&now);
  return (now.dwHighDateTime*HIGH+now.dwLowDateTime-BIAS)*100;
#elif (_POSIX_C_SOURCE >= 199309L)
  const FXTime seconds=1000000000;
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME,&ts);
  return ts.tv_sec*seconds+ts.tv_nsec;
#else
  const FXTime seconds=1000000000;
  const FXTime microseconds=1000;
  struct timeval tv;
  gettimeofday(&tv,nullptr);
  return tv.tv_sec*seconds+tv.tv_usec*microseconds;
#endif
  }


// Get steady time in nanoseconds since some arbitrary start time
FXTime FXThread::steadytime(){
#if defined(WIN32)
  const FXTime seconds=1000000000;
  LARGE_INTEGER frq,clk;
  ::QueryPerformanceFrequency(&frq);
  ::QueryPerformanceCounter(&clk);
  FXASSERT(frequency<FXLONG(9223372036));       // Overflow possible if CPU speed exceeds 9.2GHz
  FXTime s=clk.QuadPart/frq.QuadPart;
  FXTime f=clk.QuadPart%frq.QuadPart;
  return seconds*s+(seconds*f)/frq.QuadPart;
#elif (_POSIX_C_SOURCE >= 199309L)
  const FXTime seconds=1000000000;
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC,&ts);
  return ts.tv_sec*seconds+ts.tv_nsec;
#else
  const FXTime seconds=1000000000;
  const FXTime microseconds=1000;
  struct timeval tv;
  gettimeofday(&tv,nullptr);
  return tv.tv_sec*seconds+tv.tv_usec*microseconds;
#endif
  }


#if defined(WIN32) && defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))

// Return time in processor ticks.
FXTime FXThread::ticks(){
  FXTime value=__rdtsc();
  return value;
  }

#elif defined(WIN32)

// Return time in processor ticks.
FXTime FXThread::ticks(){
  FXTime value;
  FXASSERT(sizeof(FXTime)==sizeof(LARGE_INTEGER));
  QueryPerformanceCounter((LARGE_INTEGER*)&value);
  return value;
  }

#elif (defined(__GNUC__) || defined(__INTEL_COMPILER)) && defined(__i386__)

// Return time in processor ticks.
FXTime FXThread::ticks(){
  FXTime value;
  __asm__ __volatile__ ( "rdtsc" : "=A" (value));
  return value;
  }

#elif (defined(__GNUC__) || defined(__INTEL_COMPILER)) && defined(__x86_64__)

// Return time in processor ticks.
FXTime FXThread::ticks(){
  FXTime value;
  __asm__ __volatile__ ( "rdtsc              \n\t"
                         "salq	$32, %%rdx   \n\t"
                         "orq	%%rdx, %%rax \n\t" : "=q" (value));
  return value;
  }

/*
#elif (defined(__GNUC__) && defined(__aarch64__))

// Return time in processor ticks.
FXTime FXThread::ticks(){
   NSTime value;
   __asm__ __volatile__("isb; mrs %0, cntvct_el0" : "=r"(value));
   return value;
   }
*/

#elif defined(__GNUC__) && (defined(__powerpc64__) || defined(__ppc64__))

// Return time in processor ticks.
FXTime FXThread::ticks(){
  FXTime value;
  asm volatile("mfspr %0, 268" : "=r"(value));
  return value;
  }

#elif defined(__GNUC__) && (defined(__powerpc__) || defined(__ppc__))

// Return time in processor ticks.
FXTime FXThread::ticks(){
  FXuint tbl,tbu0,tbu1;
  asm volatile("mftbu %0 \n\t"
               "mftb  %1 \n\t"
               "mftbu %2 \n\t" : "=r"(tbu0), "=r"(tbl), "=r"(tbu1));
  tbl&=-(FXint)(tbu0==tbu1);
  return (static_cast<uint64_t>(tbu1) << 32) | tbl;
  return (((FXTime)tbu1) << 32) | tbl;
  }

#elif (_POSIX_C_SOURCE >= 199309L)

// Return time in processor ticks.
FXTime FXThread::ticks(){
  const FXTime seconds=1000000000;
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC,&ts);   // Monotonic: no jumps!
  return ts.tv_sec*seconds+ts.tv_nsec;
  }

#else

// Return time in processor ticks.
FXTime FXThread::ticks(){
  const FXTime seconds=1000000000;
  const FXTime microseconds=1000;
  struct timeval tv;
  gettimeofday(&tv,nullptr);
  return tv.tv_sec*seconds+tv.tv_usec*microseconds;
  }

#endif


// We want to use NtDelayExecution() because it allows for both absolute as well as
// interval wait times. In addition, it is more accurate than the other system calls.
// Unfortunately, this is an undocumented NTDLL API; we can use it anyway by using
// GetProcAddress() on the NTDLL module to dig up its location.
#if defined(WIN32)

// Typedef a pointer to NtDelayExecution()
typedef DWORD (WINAPI *PFN_NTDELAYEXECUTION)(BOOLEAN Alertable,LARGE_INTEGER* DelayInterval);

// Declare the stub function
static DWORD WINAPI NtDelayExecutionStub(BOOLEAN Alertable,LARGE_INTEGER* DelayInterval);

// Pointer to NtDelayExecution, initially pointing to the stub function
static PFN_NTDELAYEXECUTION fxNtDelayExecution=NtDelayExecutionStub;

// The stub gets the address of actual function, sets the function pointer, then calls
// actual function; next time around actual function will be called directly.
static DWORD WINAPI NtDelayExecutionStub(BOOLEAN Alertable,LARGE_INTEGER* DelayInterval){
  if(fxNtDelayExecution==NtDelayExecutionStub){
    HMODULE ntdllDll=GetModuleHandleA("ntdll.dll");
    FXASSERT(ntdllDll);
    fxNtDelayExecution=(PFN_NTDELAYEXECUTION)GetProcAddress(ntdllDll,"NtDelayExecution");
    FXASSERT(fxNtDelayExecution);
    }
  return fxNtDelayExecution(Alertable,DelayInterval);
  }

#endif


// Make the calling thread sleep for a number of nanoseconds
void FXThread::sleep(FXTime nsec){
#if defined(WIN32)
  if(100<=nsec){
    LARGE_INTEGER jiffies;
    jiffies.QuadPart=-nsec/100;
    fxNtDelayExecution((BOOLEAN)false,&jiffies);
    }
#elif (_XOPEN_SOURCE >= 600) || (_POSIX_C_SOURCE >= 200112L)
  const FXTime seconds=1000000000;
  struct timespec value;
  if(1<=nsec){
    value.tv_sec=nsec/seconds;
    value.tv_nsec=nsec%seconds;
    while(clock_nanosleep(CLOCK_MONOTONIC,0,&value,&value)!=0){ }
    }
#elif (_POSIX_C_SOURCE >= 199309L)
  const FXTime seconds=1000000000;
  struct timespec value;
  if(1<=nsec){
    value.tv_sec=nsec/seconds;
    value.tv_nsec=nsec%seconds;
    while(nanosleep(&value,&value)!=0){ }
    }
#else
  const FXTime seconds=1000000000;
  const FXTime microseconds=1000;
  const FXTime milliseconds=1000000;
  struct timeval value;
  if(microseconds<=nsec){
    value.tv_usec=(nsec/microseconds)%milliseconds;
    value.tv_sec=nsec/seconds;
    select(0,nullptr,nullptr,nullptr,&value);
    }
#endif
  }


// Wake at appointed absolute time
void FXThread::wakeat(FXTime nsec){
#if defined(WIN32)
  LARGE_INTEGER jiffies;
  jiffies.QuadPart=FXLONG(116444736000000000)+nsec/100;
  if(0<=jiffies.QuadPart){
    fxNtDelayExecution((BOOLEAN)false,&jiffies);
    }
#elif (_XOPEN_SOURCE >= 600) || (_POSIX_C_SOURCE >= 200112L)
  const FXTime seconds=1000000000;
  struct timespec value;
  if(0<=nsec){
    value.tv_sec=nsec/seconds;
    value.tv_nsec=nsec%seconds;
    while(clock_nanosleep(CLOCK_REALTIME,TIMER_ABSTIME,&value,nullptr)!=0){ }
    }
#elif (_POSIX_C_SOURCE >= 199309L)
  const FXTime seconds=1000000000;
  struct timespec value;
  nsec-=FXThread::time();
  if(1<=nsec){
    value.tv_sec=nsec/seconds;
    value.tv_nsec=nsec%seconds;
    while(nanosleep(&value,&value)!=0){ }
    }
#else
  const FXTime seconds=1000000000;
  const FXTime microseconds=1000;
  const FXTime milliseconds=1000000;
  struct timeval value;
  nsec-=FXThread::time();
  if(microseconds<=nsec){
    value.tv_usec=(nsec/microseconds)%milliseconds;
    value.tv_sec=nsec/seconds;
    select(0,nullptr,nullptr,nullptr,&value);
    }
#endif
  }


// Return thread id of caller
FXThreadID FXThread::current(){
#if defined(WIN32)
//return (FXThreadID)GetCurrentThreadId();
  return (FXThreadID)GetCurrentThread();
#else
  return (FXThreadID)pthread_self();
#endif
  }


// Return number of processors
FXint FXThread::processors(){
#if defined(WIN32)                                              // Windows
  SYSTEM_INFO info={{0}};
  GetSystemInfo(&info);
  return info.dwNumberOfProcessors;
#elif defined(_SC_NPROCESSORS_ONLN)                             // Linux
  int result;
  if((result=sysconf(_SC_NPROCESSORS_ONLN))>0){
    return result;
    }
#elif defined(HAVE_SYS_SYSCTL_H)                                // FreeBSD/NetBSD/OpenBSD/MacOSX
  int mib[4];
  int result;
  size_t len;
  mib[0]=CTL_HW;
  mib[1]=HW_AVAILCPU;
  len=sizeof(result);
  if(sysctl(mib,2,&result,&len,nullptr,0)!=-1){
    return result;
    }
  mib[0]=CTL_HW;
  mib[1]=HW_NCPU;
  len=sizeof(result);
  if(sysctl(mib,2,&result,&len,nullptr,0)!=-1){
    return result;
    }
#elif defined(__IRIX__) && defined(_SC_NPROC_ONLN)              // IRIX
  int result;
  if((result=sysconf(_SC_NPROC_ONLN))>0){
    return result;
    }
#elif defined(hpux) || defined(__hpux) || defined(_hpux)        // HPUX
  struct pst_dynamic psd;
  if(!pstat_getdynamic(&psd,sizeof(psd),(size_t)1,0)){
    return (int)psd.psd_proc_cnt;
    }
#endif
  return 1;
  }


// Return processor index of the calling thread
FXint FXThread::processor(){
  if(1<processors()){
#if defined(WIN32)
#if (WINVER >= 0x0600)                  // Vista and up
    return GetCurrentProcessorNumber();
#endif
#endif
#if defined(HAVE_SCHED_GETCPU)
    return sched_getcpu();
#endif
    return -1;
    }
  return 0;
  }


// Generate new thread local storage key
FXThreadStorageKey FXThread::createStorageKey(){
#if defined(WIN32)
  return (FXThreadStorageKey)TlsAlloc();
#else
  pthread_key_t key;
  return pthread_key_create(&key,nullptr)==0UL ? (FXThreadStorageKey)key : ~0UL;
#endif
  }


// Dispose of thread local storage key
void FXThread::deleteStorageKey(FXThreadStorageKey key){
#if defined(WIN32)
  TlsFree((DWORD)key);
#else
  pthread_key_delete((pthread_key_t)key);
#endif
  }


// Get thread local storage pointer using key
void* FXThread::getStorage(FXThreadStorageKey key){
#if defined(WIN32)
  return TlsGetValue((DWORD)key);
#else
  return pthread_getspecific((pthread_key_t)key);
#endif
  }


// Set thread local storage pointer using key
void FXThread::setStorage(FXThreadStorageKey key,void* ptr){
#if defined(WIN32)
  TlsSetValue((DWORD)key,ptr);
#else
  pthread_setspecific((pthread_key_t)key,ptr);
#endif
  }


// Set thread priority
FXbool FXThread::priority(FXThread::Priority prio){
#if defined(WIN32)
  if(tid){
    int pri;
    switch(prio){
      case PriorityMinimum:
        pri=THREAD_PRIORITY_LOWEST;
        break;
      case PriorityLower:
        pri=THREAD_PRIORITY_BELOW_NORMAL;
        break;
      case PriorityMedium:
        pri=THREAD_PRIORITY_NORMAL;
        break;
      case PriorityHigher:
        pri=THREAD_PRIORITY_ABOVE_NORMAL;
        break;
      case PriorityMaximum:
        pri=THREAD_PRIORITY_HIGHEST;
        break;
      default:
        pri=THREAD_PRIORITY_NORMAL;
        break;
      }
    return SetThreadPriority((HANDLE)tid,pri)!=0;
    }
  return false;
#elif defined(__APPLE__) || defined(__minix)
  return false;
#else
  if(tid){
    sched_param sched={0};
    int plcy=0;
    if(pthread_getschedparam((pthread_t)tid,&plcy,&sched)==0){
#if defined(_POSIX_PRIORITY_SCHEDULING)
      int priomax=sched_get_priority_max(plcy);         // Note that range may depend on scheduling policy!
      int priomin=sched_get_priority_min(plcy);
#elif defined(PTHREAD_MINPRIORITY) && defined(PTHREAD_MAX_PRIORITY)
      int priomin=PTHREAD_MIN_PRIORITY;
      int priomax=PTHREAD_MAX_PRIORITY;
#else
      int priomin=0;
      int priomax=20;
#endif
      if(priomax!=-1 && priomin!=-1){
        int priomed=(priomax+priomin)/2;
        switch(prio){
          case PriorityMinimum:
            sched.sched_priority=priomin;
            break;
          case PriorityLower:
            sched.sched_priority=(priomin+priomed)/2;
            break;
          case PriorityMedium:
            sched.sched_priority=priomed;
            break;
          case PriorityHigher:
            sched.sched_priority=(priomax+priomed)/2;
            break;
          case PriorityMaximum:
            sched.sched_priority=priomax;
            break;
          default:
            sched.sched_priority=priomed;
            break;
          }
        return pthread_setschedparam((pthread_t)tid,plcy,&sched)==0;
        }
      }
    }
  return false;
#endif
  }


// Return thread priority
FXThread::Priority FXThread::priority() const {
#if defined(WIN32)
  FXThread::Priority result=PriorityError;
  if(tid){
    int pri=GetThreadPriority((HANDLE)tid);
    if(pri!=THREAD_PRIORITY_ERROR_RETURN){
      switch(pri){
        case THREAD_PRIORITY_IDLE:
          result=PriorityMinimum;
          break;
        case THREAD_PRIORITY_BELOW_NORMAL:
          result=PriorityLower;
          break;
        case THREAD_PRIORITY_NORMAL:
          result=PriorityMedium;
          break;
        case THREAD_PRIORITY_ABOVE_NORMAL:
          result=PriorityHigher;
          break;
        case THREAD_PRIORITY_HIGHEST:
          result=PriorityMaximum;
          break;
        default:
          result=PriorityDefault;
          break;
        }
      }
    }
  return result;
#elif defined(__APPLE__) || defined(__minix)
  return PriorityError;
#else
  FXThread::Priority result=PriorityError;
  if(tid){
    sched_param sched={0};
    int plcy=0;
    if(pthread_getschedparam((pthread_t)tid,&plcy,&sched)==0){
#if defined(_POSIX_PRIORITY_SCHEDULING)
      int priomax=sched_get_priority_max(plcy);         // Note that range may depend on scheduling policy!
      int priomin=sched_get_priority_min(plcy);
#elif defined(PTHREAD_MINPRIORITY) && defined(PTHREAD_MAX_PRIORITY)
      int priomin=PTHREAD_MIN_PRIORITY;
      int priomax=PTHREAD_MAX_PRIORITY;
#else
      int priomin=0;
      int priomax=32;
#endif
      if(priomax!=-1 && priomin!=-1){
        int priomed=(priomax+priomin)/2;
        if(sched.sched_priority<priomed){
          if(sched.sched_priority<=priomin){
            result=PriorityMinimum;
            }
          else{
            result=PriorityLower;
            }
          }
        else if(sched.sched_priority>priomed){
          if(sched.sched_priority>=priomax){
            result=PriorityMaximum;
            }
          else{
            result=PriorityHigher;
            }
          }
        else{
          result=PriorityMedium;
          }
        }
      return result;
      }
    }
  return result;
#endif
  }


// Set thread scheduling policy
FXbool FXThread::policy(FXThread::Policy plcy){
#if defined(WIN32)
  return false;
#elif defined(__APPLE__) || defined(__minix)
  return false;
#else
  if(tid){
    sched_param sched={0};
    int oldplcy=0;
    int newplcy=0;
    if(pthread_getschedparam((pthread_t)tid,&oldplcy,&sched)==0){
      switch(plcy){
        case PolicyFifo:
          newplcy=SCHED_FIFO;
          break;
        case PolicyRoundRobin:
          newplcy=SCHED_RR;
          break;
        default:
          newplcy=SCHED_OTHER;
          break;
        }
#if defined(_POSIX_PRIORITY_SCHEDULING)
      sched.sched_priority=sched_get_priority_min(newplcy);
#endif
      return pthread_setschedparam((pthread_t)tid,newplcy,&sched)==0;
      }
    }
  return false;
#endif
  }


// Get thread scheduling policy
FXThread::Policy FXThread::policy() const {
#if defined(WIN32)
  return PolicyError;
#elif defined(__APPLE__) || defined(__minix)
  return PolicyError;
#else
  Policy result=PolicyError;
  if(tid){
    sched_param sched={0};
    int plcy=0;
    if(pthread_getschedparam((pthread_t)tid,&plcy,&sched)==0){
      switch(plcy){
        case SCHED_FIFO:
          result=PolicyFifo;
          break;
        case SCHED_RR:
          result=PolicyRoundRobin;
          break;
        default:
          result=PolicyDefault;
          break;
        }
      }
    }
  return result;
#endif
  }


// Change thread's processor affinity
FXbool FXThread::affinity(FXulong mask){
#if defined(WIN32)
  const FXulong bit=1;
  if(tid){
    FXint ncpus=processors();
    mask&=((bit<<ncpus)-1);
    if(mask){
      return SetThreadAffinityMask((HANDLE)tid,(FXuval)mask)!=0;
      }
    }
#elif defined(HAVE_PTHREAD_SETAFFINITY_NP)
  const FXulong bit=1;
  if(tid){
    FXint ncpus=processors();
    mask&=((bit<<ncpus)-1);
    if(mask){
      cpu_set_t cpuset;
      CPU_ZERO(&cpuset);
      for(FXint cpu=0; cpu<ncpus; ++cpu){
        if((bit<<cpu)&mask){ CPU_SET(cpu,&cpuset); }
        }
      return pthread_setaffinity_np((pthread_t)tid,sizeof(cpuset),&cpuset)==0;
      }
    }
#endif
  return false;
  }


// Get thread's processor affinity
FXulong FXThread::affinity() const {
#if defined(WIN32)
  const FXulong bit=1;
  if(tid){
    FXint ncpus=processors();
    FXuval cpuset=SetThreadAffinityMask((HANDLE)tid,(FXuval)((bit<<ncpus)-1));
    if(cpuset){
      SetThreadAffinityMask((HANDLE)tid,cpuset);
      return (FXulong)cpuset;
      }
    }
#elif defined(HAVE_PTHREAD_SETAFFINITY_NP)
  const FXulong bit=1;
  if(tid){
    FXint ncpus=processors();
    cpu_set_t cpuset;
    if(pthread_getaffinity_np((pthread_t)tid,sizeof(cpuset),&cpuset)==0){
      FXulong mask=0;
      for(FXint cpu=0; cpu<ncpus; ++cpu){
        if(CPU_ISSET(cpu,&cpuset)){ mask|=(bit<<cpu); }
        }
      return mask;
      }
    }
#endif
  return 0;
  }


#if defined(WIN32)

// Declare the function signatures
typedef HRESULT (WINAPI *PFN_SETTHREADDESCRIPTION)(HANDLE hThread,const WCHAR* desc);
typedef HRESULT (WINAPI *PFN_GETTHREADDESCRIPTION)(HANDLE hThread,WCHAR** desc);

// Declare the stub functions
static HRESULT WINAPI SetThreadDescriptionStub(HANDLE hThread,const WCHAR* desc);
static HRESULT WINAPI GetThreadDescriptionStub(HANDLE hThread,WCHAR** desc);

// Pointers to functions, initially pointing to the stub functions
static PFN_SETTHREADDESCRIPTION fxSetThreadDescription=SetThreadDescriptionStub;
static PFN_GETTHREADDESCRIPTION fxGetThreadDescription=GetThreadDescriptionStub;


// Set thread name (needs late-model Windows 10)
static HRESULT WINAPI SetThreadDescriptionStub(HANDLE hThread,const WCHAR* desc){
  if(fxSetThreadDescription==SetThreadDescriptionStub){
    HMODULE hnddll=GetModuleHandleA("kernel32.dll");
    fxSetThreadDescription=(PFN_SETTHREADDESCRIPTION)GetProcAddress(hnddll,"SetThreadDescription");
    }
  if(fxSetThreadDescription){
    return fxSetThreadDescription(hThread,desc);
    }
  return -1;
  }


// Get thread name (needs late-model Windows 10)
static HRESULT WINAPI GetThreadDescriptionStub(HANDLE hThread,WCHAR** desc){
  if(fxGetThreadDescription==GetThreadDescriptionStub){
    HMODULE hnddll=GetModuleHandleA("kernel32.dll");
    fxGetThreadDescription=(PFN_GETTHREADDESCRIPTION)GetProcAddress(hnddll,"GetThreadDescription");
    }
  if(fxGetThreadDescription){
    return fxGetThreadDescription(hThread,desc);
    }
  return -1;
  }

#endif



// Change thread description
FXbool FXThread::description(const FXString& desc){
  if(tid){
#if defined(WIN32)
    FXnchar udesc[256];
    utf2ncs(udesc,desc.text(),ARRAYNUMBER(udesc));
    return 0<=fxSetThreadDescription((HANDLE)tid,udesc);
#elif defined(__APPLE__)
    return pthread_setname_np(desc.text())==0;
#elif defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
    pthread_setname_np((pthread_t)tid,desc.text());
    return true;
#elif defined(HAVE_PTHREAD_SETNAME_NP)
    return pthread_setname_np((pthread_t)tid,desc.text())==0;
#endif
    }
  return false;
  }


// Return thread description
FXString FXThread::description() const {
  if(tid){
#if defined(WIN32)
    FXnchar* udesc;
    if(0<=fxGetThreadDescription((HANDLE)tid,&udesc)){
      FXchar desc[256];
      ncs2utf(desc,udesc,ARRAYNUMBER(desc));
      ::LocalFree(udesc);
      return desc;
      }
#elif defined(__APPLE__)
    FXchar desc[256];
    if(pthread_getname_np(*((pthread_t*)&tid),desc,ARRAYNUMBER(desc))==0){
      return desc;
      }
#elif defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
    FXchar desc[256];
    if(pthread_getname_np((pthread_t)tid,desc,ARRAYNUMBER(desc))==0){
      return desc;
      }
#elif defined(HAVE_PTHREAD_GETNAME_NP)
    FXchar desc[256];
    if(pthread_getname_np((pthread_t)tid,desc,ARRAYNUMBER(desc))==0){
      return desc;
      }
#endif
    }
  return FXString::null;
  }


// Suspend thread
FXbool FXThread::suspend(){
#if defined(WIN32)
  return tid && (SuspendThread((HANDLE)tid)!=(DWORD)-1L);
#elif defined(_HPUX_SOURCE)
  return tid && (pthread_suspend((pthread_t)tid)==0);
#elif defined(SUNOS)
  return tid && (thr_suspend((pthread_t)tid)==0);
#else
  return false;
#endif
  }


// Resume thread
FXbool FXThread::resume(){
#if defined(WIN32)
  return tid && (ResumeThread((HANDLE)tid)!=(DWORD)-1L);
#elif defined(_HPUX_SOURCE)
  return tid && (pthread_resume_np((pthread_t)tid,PTHREAD_COUNT_RESUME_NP)==0);
#elif defined(SUNOS)
  return tid && (thr_continue((pthread_t)tid)==0);
#else
  return false;
#endif
  }


// Destroy
FXThread::~FXThread(){
  if(self()==this){
    self(nullptr);
    detach();
    }
  else{
    cancel();
    }
  }

}
