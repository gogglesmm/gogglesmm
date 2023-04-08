/********************************************************************************
*                                                                               *
*                            R e a c t o r   C l a s s                          *
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
#include "fxmath.h"
#include "FXAtomic.h"
#include "FXAutoThreadStorageKey.h"
#include "FXThread.h"
#include "FXElement.h"
#include "FXReactor.h"
#include "FXReactorCore.h"
#include "FXException.h"

/*
  Notes:

  - FXReactor implements the a reactor pattern: it watches objects such as sockets,
    pipes, events, etc, blocking the calling thread in a special system call.

  - When activity occurs on any of the watched objects, the thread is woken up
    from the system call and dispatches to code that handles the activity; for
    example, a socket may have data from a network in a kernel buffer and now
    this data must be processed.

  - If no activity arrives within a maximum blocking time, the thread also wakes
    up, and simply returns with no action (the default blocking time is forever).

  - A timeout may be computed to wake the thread early; if the timeout occurs
    before the maximum blocking time expires, control is dispatched to the
    timout handler.

  - If, during the wait, an interrupt (signal) occurs, the waiting thread is
    awoken and dispatched to the signal handler.  This can happen in either
    of two ways: synchronously, or asynchronously.  Synchronous handling
    should be preferred, as asynchronous handling executes from the context
    of the interrupt service routine and may find the program in an inconsistent
    state.

  - When FXReactor dispatches to a handler, it must assume that the callback handler
    for the event may do anything whatsoever, which includes adding or removing
    signals, handles, etc.  Thus, dispatch() should be made to return, to
    re-enter at a later time.
    The callback handler indicates this by returning true, i.e. the callback
    has been processed and may have had an effect.

    If a callback handler is not handled, it should return false; in this case
    FXReactor will continue to process in dispatch() and not return until its
    done (timed out, or issued a another callback which then returned true).

  - The dispatch() driver determines which events have taken place and calls the
    appropriate handling routine.

  - Thus, once a set of events is determined, we need to keep track of this set
    and only check for new events once all the events from the last check have
    been dealt with.

  - More than one object may become active during the wait; on UNIX systems the
    FXReactor will perform a dispatch on each active object prior to blocking
    again in the system call.

  - Prior to blocking on system call, FXReactor performs a poll on the objects
    to check for activity; if none are active, then just prior to blocking a
    special "idle" handler is called.

  - Thus, the idle handler will only be called when no other items demand attention;
    performing idle processing would therefore simply cut into the amount of time
    the thread would otherwise have spent waiting in the system call.

  - The control-set flags narrow down the set of handlers to be tested; by default, all
    handlers are dispatched to (DispatchAll).

  - When DispatchSignals is not in control-set, no signals are processed.

  - When DispatchTimers is not in control-set, no timers are processed (this has the
    effect of possibly oversleeping of due timers!).

  - When DispatchIdle is not in control-set, no idle processing is performed; the
    calling thread will block in the system-call immediately after determining no
    objects are active.

  - FXReactor tries to give all objects a "fair shake" and processes active
    objects in round-robin fashion so as to not let any single object hog all the
    attention. [On Windows, this is done by shuffling the list of objects].

  - Each interrupt (signal) is handled by only one single FXReactor.  However, there
    may be multiple FXReactors, each one handling a different, non-overlapping set of
    signals.

  - Look into whether timerfd_create() may have some advantages for timers, as opposed
    to falling out of epoll system call after a set time (less checking?).

  - If using epoll() instead of select() or pselect(), we may want to raise
    RLIMIT_NOFILE as we're able to go beyond FD_SETSIZE.
*/

// Bad handle value
#if defined(WIN32)
#define BadHandle INVALID_HANDLE_VALUE
#else
#define BadHandle -1
#endif

using namespace FX;

/*******************************************************************************/

