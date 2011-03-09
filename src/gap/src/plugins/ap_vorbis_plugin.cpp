#include "ap_defs.h"
#include "ap_config.h"
#include "ap_pipe.h"
#include "ap_format.h"
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

#include "ap_vorbis_plugin.h"


VorbisDecoder::VorbisDecoder(AudioEngine * e) : DecoderPlugin(e),buffer(32768),engine(e),out(NULL),has_info(false),has_dsp(false) {
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
  if (buffer.size() && buffer.size()>=sizeof(ogg_packet)) {
    buffer.read((FXuchar*)&op,sizeof(ogg_packet));

    if (buffer.size()<op.bytes) {
      buffer.data_ptr-=sizeof(ogg_packet);
      return false;
      }

    op.packet=buffer.data_ptr;
    buffer.data_ptr+=op.bytes;
    return true;
    }
  return false;
  }


//#define AP_AVAILABLE_FRAMES(p) ((OUTPUT_PACKET_SIZE / af.framesize()) - p->nframes);
//#define AP_MAX_FRAMES ((OUTPUT_PACKET_SIZE / af.framesize());


FXbool VorbisDecoder::is_vorbis_header() {
  return (op.bytes>6 && ((op.packet[0]==1) || (op.packet[0]==3) || (op.packet[0]==5)) && (compare((const FXchar*)&op.packet[1],"vorbis",6)==0));
  }

DecoderStatus VorbisDecoder::process(Packet * packet) {
  FXASSERT(packet);

  FXfloat ** pcm=NULL;
  FXfloat * buf32=NULL;
  FXint p,navail=0;

  FXint ngiven,ntotalsamples,nsamples,sample,maxsamples,offset,c,s;

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
    fxmessage("stream position unknown\n");
    data_ptr = buffer.data_ptr;
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
        fxmessage("vorbis_synthesis_headerin failed\n");
        return DecoderError;
        }

      if (data_ptr)
        data_ptr = buffer.data_ptr;
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
          fxmessage("found stream position: %ld\n",op.granulepos-nsamples);
          stream_position=op.granulepos-nsamples;
          buffer.data_ptr=data_ptr;
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


//          buf32 = reinterpret_cast<FXfloat*>(&(out->data[out->nframes*af.framesize()]));

          buf32 = reinterpret_cast<FXfloat*>(out->ptr());

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
    buffer.data_ptr=data_ptr;
    if (has_dsp) vorbis_synthesis_restart(&dsp);
    }

  if (eos && out) {
    if (out->numFrames())  {
      engine->output->post(out);
      out=NULL;
      }
    engine->output->post(new ControlEvent(Ctrl_EOS,id));
    engine->post(new Event(AP_EOS));
    }
  return DecoderOk;
  }
