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
#include "a52dec/a52.h"
}

namespace ap {

class A52Decoder : public DecoderPlugin {
protected:
  MemoryBuffer  buffer;
  a52_state_t*  state  = nullptr;
  Packet*       out    = nullptr;
  FXlong  stream_position=0;
public:
  A52Decoder(DecoderContext*);
  FXuchar codec() const override { return Codec::A52; }
  FXbool init(ConfigureEvent*) override;
  FXbool process(Packet*) override;
  virtual ~A52Decoder();
  };

A52Decoder::A52Decoder(DecoderContext * e) : DecoderPlugin(e) {
  state = a52_init(0);
  }

A52Decoder::~A52Decoder() {
  }

FXbool A52Decoder::init(ConfigureEvent*event) {
  DecoderPlugin::init(event);
  event->af.setChannels(2);
  af=event->af;
  return true;
  }


FXbool A52Decoder::process(Packet*in) {
  FXbool eos    = (in->flags&FLAG_EOS);
  buffer.append(in->data(),in->size());
  in->unref();

  while (buffer.size()>=7) {
    int flags;
    int samplerate;
    int bitrate;
    int length = a52_syncinfo(buffer.data(),&flags,&samplerate,&bitrate);
    if (length<=0) {
      GM_DEBUG_PRINT("[a52] length returned %d\n",length);
      return false;
      }
    else if (buffer.size()<length) return true;

    int dflags=A52_STEREO;
    sample_t level = 1.0f;
    a52_frame (state,buffer.data(),&dflags,&level,0.0f);

    for (int i=0;i<6;i++) {
      a52_block(state);
      sample_t * samples = a52_samples(state);

      /// Get new buffer
      if (out==nullptr) {
        out = context->get_output_packet();
        if (out==nullptr) return true;
        out->stream_position=stream_position;
        out->stream_length=0;
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

DecoderPlugin * ap_a52_decoder(DecoderContext * ctx) {
  return new A52Decoder(ctx);
  }

}
