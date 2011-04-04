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
#include "ap_reader_plugin.h"
#include "ap_input_thread.h"
#include "ap_decoder_plugin.h"
#include "ap_decoder_thread.h"

using namespace ap;

namespace ap {

ReaderPlugin::ReaderPlugin(AudioEngine *e) : engine(e), flags(0),stream_length(-1) {
  }

ReaderPlugin::~ReaderPlugin() {
  }

ReadStatus ReaderPlugin::process(Packet*packet) {
  FXint nread = engine->input->read(packet->data(),packet->space());
  if (nread<0) {
    packet->unref();
    return ReadError;
    }
  else if (nread==0) {
    packet->af=af;
    packet->wrote(nread);
    packet->stream_position  = -1;
    packet->stream_length    = 0;
    packet->flags = FLAG_EOS;
    engine->decoder->post(packet);
    return ReadDone;
    }
  else {
    packet->af=af;
    packet->wrote(nread);
    packet->flags = 0;
    packet->stream_position = -1;
    packet->stream_length   = 0;
    engine->decoder->post(packet);
    return ReadOk;
    }
  return ReadError;
  }
}

#include "ap_config.h"

namespace ap {


extern ReaderPlugin * ap_m3u_reader(AudioEngine*);
extern ReaderPlugin * ap_pls_reader(AudioEngine*);
extern ReaderPlugin * ap_xspf_reader(AudioEngine*);


extern ReaderPlugin * ap_wav_reader(AudioEngine*);

#ifdef HAVE_FLAC_PLUGIN
extern ReaderPlugin * ap_flac_reader(AudioEngine*);
#endif

#ifdef HAVE_OGG_PLUGIN
extern ReaderPlugin * ap_ogg_reader(AudioEngine*);
#endif

#ifdef HAVE_MUSEPACK_PLUGIN
extern ReaderPlugin * ap_musepack_reader(AudioEngine*);
#endif

#ifdef HAVE_MAD_PLUGIN
extern ReaderPlugin * ap_mad_reader(AudioEngine*);
#endif

#ifdef HAVE_AAC_PLUGIN
extern ReaderPlugin * ap_aac_reader(AudioEngine*);
#endif

#ifdef HAVE_CDDA_PLUGIN
extern ReaderPlugin * ap_cdda_reader(AudioEngine*);
#endif

ReaderPlugin* ReaderPlugin::open(AudioEngine * engine,FXuint type) {
  switch(type){
    case Format::WAV      : return ap_wav_reader(engine); break;
#ifdef HAVE_OGG_PLUGIN
    case Format::OGG      : return ap_ogg_reader(engine); break;
#endif
#ifdef HAVE_FLAC_PLUGIN
    case Format::FLAC     : return ap_flac_reader(engine); break;
#endif
#ifdef HAVE_MAD_PLUGIN
    case Format::MP3      : return ap_mad_reader(engine); break;
#endif
#ifdef HAVE_AAC_PLUGIN
    case Format::AAC      : return ap_aac_reader(engine); break;
#endif
#ifdef HAVE_MUSEPACK_PLUGIN
    case Format::Musepack : return ap_musepack_reader(engine); break;
#endif
#ifdef HAVE_CDDA_PLUGIN
    case Format::CDDA     : return ap_cdda_reader(engine); break;
#endif
    case Format::M3U      : return ap_m3u_reader(engine); break;
    case Format::PLS      : return ap_pls_reader(engine); break;
    case Format::XSPF     : return ap_xspf_reader(engine); break;
    default               : return NULL; break;
    }
  return NULL;
  }
}




















