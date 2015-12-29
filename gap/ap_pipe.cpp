/*******************************************************************************
*                         Goggles Audio Player Library                         *
********************************************************************************
*           Copyright (C) 2010-2016 by Sander Jansen. All Rights Reserved      *
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
#include "ap_pipe.h"
#include "ap_utils.h"

/// On Linux we want to use pipe2
#if defined(__linux__) && defined(__GLIBC__) && (__GLIBC__ > 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 9))
  #define HAVE_PIPE2
  #ifndef _GNU_SOURCE
  #define _GNU_SOURCE
  #endif
  #include <fcntl.h>
#endif

#ifndef _WIN32
#if defined(__linux__)
  #define HAVE_PPOLL // On Linux we have ppoll
  #ifndef _GNU_SOURCE
    #define _GNU_SOURCE
  #endif
#endif
#include <signal.h>
#include <poll.h>
#include <unistd.h>
#include <errno.h>
#endif

/// On Linux we want to use eventfd
#if defined(__linux__) && defined(__GLIBC__) && (__GLIBC__ > 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 8))
#define HAVE_EVENTFD
#endif

#ifdef HAVE_EVENTFD
#include <sys/eventfd.h>
#endif

#ifdef _WIN32
#include <windows.h>
#endif


namespace ap {



Pipe::Pipe() {
  h[0]=BadHandle;
  h[1]=BadHandle;
  }

Pipe::~Pipe() {
  close();
  }

FXbool Pipe::create() {
#ifdef _WIN32
  if (CreatePipe(&h[0],&h[1],nullptr,0)==0)
    return false;

  // Set the read end non-blocking
  DWORD mode = PIPE_NOWAIT;
  if (SetNamedPipeHandleState(h[0],&mode,nullptr,nullptr)==0)
    return false;
#else

#ifdef HAVE_PIPE2
  if (pipe2(h,O_CLOEXEC)==0) {

    // Set the read end non-blocking
    if (!ap_set_nonblocking(h[0])){
      close();
      return false;
      }

    return true;
    }

  // In case of EINVAL (invalid flags) try again using regular pipe api
  if (errno!=EINVAL)
    return false;
#endif

  /// Create Pipe
  if (pipe(h)==-1)
    return false;

  /// Set the close on exec. flag.
  if (!ap_set_closeonexec(h[0]) || !ap_set_closeonexec(h[1])) {
    close();
    return false;
    }

  /// Set the read end non-blocking
  if (!ap_set_nonblocking(h[0])){
    close();
    return false;
    }
#endif
  return true;
  }


void Pipe::close() {
#ifdef _WIN32
  if (h[0]!=BadHandle) CloseHandle(h[0]);
  if (h[1]!=BadHandle) CloseHandle(h[1]);
  h[0]=h[1]=BadHandle;
#else
  if (h[0]!=BadHandle) ::close(h[0]);
  if (h[1]!=BadHandle) ::close(h[1]);
  h[0]=h[1]=BadHandle;
#endif
  }




EventPipe::EventPipe() {
  }

EventPipe::~EventPipe() {
  }

void EventPipe::push(Event *ptr) {
#ifdef _WIN32
  DWORD nw;
  if (WriteFile(h[1],&ptr,sizeof(Event*),&nw,nullptr)==FALSE)
    fxwarning("gogglesmm: EventPipe::push failed\n");
#else
  if (write(h[1],&ptr,sizeof(Event*))!=sizeof(Event*))
    fxwarning("gogglesmm: EventPipe::push failed\n");
#endif
  }

Event * EventPipe::pop() {
#ifdef _WIN32
  Event * ptr = nullptr;
  DWORD nr;
  if(::ReadFile(h[0],&ptr,(DWORD)sizeof(Event*),&nr,nullptr) && nr==sizeof(Event*)){
    return ptr;
    }
  return nullptr;
#else
  Event * ptr = nullptr;
  if (read(h[0],&ptr,sizeof(Event*))==sizeof(Event*))
    return ptr;
  else
    return nullptr;
#endif
  }

FXbool EventPipe::wait(const NotifyPipe & interrupt) {
#ifdef _WIN32
  HANDLE fds[2] = {interrupt.handle(),h[0]};
  const FXint nfds=2;
  FXuint result = WaitForMultipleObjects(nfds,fds,false,INFINITE);
  if (result == WAIT_OBJECT_0 + 1) {
    return true;
    }
  return false;
#else
  struct pollfd fds[2];
  const FXint nfds=2;
  FXint n;
  fds[0].fd     = interrupt.handle();
  fds[0].events = POLLIN;
  fds[1].fd     = h[0];
  fds[1].events = POLLIN;
  do {
#ifdef HAVE_PPOLL
    n = ppoll(fds,nfds,nullptr,nullptr);
#else
    n = poll(fds,nfds,-1);
#endif
    }
  while(n==-1 && (errno==EAGAIN || errno==EINTR));
  if (0<n && fds[0].revents==0) {
    FXASSERT(fds[1].revents!=0);
    return true;
    }
  return false;
#endif
  }



NotifyPipe::NotifyPipe() {
  }

NotifyPipe::~NotifyPipe() {
  }

FXbool NotifyPipe::create() {
#if defined(_WIN32)
  h[0]=CreateEvent(nullptr,TRUE,FALSE,nullptr);
  if (h[0]==BadHandle)
    return false;
#elif defined(HAVE_EVENTFD)
  h[0]=eventfd(0,EFD_CLOEXEC|EFD_NONBLOCK);
  if (h[0]==BadHandle) {
    if (errno==EINVAL) {
      h[0]=eventfd(0,0); /// try again without flags
      if (h[0]==BadHandle) {
        return false;
        }
      if (!ap_set_closeonexec(h[0])) {
        close();
        FXASSERT(0);
        return false;
        }
      if (!ap_set_nonblocking(h[0])) {
        close();
        FXASSERT(0);
        return false;
        }
      }
    else {
      FXASSERT(0);
      return false;
      }

    }
#else
  return Pipe::create();
#endif
  return true;
  }

void NotifyPipe::clear() {
#if defined(_WIN32)
  ResetEvent(h[0]);
#elif defined(HAVE_EVENTFD)
  FXlong value;
  while(read(h[0],&value,sizeof(FXlong))>0);
#else
  FXchar buf[16];
  FXint result;
  while((result = read(h[0],buf,16))>0) ;
#endif
  }

void NotifyPipe::signal() {
#if defined(_WIN32)
  SetEvent(h[0]);
#elif defined(HAVE_EVENTFD)
  const FXlong value=1;
  if (write(h[0],&value,sizeof(FXlong))!=sizeof(FXlong))
    fxwarning("gogglesmm: NotifyPipe::signal failed\n");
#else
  const FXchar buf=1;
  if (write(h[1],&buf,1)!=1)
    fxwarning("gogglesmm: NotifyPipe::signal failed\n");
#endif
  }


void NotifyPipe::wait() {
#ifdef _WIN32
  if (WaitForSingleObject(h[0],INFINITE)!=WAIT_OBJECT_0){
    fxwarning("gogglesmm: NotifyPipe::wait failed\n");
    }
#else
  FXint nfds=1,n;
  struct pollfd fds;
  fds.fd     = h[0];
  fds.events = POLLIN;
  do {
#ifdef HAVE_PPOLL
    n = ppoll(&fds,nfds,nullptr,nullptr);
#else
    n = poll(&fds,nfds,-1);
#endif
    }
  while(n==-1 && (errno==EAGAIN || errno==EINTR));
  if(n!=1) fxwarning("gogglesmm: NotifyPipe::wait failed\n");
#endif
  }

}
