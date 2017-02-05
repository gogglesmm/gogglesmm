/*******************************************************************************
*                         Goggles Audio Player Library                         *
********************************************************************************
*           Copyright (C) 2010-2015 by Sander Jansen. All Rights Reserved      *
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
#include "ap_event_private.h"
#include "ap_packet.h"
#include "ap_decoder_plugin.h"

extern "C" {
#include <stdint.h>
#include "dca.h"
}

namespace ap {

class DCADecoder : public DecoderPlugin {
protected:
  MemoryBuffer  buffer;
  dca_state_t*  state  = nullptr;
  Packet*       out    = nullptr;
  FXlong  stream_position=0;
public:
  DCADecoder(DecoderContext*);
  FXuchar codec() const override { return Codec::DCA; }
  FXbool init(ConfigureEvent*) override;
  FXbool process(Packet*) override;
  virtual ~DCADecoder();
  };

DCADecoder::DCADecoder(DecoderContext * e) : DecoderPlugin(e) {
  state = dca_init(0);
  }

DCADecoder::~DCADecoder() {
  }

FXbool DCADecoder::init(ConfigureEvent*event) {
  DecoderPlugin::init(event);
  event->af.setChannels(2);
  af=event->af;
  return true;
  }


FXbool DCADecoder::process(Packet*in) {
  FXbool eos    = (in->flags&FLAG_EOS);
  FXint  stream = in->stream;
  FXlong stream_length=in->stream_length;
  buffer.append(in->data(),in->size());
  in->unref();

  while (buffer.size()>=14) {
    int flags;
    int samplerate;
    int bitrate;
    int framelength;
    int length = dca_syncinfo(state,buffer.data(),&flags,&samplerate,&bitrate,&framelength);
    if (length<=0) {
      fxmessage("length returned %ld\n",length);
      return false;
      }
    else if (buffer.size()<length) return true;

    int dflags=DCA_STEREO;
    level_t level = 1.0f;
    dca_frame (state,buffer.data(),&dflags,&level,0.0f);

    int nblocks = dca_blocks_num (state);
    for (int i=0;i<nblocks;i++) {
      dca_block(state);
      sample_t * samples = dca_samples(state);

      /// Get new buffer
      if (out==nullptr) {
        out = engine->decoder->get_output_packet();
        if (out==nullptr) return true;
        out->stream_position=stream_position;
        out->stream_length=stream_length;
        out->af=af;
        }

      FXfloat * data = out->flt();
      for (FXint i=0,d=0;i<256;i++) {
        data[d++] = samples[i];
        data[d++] = samples[256+i];
        }
      out->wroteFrames(256);
      if (out->availableFrames()<256) {
        context->post_output_packet(out);
        }
      stream_position+=256;
      }
    buffer.readBytes(length);
    }
  if (eos) {
    context->post_output_packet(out,true);
    }
  return true;
  }

DecoderPlugin * ap_dca_decoder(DecoderContext * ctx) {
  return new DCADecoder(ctx);
  }

}
