#include "ap_config.h"
#include "ap_defs.h"
#include "ap_utils.h"
#include "ap_pipe.h"
#include "ap_event.h"
#include "ap_format.h"
#include "ap_memory_buffer.h"
#include "ap_input_plugin.h"
#include "ap_event_queue.h"
#include "ap_thread_queue.h"
#include "ap_packet.h"
#include "ap_engine.h"
#include "ap_thread.h"
#include "ap_thread_queue.h"
#include "ap_input_thread.h"

using namespace ap;

namespace ap {

InputPlugin::InputPlugin(InputThread * i,FXival size) : input(i), buffer(size) {
  }

InputPlugin::InputPlugin(InputThread * i) : input(i), buffer(0) {
  }

InputPlugin::~InputPlugin() {
  }


FXival InputPlugin::fillBuffer(FXival count) {
  buffer.reserve(count);
  FXival nread = InputPlugin::readBlock(buffer.ptr(),count,false);
  if (nread>0)
    buffer.wrote(nread);
  return nread;
  }

FXival InputPlugin::readBlock(void*data,FXival count,FXbool wait){
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
    else if (nread==-2 ) { // block!
      /// wait if we have no data yet
      /// In case we receive data from socket and we don't know how long the stream will be.
      if (wait || (ncount==count)) {
        if (!ap_wait_read(input->getFifoHandle(),handle()))
          return -1;
        }
      else {
        return count-ncount;
        }
      }
    else {
      return -1;
      }
    }
  return count;
  }


FXival InputPlugin::preview(void*data,FXival count) {
  if (serial() || buffer.size()) {
    if (buffer.size()<count)
      fillBuffer(count-buffer.size());
    return buffer.copy(data,count);
    }
  else { // no need to buffer if we have non-serial streams
    FXlong readpos = position();
    FXival nblock  = InputPlugin::readBlock(data,count);
    position(readpos,FXIO::Begin);
    return nblock;
    }
  }

FXival InputPlugin::read(void * d,FXival count){
  if (__unlikely(buffer.size()>0)) {
    FXchar * data = (FXchar*)d;
    FXival nbuffer = buffer.read(data,count);
    if (nbuffer==count) return nbuffer;
    FXival nblock = InputPlugin::readBlock(data+nbuffer,count-nbuffer);
    if (nblock==-1) return -1;
    return nbuffer+nblock;
    }
  else {
    return InputPlugin::readBlock(d,count);
    }
  }

}

























