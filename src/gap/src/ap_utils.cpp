#include "ap_defs.h"
#include "ap_utils.h"

#ifdef __linux__
#include <sys/prctl.h>
#endif

void ap_set_thread_name(const FXchar * name) {
#ifdef __linux__
  prctl(PR_SET_NAME,(unsigned long)name,0,0,0);
#endif
  }

void ap_get_device(FXString & device) {
  FXString env = FXSystem::getEnvironment("GAP_DEVICE");
  if (!env.empty()) device=env;
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
#ifndef WIN32
  fd_set rd;
  fd_set wr;
  fd_set er;

  FD_ZERO(&rd);
  FD_ZERO(&wr);
  FD_ZERO(&er);

  FD_SET((int)h1,&rd);
  FD_SET((int)h1,&er);

  FD_SET((int)h2,&rd);
  FD_SET((int)h2,&er);

  int maxfds=FXMAX(h1,h2);

  if (pselect(maxfds+1,&rd,&wr,&er,NULL,NULL)>0) {
    if ( (FD_ISSET(h1,&rd) || FD_ISSET(h1,&er)) &&
         (FD_ISSET(h2,&rd) || FD_ISSET(h2,&er))) {
      return 3;
      }
    else if (FD_ISSET(h1,&rd) || FD_ISSET(h1,&er)) {
      return 1;
      }
    else if (FD_ISSET(h2,&rd) || FD_ISSET(h2,&er)) {
      return 2;
      }
    }
#endif
  return 0;
  }
