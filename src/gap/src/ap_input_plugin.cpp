#include "ap_defs.h"
#include "ap_utils.h"
#include "ap_event.h"
#include "ap_pipe.h"
#include "ap_format.h"
#include "ap_memory_buffer.h"
#include "ap_input_plugin.h"

using namespace ap;

namespace ap {

InputPlugin::InputPlugin(FXInputHandle f) : fifo(f) {
  }

InputPlugin::~InputPlugin() {
  }

FXival InputPlugin::read(void*data,FXival count){
  FXival nread;
  FXival ncount=count;
  FXchar * buffer = (FXchar *)data;
  while(ncount>0) {
    nread=read_raw(buffer,ncount);
    if (__likely(nread>0)) {
      buffer+=nread;
      ncount-=nread;
      }
    else if (nread==0) { // eof!
      return count-ncount; 
      }
    else if (nread==-2) { // block!
      FXASSERT(handle()!=BadHandle);
      if (!ap_wait_read(fifo,handle()))
        return -1;
      }
    else {
      return -1;
      }
    }
  return count;
  }

}
