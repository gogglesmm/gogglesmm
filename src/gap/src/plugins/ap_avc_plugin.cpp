#include "ap_defs.h"
#include "ap_config.h"
#include "ap_pipe.h"
#include "ap_format.h"
#include "ap_event.h"
#include "ap_event_private.h"
#include "ap_event_queue.h"
#include "ap_thread_queue.h"
#include "ap_memory_buffer.h"
#include "ap_packet.h"
#include "ap_engine.h"
#include "ap_input_plugin.h"
#include "ap_decoder_plugin.h"
#include "ap_thread.h"
#include "ap_input_thread.h"
#include "ap_memory_buffer.h"
#include "ap_decoder_thread.h"
#include "ap_memory_buffer.h"
#include "ap_output_thread.h"


extern "C" {
// Apparently we need this in order to use ffmpeg from c++.
// /usr/include/libavutil/common.h:168:47: error: ‘UINT64_C’ was not declared in this scope
#include <libavcodec/avcodec.h>
}

namespace ap {


class OutputPacket;

class AVDecoder : public DecoderPlugin {
protected:
  AVCodecContext * ctx;
protected:
  MemoryStream     buffer;
  MemoryBuffer     outbuf;
protected:
  Packet * out;
public:
  AVDecoder(AudioEngine*);
  FXuchar codec() const { return Codec::PCM; }
  FXbool flush();
  FXbool init(ConfigureEvent*);
  DecoderStatus process(Packet*);
  virtual ~AVDecoder();
  };





AVDecoder::AVDecoder(AudioEngine * e) : DecoderPlugin(e), ctx(NULL),outbuf(AVCODEC_MAX_AUDIO_FRAME_SIZE),out(NULL) {

  avcodec_init();
  avcodec_register_all();

  AVCodec * codec = avcodec_find_decoder(CODEC_ID_MP3);
  FXASSERT(codec);

  ctx = avcodec_alloc_context();
  if (avcodec_open(ctx,codec)<0)
    fxerror("error opening codec\n");


  }

AVDecoder::~AVDecoder() {
  flush();
  if (ctx) avcodec_close(ctx);
  }

FXbool AVDecoder::init(ConfigureEvent*event) {
  switch(ctx->sample_fmt) {
    case SAMPLE_FMT_U8    : event->af.format = AP_FORMAT_U8;     break;
    case SAMPLE_FMT_S16   : event->af.format = AP_FORMAT_S16;    break;
    case SAMPLE_FMT_S32   : event->af.format = AP_FORMAT_S32;    break;
    case SAMPLE_FMT_FLT   : event->af.format = AP_FORMAT_FLOAT;  break;
    default               : return false;                         break;
    }
  af=event->af;
  return true;
  }

FXbool AVDecoder::flush() {
  if (out) {
    out->unref();
    out=NULL;
    }
  return true;
  }

DecoderStatus AVDecoder::process(Packet*in) {
  AVPacket avp;

  fxmessage("decode packet %d\n",in->size());

  FXASSERT(in);
  buffer.append(in->data(),in->size());
  buffer.padding(FF_INPUT_BUFFER_PADDING_SIZE+1);
  in->unref();


  av_init_packet(&avp);

  avp.data = buffer.data_ptr;
  avp.size = buffer.size()-(FF_INPUT_BUFFER_PADDING_SIZE+1);

  fxmessage("buffer %d\n",buffer.size());


  int16_t * out_samples = reinterpret_cast<int16_t*>(outbuf.data());    /// output buffer
  int       out_size    = outbuf.capacity();                            /// output buffer size

  int result = avcodec_decode_audio3(ctx,out_samples,&out_size,&avp);
  fxmessage("decode_audio3: %d\n",result);

  if (result<0)
    return DecoderError;

  buffer.read(result+(FF_INPUT_BUFFER_PADDING_SIZE+1));

  if (out_size) {
    /// Get new buffer
    if (out==NULL) {
      out = engine->decoder->get_output_packet();
      if (out==NULL) return DecoderInterrupted; // FIXME
      out->af=af;
      }

//    fxmessage("got %d bytes %d %d\n",out_size,ctx->sample_rate,ctx->channels);
    out->append(outbuf.data(),out_size);
    if (out->availableFrames()==0) {
      engine->output->post(out);
      out=NULL;
      }
    }
  fxmessage("success\n");
  return DecoderOk;
  }


//InputPlugin * ap_aac_input(AudioEngine * engine) {
//  return new AacInput(engine);
//  }

DecoderPlugin * ap_avc_decoder(AudioEngine * engine) {
  return new AVDecoder(engine);
  }


}
