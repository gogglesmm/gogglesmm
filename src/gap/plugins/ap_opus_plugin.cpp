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

#include <ogg/ogg.h>
#include <opus/opus.h>


namespace ap {

class OpusDecoderPlugin : public DecoderPlugin{
protected:
  MemoryBuffer  buffer;
  OpusDecoder*  opus;
  FXfloat*      pcm;
protected:
  FXbool get_next_packet();
  FXbool is_vorbis_header();
protected:
  AudioEngine *     engine;
  ogg_packet        op;
  Packet *          out;
  FXint             stream_position;
public:
  OpusDecoderPlugin(AudioEngine*);

  FXuchar codec() const { return Codec::Opus; }
  FXbool init(ConfigureEvent*);
  DecoderStatus process(Packet*);
  FXbool flush();

  virtual ~OpusDecoderPlugin();
  };





OpusDecoderPlugin::OpusDecoderPlugin(AudioEngine * e) : DecoderPlugin(e),buffer(32768),engine(e),out(NULL) {
  }

OpusDecoderPlugin::~OpusDecoderPlugin(){
  }

#define MAX_FRAME_SIZE (960*6)

FXbool OpusDecoderPlugin::init(ConfigureEvent*event) {
  af=event->af;
  buffer.clear();

  fxmessage("init");

  int error;
  opus = opus_decoder_create(af.rate,af.channels,&error);
  if (error!=OPUS_OK)
    return false;

  fxmessage("success");
  allocElms(pcm,MAX_FRAME_SIZE*af.channels*2);

  stream_position=-1;
  return true;
  }


FXbool OpusDecoderPlugin::flush() {
  if (out) {
    out->unref();
    out=NULL;
    }
  buffer.clear();
  stream_position=-1;
  return true;
  }


FXbool OpusDecoderPlugin::get_next_packet() {
  if (buffer.size() && buffer.size()>=(FXival)sizeof(ogg_packet)) {
    buffer.read((FXuchar*)&op,sizeof(ogg_packet));
    if (buffer.size()<op.bytes) {
      buffer.readBytes(-sizeof(ogg_packet));
      return false;
      }

    op.packet=(FXuchar*)buffer.data();
    buffer.readBytes(op.bytes);
    return true;
    }
  return false;
  }


FXbool OpusDecoderPlugin::is_vorbis_header() {
  return (op.bytes>6 && ((op.packet[0]==1) || (op.packet[0]==3) || (op.packet[0]==5)) && (compare((const FXchar*)&op.packet[1],"vorbis",6)==0));
  }




DecoderStatus OpusDecoderPlugin::process(Packet * packet) {

  buffer.append(packet->data(),packet->size());
  packet->unref();

  while(get_next_packet()) {
    fxmessage("decode %d\n",op.bytes);

    //memset(pcm,0,MAX_FRAME_SIZE*af.channels);
    FXint nsamples = opus_decode_float(opus,(unsigned char*)op.packet,op.bytes,pcm,MAX_FRAME_SIZE,0);
    fxmessage("got %d nsamples\n",nsamples);

     const FXuchar * pcmi = (const FXuchar*)pcm;

    while(nsamples>0) {

      /// Get new buffer
      if (out==NULL) {
        out = engine->decoder->get_output_packet();
        if (out==NULL) return DecoderInterrupted;
        out->stream_position=0;
        out->stream_length=0;
        out->af=af;
        }


 
      FXint nw = FXMIN(out->availableFrames(),nsamples);
      if (nw>0){
        fxmessage("add %d / %d / %d / %d\n",nw,nsamples,out->availableFrames(),af.framesize());
        out->appendFrames(pcmi,nw);
        pcmi+=(nw*af.framesize());
        nsamples-=nw;
        }

      if (out->availableFrames()==0) {
        fxmessage("posting\n");
        engine->output->post(out);
        out=NULL;
        }
      }
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
  return DecoderOk;
  }


DecoderPlugin * ap_opus_decoder(AudioEngine * engine) {
  return new OpusDecoderPlugin(engine);
  }


}
