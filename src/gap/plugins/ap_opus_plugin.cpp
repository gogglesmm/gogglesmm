/*******************************************************************************
*                         Goggles Audio Player Library                         *
********************************************************************************
*           Copyright (C) 2010-2014 by Sander Jansen. All Rights Reserved      *
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
#include "ap_config.h"
#include "ap_pipe.h"
#include "ap_format.h"
#include "ap_device.h"
#include "ap_buffer.h"
#include "ap_event.h"
#include "ap_reactor.h"
#include "ap_event_private.h"
#include "ap_packet.h"
#include "ap_event_queue.h"
#include "ap_thread_queue.h"
#include "ap_engine.h"
#include "ap_input_plugin.h"
#include "ap_decoder_plugin.h"
#include "ap_thread.h"
#include "ap_input_thread.h"
#include "ap_decoder_thread.h"
#include "ap_output_thread.h"
#include "ap_ogg_decoder.h"

#include <opus/opus.h>


namespace ap {


class OpusDecoderPlugin : public OggDecoder{
protected:
  OpusDecoder*  opus;
  FXfloat    *  pcm;
  FXushort      stream_offset_start;
protected:
  FXbool find_stream_position();
  FXlong find_stream_length();
protected:
public:
  OpusDecoderPlugin(AudioEngine*);

  FXuchar codec() const { return Codec::Opus; }
  FXbool init(ConfigureEvent*);
  DecoderStatus process(Packet*);
  FXbool flush(FXlong);


  virtual ~OpusDecoderPlugin();
  };


OpusDecoderPlugin::OpusDecoderPlugin(AudioEngine * e) : OggDecoder(e),opus(NULL),pcm(NULL) {
  }

OpusDecoderPlugin::~OpusDecoderPlugin(){
  freeElms(pcm);
  if (opus) {
    opus_decoder_destroy(opus);
    opus=NULL;
    }
  }

#define MAX_FRAME_SIZE (960*6)

FXbool OpusDecoderPlugin::init(ConfigureEvent*event) {
  OggDecoder::init(event);
  af=event->af;
  int error;
  if (opus) {
    opus_decoder_destroy(opus);
    }


  opus = opus_decoder_create(af.rate,af.channels,&error);
  if (error!=OPUS_OK)
    return false;

  if (pcm)
    resizeElms(pcm,MAX_FRAME_SIZE*af.channels*2);
  else
    allocElms(pcm,MAX_FRAME_SIZE*af.channels*2);

  stream_offset_start = event->stream_offset_start;
  return true;
  }


FXbool OpusDecoderPlugin::flush(FXlong offset) {
  OggDecoder::flush(offset); 
  //if (opus) opus_decoder_ctl(opus,OPUS_RESET_STATE);
  return true;
  }


FXbool OpusDecoderPlugin::find_stream_position() {
  const FXuchar * data_ptr = get_packet_offset();
  FXlong    nsamples = 0;
  GM_DEBUG_PRINT("[opus] find stream position\n");
  while(get_next_packet()) {
    nsamples += opus_packet_get_nb_samples((unsigned char*)op.packet,op.bytes,48000);
    if (op.granulepos>=0) {
      GM_DEBUG_PRINT("[opus] found stream position: %ld\n",op.granulepos-nsamples);
      stream_position=op.granulepos-nsamples;
      set_packet_offset(data_ptr);
      return true;
      }
    }
  set_packet_offset(data_ptr);
  return false;
  }

FXlong OpusDecoderPlugin::find_stream_length() {
  const FXuchar * data_ptr = get_packet_offset();
  FXlong    nlast = 0;
  GM_DEBUG_PRINT("[opus] find stream length\n");
  while(get_next_packet()) {
    nlast = op.granulepos;
    }
  set_packet_offset(data_ptr);
  return nlast - stream_offset_start;
  }




DecoderStatus OpusDecoderPlugin::process(Packet * packet) {
  FXbool eos           = packet->flags&FLAG_EOS;
  FXuint id            = packet->stream;
  FXlong stream_length = packet->stream_length;
  FXlong stream_end    = stream_length;

  OggDecoder::process(packet);

  if (stream_position==-1 && !find_stream_position())
    return DecoderOk;

  if (eos && stream_end==-1) {
    stream_end = find_stream_length();
    FXASSERT(stream_position-stream_offset_start<stream_end);
    }

  while(get_next_packet()) {
    FXint nsamples = opus_decode_float(opus,(unsigned char*)op.packet,op.bytes,pcm,MAX_FRAME_SIZE,0);

    const FXuchar * pcmi = (const FXuchar*)pcm;

    // Adjust for beginning of stream
    if (stream_position<stream_offset_start) {
      FXlong offset = FXMIN(nsamples,stream_offset_start - stream_position);
      GM_DEBUG_PRINT("[opus] stream offset start %hu. Skip %ld at %ld of %d\n",stream_offset_start,offset,stream_position,nsamples);
      nsamples-=offset;
      pcmi+=(offset*af.framesize());
      stream_position+=offset;
      }

    //GM_DEBUG_PRINT("[opus] decoded %d frames\n",nsamples);
    if (eos) {
      FXlong total = stream_position-stream_offset_start+nsamples;
      if (total>stream_end) {
        GM_DEBUG_PRINT("adjusting end trimming by %ld\n",(total-stream_end));
        nsamples -= (total-stream_end);
        }
      }

    while(nsamples>0) {
      /// Get new buffer
      if (out==NULL) {
        out = engine->decoder->get_output_packet();
        if (out==NULL) return DecoderInterrupted;
        out->stream_position=stream_position - stream_offset_start;
        out->stream_length=stream_length;
        out->af=af;
        }

      FXint nw = FXMIN(out->availableFrames(),nsamples);
      if (nw>0){
        //fxmessage("add %d / %d / %d / %d\n",nw,nsamples,out->availableFrames(),af.framesize());
        out->appendFrames(pcmi,nw);
        pcmi+=(nw*af.framesize());
        nsamples-=nw;
        stream_position+=nw;
        }

      if (out->availableFrames()==0) {
        //fxmessage("posting\n");
        engine->output->post(out);
        out=NULL;
        }
      }
    }

  if (eos) {
    if (out && out->numFrames())  {
      engine->output->post(out);
      out=NULL;
      }
    engine->output->post(new ControlEvent(End,id));
    }
  return DecoderOk;
  }


DecoderPlugin * ap_opus_decoder(AudioEngine * engine) {
  return new OpusDecoderPlugin(engine);
  }


}
