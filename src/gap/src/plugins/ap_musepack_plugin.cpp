#include "ap_defs.h"
#include "ap_config.h"
#include "ap_pipe.h"
#include "ap_format.h"
#include "ap_device.h"
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
#include "ap_decoder_thread.h"


#include <mpcdec/mpcdec.h>

namespace ap {

/*
Due to the way musepack decoder is designed
the input plugin actually decodes the data.
*/
class MusepackInput : public InputPlugin {
protected:
  FXfloat              buffer[MPC_DECODER_BUFFER_LENGTH];
  FXuint               nframes;
  FXint                frame;

  FXlong               stream_position;
  Packet*              packet;
  mpc_streaminfo       si;
  mpc_reader_t         reader;
  mpc_decoder          decoder;
protected:
  InputStatus parse();
protected:
  static mpc_int32_t  mpc_input_read(void *t, void *ptr, mpc_int32_t size);
  static mpc_bool_t   mpc_input_seek(void *t, mpc_int32_t offset);
  static mpc_int32_t  mpc_input_tell(void *t);
  static mpc_int32_t  mpc_input_size(void *t);
  static mpc_bool_t   mpc_input_canseek(void*);
public:
  MusepackInput(AudioEngine*);
  FXbool init();
  FXbool can_seek() const;
  FXbool seek(FXdouble);
  InputStatus process(Packet*);
  };


mpc_int32_t MusepackInput::mpc_input_read(void *t, void *ptr, mpc_int32_t size){
  InputThread * input = reinterpret_cast<InputThread*>(t);
  FXASSERT(input);
  return input->read(ptr,size);
  }

mpc_bool_t MusepackInput::mpc_input_seek(void *t, mpc_int32_t offset){
  InputThread * input = reinterpret_cast<InputThread*>(t);
  FXASSERT(input);
  FXlong pos=input->position((FXlong)offset,FXIO::Begin);
  if (pos!=offset)
    return false;
  else
    return true;
  }

mpc_int32_t MusepackInput::mpc_input_tell(void *t){
  InputThread * input = reinterpret_cast<InputThread*>(t);
  FXASSERT(input);
  return (FXint) input->position();
  }

mpc_int32_t  MusepackInput::mpc_input_size(void *t){
  InputThread * input = reinterpret_cast<InputThread*>(t);
  FXASSERT(input);
  return  input->size();
  }

mpc_bool_t MusepackInput::mpc_input_canseek(void*t){
  InputThread * input = reinterpret_cast<InputThread*>(t);
  FXASSERT(input);
  return !input->serial();
  }

MusepackInput::MusepackInput(AudioEngine *e) : InputPlugin(e), stream_position(0) {
  reader.read     = mpc_input_read;
  reader.seek     = mpc_input_seek;
  reader.tell     = mpc_input_tell;
  reader.get_size = mpc_input_size;
  reader.canseek  = mpc_input_canseek;
  reader.data     = engine->input;
  }

FXbool MusepackInput::init() {
  mpc_streaminfo_init(&si);
//  mpc_decoder_setup(&decoder,&reader);
  nframes=0;
  frame=0;
  return true;
  }

FXbool MusepackInput::can_seek() const {
  return !engine->input->serial() && stream_length>0;
  }

FXbool MusepackInput::seek(FXdouble pos) {
  if (!engine->input->serial() && stream_length>0) {
    FXlong offset = stream_length*pos;
    if (mpc_decoder_seek_sample(&decoder,offset)){
      stream_position=offset;
      return true;
      }
    }
  return false;
  }


InputStatus MusepackInput::parse() {
  if (mpc_streaminfo_read(&si,&reader)==ERROR_CODE_OK) {
    stream_position = 0;
    stream_length   = mpc_streaminfo_get_length_samples(&si);
    af.set(AP_FORMAT_FLOAT,si.sample_freq,si.channels);
    engine->decoder->post(new ConfigureEvent(af,Codec::PCM,stream_length));

    mpc_decoder_setup(&decoder,&reader);
    if (!mpc_decoder_initialize(&decoder,&si))
      return InputError;

    flags|=FLAG_PARSED;
    return InputOk;
    }
  else
    return InputError;
  }

InputStatus MusepackInput::process(Packet * p){

  if (!(flags&FLAG_PARSED)) {
    fxmessage("parsing %d\n",flags&FLAG_PARSED);
    InputStatus status = parse();
    if (status!=InputOk) {
      p->unref();
      return status;
      }
    }

  packet                  = p;
  packet->stream_position = stream_position;
  packet->stream_length   = stream_length;
  packet->af              = af;
  packet->flags           = 0;

  while(packet) {

    if (nframes==0) {
      frame=0;
      nframes = mpc_decoder_decode(&decoder,buffer,0,0);
      if (nframes==(FXuint)-1) return InputError;
      else if (nframes==0) {
        if (packet->size()) {
          engine->decoder->post(packet);
          packet=NULL;
          }
        return InputDone;
        }
      }
    FXint ncopy = FXMIN(nframes,packet->space()/af.framesize());
    packet->append(((FXchar*)&buffer)+(frame*af.framesize()),ncopy*af.framesize());
    frame+=ncopy;
    nframes-=ncopy;
    stream_position+=ncopy;
    if (packet->space()<af.framesize()) {
      engine->decoder->post(packet);
      packet=NULL;
      }
    }
  return InputOk;
  }



InputPlugin * ap_musepack_input(AudioEngine * engine) {
  return new MusepackInput(engine);
  }

//DecoderPlugin * ap_mad_decoder(AudioEngine * engine) {
//  return new MadDecoder(engine);
//  }



}









