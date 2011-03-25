#include "ap_defs.h"
#include "ap_event.h"
#include "ap_pipe.h"
#include "ap_event_queue.h"
#include "ap_thread_queue.h"
#include "ap_format.h"
#include "ap_memory_buffer.h"
#include "ap_packet.h"
#include "ap_engine.h"
#include "ap_thread.h"
#include "ap_input_plugin.h"
#include "ap_input_thread.h"
#include "ap_decoder_plugin.h"
#include "ap_decoder_thread.h"

using namespace ap;

namespace ap {

InputPlugin::InputPlugin(AudioEngine *e) : engine(e), flags(0),stream_length(-1) {
  }

InputPlugin::~InputPlugin() {
  }

InputStatus InputPlugin::process(Packet*packet) {
  FXint nread = engine->input->read(packet->data(),packet->space());
  if (nread<0) {
    packet->unref();
    return InputError;
    }
  else if (nread==0) {
    packet->af=af;
    packet->wrote(nread);
    packet->stream_position  = -1;
    packet->stream_length    = 0;
    packet->flags = FLAG_EOS;
    engine->decoder->post(packet);
    return InputDone;
    }
  else {
    packet->af=af;
    packet->wrote(nread);
    packet->flags = 0;
    packet->stream_position = -1;
    packet->stream_length   = 0;
    engine->decoder->post(packet);
    return InputOk;
    }
  return InputError;
  }
}

#include "ap_config.h"

namespace ap {



extern InputPlugin * ap_wav_input(AudioEngine*);

#ifdef HAVE_FLAC_PLUGIN
extern InputPlugin * ap_flac_input(AudioEngine*);
#endif

#ifdef HAVE_OGG_PLUGIN
extern InputPlugin * ap_ogg_input(AudioEngine*);
#endif

#ifdef HAVE_MUSEPACK_PLUGIN
extern InputPlugin * ap_musepack_input(AudioEngine*);
#endif

#ifdef HAVE_MAD_PLUGIN
extern InputPlugin * ap_mad_input(AudioEngine*);
#endif

#ifdef HAVE_AAC_PLUGIN
extern InputPlugin * ap_aac_input(AudioEngine*);
#endif

#ifdef HAVE_CDDA_PLUGIN
extern InputPlugin * ap_cdda_input(AudioEngine*);
#endif

InputPlugin* InputPlugin::open(AudioEngine * engine,const FXString & uri) {
  FXString scheme = FXURL::scheme(uri);
  if (scheme.empty() || scheme=="file") {
    FXString extension=FXPath::extension(uri);

    if (comparecase(extension,"wav")==0) {
      return ap_wav_input(engine);
      }
  #ifdef HAVE_FLAC_PLUGIN
    if (comparecase(extension,"flac")==0) {
      return ap_flac_input(engine);
      }
  #endif
  #ifdef HAVE_OGG_PLUGIN
    else if (comparecase(extension,"ogg")==0 || comparecase(extension,"oga")==0) {
      return ap_ogg_input(engine);
      }
  #endif
  #ifdef HAVE_MUSEPACK_PLUGIN
    else if (comparecase(extension,"mpc")==0) {
      return ap_musepack_input(engine);
      }
  #endif
  #ifdef HAVE_MAD_PLUGIN
    else if (comparecase(extension,"mp3")==0) {
      return ap_mad_input(engine);
      }
  #endif
  #ifdef HAVE_AAC_PLUGIN
    else if (comparecase(extension,"mp4")==0 ||
             comparecase(extension,"m4a")==0 ||
             comparecase(extension,"m4p")==0 ||
             comparecase(extension,"m4b")==0 /*||
             comparecase(extension,"aac")==0*/) {
      return ap_aac_input(engine);
      }
  #endif
    }
#ifdef HAVE_CDDA_PLUGIN
  else if (scheme=="cdda"){
    return ap_cdda_input(engine);
    }
#endif
  return NULL;
  }
}
