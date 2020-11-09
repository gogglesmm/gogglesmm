/********************************************************************************
*                                                                               *
*                      C a l l b a c k   D i s p a t c h e r                    *
*                                                                               *
*********************************************************************************
* Copyright (C) 2006,2020 by Jeroen van der Zijp.   All Rights Reserved.        *
********************************************************************************/
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "fxmath.h"
#include "FXAtomic.h"
#include "FXElement.h"
#include "FXHash.h"
#include "FXCallback.h"
#include "FXAutoThreadStorageKey.h"
#include "FXThread.h"
#include "FXException.h"
#include "FXReactor.h"
#include "FXDispatcher.h"

/*
  Notes:

  - FXDispatcher extends FXReactor by implementing convenient callbacks for
    handles, signals, timers, and idle processing performed by FXReactor.

  - The callbacks may connect to member functions of classes, but also global
    functions.

  - Special handles [e.g. wakeup pipe or display connection], may be added by
    addHandle(hnd,mode) instead of addHandle(cb,hnd,mode,ptr).  In this case,
    no hash entry will be added, which means when these types of handles are
    dispatched via overrides of dispatchHandle(), they must be filtered out prior
    to being processed by this implementation of dispatchHandle(); otherwise, a
    core dump may result.

  - Likewise, addSignal(sig,async) may be used in lieu of addSignal(cb,sig,ptr,async)
    to establis special ways of handling select signals.  The addSignal(sig,async)
    will not establish a handler callback, and thus when such a signal is raised,
    it must be filtered via overrides of dispatchSignal prior to being processed
    by this implementation of dispatchSignal(); otherwise, a core dump may result.
*/

using namespace FX;

/*******************************************************************************/

