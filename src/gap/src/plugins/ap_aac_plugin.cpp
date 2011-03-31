#include "ap_defs.h"
#include "ap_config.h"
#include "ap_pipe.h"
#include "ap_format.h"
#include "ap_device.h"
#include "ap_event.h"
#include "ap_event_private.h"
#include "ap_memory_buffer.h"
#include "ap_packet.h"
#include "ap_event_queue.h"
#include "ap_thread_queue.h"
#include "ap_engine.h"
#include "ap_reader_plugin.h"
#include "ap_decoder_plugin.h"
#include "ap_thread.h"
#include "ap_input_thread.h"
#include "ap_decoder_thread.h"
#include "ap_output_thread.h"


#include "neaacdec.h"
#include "mp4ff.h"

namespace ap {

enum {
  AAC_FLAG_CONFIG = 0x1
  };

class AacInput : public ReaderPlugin {
protected:
  mp4ff_callback_t callback;
  mp4ff_t*         handle;
protected:
  static FXuint mp4_read(void*,void*,FXuint);
  static FXuint mp4_write(void*,void*,FXuint);
  static FXuint mp4_seek(void*,FXulong);
	static FXuint mp4_truncate(void*);
protected:
  Packet              * packet;
  FXlong                datastart;
  FXint                 nframes;
  FXint                 frame;
  FXint                 track;
protected:
  ReadStatus parse();
public:
  AacInput(AudioEngine*);
  FXuchar format() const { return Format::AAC; };

