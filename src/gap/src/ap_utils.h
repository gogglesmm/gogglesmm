#ifndef AP_UTILS_H
#define AP_UTILS_H

#include "ap_common.h"

namespace ap {


extern GMAPI FXString ap_get_environment(const FXchar * key,const FXchar * def=NULL);

extern FXbool ap_set_closeonexec(FXInputHandle fd);

enum {
  WIO_TIMEOUT   = 0,
  WIO_INTERRUPT = 0x1,
  WIO_HANDLE    = 0x2,
  WIO_BOTH      = (WIO_INTERRUPT|WIO_HANDLE),
  };

extern FXuint ap_wait(FXInputHandle handle,FXTime timeout=0);

extern FXuint ap_wait_write(FXInputHandle interrupt,FXInputHandle handle,FXTime timeout=30000000000);

extern FXuint ap_wait_read(FXInputHandle interrupt,FXInputHandle handle,FXTime timeout=30000000000);


}
#endif