namespace FX {

// Units of time in nanoseconds
const FXTime seconds=1000000000;
const FXTime milliseconds=1000000;
const FXTime microseconds=1000;

// Sleep no longer than this
const FXTime FXReactor::maxwait=86400*seconds;


// Construct reactor object
FXReactor::FXReactor():internals(nullptr),sigreceived(0),numhandles(0),numwatched(0),numraised(0){
#if defined(WIN32)
  current=-1;
#else
  current=0;
#endif
  }


// Initialize reactor
FXbool FXReactor::init(){
  if(!internals){
    if(callocElms(internals,1)){
#if defined(HAVE_EPOLL_CREATE1)
      internals->handle=epoll_create1(EPOLL_CLOEXEC);
      if(internals->handle<0){ freeElms(internals); return false; }
#endif
      sigreceived=0;
      numhandles=0;
      numwatched=0;
      numraised=0;
      current=-1;
      return true;
      }
    }
  return false;
  }

/*******************************************************************************/

// Which reactor is responsible for which signal
FXReactor *volatile FXReactor::sigmanager[64];


// Handler for a synchronous managed signal simply sets flag that a signal
// was raised, and remembers most-recently raised signal.
void FXReactor::signalhandler(FXint sig){
  sigmanager[sig]->internals->signotified[sig]=1;
  sigmanager[sig]->sigreceived=sig;
  }


// Asynchronous signal handler for managed signal directly dispatches to
// handling code; this is potentially dangerous, so use at your own risk.
void FXReactor::signalhandlerasync(FXint sig){
  sigmanager[sig]->dispatchSignal(sig);
  }


// Return true if signal sig is handled.
FXbool FXReactor::hasSignal(FXint sig) const {
  return (0<sig && sig<64 && sigmanager[sig]==this);
  }


// Append signal to signal-set observed by the reactor
FXbool FXReactor::addSignal(FXint sig,FXbool async){
  if(internals){
    if(sig<=0) return false;
    if(sig>=64) return false;
    if(atomicBoolCas(&sigmanager[sig],(FXReactor*)nullptr,this)){
      void (CDECL *handler)(int);
      if(async){
        handler=FXReactor::signalhandlerasync;  // Asynchronous callback
        }
      else{
        handler=FXReactor::signalhandler;       // Normal callback
        }
      internals->signotified[sig]=0;            // Set non-raised
#if defined(WIN32)
      if(signal(sig,handler)==SIG_ERR){         // Set handler
        sigmanager[sig]=nullptr;
        return false;
        }
#elif defined(_POSIX_SOURCE) || defined(_INCLUDE_POSIX_SOURCE) || defined(_XOPEN_SOURCE)
      struct sigaction sigact;
      sigact.sa_handler=handler;
      sigact.sa_flags=0;
      sigfillset(&sigact.sa_mask);              // Block signals while running handler
      if(sigaction(sig,&sigact,nullptr)==-1){   // Set handler
        sigmanager[sig]=nullptr;
        return false;
        }
#else
      if(signal(sig,handler)==SIG_ERR){         // Set handler
        sigmanager[sig]=nullptr;
        return false;
        }
#endif
      return true;
      }
    }
  return false;
  }


// Remove signal from signal-set observed by the reactor
FXbool FXReactor::remSignal(FXint sig){
  if(internals){
    if(sig<=0) return false;
    if(sig>=64) return false;
    if(sigmanager[sig]==this){
#if defined(WIN32)
      if(signal(sig,SIG_DFL)==SIG_ERR){         // Unset handler
        return false;
        }
#elif defined(_POSIX_SOURCE) || defined(_INCLUDE_POSIX_SOURCE) || defined(_XOPEN_SOURCE)
      struct sigaction sigact;
      sigact.sa_handler=SIG_DFL;
      sigact.sa_flags=0;
      sigemptyset(&sigact.sa_mask);             // Pass signals while running handler
      if(sigaction(sig,&sigact,nullptr)==-1){   // Unset handler
        return false;
        }
#else
      if(signal(sig,SIG_DFL)==SIG_ERR){         // Unset handler
        return false;
        }
#endif
      sigmanager[sig]=nullptr;                  // Now release it
      internals->signotified[sig]=0;            // Set non-raised
      return true;
      }
    }
  return false;
  }


// Dispatch when a signal was fired
FXbool FXReactor::dispatchSignal(FXint){
  return false;
  }

/*******************************************************************************/

// Return timeout when something needs to happen
FXTime FXReactor::nextTimeout(){
  return forever;
  }


// Dispatch when timeout expires
FXbool FXReactor::dispatchTimeout(FXTime){
  return false;
  }

/*******************************************************************************/

// Dispatch when idle
FXbool FXReactor::dispatchIdle(){
  return false;
  }

/*******************************************************************************/

#if defined(WIN32)

// Find index of the given handle; -1 if not found
static inline FXint findHandle(FXInputHandle hnd,const FXInputHandle handles[],FXint n){
  while(--n>=0 && handles[n]!=hnd){ }
  return n;
  }

#endif

// Append new handle hnd to watch-list
FXbool FXReactor::addHandle(FXInputHandle hnd,FXuint mode){
  if(internals){
#if defined(WIN32)
    if(hnd==BadHandle) return false;
    if(numhandles>=MAXIMUM_WAIT_OBJECTS) return false;
    if(findHandle(hnd,internals->handles,numhandles)>=0) return false;
    internals->handles[numhandles]=hnd;
    internals->modes[numhandles]=mode;
    numhandles++;
    return true;
#elif defined(HAVE_EPOLL_CREATE1)
    struct epoll_event ev;
    if(hnd<0) return false;
    ev.events=0;
    ev.data.fd=hnd;
    if(mode&InputRead) ev.events|=EPOLLIN;
    if(mode&InputWrite) ev.events|=EPOLLOUT;
    if(mode&InputExcept) ev.events|=EPOLLPRI;
    if(epoll_ctl(internals->handle,EPOLL_CTL_ADD,hnd,&ev)!=0) return false;
    numhandles++;
    return true;
#else
    if(hnd<0) return false;
    if(hnd>=FD_SETSIZE) return false;
    if(FD_ISSET(hnd,&internals->handles[0]) || FD_ISSET(hnd,&internals->handles[1]) || FD_ISSET(hnd,&internals->handles[2])) return false;
    if(mode&InputRead){ FD_SET(hnd,&internals->handles[0]); }
    if(mode&InputWrite){ FD_SET(hnd,&internals->handles[1]); }
    if(mode&InputExcept){ FD_SET(hnd,&internals->handles[2]); }
    if(numhandles<=hnd){ numhandles=hnd+1; }
    return true;
#endif
    }
  return false;
  }


// Remove handle hnd from list
FXbool FXReactor::remHandle(FXInputHandle hnd){
  if(internals){
#if defined(WIN32)
    FXint s;
    if(hnd==BadHandle) return false;
    if((s=findHandle(hnd,internals->handles,numhandles))<0) return false;
    internals->handles[s]=internals->handles[numhandles-1];
    internals->modes[s]=internals->modes[numhandles-1];
    if(current==s) current=-1;                          // Removed a raised handle
    if(current==numhandles-1) current=s;                // Renumbered a raised handle
    numhandles--;
    return true;
#elif defined(HAVE_EPOLL_CREATE1)
    struct epoll_event ev;
    if(hnd<0) return false;
    ev.events=0;                                        // Doesn't really matter, ignored by kernel
    ev.data.fd=0;
    if(epoll_ctl(internals->handle,EPOLL_CTL_DEL,hnd,&ev)!=0) return false;
    numhandles--;
    return true;
#else
    if(hnd<0) return false;
    if(hnd>=FD_SETSIZE) return false;
    if(!FD_ISSET(hnd,&internals->handles[0]) && !FD_ISSET(hnd,&internals->handles[1]) && !FD_ISSET(hnd,&internals->handles[2])) return false;
    if(FD_ISSET(hnd,&internals->watched[0])){ FD_CLR(hnd,&internals->watched[0]); numraised--; }
    if(FD_ISSET(hnd,&internals->watched[1])){ FD_CLR(hnd,&internals->watched[1]); numraised--; }
    if(FD_ISSET(hnd,&internals->watched[2])){ FD_CLR(hnd,&internals->watched[2]); numraised--; }
    FD_CLR(hnd,&internals->handles[0]);
    FD_CLR(hnd,&internals->handles[1]);
    FD_CLR(hnd,&internals->handles[2]);
    if(hnd==numhandles-1){
      while(0<numhandles && !FD_ISSET(numhandles-1,&internals->handles[0]) && !FD_ISSET(numhandles-1,&internals->handles[1]) && !FD_ISSET(numhandles-1,&internals->handles[2])){
        --numhandles;
        }
      }
    return true;
#endif
    }
  return false;
  }


// Dispatch when when handle hnd is signaled with mode
FXbool FXReactor::dispatchHandle(FXInputHandle,FXuint,FXuint){
  return false;
  }

#if defined(WIN32) //////////////////////////////////////////////////////////////

// Dispatch driver
FXbool FXReactor::dispatch(FXTime blocking,FXuint flags){
  if(internals){
    FXTime now,due,delay,interval;
    FXuint sig,nxt,mode,ms;
    FXInputHandle hnd;

    // Loop till we got something
    while(1){

      // Check for timeout
      delay=forever;
      if(flags&DispatchTimers){
        due=nextTimeout();
        if(due<forever){
          now=FXThread::time();
          delay=due-now;
          if(delay<microseconds){
            if(dispatchTimeout(due)) return true;       // Timer activity
            continue;
            }
          }
        }

      // Check for signal
      if(flags&DispatchSignals){
        sig=nxt=sigreceived;
        if(atomicSet(&internals->signotified[sig],0)){
          do{ nxt=(nxt+63)&63; }while(!internals->signotified[nxt] && nxt!=sig);
          sigreceived=nxt;
          if(dispatchSignal(sig)) return true;          // Signal activity
          continue;
          }
        }

      // Check active handles
      if(0<=current && current<numhandles){
        hnd=internals->handles[current];                // Shuffle raised handle up in the list
        mode=internals->modes[current];                 // To give all handles equal play time
        nxt=(current+1)%numhandles;
        swap(internals->handles[current],internals->handles[nxt]);
        swap(internals->modes[current],internals->modes[nxt]);
        current=-1;
        if(dispatchHandle(hnd,mode,flags)) return true; // IO activity
        continue;
        }

      // Select active handles; don't block
      current=WaitForMultipleObjectsEx(numhandles,internals->handles,false,0,true);

      // No handles were active
      if(current==WAIT_TIMEOUT){

        // Idle callback if we're about to block
        if(flags&DispatchIdle){
          if(dispatchIdle()) return true;               // Idle activity
          }

        // We're not blocking
        if(blocking<=0) return false;

        // Indefinite wait
        ms=INFINITE;

        // If not blocking indefinitely, don't exceed maxwait time interval.
        interval=Math::imin(delay,blocking);
        if(interval<forever){
          interval=Math::imin(interval,maxwait);
          ms=(FXuint)(interval/milliseconds);
          }

        // Select active handles, wait for timeout or maximum block time
        current=WaitForMultipleObjectsEx(numhandles,internals->handles,false,ms,true);

        // Return if there was no timeout within maximum block time
        if(current==WAIT_TIMEOUT){
          if(blocking<forever){                         // Next blocking period reduced by time already expired
            blocking-=interval;
            if(blocking<=0) return false;               // Nothing happened during blocking period!
            }
          continue;
          }
        }

      // I/O completion took place; maybe some i/o took place, causing
      // somestuff to have been changed; we must leave loop.
      if(current==WAIT_IO_COMPLETION) return false;

      // Bad stuff happened
      if(current==WAIT_FAILED || current>=WAIT_ABANDONED_0){
        throw FXFatalException("FXReactor::dispatch: error waiting on handles.");
        }
      }
    }
  return false;
  }

#elif defined(HAVE_EPOLL_CREATE1) ///////////////////////////////////////////////

// Dispatch driver
FXbool FXReactor::dispatch(FXTime blocking,FXuint flags){
  if(internals){
    FXTime now,due,delay,interval;
    FXuint sig,nxt,mode,ms;
    FXInputHandle hnd;

    // Loop till we got something
    while(1){

      // Check for timeout
      delay=forever;
      if(flags&DispatchTimers){
        due=nextTimeout();
        if(due<forever){
          now=FXThread::time();
          delay=due-now;
          if(delay<microseconds){
            if(dispatchTimeout(due)) return true;       // Timer activity
            continue;
            }
          }
        }

      // Check for signal
      if(flags&DispatchSignals){
        sig=nxt=sigreceived;
        if(atomicSet(&internals->signotified[sig],0)){
          do{ nxt=(nxt+63)&63; }while(!internals->signotified[nxt] && nxt!=sig);
          sigreceived=nxt;
          if(dispatchSignal(sig)) return true;          // Signal activity
          continue;
          }
        }

      // Check active handles
      if(0<numraised){
        mode=0;
        numraised--;
        current=(current+1)%numwatched;
        hnd=internals->events[current].data.fd;
        if(internals->events[current].events&EPOLLIN){ mode|=InputRead; }
        if(internals->events[current].events&EPOLLOUT){ mode|=InputWrite; }
        if(internals->events[current].events&EPOLLERR){ mode|=InputExcept; }
        if(dispatchHandle(hnd,mode,flags)) return true; // IO activity
        continue;
        }

      // Select active handles and check signals; don't block
      numwatched=epoll_pwait(internals->handle,internals->events,ARRAYNUMBER(internals->events),0,nullptr);

      // No active handles yet; need to wait
      if(numwatched==0){

        // Idle callback if we're about to block
        if(flags&DispatchIdle){
          if(dispatchIdle()) return true;               // Idle activity
          }

        // We're not blocking
        if(blocking<=0) return false;

        // Indefinite wait
        ms=-1;

        // If not blocking indefinitely, don't exceed maxwait time interval.
        interval=Math::imin(delay,blocking);
        if(interval<forever){
          interval=Math::imin(interval,maxwait);
          ms=(FXuint)(interval/milliseconds);
          }

        // Select active handles and check signals, waiting for timeout or maximum block time
        numwatched=epoll_pwait(internals->handle,internals->events,ARRAYNUMBER(internals->events),ms,nullptr);

        // Return if there was no timeout within maximum block time
        if(numwatched==0){
          if(blocking<forever){                         // Next blocking period reduced by time already expired
            blocking-=interval;
            if(blocking<=0) return false;               // Nothing happened during blocking period!
            }
          continue;
          }
        }

      // Bad stuff happened
      if(numwatched<0){
        if(errno!=EAGAIN && errno!=EINTR){ throw FXFatalException("FXReactor::dispatch: error waiting on handles."); }
        continue;
        }

      // Keep track of original set
      numraised=numwatched;
      }
    }
  return false;
  }

#else ///////////////////////////////////////////////////////////////////////////

// Dispatch driver
FXbool FXReactor::dispatch(FXTime blocking,FXuint flags){
  if(internals){
    FXTime now,due,delay,interval;
    FXuint sig,nxt,mode;
#if (_POSIX_C_SOURCE >= 200112L)
    struct timespec delta;
#else
    struct timeval delta;
#endif

    // Loop till we got something
    while(1){

      // Check for timeout
      delay=forever;
      if(flags&DispatchTimers){
        due=nextTimeout();
        if(due<forever){
          now=FXThread::time();
          delay=due-now;
          if(delay<microseconds){
            if(dispatchTimeout(due)) return true;       // Timer activity
            continue;
            }
          }
        }

      // Check for signal
      if(flags&DispatchSignals){
        sig=nxt=sigreceived;
        if(atomicSet(&internals->signotified[sig],0)){
          do{ nxt=(nxt+63)&63; }while(!internals->signotified[nxt] && nxt!=sig);
          sigreceived=nxt;
          if(dispatchSignal(sig)) return true;          // Signal activity
          continue;
          }
        }

      // Check active handles
      if(0<numraised){
        mode=0;
        do{
          current=(current+1)%numwatched;
          if(FD_ISSET(current,&internals->watched[0])){
            FD_CLR(current,&internals->watched[0]);
            mode|=InputRead;
            numraised--;
            }
          if(FD_ISSET(current,&internals->watched[1])){
            FD_CLR(current,&internals->watched[1]);
            mode|=InputWrite;
            numraised--;
            }
          if(FD_ISSET(current,&internals->watched[2])){
            FD_CLR(current,&internals->watched[2]);
            mode|=InputExcept;
            numraised--;
            }
          }
        while(mode==0);

        // IO handle became active
        if(dispatchHandle(current,mode,flags)) return true;     // IO activity
        continue;
        }

      // Prepare handles to check
      internals->watched[0]=internals->handles[0];
      internals->watched[1]=internals->handles[1];
      internals->watched[2]=internals->handles[2];

      // Select active handles and check signals; don't block
#if (_POSIX_C_SOURCE >= 200112L)
      numraised=pselect(numhandles,&internals->watched[0],&internals->watched[1],&internals->watched[2],nullptr,nullptr);
#else
      numraised=select(numhandles,&internals->watched[0],&internals->watched[1],&internals->watched[2],nullptr);
#endif

      // No handles were active
      if(numraised==0){

        // Idle callback if we're about to block
        if(flags&DispatchIdle){
          if(dispatchIdle()) return true;               // Idle activity
          }

        // We're not blocking
        if(blocking<=0) return false;

        // Prepare handles to check
        internals->watched[0]=internals->handles[0];
        internals->watched[1]=internals->handles[1];
        internals->watched[2]=internals->handles[2];

        // Nanoseconds to wait
        interval=Math::imin(delay,blocking);
        if(interval<forever){
          interval=Math::imin(interval,maxwait);
          }

        // Select active handles and check signals, waiting for timeout or maximum block time
#if (_POSIX_C_SOURCE >= 200112L)
        delta.tv_sec=interval/seconds;
        delta.tv_nsec=(interval-seconds*delta.tv_sec);
        numraised=pselect(numhandles,&internals->watched[0],&internals->watched[1],&internals->watched[2],&delta,nullptr);
#else
        delta.tv_sec=interval/seconds;
        delta.tv_usec=(interval-seconds*delta.tv_sec)/microseconds;
        numraised=select(numhandles,&internals->watched[0],&internals->watched[1],&internals->watched[2],&delta);
#endif

        // Return if there was no timeout within maximum block time
        if(numraised==0){
          if(blocking<forever){                         // Next blocking period reduced by time already expired
            blocking-=interval;
            if(blocking<=0) return false;               // Nothing happened during blocking period!
            }
          continue;
          }
        }

      // Bad stuff happened
      if(numraised<0){
        if(errno!=EAGAIN && errno!=EINTR){ throw FXFatalException("FXReactor::dispatch: error waiting on handles."); }
        continue;
        }

      // Keep track of original set
      numwatched=numhandles;
      }
    }
  return false;
  }

#endif //////////////////////////////////////////////////////////////////////////

// Exit reactor
FXbool FXReactor::exit(){
  if(internals){
    for(FXint s=1; s<64; ++s){
      remSignal(s);
      }
#if defined(HAVE_EPOLL_CREATE1)
    close(internals->handle);
#endif
    freeElms(internals);
    sigreceived=0;
    numhandles=0;
    numwatched=0;
    numraised=0;
    current=-1;
    return true;
    }
  return false;
  }


// Destroy reactor object
FXReactor::~FXReactor(){
  exit();
  }

}
