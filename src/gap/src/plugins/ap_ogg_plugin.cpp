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
#include "ap_input_plugin.h"
#include "ap_decoder_plugin.h"
#include "ap_thread.h"
#include "ap_input_thread.h"
#include "ap_memory_buffer.h"
#include "ap_decoder_thread.h"

#include <ogg/ogg.h>
#include <vorbis/codec.h>

namespace ap {

class AudioEngine;

struct OggInputState {
  FXbool has_stream;
  FXbool has_eos;
  FXbool has_page;
  FXbool has_packet;
  FXbool header_written;
  FXuint bytes_written;
  OggInputState() : has_stream(false),has_eos(false),has_page(false),has_packet(false),header_written(false),bytes_written(0) {}
  void reset() {has_stream=false;has_eos=false;has_page=false; has_packet=false; header_written=false;bytes_written=0; }
  };

class OggInput : public InputPlugin {
protected:
  enum {
    FLAG_VORBIS_HEADER_INFO    = 0x2,
    FLAG_VORBIS_HEADER_COMMENT = 0x4,
    FLAG_VORBIS_HEADER_BLOCK   = 0x8,
    };
protected:
  OggInputState    state;
  ogg_stream_state stream;
  ogg_sync_state   sync;
  ogg_page         page;
  ogg_packet       op;
protected:
  vorbis_info      vi;
  vorbis_comment   vc;
protected:
  Packet *        packet;
  FXint           ogg_packet_written;
  InputStatus     status;
  FXuchar         codec;
  FXlong          stream_start;
  FXlong          input_position;
protected:
  FXbool match_page();
  FXbool fetch_next_page();
  FXbool fetch_next_packet();
  void   submit_ogg_packet();
  void   check_vorbis_length(vorbis_info*);
  InputStatus parse();
  InputStatus parse_vorbis_stream();
public:
  OggInput(AudioEngine *);
  FXuchar format() const { return Format::OGG; };
  FXbool init();
  FXbool seek(FXdouble);
  FXbool can_seek() const;
  InputStatus process(Packet*);
  virtual ~OggInput();
  };














#ifdef HAVE_FLAC_PLUGIN
extern FXbool flac_parse_streaminfo(const FXuchar * buffer,AudioFormat & config,FXlong & nframes);
#endif

OggInput::OggInput(AudioEngine * e) : InputPlugin(e),packet(NULL),ogg_packet_written(-1),codec(Codec::Invalid) {
  ogg_sync_init(&sync);
  }


OggInput::~OggInput() {
  ogg_sync_clear(&sync);
  }

FXbool OggInput::can_seek() const {
  return stream_length>0;
  }

FXbool OggInput::seek(FXdouble pos){
 FXASSERT(stream_length>0);

    if (packet) {
      packet->unref();
      packet=NULL;
      }

    state.has_packet=false;
    state.has_page=false;
    state.header_written=false;
    state.bytes_written=0;
    ogg_sync_reset(&sync);
    ogg_stream_reset(&stream);


    FXlong target  = stream_start + (stream_length * pos);
    FXlong offset  = engine->input->size() * pos;
    FXlong lastpos = -1;

    input_position = engine->input->position(offset,FXIO::Begin);

    fxmessage("target seek %ld / %ld => %ld\n",target,stream_length,offset);

    while(fetch_next_page()) {
      if (ogg_page_granulepos(&page)>target) {
        fxmessage("found %ld %ld %ld\n",ogg_page_granulepos(&page),lastpos,offset);
        if (lastpos>=0 || offset==0) {
          engine->input->position((lastpos>=0) ? lastpos : offset,FXIO::Begin);
          ogg_sync_reset(&sync);
          ogg_stream_reset(&stream);
          return true;
          }
        else {
          offset=FXMIN(0,offset>>1);
          fxmessage("went to far. start at %ld\n",offset);
          input_position=engine->input->position(offset,FXIO::Begin);
          lastpos=-1;
          ogg_sync_reset(&sync);
          ogg_stream_reset(&stream);
          }
        }
      else {
        lastpos=input_position;
        }
      }

    if (lastpos==-1) {
      fxmessage("seeking beyond end of stream\n");
      }
    else {
      fxmessage("seeking to %ld\n",lastpos);
      engine->input->position(lastpos,FXIO::Begin);
      ogg_sync_reset(&sync);
      ogg_stream_reset(&stream);
      }
    return true;
  }


FXbool OggInput::init() {
  ogg_sync_clear(&sync);
  ogg_sync_reset(&sync);
  flags&=~(FLAG_PARSED|FLAG_VORBIS_HEADER_INFO|FLAG_VORBIS_HEADER_COMMENT|FLAG_VORBIS_HEADER_BLOCK);
  status=InputOk;

  input_position=0;

  stream_length=0;
  stream_start=0;

  if (state.has_stream) {
    ogg_stream_clear(&stream);
    state.has_stream=false;
    }
  return true;
  }



#define BUFFERSIZE 32768

FXbool OggInput::match_page() {
  if (state.has_stream) {
    if (ogg_page_serialno(&page)==stream.serialno)
      return true;
    }
  else {
    if (ogg_page_bos(&page)) {
      FXint serial = ogg_page_serialno(&page);
      ogg_stream_init(&stream,serial);
      state.has_stream=true;
      return true;
      }
    }
  return false;
  }

void OggInput::check_vorbis_length(vorbis_info * info) {
  stream_length=0;
  stream_start=0;
  if (!engine->input->serial()) {
    FXlong cpos = input_position;
    FXlong size = engine->input->size();

    /// First Determine the pcm offset at the start of the stream
    FXint cb,lb=-1,tb=0;
    while(fetch_next_packet()) {
      cb=vorbis_packet_blocksize(info,&op);
      if (lb!=-1) tb+=(lb+cb)>>2;
      lb=cb;
      if (op.granulepos!=-1) {
        stream_start=op.granulepos-tb;
        fxmessage("stream offset=%ld %d %ld\n",op.granulepos,tb,stream_start);
        break;
        }
      }


    /// TODO need a smart way of finding the last page in stream.
    if (size>=0xFFFF) {
      engine->input->position((size-0xFFFF),FXIO::Begin);
      ogg_sync_reset(&sync);
      }

    /// Go to last page of stream to find out last pcm position
    while(fetch_next_page()) {
      if (ogg_page_eos(&page)) {
        stream_length = ogg_page_granulepos(&page) - stream_start;
        fxmessage("found total %ld frames\n",stream_length);
        break;
        }
      }
    engine->input->position(cpos,FXIO::Begin);
    ogg_sync_reset(&sync);
    ogg_stream_reset(&stream);
    }
  }



InputStatus OggInput::parse_vorbis_stream() {
  if (op.packet[0]==1) {

    vorbis_info_init(&vi);
    vorbis_comment_init(&vc);

    if (vorbis_synthesis_headerin(&vi,&vc,&op)<0)
      goto error;

    codec=Codec::Vorbis;
    af.set(AP_FORMAT_FLOAT,vi.rate,vi.channels);
    engine->decoder->post(new ConfigureEvent(af,codec));

    flags|=FLAG_VORBIS_HEADER_INFO;
    submit_ogg_packet();
    }
  else if (op.packet[0]==3) {

    if (!(flags&FLAG_VORBIS_HEADER_INFO))
      goto error;

    if (vorbis_synthesis_headerin(&vi,&vc,&op)<0)
      goto error;

    flags|=FLAG_VORBIS_HEADER_COMMENT;
    submit_ogg_packet();
    }
  else if (op.packet[0]==5) {

    if (!(flags&(FLAG_VORBIS_HEADER_INFO|FLAG_VORBIS_HEADER_COMMENT)))
      goto error;

    if (vorbis_synthesis_headerin(&vi,&vc,&op)<0)
      goto error;

    flags|=(FLAG_VORBIS_HEADER_BLOCK|FLAG_PARSED);
    submit_ogg_packet();

    check_vorbis_length(&vi);

    /// Success
    vorbis_info_clear(&vi);
    vorbis_comment_clear(&vc);
    }
  else {
    goto error;
    }
  return InputOk;
error:
  vorbis_info_clear(&vi);
  vorbis_comment_clear(&vc);
  return InputError;
  }




InputStatus OggInput::parse() {
  input_position = engine->input->position();

  while(packet) {

    if (!fetch_next_packet())
      return InputError;

    if (compare((const FXchar*)&op.packet[1],"vorbis",6)==0){

      if (parse_vorbis_stream()!=InputOk)
        return InputError;
      }

#ifdef HAVE_FLAC_PLUGIN
    else if (compare((const FXchar*)&op.packet[1],"FLAC",4)==0) {

      /// Make sure we have enough bytes
      if (op.bytes<51)
        return InputError;

      /// Check Mapping Version
      if (op.packet[5]!=0x01 || op.packet[6]!=0x00)
        return InputError;

      /// Parse the stream info block.
      /// FIXME stream_length may not be set.
      if (!flac_parse_streaminfo(op.packet+13,af,stream_length))
        return InputError;

      /// Post Config, done parsing.
      codec=Codec::FLAC;
      engine->decoder->post(new ConfigureEvent(af,codec,stream_length));

      flags|=FLAG_PARSED;

      submit_ogg_packet();

      /// Success
      return InputOk;
      }
#endif
    else {
      return InputError;
      }

    if (flags&FLAG_PARSED)
      return InputOk;
    }
  return InputError;
  }


void OggInput::submit_ogg_packet() {
  FXASSERT(packet);
  FXASSERT(packet->capacity()>sizeof(ogg_packet));

  state.has_packet=true;

  if (state.header_written==false) {
    if (codec==Codec::Vorbis) {
      if (packet->space()>sizeof(ogg_packet)) {
        packet->append(&op,sizeof(ogg_packet));
        state.header_written=true;
        if (packet->stream_position==-1) {
          packet->stream_position=op.granulepos;
          }
        }
      else {
        packet->af=af;
        engine->decoder->post(packet);
        packet=NULL;
        return;
        }
      }
    else {
      state.header_written=true;
      }
    }

  FXuint nbytes = FXMIN((op.bytes-state.bytes_written),packet->space());
  if (nbytes) {
    packet->append(&op.packet[state.bytes_written],nbytes);
    state.bytes_written+=nbytes;
    if (op.e_o_s) { packet->flags|=FLAG_EOS; state.has_eos=true; }
    }

  /// Check to make sure we're done with the packet
  if (state.header_written && state.bytes_written==op.bytes) {
    state.header_written=false;
    state.bytes_written=0;
    state.has_packet=false;
    }

  if (packet->space()==0 || (packet->flags&FLAG_EOS)) {
    packet->af=af;
    engine->decoder->post(packet);
    packet=NULL;
    }
  }






FXbool OggInput::fetch_next_page() {
/*
  // Available from Ogg 1.1.4
  if(ogg_sync_check(&sync))
    return false;
*/
  while(1) {
    long result=ogg_sync_pageseek(&sync,&page);
    if (result>0) { /// Return page with size result
      input_position+=result;
      if (match_page())
        return true;
      }
    else if (result==0) { /// Need more bytes
      FXchar * buffer=ogg_sync_buffer(&sync,BUFFERSIZE);
      if (buffer==NULL) return false;
      FXival nbytes = engine->input->read(buffer,BUFFERSIZE);
      if (nbytes<=0)
        return false;
      ogg_sync_wrote(&sync,nbytes);
      }
    else { // skipped some bytes
      input_position+=(-result);
      }
    }
  return false;
  }


FXbool OggInput::fetch_next_packet() {
  FXint result;
  while(1) {
    if (state.has_page==false) {

      if (!fetch_next_page())
        return false;

      if (ogg_stream_pagein(&stream,&page))
        return false;

      state.has_page=true;
      }
    do {
      result=ogg_stream_packetout(&stream,&op);
      if (result==1) {
        return true;
        }
      }
    while(result==-1);
    state.has_page=false;
    }
  return false;
  }


InputStatus OggInput::process(Packet * p) {
  packet                  = p;
  packet->stream_position = -1;
  packet->stream_length   = stream_length;

  if (state.has_packet)
    submit_ogg_packet();

  if (__unlikely(!(flags&FLAG_PARSED))) {
    InputStatus result = parse();
    if (result!=InputOk)
      return result;
    }

  while(packet && fetch_next_packet()) {
    submit_ogg_packet();
    }

  if (state.has_eos)
    return InputDone;
  else
    return InputOk;
  }


InputPlugin * ap_ogg_input(AudioEngine * engine) {
  return new OggInput(engine);
  }


}









