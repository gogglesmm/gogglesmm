/*******************************************************************************
*                         Goggles Audio Player Library                         *
********************************************************************************
*           Copyright (C) 2010-2015 by Sander Jansen. All Rights Reserved      *
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
#include "ap_packet.h"
#include "ap_id3v2.h"
#include "ap_engine.h"
#include "ap_input_plugin.h"
#include "ap_reader_plugin.h"
#include "ap_decoder_plugin.h"
#include "ap_decoder_thread.h"

#include <FLAC/stream_decoder.h>

namespace ap {


static const FXuchar crc8_lookup[256] = {
  0x00, 0x07, 0x0e, 0x09, 0x1c, 0x1b, 0x12, 0x15, 0x38, 0x3f, 0x36, 0x31, 0x24, 0x23, 0x2a, 0x2d,
  0x70, 0x77, 0x7e, 0x79, 0x6c, 0x6b, 0x62, 0x65, 0x48, 0x4f, 0x46, 0x41, 0x54, 0x53, 0x5a, 0x5d,
  0xe0, 0xe7, 0xee, 0xe9, 0xfc, 0xfb, 0xf2, 0xf5, 0xd8, 0xdf, 0xd6, 0xd1, 0xc4, 0xc3, 0xca, 0xcd,
  0x90, 0x97, 0x9e, 0x99, 0x8c, 0x8b, 0x82, 0x85, 0xa8, 0xaf, 0xa6, 0xa1, 0xb4, 0xb3, 0xba, 0xbd,
  0xc7, 0xc0, 0xc9, 0xce, 0xdb, 0xdc, 0xd5, 0xd2, 0xff, 0xf8, 0xf1, 0xf6, 0xe3, 0xe4, 0xed, 0xea,
  0xb7, 0xb0, 0xb9, 0xbe, 0xab, 0xac, 0xa5, 0xa2, 0x8f, 0x88, 0x81, 0x86, 0x93, 0x94, 0x9d, 0x9a,
  0x27, 0x20, 0x29, 0x2e, 0x3b, 0x3c, 0x35, 0x32, 0x1f, 0x18, 0x11, 0x16, 0x03, 0x04, 0x0d, 0x0a,
  0x57, 0x50, 0x59, 0x5e, 0x4b, 0x4c, 0x45, 0x42, 0x6f, 0x68, 0x61, 0x66, 0x73, 0x74, 0x7d, 0x7a,
  0x89, 0x8e, 0x87, 0x80, 0x95, 0x92, 0x9b, 0x9c, 0xb1, 0xb6, 0xbf, 0xb8, 0xad, 0xaa, 0xa3, 0xa4,
  0xf9, 0xfe, 0xf7, 0xf0, 0xe5, 0xe2, 0xeb, 0xec, 0xc1, 0xc6, 0xcf, 0xc8, 0xdd, 0xda, 0xd3, 0xd4,
  0x69, 0x6e, 0x67, 0x60, 0x75, 0x72, 0x7b, 0x7c, 0x51, 0x56, 0x5f, 0x58, 0x4d, 0x4a, 0x43, 0x44,
  0x19, 0x1e, 0x17, 0x10, 0x05, 0x02, 0x0b, 0x0c, 0x21, 0x26, 0x2f, 0x28, 0x3d, 0x3a, 0x33, 0x34,
  0x4e, 0x49, 0x40, 0x47, 0x52, 0x55, 0x5c, 0x5b, 0x76, 0x71, 0x78, 0x7f, 0x6a, 0x6d, 0x64, 0x63,
  0x3e, 0x39, 0x30, 0x37, 0x22, 0x25, 0x2c, 0x2b, 0x06, 0x01, 0x08, 0x0f, 0x1a, 0x1d, 0x14, 0x13,
  0xae, 0xa9, 0xa0, 0xa7, 0xb2, 0xb5, 0xbc, 0xbb, 0x96, 0x91, 0x98, 0x9f, 0x8a, 0x8d, 0x84, 0x83,
  0xde, 0xd9, 0xd0, 0xd7, 0xc2, 0xc5, 0xcc, 0xcb, 0xe6, 0xe1, 0xe8, 0xef, 0xfa, 0xfd, 0xf4, 0xf3
};



class FlacReader : public ReaderPlugin {
protected:
  FXlong   stream_start;
  FXushort minblocksize;
  FXushort maxblocksize;
  FXuint   minframesize;
  FXuint   maxframesize;
  struct SeekPoint {
    FXulong   sample;
    FXlong    offset;
    FXushort nsamples;
    };
  FXArray<SeekPoint> seektable;
protected:
  ReplayGain gain;
  MetaInfo*  meta;
protected:
  FXbool parse_blockheader(FXuchar & blocktype,FXuint & blocksize,FXbool & last);
  FXbool parse_streaminfo();
  FXbool parse_seektable(FXuint blocksize);
  FXbool parse_vorbiscomment(FXuint blocksize);
  FXint  parse_utf_value(const FXuchar * buffer,FXuint & value);
  FXint  parse_utf_value(const FXuchar * buffer,FXlong & value);
  FXbool sync(FXlong & offset,FXlong & sample,FXuint & blocksize);
protected:
  ReadStatus parse();
public:
  FlacReader(AudioEngine*);
  FXuchar format() const { return Format::FLAC; };
  FXbool init(InputPlugin*);
  FXbool seek(FXlong offset);
  FXbool can_seek() const;
  ReadStatus process(Packet*);
  ~FlacReader();
  };


class FlacDecoder : public DecoderPlugin {
protected:
  FLAC__StreamDecoder * flac;
  FXint stream_length;
protected:
  Packet * in;
  Packet * out;
protected:
  static FLAC__StreamDecoderWriteStatus   flac_decoder_write(const FLAC__StreamDecoder*,const FLAC__Frame*,const FLAC__int32*const[],void*);
  static FLAC__StreamDecoderReadStatus    flac_decoder_read(const FLAC__StreamDecoder*,FLAC__byte buffer[],size_t*,void*);
  static void                             flac_decoder_error(const FLAC__StreamDecoder *, FLAC__StreamDecoderErrorStatus, void *);
public:
  FlacDecoder(AudioEngine*);
  FXuchar codec() const { return Codec::FLAC; }
  FXbool flush(FXlong offset=0);
  FXbool init(ConfigureEvent*);
  DecoderStatus process(Packet*);
  ~FlacDecoder();
  };


extern void ap_replaygain_from_vorbis_comment(ReplayGain & gain,const FXchar * comment,FXint len);
extern void ap_meta_from_vorbis_comment(MetaInfo * meta, const FXchar * comment,FXint len);
extern void ap_parse_vorbiscomment(const FXchar * buffer,FXint len,ReplayGain & gain,MetaInfo * meta);

void flac_parse_vorbiscomment(const FXchar * buffer,FXint len,ReplayGain & gain,MetaInfo * meta) {
  FXString comment;
  const FXchar * end = buffer+len;

  // Check this is a VorbisComment.
  if ((buffer[0]&0x7f)!=4)
    return;

  /// skip the metaheader block
  buffer+=4;
  if (buffer>=end) return;

  ap_parse_vorbiscomment(buffer,len-4,gain,meta);
  }



FlacReader::FlacReader(AudioEngine* e) : ReaderPlugin(e),meta(nullptr) {
  }

FlacReader::~FlacReader(){
  }

FXbool FlacReader::init(InputPlugin*plugin) {
  ReaderPlugin::init(plugin);
  gain.reset();
  seektable.clear();
  if (meta) {
    meta->unref();
    meta=nullptr;
    }
  flags&=~FLAG_PARSED;
  return true;
  }

FXbool FlacReader::can_seek() const {
  return stream_length>0;
  }


FXint FlacReader::parse_utf_value(const FXuchar * buffer,FXlong & value) {
  FXint p=0;
  value = buffer[p++];
  if(0xC0<=value) { if (!FXISFOLLOWUTF8(buffer[p])) {return false;} value=(value<<6)^buffer[p++]^0x3080;
  if(0x800<=value) { if (!FXISFOLLOWUTF8(buffer[p])) {return false;} value=(value<<6)^buffer[p++]^0x20080;
  if(0x10000<=value) { if (!FXISFOLLOWUTF8(buffer[p])) {return false;} value=(value<<6)^buffer[p++]^0x400080;
  if(0x100000<=value) { if (!FXISFOLLOWUTF8(buffer[p])) {return false;} value=(value<<6)^buffer[p++]^0x8000080;
  if(0x4000000<=value) { if (!FXISFOLLOWUTF8(buffer[p])) {return false;} value=(value<<6)^buffer[p++]^0x100000080;
  if(0x80000000<=value) { if (!FXISFOLLOWUTF8(buffer[p])) {return false;} value=(value<<6)^buffer[p++]^0x2000000080;}}}}}}
  return p;
  }


FXint FlacReader::parse_utf_value(const FXuchar * buffer,FXuint & value) {
  FXint p=0;
  value = buffer[p++];
  if(0xC0<=value) { if (!FXISFOLLOWUTF8(buffer[p])) {return false;} value=(value<<6)^buffer[p++]^0x3080;
  if(0x800<=value) { if (!FXISFOLLOWUTF8(buffer[p])) {return false;} value=(value<<6)^buffer[p++]^0x20080;
  if(0x10000<=value) { if (!FXISFOLLOWUTF8(buffer[p])) {return false;} value=(value<<6)^buffer[p++]^0x400080;
  if(0x100000<=value) { if (!FXISFOLLOWUTF8(buffer[p])) {return false;} value=(value<<6)^buffer[p++]^0x8000080;
  if(0x4000000<=value) { if (!FXISFOLLOWUTF8(buffer[p])) {return false;} value=(value<<6)^buffer[p++]^0x100000080;}}}}}
  return p;
  }


#define match_frame_header(ptr,blocking) ((ptr)[0])==0xff &&\
                                         ((ptr)[1])==blocking &&\
                                         ((ptr)[2]&0xf0)!=0 &&\
                                         ((ptr)[2]&0xf)!=0xf &&\
                                         ((ptr)[3])<0xb0 &&\
                                         ((ptr)[3]&0x1)==0 &&\
                                         ((ptr)[3]&0x6)!=0x6

FXbool FlacReader::sync(FXlong & offset,FXlong & sample,FXuint & blocksize) {
  const FXuchar blocking_strategy = (minblocksize==maxblocksize) ? 0xf8 : 0xf9;
  FXuchar samplesize;
  FXuint  samplerate;
  FXuint  framenumber;
  FXuchar crc;
  FXuchar bytes[20];
  FXint p,h;

  if (input->read(&bytes,20)!=20)
    return false;

  do {

    // Find start of frame header in bytes
    for (p=0;p<16;p++) {

      // Match frame header start
      if (match_frame_header(bytes+p,blocking_strategy)) {

        h=p;

        // Match samplesize (invalid values were already filtered out)
        switch(bytes[h+3]&0xf){
          case  2: samplesize =  8; break;
          case  4: samplesize = 12; break;
          case  8: samplesize = 16; break;
          case 10: samplesize = 20; break;
          case 12: samplesize = 24; break;
          default: samplesize = af.bps(); break;
          }
        if (samplesize!=af.bps()) continue;

        // Match samplerate
        const FXuchar sr = bytes[h+2]&0xf;
        switch(sr){
          case  0: samplerate=af.rate; break;
          case  1: samplerate=88200;   break;
          case  2: samplerate=176400;  break;
          case  3: samplerate=192000;  break;
          case  4: samplerate=8000;    break;
          case  5: samplerate=16000;   break;
          case  6: samplerate=22050;   break;
          case  7: samplerate=24000;   break;
          case  8: samplerate=32000;   break;
          case  9: samplerate=44100;   break;
          case 10: samplerate=48000;   break;
          case 11: samplerate=96000;   break;
          default: samplerate=0;       break;
          }
        if (samplerate && samplerate!=af.rate) continue;

        // Match blocksize [for fixed blocksize streams]
        const FXuchar bs = bytes[h+2]>>4;
        if (minblocksize==maxblocksize && bs!=6 && bs!=7) {
          if (bs==1)
            blocksize=192;
          else if (bs>=8)
            blocksize=256<<(bs-8);
          else
            blocksize=576<<(bs-2);
          if (blocksize!=minblocksize) continue;
          }

        // Match Channel Count
        const FXuchar ch = bytes[h+3]>>4;
        if (ch<0x7) {
          if (af.channels!=(ch+1)) continue;
          }
        else if (af.channels!=2) continue;

        // Get remaining bytes
        memmove(bytes,bytes+p,20-p);
        if (input->read(bytes+20-p,p)!=p)
          return false;
        p=0;h=4;


        // read frame or sample
        if (minblocksize==maxblocksize) {
          FXint n = parse_utf_value(bytes+h,framenumber);
          if (n==0) continue;
          h+=n;
          sample=minblocksize*framenumber;
          }
        else {
          FXint n = parse_utf_value(bytes+h,sample);
          if (n==0) continue;
          h+=n;
          }

        // Check sample number
        if (sample<0 || sample>stream_length) continue;

        if (bs==6) { // read 8-bit blocksize
          blocksize = 1 + bytes[h++];
          if (minblocksize==maxblocksize && blocksize!=minblocksize) continue;
          }
        else if (bs==7) { // read 16-bit blocksize
#if FOX_BIGENDIAN == 0
          blocksize = 1 + ((bytes[h]<<8) | (bytes[h+1]));
#else
          blocksize = 1 + ((bytes[h]) | (bytes[h+1]<<8));
#endif
          h+=2;
          if (minblocksize==maxblocksize && blocksize!=minblocksize) continue;
          }

        if (sr==12) { // read 8-bit samplerate
          samplerate = bytes[h++]*1000;
          if (samplerate!=af.rate) continue;
          }
        else if (sr>=13) { // read 16-bit samplerate
#if FOX_BIGENDIAN == 0
          samplerate = (bytes[h]<<8) | (bytes[h+1]);
#else
          samplerate = (bytes[h]) | (bytes[h+1]<<8);
#endif
          h+=2;
          if (sr==14) samplerate *= 10;
          if (samplerate!=af.rate) continue;
          }

        crc = 0;
        for (FXuchar c=0;c<h;c++){
          crc = crc8_lookup[crc ^ bytes[c]];
          }
        if (crc!=bytes[h++]){
          continue;
          }

        offset=input->position()-20;
        return true;
        }
      }
    FXASSERT(p==16);
    memmove(bytes,bytes+16,4);
    if (input->read(bytes+4,16)!=16)
      return false;
    }
  while(1);
  }




FXbool FlacReader::seek(FXlong target) {
  const FXulong placeholder = 0xFFFFFFFFFFFFFFFF;

  FXlong min_offset=stream_start;
  FXlong max_offset=input->size();
  FXlong min_sample=0;
  FXlong max_sample=stream_length;
  FXlong sample;
  FXuint blocksize;
  FXuint framesize = ((minframesize+maxframesize) / 2) + 1;
  FXint count=0;


  // Use seektable to reduce search range
  for (FXint i=0;i<seektable.no();i++) {
    if ((seektable[i].sample!=placeholder) && seektable[i].nsamples && seektable[i].sample > (FXulong)target && seektable[i].offset+stream_start<max_offset){
      max_offset = stream_start+seektable[i].offset;
      max_sample = seektable[i].sample;
      }
    if ((seektable[i].sample!=placeholder) && seektable[i].nsamples && seektable[i].sample < (FXulong)target && seektable[i].offset+stream_start>min_offset){
      min_offset = stream_start+seektable[i].offset;
      min_sample = seektable[i].sample;
      }
    }


  do {

    if (min_offset>=max_offset) {
      GM_DEBUG_PRINT("[flac] seek failed! min_offset>=max_offset %ld >= %ld",min_offset,max_offset);
      return false;
      }

    FXlong position = min_offset + (((FXdouble)(target-min_sample)/(FXdouble)(max_sample-min_sample))*(max_offset-min_offset)) - framesize;


    if (position<min_offset) position=min_offset;

    FXASSERT(position<max_offset);
    FXASSERT(position>=min_offset);

    input->position(position,FXIO::Begin);

    if (sync(position,sample,blocksize)) {

      if (target<sample) {
        max_sample=sample+blocksize;
        max_offset=position;
        }
      else if (target>=(sample+blocksize)){
        if (minframesize){
          position+=minframesize;
          input->position(position,FXIO::Begin);
          }
        if (!sync(position,sample,blocksize)) {
          return false;
          }
        if (target>=sample && target<sample+blocksize){
          input->position(position,FXIO::Begin);
          return true;
          }
        min_sample=sample;
        min_offset=position;
        }
      else {
        input->position(position,FXIO::Begin);
        return true;
        }
      count++;
      continue;
      }
    return false;
    }
  while(count<10);

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




FXbool FlacReader::parse_blockheader(FXuchar & type,FXuint & size,FXbool & last) {
  FXuchar header[4];

  if (input->read(header,4)!=4)
    return false;

  last = header[0]&0x80;
  type = header[0]&0x7f;
#if FOX_BIGENDIAN == 0
  size = (header[3]) | (header[2]<<8) | (header[1]<<16);
#else
  size = (header[3]<<16) | (header[2]<<8) | (header[1]);
#endif
  if (type==127)
    return false;
  return true;
  }



static const FXuint flac_channel_map[]={
  AP_CHANNELMAP_MONO,

  AP_CHANNELMAP_STEREO,

  AP_CMAP3(Channel::FrontLeft,
           Channel::FrontRight,
           Channel::FrontCenter),

  AP_CMAP4(Channel::FrontLeft,
           Channel::FrontRight,
           Channel::BackLeft,
           Channel::BackRight),

  AP_CMAP5(Channel::FrontLeft,
           Channel::FrontRight,
           Channel::FrontCenter,
           Channel::BackLeft,
           Channel::BackRight),

  AP_CMAP6(Channel::FrontLeft,
           Channel::FrontRight,
           Channel::FrontCenter,
           Channel::LFE,
           Channel::BackLeft,
           Channel::BackRight),

  AP_CMAP7(Channel::FrontLeft,
           Channel::FrontRight,
           Channel::FrontCenter,
           Channel::LFE,
           Channel::BackCenter,
           Channel::SideLeft,
           Channel::SideRight),

  AP_CMAP8(Channel::FrontLeft,
           Channel::FrontRight,
           Channel::FrontCenter,
           Channel::LFE,
           Channel::BackLeft,
           Channel::BackRight,
           Channel::SideLeft,
           Channel::SideRight),
  };




FXbool flac_audioformat(const FXuchar * info,AudioFormat & af,FXlong & stream_length){
  FXuint   samplerate;
  FXuchar  samplesize;
  FXuchar  channels;

  samplerate    = ((FXuint)info[0]<<12 )| ((FXuint)info[1]<<4) | ((((FXuint)info[2])>>4)&0xf);
  samplesize    = 1+(((((FXuint)info[2])&0x1)<<4) | ((info[3]>>4)&0xf));
  channels      = 1+ ((info[2]>>1)&0x7);
  stream_length = ((info[3]&0xf0)<<28) | (info[4]<<24) | (info[5]<<16) | (info[6]<<8) | (info[7]);

  if (channels<1 || channels>8)
    return false;

  af.set(Format::Signed|Format::Little,samplesize,
                                       samplesize>>3,
                                       samplerate,
                                       channels,
                                       flac_channel_map[channels-1]);
  return true;
  }







FXbool FlacReader::parse_streaminfo() {
  if (!input->read_uint16_be(minblocksize))
    return false;

  if (!input->read_uint16_be(maxblocksize))
    return false;

  if (!input->read_uint24_be(minframesize))
    return false;

  if (!input->read_uint24_be(maxframesize))
    return false;

  FXuchar info[8];

  if (input->read(&info,8)!=8)
    return false;

  if (!flac_audioformat(info,af,stream_length))
    return false;

  input->position(16,FXIO::Current);
  return true;
  }


FXbool FlacReader::parse_seektable(FXuint blocksize) {
  if (blocksize%18==0 && seektable.no()==0) {
    FXint npoints = blocksize / 18;
    seektable.no(npoints);
    for (FXint i=0;i<npoints;i++) {
      input->read_uint64_be(seektable[i].sample);
      input->read_int64_be(seektable[i].offset);
      input->read_uint16_be(seektable[i].nsamples);
      }
    return true;
    }
  return false;
  }

FXbool FlacReader::parse_vorbiscomment(FXuint blocksize) {
  FXuint    length;
  FXuint    ncomments;
  FXuchar * data=nullptr;
  FXuint    datalength=0;

  if (blocksize>8) {

    meta = new MetaInfo;

    if (!input->read_uint32_le(length))
      return false;

    if (4+length>blocksize)
      return false;

    input->position(length,FXIO::Current);

    if (!input->read_uint32_le(ncomments))
      return false;

    for (FXuint c=0;c<ncomments;c++) {

      // Length of comment
      if (!input->read_uint32_le(length))
        return false;

      if (datalength<length){
        resizeElms(data,length);
        datalength=length;
        }

      if (input->read(data,length)!=length)
        return false;

      ap_replaygain_from_vorbis_comment(gain,(const FXchar*)data,length);
      ap_meta_from_vorbis_comment(meta,(const FXchar*)data,length);
      }
    }
  return true;
  }



ReadStatus FlacReader::parse() {
  enum {
    StreamInfo    = 0,
    Padding       = 1,
    Application   = 2,
    Seektable     = 3,
    VorbisComment = 4,
    Cuesheet      = 5,
    Picture       = 6
    };

  stream_length=0;

  FXuchar id[4];

  if (input->read(&id,4)!=4)
    return ReadError;

  // Handle flac files starting with id3v2
  if (id[0]=='I' && id[1]=='D' && id[2]=='3') {

    if (!ID3V2::skip(input,id))
      return ReadError;

    if (input->read(&id,4)!=4)
      return ReadError;
    }

  if (id[0]!='f' || id[1]!='L' || id[2]!='a' || id[3]!='C'){
    return ReadError;
    }

  FXuchar blocktype;
  FXuint  blocksize;
  FXbool  last;

  if (!parse_blockheader(blocktype,blocksize,last))
    return ReadError;

  if (blocktype!=StreamInfo || blocksize!=34)
    return ReadError;

  if (!parse_streaminfo())
    return ReadError;

  while(parse_blockheader(blocktype,blocksize,last)) {
    switch(blocktype) {
      case StreamInfo   : return ReadError; break;
      case Seektable    : if (!parse_seektable(blocksize))
                            return ReadError;
                          break;
      case VorbisComment: if (!parse_vorbiscomment(blocksize))
                            return ReadError;
                          break;
      case Application  :
      case Cuesheet     :
      case Picture      :
      case Padding      :
      default           : if (blocksize>0)
                            input->position(blocksize,FXIO::Current);
                          break;
      }
    if (last) {
      stream_start  = input->position();

      flags|=FLAG_PARSED;

      ConfigureEvent * config = new ConfigureEvent(af,Codec::FLAC,stream_length);
      config->replaygain=gain;
      engine->decoder->post(config);

      if (meta) {
        engine->decoder->post(meta);
        meta=nullptr;
        }
      return ReadOk;
      }
    }
  return ReadError;
  }


FLAC__StreamDecoderWriteStatus FlacDecoder::flac_decoder_write(const FLAC__StreamDecoder */*decoder*/, const FLAC__Frame *frame, const FLAC__int32 *const buffer[], void *client_data) {
  FlacDecoder * plugin = static_cast<FlacDecoder*>(client_data);
  FXASSERT(frame);
  FXASSERT(buffer);
  FXint s,c,p=0;
  FXint sample  = 0;
  FXint nchannels = frame->header.channels;
  FXint ncopy;

  FXlong stream_position = frame->header.number.sample_number;


  FXint nframes = frame->header.blocksize;

  FXASSERT(frame->header.number_type==FLAC__FRAME_NUMBER_TYPE_SAMPLE_NUMBER);

  if (nframes==0)
    return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;

  Packet * packet = plugin->out;
  if (packet) {
    FXASSERT(packet->stream_position+packet->numFrames()==stream_position);
    if (packet->numFrames()==0)
      packet->stream_position = stream_position;
    }

  if (stream_position<plugin->stream_decode_offset) {
    FXlong offset = FXMIN(nframes,plugin->stream_decode_offset-stream_position);
    GM_DEBUG_PRINT("[flac] stream decode offset %ld. Skipping %ld of %ld \n",plugin->stream_decode_offset,offset,plugin->stream_decode_offset-stream_position);
    nframes-=offset;
    stream_position+=offset;
    sample+=offset;
    }


  while(nframes>0) {

    /// get a fresh packet
    if (!packet) {
      packet=plugin->engine->decoder->get_output_packet();
      if (packet==nullptr) {
        return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
        }
      packet->af=plugin->af;
//      packet->stream_position=frame->header.number.sample_number+(frame->header.blocksize-nframes);
      packet->stream_position=stream_position;
      packet->stream_length=plugin->stream_length;
      plugin->out=packet;
      }

    ncopy = FXMIN(nframes,packet->availableFrames());
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
    sample+=ncopy;
    nframes-=ncopy;
    stream_position+=ncopy;
    packet->wroteFrames(ncopy);
    if (packet->availableFrames()==0) {
      plugin->out=nullptr;
      plugin->engine->output->post(packet);
      packet=nullptr;
      }
    }
  return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
  }


