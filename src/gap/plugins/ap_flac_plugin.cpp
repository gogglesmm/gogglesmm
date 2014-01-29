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


#if FOX_BIGENDIAN == 0
#define FLAC_LAST_BLOCK       0x80
#define FLAC_BLOCK_TYPE_MASK  0x7f

#define FLAC_BLOCK_TYPE(h) (h&0x7f)
#define FLAC_BLOCK_SIZE(h) ( ((h&0xFF00)<<8) | ((h&0xFF0000)>>8) | ((h&0xFF000000)>>24) )

#define FLAC_BLOCK_SET_TYPE(h,type) (h|=(type&FLAC_BLOCK_TYPE_MASK))
#define FLAC_BLOCK_SET_SIZE(h,size) (h|=(((size&0xFF)<<24) | ((size&0xFF0000)>>16) | ((size&0xFF00)<<8)))

#define FLAC_INFO_MIN_BLOCK_SIZE(x)   (INT16_BE(x+0))
#define FLAC_INFO_MAX_BLOCK_SIZE(x)   (INT16_BE(x+2))
#define FLAC_INFO_MIN_FRAME_SIZE(x)   (INT24_BE(x+4))
#define FLAC_INFO_MAX_FRAME_SIZE(x)   (INT24_BE(x+7))
#define FLAC_INFO_SAMPLE_RATE(x)     ( ((*(x+10))<<12) | ((*(x+11))<<4) | (((*(x+12))>>4)&0xF) )
#define FLAC_INFO_CHANNELS(x)  (1 + (((*(x+12))>>1)&0x7))
#define FLAC_INFO_BPS(x)       (1 + ((((*(x+12))&0x1)<<4) | (((*(x+13))>>4)&0xF) ))
#define FLAC_INFO_NSAMPLES(x)  ( (((FXlong)(*(x+13))&0xF)<<32) | ((*(x+14))<<24) | ((*(x+15))<<16) | ((*(x+16))<<8) | (*(x+17) ) )

#else
#error "BUG: FLAC  macros not defined for Big Endian Architecture"
#endif

#include <FLAC/stream_decoder.h>