namespace FX {


// Handle callback
struct FXDispatcher::Handle {
  HandleCallback     cb;          // Callback
  void              *ptr;         // User data
  };


// Signal callback
struct FXDispatcher::Signal {
  SignalCallback     cb;          // Callback
  void              *ptr;         // User data
  };


// Timer callback
struct FXDispatcher::Timer {
  TimeoutCallback    cb;          // Callback
  FXTime             due;         // When timer is due (ns)
  Timer             *next;        // Next timeout in list
  void              *ptr;         // User data
  };


// Idle callback
struct FXDispatcher::Idle {
  IdleCallback       cb;          // Callback
  Idle              *next;        // Next chore in list
  void              *ptr;         // User data
  };


/*******************************************************************************/

// Construct dispatcher object
FXDispatcher::FXDispatcher():signals(NULL),timers(NULL),idles(NULL),timerrecs(NULL),idlerecs(NULL){
  }


// Initialize dispatcher
FXbool FXDispatcher::init(){
  if(FXReactor::init()){
    callocElms(signals,64);
    timers=NULL;
    idles=NULL;
    timerrecs=NULL;
    idlerecs=NULL;
    return true;
    }
  return false;
  }

/*******************************************************************************/

// Add signal to signal-set observed by the dispatcher
FXbool FXDispatcher::addSignal(SignalCallback cb,FXint sig,void* ptr,FXbool async){
  if(FXReactor::addSignal(sig,async)){
    signals[sig]=new Signal;
    signals[sig]->cb=cb;                                // Set callback
    signals[sig]->ptr=ptr;                              // Set pointer
    return true;
    }
  return false;
  }


// Add signal to signal-set observed by the dispatcher
FXbool FXDispatcher::addSignal(FXint sig,FXbool async){
  return FXReactor::addSignal(sig,async);
  }


// Remove signal from signal-set observed by the dispatcher
FXbool FXDispatcher::remSignal(FXint sig){
  if(FXReactor::remSignal(sig)){
    delete signals[sig];
    signals[sig]=NULL;
    return true;
    }
  return false;
  }


// Dispatch when a signal was fired
FXbool FXDispatcher::dispatchSignal(FXint sig){
  if(hasSignal(sig)){
    return signals[sig] && signals[sig]->cb(this,sig,signals[sig]->ptr);
    }
  return false;
  }

/*******************************************************************************/

// Add timeout callback cb at time due (ns since Epoch).
void* FXDispatcher::addTimeout(TimeoutCallback cb,FXTime due,void* ptr){
  void* res=NULL;
  if(isInitialized()){
    Timer **tt=&timers,*t,*x;
    while((x=*tt)!=NULL){
      if(x->cb==cb){ *tt=x->next; res=x->ptr; t=x; goto a; }
      tt=&x->next;
      }
    if(timerrecs){
      t=timerrecs;
      timerrecs=t->next;
      }
    else{
      t=new Timer;
      }
a:  t->cb=cb;
    t->due=due;
    t->next=NULL;
    t->ptr=ptr;
    tt=&timers;
    while((x=*tt) && x->due<t->due){
      tt=&x->next;
      }
    t->next=*tt;
    *tt=t;
    }
  return res;
  }


// Add timeout callback cb after time interval (ns).
void* FXDispatcher::addInterval(TimeoutCallback cb,FXTime interval,void* ptr){
  return addTimeout(cb,FXThread::time()+interval,ptr);
  }


// Remove timeout callback cb.
void* FXDispatcher::remTimeout(TimeoutCallback cb){
  void* res=NULL;
  if(isInitialized()){
    Timer **tt=&timers,*t;
    while((t=*tt)!=NULL){
      if(t->cb==cb){
        *tt=t->next;
        res=t->ptr;
        t->next=timerrecs;
        timerrecs=t;
        continue;
        }
      tt=&t->next;
      }
    }
  return res;
  }


// Return the remaining time, in nanoseconds
FXTime FXDispatcher::getTimeout(TimeoutCallback cb) const {
  for(Timer *t=timers; t; t=t->next){
    if(t->cb==cb) return t->due;
    }
  return forever;
  }


// Return timeout when something needs to happen
FXTime FXDispatcher::nextTimeout(){
  return timers ? timers->due : forever;
  }


// Return true if timeout callback cb been set.
FXbool FXDispatcher::hasTimeout(TimeoutCallback cb) const {
  for(Timer *t=timers; t; t=t->next){
    if(t->cb==cb) return true;
    }
  return false;
  }


// Dispatch when timeout expires
FXbool FXDispatcher::dispatchTimeout(FXTime due){
  if(timers && timers->due<=due){
    Timer *t=timers;
    timers=t->next;
    t->next=timerrecs;
    timerrecs=t;
    return t->cb(this,t->due,t->ptr);
    }
  return false;
  }

/*******************************************************************************/

// Add idle callback be executed when dispatch about to block.
void* FXDispatcher::addIdle(IdleCallback cb,void* ptr){
  void* res=NULL;
  if(isInitialized()){
    Idle **cc=&idles,*c,*x;
    while((x=*cc)!=NULL){         // Search list for cb
      if(x->cb==cb){
        *cc=x->next; res=x->ptr; c=x;
        goto a;
        }
      cc=&x->next;
      }
    if(idlerecs){                 // Recycled chore
      c=idlerecs;
      idlerecs=c->next;
      }
    else{                         // Fresh chore
      c=new Idle;
      }
a:  c->cb=cb;
    c->ptr=ptr;
    c->next=NULL;
    while((x=*cc)!=NULL){         // Continue to end of list
      cc=&x->next;
      }
    *cc=c;
    }
  return res;
  }


// Remove idle callback cb.
void* FXDispatcher::remIdle(IdleCallback cb){
  void *res=NULL;
  if(isInitialized()){
    Idle **cc=&idles,*c;
    while((c=*cc)!=NULL){
      if(c->cb==cb){
        *cc=c->next;
        res=c->ptr;
        c->next=idlerecs;
        idlerecs=c;
        continue;
        }
      cc=&c->next;
      }
    }
  return res;
  }


// Return true if idle callback cb been set.
FXbool FXDispatcher::hasIdle(IdleCallback cb) const {
  for(Idle *c=idles; c; c=c->next){
    if(c->cb==cb) return true;
    }
  return false;
  }


// Dispatch one idle callback.
FXbool FXDispatcher::dispatchIdle(){
  if(idles){
    Idle *c=idles;
    idles=c->next;
    c->next=idlerecs;
    idlerecs=c;
    return c->cb(this,c->ptr);
    }
  return false;
  }

/*******************************************************************************/

// Add callback cb with new handle hnd to watch-list
FXbool FXDispatcher::addHandle(HandleCallback cb,FXInputHandle hnd,FXuint mode,void* ptr){
  if(FXReactor::addHandle(hnd,mode)){
    Handle *handle=new Handle();
    handle->cb=cb;
    handle->ptr=ptr;
    handles.insert((FXptr)(FXuval)hnd,handle);
    return true;
    }
  return false;
  }


// Add new handle hnd to watch-list
FXbool FXDispatcher::addHandle(FXInputHandle hnd,FXuint mode){
  return FXReactor::addHandle(hnd,mode);
  }


// Remove handle hnd from list
FXbool FXDispatcher::remHandle(FXInputHandle hnd){
  if(FXReactor::remHandle(hnd)){
    Handle *handle=static_cast<Handle*>(handles.remove((FXptr)(FXuval)hnd));
    delete handle;
    return true;
    }
  return false;
  }


// Return true if handle has been set.
FXbool FXDispatcher::hasHandle(FXInputHandle hnd) const {
  if(isInitialized()){
    return 0<=handles.find((FXptr)(FXuval)hnd);
    }
  return false;
  }


// Dispatch when when handle hnd is signaled with mode
FXbool FXDispatcher::dispatchHandle(FXInputHandle hnd,FXuint mode,FXuint flags){
  Handle *handle=static_cast<Handle*>(handles[(FXptr)(FXuval)hnd]);
  return handle && handle->cb(this,hnd,mode,handle->ptr);
  }

/*******************************************************************************/

// Exit dispatcher
FXbool FXDispatcher::exit(){
  if(FXReactor::exit()){
    Idle  *c;
    Timer *t;
    FXival i;
    for(i=0; i<handles.no(); ++i){
      if(handles.empty(i)) continue;
      delete static_cast<Handle*>(handles.data(i));
      }
    while((t=timers)!=NULL){
      timers=t->next;
      delete t;
      }
    while((t=timerrecs)!=NULL){
      timerrecs=t->next;
      delete t;
      }
    while((c=idles)!=NULL){
      idles=c->next;
      delete c;
      }
    while((c=idlerecs)!=NULL){
      idlerecs=c->next;
      delete c;
      }
    for(i=0; i<64; ++i){
      delete signals[i];
      }
    freeElms(signals);
    handles.clear();
    timers=NULL;
    idles=NULL;
    timerrecs=NULL;
    idlerecs=NULL;
    return true;
    }
  return false;
  }


// Destroy dispatcher object
FXDispatcher::~FXDispatcher(){
  exit();
  }

}
