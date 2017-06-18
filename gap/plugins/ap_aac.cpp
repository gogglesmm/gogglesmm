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
#include "ap_packet.h"
#include "ap_event_private.h"
#include "ap_reader_plugin.h"
#include "ap_input_plugin.h"
#include "ap_decoder_plugin.h"

#include "neaacdec.h"

namespace ap {

#ifdef HAVE_FAAD

class AACReader : public ReaderPlugin {
public:
  AACReader(InputContext * ctx) : ReaderPlugin(ctx) {}
  FXbool init(InputPlugin*plugin) override { ReaderPlugin::init(plugin); flags=0; return true; }
  FXuchar format() const override { return Format::AAC; }

  ReadStatus process(Packet*p) override;

  ~AACReader() {}
  };

ReaderPlugin * ap_aac_reader(InputContext * ctx) {
  return new AACReader(ctx);
  }

ReadStatus AACReader::process(Packet*packet) {
  if (!(flags&FLAG_PARSED)) {
    GM_DEBUG_PRINT("[aac] finding sync\n");
    FXuchar buffer[2];
    if (input->read(buffer,2)!=2)
      return ReadError;
    do {
      if ((buffer[0]==0xFF) && (buffer[1]&0xf0)==0xf0) {
        GM_DEBUG_PRINT("[aac] found sync\n");
        context->post_configuration(new ConfigureEvent(af,Codec::AAC));
        flags|=FLAG_PARSED;
        packet->append(buffer,2);
        break;
        }
      buffer[0]=buffer[1];
      if (input->read(&buffer[1],1)!=1)
        return ReadError;
      }
    while(1);
    }
  return ReaderPlugin::process(packet);
  }











class AacDecoder : public DecoderPlugin {
protected:
  NeAACDecHandle handle;
  MemoryBuffer   buffer;
  FXlong         stream_position = -1;
  FXushort       stream_offset_start;
  FXbool         rawmode=false;
  FXbool         use_internal_buffer=false;
protected:
  Packet *       out = nullptr;
protected:
  FXbool getNextFrame(Packet *& packet,FXuchar *& ptr,FXuint & framesize);
  FXbool process_frames(Packet*);
  FXbool process_raw(Packet*);
  FXbool create();
  FXint  process_output(FXuint streamid,FXlong stream_length,void * outsamples,FXint nsamples);
public:
  AacDecoder(DecoderContext*);
  FXuchar codec() const override { return Codec::AAC; }
  FXbool flush(FXlong offset=0) override;
  FXbool init(ConfigureEvent*) override ;
  FXbool process(Packet*) override;
  ~AacDecoder();
  };



AacDecoder::AacDecoder(DecoderContext * e) : DecoderPlugin(e),
  handle(nullptr),
  buffer(0) {
  }

AacDecoder::~AacDecoder() {
  flush();
  if (handle) {
    NeAACDecClose(handle);
    handle=nullptr;
    }
  }


FXbool AacDecoder::create() {
  static const FXbool is_fixed_point_decoder = NeAACDecGetCapabilities()&FIXED_POINT_CAP;

  handle = NeAACDecOpen();
  if (handle==nullptr) return false;

  NeAACDecConfiguration * config = NeAACDecGetCurrentConfiguration(handle);
  if (__unlikely(is_fixed_point_decoder)) {
    config->outputFormat = FAAD_FMT_16BIT;
    af.format = AP_FORMAT_S16;
    }
  else {
    config->outputFormat = FAAD_FMT_FLOAT;
    af.format = AP_FORMAT_FLOAT;
    }
  NeAACDecSetConfiguration(handle, config);
  return true;
  }

FXbool AacDecoder::init(ConfigureEvent*event) {
  DecoderPlugin::init(event);
  buffer.clear();
  af=event->af;
  if (handle) {
    NeAACDecClose(handle);
    handle=nullptr;
    }
  use_internal_buffer=false;

  DecoderSpecificConfig * ac = dynamic_cast<DecoderSpecificConfig*>(event->dc);
  if (ac) {
    long unsigned int samplerate;
    FXuchar           channels;
    if (create() && NeAACDecInit2(handle,ac->config,ac->config_bytes,&samplerate,&channels)<0){
      NeAACDecClose(handle);
      handle=nullptr;
      return false;
      }
    event->af=af; // Pass decoder output format back to OutputThread.
    rawmode=false;
    }
  else {
    rawmode=true;
    }
  stream_position=-1;
  stream_offset_start=event->stream_offset_start;
  return true;
  }

FXbool AacDecoder::flush(FXlong offset) {
  DecoderPlugin::flush(offset);
  buffer.clear();
  if (out) {
    out->unref();
    out=nullptr;
    }
  stream_position=-1;
  if (handle) NeAACDecPostSeekReset(handle,0); // always drop the first frame
  return true;
  }


FXbool AacDecoder::getNextFrame(Packet *& packet,FXuchar *& ptr,FXuint & framesize) {
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


FXint AacDecoder::process_output(FXuint stream_id,FXlong stream_length,void * outsamples,FXint nsamples) {
  const FXlong stream_begin = FXMAX(stream_offset_start,stream_decode_offset);

  FXint nframes = nsamples / af.channels;
  //GM_DEBUG_PRINT("process_output %d\n",nframes);

  // trim all samples
  if ((stream_position+nframes)<stream_begin) {
    stream_position+=nframes;
    return 0;
    }

  // trim end of stream
  if (stream_length>0 && stream_position+nframes > stream_length) {
    nframes = FXMIN(nframes,stream_length-stream_position);
    }

  if (use_internal_buffer) {
    FXuchar * in = reinterpret_cast<FXuchar*>(outsamples);
    if (stream_position<stream_begin) {
      FXint skip_frames = FXMIN(nframes,(stream_begin-stream_position));
      FXint nbytes = (skip_frames*af.framesize());
      in+=nbytes;
      nframes-=skip_frames;
      stream_position += skip_frames;
      }
    while(nframes) {
      if (out==nullptr){
        out = context->get_output_packet();
        if (out==nullptr) return 1;
        out->af              = af;
        out->stream          = stream_id;
        out->stream_position = stream_position - stream_offset_start;
        out->stream_length   = stream_length - stream_offset_start;
        }
      stream_position += out->copyFrames(in,nframes);
      if (out->availableFrames()==0)
        context->post_output_packet(out);
      }
    }
  else {
    stream_position+=nframes;
    out->wroteFrames(nframes);
    if (stream_position<stream_begin) {
      FXint skip_frames = FXMIN(nframes,(stream_begin-stream_position));
      out->trimBegin(af.framesize()*skip_frames);
      }
    if (out->availableFrames() < (nsamples / af.channels))
      context->post_output_packet(out);
    }
  return 0;
  }


FXbool AacDecoder::process_raw(Packet*packet) {
  const FXbool eos           = packet->flags&FLAG_EOS;
  const FXlong stream_length = packet->stream_length;
  const FXuint stream_id     = packet->stream;

  if (stream_position==-1 && buffer.size()==0) {
    GM_DEBUG_PRINT("[aac] stream_position %lld\n",stream_position);
    stream_position = packet->stream_position;
    }

  // buffer data
  buffer.append(packet->data(),packet->size());
  packet->unref();
  packet=nullptr;

  // initialize the decoder
  if (handle==nullptr) {

    long unsigned int samplerate;
    FXuchar channels;

    // Create Decoder Instance
    if (!create())
      return false;

    // Init Decoder
    long n = NeAACDecInit(handle,buffer.data(),buffer.size(),&samplerate,&channels);
    if (n<0) {
      buffer.clear();
      return false;
      }
    else if (n>0) {
      buffer.readBytes(n);
      }

    af.rate = samplerate;
    af.setChannels(channels);
    context->post_configuration(new ConfigureEvent(af,Codec::AAC));
    stream_position=0;
    }

  const FXuint bytes_needed  = FAAD_MIN_STREAMSIZE*af.channels;
  NeAACDecFrameInfo frame;

  // decode data
  do {

    void * outsamples = nullptr;

    // done for now
    if (buffer.size() < bytes_needed && eos == false)
      return true;

    // get output buffer
    if (use_internal_buffer==false && out==nullptr){
      out = context->get_output_packet();
      if (out==nullptr) return true;
      out->af              = af;
      out->stream          = stream_id;
      out->stream_position = stream_position;
      out->stream_length   = stream_length - stream_offset_start;
      }

    // Looks like the decoder already takes care of stripping any encoder delay. So first call will return 0 samples.
    if (use_internal_buffer) {
      outsamples = NeAACDecDecode(handle,&frame,buffer.data(),buffer.size());
      }
    else {
      void * outbuffer = out->ptr();
      NeAACDecDecode2(handle,&frame,buffer.data(),buffer.size(),&outbuffer,out->availableFrames()*out->af.framesize());
      if (frame.error == 27) {
        GM_DEBUG_PRINT("[aac] using faad internal buffer (%d)\n",out->availableFrames()*out->af.framesize());
        use_internal_buffer=true;
        outsamples = NeAACDecDecode(handle,&frame,buffer.data(),buffer.size());
        }
      }

    if (frame.error > 0) {
      GM_DEBUG_PRINT("[aac] error %hhu (%lu) buffer size %ld: %s\n",frame.error,frame.bytesconsumed,buffer.size(),faacDecGetErrorMessage(frame.error));
      return false;
      }


    if (frame.bytesconsumed > 0) {
      buffer.readBytes(frame.bytesconsumed);
      }

    if (frame.samples==0)
      continue;

    if (process_output(stream_id, stream_length, outsamples, frame.samples))
      return true;
    }
  while( buffer.size() && frame.bytesconsumed );


  if (eos) {
    FXASSERT(stream_position==stream_length);
    //GM_DEBUG_PRINT("stream_position %ld == stream_length %ld\n",stream_position-stream_offset_start,stream_length-stream_offset_start);
    context->post_output_packet(out,true);
    }
  return true;
  }


FXbool AacDecoder::process_frames(Packet*packet) {
  const FXbool eos           = packet->flags&FLAG_EOS;
  const FXlong stream_length = packet->stream_length;
  const FXuint stream_id     = packet->stream;
  FXuchar *    framedata     = nullptr;
  FXuint       framesize     = 0;
  void * outsamples = nullptr;

  if (stream_position==-1) {
    GM_DEBUG_PRINT("[aac] stream_position %ld\n",stream_position);
    stream_position = packet->stream_position;
    }

  while(getNextFrame(packet,framedata,framesize)) {

    NeAACDecFrameInfo frame;

    // get output buffer
    if (use_internal_buffer==false && out==nullptr){
      out = context->get_output_packet();
      if (out==nullptr) return true;
      out->af              = af;
      out->stream          = stream_id;
      out->stream_position = stream_position;
      out->stream_length   = stream_length - stream_offset_start;
      }

    // Looks like the decoder already takes care of stripping any encoder delay. So first call will return 0 samples.
    if (use_internal_buffer) {
      outsamples = NeAACDecDecode(handle,&frame,framedata,framesize);
      }
    else {
      void * outbuffer = out->ptr();
      NeAACDecDecode2(handle,&frame,framedata,framesize,&outbuffer,out->availableFrames()*out->af.framesize());
      if (frame.error == 27) {
        GM_DEBUG_PRINT("[aac] using faad internal buffer (%d)\n",out->availableFrames()*out->af.framesize());
        use_internal_buffer=true;
        outsamples = NeAACDecDecode(handle,&frame,framedata,framesize);
        }
      }

    if (stream_position == 0)
      GM_DEBUG_PRINT("[aac] nframes %u\n",frame.samples/af.channels);

    if (frame.error > 0) {
      GM_DEBUG_PRINT("[aac] fatal decoder error %hhu: %s\n",frame.error,faacDecGetErrorMessage(frame.error));
      return false;
      }

    if (frame.samplerate!=af.rate) {
      GM_DEBUG_PRINT("[aac] mismatch samplerate\n");
      af.rate = frame.samplerate;
      if (out) out->af = af;
      context->post_configuration(new ConfigureEvent(af,Codec::AAC));
      }

    if (frame.samples==0)
      continue;

    if (process_output(stream_id, stream_length, outsamples, frame.samples))
      return true;
    }

  if (eos) {
    FXASSERT(stream_position==stream_length);
    GM_DEBUG_PRINT("stream_position %ld == stream_length %ld\n",stream_position-stream_offset_start,stream_length-stream_offset_start);
    context->post_output_packet(out,true);
    }
  return true;
  }

FXbool AacDecoder::process(Packet*packet){
  if (__unlikely(rawmode==true))
    return process_raw(packet);
  else
    return process_frames(packet);
  }

DecoderPlugin * ap_aac_decoder(DecoderContext * ctx) {
  return new AacDecoder(ctx);
  }

#endif
}


