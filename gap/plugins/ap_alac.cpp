/*******************************************************************************
*                         Goggles Audio Player Library                         *
********************************************************************************
*           Copyright (C) 2010-2017 by Sander Jansen. All Rights Reserved      *
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
#include "ap_buffer.h"
#include "ap_packet.h"
#include "ap_event_private.h"
#include "ap_decoder_plugin.h"

#include "alac.h"

namespace ap {


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
  AlacDecoder(DecoderContext*);
  FXuchar codec() const override { return Codec::ALAC; }
  FXbool flush(FXlong offset=0) override;
  FXbool init(ConfigureEvent*) override ;
  FXbool process(Packet*) override;
  ~AlacDecoder();
  };


AlacDecoder::AlacDecoder(DecoderContext * e) : DecoderPlugin(e),handle(nullptr),stream_position(-1),out(NULL) {
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
  af=event->af;

  // Alac Decoder
  handle=create_alac(event->af.bps(),event->af.channels);
  if (handle==nullptr)
    return false;

  // Make sure we can init the decoder
  DecoderSpecificConfig * dc = dynamic_cast<DecoderSpecificConfig*>(event->dc);
  if (dc==nullptr && dc->config_bytes==0) {
    dispose_alac(handle);
    handle=nullptr;
    return false;
    }

  alac_set_info(handle,(FXchar*)dc->config);
  outbuf.resize(handle->setinfo_max_samples_per_frame*af.framesize());
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
        return true;
        }
      }

    // incomplete data in packet
    if (packet->size())
      buffer.append(packet->data(),packet->size());
    packet->unref();
    packet=nullptr;
    return false;
    }
  return false;
  }

FXbool AlacDecoder::process(Packet*packet){
  const FXbool eos = packet->flags&FLAG_EOS;
  const FXlong stream_length = packet->stream_length;
  const FXuint stream_id = packet->stream;

  if (stream_position==-1) {
    stream_position = packet->stream_position;
    }

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
        out = context->get_output_packet();
        if (out==nullptr) return true;
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
        context->post_output_packet(out);
        }
      }
    outbuf.clear();
    }

  if (eos) {
    context->post_output_packet(out,true);
    }
  return true;
  }


DecoderPlugin * ap_alac_decoder(DecoderContext * ctx) {
  return new AlacDecoder(ctx);
  }

#endif
}


