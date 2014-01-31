/*******************************************************************************
*                         Goggles Audio Player Library                         *
********************************************************************************
*           Copyright (C) 2010-2014 by Sander Jansen. All Rights Reserved      *
*                               ---                                            *
* This program is free software: you can redistribute it and/or modify         *
* it under the terms of the GNU General Public License as published by         *
* the Free Software Foundation, either version 3 of the License, or            *
* (at your option) any later version.                                          *
*                                                                              *
* This program is distributed in the hope that it will be useful,              *
* but WITHOUT ANY WARRANTY; without even the implied warranty of               *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                *
* GNU General Public License for more details.                                 *
*                                                                              *
* You should have received a copy of the GNU General Public License            *
* along with this program.  If not, see http://www.gnu.org/licenses.           *
********************************************************************************/
#include "ap_defs.h"
#include "ap_reactor.h"

#ifndef WIN32
#include <poll.h>
#include <errno.h>
#endif

namespace ap {

Reactor::Reactor() : pfds(NULL),nfds(0),mfds(0), timers(NULL) {
  }

Reactor::~Reactor() {
#ifndef WIN32
  freeElms(pfds);
#endif
  
  /// Delete all inputs
  for (FXint i=0;i<inputs.no();i++){
    delete inputs[i];
    }
  inputs.clear();

  /// Delete all deferred
  for (FXint i=0;i<deferred.no();i++){
    delete deferred[i];
    }
  deferred.clear();

  /// Delete all timers
  while(timers) {
    Timer * n = timers->next;
    delete timers;
    timers=n;
    }

  }

#ifdef DEBUG
void Reactor::debug() {
  int ntimers = 0;
  for (Timer * t=timers;t;t=t->next) ntimers++;
  fxmessage("Reactor timers=%d inputs=%d deferred=%d\n",ntimers,inputs.no(),deferred.no());
  }
#endif

void Reactor::dispatch() {
#ifndef WIN32
  FXint i;

  for (i=inputs.no()-1;i>=0;i--) {
    if (pfds[i].revents) { 
      inputs[i]->mode|=((pfds[i].revents&POLLIN )          ? Input::IsReadable : 0);
      inputs[i]->mode|=((pfds[i].revents&POLLOUT)          ? Input::IsWritable : 0);
      inputs[i]->mode|=((pfds[i].revents&(POLLERR|POLLHUP))? Input::IsException: 0);
      inputs[i]->onSignal();
      }
    }
  
  FXint offset=inputs.no();
  for (i=0;i<native.no();i++){
    native[i]->dispatch(pfds+offset);
    offset+=native[i]->no();
    }

  FXTime now = FXThread::time();
  for (Timer * t = timers;t;t=t->next) {
    if (t->time>0 && t->time<=now) {
      t->time=0;
      t->onExpired();
      t = timers; // so onExpired can remove itself from the list...
      }
    }
#endif
  }


void Reactor::wait(FXTime timeout) {
#ifndef WIN32
  FXint n;
  if (timeout>=0) {
    struct timespec ts;
    ts.tv_sec  = timeout / 1000000000;
    ts.tv_nsec = timeout % 1000000000;
    do {
      n = ppoll(pfds,nfds,&ts,NULL);
      }
    while(n==-1 && errno==EINTR);
    }
  else {
    do {
      n = ppoll(pfds,nfds,NULL,NULL);
      }
    while(n==-1 && errno==EINTR);
    }    
#endif
  }

FXTime Reactor::prepare() {
  FXTime timeout;
  FXint i;
#ifndef WIN32
  nfds = inputs.no();
  for (i=0;i<native.no();i++) nfds+=native[i]->no();

  if (nfds>mfds) {  
    mfds=nfds;
    if (pfds==NULL)
      allocElms(pfds,mfds);
    else
      resizeElms(pfds,mfds);
    }

  for (FXint i=0;i<inputs.no();i++) {
    if ((inputs[i]->mode&Input::Disabled) || (inputs[i]->mode&(Input::Readable|Input::Writable|Input::Exception))==0) {
      pfds[i].fd       = -inputs[i]->handle;
      pfds[i].events   = 0;
      pfds[i].revents  = 0;
      }
    else {
      pfds[i].fd       = inputs[i]->handle;
      pfds[i].events   = 0;
      pfds[i].events  |= (inputs[i]->mode&Input::Readable)  ? POLLIN : 0;
      pfds[i].events  |= (inputs[i]->mode&Input::Writable)  ? POLLOUT : 0;
      pfds[i].events  |= (inputs[i]->mode&Input::Exception) ? POLLERR|POLLHUP : 0;
      pfds[i].revents  = 0;
      inputs[i]->mode&=~(Input::IsReadable|Input::IsWritable|Input::IsException);
      }
    }

  FXint offset = inputs.no();
  for (i=0;i<native.no();i++) {
    native[i]->prepare(pfds+offset);
    offset+=native[i]->no();
    }

  FXTime now = FXThread::time();
  Timer * t = timers;
  while(t && t->time<now) t=t->next;
  if (t) 
    timeout = t->time - now;   
  else
    timeout = -1;

  return timeout;    
#endif
  }


void Reactor::addNative(Native*n) {
  native.append(n);
  }

void Reactor::removeNative(Native*n){
  native.remove(n);
  }


void Reactor::addInput(Input*w) {
  inputs.append(w);
  }

void Reactor::removeInput(Input*w){
  inputs.remove(w);
  }



void Reactor::addTimer(Timer*t,FXTime time) {
  t->time = time;
  Timer**tt;
  for (tt=&timers;*tt &&((*tt)->time<t->time);tt=&(*tt)->next) {}
  t->next=*tt;
  *tt=t;
  }

void Reactor::removeTimer(Timer*timer) {
  for (Timer**tt=&timers;*tt;tt=&(*tt)->next){
    if ((*tt)==timer){
      Timer * next = (*tt)->next;
      *tt=next;
      break;
      }
    }
  }


void Reactor::addDeferred(Deferred*d) {
  deferred.append(d);
  }

void Reactor::removeDeferred(Deferred*d){
  deferred.remove(d);
  }


FXbool Reactor::dispatchDeferred() {
  FXbool done=false;
  for (FXint i=deferred.no()-1;i>=0;i--){
    if ((deferred[i]->mode&Deferred::Disabled)==0) {
      deferred[i]->run(); 
      done=true;
      }
    }
  return done;
  }

void Reactor::runPending() {
  if (dispatchDeferred()==false) {
    prepare();
    wait(0);
    dispatch();
    }
  }

void Reactor::runOnce() {
  if (dispatchDeferred()==false) {
    FXTime timeout = prepare();
    wait(timeout);
    dispatch();
    } 
  }

void Reactor::runOnce(FXTime wakeup) {
  if (dispatchDeferred()==false) {
    FXTime timeout = prepare();
    if (timeout<0 || wakeup<timeout)
      timeout = wakeup;
    wait(timeout);
    dispatch();
    } 
  }



}
