/*******************************************************************************
*                         Goggles Audio Player Library                         *
********************************************************************************
*           Copyright (C) 2010-2012 by Sander Jansen. All Rights Reserved      *
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

#include <ogg/ogg.h>
#include <vorbis/codec.h>

namespace ap {

class AudioEngine;



struct OggReaderState {
  FXbool has_stream;
  FXbool has_eos;
  FXbool has_page;
  FXbool has_packet;
  FXbool header_written;
  FXuint bytes_written;
  OggReaderState() : has_stream(false),has_eos(false),has_page(false),has_packet(false),header_written(false),bytes_written(0) {}
  void reset() {has_stream=false;has_eos=false;has_page=false; has_packet=false; header_written=false;bytes_written=0; }
  };

class OggReader : public ReaderPlugin {
protected:
  enum {
    FLAG_VORBIS_HEADER_INFO    = 0x2,
    FLAG_VORBIS_HEADER_COMMENT = 0x4,
    FLAG_VORBIS_HEADER_BLOCK   = 0x8,
    FLAG_OGG_FLAC              = 0x10,
    };
protected:
  OggReaderState    state;
  ogg_stream_state stream;
  ogg_sync_state   sync;
  ogg_page         page;
  ogg_packet       op;
protected:
  vorbis_info      vi;
  vorbis_comment   vc;
protected:
  Packet *        packet;
  Event  *        headers;
  FXint           ogg_packet_written;
  ReadStatus      status;
  FXuchar         codec;
  FXlong          stream_start;
  FXlong          input_position;
protected:
  FXbool match_page();
  FXbool fetch_next_page();
  FXbool fetch_next_packet();
  void   submit_ogg_packet(FXbool post=true);
  void   check_vorbis_length(vorbis_info*);
  void   add_header(Packet * p);
  void   send_headers();
  void   clear_headers();

  ReadStatus parse();
  ReadStatus parse_vorbis_stream();
#ifdef HAVE_FLAC_PLUGIN
  ReadStatus parse_flac_stream();
#endif
public:
  OggReader(AudioEngine *);
  FXuchar format() const { return Format::OGG; };
  FXbool init(InputPlugin*);
  FXbool seek(FXdouble);
  FXbool can_seek() const;
  ReadStatus process(Packet*);
  virtual ~OggReader();
  };















OggReader::OggReader(AudioEngine * e) : ReaderPlugin(e),packet(NULL),headers(NULL),ogg_packet_written(-1),codec(Codec::Invalid) {
  ogg_sync_init(&sync);
  }


OggReader::~OggReader() {
  clear_headers();
  ogg_sync_clear(&sync);
  }

FXbool OggReader::can_seek() const {
  return stream_length>0;
  }

FXbool OggReader::seek(FXdouble pos){
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
    FXlong offset  = input->size() * pos;
    FXlong lastpos = -1;

    input_position = input->position(offset,FXIO::Begin);

    GM_DEBUG_PRINT("target seek %ld / %ld => %ld\n",target,stream_length,offset);

    while(fetch_next_page()) {
      if (ogg_page_granulepos(&page)>target) {
        GM_DEBUG_PRINT("found %ld %ld %ld\n",ogg_page_granulepos(&page),lastpos,offset);
        if (lastpos>=0 || offset==0) {
          input->position((lastpos>=0) ? lastpos : offset,FXIO::Begin);
          ogg_sync_reset(&sync);
          ogg_stream_reset(&stream);
          return true;
          }
        else {
          offset=FXMIN(0,offset>>1);
          GM_DEBUG_PRINT("went to far. start at %ld\n",offset);
          input_position=input->position(offset,FXIO::Begin);
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
      GM_DEBUG_PRINT("seeking beyond end of stream\n");
      }
    else {
      GM_DEBUG_PRINT("seeking to %ld\n",lastpos);
      input->position(lastpos,FXIO::Begin);
      ogg_sync_reset(&sync);
      ogg_stream_reset(&stream);
      }
    return true;
  }


FXbool OggReader::init(InputPlugin*plugin) {
  ReaderPlugin::init(plugin);
  ogg_sync_clear(&sync);
  ogg_sync_reset(&sync);
  flags&=~(FLAG_PARSED|FLAG_OGG_FLAC|FLAG_VORBIS_HEADER_INFO|FLAG_VORBIS_HEADER_COMMENT|FLAG_VORBIS_HEADER_BLOCK);
  status=ReadOk;

  clear_headers();

  input_position=-1;
  stream_length=0;
  stream_start=0;

  if (state.has_stream) {
    ogg_stream_clear(&stream);
    state.has_stream=false;
    }
  return true;
  }



#define BUFFERSIZE 32768

FXbool OggReader::match_page() {
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

void OggReader::check_vorbis_length(vorbis_info * info) {
  stream_length=0;
  stream_start=0;
  if (!input->serial()) {
    FXlong cpos = input_position;
    FXlong size = input->size();

    /// First Determine the pcm offset at the start of the stream
    FXint cb,lb=-1,tb=0;
    while(fetch_next_packet()) {
      cb=vorbis_packet_blocksize(info,&op);
      if (lb!=-1) tb+=(lb+cb)>>2;
      lb=cb;
      if (op.granulepos!=-1) {
        stream_start=op.granulepos-tb;
        GM_DEBUG_PRINT("stream offset=%ld %d %ld\n",op.granulepos,tb,stream_start);
        break;
        }
      }

    /// TODO need a smart way of finding the last page in stream.
    if (size>=0xFFFF) {
      input->position((size-0xFFFF),FXIO::Begin);
      ogg_sync_reset(&sync);
      }

    /// Go to last page of stream to find out last pcm position
    while(fetch_next_page()) {
      if (ogg_page_eos(&page)) {
        stream_length = ogg_page_granulepos(&page) - stream_start;
        GM_DEBUG_PRINT("found total %ld frames\n",stream_length);
        break;
        }
      }
    input->position(cpos,FXIO::Begin);
    ogg_sync_reset(&sync);
    ogg_stream_reset(&stream);
    }
  }

extern void ap_replaygain_from_vorbis_comment(ReplayGain & gain,const FXchar * comment,FXint len);
extern void ap_meta_from_vorbis_comment(MetaInfo * meta, const FXchar * comment,FXint len);


ReadStatus OggReader::parse_vorbis_stream() {
  if (op.packet[0]==1) {

    vorbis_info_init(&vi);
    vorbis_comment_init(&vc);

    if (vorbis_synthesis_headerin(&vi,&vc,&op)<0)
      goto error;

    codec=Codec::Vorbis;
    af.set(AP_FORMAT_FLOAT,vi.rate,vi.channels);

    flags|=FLAG_VORBIS_HEADER_INFO;
    submit_ogg_packet(false);
    }
  else if (op.packet[0]==3) {

    if (!(flags&FLAG_VORBIS_HEADER_INFO))
      goto error;

    if (vorbis_synthesis_headerin(&vi,&vc,&op)<0)
      goto error;

    flags|=FLAG_VORBIS_HEADER_COMMENT;
    submit_ogg_packet(false);
    }
  else if (op.packet[0]==5) {

    if (!(flags&(FLAG_VORBIS_HEADER_INFO|FLAG_VORBIS_HEADER_COMMENT)))
      goto error;

    if (vorbis_synthesis_headerin(&vi,&vc,&op)<0)
      goto error;

    flags|=(FLAG_VORBIS_HEADER_BLOCK|FLAG_PARSED);


    ConfigureEvent * config = new ConfigureEvent(af,codec);

    /// Get Replay Gain
    for (int i=0;i<vc.comments;i++) {
      ap_replaygain_from_vorbis_comment(config->replaygain,vc.user_comments[i],vc.comment_lengths[i]);
      }

    /// Meta Info
    MetaInfo * meta = new MetaInfo;
    for (int i=0;i<vc.comments;i++){
      ap_meta_from_vorbis_comment(meta,vc.user_comments[i],vc.comment_lengths[i]);
      }

    /// Now we are ready to init the decoder
    engine->decoder->post(config);

    //// Send Meta Info
    engine->decoder->post(meta);

    /// Add last packet
    submit_ogg_packet(false);

    /// Send all headers
    send_headers();

    /// Check length...
    check_vorbis_length(&vi);

    /// Success
    vorbis_info_clear(&vi);
    vorbis_comment_clear(&vc);
    }
  else {
    goto error;
    }
  return ReadOk;
error:
  vorbis_info_clear(&vi);
  vorbis_comment_clear(&vc);
  return ReadError;
  }

#ifdef HAVE_FLAC_PLUGIN

extern void flac_parse_vorbiscomment(const FXchar * buffer,FXint len,ReplayGain & gain,MetaInfo * meta);
extern FXbool flac_parse_streaminfo(const FXuchar * buffer,AudioFormat & config,FXlong & nframes);

ReadStatus OggReader::parse_flac_stream() {
  if (flags&FLAG_OGG_FLAC)  {

    codec=Codec::FLAC;

    ConfigureEvent * config = new ConfigureEvent(af,codec,stream_length);
    MetaInfo * meta         = new MetaInfo;

    flac_parse_vorbiscomment((const FXchar*)op.packet,op.bytes,config->replaygain,meta);

    /// Now we are ready to init the decoder
    engine->decoder->post(config);

    //// Send Meta Info
    engine->decoder->post(meta);

    /// Add last packet
    submit_ogg_packet(false);

    /// Send all headers
    send_headers();

    flags|=FLAG_PARSED;
    }
  else {
    /// Make sure we have enough bytes
    if (op.bytes<51)
      return ReadError;

    /// Check Mapping Version
    if (op.packet[5]!=0x01 || op.packet[6]!=0x00)
      return ReadError;

    /// Parse the stream info block.
    /// FIXME stream_length may not be set.
    if (!flac_parse_streaminfo(op.packet+13,af,stream_length))
      return ReadError;

    flags|=FLAG_OGG_FLAC;
    submit_ogg_packet(false);
    }
  /// Success
  return ReadOk;
  }
#endif

ReadStatus OggReader::parse() {
  if (input_position==-1)
    input_position = input->position();

  while(packet) {

    if (!fetch_next_packet())
      return ReadError;

    if (compare((const FXchar*)&op.packet[1],"vorbis",6)==0){
      if (parse_vorbis_stream()!=ReadOk)
        return ReadError;
      }
#ifdef HAVE_FLAC_PLUGIN
    else if ((flags&FLAG_OGG_FLAC) || compare((const FXchar*)&op.packet[1],"FLAC",4)==0) {
      if (parse_flac_stream()!=ReadOk)
        return ReadError;
      }
#endif
    else {
      return ReadError;
      }

    if (flags&FLAG_PARSED)
      return ReadOk;
    }
  return ReadOk;
  }




void OggReader::add_header(Packet * p) {
  Event * h = headers;
  p->next = NULL;
  if (h) {
    while(h->next) h=h->next;
    h->next = p;
    }
  else {
    headers = p;
    }
  }

void OggReader::send_headers() {
  while(headers) {
    Packet * p = dynamic_cast<Packet*>(headers);
    headers    = headers->next;
    p->next    = NULL;
    engine->decoder->post(p);
    }
  }

void OggReader::clear_headers() {
  while(headers) {
    Event * p = headers;
    headers = headers->next;
    p->unref();
    }
  }


void OggReader::submit_ogg_packet(FXbool post) {
  FXASSERT(packet);
  FXASSERT((FXuval)packet->capacity()>sizeof(ogg_packet));

  state.has_packet=true;

  if (state.header_written==false) {
    if (codec==Codec::Vorbis) {
      if (packet->space()>(FXival)sizeof(ogg_packet)) {
        packet->append(&op,sizeof(ogg_packet));
        state.header_written=true;
        if (packet->stream_position==-1) {
          packet->stream_position=op.granulepos;
          }
        }
      else {
        packet->af=af;
        if (post)
          engine->decoder->post(packet);
        else
          add_header(packet);
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
    if (post)
      engine->decoder->post(packet);
    else
      add_header(packet);
    packet=NULL;
    }
  }






FXbool OggReader::fetch_next_page() {
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
      FXchar * buffer = ogg_sync_buffer(&sync,BUFFERSIZE);
      if (buffer==NULL) return false;
      FXival nbytes = input->read(buffer,BUFFERSIZE);
      if (nbytes<=0) return false;
      ogg_sync_wrote(&sync,nbytes);
      }
    else { // skipped some bytes
      input_position+=(-result);
      }
    }
  return false;
  }


FXbool OggReader::fetch_next_packet() {
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


ReadStatus OggReader::process(Packet * p) {
  packet                  = p;
  packet->stream_position = -1;
  packet->stream_length   = stream_length;

  if (__unlikely((flags&FLAG_PARSED)==0)) {

    if (state.has_packet)
      submit_ogg_packet(false);

    if (parse()!=ReadOk)
      return ReadError;

    if (__unlikely((flags&FLAG_PARSED)==0))
      return ReadOk;

    }


  if (state.has_packet)
    submit_ogg_packet();

  while(packet && fetch_next_packet()) {
    submit_ogg_packet();
    }

  if (state.has_eos)
    return ReadDone;
  else
    return ReadOk;
  }


ReaderPlugin * ap_ogg_reader(AudioEngine * engine) {
  return new OggReader(engine);
  }


}









