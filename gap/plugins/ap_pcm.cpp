/*******************************************************************************
*                         Goggles Audio Player Library                         *
********************************************************************************
*           Copyright (C) 2010-2021 by Sander Jansen. All Rights Reserved      *
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
#include "ap_packet.h"
#include "ap_decoder_plugin.h"

namespace ap {

class PCMDecoder : public DecoderPlugin {
public:
  PCMDecoder(DecoderContext*);
  FXuchar codec() const override { return Codec::PCM; }
  FXbool init(ConfigureEvent*) override;
  FXbool process(Packet*) override;
  virtual ~PCMDecoder();
  };

PCMDecoder::PCMDecoder(DecoderContext * ctx) : DecoderPlugin(ctx) {
  }

PCMDecoder::~PCMDecoder() {
  }

FXbool PCMDecoder::init(ConfigureEvent*event) {
  DecoderPlugin::init(event);
  return true;
  }


FXbool PCMDecoder::process(Packet*in) {
  const FXbool eos = (in->flags&FLAG_EOS);
  context->post_output_packet(in,eos);
  return true;
  }

DecoderPlugin * ap_pcm_decoder(DecoderContext * context) {
  return new PCMDecoder(context);
  }

}
