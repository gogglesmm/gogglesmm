/*******************************************************************************
*                         Goggles Audio Player Library                         *
********************************************************************************
*           Copyright (C) 2010-2014 by Sander Jansen. All Rights Reserved      *
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
#include "ap_event.h"
#include "ap_pipe.h"
#include "ap_event_queue.h"
#include "ap_thread_queue.h"
#include "ap_format.h"
#include "ap_buffer.h"
#include "ap_packet.h"
#include "ap_engine.h"
#include "ap_thread.h"
#include "ap_reader_plugin.h"
#include "ap_input_thread.h"
#include "ap_input_plugin.h"
#include "ap_decoder_plugin.h"
#include "ap_decoder_thread.h"

using namespace ap;

namespace ap {

ReaderPlugin::ReaderPlugin(AudioEngine *e) : engine(e),input(NULL),flags(0),stream_length(-1) {
  }

ReaderPlugin::~ReaderPlugin() {
  }

FXlong ReaderPlugin::seek_offset(FXdouble value) const {
  if (stream_length>0)
    return stream_length*value;
  else
    return -1;
  }

FXbool ReaderPlugin::init(InputPlugin* plugin) {
  input=plugin;
  stream_length=-1;
  return true;
  }

ReadStatus ReaderPlugin::process(Packet*packet) {
  FXint nread = input->read(packet->ptr(),packet->space());
  if (nread<0) {
    packet->unref();
    return ReadError;
    }
  else if (nread==0) {
    packet->af=af;
    packet->wroteBytes(nread);
    packet->stream_position  = -1;
    packet->stream_length    = 0;
    packet->flags = FLAG_EOS;
    engine->decoder->post(packet);
    return ReadDone;
    }
  else {
    packet->af=af;
    packet->wroteBytes(nread);
    packet->flags = 0;
    packet->stream_position = -1;
    packet->stream_length   = 0;
    engine->decoder->post(packet);
    return ReadOk;
    }
  return ReadError;
  }



TextReader::TextReader(AudioEngine*e) : ReaderPlugin(e) {
  }

TextReader::~TextReader(){
  }

FXbool TextReader::init(InputPlugin * plugin) {
  ReaderPlugin::init(plugin);
  textbuffer.clear();
  return true;
  }

ReadStatus TextReader::process(Packet*packet) {
  packet->unref();
  GM_DEBUG_PRINT("[text] starting read %ld\n",input->size());
  if (input->size()>0) {
    textbuffer.length(input->size());
    if (input->read(textbuffer.text(),input->size())!=input->size())
      return ReadError;
    }
  else {
    FXint len=0,nread=0;
    const FXint chunk=4096;
    do {
      len+=nread;
      textbuffer.length(textbuffer.length()+chunk);
      nread=input->read(&textbuffer[len],chunk);
      }
    while(nread>0);
    textbuffer.trunc(len);
    if (nread==-1)
      return ReadError;
    }
  return ReadDone;
  }




}

#include "ap_config.h"

namespace ap {


extern ReaderPlugin * ap_m3u_reader(AudioEngine*);
extern ReaderPlugin * ap_pls_reader(AudioEngine*);
extern ReaderPlugin * ap_xspf_reader(AudioEngine*);
extern ReaderPlugin * ap_asx_reader(AudioEngine*);

extern ReaderPlugin * ap_wav_reader(AudioEngine*);

#ifdef HAVE_FLAC_PLUGIN
extern ReaderPlugin * ap_flac_reader(AudioEngine*);
#endif

#ifdef HAVE_OGG_PLUGIN
extern ReaderPlugin * ap_ogg_reader(AudioEngine*);
#endif

#ifdef HAVE_MUSEPACK_PLUGIN
extern ReaderPlugin * ap_musepack_reader(AudioEngine*);
#endif

#ifdef HAVE_MAD_PLUGIN
extern ReaderPlugin * ap_mad_reader(AudioEngine*);
#endif

#ifdef HAVE_AAC_PLUGIN
extern ReaderPlugin * ap_aac_reader(AudioEngine*);
#endif

#ifdef HAVE_MP4_PLUGIN
extern ReaderPlugin * ap_mp4_reader(AudioEngine*);
#endif

#ifdef HAVE_CDDA_PLUGIN
extern ReaderPlugin * ap_cdda_reader(AudioEngine*);
#endif

#ifdef HAVE_AVC_PLUGIN
extern ReaderPlugin * ap_asf_reader(AudioEngine*);
extern ReaderPlugin * ap_avf_reader(AudioEngine*);
extern ReaderPlugin * ap_asx_reader(AudioEngine*);
#endif

#ifdef HAVE_WAVPACK_PLUGIN
extern ReaderPlugin * ap_wavpack_reader(AudioEngine*);
#endif




ReaderPlugin* ReaderPlugin::open(AudioEngine * engine,FXuint type) {
  switch(type){
    case Format::WAV      : return ap_wav_reader(engine); break;
#ifdef HAVE_OGG_PLUGIN
    case Format::OGG      : return ap_ogg_reader(engine); break;
#endif
#ifdef HAVE_FLAC_PLUGIN
    case Format::FLAC     : return ap_flac_reader(engine); break;
#endif
#ifdef HAVE_MAD_PLUGIN
    case Format::MP3      : return ap_mad_reader(engine); break;
#endif
#ifdef HAVE_AAC_PLUGIN
    case Format::AAC      : return ap_aac_reader(engine); break;
#endif
#ifdef HAVE_MP4_PLUGIN
    case Format::MP4      : return ap_mp4_reader(engine); break;
#endif
#ifdef HAVE_MUSEPACK_PLUGIN
    case Format::Musepack : return ap_musepack_reader(engine); break;
#endif
#ifdef HAVE_WAVPACK_PLUGIN
    case Format::WavPack  : return ap_wavpack_reader(engine); break;
#endif
#ifdef HAVE_CDDA_PLUGIN
    case Format::CDDA     : return ap_cdda_reader(engine); break;
#endif
    case Format::M3U      : return ap_m3u_reader(engine); break;
    case Format::PLS      : return ap_pls_reader(engine); break;
    case Format::XSPF     : return ap_xspf_reader(engine); break;

#ifdef HAVE_AVC_PLUGIN
    case Format::ASX      : return ap_asx_reader(engine); break;
    case Format::ASF      : return ap_asf_reader(engine); break;
    case Format::ASFX     : {

    FXchar buffer[1024];
    FXival nbuffer=0;
    nbuffer=engine->input->preview(buffer,1024);
    if (nbuffer>0) {
//      fxmessage("got preview buffer of %d\n",nbuffer);
//      fxmessage("%s\n",buffer);
      if (comparecase(buffer,"<ASX",4)==0)
        return ap_asx_reader(engine);
      else
        return ap_asf_reader(engine);
      }

                            }

#endif







//    case Format::MP3      : return ap_avf_reader(engine); break;
    default               : return NULL; break;
    }
  return NULL;
  }
}




















