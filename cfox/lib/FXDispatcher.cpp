/********************************************************************************
*                                                                               *
*                         E v e n t   D i s p a t c h e r                       *
*                                                                               *
*********************************************************************************
* Copyright (C) 2006,2019 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "FXArray.h"
#include "FXAtomic.h"
#include "FXHash.h"
#include "FXStream.h"
#include "FXRectangle.h"
#include "FXString.h"
#include "FXEvent.h"
#include "FXObject.h"
#include "FXMutex.h"
#include "FXAutoThreadStorageKey.h"
#include "FXThread.h"
#include "FXElement.h"
#include "FXDispatcher.h"
#include "FXException.h"

/*
  Notes:

  - Special subclass of FXDispatcher for when there is a GUI event source.

  - FXDispatcher does NOT loop; but does maintain set of event sources and raised
    events; thus, recursive event loops don't lose track.

  - Process IO handles in round-robin fashion so each one gets equal opportunity of being
    handled; on Windows, the list of handles is simply shuffled.

  - A successful handling of a callback causes return from dispatch; an unsuccessful
    callback causes FXDispatcher::dispatch() to go around to the next event.

  - If we allow another thread to add/remove event sources, then we will need a
    signaling pipe to notify the blocking thread (except when using epoll).

  - Use epoll_pwait(() if available; otherwise fall back to pselect().  If neither
    of these available, we will use select(); on Windows, WaitForMultipleObjects(),
    or MsgWaitForMultipleObjects.

  - When using WaitForMultipleObjects(), only one handle is dispatched at a time.
    The list is reshuffled prior to dispatch, thus providing fairness; also, this
    is reentrant in case a nested event loop returns to FXDispatcher again.

  - FXDispatcher (or subclasses thereof) don't loop.  Just block and dispatch.
    FXInvocation represents a loop.  They're linked, and per-thread.  It calls
    FXDispatcher::dispatch() repeatedly until some condition calls for it to stop.

  - Each signal is handled by only one single FXDispatcher.  However, there may
    be multiple FXDispatcher's, each one handling a different, non-overlapping
    set of signals.

  - FIXME keep track of time we entered event loop; so we can process events for
    a certain interval only (better than runWhileEvents() currently does).

*/


// Bad handle value
#ifdef WIN32
#define BadHandle INVALID_HANDLE_VALUE
#else
#define BadHandle -1
#endif


using namespace FX;

/*******************************************************************************/

