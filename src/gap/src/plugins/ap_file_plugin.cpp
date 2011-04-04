#include "ap_defs.h"
#include "ap_utils.h"
#include "ap_event.h"
#include "ap_pipe.h"
#include "ap_format.h"
#include "ap_memory_buffer.h"
#include "ap_input_plugin.h"
#include "ap_file_plugin.h"

using namespace ap;

namespace ap {


FileInput::FileInput(FXInputHandle f) : InputPlugin(f) {
  }

FileInput::~FileInput() {
  }

FXbool FileInput::open(const FXString & uri) {
  if (file.open(uri,FXIO::Reading)){
    filename=uri;
    return true;
    }
  return false;
  }

FXival FileInput::read_raw(void*data,FXival ncount) {
  return file.readBlock(data,ncount);
  }

FXival FileInput::position(FXlong offset,FXuint from) {
  return file.position(offset,from);
  }

FXival FileInput::position() const {
  return file.position();
  }

FXlong FileInput::size() {
  return file.size();
  }

FXbool FileInput::eof()  {
  return file.eof();
  }

FXbool FileInput::serial() const {
  return file.isSerial();
  }

FXuint FileInput::plugin() const {
  FXString extension=FXPath::extension(filename);
  return ap_format_from_extension(extension);
  }


}
