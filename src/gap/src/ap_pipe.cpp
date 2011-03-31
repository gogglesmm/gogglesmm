#include "ap_defs.h"
#include "ap_event.h"
#include "ap_pipe.h"

#ifndef WIN32
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#endif

/// On Linux we want to use eventfd
#if defined(__linux__) && defined(__GLIBC__) && (__GLIBC__ > 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 8))
#define HAVE_EVENTFD
#endif

#ifdef HAVE_EVENTFD
#include <sys/eventfd.h>
#endif


namespace ap {


#ifndef WIN32
static FXbool ap_set_nonblocking(FXint fd) {
  FXint flags;

  flags = fcntl(fd, F_GETFL);
  if (flags==-1) return false;

  flags |= O_NONBLOCK;

  if (fcntl(fd,F_SETFL,flags)==-1)
    return false;

  return true;
  }

static FXbool ap_set_closeonexec(FXint fd) {
  FXint flags;

  flags = fcntl(fd, F_GETFD);
  if (flags==-1) return false;

  flags |= FD_CLOEXEC;

  if (fcntl(fd,F_SETFD,flags)==-1)
    return false;

  return true;
  }
#endif



Pipe::Pipe() {
  h[0]=BadHandle;
  h[1]=BadHandle;
  }

Pipe::~Pipe() {
  close();
  }

FXbool Pipe::create() {
#ifdef WIN32
  if (CreatePipe(&h[0],&h[1],NULL,0)==0)
    return false;
#else
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
#ifdef WIN32
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
#ifdef WIN32
  DWORD nw;
  WriteFile(device,&ptr,(DWORD)sizeof(Event*),&nw,NULL);
#else
  write(h[1],&ptr,sizeof(Event*));
#endif
  }

Event * EventPipe::pop() {
#ifdef WIN32
  Event * ptr = NULL;
  DWORD nr;
  if(::ReadFile(device,&ptr,(DWORD)sizeof(Event*),&nr,NULL)!=0 && nr==sizeof(Event*)){
    return ptr;
    }
  return NULL;
#else
  Event * ptr = NULL;
  if (read(h[0],&ptr,sizeof(Event*))==sizeof(Event*))
    return ptr;
  else
    return NULL;
#endif
  }


NotifyPipe::NotifyPipe() {
  }

NotifyPipe::~NotifyPipe() {
  }

FXbool NotifyPipe::create() {
#if defined(WIN32)
  h[0]=CreateEvent(NULL,TRUE,FALSE,NULL);
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
#if defined(WIN32)
  ResetEvent(h[0]);
#elif defined(HAVE_EVENTFD)
  FXlong value;
  while(read(h[0],&value,sizeof(FXlong))>0);
//  read(h[0],&value,sizeof(FXlong));
#else
  FXchar buf[16];
  FXint result;
  while((result = read(h[0],buf,16))>0) ;
#endif
  }

void NotifyPipe::signal() {
#if defined(WIN32)
  SetEvent(h[0]);
#elif defined(HAVE_EVENTFD)
  const FXlong value=1;
  write(h[0],&value,sizeof(FXlong));
#else
  const FXchar buf=1;
  write(h[1],&buf,1);
#endif
  }

}
