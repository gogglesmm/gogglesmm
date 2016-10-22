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
#include "ap_event_private.h"
#include "ap_engine.h"
#include "ap_reader_plugin.h"
#include "ap_input_plugin.h"
#include "ap_decoder_plugin.h"
#include "ap_decoder_thread.h"

#include "neaacdec.h"

namespace ap {

#ifdef HAVE_FAAD

class AACReader : public ReaderPlugin {
public:
  AACReader(AudioEngine*e) : ReaderPlugin(e) {}
  FXbool init(InputPlugin*plugin) override { ReaderPlugin::init(plugin); flags=0; return true; }
  FXuchar format() const override { return Format::AAC; }

  ReadStatus process(Packet*p) override;

  ~AACReader() {}
  };

ReaderPlugin * ap_aac_reader(AudioEngine * engine) {
  return new AACReader(engine);
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
        engine->decoder->post(new ConfigureEvent(af,Codec::AAC));
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
  FXlong         stream_position;
protected:
  Packet * out;
public:
  AacDecoder(AudioEngine*);
  FXuchar codec() const override { return Codec::AAC; }
  FXbool flush(FXlong offset=0) override;
  FXbool init(ConfigureEvent*) override ;
  DecoderStatus process(Packet*) override;
  ~AacDecoder();
  };



AacDecoder::AacDecoder(AudioEngine * e) : DecoderPlugin(e),handle(nullptr),stream_position(-1),out(nullptr) {
  }

AacDecoder::~AacDecoder() {
  flush();
  if (handle) {
    NeAACDecClose(handle);
    handle=nullptr;
    }
  }

FXbool AacDecoder::init(ConfigureEvent*event) {
  DecoderPlugin::init(event);
  buffer.clear();
  af=event->af;
  if (handle) {
    NeAACDecClose(handle);
    handle=nullptr;
    }

  DecoderSpecificConfig * ac = dynamic_cast<DecoderSpecificConfig*>(event->dc);
  if (ac) {
    long unsigned int samplerate;
    FXuchar           channels;
    handle = NeAACDecOpen();
    if (NeAACDecInit2(handle,ac->config,ac->config_bytes,&samplerate,&channels)<0){
      NeAACDecClose(handle);
      handle=nullptr;
      return false;
      }
    }
  stream_position=-1;
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
  if (handle) NeAACDecPostSeekReset(handle,-1);
  return true;
  }

DecoderStatus AacDecoder::process(Packet*packet){
  const FXbool eos = packet->flags&FLAG_EOS;
  const FXlong stream_length = packet->stream_length;
  const FXuint stream_id = packet->stream;

  if (stream_position==-1 && buffer.size()==0) {
    stream_position = packet->stream_position;
    }

  long unsigned int samplerate;
  FXuchar           channels;
  NeAACDecFrameInfo frame;

  buffer.append(packet->data(),packet->size());
  packet->unref();
  packet=nullptr;
  if (handle==nullptr) {
    handle = NeAACDecOpen();
    long n = NeAACDecInit(handle,buffer.data(),buffer.size(),&samplerate,&channels);
    if (n<0) {
      buffer.clear();
      return DecoderError;
      }
    else if (n>0) buffer.readBytes(n);
    af.set(AP_FORMAT_S16,samplerate,channels);
    engine->output->post(new ConfigureEvent(af,Codec::AAC));
    stream_position=0;
    }

  const FXuint bytes_needed = FAAD_MIN_STREAMSIZE*af.channels;

  if (buffer.size()<bytes_needed && eos==false) {
    return DecoderOk;
    }

  FXlong stream_begin = stream_decode_offset;
  do {

    if (out==nullptr){
      out = engine->decoder->get_output_packet();
      if (out==nullptr) return DecoderInterrupted;
      out->af              = af;
      out->stream          = stream_id;
      out->stream_position = stream_position;
      out->stream_length   = stream_length;
      }

    void * outbuffer = out->ptr();

    // Looks like the decoder already takes care of stripping any encoder delay. So first call will return 0 samples.
    NeAACDecDecode2(handle,&frame,buffer.data(),buffer.size(),&outbuffer,out->availableFrames()*out->af.framesize());
    if (frame.bytesconsumed>0) {
      buffer.readBytes(frame.bytesconsumed);
      }

    if (frame.error > 0) {
      GM_DEBUG_PRINT("[aac] error %d (%ld): %s\n",frame.error,frame.bytesconsumed,faacDecGetErrorMessage(frame.error));
      return DecoderError;
      }


    if (frame.samples>0) {

      FXint nframes = frame.samples / frame.channels;

      if (stream_length>0)
        nframes = FXMIN((FXlong)nframes,(stream_length-stream_position));

      if (__unlikely(stream_position<stream_begin)) {
        if ((nframes+stream_position)<stream_begin) {
          GM_DEBUG_PRINT("[aac] stream decode offset %ld. Full skip %d\n",stream_begin,nframes);
          stream_position+=nframes;
          }
        else {
          GM_DEBUG_PRINT("[aac] stream decode offset %ld. Partial skip %ld\n",stream_begin,(stream_begin-stream_position));
          out->wroteFrames(nframes);
          out->trimBegin(af.framesize()*(stream_begin-stream_position));
          out->stream_position = stream_begin;
          stream_position+=nframes;
          }
        }
      else {
        stream_position+=nframes;
        out->wroteFrames(nframes);
        }

      if (out->availableFrames()<1024) {
        engine->output->post(out);
        out=nullptr;
        }
      }
    }
  while(((buffer.size()>=bytes_needed) || eos) && frame.bytesconsumed);

  if (eos) {
    FXASSERT(stream_position==stream_length);
    if (out) {
      engine->output->post(out);
      out=nullptr;
      }
    engine->output->post(new ControlEvent(End,stream_id));
    }

  return DecoderOk;
  }




DecoderPlugin * ap_aac_decoder(AudioEngine * engine) {
  return new AacDecoder(engine);
  }

#endif
}


