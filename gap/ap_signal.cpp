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
#include "ap_signal.h"
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
  #include <signal.h>
  #include <poll.h>
  #include <unistd.h>
  #include <errno.h>
#endif


#ifdef HAVE_EVENTFD
  #include <sys/eventfd.h>
#endif

#ifdef _WIN32
  #include <windows.h>
#endif


namespace ap {


#if !defined(_WIN32) && !defined(HAVE_EVENTFD)
static FXbool create_pipe(FXInputHandle & rh,FXInputHandle & wh) {
  FXInputHandle h[2];
#ifdef HAVE_PIPE2
  if (pipe2(h,O_CLOEXEC)==0) {

    // Set the read end non-blocking
    if (!ap_set_nonblocking(h[0])){
      ::close(h[0]);
      ::close(h[1]);
      return false;
      }

    rh=h[0];
    wh=h[1];
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
    ::close(h[0]);
    ::close(h[1]);
    return false;
    }

  /// Set the read end non-blocking
  if (!ap_set_nonblocking(h[0])){
    ::close(h[0]);
    ::close(h[1]);
    return false;
    }
  rh=h[0];
  wh=h[1];
  return true;
}

#endif



#if !defined(_WIN32) && !defined(HAVE_EVENTFD)
Signal::Signal() : wrptr(BadHandle), device(BadHandle) {}
#else
Signal::Signal() : device(BadHandle) {}
#endif

FXbool Signal::create() {
#if defined(_WIN32)
  device=CreateEvent(nullptr,true,false,nullptr);
  if (device==BadHandle) return false;
#elif defined(HAVE_EVENTFD)
  device=eventfd(0,EFD_CLOEXEC|EFD_NONBLOCK);
  if (device==BadHandle) return false;
#else
  if (!create_pipe(device,wrptr)) return false;
#endif
  return true;
  }

void Signal::set() {
#if defined(_WIN32)
  SetEvent(device);
#elif defined(HAVE_EVENTFD)
  const FXlong value=1;
  write(device,&value,sizeof(FXlong));
#else
  const FXuchar value=1;
  write(wrptr,&value,sizeof(FXuchar));
#endif
  }

void Signal::clear() {
#if defined(_WIN32)
  ResetEvent(device);
#elif defined(HAVE_EVENTFD)
  FXlong value;
  read(device,&value,sizeof(FXlong));
#else
  FXuchar value[16];
  while(read(device,value,16)>0);
#endif
  }

void Signal::close() {
#if defined(_WIN32)
  if(device!=BadHandle) CloseHandle(device);
#elif defined(HAVE_EVENTFD)
  if(device!=BadHandle) ::close(device);
#else
  if(device!=BadHandle) ::close(device);
  if(wrptr!=BadHandle) ::close(wrptr);
#endif
  }

void Signal::wait() {
#if defined(_WIN32)
  WaitForSingleObject(device,INFINITE);
#else
  FXint n;
  struct pollfd fds;
  fds.fd     = device;
  fds.events = POLLIN;
  do {
    n = poll(&fds,1,-1);
    }
  while(n==-1 && (errno==EAGAIN || errno==EINTR));
#endif
  }


WaitEvent Signal::wait(FXInputHandle input,WaitMode mode,FXTime timeout/*=0*/) const{
#ifdef _WIN32
  HANDLE handles[2]={input,device}
  DWORD result=WaitForMultipleObjects(2,handles,false,(timeout>0) ? (timeout / NANOSECONDS_PER_MILLISECOND) : INFINITE);
  if (__likely(result>=WAIT_OBJECT_0)) {
    if (WaitForSingleObject(device,0)==WAIT_OBJECT_0)
      return WaitEvent::Signal;
    else
      return WaitEvent::Input;
    }
  else if (result==WAIT_TIMEOUT) {
    return WaitEvent::Timeout;
    }
  return WaitEvent::Error;
#else
  int n;
  struct pollfd handles[2];
  handles[0].fd=input;
  handles[0].events = (mode==WaitMode::Read) ? POLLIN : POLLOUT;
  handles[1].fd=device;
  handles[1].events = POLLIN;
#ifdef HAVE_PPOL
  struct timespec ts;
  if (timeout) {
    ts.tv_sec  = timeout / NANOSECONDS_PER_SECOND;
    ts.tv_nsec = timeout % NANOSECONDS_PER_SECOND;
    }
x:n=ppoll(handles,2,timeout ? &ts : nullptr,nullptr);
  if (__unlikely(n<0)) {
    if (errno==EAGAIN || errno==EINTR)
      goto x;
    return WaitEvent::Error;
    }
#else
x:n=poll(handles,2,timeout ? (timeout/NANOSECONDS_PER_MILLISECOND) : -1);
  if (__unlikely(n<0)) {
    if (errno==EAGAIN || errno==EINTR)
      goto x;
    return WaitEvent::Error;
    }
#endif
  if(__likely(n>0)) {
    if (handles[1].revents)
      return WaitEvent::Signal;
    else
      return WaitEvent::Input;
    }
  return WaitEvent::Timeout;
#endif
  }



//---------------------------------------------------



#if !defined(_WIN32) && !defined(HAVE_EVENTFD)
Semaphore::Semaphore() : wrptr(BadHandle), device(BadHandle) {}
#else
Semaphore::Semaphore() : device(BadHandle) {}
#endif


FXbool Semaphore::create(FXint count) {
#if defined(_WIN32)
  device=CreateSemaphore(nullptr,count,count,nullptr);
  if (device==BadHandle) return false;
#elif defined(HAVE_EVENTFD)
  device=eventfd(count,EFD_SEMAPHORE|EFD_CLOEXEC|EFD_NONBLOCK);
  if (device==BadHandle) return false;
#else
  if (!create_pipe(device,wrptr)) return false;
  while(count--) release();
#endif
  return true;
  }

void Semaphore::release() {
#if defined(_WIN32)
  ReleaseSemaphore(device,1,nullptr);
#elif defined(HAVE_EVENTFD)
  const FXlong value=1;
  write(device,&value,sizeof(FXlong));
#else
  const FXuchar value=1;
  write(wrptr,&value,sizeof(FXuchar));
#endif
  }


FXbool Semaphore::wait(const Signal & input) {
#if defined(_WIN32)
  HANDLE handles[2]={device,input.handle()}
  DWORD result=WaitForMultipleObjects(2,devices,false,INFINITE);
  if(result==WAIT_OBJECT_0) {
    if (WaitForSingleObject(device,0)==WAIT_OBJECT_0){
      release();
      return false;
      }
    return true;
    }
  return false;
#else

  /// Maybe read first before calling poll??

  struct pollfd fds[2];
  FXint n;
  fds[0].fd     = device;
  fds[0].events = POLLIN;
  fds[1].fd     = input.handle();
  fds[1].events = POLLIN;
  do {
    n = poll(fds,2,-1);
    }
  while(n==-1 && (errno==EAGAIN || errno==EINTR));
  if (n>0) {
    if (fds[1].revents)
      return false;

    if (fds[0].revents) {
#ifdef HAVE_EVENTFD
      FXlong value;
      if (read(device,&value,sizeof(FXlong))==sizeof(FXlong)){
        return true;
        }
#else
      FXuchar value;
      if (read(device,&value,sizeof(FXuchar))==sizeof(FXuchar)){
        return true;
        }
#endif
      // This shouldn't happen in a single consumer version
      FXASSERT(0);
      fxwarning("Fatal in Semaphore::wait");
      }
    }
  return false;
#endif
  }



void Semaphore::close() {
#if defined(_WIN32)
  if(device!=BadHandle) CloseHandle(device);
#elif defined(HAVE_EVENTFD)
  if(device!=BadHandle) ::close(device);
#else
  if(device!=BadHandle) ::close(device);
  if(wrptr!=BadHandle) ::close(wrptr);
#endif
  }

/*
FXbool Semaphore::trywait() {
#if defined(_WIN32)
  return WaitForSingleObject(device,0)==WAIT_OBJECT_0;
#elif defined(HAVE_EVENTFD)
  FXlong value;
  if (read(device,&value,sizeof(FXlong))==sizeof(FXlong)){
    return true;
    }
  else {
    return false;
    }
#else
  FXuchar value;
  if (read(device,&value,sizeof(FXuchar))==sizeof(FXuchar))
    return true;
  else
    return false;
#endif
  }
*/

}