FLAC__StreamDecoderReadStatus FlacDecoder::flac_decoder_read(const FLAC__StreamDecoder */*decoder*/, FLAC__byte buffer[], size_t *bytes, void *client_data) {
  FlacDecoder * plugin = static_cast<FlacDecoder*>(client_data);
  FXASSERT(plugin);
  FXASSERT(bytes && ((*bytes)>0));

  FXival nbytes = (*bytes);
  FXival p=0;
  FXival ncopy;

  Packet * packet = plugin->in;
  plugin->in=nullptr;
  do {

    if (packet==nullptr) {
      Event * event = plugin->engine->decoder->get_decoder_packet();
      if (event==nullptr) {
        return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
        }
      packet = dynamic_cast<Packet*>(event);
      }

    FXASSERT(packet);
    FXASSERT(packet->next==nullptr);

    /* Check for a end of stream packet */
    if (packet->size()==0 && packet->flags&FLAG_EOS) {
      (*bytes) = (*bytes) - nbytes;
      if ((*bytes) > 0) {
        plugin->in = packet; /// for next time
        return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
        }
      else {
        plugin->in = nullptr;
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
      plugin->in=nullptr;
      packet->unref();
      packet=nullptr;
      FXASSERT(packet==nullptr);
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

FlacDecoder::FlacDecoder(AudioEngine * e) : DecoderPlugin(e), flac(nullptr),in(nullptr),out(nullptr) {
  }

FlacDecoder::~FlacDecoder() {
  flush();
  if (flac) {
    FLAC__stream_decoder_finish(flac);
    FLAC__stream_decoder_delete(flac);
    flac = nullptr;
    }
  }

FXbool FlacDecoder::init(ConfigureEvent*event) {
  DecoderPlugin::init(event);
  if (flac == nullptr) {
    flac =  FLAC__stream_decoder_new();

    if ( flac == nullptr)
      return false;

    FLAC__stream_decoder_set_md5_checking(flac,false);

    if (FLAC__stream_decoder_init_stream(flac,flac_decoder_read,nullptr,nullptr,nullptr,nullptr,
                                            flac_decoder_write,nullptr,
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

FXbool FlacDecoder::flush(FXlong offset) {
  DecoderPlugin::flush(offset);
  FLAC__stream_decoder_flush(flac);
  if (in) {
    in->unref();
    in=nullptr;
    }
  if (out) {
    out->unref();
    out=nullptr;
    }
  return true;
  }

DecoderStatus FlacDecoder::process(Packet*packet){
  if (flac) {
    FXASSERT(in==nullptr);
    FXASSERT(out==nullptr);

    in=packet;
    FXuint stream=in->stream;

    FXASSERT(in);
    FXASSERT(in->next==nullptr);
    FXbool result = FLAC__stream_decoder_process_until_end_of_stream(flac);

    FLAC__stream_decoder_flush(flac);
    if (result) {
      if (out) {
        engine->output->post(out);
        out=nullptr;
        }
      }

    if (in) {
      in->unref();
      in=nullptr;
      }

    if (out) {
      out->unref();
      out=nullptr;
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
