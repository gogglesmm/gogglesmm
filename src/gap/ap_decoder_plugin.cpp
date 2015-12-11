/*******************************************************************************
*                         Goggles Audio Player Library                         *
********************************************************************************
*           Copyright (C) 2010-2016 by Sander Jansen. All Rights Reserved      *
*                               ---                                            *
* This program is free software: you can redistribute it and/or modify         *
* it under the terms of the GNU General Public License as published by         *
* the Free Software Foundation, either version 3 of the License, or            *
* (at your option) any later version.                                          *
*                                                                              *
* This program is distributed in the hope that it will be useful,              *
* but WITHOUT ANY WARRANTY; without even the implied warranty of               *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                *
* GNU General Public License for more details.                                 *
*                                                                              *
* You should have received a copy of the GNU General Public License            *
* along with this program.  If not, see http://www.gnu.org/licenses.           *
********************************************************************************/
#include "ap_defs.h"
#include "ap_event.h"
#include "ap_event_queue.h"

#include "ap_format.h"
#include "ap_buffer.h"
#include "ap_engine.h"

#include "ap_reader_plugin.h"
#include "ap_decoder_plugin.h"

namespace ap {

DecoderPlugin::DecoderPlugin(AudioEngine *e) : engine(e),stream_decode_offset(0) {
  }

FXbool DecoderPlugin::init(ConfigureEvent*){
  stream_decode_offset = 0;
  return true;
  }


FXbool DecoderPlugin::flush(FXlong offset) {
  stream_decode_offset = offset;
  return true;
  }


}

#include "ap_config.h"

namespace ap {

extern DecoderPlugin * ap_flac_decoder(AudioEngine*);
extern DecoderPlugin * ap_pcm_decoder(AudioEngine*);
extern DecoderPlugin * ap_vorbis_decoder(AudioEngine*);
extern DecoderPlugin * ap_mad_decoder(AudioEngine*);
extern DecoderPlugin * ap_aac_decoder(AudioEngine*);
extern DecoderPlugin * ap_opus_decoder(AudioEngine*);

DecoderPlugin* DecoderPlugin::open(AudioEngine * engine,FXuchar codec) {
  switch(codec) {
    case Codec::PCM     : return ap_pcm_decoder(engine); break;
#if defined(HAVE_VORBIS_PLUGIN) || defined(HAVE_TREMOR_PLUGIN)
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

#ifdef HAVE_OPUS_PLUGIN
    case Codec::Opus    : return ap_opus_decoder(engine); break;
#endif

    default             : break;
    }
  return NULL;
  }

}

