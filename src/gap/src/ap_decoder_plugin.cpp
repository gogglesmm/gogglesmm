#include "ap_defs.h"
#include "ap_event.h"
#include "ap_event_queue.h"

#include "ap_format.h"
#include "ap_memory_buffer.h"
#include "ap_engine.h"

#include "ap_reader_plugin.h"
#include "ap_decoder_plugin.h"

namespace ap {

DecoderPlugin::DecoderPlugin(AudioEngine *e) : engine(e) {
  }

}

#include "ap_config.h"

namespace ap {

extern DecoderPlugin * ap_flac_decoder(AudioEngine*);
extern DecoderPlugin * ap_pcm_decoder(AudioEngine*);
extern DecoderPlugin * ap_vorbis_decoder(AudioEngine*);
extern DecoderPlugin * ap_mad_decoder(AudioEngine*);
extern DecoderPlugin * ap_aac_decoder(AudioEngine*);


DecoderPlugin* DecoderPlugin::open(AudioEngine * engine,FXuchar codec) {
  switch(codec) {
    case Codec::PCM     : return ap_pcm_decoder(engine); break;
#ifdef HAVE_VORBIS_PLUGIN
    case Codec::Vorbis  : return ap_vorbis_decoder(engine); break;
#endif
#ifdef HAVE_FLAC_PLUGIN
    case Codec::FLAC    : return ap_flac_decoder(engine); break;
#endif
#ifdef HAVE_MAD_PLUGIN
    case Codec::MPEG    : return ap_mad_decoder(engine); break;
#endif
//#ifdef HAVE_AVCODEC_PLUGIN
//    case Codec::MPEG    : return new AVDecoder(engine); break;
//#endif
#ifdef HAVE_AAC_PLUGIN
    case Codec::AAC     : return ap_aac_decoder(engine); break;
#endif
    default             : break;
    }
  return NULL;
  }

}