  FXbool init();
  FXbool can_seek() const;
  FXbool seek(FXdouble);
  ReadStatus process(Packet*);
  ~AacInput();
  };

class AacDecoder : public DecoderPlugin {
protected:
  NeAACDecHandle handle;
protected:
  Packet * in;
  Packet * out;
public:
  AacDecoder(AudioEngine*);
  FXuchar codec() const { return Codec::AAC; }
  FXbool flush();
  FXbool init(ConfigureEvent*);
  DecoderStatus process(Packet*);
  ~AacDecoder();
  };


static void ap_write_unsigned(Packet * p,FXuint b) {
  memcpy(p->ptr(),(const FXchar*)&b,4);
  p->wrote(4);
  }


static FXuint ap_read_unsigned(FXuchar * buffer,FXint pos) {
  FXuint v;
  ((FXuchar*)&v)[0]=buffer[pos+0];
  ((FXuchar*)&v)[1]=buffer[pos+1];
  ((FXuchar*)&v)[2]=buffer[pos+2];
  ((FXuchar*)&v)[3]=buffer[pos+3];
  return v;
  }


FXuint AacInput::mp4_read(void*ptr,void*data,FXuint len){
  InputThread* input = reinterpret_cast<InputThread*>(ptr);
  return (FXuint) input->read(data,len);
  }

FXuint AacInput::mp4_write(void*,void*,FXuint){
  FXASSERT(0);
//  InputThread* input = reinterpret_cast<InputThread*>(ptr);
  return 0;
  }

FXuint AacInput::mp4_seek(void*ptr,FXulong p){
  InputThread* input = reinterpret_cast<InputThread*>(ptr);
  return input->position(p,FXIO::Begin);
  }

FXuint AacInput::mp4_truncate(void*){
  FXASSERT(0);
  //InputThread* input = reinterpret_cast<InputThread*>(ptr);
  return 0;
  }



AacInput::AacInput(AudioEngine* e) : ReaderPlugin(e),handle(NULL),nframes(-1),track(-1) {
  callback.read      = mp4_read;
  callback.write     = mp4_write;
  callback.seek      = mp4_seek;
  callback.truncate  = mp4_truncate;
  callback.user_data = engine->input;
  }

AacInput::~AacInput(){
  if (handle) mp4ff_close(handle);
  }

FXbool AacInput::init() {

  if (handle) {
    mp4ff_close(handle);
    handle=NULL;
    }

  track=-1;
  frame=0;
  nframes=-1;

  flags&=~FLAG_PARSED;
  return true;
  }


FXbool AacInput::can_seek() const {
  return true;
  }

FXbool AacInput::seek(FXdouble pos){
  FXint f = mp4ff_find_sample(handle,track,pos*stream_length,NULL);
  if (f>=0) frame=f;
  return true;
  }


ReadStatus AacInput::process(Packet*p) {
  packet=p;
  packet->stream_position=-1;
  packet->stream_length=stream_length;
  packet->flags=0;

  if (!(flags&FLAG_PARSED)) {
    return parse();
    }

  for (;frame<nframes;frame++) {
    FXint size = mp4ff_read_sample_getsize(handle,track,frame);

    if (size<=0) {
      packet->unref();
      return ReadError;
      }

    if (packet->space()<(size+4)) {
      if (packet->size()) {
        engine->decoder->post(packet);
        packet=NULL;
        }
      return ReadOk;
      }

    /// Write size
    ap_write_unsigned(packet,size);

    FXint n = mp4ff_read_sample_v2(handle,track,frame,packet->ptr());
    if (n<=0) {
      packet->unref();
      return ReadError;
      }

    packet->wrote(size);

    if (packet->stream_position==-1)
      packet->stream_position = mp4ff_get_sample_position(handle,track,frame);

    if (frame==nframes-1)
      packet->flags|=FLAG_EOS;
    }

  if (packet->size() && packet->flags&FLAG_EOS) {
    engine->decoder->post(packet);
    packet=NULL;
    return ReadDone;
    }
  return ReadOk;
  }



ReadStatus AacInput::parse() {
  mp4AudioSpecificConfig cfg;
  FXuchar* buffer;
  FXuint   size;
  FXint    result;
  FXint    ntracks;


  FXASSERT(handle==NULL);
  FXASSERT(packet);

  handle = mp4ff_open_read(&callback);
  if (handle==NULL)
    goto error;

  ntracks = mp4ff_total_tracks(handle);
  if (ntracks<=0)
    goto error;

  for (FXint i=0;i<ntracks;i++) {
    if ((mp4ff_get_decoder_config(handle,i,&buffer,&size)==0) && buffer && size) {
      if (NeAACDecAudioSpecificConfig(buffer,size,&cfg)==0) {

        af.set(AP_FORMAT_S16,mp4ff_get_sample_rate(handle,i),mp4ff_get_channel_count(handle,i));
        af.debug();

        if (size>packet->space()) {
          fxmessage("MP4 config buffer is too big for decoder packet");
          free(buffer);
          goto error;
          }

        track=i;
        frame=0;
        nframes=mp4ff_num_samples(handle,i);
        stream_length=mp4ff_get_track_duration(handle,i);

        packet->append(buffer,size);
        packet->flags|=AAC_FLAG_CONFIG;
        engine->decoder->post(new ConfigureEvent(af,Codec::AAC));
        engine->decoder->post(packet);

        packet=NULL;
        flags|=FLAG_PARSED;
        free(buffer);
        return ReadOk;
        }
      free(buffer);
      }
    }

error:
  packet->unref();
  return ReadError;
  }








AacDecoder::AacDecoder(AudioEngine * e) : DecoderPlugin(e),handle(NULL),in(NULL),out(NULL) {
  }

AacDecoder::~AacDecoder() {
  flush();
  }

FXbool AacDecoder::init(ConfigureEvent*event) {
  af=event->af;

  if (handle) {
    NeAACDecClose(handle);
    handle=NULL;
    }

  handle = NeAACDecOpen();
  if (handle==NULL)
    return false;

  return true;
  }

FXbool AacDecoder::flush() {
  FXASSERT(out==NULL);
  return true;
  }

DecoderStatus AacDecoder::process(Packet*packet){
  long unsigned int samplerate;
  FXuchar channels;

  if (packet->flags&AAC_FLAG_CONFIG) {
    fxmessage("got config packet\n");
    if (NeAACDecInit2(handle,packet->data(),packet->size(),&samplerate,&channels)<0){
      packet->unref();
      return DecoderError;
      }
    }
  else {
    NeAACDecFrameInfo frame;

    FXint fs   = packet->stream_position;
    FXbool eos = packet->flags&FLAG_EOS;

    FXint total = packet->size();
    FXint nused =0;

    while (total>0) {

      if (out==NULL){
        out = engine->decoder->get_output_packet();
        if (out==NULL) return DecoderInterrupted;
        out->af = af;
        out->stream_position = fs;
        out->stream_length   = packet->stream_length;
        }


      FXuint sz = ap_read_unsigned(packet->data(),nused);
      nused+=4;
      total-=4;

      void * buffer = out->ptr();

      NeAACDecDecode2(handle,&frame,&packet->data()[nused],sz,&buffer,out->availableFrames()*out->af.framesize());

	    if(frame.error > 0) {

	      fxmessage("MP4 (%ld): %s\n",frame.bytesconsumed,faacDecGetErrorMessage(frame.error));
//	      fxmessage("Buffer size was %d\n",out->available_frames()*out->af.framesize());
//	      AP_RELEASE_EVENT(packet);
//        return DecoderPlugin::StatusError;
        nused+=sz;
        total-=sz;
	      }

      if (frame.bytesconsumed>0) {
        nused+=frame.bytesconsumed;
	      total-=frame.bytesconsumed;
	      }

      if (frame.samples) {
  	    fs+=(frame.samples/frame.channels);
	      out->wroteFrames((frame.samples/frame.channels));
        if (out->availableFrames()==0) {
          engine->output->post(out);
          out=NULL;
          }
        }
	    }

    if (eos) {
      if (out) {
        engine->output->post(out);
        out=NULL;
        }
      engine->output->post(new ControlEvent(Ctrl_EOS,packet->stream));
      engine->post(new Event(AP_EOS));
      }
    }

  packet->unref();
  return DecoderOk;
  }



ReaderPlugin * ap_aac_input(AudioEngine * engine) {
  return new AacInput(engine);
  }

DecoderPlugin * ap_aac_decoder(AudioEngine * engine) {
  return new AacDecoder(engine);
  }


}