namespace ap {


class FlacReader : public ReaderPlugin {
protected:
  FLAC__StreamDecoder * flac;
  ReplayGain            gain;
  MetaInfo            * meta;
  FXlong                lastseek;
protected:
  static FLAC__StreamDecoderSeekStatus    flac_input_seek(const FLAC__StreamDecoder*,FLAC__uint64,void*);
  static FLAC__StreamDecoderTellStatus    flac_input_tell(const FLAC__StreamDecoder*,FLAC__uint64*,void*);
  static FLAC__StreamDecoderLengthStatus  flac_input_length(const FLAC__StreamDecoder*,FLAC__uint64*,void*);
  static FLAC__StreamDecoderWriteStatus   flac_input_write(const FLAC__StreamDecoder*,const FLAC__Frame*,const FLAC__int32*const[],void*);
  static FLAC__StreamDecoderReadStatus    flac_input_read(const FLAC__StreamDecoder*,FLAC__byte buffer[],size_t*,void*);
  static FLAC__bool                       flac_input_eof(const FLAC__StreamDecoder*,void*);
  static void                             flac_input_meta(const FLAC__StreamDecoder*,const FLAC__StreamMetadata*,void*);
  static void                             flac_input_error(const FLAC__StreamDecoder *, FLAC__StreamDecoderErrorStatus, void *);
protected:
  ReadStatus parse();
public:
  FlacReader(AudioEngine*);
  FXuchar format() const { return Format::FLAC; };
  FXbool init(InputPlugin*);
  FXbool seek(FXdouble);
  FXbool can_seek() const;
  ReadStatus process(Packet*);
  ~FlacReader();
  };

class OutputPacket;

class FlacDecoder : public DecoderPlugin {
protected:
  FLAC__StreamDecoder * flac;
  FXint stream_length;
protected:
  Packet * in;
  Packet  * out;
protected:
  static FLAC__StreamDecoderWriteStatus   flac_decoder_write(const FLAC__StreamDecoder*,const FLAC__Frame*,const FLAC__int32*const[],void*);
  static FLAC__StreamDecoderReadStatus    flac_decoder_read(const FLAC__StreamDecoder*,FLAC__byte buffer[],size_t*,void*);
  static void                             flac_decoder_error(const FLAC__StreamDecoder *, FLAC__StreamDecoderErrorStatus, void *);
public:
  FlacDecoder(AudioEngine*);
  FXuchar codec() const { return Codec::FLAC; }
  FXbool flush();
  FXbool init(ConfigureEvent*);
  DecoderStatus process(Packet*);
  ~FlacDecoder();
  };








extern void ap_replaygain_from_vorbis_comment(ReplayGain & gain,const FXchar * comment,FXint len);
extern void ap_meta_from_vorbis_comment(MetaInfo * meta, const FXchar * comment,FXint len);
extern void ap_parse_vorbiscomment(const FXchar * buffer,FXint len,ReplayGain & gain,MetaInfo * meta);

enum {
  FLAC_BLOCK_STREAMINFO     = 0,
  FLAC_BLOCK_VORBISCOMMENT  = 4
  };


void flac_parse_vorbiscomment(const FXchar * buffer,FXint len,ReplayGain & gain,MetaInfo * meta) {
  FXString comment;
  const FXchar * end = buffer+len;

  FXuint header=((const FXuint*)buffer)[0];
  if (FLAC_BLOCK_TYPE(header)!=FLAC_BLOCK_VORBISCOMMENT)
    return;

  /// skip the metaheader block
  buffer+=4;
  if (buffer>=end) return;

  ap_parse_vorbiscomment(buffer,len-4,gain,meta);
  }


FXbool flac_parse_streaminfo(const FXuchar * buffer,AudioFormat & af,FXlong & nframes) {
  FXuint header=((const FXuint*)buffer)[0];

  if (FLAC_BLOCK_TYPE(header)!=FLAC_BLOCK_STREAMINFO || FLAC_BLOCK_SIZE(header)!=34)
    return false;

  const FXuchar * const info = buffer + 4;

//  FXushort min_block_size;
//  FXushort max_block_size;
//  FXuint   min_frame_size;
//  FXuint   max_frame_size;
  FXuint   sample_rate;
  FXchar   channels;
  FXchar   bps;

//  min_block_size = FLAC_INFO_MIN_BLOCK_SIZE(info);
//  max_block_size = FLAC_INFO_MAX_BLOCK_SIZE(info);
//  min_frame_size = FLAC_INFO_MIN_FRAME_SIZE(info);
//  max_frame_size = FLAC_INFO_MAX_FRAME_SIZE(info);
  sample_rate    = FLAC_INFO_SAMPLE_RATE(info);
  channels       = FLAC_INFO_CHANNELS(info);
  bps            = FLAC_INFO_BPS(info);
  nframes        = FLAC_INFO_NSAMPLES(info);

  af.set(Format::Signed|Format::Little,bps,bps>>3,sample_rate,channels);
  af.debug();
  return true;
  }




FlacReader::FlacReader(AudioEngine* e) : ReaderPlugin(e), flac(NULL),meta(NULL) {
  }

FlacReader::~FlacReader(){
  if (flac) {
    FLAC__stream_decoder_finish(flac);
    FLAC__stream_decoder_delete(flac);
    flac = NULL;
    }
  }

FXbool FlacReader::init(InputPlugin*plugin) {
  ReaderPlugin::init(plugin);
  if (flac == NULL) {

    flac =  FLAC__stream_decoder_new();
    if ( flac == NULL)
      return false;


    FLAC__stream_decoder_set_metadata_respond(flac,FLAC__METADATA_TYPE_VORBIS_COMMENT);

    /// Init Stream
    if (FLAC__stream_decoder_init_stream(flac,flac_input_read,
                                              flac_input_seek,
                                              flac_input_tell,
                                              flac_input_length,
                                              flac_input_eof,
                                              flac_input_write,
                                              flac_input_meta,
                                              flac_input_error,
                                               this)!=FLAC__STREAM_DECODER_INIT_STATUS_OK){
      FXASSERT(0);
      return false;
      }
    }
  gain.reset();
  if (meta) {
    meta->unref();
    meta=NULL;
    }
  flags&=~FLAG_PARSED;
  return true;
  }

FXbool FlacReader::can_seek() const {
  return stream_length>0;
  }

FXbool FlacReader::seek(FXdouble pos){
  FXASSERT(stream_length>0);
  FXlong offset = (FXlong)(((FXdouble)stream_length)*pos);
  GM_DEBUG_PRINT("[flac_reader] seek to %g %ld / %ld\n",pos,offset,stream_length);
  FLAC__stream_decoder_flush(flac);
  if (FLAC__stream_decoder_seek_absolute(flac,offset)) {
    input->position(lastseek,FXIO::Begin);
    return true;
    }
  return false;
  }


ReadStatus FlacReader::process(Packet*p) {
  if (!(flags&FLAG_PARSED)) {
    ReadStatus result = parse();
    if (result!=ReadOk) {
      p->unref();
      return result;
      }
    }
  return ReaderPlugin::process(p);
  }



ReadStatus FlacReader::parse() {
  FXASSERT(flac);
  FLAC__uint64 pos;
  stream_length=0;

  if (FLAC__stream_decoder_process_until_end_of_metadata(flac)){

    if (FLAC__stream_decoder_get_decode_position(flac,&pos))
      input->position(pos,FXIO::Begin);
    else
      input->position(0,FXIO::Begin);


    ConfigureEvent * config = new ConfigureEvent(af,Codec::FLAC,stream_length);
    config->replaygain=gain;
    engine->decoder->post(config);
    flags|=FLAG_PARSED;

    if (meta) {
      engine->decoder->post(meta);
      meta=NULL;
      }
    return ReadOk;
    }

/*  switch(FLAC__stream_decoder_get_state(flac)){
    case FLAC__STREAM_DECODER_SEARCH_FOR_METADATA: fxmessage("FLAC__STREAM_DECODER_SEARCH_FOR_METADATA"); break;
    case FLAC__STREAM_DECODER_READ_METADATA: fxmessage("FLAC__STREAM_DECODER_READ_METADATA"); break;
    case FLAC__STREAM_DECODER_SEARCH_FOR_FRAME_SYNC: fxmessage("FLAC__STREAM_DECODER_SEARCH_FOR_FRAME_SYNC"); break;
    case FLAC__STREAM_DECODER_READ_FRAME: fxmessage("FLAC__STREAM_DECODER_READ_FRAME"); break;
    case FLAC__STREAM_DECODER_END_OF_STREAM: fxmessage("FLAC__STREAM_DECODER_END_OF_STREAM"); break;
    case FLAC__STREAM_DECODER_OGG_ERROR: fxmessage("FLAC__STREAM_DECODER_OGG_ERROR"); break;
    case FLAC__STREAM_DECODER_SEEK_ERROR: fxmessage("FLAC__STREAM_DECODER_SEEK_ERROR"); break;
    case FLAC__STREAM_DECODER_ABORTED: fxmessage("FLAC__STREAM_DECODER_ABORTED"); break;
    case FLAC__STREAM_DECODER_MEMORY_ALLOCATION_ERROR: fxmessage("FLAC__STREAM_DECODER_MEMORY_ALLOCATION_ERROR"); break;
    case FLAC__STREAM_DECODER_UNINITIALIZED: fxmessage("FLAC__STREAM_DECODER_UNINITIALIZED"); break;
    }
*/
  return ReadError;
  }


FLAC__StreamDecoderSeekStatus FlacReader::flac_input_seek(const FLAC__StreamDecoder */*decoder*/,FLAC__uint64 absolute_byte_offset, void *client_data){
  FlacReader * plugin = reinterpret_cast<FlacReader*>(client_data);
//  fxmessage("seek\n");
// FIXME
//  if (inputflac->input->io->isSerial())
//    return FLAC__STREAM_DECODER_SEEK_STATUS_UNSUPPORTED;
  FXlong pos = plugin->input->position(absolute_byte_offset,FXIO::Begin);
  plugin->lastseek = pos;

  if (pos<0 || ((FXulong)pos)!=absolute_byte_offset)
    return FLAC__STREAM_DECODER_SEEK_STATUS_ERROR;
  else
    return FLAC__STREAM_DECODER_SEEK_STATUS_OK;
  }


FLAC__StreamDecoderTellStatus FlacReader::flac_input_tell(const FLAC__StreamDecoder */*decoder*/, FLAC__uint64 *absolute_byte_offset, void *client_data){
  FlacReader * plugin = reinterpret_cast<FlacReader*>(client_data);
//  fxmessage("tell\n");
// FIXME
//  if (inputflac->input->io->isSerial())
//    return FLAC__STREAM_DECODER_TELL_STATUS_UNSUPPORTED;

  FXlong pos = plugin->input->position();
  if (pos<0)
    return FLAC__STREAM_DECODER_TELL_STATUS_ERROR;

  (*absolute_byte_offset)=pos;
  return FLAC__STREAM_DECODER_TELL_STATUS_OK;
  }


FLAC__StreamDecoderLengthStatus FlacReader::flac_input_length(const FLAC__StreamDecoder */*decoder*/, FLAC__uint64 *stream_length, void *client_data){
  FlacReader * plugin = reinterpret_cast<FlacReader*>(client_data);

///  if (plugin->engine->input->isSerial())
 //   return FLAC__STREAM_DECODER_LENGTH_STATUS_UNSUPPORTED;

  FXlong length = plugin->input->size();
  if (length<0)
    return FLAC__STREAM_DECODER_LENGTH_STATUS_ERROR;

  (*stream_length)=length;
  return FLAC__STREAM_DECODER_LENGTH_STATUS_OK;
  }

FLAC__bool FlacReader::flac_input_eof(const FLAC__StreamDecoder */*decoder*/, void *client_data){
  FlacReader * plugin = reinterpret_cast<FlacReader*>(client_data);
  return plugin->input->eof();
  }


FLAC__StreamDecoderWriteStatus FlacReader::flac_input_write(const FLAC__StreamDecoder */*decoder*/, const FLAC__Frame */*frame*/, const FLAC__int32 *const /*buffer*/[], void */*client_data*/) {
//  FXASSERT(0);
  return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;//FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
  }


FLAC__StreamDecoderReadStatus FlacReader::flac_input_read(const FLAC__StreamDecoder */*decoder*/, FLAC__byte buffer[], size_t *bytes, void *client_data) {
  FlacReader * plugin = reinterpret_cast<FlacReader*>(client_data);
  FXASSERT(bytes);

  if ((*bytes)<=0)
    return FLAC__STREAM_DECODER_READ_STATUS_ABORT;

  FXival nbytes = plugin->input->read(buffer,(*bytes));

  if (nbytes<0) {
    return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
    }
  else if (nbytes==0) {
    (*bytes)=nbytes;
    return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
    }
  else {
    (*bytes)=nbytes;
    return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
    }
  }



void FlacReader::flac_input_meta(const FLAC__StreamDecoder */*decoder*/, const FLAC__StreamMetadata *metadata, void *client_data) {
  FlacReader * plugin = reinterpret_cast<FlacReader*>(client_data);
  switch(metadata->type) {
    case FLAC__METADATA_TYPE_STREAMINFO :
      plugin->af.set(Format::Signed|Format::Little,metadata->data.stream_info.bits_per_sample,
                                                   metadata->data.stream_info.bits_per_sample>> 3,
                                                   metadata->data.stream_info.sample_rate,
                                                   metadata->data.stream_info.channels);

      plugin->stream_length=metadata->data.stream_info.total_samples;
      break;
    case FLAC__METADATA_TYPE_VORBIS_COMMENT:
      for (FXuint i=0;i<metadata->data.vorbis_comment.num_comments;i++) {
        ap_replaygain_from_vorbis_comment(plugin->gain,(const FXchar*)metadata->data.vorbis_comment.comments[i].entry,metadata->data.vorbis_comment.comments[i].length);
        }

      plugin->meta = new MetaInfo;
      for (FXuint i=0;i<metadata->data.vorbis_comment.num_comments;i++) {
        ap_meta_from_vorbis_comment(plugin->meta,(const FXchar*)metadata->data.vorbis_comment.comments[i].entry,metadata->data.vorbis_comment.comments[i].length);
        }
      break;
    default: break;
    }
 }

void FlacReader::flac_input_error(const FLAC__StreamDecoder */*decoder*/, FLAC__StreamDecoderErrorStatus status, void */*client_data*/) {
  //FlacReader * plugin = reinterpret_cast<FlacReader*>(client_data);
  switch(status) {
    case FLAC__STREAM_DECODER_ERROR_STATUS_LOST_SYNC          : fxmessage("flac_decoder_error: An error in the stream caused the decoder to lose synchronization."); break;
    case FLAC__STREAM_DECODER_ERROR_STATUS_BAD_HEADER         : fxmessage("flac_decoder_error: The decoder encountered a corrupted frame header."); break;
    case FLAC__STREAM_DECODER_ERROR_STATUS_FRAME_CRC_MISMATCH : fxmessage("flac_decoder_error: The frame's data did not match the CRC in the footer."); break;
    case FLAC__STREAM_DECODER_ERROR_STATUS_UNPARSEABLE_STREAM : fxmessage("flac_decoder_error: The decoder encountered reserved fields in use in the stream. "); break;
    }
  }




















FLAC__StreamDecoderWriteStatus FlacDecoder::flac_decoder_write(const FLAC__StreamDecoder */*decoder*/, const FLAC__Frame *frame, const FLAC__int32 *const buffer[], void *client_data) {
  FlacDecoder * plugin = reinterpret_cast<FlacDecoder*>(client_data);
  FXASSERT(frame);
  FXASSERT(buffer);
  FXint s,c,p=0;
  FXint sample  = 0;
  FXint nchannels = frame->header.channels;
  FXint framesize = (frame->header.bits_per_sample>>3)*nchannels;
  FXint navail = 0,ncopy;


  FXint nframes = frame->header.blocksize;

  FXASSERT(frame->header.number_type==FLAC__FRAME_NUMBER_TYPE_SAMPLE_NUMBER);

  if (nframes==0)
    return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;

  Packet * packet = plugin->out;
  if (packet) {
    navail = packet->availableFrames();
    if (packet->numFrames()==0)
      packet->stream_position=frame->header.number.sample_number;
    }

  while(nframes>0) {

    /// get a fresh packet
    if (!packet) {
      packet=plugin->engine->decoder->get_output_packet();
      if (packet==NULL) {
        return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
        }
      packet->af=plugin->af;
      packet->stream_position=frame->header.number.sample_number+(frame->header.blocksize-nframes);
      packet->stream_length=plugin->stream_length;
      navail = (packet->space()) / framesize;
      plugin->out=packet;
      }

    ncopy = FXMIN(nframes,navail);
    switch(frame->header.bits_per_sample) {
      case 8:
        {
          FXchar * buf8 = packet->s8();
          for (p=0,s=sample;s<(ncopy+sample);s++)
            for (c=0;c<nchannels;c++,p++)
              buf8[p]=buffer[c][s];
        }
        break;
      case 16:
        {
          FXshort * buf16 = packet->s16();
          for (p=0,s=sample;s<(ncopy+sample);s++)
            for (c=0;c<nchannels;c++,p++)
              buf16[p]=buffer[c][s];
        }
        break;
      case 24:
        {
          FXchar * buf8 = packet->s8();
          for (p=0,s=sample;s<(ncopy+sample);s++) {
            for (c=0;c<nchannels;c++,p+=3) {
              buf8[p+0]=(buffer[c][s]&0xFF);
              buf8[p+1]=(buffer[c][s]&0xFF00)>>8;
              buf8[p+2]=(buffer[c][s]&0xFF0000)>>16;
              }
            }
        }
        break;
      default: return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT; break;
      }
    navail-=ncopy;
    nframes-=ncopy;
    packet->wroteFrames(ncopy);
    sample+=ncopy;
    if (navail==0) {
      plugin->out=NULL;
      plugin->engine->output->post(packet);
      packet=NULL;
      }
    }
  return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
  }


FLAC__StreamDecoderReadStatus FlacDecoder::flac_decoder_read(const FLAC__StreamDecoder */*decoder*/, FLAC__byte buffer[], size_t *bytes, void *client_data) {
  FlacDecoder * plugin = reinterpret_cast<FlacDecoder*>(client_data);
  FXASSERT(plugin);
  FXASSERT(bytes && ((*bytes)>0));

  FXival nbytes = (*bytes);
  FXival p=0;
  FXival ncopy;

  Packet * packet = plugin->in;
  plugin->in=NULL;
  do {

    if (packet==NULL) {
      Event * event = plugin->engine->decoder->get_decoder_packet();
      if (event==NULL) {
        return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
        }
      packet = dynamic_cast<Packet*>(event);
      }

    FXASSERT(packet);
    FXASSERT(packet->next==NULL);

    /* Check for a end of stream packet */
    if (packet->size()==0 && packet->flags&FLAG_EOS) {
      (*bytes) = (*bytes) - nbytes;
      if ((*bytes) > 0) {
        plugin->in = packet; /// for next time
        return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
        }
      else {
        plugin->in = NULL;
        packet->unref();
        return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
        }
      }

    /* Got some data packet, copy data to decoder */
    ncopy=FXMIN(nbytes,packet->size());
    memcpy(&buffer[p],packet->data(),ncopy);
    nbytes-=ncopy;
    packet->readBytes(ncopy);

//    packet->size-=ncopy;
    p+=ncopy;

    /* release packet if we consumed all its data */
    if (packet->size()==0) {
      plugin->in=NULL;
      packet->unref();
      packet=NULL;
      FXASSERT(packet==NULL);
      }
    else {  /* move left over data to front, for next run */
//      memmove(packet->data,&packet->data[ncopy],packet->size);
      plugin->in=packet;
      }
    }
  while(nbytes>0);
  return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
  }

void FlacDecoder::flac_decoder_error(const FLAC__StreamDecoder */*decoder*/, FLAC__StreamDecoderErrorStatus/*status*/, void */*client_data*/) {
#if 0
  //FlacDecoder * plugin = reinterpret_cast<FlacDecoder*>(client_data);
  //FXASSERT(plugin);

  switch(status) {
    case FLAC__STREAM_DECODER_ERROR_STATUS_LOST_SYNC          : fxmessage("flac_decoder_error: An error in the stream caused the decoder to lose synchronization.\n"); break;
    case FLAC__STREAM_DECODER_ERROR_STATUS_BAD_HEADER         : fxmessage("flac_decoder_error: The decoder encountered a corrupted frame header.\n"); break;
    case FLAC__STREAM_DECODER_ERROR_STATUS_FRAME_CRC_MISMATCH : fxmessage("flac_decoder_error: The frame's data did not match the CRC in the footer.\n"); break;
    case FLAC__STREAM_DECODER_ERROR_STATUS_UNPARSEABLE_STREAM : fxmessage("flac_decoder_error: The decoder encountered reserved fields in use in the stream.\n"); break;
    }
#endif
  }

FlacDecoder::FlacDecoder(AudioEngine * e) : DecoderPlugin(e), flac(NULL),in(NULL),out(NULL) {
  }

FlacDecoder::~FlacDecoder() {
  flush();
  if (flac) {
    FLAC__stream_decoder_finish(flac);
    FLAC__stream_decoder_delete(flac);
    flac = NULL;
    }
  }

FXbool FlacDecoder::init(ConfigureEvent*event) {
  if (flac == NULL) {
    flac =  FLAC__stream_decoder_new();

    if ( flac == NULL)
      return false;

    FLAC__stream_decoder_set_md5_checking(flac,false);

    if (FLAC__stream_decoder_init_stream(flac,flac_decoder_read,NULL,NULL,NULL,NULL,
                                            flac_decoder_write,NULL,
                                            flac_decoder_error,
                                            this)!=FLAC__STREAM_DECODER_INIT_STATUS_OK){
      return false;
      }

     }
  else {
    // Apparently just flushing the decoder is not enough.
    // Prevent faulty seeking behaviour by resetting the decoder as well.
    FLAC__stream_decoder_reset(flac);
    }
  af=event->af;
  stream_length=event->stream_length;
  return true;
  }

FXbool FlacDecoder::flush() {
  FLAC__stream_decoder_flush(flac);
  if (in) {
    in->unref();
    in=NULL;
    }
  if (out) {
    out->unref();
    out=NULL;
    }
  return true;
  }

DecoderStatus FlacDecoder::process(Packet*packet){
  if (flac) {
    FXASSERT(in==NULL);
    FXASSERT(out==NULL);

    in=packet;
    FXuint stream=in->stream;

    FXASSERT(in);
    FXASSERT(in->next==NULL);
    FXbool result = FLAC__stream_decoder_process_until_end_of_stream(flac);

    FLAC__stream_decoder_flush(flac);
    if (result) {
      if (out) {
        engine->output->post(out);
        out=NULL;
        }
      }

    if (in) {
      in->unref();
      in=NULL;
      }

    if (out) {
      out->unref();
      out=NULL;
      }
    engine->output->post(new ControlEvent(End,stream));
    return DecoderOk;
    }
  return DecoderError;
  }

ReaderPlugin * ap_flac_reader(AudioEngine * engine) {
  return new FlacReader(engine);
  }

DecoderPlugin * ap_flac_decoder(AudioEngine * engine) {
  return new FlacDecoder(engine);
  }


}
