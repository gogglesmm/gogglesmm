#include "ap_defs.h"
#include "ap_pipe.h"
#include "ap_format.h"
#include "ap_device.h"
#include "ap_event.h"
#include "ap_event_private.h"
#include "ap_event_queue.h"
#include "ap_thread_queue.h"
#include "ap_buffer.h"
#include "ap_packet.h"
#include "ap_engine.h"
#include "ap_input_plugin.h"
#include "ap_decoder_plugin.h"
#include "ap_thread.h"
#include "ap_input_thread.h"
#include "ap_buffer.h"
#include "ap_decoder_thread.h"
#include "ap_output_thread.h"

namespace ap {

class PCMDecoder : public DecoderPlugin {
public:
  PCMDecoder(AudioEngine*);
  FXuchar codec() const { return Codec::PCM; }
  FXbool flush();
  FXbool init(ConfigureEvent*);
  DecoderStatus process(Packet*);
  virtual ~PCMDecoder();
  };

PCMDecoder::PCMDecoder(AudioEngine * e) : DecoderPlugin(e) {
  }

PCMDecoder::~PCMDecoder() {
  }

FXbool PCMDecoder::init(ConfigureEvent*/*event*/) {
  return true;
  }

FXbool PCMDecoder::flush() {
  return true;
  }

DecoderStatus PCMDecoder::process(Packet*in) {
  FXbool eos    = (in->flags&FLAG_EOS);
  FXint  stream = in->stream;

  /// Simply Forward to output
  engine->output->post(in);

  if (eos) {
    engine->output->post(new ControlEvent(End,stream));
    }
  return DecoderOk;
  }

DecoderPlugin * ap_pcm_decoder(AudioEngine * engine) {
  return new PCMDecoder(engine);
  }

}
