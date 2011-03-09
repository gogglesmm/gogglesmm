#include "ap_defs.h"
#include "ap_event.h"
#include "ap_event_queue.h"

#include "ap_format.h"
#include "ap_memory_buffer.h"
#include "ap_engine.h"

#include "ap_input_plugin.h"
#include "ap_decoder_plugin.h"

DecoderPlugin::DecoderPlugin(AudioEngine *e) : engine(e) {
  }

#include "ap_config.h"
#include "plugins/ap_flac_plugin.h"
#include "plugins/ap_vorbis_plugin.h"
#include "plugins/ap_mad_plugin.h"
#include "plugins/ap_pcm_plugin.h"
#include "plugins/ap_aac_plugin.h"
#include "plugins/ap_avc_plugin.h"

DecoderPlugin* DecoderPlugin::open(AudioEngine * engine,FXuchar codec) {
  switch(codec) {
    case Codec::PCM     : return new PCMDecoder(engine); break;
#ifdef HAVE_VORBIS_PLUGIN
    case Codec::Vorbis  : return new VorbisDecoder(engine); break;
#endif
#ifdef HAVE_FLAC_PLUGIN
    case Codec::FLAC    : return new FlacDecoder(engine); break;
#endif
#ifdef HAVE_MAD_PLUGIN
    case Codec::MPEG    : return new MadDecoder(engine); break;
#endif
//#ifdef HAVE_AVCODEC_PLUGIN
//    case Codec::MPEG    : return new AVDecoder(engine); break;
//#endif
#ifdef HAVE_AAC_PLUGIN
    case Codec::AAC     : return new AacDecoder(engine); break;
#endif
    default             : break;
    }
  return NULL;
  }



