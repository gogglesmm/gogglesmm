/*******************************************************************************
*                         Goggles Audio Player Library                         *
********************************************************************************
*           Copyright (C) 2010-2018 by Sander Jansen. All Rights Reserved      *
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
#include "ap_vorbis.h"
#include "ap_ogg_decoder.h"

#if defined(HAVE_VORBIS)
#include <vorbis/codec.h>
#elif defined(HAVE_TREMOR)
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
  void init_info();
  void reset_decoder();
public:
  VorbisDecoder(DecoderContext*);

  FXuchar codec() const override { return Codec::Vorbis; }

  FXbool init(ConfigureEvent*) override;

  FXbool process(Packet*) override;

  FXbool flush(FXlong) override;

  virtual ~VorbisDecoder();
  };


VorbisDecoder::VorbisDecoder(DecoderContext * e) : OggDecoder(e),has_info(false),has_dsp(false) {
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


extern const FXuint vorbis_channel_map[];


FXbool VorbisDecoder::init(ConfigureEvent*event) {
  OggDecoder::init(event);

#ifdef HAVE_VORBIS
  event->af.format = AP_FORMAT_FLOAT;
#else // HAVE_TREMOR
  event->af.format = AP_FORMAT_S16;
#endif
  event->af.channelmap = vorbis_channel_map[event->af.channels-1];

  stream_offset_start = event->stream_offset_start;

  af=event->af;

  reset_decoder();

  if (event->dc) {

    init_info();

    VorbisConfig * vorbis_config = dynamic_cast<VorbisConfig*>(event->dc);
    FXASSERT(vorbis_config);

    ogg_packet op;

    op.b_o_s      = 1;
    op.e_o_s      = 0;
    op.granulepos = -1;
    op.packet     = vorbis_config->info;
    op.bytes      = vorbis_config->info_bytes;

    if (vorbis_synthesis_headerin(&info,&comment,&op)<0) {
      GM_DEBUG_PRINT("[vorbis] info header failed\n");
      return false;
      }

    op.b_o_s  = 0;
    op.packet = vorbis_config->setup;
    op.bytes  = vorbis_config->setup_bytes;

    if (vorbis_synthesis_headerin(&info,&comment,&op)<0) {
      GM_DEBUG_PRINT("[vorbis] setup header failed\n");
      return false;
      }

    if (vorbis_synthesis_init(&dsp,&info)<0)
      return false;

    if (vorbis_block_init(&dsp,&block)<0) {
      vorbis_dsp_clear(&dsp);
      return false;
      }

    GM_DEBUG_PRINT("[vorbis] setup complete\n");
    stream_position = -1;
    has_dsp=true;
    return true;
    }
  return false;
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

#ifdef HAVE_TREMOR
static ogg_int32_t CLIP_TO_15(ogg_int32_t x) {
  int ret=x;
  ret-= ((x<=32767)-1)&(x-32767);
  ret-= ((x>=-32768)-1)&(x+32768);
  return(ret);
  }
#endif

void VorbisDecoder::init_info() {
  if (has_info) {
    vorbis_info_clear(&info);
    }
  vorbis_info_init(&info);
  has_info=true;
  }

void VorbisDecoder::reset_decoder() {
  if (has_dsp) {
    vorbis_block_clear(&block);
    vorbis_dsp_clear(&dsp);
    has_dsp=false;
    }
  }


FXbool VorbisDecoder::process(Packet * packet) {
  OggDecoder::process(packet);

  FXASSERT(packet);

#if defined(HAVE_VORBIS)
  FXfloat ** pcm=nullptr;
  FXfloat * buf32=nullptr;
#elif defined(HAVE_TREMOR)
  FXint ** pcm=nullptr;
  FXshort * buf32=nullptr;
#else
#error "No vorbis decoder library specified"
#endif

  FXint p,navail=0;

  FXint ngiven,ntotalsamples,nsamples,sample,c,s;

  FXbool  eos=packet->flags&FLAG_EOS;
  const FXlong stream_length=packet->stream_length;

  if (out) {
    navail = out->availableFrames();
    }

  while(get_next_packet(packet)) {

    if (__unlikely(is_vorbis_header())) {
      GM_DEBUG_PRINT("[vorbis] unexpected vorbis header found. Resetting decoder\n");
      return false;
      }

    if (vorbis_synthesis(&block,&op)==0)
      vorbis_synthesis_blockin(&dsp,&block);

    while((ngiven=vorbis_synthesis_pcmout(&dsp,&pcm))>0) {

      const FXlong stream_begin = FXMAX(stream_offset_start,stream_decode_offset);

      // don't go past the stream_length
      if (stream_length>0 && stream_position+ngiven>stream_length) {
        GM_DEBUG_PRINT("[vorbis] unexpected stream position > stream length: %ld > %ld (%ld)\n",stream_position+ngiven,stream_length,stream_position+ngiven-stream_length);
        vorbis_synthesis_read(&dsp,stream_position+ngiven-stream_length);
        ngiven = stream_length - stream_position;
        }

      if (__unlikely(stream_position<stream_begin)) {
        FXlong offset = FXMIN(ngiven,stream_begin - stream_position);
        GM_DEBUG_PRINT("[vorbis] stream decode offset %ld. Skipping %ld of %ld \n",stream_begin,offset,stream_begin-stream_position);
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
        if (out==nullptr) {
          out = context->get_output_packet();
          if (out==nullptr) {
            if (packet) packet->unref();
            return true;
            }
          out->stream_position=stream_position;
          out->stream_length=stream_length - stream_offset_start;
          out->af=af;
          navail = out->availableFrames();
          }

#if defined(HAVE_VORBIS)
        buf32 = out->flt();
#elif defined(HAVE_TREMOR)
        buf32 = out->s16();
#else
#error "No vorbis decoder library specified"
#endif
        /// Copy Samples
        nsamples = FXMIN(ntotalsamples,navail);
        for (p=0,s=sample;s<(nsamples+sample);s++){
          for (c=0;c<info.channels;c++,p++) {
#if defined(HAVE_VORBIS)
            buf32[p]=pcm[c][s];
#elif defined(HAVE_TREMOR)
            buf32[p]=CLIP_TO_15(pcm[c][s]>>9);
#else
#error "No vorbis decoder library specified"
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
        if (navail==0) {
          context->post_output_packet(out);
          }
        }
      vorbis_synthesis_read(&dsp,ngiven);
      }
    }

  if (eos) {
    context->post_output_packet(out,true);
    }
  return true;
  }


DecoderPlugin * ap_vorbis_decoder(DecoderContext * ctx) {
  return new VorbisDecoder(ctx);
  }


}
