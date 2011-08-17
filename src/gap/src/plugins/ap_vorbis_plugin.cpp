#include "ap_defs.h"
#include "ap_config.h"
#include "ap_pipe.h"
#include "ap_format.h"
#include "ap_device.h"
#include "ap_memory_buffer.h"
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
#include <vorbis/codec.h>

namespace ap {

class VorbisDecoder : public DecoderPlugin{
protected:
  MemoryStream buffer;
protected:
  FXbool get_next_packet();
  FXbool is_vorbis_header();
protected:
  AudioEngine *     engine;
  vorbis_info       info;
  vorbis_comment    comment;
  vorbis_dsp_state  dsp;
  vorbis_block      block;
  ogg_packet        op;
  FXbool            has_info;
  FXbool            has_dsp;
  Packet *          out;
  FXint             stream_position;
public:
  VorbisDecoder(AudioEngine*);

  FXuchar codec() const { return Codec::Vorbis; }
  FXbool init(ConfigureEvent*);
  DecoderStatus process(Packet*);
  FXbool flush();

  virtual ~VorbisDecoder();
  };





VorbisDecoder::VorbisDecoder(AudioEngine * e) : DecoderPlugin(e),buffer(32768),engine(e),has_info(false),has_dsp(false),out(NULL) {
  }

VorbisDecoder::~VorbisDecoder(){
  if (has_dsp) {
    vorbis_block_clear(&block);
    vorbis_dsp_clear(&dsp);
    has_dsp=false;
    }
  if (has_info) {
    vorbis_comment_clear(&comment);
    vorbis_info_clear(&info);
    has_info=false;
    }
  }



FXbool VorbisDecoder::init(ConfigureEvent*event) {
  af=event->af;
  buffer.clear();

  if (has_dsp) {
    vorbis_block_clear(&block);
    vorbis_dsp_clear(&dsp);
    has_dsp=false;
    }

  if (has_info) {
    vorbis_comment_clear(&comment);
    vorbis_info_clear(&info);
    has_info=false;
    }

  /// Init
  vorbis_info_init(&info);
  vorbis_comment_init(&comment); // not strictly necessary
  has_info=true;

  stream_position=-1;
  return true;
  }


FXbool VorbisDecoder::flush() {
  if (out) {
    out->unref();
    out=NULL;
    }

  buffer.clear();
  stream_position=-1;
//  if (has_dsp)
//    vorbis_synthesis_restart(&dsp);
  return true;
  }


FXbool VorbisDecoder::get_next_packet() {
  if (buffer.size() && buffer.size()>=(FXival)sizeof(ogg_packet)) {
    buffer.read((FXuchar*)&op,sizeof(ogg_packet));
    if (buffer.size()<op.bytes) {
      buffer.sr-=sizeof(ogg_packet);
      return false;
      }

    op.packet=buffer.sr;
    buffer.sr+=op.bytes;
    return true;
    }
  return false;
  }


FXbool VorbisDecoder::is_vorbis_header() {
  return (op.bytes>6 && ((op.packet[0]==1) || (op.packet[0]==3) || (op.packet[0]==5)) && (compare((const FXchar*)&op.packet[1],"vorbis",6)==0));
  }

DecoderStatus VorbisDecoder::process(Packet * packet) {
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
    data_ptr = buffer.sr;
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
        data_ptr = buffer.sr;
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

        buffer.sr=data_ptr;
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
    buffer.sr=data_ptr;
    if (has_dsp) vorbis_synthesis_restart(&dsp);
    }

  if (eos && out) {
    if (out->numFrames())  {
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
