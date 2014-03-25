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


#if defined(HAVE_VORBIS_PLUGIN)
#include <vorbis/codec.h>
#elif defined(HAVE_TREMOR_PLUGIN)
#include <tremor/ivorbiscodec.h>
#else
#error "No vorbis decoder library specified"
#endif

namespace ap {

class VorbisDecoder : public OggDecoder {
protected:
  FXbool is_vorbis_header();
protected:
  vorbis_info       info;
  vorbis_comment    comment;
  vorbis_dsp_state  dsp;
  vorbis_block      block;
  FXbool            has_info;
  FXbool            has_dsp;
protected:
  FXbool init_decoder();
  void   reset_decoder();
  FXbool find_stream_position();
public:
  VorbisDecoder(AudioEngine*);

  FXuchar codec() const { return Codec::Vorbis; }
  FXbool init(ConfigureEvent*);
  DecoderStatus process(Packet*);
  FXbool flush(FXlong);

  virtual ~VorbisDecoder();
  };


VorbisDecoder::VorbisDecoder(AudioEngine * e) : OggDecoder(e),has_info(false),has_dsp(false) {
  // Dummy comment structure. libvorbis will only check for a non-null vendor.
  vorbis_comment_init(&comment);
  comment.vendor = (FXchar*)"";
  }

VorbisDecoder::~VorbisDecoder(){
  reset_decoder();
  if (has_info) {
    vorbis_info_clear(&info);
    has_info=false;
    }
  }



FXbool VorbisDecoder::init(ConfigureEvent*event) {
  OggDecoder::init(event);

  af=event->af;

  reset_decoder();

  if (has_info) {
    vorbis_info_clear(&info);
    has_info=false;
    }

  /// Init
  vorbis_info_init(&info);
  has_info=true;

  return true;
  }


FXbool VorbisDecoder::flush(FXlong offset) {
  OggDecoder::flush(offset);
//  if (has_dsp)
//    vorbis_synthesis_restart(&dsp);
  return true;
  }


FXbool VorbisDecoder::is_vorbis_header() {
  return (op.bytes>6 && ((op.packet[0]==1) || (op.packet[0]==3) || (op.packet[0]==5)) && (compare((const FXchar*)&op.packet[1],"vorbis",6)==0));
  }

#ifdef HAVE_TREMOR_PLUGIN
static ogg_int32_t CLIP_TO_15(ogg_int32_t x) {
  int ret=x;
  ret-= ((x<=32767)-1)&(x-32767);
  ret-= ((x>=-32768)-1)&(x+32768);
  return(ret);
  }
#endif


void VorbisDecoder::reset_decoder() {
  if (has_dsp) {
    vorbis_block_clear(&block);
    vorbis_dsp_clear(&dsp);
    has_dsp=false;
    }
  }

FXbool VorbisDecoder::init_decoder() {
  const FXuchar * data_ptr = get_packet_offset();
  while(get_next_packet()) {
    if (is_vorbis_header()) {
      if (vorbis_synthesis_headerin(&info,&comment,&op)<0) {
        GM_DEBUG_PRINT("[vorbis] vorbis_synthesis_headerin failed\n");
        return false;
        }
      }
    else {

      if (vorbis_synthesis_init(&dsp,&info)<0)
        return false;

      if (vorbis_block_init(&dsp,&block)<0) {
        vorbis_dsp_clear(&dsp);
        return false;
        }

      has_dsp=true;
      push_back_packet();
      return true;
      }
    }
  set_packet_offset(data_ptr);
  return true;
  }



FXbool VorbisDecoder::find_stream_position() {
  const FXuchar * data_ptr = get_packet_offset();
  FXint cb,lb=-1,tb=0;
  while(get_next_packet()) {
    if (is_vorbis_header()){
      reset_decoder();
      goto reset;
      }
    cb=vorbis_packet_blocksize(&info,&op);
    if (lb!=-1) tb+=(lb+cb)>>2;
    lb=cb;
    if (op.granulepos!=-1) {
      stream_position=op.granulepos-tb;
      set_packet_offset(data_ptr);
      return true;
      }
    }
reset:
  set_packet_offset(data_ptr);
  return false;
  }




























DecoderStatus VorbisDecoder::process(Packet * packet) {
  FXASSERT(packet);

#ifdef HAVE_VORBIS_PLUGIN
  FXfloat ** pcm=NULL;
  FXfloat * buf32=NULL;
#else // HAVE_TREMOR_PLUGIN
  FXint ** pcm=NULL;
  FXshort * buf32=NULL;
#endif

  FXint p,navail=0;

  FXint ngiven,ntotalsamples,nsamples,sample,c,s;

  FXbool  eos=packet->flags&FLAG_EOS;
  FXuint   id=packet->stream;
  FXlong  len=packet->stream_length;


  OggDecoder::process(packet);

  /// Init Decoder
  if (!has_dsp) {
    if (!init_decoder())
      return DecoderError;
    if (!has_dsp)
      return DecoderOk;
    }

  /// Find Stream Position
  if (stream_position==-1 && !find_stream_position())
    return DecoderOk;


  if (out) {
    navail = out->availableFrames();
    }

  while(get_next_packet()) {

    if (__unlikely(is_vorbis_header())) {
      GM_DEBUG_PRINT("[vorbis] unexpected vorbis header found. Resetting decoder\n");
      push_back_packet();
      reset_decoder();
      return DecoderOk;
      }

    if (vorbis_synthesis(&block,&op)==0)
      vorbis_synthesis_blockin(&dsp,&block);

    while((ngiven=vorbis_synthesis_pcmout(&dsp,&pcm))>0) {
      if (len>0) FXASSERT(stream_position+ngiven<=len);

      if (__unlikely(stream_position<stream_decode_offset)) {
        FXlong offset = FXMIN(ngiven,stream_decode_offset - stream_position);
        GM_DEBUG_PRINT("[vorbis] stream decode offset %ld. Skipping %ld of %ld \n",stream_decode_offset,offset,stream_decode_offset-stream_position);
        ngiven-=offset;
        stream_position+=offset;
        sample=offset;
        vorbis_synthesis_read(&dsp,offset);
        if (ngiven==0) continue;
        }
      else {
        sample=0;
        }

      for (ntotalsamples=ngiven;ntotalsamples>0;) {

        /// Get new buffer
        if (out==NULL) {
          out = engine->decoder->get_output_packet();
          if (out==NULL) return DecoderInterrupted;
          out->stream_position=stream_position;
          out->stream_length=len;
          out->af=af;
          navail = out->availableFrames();
          }

#ifdef HAVE_VORBIS_PLUGIN
        buf32 = out->flt();
#else // HAVE_TREMOR_PLUGIN
        buf32 = out->s16();
#endif
        /// Copy Samples
        nsamples = FXMIN(ntotalsamples,navail);
        for (p=0,s=sample;s<(nsamples+sample);s++){
          for (c=0;c<info.channels;c++,p++) {
#ifdef HAVE_VORBIS_PLUGIN
            buf32[p]=pcm[c][s];
#else
            buf32[p]=CLIP_TO_15(pcm[c][s]>>9);
#endif
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

  if (eos) {
    if (out && out->numFrames())  {
      engine->output->post(out);
      out=NULL;
      }
    engine->output->post(new ControlEvent(End,id));
    }
  return DecoderOk;
  }


DecoderPlugin * ap_vorbis_decoder(AudioEngine * engine) {
  return new VorbisDecoder(engine);
  }


}
