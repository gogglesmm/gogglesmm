#include "ap_defs.h"
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
#include "ap_pcm_plugin.h"

namespace ap {


PCMDecoder::PCMDecoder(AudioEngine * e) : DecoderPlugin(e), out(NULL) {
  }

PCMDecoder::~PCMDecoder() {
  flush();
  }

FXbool PCMDecoder::init(ConfigureEvent*event) {
  af=event->af;
  return true;
  }

FXbool PCMDecoder::flush() {
  if (out) {
    out->unref();
    out=NULL;
    }
  return true;
  }

DecoderStatus PCMDecoder::process(Packet*in) {
  FXASSERT(in);
  FXuint    nframes = in->numFrames();
  FXuchar * buffer  = in->ptr();
  while(nframes) {
    if (out==NULL) {
      out = engine->decoder->get_output_packet();
      if (out==NULL) return DecoderInterrupted; // FIXME
      out->af=af;
//      out->nframes=0;
      out->stream_position=in->stream_position;
      out->stream_length=in->stream_length;
      }
    out->appendFrames(buffer,nframes);
    if (out->full()){
      engine->output->post(out);
      out=NULL;
      }
    }
  if (out && (in->flags&FLAG_EOS)) {
    engine->output->post(out);
    out=NULL;
    }
  in->unref();
  return DecoderOk;
  }

}
