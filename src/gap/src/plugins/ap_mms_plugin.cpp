#include "ap_defs.h"
#include "ap_utils.h"
#include "ap_event.h"
#include "ap_pipe.h"
#include "ap_format.h"
#include "ap_memory_buffer.h"
#include "ap_input_plugin.h"
#include "ap_mms_plugin.h"

#include <libmms/mms.h>

using namespace ap;

namespace ap {


MMSInput::MMSInput(InputThread* i) : InputPlugin(i),mms(NULL) {
  }

MMSInput::~MMSInput() {
  }

FXbool MMSInput::open(const FXString & uri) {
  mms=mms_connect(NULL,NULL,uri.text(),128*1024);
  if (!mms) {
    fxmessage("failed to connect\n");
    return false;
    }
  fxmessage("mms connected\n");
  return true;
  }

FXival MMSInput::read_raw(void*data,FXival ncount) {
  FXint result = mms_read(NULL,(mms_t*)mms,(FXchar*)data,ncount);
  if (result<0) return -1;
  return result;
  }

FXlong MMSInput::position(FXlong /*offset*/,FXuint /*from*/) {
  return -1;
  }

FXlong MMSInput::position() const {
  return -1;
  }

FXlong MMSInput::size() {
  return -1;
  }

FXbool MMSInput::eof()  {
  return -1;
  }

FXbool MMSInput::serial() const {
  return true;
  }

FXuint MMSInput::plugin() const {
  return Format::Unknown;
  }


}