namespace FX {

// Handles being watched
struct FXDispatcher::FXHandles {
#if defined(WIN32)
  FXInputHandle      handles[MAXIMUM_WAIT_OBJECTS];     // Handles
  FXuint             modes[MAXIMUM_WAIT_OBJECTS];       // IO Modes each handle
#elif defined(HAVE_EPOLL_CREATE1)
  struct epoll_event events[128];                       // Events
  FXInputHandle      handle;                            // Poll handle
#else
  fd_set             watched[3];                        // Watched handles (FD_SETSIZE elements)
  fd_set             handles[3];                        // Known handles
#endif
  };


/*******************************************************************************/


// Construct dispatcher object
FXDispatcher::FXDispatcher():handles(NULL),sigreceived(0),numhandles(0),numwatched(0),numraised(0),current(-1),initialized(false){
  FXTRACE((10,"FXDispatcher::FXDispatcher\n"));
  clearElms(signotified,ARRAYNUMBER(signotified));
  }


// Initialize dispatcher
FXbool FXDispatcher::init(){
  FXTRACE((10,"FXDispatcher::init()\n"));
  if(!initialized){
    if(allocElms(handles,1)){
#if defined(WIN32)
      clearElms(handles->handles,ARRAYNUMBER(handles->handles));
      clearElms(handles->modes,ARRAYNUMBER(handles->modes));
#elif defined(HAVE_EPOLL_CREATE1)
      clearElms(handles->events,ARRAYNUMBER(handles->events));
      handles->handle=epoll_create1(EPOLL_CLOEXEC);
      if(handles->handle<0){ freeElms(handles); return false; }
#else
      FD_ZERO(&handles->handles[0]);            // No handles
      FD_ZERO(&handles->handles[1]);
      FD_ZERO(&handles->handles[2]);
      FD_ZERO(&handles->watched[0]);
      FD_ZERO(&handles->watched[1]);
      FD_ZERO(&handles->watched[2]);
#endif
      clearElms(signotified,ARRAYNUMBER(signotified));
      sigreceived=0;
      numhandles=0;
      numwatched=0;
      numraised=0;
      current=-1;
      initialized=true;
      return true;
      }
    }
  return false;
  }


/*******************************************************************************/

// Which dispatcher responsible for which signal
FXDispatcher *volatile FXDispatcher::sigmanager[64];


// Handler for a synchronous managed signal simply sets flag that a signal
// was raised, and remembers most-recently raised signal.
void FXDispatcher::signalhandler(FXint sig){
  sigmanager[sig]->signotified[sig]=1;
  sigmanager[sig]->sigreceived=sig;
  }


// Asynchronous signal handler for managed signal directly dispatches to
// handling code; this is potentially dangerous, so use at your own risk.
void FXDispatcher::signalhandlerasync(FXint sig){
  sigmanager[sig]->dispatchSignal(sig);
  }


// Return true if dispatcher manages given signal
FXbool FXDispatcher::hasSignal(FXint sig){
  return (sigmanager[sig]==this);
  }


// Append signal to signal-set observed by the dispatcher
FXbool FXDispatcher::addSignal(FXint sig,FXbool async){
  if(initialized){
    if(atomicBoolCas(&sigmanager[sig],(FXDispatcher*)NULL,this)){
      void (CDECL *handler)(int);
      if(async){
        handler=FXDispatcher::signalhandlerasync;       // Asynchronous callback
        }
      else{
        handler=FXDispatcher::signalhandler;            // Normal callback
        }
      signotified[sig]=0;                               // Set non-raised
#ifdef WIN32
      if(signal(sig,handler)==SIG_ERR){                 // Set handler
        sigmanager[sig]=NULL;
        return false;
        }
#elif (_POSIX_C_SOURCE >= 200112L) || (_XOPEN_SOURCE >= 600)
      struct sigaction sigact;
      sigact.sa_handler=handler;
      sigact.sa_flags=0;
      sigfillset(&sigact.sa_mask);
      if(sigaction(sig,&sigact,NULL)==-1){              // Set handler
        sigmanager[sig]=NULL;
        return false;
        }
#else
      if(signal(sig,handler)==SIG_ERR){                 // Set handler
        sigmanager[sig]=NULL;
        return false;
        }
#endif
      return true;
      }
    }
  return false;
  }


// Remove signal from signal-set observed by the dispatcher
FXbool FXDispatcher::remSignal(FXint sig){
  if(initialized){
    if(sigmanager[sig]==this){
#ifdef WIN32
      if(signal(sig,SIG_DFL)==SIG_ERR){                 // Unset handler
        signotified[sig]=0;                             // Set non-raised
        sigmanager[sig]=NULL;                           // Now release it
        return false;
        }
#elif (_POSIX_C_SOURCE >= 200112L) || (_XOPEN_SOURCE >= 600)
      struct sigaction sigact;
      sigact.sa_handler=SIG_DFL;
      sigact.sa_flags=0;
      sigemptyset(&sigact.sa_mask);
      if(sigaction(sig,&sigact,NULL)==-1){              // Unset handler
        sigmanager[sig]=NULL;                           // Now release it
        signotified[sig]=0;                             // Set non-raised
        return false;
        }
#else
      if(signal(sig,SIG_DFL)==SIG_ERR){                 // Unset handler
        sigmanager[sig]=NULL;                           // Now release it
        signotified[sig]=0;                             // Set non-raised
        return false;
        }
#endif
      sigmanager[sig]=NULL;                             // Now release it
      signotified[sig]=0;                               // Set non-raised
      return true;
      }
    }
  return false;
  }


// Remove all signals
FXbool FXDispatcher::remAllSignals(){
  for(FXuint sig=1; sig<ARRAYNUMBER(sigmanager); ++sig){
    if(!remSignal(sig)) return false;
    }
  return true;
  }

/*******************************************************************************/

// Append new handle hnd to watch-list
FXbool FXDispatcher::addHandle(FXInputHandle hnd,FXuint mode){
  FXTRACE((11,"FXDispatcher::addHandle(%d,%d)\n",hnd,mode));
  if(initialized){
#if defined(WIN32)
    if(hnd!=BadHandle && numhandles<MAXIMUM_WAIT_OBJECTS){
      for(FXint i=numhandles-1; i>=0; --i){
        if(handles->handles[i]==hnd) return false;
        }
      handles->handles[numhandles]=hnd;
      handles->modes[numhandles]=mode;
      numhandles++;
      return true;
      }
#elif defined(HAVE_EPOLL_CREATE1)
    if(0<=hnd){
      struct epoll_event ev;
      ev.events=0;
      ev.data.fd=hnd;
      if(mode&InputRead) ev.events|=EPOLLIN;
      if(mode&InputWrite) ev.events|=EPOLLOUT;
      if(mode&InputExcept) ev.events|=EPOLLPRI;
      if(::epoll_ctl(handles->handle,EPOLL_CTL_ADD,hnd,&ev)!=0){ return false; }
      numhandles++;
      return true;
      }
#else
    if(0<=hnd && hnd<FD_SETSIZE && !(FD_ISSET(hnd,&handles->handles[0]) || FD_ISSET(hnd,&handles->handles[1]) || FD_ISSET(hnd,&handles->handles[2]))){
      if(mode&InputRead){ FD_SET(hnd,&handles->handles[0]); }
      if(mode&InputWrite){ FD_SET(hnd,&handles->handles[1]); }
      if(mode&InputExcept){ FD_SET(hnd,&handles->handles[2]); }
      if(numhandles<=hnd){ numhandles=hnd+1; }
      return true;
      }
#endif
    }
  return false;
  }


// Remove handle hnd from list
FXbool FXDispatcher::remHandle(FXInputHandle hnd){
  FXTRACE((11,"FXDispatcher::remHandle(%d)\n",hnd));
  if(initialized){
#if defined(WIN32)
    if(hnd!=BadHandle){
      for(FXint i=numhandles-1; i>=0; --i){
        if(handles->handles[i]==hnd){
          handles->handles[i]=handles->handles[numhandles-1];
          handles->modes[i]=handles->modes[numhandles-1];
          if(current==i) current=-1;                    // Removed a raised handle
          if(current==numhandles-1) current=i;          // Renumbered a raised handle
          numhandles--;
          return true;
          }
        }
      return false;
      }
#elif defined(HAVE_EPOLL_CREATE1)
    if(0<=hnd){
      struct epoll_event ev;
      ev.events=0;              // Doesn't really matter, ignored by kernel
      ev.data.fd=0;
      if(::epoll_ctl(handles->handle,EPOLL_CTL_DEL,hnd,&ev)!=0){ return false; }
      numhandles--;
      return true;
      }
#else
    if(0<=hnd && hnd<numhandles && (FD_ISSET(hnd,&handles->handles[0]) || FD_ISSET(hnd,&handles->handles[1]) || FD_ISSET(hnd,&handles->handles[2]))){
      if(FD_ISSET(hnd,&handles->watched[0])){ FD_CLR(hnd,&handles->watched[0]); numraised--; }
      if(FD_ISSET(hnd,&handles->watched[1])){ FD_CLR(hnd,&handles->watched[1]); numraised--; }
      if(FD_ISSET(hnd,&handles->watched[2])){ FD_CLR(hnd,&handles->watched[2]); numraised--; }
      FD_CLR(hnd,&handles->handles[0]);
      FD_CLR(hnd,&handles->handles[1]);
      FD_CLR(hnd,&handles->handles[2]);
      if(hnd==numhandles-1){
        while(0<numhandles && !FD_ISSET(numhandles-1,&handles->handles[0]) && !FD_ISSET(numhandles-1,&handles->handles[1]) && !FD_ISSET(numhandles-1,&handles->handles[2])){
          numhandles--;
          }
        }
      return true;
      }
#endif
    }
  return false;
  }

/*******************************************************************************/

// The dispatch driver determines which events have taken place and calls the
// appropriate handling routine.
// Only one event is handled each time through the dispatch() routine, since
// event sources may be added or removed each time a handler is called.
// Thus, once a set of events is determined, we need to keep track of this set
// and only check for new events once all the events from the last check have
// been dealt with.

#if defined(WIN32) //////////////////////////////////////////////////////////////

// Dispatch driver
FXbool FXDispatcher::dispatch(FXTime blocking,FXuint flags){
  FXTRACE((10,"FXDispatcher::dispatch(%lld,%x)\n",blocking,flags));
  if(initialized){
    FXInputHandle hnd;
    FXTime now,due,delay,interval;
    FXuint sig,nxt,mode;

    // Loop till we got something
    while(1){

      // Check for timeout
      delay=forever;
      due=getTimeout();
      if(due<forever){
        now=FXThread::time();
        delay=due-now;
        if(delay<FXLONG(1000)){
          if(dispatchTimeout(due)) return true;         // Timer activity
          continue;
          }
        }

      // Check for signal
      sig=nxt=sigreceived;
      if(atomicSet(&signotified[sig],0)){
        do{ nxt=(nxt+63)&63; }while(nxt!=sig && !signotified[nxt]);
        sigreceived=nxt;
        if(dispatchSignal(sig)) return true;            // Signal activity
        continue;
        }

      // Check active handles
      if(0<=current && current<numhandles){
        nxt=(current+1)%numhandles;
        hnd=handles->handles[current];                  // Shuffle raised handle up in the list
        mode=handles->modes[current];                   // To give all handles equal play time
        handles->handles[current]=handles->handles[nxt];
        handles->modes[current]=handles->modes[nxt];
        handles->handles[nxt]=hnd;
        handles->modes[nxt]=mode;
        current=-1;
        if(dispatchHandle(hnd,mode)) return true;       // IO activity
        continue;
        }

      // Select active handles and check signals; don't block
      current=WaitForMultipleObjects(numhandles,handles->handles,false,0);

      // Bad stuff happened
      if(current==WAIT_FAILED || current>=WAIT_ABANDONED_0){
        throw FXFatalException("FXDispatcher::dispatch: error waiting on handles.");
        }

      // No active handles yet; need to wait
      if(current==WAIT_TIMEOUT){

        // Idle callback if we're about to block
        if(dispatchIdle()) return true;                 // Idle activity

        // We're not blocking
        if(blocking<=0) return false;

        // Block this long
        interval=FXMIN(delay,blocking);

        // Select active handles and check signals, waiting for timeout or maximum block time
        current=WaitForMultipleObjects(numhandles,handles->handles,false,(interval<forever) ? (FXuint)((interval+FXLONG(500000))/FXLONG(1000000)) : INFINITE);

        // Bad stuff happened
        if(current==WAIT_FAILED || current>=WAIT_ABANDONED_0){
          throw FXFatalException("FXDispatcher::dispatch: error waiting on handles.");
          }

        // Return if there was no timeout within maximum block time
        if(current==WAIT_TIMEOUT){
          if(blocking<forever){                         // Next blocking period reduced by time already expired
            blocking-=delay;
            if(blocking<=0) return false;               // Nothing happened during blocking period!
            }
          continue;
          }
        }
      }
    }
  return false;
  }

#elif defined(HAVE_EPOLL_CREATE1) ///////////////////////////////////////////////

// Dispatch driver
FXbool FXDispatcher::dispatch(FXTime blocking,FXuint flags){
  FXTRACE((10,"FXDispatcher::dispatch(%lld,%x)\n",blocking,flags));
  if(initialized){
    FXInputHandle hnd;
    FXTime now,due,delay,interval;
    FXuint sig,nxt,mode;

    // Loop till we got something
    while(1){

      // Check for timeout
      delay=forever;
      due=getTimeout();
      if(due<forever){
        now=FXThread::time();
        delay=due-now;
        if(delay<FXLONG(1000)){
          if(dispatchTimeout(due)) return true;         // Time activity
          continue;
          }
        }

      // Check for signal
      sig=nxt=sigreceived;
      if(atomicSet(&signotified[sig],0)){
        do{ nxt=(nxt+63)&63; }while(nxt!=sig && !signotified[nxt]);
        sigreceived=nxt;
        if(dispatchSignal(sig)) return true;            // Signal activity
        continue;
        }

      // Check active handles
      if(0<numraised){
        mode=0;
        numraised--;
        current=(current+1)%numwatched;
        hnd=handles->events[current].data.fd;
        if(handles->events[current].events&EPOLLIN){ mode|=InputRead; }
        if(handles->events[current].events&EPOLLOUT){ mode|=InputWrite; }
        if(handles->events[current].events&EPOLLERR){ mode|=InputExcept; }
        if(dispatchHandle(hnd,mode)) return true;       // IO activity
        continue;
        }

      // Select active handles and check signals; don't block
      numwatched=epoll_pwait(handles->handle,handles->events,ARRAYNUMBER(handles->events),0,NULL);

      // Bad stuff happened
      if(numwatched<0){
        if(errno!=EAGAIN && errno!=EINTR){ throw FXFatalException("FXDispatcher::dispatch: error waiting on handles."); }
        continue;
        }

      // No active handles yet; need to wait
      if(numwatched==0){

        // Idle callback if we're about to block
        if(dispatchIdle()) return true;                 // Idle activity

        // We're not blocking
        if(blocking<=0) return false;

        // Nanoseconds to wait
        interval=FXMIN(delay,blocking);

        // Select active handles and check signals, waiting for timeout or maximum block time
        numwatched=epoll_pwait(handles->handle,handles->events,ARRAYNUMBER(handles->events),(interval<forever) ? (FXint)((interval+FXLONG(500000))/FXLONG(1000000)) : -1,NULL);

        // Bad stuff happened
        if(numwatched<0){
          if(errno!=EAGAIN && errno!=EINTR){ throw FXFatalException("FXDispatcher::dispatch: error waiting on handles."); }
          continue;
          }

        // Return if there was no timeout within maximum block time
        if(numwatched==0){
          if(blocking<forever){                         // Next blocking period reduced by time already expired
            blocking-=delay;
            if(blocking<=0) return false;               // Nothing happened during blocking period!
            }
          continue;
          }
        }
      numraised=numwatched;
      }
    }
  return false;
  }

#else ///////////////////////////////////////////////////////////////////////////

// Helper function
static FXint sselect(FXint nfds,fd_set* readfds,fd_set* writefds,fd_set* errorfds,FXTime wait,const sigset_t* watchset){
#if (_POSIX_C_SOURCE >= 200112L) || (_XOPEN_SOURCE >= 600)
  FXint result;
  if(wait<forever){
    struct timespec delta;
    delta.tv_nsec=wait%1000000000LL;
    delta.tv_sec=wait/1000000000LL;
    result=pselect(nfds,readfds,writefds,errorfds,&delta,watchset);
    }
  else{
    result=pselect(nfds,readfds,writefds,errorfds,NULL,watchset);
    }
  return result;
#else
  FXint result;
  if(wait<forever){
    struct timeval delta;
    wait=(wait+500LL)/1000LL;
    delta.tv_usec=wait%1000000LL;
    delta.tv_sec=wait/1000000LL;
    result=select(nfds,readfds,writefds,errorfds,&delta);
    }
  else{
    result=select(nfds,readfds,writefds,errorfds,NULL);
    }
  return result;
#endif
  }


// Dispatch driver
FXbool FXDispatcher::dispatch(FXTime blocking,FXuint flags){
  FXTRACE((10,"FXDispatcher::dispatch(%lld,%x)\n",blocking,flags));
  if(initialized){
    FXTime now,due,delay,interval;
    FXuint sig,nxt,mode;

    // Loop till we got something
    while(1){

      // Check for timeout
      delay=forever;
      due=getTimeout();
      if(due<forever){
        now=FXThread::time();
        delay=due-now;
        if(delay<FXLONG(1000)){
          if(dispatchTimeout(due)) return true;         // Timer activity
          continue;
          }
        }

      // Check for signal
      sig=nxt=sigreceived;
      if(atomicSet(&signotified[sig],0)){
        do{ nxt=(nxt+63)&63; }while(nxt!=sig && !signotified[nxt]);
        sigreceived=nxt;
        if(dispatchSignal(sig)) return true;            // Signal activity
        continue;
        }

      // Check active handles
      if(0<numraised){
        mode=0;
        do{
          current=(current+1)%numwatched;
          if(FD_ISSET(current,&handles->watched[0])){
            FD_CLR(current,&handles->watched[0]);
            mode|=InputRead;
            numraised--;
            }
          if(FD_ISSET(current,&handles->watched[1])){
            FD_CLR(current,&handles->watched[1]);
            mode|=InputWrite;
            numraised--;
            }
          if(FD_ISSET(current,&handles->watched[2])){
            FD_CLR(current,&handles->watched[2]);
            mode|=InputExcept;
            numraised--;
            }
          }
        while(mode==0);
        if(dispatchHandle(current,mode)) return true;   // IO activity
        continue;
        }

      // Prepare handles to check
      handles->watched[0]=handles->handles[0];
      handles->watched[1]=handles->handles[1];
      handles->watched[2]=handles->handles[2];

      // Select active handles and check signals; don't block
      numraised=sselect(numhandles,&handles->watched[0],&handles->watched[1],&handles->watched[2],0,NULL);

      // Bad stuff happened
      if(numraised<0){
        if(errno!=EAGAIN && errno!=EINTR){ throw FXFatalException("FXDispatcher::dispatch: error waiting on handles."); }
        continue;
        }

      // No handles were active
      if(numraised==0){

        // Idle callback if we're about to block
        if(dispatchIdle()) return true;                 // Idle activity

        // We're not blocking
        if(blocking<=0) return false;

        // Prepare handles to check
        handles->watched[0]=handles->handles[0];
        handles->watched[1]=handles->handles[1];
        handles->watched[2]=handles->handles[2];

        // Nanoseconds to wait
        interval=FXMIN(delay,blocking);

        // Select active handles and check signals, waiting for timeout or maximum block time
        numraised=sselect(numhandles,&handles->watched[0],&handles->watched[1],&handles->watched[2],interval,NULL);

        // Bad stuff happened
        if(numraised<0){
          if(errno!=EAGAIN && errno!=EINTR){ throw FXFatalException("FXDispatcher::dispatch: error waiting on handles."); }
          continue;
          }

        // Return if there was no timeout within maximum block time
        if(numraised==0){
          if(blocking<forever){                         // Next blocking period reduced by time already expired
            blocking-=delay;
            if(blocking<=0) return false;               // Nothing happened during blocking period!
            }
          continue;
          }
        }
      numwatched=numhandles;
      }
    }
  return false;
  }

#endif //////////////////////////////////////////////////////////////////////////

/*******************************************************************************/

// Dispatch when a signal was fired
FXbool FXDispatcher::dispatchSignal(FXint sig){
  FXTRACE((10,"FXDispatcher::dispatchSignal(%d)\n",sig));
  return false;
  }

/*******************************************************************************/

// Dispatch when when handle hnd is signaled with mode
FXbool FXDispatcher::dispatchHandle(FXInputHandle hnd,FXuint mode){
  FXTRACE((10,"FXDispatcher::dispatchHandle(%d,%x)\n",hnd,mode));
  return false;
  }

/*******************************************************************************/

// Return timeout when something needs to happen
FXTime FXDispatcher::getTimeout(){
  return forever;
  }


// Dispatch when timeout expires
FXbool FXDispatcher::dispatchTimeout(FXTime due){
  FXTRACE((10,"FXDispatcher::dispatchTimeout(%lld)\n",due));
  return false;
  }

/*******************************************************************************/


// Dispatch when idle
FXbool FXDispatcher::dispatchIdle(){
  FXTRACE((10,"FXDispatcher::dispatchIdle()\n"));
  return false;
  }

/*******************************************************************************/

// Exit dispatcher
FXbool FXDispatcher::exit(){
  if(initialized){
    remAllSignals();
#if defined(WIN32)
    ///////////////
#elif defined(HAVE_EPOLL_CREATE1)
    close(handles->handle);
#else
    ///////////////
#endif
    freeElms(handles);
    clearElms(signotified,ARRAYNUMBER(signotified));
    sigreceived=0;
    numhandles=0;
    numwatched=0;
    numraised=0;
    current=-1;
    initialized=false;
    return true;
    }
  return false;
  }


// Destroy dispatcher object
FXDispatcher::~FXDispatcher(){
  exit();
  }

}
