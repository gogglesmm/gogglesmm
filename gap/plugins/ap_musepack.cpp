/*******************************************************************************
*                         Goggles Audio Player Library                         *
********************************************************************************
*           Copyright (C) 2010-2016 by Sander Jansen. All Rights Reserved      *
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
#include "ap_event.h"
#include "ap_event_private.h"
#include "ap_event_queue.h"
#include "ap_thread_queue.h"
#include "ap_buffer.h"
#include "ap_packet.h"
#include "ap_engine.h"
#include "ap_reader_plugin.h"
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
class MusepackReader : public ReaderPlugin {
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
  ReadStatus parse();
protected:
  static mpc_int32_t  mpc_input_read(void *t, void *ptr, mpc_int32_t size);
  static mpc_bool_t   mpc_input_seek(void *t, mpc_int32_t offset);
  static mpc_int32_t  mpc_input_tell(void *t);
  static mpc_int32_t  mpc_input_size(void *t);
  static mpc_bool_t   mpc_input_canseek(void*);
public:
  MusepackReader(AudioEngine*);
  FXuchar format() const { return Format::Musepack; };
  FXbool init(InputPlugin*);
  FXbool can_seek() const;
  FXbool seek(FXdouble);
  ReadStatus process(Packet*);
  };


mpc_int32_t MusepackReader::mpc_input_read(void *t, void *ptr, mpc_int32_t size){
  InputThread * input = static_cast<InputThread*>(t);
  FXASSERT(input);
  return input->read(ptr,size);
  }

mpc_bool_t MusepackReader::mpc_input_seek(void *t, mpc_int32_t offset){
  InputThread * input = static_cast<InputThread*>(t);
  FXASSERT(input);
  FXlong pos=input->position((FXlong)offset,FXIO::Begin);
  if (pos!=offset)
    return false;
  else
    return true;
  }

mpc_int32_t MusepackReader::mpc_input_tell(void *t){
  InputThread * input = static_cast<InputThread*>(t);
  FXASSERT(input);
  return (FXint) input->position();
  }

mpc_int32_t  MusepackReader::mpc_input_size(void *t){
  InputThread * input = static_cast<InputThread*>(t);
  FXASSERT(input);
  return  input->size();
  }

mpc_bool_t MusepackReader::mpc_input_canseek(void*t){
  InputThread * input = static_cast<InputThread*>(t);
  FXASSERT(input);
  return !input->serial();
  }

MusepackReader::MusepackReader(AudioEngine *e) : ReaderPlugin(e), stream_position(0), packet(nullptr) {
  reader.read     = mpc_input_read;
  reader.seek     = mpc_input_seek;
  reader.tell     = mpc_input_tell;
  reader.get_size = mpc_input_size;
  reader.canseek  = mpc_input_canseek;
  reader.data     = engine->input;
  }

FXbool MusepackReader::init(InputPlugin*plugin) {
  ReaderPlugin::init(plugin);
  mpc_streaminfo_init(&si);
//  mpc_decoder_setup(&decoder,&reader);
  nframes=0;
  frame=0;
  return true;
  }

FXbool MusepackReader::can_seek() const {
  return !engine->input->serial() && stream_length>0;
  }

FXbool MusepackReader::seek(FXdouble pos) {
  if (!engine->input->serial() && stream_length>0) {
    FXlong offset = stream_length*pos;
    if (mpc_decoder_seek_sample(&decoder,offset)){
      stream_position=offset;
      return true;
      }
    }
  return false;
  }


ReadStatus MusepackReader::parse() {
  FXint error=0;
  if ((error=mpc_streaminfo_read(&si,&reader))==ERROR_CODE_OK) {
    stream_position = 0;
    stream_length   = mpc_streaminfo_get_length_samples(&si);
    af.set(AP_FORMAT_FLOAT,si.sample_freq,si.channels);
    engine->decoder->post(new ConfigureEvent(af,Codec::PCM,stream_length));

    mpc_decoder_setup(&decoder,&reader);
    if (!mpc_decoder_initialize(&decoder,&si))
      return ReadError;

    flags|=FLAG_PARSED;
    return ReadOk;
    }
  else {
    return ReadError;
    }
  }

ReadStatus MusepackReader::process(Packet * packet){

  if (!(flags&FLAG_PARSED)) {
    GM_DEBUG_PRINT("parsing %d\n",flags&FLAG_PARSED);
    ReadStatus status = parse();
    if (status!=ReadOk) {
      packet->unref();
      return status;
      }
    }

  packet->stream_position = stream_position;
  packet->stream_length   = stream_length;
  packet->af              = af;
  packet->flags           = 0;

  while(packet) {

    if (nframes==0) {
      frame=0;
      nframes = mpc_decoder_decode(&decoder,buffer,0,0);
      if (nframes==(FXuint)-1) {
        packet->unref();
        return ReadError;
        }
      else if (nframes==0) {
        FXint stream = packet->stream;
        if (packet->size()) {
          engine->decoder->post(packet);
          packet=nullptr;
          }
        else {
          packet->unref();
          }
        engine->decoder->post(new ControlEvent(End,stream));
        return ReadDone;
        }
      }
    FXint ncopy = FXMIN(nframes,packet->space()/af.framesize());
    packet->append(((FXchar*)&buffer)+(frame*af.framesize()),ncopy*af.framesize());
    frame+=ncopy;
    nframes-=ncopy;
    stream_position+=ncopy;
    if (packet->space()<af.framesize()) {
      engine->decoder->post(packet);
      packet=nullptr;
      }
    }
  return ReadOk;
  }



ReaderPlugin * ap_musepack_reader(AudioEngine * engine) {
  return new MusepackReader(engine);
  }

//DecoderPlugin * ap_mad_decoder(AudioEngine * engine) {
//  return new MadDecoder(engine);
//  }



}









