/*******************************************************************************
*                         Goggles Audio Player Library                         *
********************************************************************************
*           Copyright (C) 2010-2012 by Sander Jansen. All Rights Reserved      *
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
  FXbool flush();


  virtual ~OpusDecoderPlugin();
  };


OpusDecoderPlugin::OpusDecoderPlugin(AudioEngine * e) : OggDecoder(e) {
  }

OpusDecoderPlugin::~OpusDecoderPlugin(){
  }

#define MAX_FRAME_SIZE (960*6)

FXbool OpusDecoderPlugin::init(ConfigureEvent*event) {
  OggDecoder::init(event);
  af=event->af;
  int error;
  opus = opus_decoder_create(af.rate,af.channels,&error);
  if (error!=OPUS_OK)
    return false;

  allocElms(pcm,MAX_FRAME_SIZE*af.channels*2);
  stream_offset_start = event->stream_offset_start;
  return true;
  }


FXbool OpusDecoderPlugin::flush() {
  OggDecoder::flush(); 
  freeElms(pcm);
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
      GM_DEBUG_PRINT("[opus] stream offset start %hu. Skip %ld at %d of %d\n",stream_offset_start,offset,stream_position,nsamples);
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









/*
nt opus_decode_float	(	OpusDecoder * 	st,
const unsigned char * 	data,
opus_int32 	len,
float * 	pcm,
int 	frame_size,
int 	decode_fec
)
*/


#if  0
  FXASSERT(packet);

  FXfloat ** pcm=NULL;
  FXfloat * buf32=NULL;
  FXint p,navail=0;

  FXint ngiven,ntotalsamples,nsamples,sample,c,s;

  FXbool eos=packet->flags&FLAG_EOS;
  FXuint id=packet->stream;
  FXint  len=packet->stream_length;

  buffer.append(packet->data(),packet->size());

  packet->unref();

  if (out) {
    navail = out->availableFrames();
    }

  FXuchar * data_ptr = NULL;
  if (stream_position==-1) {
    GM_DEBUG_PRINT("stream position unknown\n");
    data_ptr = buffer.data();
    nsamples = 0;
    }

  while(get_next_packet()) {

    if (__unlikely(is_vorbis_header())) {

      if (has_dsp) {
        vorbis_block_clear(&block);
        vorbis_dsp_clear(&dsp);
        has_dsp=false;
        }

      if (vorbis_synthesis_headerin(&info,&comment,&op)<0) {
        GM_DEBUG_PRINT("vorbis_synthesis_headerin failed\n");
        return DecoderError;
        }

      if (data_ptr)
        data_ptr = buffer.data();
      }
    else {

      if (__unlikely(!has_dsp)) {

        if (vorbis_synthesis_init(&dsp,&info)<0)
          return DecoderError;

        if (vorbis_block_init(&dsp,&block)<0) {
          vorbis_dsp_clear(&dsp);
          return DecoderError;
          }

        has_dsp=true;
        }
      if (stream_position==-1) {
//        fxmessage("packet: %ld %ld\n",op.packetno,op.granulepos);
        if (vorbis_synthesis(&block,&op)==0) {
          vorbis_synthesis_blockin(&dsp,&block);
          while((ngiven=vorbis_synthesis_pcmout(&dsp,NULL))>0) {
            nsamples+=ngiven;
            vorbis_synthesis_read(&dsp,ngiven);
            }
          }
        if (op.granulepos>=0) {
          GM_DEBUG_PRINT("found stream position: %ld\n",op.granulepos-nsamples);
          stream_position=op.granulepos-nsamples;
 //       else {
//          stream_position=0;
//          }

        buffer.setReadPosition(data_ptr);
        data_ptr=NULL;
        vorbis_synthesis_restart(&dsp);

          }
        continue;

        }

      if (vorbis_synthesis(&block,&op)==0)
        vorbis_synthesis_blockin(&dsp,&block);

      while((ngiven=vorbis_synthesis_pcmout(&dsp,&pcm))>0) {
//        fxmessage("got %d samples\n",ngiven);
        for (sample=0,ntotalsamples=ngiven;ntotalsamples>0;) {

          /// Get new buffer
          if (out==NULL) {
            out = engine->decoder->get_output_packet();
            if (out==NULL) return DecoderInterrupted;
            out->stream_position=stream_position;
            out->stream_length=len;
            out->af=af;
            navail = out->availableFrames();
            }

          buf32 = out->flt();

          /// Copy Samples
          nsamples = FXMIN(ntotalsamples,navail);
          for (p=0,s=sample;s<(nsamples+sample);s++){
            for (c=0;c<info.channels;c++,p++) {
              FXASSERT(s<ngiven);
              buf32[p]=pcm[c][s];
              }
            }

          /// Update sample counts
          out->wroteFrames(nsamples);

          sample+=nsamples;
          navail-=nsamples;
          ntotalsamples-=nsamples;
          stream_position+=nsamples;

          /// Send out packet if full
          ///FIXME handle EOS.
          if (navail==0) {
            engine->output->post(out);
            out=NULL;
            }
          }
        vorbis_synthesis_read(&dsp,ngiven);
        }
      }
    }

  /// Reset read ptr if we're still looking for the stream position..
  if (data_ptr) {
    buffer.setReadPosition(data_ptr);
    if (has_dsp) vorbis_synthesis_restart(&dsp);
    }

  if (eos) {
    if (out && out->numFrames())  {
      engine->output->post(out);
      out=NULL;
      }
    engine->output->post(new ControlEvent(End,id));
    }
#endif


DecoderPlugin * ap_opus_decoder(AudioEngine * engine) {
  return new OpusDecoderPlugin(engine);
  }


}
