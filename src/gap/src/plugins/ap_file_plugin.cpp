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
  if (comparecase(extension,"wav")==0)
    return Format::WAV;
  else if (comparecase(extension,"flac")==0)
    return Format::FLAC;
  else if (comparecase(extension,"ogg")==0 || comparecase(extension,"oga")==0)
    return Format::OGG;
  else if (comparecase(extension,"mp3")==0)
    return Format::MP3;
  else if (comparecase(extension,"mpc")==0)
    return Format::Musepack;
  else if (comparecase(extension,"mp4")==0 ||
           comparecase(extension,"m4a")==0 ||
           comparecase(extension,"m4p")==0 ||
           comparecase(extension,"m4b")==0 )
    return Format::AAC;
  else
    return Format::Unknown;
  }


}
