#include "ap_defs.h"
#include "ap_utils.h"

#ifdef __linux__
#include <sys/prctl.h>
#endif

namespace ap {

void ap_get_version(FXuchar & major,FXuchar & minor) {
  major=AP_MAJOR;
  minor=AP_MINOR;
  }

FXbool ap_check_version(FXuchar major,FXuchar minor) {
  /// For now, there is no stable api
  if ((major!=AP_MAJOR) || (minor!=AP_MINOR))
    return false;
  else
    return true;
  }



void ap_set_thread_name(const FXchar * name) {
#ifdef __linux__
  prctl(PR_SET_NAME,(unsigned long)name,0,0,0);
#endif
  }

FXString ap_get_environment(const FXchar * key,const FXchar * def) {
  FXString value = FXSystem::getEnvironment(key);
  if (value.empty())
    return def;
  else
    return value;
  }



#if 0

FXuint ap_wait(FXInputHandle handle,FXlong timeout) {
#ifndef WIN32
  struct timespec delta;

  fd_set rd;
  fd_set wr;
  fd_set er;

  FD_ZERO(&rd);
  FD_ZERO(&wr);
  FD_ZERO(&er);

  FD_SET((int)handle,&rd);
  FD_SET((int)handle,&er);
  int maxfds=(int)handle;

  delta.tv_nsec=blocking%1000000000;
  delta.tv_sec=blocking/1000000000;

  if (pselect(maxfds+1,&rd,&wr,&er,&delta,NULL)>0) {
    if (FD_ISSET(handle,&rd) || FD_ISSET(handle,&er)) {
      return 1;
      }
    }
#endif
  return 0;
  }
#endif

FXuint ap_wait(FXInputHandle handle) {
#ifndef WIN32
  fd_set rd;
  fd_set wr;
  fd_set er;

  FD_ZERO(&rd);
  FD_ZERO(&wr);
  FD_ZERO(&er);

  FD_SET((int)handle,&rd);
  FD_SET((int)handle,&er);
  int maxfds=(int)handle;

  if (pselect(maxfds+1,&rd,&wr,&er,NULL,NULL)>0) {
    if (FD_ISSET(handle,&rd) || FD_ISSET(handle,&er)) {
      return 1;
      }
    }
  #endif
  return 0;
  }

FXuint ap_wait(FXInputHandle h1,FXInputHandle h2) {
  return ap_wait_read(h1,h2,0);
  }


FXuint ap_wait_write(FXInputHandle interrupt,FXInputHandle handle,FXTime timeout) {
//  fxmessage("Wait for write\n");
#ifndef WIN32
  FXint result=WIO_TIMEOUT;
  int maxfds=FXMAX(interrupt,handle);
  struct timespec delta;

  fd_set rd;
  fd_set wr;
  fd_set er;

  FD_ZERO(&rd);
  FD_ZERO(&wr);
  FD_ZERO(&er);

  FD_SET((int)interrupt,&rd);
  FD_SET((int)interrupt,&er);
  FD_SET((int)handle,&wr);
  FD_SET((int)handle,&er);

  if (timeout) {
    delta.tv_nsec=timeout%1000000000;
    delta.tv_sec=timeout/1000000000;
    if (pselect(maxfds+1,&rd,&wr,&er,&delta,NULL)) {
      if (FD_ISSET((int)interrupt,&rd) || FD_ISSET((int)interrupt,&er) )
        result|=WIO_INTERRUPT;

      if(FD_ISSET((int)handle,&wr) || FD_ISSET((int)handle,&er))
        result|=WIO_HANDLE;
      }
    }
  else {
    if (pselect(maxfds+1,&rd,&wr,&er,NULL,NULL)) {
      if (FD_ISSET((int)interrupt,&rd) || FD_ISSET((int)interrupt,&er) )
        result|=WIO_INTERRUPT;

      if(FD_ISSET((int)handle,&wr) || FD_ISSET((int)handle,&er))
        result|=WIO_HANDLE;
      }
    }
  return result;
#endif
  }



FXuint ap_wait_read(FXInputHandle interrupt,FXInputHandle handle,FXTime timeout){
//  fxmessage("Wait for read\n");
#ifndef WIN32
  FXuint result=WIO_TIMEOUT;
  int maxfds=FXMAX(interrupt,handle);
  struct timespec delta;

  fd_set rd;
  fd_set wr;
  fd_set er;

  FD_ZERO(&rd);
  FD_ZERO(&wr);
  FD_ZERO(&er);

  FD_SET((int)interrupt,&rd);
  FD_SET((int)interrupt,&er);
  FD_SET((int)handle,&rd);
  FD_SET((int)handle,&er);

  if (timeout) {
    delta.tv_nsec=timeout%1000000000;
    delta.tv_sec=timeout/1000000000;
    if (pselect(maxfds+1,&rd,&wr,&er,&delta,NULL)) {
      if (FD_ISSET((int)interrupt,&rd) || FD_ISSET((int)interrupt,&er)){
        result|=WIO_INTERRUPT;
        }
      if(FD_ISSET((int)handle,&rd) || FD_ISSET((int)handle,&er)) {
        result|=WIO_HANDLE;
        }
      }
    }
  else {
    if (pselect(maxfds+1,&rd,&wr,&er,NULL,NULL)) {
      if (FD_ISSET((int)interrupt,&rd) || FD_ISSET((int)interrupt,&er)){
        result|=WIO_INTERRUPT;
        }
      if(FD_ISSET((int)handle,&rd) || FD_ISSET((int)handle,&er)) {
        result|=WIO_HANDLE;
        }
      }
    }
  return result;
#endif
  }



}
