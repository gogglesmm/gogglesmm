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
#include "ap_reactor.h"
#include "ap_event_private.h"
#include "ap_buffer.h"
#include "ap_packet.h"
#include "ap_event_queue.h"
#include "ap_thread_queue.h"
#include "ap_engine.h"
#include "ap_reader_plugin.h"
#include "ap_input_plugin.h"
#include "ap_decoder_plugin.h"
#include "ap_thread.h"
#include "ap_input_thread.h"
#include "ap_decoder_thread.h"
#include "ap_output_thread.h"


//#include "ALACDecoder.h"
//#include "ALACBitUtilities.h"
#include "alac.h"

namespace ap {

enum {
  AAC_FLAG_CONFIG = 0x2,
  AAC_FLAG_FRAME  = 0x4
  };

#ifdef HAVE_ALAC

class AlacDecoder : public DecoderPlugin {
protected:
  MemoryBuffer   buffer;
  MemoryBuffer   outbuf;
  alac_file*     handle = nullptr;
  FXlong         stream_position = -1;
protected:
  Packet * out;
protected:
  FXbool getNextFrame(Packet *& packet,FXuchar *& ptr,FXuint & framesize);
public:
  AlacDecoder(AudioEngine*);
  FXuchar codec() const override { return Codec::ALAC; }
  FXbool flush(FXlong offset=0) override;
  FXbool init(ConfigureEvent*) override ;
  DecoderStatus process(Packet*) override;
  ~AlacDecoder();
  };


AlacDecoder::AlacDecoder(AudioEngine * e) : DecoderPlugin(e),handle(nullptr),stream_position(-1),out(NULL) {
  }

AlacDecoder::~AlacDecoder() {
  flush();
  if (handle) {
    dispose_alac(handle);
    handle=nullptr;
    }
  }


FXbool AlacDecoder::init(ConfigureEvent*event) {
  DecoderPlugin::init(event);
  buffer.clear();
  if (handle) {
    dispose_alac(handle);
    handle=nullptr;
    }
  handle=create_alac(event->af.bps(),event->af.channels);
  af=event->af;
  stream_position=-1;
  return true;
  }


FXbool AlacDecoder::flush(FXlong offset) {
  DecoderPlugin::flush(offset);
  buffer.clear();
  outbuf.clear();
  if (out) {
    out->unref();
    out=NULL;
    }
  stream_position=-1;
  return true;
  }



FXbool AlacDecoder::getNextFrame(Packet *& packet,FXuchar *& ptr,FXuint & framesize) {
  framesize=0;

  // data in local cache
  if (buffer.size()) {

    // read next framesize
    memcpy(&framesize,buffer.data(),4);

    // get frame from buffer
    if (framesize+4<=buffer.size()){
      buffer.readBytes(4);
      ptr=buffer.data();
      buffer.readBytes(framesize);
      return true;
      }

    // see if packet contains additional data
    if (packet) {

      // get missing data from packet
      FXuint missing = framesize - (buffer.size()-4);
      if (packet->size()>=missing) {
        buffer.append(packet->data(),missing);
        packet->readBytes(missing);
        buffer.readBytes(4);
        ptr=buffer.data();
        buffer.readBytes(framesize);
        if (packet->size()==0) {
          packet->unref();
          packet=nullptr;
          }
        return true;
        }

      // incomplete data in packet
      buffer.append(packet->data(),packet->size());
      packet->unref();
      packet=nullptr;
      }
    return false;
    }

  if (packet) {

    if (packet->size()>=4) {

      // read next framesize
      memcpy(&framesize,packet->data(),4);

      // get frame from packet
      if (framesize+4<=packet->size()){
        packet->readBytes(4);
        ptr=packet->data();
        packet->readBytes(framesize);
        if (packet->size()==0) {
          packet->unref();
          packet=nullptr;
          }
        return true;
        }
      }

    // incomplete data in packet
    buffer.append(packet->data(),packet->size());
    packet->unref();
    packet=nullptr;
    return false;
    }
  return false;
  }

DecoderStatus AlacDecoder::process(Packet*packet){
  const FXbool eos = packet->flags&FLAG_EOS;
  const FXlong stream_length = packet->stream_length;
  const FXuint stream_id = packet->stream;

  if (stream_position==-1) {
    stream_position = packet->stream_position;
    }

  if (packet->flags&AAC_FLAG_CONFIG) {
    alac_set_info(handle,(FXchar*)packet->data());
    outbuf.resize(handle->setinfo_max_samples_per_frame*af.framesize());
    packet->unref();
    packet=nullptr;
    }
  else {
    FXuchar * inputdata=nullptr;
    FXuint    framesize=0;

    while(getNextFrame(packet,inputdata,framesize)) {
      FXint nframes=outbuf.space();
      decode_frame(handle,inputdata,outbuf.ptr(),&nframes);
      outbuf.wroteBytes(nframes);
      nframes /= af.framesize();

      while(nframes) {

        // Get output packet
        if (out==NULL){
          out = engine->decoder->get_output_packet();
          if (out==nullptr) return DecoderInterrupted;
          out->af              = af;
          out->stream          = stream_id;
          out->stream_position = stream_position;
          out->stream_length   = stream_length;
          }

        // copy max frames
        FXuint ncopy = FXMIN(out->availableFrames(),nframes);
        out->appendFrames(outbuf.data(),ncopy);
        outbuf.readBytes(ncopy*af.framesize());
        nframes-=ncopy;
        stream_position+=ncopy;

        // Send to
        if (out->availableFrames()==0) {
          engine->output->post(out);
          out=NULL;
          }
        }
      outbuf.clear();
      }
    if (eos) {
      if (out) {
        engine->output->post(out);
        out=NULL;
        }
      engine->output->post(new ControlEvent(End,stream_id));
      }
    }
  return DecoderOk;
  }


DecoderPlugin * ap_alac_decoder(AudioEngine * engine) {
  return new AlacDecoder(engine);
  }

#endif
}


