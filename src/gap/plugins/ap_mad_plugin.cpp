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
#include "ap_id3v2.h"
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

#include <mad.h>

#include <FX88591Codec.h>


namespace ap {

class XingHeader;
class VBRIHeader;
class LameHeader;
class ID3V2;
class ID3V1;
struct mpeg_frame;

class MadReader : public ReaderPlugin {
protected:
  FXuchar      buffer[4];
  FXbool       sync;
  FXint        bitrate;
protected:
  FXlong       input_start;
  FXlong       input_end;
  FXlong       stream_position;
protected:
  XingHeader * xing;
  VBRIHeader * vbri;
  LameHeader * lame;
protected:
  ID3V1      * id3v1;
  ID3V2      * id3v2;
  //ApeTag     * apetag;
protected:
  FXbool parse_id3v1();
  FXbool parse_id3v2();
  FXbool parse_ape();
  FXbool parse_lyrics();
protected:
  ReadStatus parse(Packet*);
  FXbool readFrame(Packet*,const mpeg_frame&);
  void parseFrame(Packet*,const mpeg_frame&);


  void set_replay_gain(ConfigureEvent*);
  void send_meta();
  void clear_headers();
  void clear_tags();
public:
  MadReader(AudioEngine*);

  FXuchar format() const { return Format::MP3; };

  FXbool init(InputPlugin*);
  FXbool can_seek() const;
  FXbool seek(FXdouble);
  ReadStatus process(Packet*);
  virtual ~MadReader();
  };


class MadDecoder : public DecoderPlugin {
protected:
  MemoryBuffer buffer;
  Packet * out;
protected:
  mad_synth synth;
  mad_stream stream;
  mad_frame frame;
protected:
  FXuchar flags;
  FXlong  stream_position;
  FXshort stream_offset_start;
  FXshort stream_offset_end;
  FXint   frame_counter;

protected:
  enum {
    FLAG_INIT = 0x1,
    };
public:
  MadDecoder(AudioEngine*);
  FXuchar codec() const { return Codec::MPEG; }
  FXbool init(ConfigureEvent*);
  DecoderStatus process(Packet*);
  FXbool flush();
  virtual ~MadDecoder();
  };




static const FXchar v1_layer2_validation[]={
1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,
1,0,0,0,1,0,1,1,1,1,1,1,1,1,1,0,
1,0,0,0,1,0,1,1,1,1,1,1,1,1,1,0,
1,0,0,0,1,0,1,1,1,1,1,1,1,1,1,0,
};

static const FXshort bitrates[]={
  0,32,64,96,128,160,192,224, 256,288,320,352, 384,416,448,-1, /// v1,l1
  0,32,48,56, 64, 80, 96,112, 128,160,192,224, 256,320,384,-1, /// v1,l2
  0,32,40,48,	56, 64, 80, 96, 112,128,160,192, 224,256,320,-1, /// v1,l3
  0,32,48,56, 64, 80, 96,112, 128,144,160,176, 192,224,256,-1, /// v2,l1
  0, 8,16,24, 32, 40, 48, 56,  64, 80, 96,112, 128,144,160,-1  /// v2,l2,l3
  };

static const FXint samplerates[]={
  44100,48000,32000,-1, /// v1
  22050,24000,16000,-1, /// v2
  11025,12000, 8000,-1  /// v2.5
  };

static const FXchar * const channels[]={
  "Stereo",
  "Joint",
  "Dual",
  "Single"
  };




struct mpeg_frame {
private:
  FXuint header;
public:
  enum {
    Invalid = -1,
    V1			=  0,
    V2			=  1,
    V25		  =  2,
    };

  enum {
    Layer_1 = 0,
    Layer_2 = 1,
    Layer_3 = 2,
    };

  enum {
    Stereo = 0,
    Joint	 = 1,
    Dual   = 2,
    Single = 3
    };
protected:
public:
  mpeg_frame() : header(0) {}


#define PRINT_YES_NO(x) (x ? "yes" : "no")

  void debug() {
    fxmessage("   has_sync: %s\n",PRINT_YES_NO(sync()));
    fxmessage("    version: %d\n",version()+1);
    fxmessage("      layer: %d\n",layer()+1);
    fxmessage("    bitrate: %d\n",bitrate());
    fxmessage(" samplerate: %d\n",samplerate());
    fxmessage("   channels: %d\n",channel());
    fxmessage("    samples: %d\n",nsamples());
    fxmessage("       size: %d\n",size());
    }


  FXbool validate(const FXuchar * buffer) {
    header=INT32_BE(buffer);
    if ((!sync()) ||
        (version()==Invalid) ||
        (layer()==Invalid) ||
        (bitrate()==Invalid) ||
        (samplerate()==-1)
        ) {
      return false;
      }
    //debug();
    return true;
    }

  FXbool sync() const {
    return (header>>21)==0x7ff;
    }

  FXchar version() const {
    const FXuchar v = (header&0x180000)>>19;
    switch(v) {
      case 0 : return V25; break;
      case 2 : return V2;  break;
      case 3 : return V1;  break;
      default: break;
      }
    return Invalid;
    }

  FXchar layer() const {
    const FXuchar v = ((header>>17)&0x3);
    switch(v) {
      case 1 : return Layer_3; break;
      case 2 : return Layer_2; break;
      case 3 : return Layer_1; break;
      default: break;
      }
    //fxmessage("Invalid layer %u\n",v);
    return Invalid;
    }

  FXbool crc() const {
    return ((header>>16)&0x1)==0;
    }

  FXint bitrate() const {
    const FXuint b = ((header>>12)&0xf);
    if (version()==V1)
      return 1000*(FXint)bitrates[b+(layer()<<4)];
    else if (version()==V2 || version()==V25) {
      if (layer()==Layer_1)
        return 1000*(FXint)bitrates[48+b];
      else
        return 1000*(FXint)bitrates[64+b];
      }
    return Invalid;
    }

  FXint samplerate() const {
    const FXuchar s = ((header>>10)&0x3);
    FXASSERT(version()!=Invalid);
    return samplerates[s+(version()<<2)];
    }

  FXint padding() const {
    if ((header>>9)&0x1)
      return (layer()==Layer_1) ? 4 : 1;
    else
      return 0;
    }

  FXuchar channel() const {
    return ((header>>6)&0x3);
    }

  FXint nsamples() const {
    if (layer()==Layer_1)
      return 384;
    else if (layer()==Layer_2 || version()==V1)
      return 1152;
    else  /// layer_3 && (V2 || V2_5)
      return 576;
    }

  FXint size() const {
    FXint s = nsamples() * (bitrate() / 8);
    s /= samplerate();
    s += padding();
    return s;
    }

  FXint xing_offset() const {
    if (version()==V1) {
      if (channel()!=3)
        return 32+4;
      else
        return 17+4;
      }
    else {
      if (channel()!=3)
        return 17+4;
      else
        return 9+4;
      }
    }

  FXint vbri_offset() const {
    return 32+4;
    }

  FXint lame_offset()  const {
    return xing_offset() + 120;
    }


  };



class VBRIHeader {
public:
  FXushort version;
  FXushort delay;
  FXushort quality;
  FXuint   nbytes;
  FXuint   nframes;
  FXushort ntoc;
  FXushort toc_scale;
  FXushort toc_entry_nbytes;
  FXushort toc_entry_nframes;
  FXuint*  toc;
public:
  VBRIHeader(const FXuchar * buffer,FXival nbytes);
  ~VBRIHeader();
  };

VBRIHeader::VBRIHeader(const FXuchar * buffer,FXival) : toc(NULL) {
  version           = INT16_BE(buffer+4);
  delay             = INT16_BE(buffer+6);
  quality           = INT16_BE(buffer+8);
  nbytes            = INT32_BE(buffer+10);
  nframes           = INT32_BE(buffer+14);
  ntoc              = INT16_BE(buffer+18);
  toc_scale         = INT16_BE(buffer+20);
  toc_entry_nbytes  = INT16_BE(buffer+22);
  toc_entry_nframes = INT16_BE(buffer+24);

  if (toc_entry_nbytes>=1 && toc_entry_nbytes <=4) {
    callocElms(toc,ntoc);
    for (FXint i=0;i<ntoc;i++) {
#if FOX_BIGENDIAN == 0
      for (FXint j=0;j<toc_entry_nbytes;j++) {
        toc[i]=(toc[i]<<8) | *(buffer+26+j+(i*toc_entry_nbytes));
        }
#else
#error fixme
#endif
      }
    }
#ifdef DEBUG
  fxmessage("VBRI:\n");
  fxmessage("\t          version:%d\n",version);
  fxmessage("\t            delay:%d\n",delay);
  fxmessage("\t          quality:%d\n",quality);
  fxmessage("\t           nbytes:%u\n",nbytes);
  fxmessage("\t          nframes:%u\n",nframes);
  fxmessage("\t             ntoc:%d\n",ntoc);
  fxmessage("\t        toc_scale:%d\n",toc_scale);
  fxmessage("\t toc_entry_nbytes:%d\n",toc_entry_nbytes);
  fxmessage("\ttoc_entry_nframes:%d\n",toc_entry_nframes);

  for (FXint i=0;i<ntoc;i++) {
    fxmessage("\ttoc[%d]=%d\n",i,toc[i]);
    }
#endif
  }

VBRIHeader::~VBRIHeader(){
  if (toc) freeElms(toc);
  }

#define XING_HEADER_SIZE 120

class XingHeader {
public:
  FXuint    flags;
  FXint     nframes;
  FXint     nbytes;
  FXint     vbr_scale;
  FXuchar   toc[100];
public:
  enum {
    HAS_FRAMES    = 0x1,
    HAS_BYTES     = 0x2,
    HAS_TOC       = 0x4,
    HAS_VBR_SCALE = 0x8
    };
public:
  XingHeader(const FXuchar * buffer,FXival nbytes);

  FXlong seek(FXdouble pos,FXlong length);
  };


XingHeader::XingHeader(const FXuchar * buffer,FXival /*nb*/) : flags(0),nframes(0),nbytes(0),vbr_scale(0) {
  buffer+=4;

  GM_DEBUG_PRINT("Xing:\n");

  flags  = INT32_BE(buffer);
  buffer+=4;
  if (flags&HAS_FRAMES) {
    nframes= INT32_BE(buffer);
    buffer+=4;
    GM_DEBUG_PRINT("\t  nframes: %u\n",nframes);
    }

  if (flags&HAS_BYTES) {
    nbytes = INT32_BE(buffer);
    buffer+=4;
    GM_DEBUG_PRINT("\t   nbytes: %u\n",nbytes);
    }

  if (flags&HAS_TOC) {
    memcpy(toc,buffer,100);
    buffer+=100;
    GM_DEBUG_PRINT("\t      toc: yes\n");
    }

  if (flags&HAS_VBR_SCALE) {
    vbr_scale =  INT32_BE(buffer);
    buffer+=4;
    GM_DEBUG_PRINT("\tvbr_scale: %d\n",vbr_scale);
    }
  }

FXlong XingHeader::seek(FXdouble pos,FXlong length) {
  if (flags&HAS_TOC) {
    FXdouble fa,fb,fx;

    FXdouble percent = FXCLAMP(0.0,(pos * 100.0),100.0);

//    fxmessage("percent: %g\n",percent);
    FXint a = FXMIN(99,((FXint)percent));

 //   fxmessage("a: %d\n",a);
    fa = toc[a];
 //   fxmessage("fa: %g\n",fa);

    if (a<99)
      fb = toc[a+1];
    else
      fb = 256.0;

//    fxmessage("fb: %g\n",fb);

    fx=FXLERP(fa,fb,(percent-a));
    return (FXlong) ((1.0/256.0)*fx*length);
    }
  else {
    return -1;
    }
  }



class LameHeader {
protected:
  FXdouble parse_replay_gain(const FXuchar * buffer);
public:
  FXushort padstart;
  FXushort padend;
  FXuint   length;
  ReplayGain replaygain;
public:
  LameHeader(const FXuchar * buffer,FXival nbytes);
  };

LameHeader::LameHeader(const FXuchar * buffer,FXival/* nbytes*/) : padstart(0), padend(0), length(0) {
  FXuchar revision = (*(buffer+9))>>4;
  FXuchar vbr_methed = (*(buffer+9))&0xf;

  FXint lowpass = (*(buffer+10)) * 100;


  FXfloat peak = INT32_BE(buffer+11);

  replaygain.album_peak = peak;
  replaygain.track_peak = peak;
  replaygain.track      = parse_replay_gain(buffer+15);
  replaygain.album      = parse_replay_gain(buffer+17);


  FXuchar encoding_flags = (*(buffer+19))>>4;
  FXuchar lame_type = (*(buffer+19))&0xf;

//   FXuchar bitrate = (*(buffer+21));

  padstart = ((FXuint)*(buffer+22))<<4 | (((FXuint)*(buffer+23))>>4);
  padend   = ((FXuint)*(buffer+23)&0xf)<<8 | ((FXuint)*(buffer+24));

  FXuchar misc = (*(buffer+25));

//   FXuchar mp3gain = (*(buffer+25));
//   FXushort surround = INT16_BE(buffer+26);
  length = INT32_BE(buffer+28);

#ifdef DEBUG
  fxmessage("Lame Info:\n");
  fxmessage("\t      revision: %d\n",revision);
  fxmessage("\t    vbr_method: %d\n",vbr_methed);
  fxmessage("\t       lowpass: %d\n",lowpass);
  fxmessage("\t    track gain: %g\n",replaygain.track);
  fxmessage("\t    album gain: %g\n",replaygain.album);
  fxmessage("\t          peak: %g\n",peak);
  fxmessage("\tencoding_flags: %x\n",encoding_flags);
  fxmessage("\t     lame_type: %d\n",lame_type);
  fxmessage("\t       padding: %d %d\n",padstart,padend);
  fxmessage("\t          misc: %x\n",misc);
  fxmessage("\t        length: %d\n",length);
#endif
  }

FXdouble LameHeader::parse_replay_gain(const FXuchar * buffer) {
  struct {
    signed int x:10;
    } s10;
  FXint gain = s10.x = ((*buffer)&0xC0)<<2 | (*(buffer+1));
  if (gain) {
    if ((*buffer)&0x20)
      gain=-gain;
    return (gain/10.0f);
    }
  return NAN;
  }



class ID3V1 {
public:
  FXString title;
  FXString artist;
  FXString album;
protected:
  void parse_field(const FXchar * start,FXint maxlen,FXString & field);
public:
  ID3V1(const FXchar * buffer,FXint len);

  FXbool empty() const;
  };

void ID3V1::parse_field(const FXchar * start,FXint maxlen,FXString & field){
  const FXchar * end = start+maxlen-1;

  /// Trim beginning
  while(start<=end && (*start==' ')) start++;

  /// Trim end
  while(end>=start && (*end=='\0' || *end==' ')) end--;

  /// Anything Left?
  if (end>=start) {
    FX88591Codec codec;
    FXint n = codec.mb2utflen(start,end-start+1);
    if (n>0) {
      field.length(n);
      codec.mb2utf(field.text(),field.length(),start,end-start+1);
      }
    }
  }

ID3V1::ID3V1(const FXchar * b,FXint /*len*/) {
  parse_field(b,30,title);
  parse_field(b+30,30,artist);
  parse_field(b+60,30,album);
  }

FXbool ID3V1::empty() const {
  return (title.empty() && artist.empty() && album.empty());
  }




/// Need MP3 files with APETAGS to test this...
#if 0

class ApeTag {
public:
  FXString title;
protected:
 enum {
   APE_HEADER          = (1<<29),
   APE_CONTAINS_FOOTER = (1<<30),
   APE_CONTAINS_HEADER = (1<<31),
   };
protected:
  void parse_item(FXuint flags,const FXchar * key,FXint key_length,const FXchar * value,FXint value_length);
public:
  ApeTag(const FXchar * buffer,FXint len);
  };



void ApeTag::parse_item(FXuint flags,const FXchar * key,FXint key_length,const FXchar * value,FXint value_length){
  FXint type = (flags>>1)&0x3;
  if (type==0) {
    fxmessage("key: %s\n",key);
    if (comparecase(key,"title",6)==0) {
      title.assign(value,value_length);
      }
    }
  }



ApeTag::ApeTag(const FXchar * buffer,FXint len) {
  const FXchar * end = buffer+len;
  const FXchar * p;

  FXint version = INT32_LE(buffer+8);
  FXint size    = INT32_LE(buffer+12);
  FXint nitems  = INT32_LE(buffer+16);
  FXint flags   = INT32_LE(buffer+20);

  buffer+=32;
  for (FXint i=0;i<nitems && (buffer+10)<end;i++) {
    p = buffer+8;
    while(*p!='\0' && p<end) p++;
    if (p>=end) return;
    const FXchar * item_key   = buffer+8;
    const FXchar * item_data  = p;
    FXint          item_size  = INT32_LE(buffer);
    FXuint         item_flags = INT32_LE(buffer+4);
    parse_item(item_flags,item_key,p-item_key,item_data,item_size);
    buffer=item_data+item_size;
    }
  }

#endif








MadReader::MadReader(AudioEngine*e) : ReaderPlugin(e),
  sync(false),
  xing(NULL),
  vbri(NULL),
  lame(NULL),
  id3v1(NULL),
  id3v2(NULL) {
  //apetag(NULL) {
  }

MadReader::~MadReader() {
  clear_headers();
  clear_tags();
  }

void MadReader::clear_tags() {
  if (id3v1) {
    delete id3v1;
    id3v1=NULL;
    }
  if (id3v2) {
    delete id3v2;
    id3v2=NULL;
    }
/*
  if (apetag){
    delete apetag;
    apetag=NULL;
    }
*/
  }

void MadReader::clear_headers() {
  if (xing) {
    delete xing;
    xing=NULL;
    }
  if (vbri) {
    delete vbri;
    vbri=NULL;
    }
  if (lame) {
    delete lame;
    lame=NULL;
    }
  }

FXbool MadReader::init(InputPlugin*plugin){
  ReaderPlugin::init(plugin);
  GM_DEBUG_PRINT("[mad_reader] init()\n");
  buffer[0]=buffer[1]=buffer[2]=buffer[3]=0;
  clear_headers();
  clear_tags();
  flags&=~FLAG_PARSED;
  sync=false;
  input_start=0;
  input_end=0;
  return true;
  }

FXbool MadReader::can_seek() const{
  return true;
  }

FXbool MadReader::seek(FXdouble pos) {
  if (!input->serial()){
    FXlong offset = 0;
    if (xing) {
      offset = xing->seek(pos,(input_end - input_start));
      GM_DEBUG_PRINT("offset: %ld\n",offset);
      if (offset==-1) return false;
      stream_position = stream_length * pos;
      }
    else if (vbri) {
      }
    else {
      stream_position = stream_length * pos;
      offset = (input_end - input_start) * pos;
      }
    input->position(input_start+offset,FXIO::Begin);
    }
  return true;
  }


FXbool MadReader::readFrame(Packet * packet,const mpeg_frame & frame) {
  memcpy(packet->ptr(),buffer,4);
  FXint nread = input->read(packet->ptr()+4,frame.size()-4);
  if (nread!=(frame.size()-4)) {
    packet->unref();
    return false;
    }
  packet->wroteBytes(frame.size());
  return true;
  }


void MadReader::parseFrame(Packet * packet,const mpeg_frame & frame) {
  if (compare((const FXchar*)(packet->data()+frame.xing_offset()),"Xing",4)==0 ||
      compare((const FXchar*)(packet->data()+frame.xing_offset()),"Info",4)==0 ) {

    xing = new XingHeader(packet->data()+frame.xing_offset(),packet->size()-frame.xing_offset());

    GM_DEBUG_PRINT("  nbytes: %d\n",xing->nbytes);
    GM_DEBUG_PRINT(" nframes: %d\n",xing->nframes);
    GM_DEBUG_PRINT("nsamples: %d\n",frame.nsamples());
    GM_DEBUG_PRINT("    rate: %d\n",frame.samplerate());

    const FXint lame_offset = frame.xing_offset()+XING_HEADER_SIZE;
    if (compare((const FXchar*)(packet->data()+lame_offset),"LAME",4)==0) {
      lame = new LameHeader(packet->data()+lame_offset,packet->size()-lame_offset);
      }
    }

  if (compare((const FXchar*)(packet->data()+frame.vbri_offset()),"VBRI",4)==0) {
    vbri = new VBRIHeader(packet->data()+frame.vbri_offset(),packet->size()-frame.vbri_offset());
    }

  if (xing || vbri || lame) {
    input_start = input_start + frame.size(); // start at next frame.
    if (xing) {
      stream_length = xing->nframes * frame.nsamples();
      if (input_end==-1 || (xing->nbytes>0 && input_start+xing->nbytes<=input_end))
        input_end = input_start+xing->nbytes;
      }
    else if (vbri) {
      stream_length = vbri->nframes * frame.nsamples();
      if (input_end==-1 || (vbri->nbytes>0 && input_start+vbri->nbytes<=input_end))
        input_end = input_start+vbri->nbytes;
      }
    else {
      GM_DEBUG_PRINT("[mad_reader] only lame header found\n");
      }

    if (lame) {
      GM_DEBUG_PRINT("[mad_reader] lame adjusting stream length by -%d frames \n",(lame->padstart + lame->padend));
      stream_length -= (lame->padstart + lame->padend);
      }

    }
  else {
    bitrate = frame.bitrate();
    if (bitrate>0 && input_end>input_start)
      stream_length =  (FXlong)frame.samplerate() * ((input_end-input_start) / (bitrate / 8) );
    }
  }

FXbool MadReader::parse_id3v1() {
  input->position(input_end-128,FXIO::Begin);

  if (input->read(buffer,3)!=3)
    return false;

  if (compare((FXchar*)buffer,"TAG",3)==0) {
    GM_DEBUG_PRINT("[mad_reader] found id3v1 tag\n");

    FXchar tag[125];
    if (input->read(tag,125)!=125)
      return false;

    id3v1 = new ID3V1(tag,125);

    input_end -= 128;
    }
  return true;
  }

FXbool MadReader::parse_ape() {

#if 0
  FXchar buf[32];

  input->position(input_end-32,FXIO::Begin);

  if (input->read(ape,32)!=32)
    return false;

  if (compare(ape,"APETAGEX",8)==0) {
    fxmessage("[mad_reader] found ape tag");

    FXint ape_version  = INT32_LE(buf+8);
    FXint ape_size     = INT32_LE(buf+12);
    FXint ape_flags    = INT32_LE(buf+20);

    FXuchar * ape_buffer=NULL;
    allocElms(ape_buffer,ape_size);

    if (input->read(ape_buffer+32,ape_size)!=ape_size) {
      return false;
      }
    memcpy(ape_buffer,buf,32);
    ape = new ApeTag(ape_buffer,ape_size+32);
    freeElms(ape_buffer;
    }
  return true;
  }
#endif
  FXchar buf[32];
  FXchar * ape = buf;

  input->position(input_end-32,FXIO::Begin);

  if (input->read(ape,32)!=32)
    return false;

  if (compare(ape,"APETAGEX",8)==0) {
    GM_DEBUG_PRINT("[mad_reader] found ape tag");

    FXint ape_version  = INT32_LE(buf+8);
    FXint ape_size     = INT32_LE(buf+12);
    FXint ape_flags    = INT32_LE(buf+20);
//    FXint ape_count    = INT32_LE(ape+16);
 //   FXint ape_reserved = INT32_LE(ape+24);

    enum {
      APE_HEADER          = (1<<29),
      APE_CONTAINS_FOOTER = (1<<30),
      APE_CONTAINS_HEADER = (1<<31)
      };

    if (ape_version==2000) {
      if (ape_flags&APE_HEADER) {
        /// We expect a footer
        GM_DEBUG_PRINT("mad_input: found ape tag header but expected a footer?\n");
        }
      else {
        if (ape_flags&APE_CONTAINS_HEADER)
          input_end = input_end - ape_size - 32;
        else
          input_end = input_end - ape_size;
        }
      }
    else if (ape_version==1000) {
      input_end = input_end - ape_size;
      }
    else {
      GM_DEBUG_PRINT("[mad_reader] unknown ape version %d\n",ape_version);
      }
    }
  return true;
  }




FXbool MadReader::parse_id3v2() {
  FXuchar info[6];

  if (input->read(info,6)!=6)
    return NULL;

  const FXuchar & id3v2_flags = info[1];
  FXint tagsize = ID3_SYNCSAFE_INT32(info+2);


  tagsize+=10;
  if (id3v2_flags&ID3V2::HAS_FOOTER) {
    tagsize+=10;
    }

  FXuchar * tagbuffer=NULL;
  allocElms(tagbuffer,tagsize);

  if (input->read(tagbuffer+10,tagsize-10)!=tagsize-10){
    freeElms(tagbuffer);
    return NULL;
    }

  memcpy(tagbuffer,buffer,4);
  memcpy(tagbuffer+4,info,6);

  id3v2 = new ID3V2(tagbuffer,tagsize);

  freeElms(tagbuffer);
  return true;
  }

FXbool MadReader::parse_lyrics() {
  FXchar buf[11];

  input->position(input_end-9,FXIO::Begin);

  if (input->read(buf,9)!=9)
    return false;

  if (comparecase(buf,"LYRICS200",9)==0){
    input->position(input_end-15,FXIO::Begin);
    if (input->read(buf,6)!=6)
      return false;

    FXint size = FXString(buf,6).toInt();
    input_end = input_end - (15 + size);
    }
  else if (comparecase(buf,"LYRICSEND",9)==0) {

    input->position(input_end-5100,FXIO::Begin);

    if (input->read(buf,11)!=11)
      return false;

    FXint i,nb=11;

    while(nb<5100) {

      /* Check if we found start of lyrics */
      if (buf[0]=='B' && comparecase(buf,"LYRICSBEGIN")==0) {
        input_end = input->position()-11;
        return true;
        }

      for (i=0;i<10;i++)
        buf[i]=buf[i+1];

      if (!input->read(&buf[10],1)!=1)
        return false;

      nb++;
      }
    }
  return true;
  }






void MadReader::set_replay_gain(ConfigureEvent* event) {
  if (id3v2 && !id3v2->replaygain.empty()) {
    GM_DEBUG_PRINT("[mad_reader] gain from id3v2\n");
    event->replaygain = id3v2->replaygain;
    }
  else if (lame && !lame->replaygain.empty()){
    GM_DEBUG_PRINT("[mad_reader] gain from lame\n");
    event->replaygain = lame->replaygain;
    }
  }

void MadReader::send_meta() {
  if (id3v2 && !id3v2->empty()) {
    GM_DEBUG_PRINT("[mad_reader] meta from id3v2\n");
    MetaInfo * meta = new MetaInfo;
    meta->artist.adopt(id3v2->artist);
    meta->album.adopt(id3v2->album);
    meta->title.adopt(id3v2->title);
    engine->decoder->post(meta);
    }
  else if (id3v1 && !id3v1->empty()) {
    GM_DEBUG_PRINT("[mad_reader] meta from id3v1\n");
    MetaInfo * meta = new MetaInfo;
    meta->artist.adopt(id3v1->artist);
    meta->album.adopt(id3v1->album);
    meta->title.adopt(id3v1->title);
    engine->decoder->post(meta);
    }
  }



ReadStatus MadReader::parse(Packet * packet) {
  mpeg_frame frame;
  FXbool found=false;

  stream_position=0;
  stream_length=-1;
  FXint nsamples=0;

  while(1) {

    if (sync) {
      buffer[0]=buffer[1];
      buffer[1]=buffer[2];
      buffer[2]=buffer[3];
      if (input->read(&buffer[3],1)!=1){
        return ReadError;
        }
      }
    else {
      if (input->read(buffer,4)!=4){
        return ReadError;
        }
      }

    if (frame.validate(buffer)) {


      /// Success if we're able to fill up the packet!
      if (frame.size()>packet->space()) {
        if (!found) return ReadError;
#if MAD_FLOAT_OUTPUT
        af.set(AP_FORMAT_FLOAT,frame.samplerate(),(frame.channel()==mpeg_frame::Single) ? 1 : 2);
#else
        af.set(AP_FORMAT_S16,frame.samplerate(),(frame.channel()==mpeg_frame::Single) ? 1 : 2);
#endif
        ConfigureEvent * cfg = new ConfigureEvent(af,Codec::MPEG);

        set_replay_gain(cfg);

        if (lame) {
          cfg->stream_offset_start = lame->padstart;
          cfg->stream_offset_end   = lame->padend;
          }
        engine->decoder->post(cfg);

        /// Send Meta Data if any
        send_meta();

        flags|=FLAG_PARSED;
        packet->af              = af;
        packet->stream_position = stream_position;
        packet->stream_length   = stream_length;
        stream_position        += nsamples;
        engine->decoder->post(packet);
        return ReadOk;
        }

      /// Mark this frame as start of our file.
      if (!found) {
        input_start = input->position() - 4;
        input_end   = input->size();
        }

      readFrame(packet,frame);

      if (!found) {

        if (!input->serial()) {

          if (!parse_id3v1())
            return ReadError;

          /*
             It's unspecified whether the lyrics frame comes
             before or after the ape frame, so check both
          */
          if (!parse_lyrics())
            return ReadError;

          if (!parse_ape())
            return ReadError;

          if (!parse_lyrics())
            return ReadError;

          input->position(input_start,FXIO::Begin);
          }
        parseFrame(packet,frame);
        if (xing||vbri||lame)
          packet->clear();
        else
          nsamples+=frame.nsamples();
        found=true;
        }
      else {
        nsamples+=frame.nsamples();
        }
      sync=false;
      continue;
      }
    else {
      found=false;
      nsamples=0;
      packet->clear();
      clear_headers();
      }


    if (buffer[0]=='I' && buffer[1]=='D' && buffer[2]=='3') {
      if (!parse_id3v2())
        return ReadError;
      sync=false;
      continue;
      }


    if (buffer[0]==0 && buffer[1]==0 && buffer[2]==0 && buffer[3]==0)
      sync=false;
    else
      sync=true;
    }
  return ReadError;
  }

ReadStatus MadReader::process(Packet*packet) {
  packet->af              = af;
  packet->stream_position = stream_position;
  packet->stream_length   = stream_length;

  if (!(flags&FLAG_PARSED)) {
    return parse(packet);
    }

  mpeg_frame frame;

  ReadStatus status=ReadOk;
  FXint nread;
  FXbool lostsync=false;

  while(1) {
    if (frame.validate(buffer)) {

      lostsync=false;

      if (frame.size()>packet->space())
        goto done;

      memcpy(packet->ptr(),buffer,4);
      nread = input->read(packet->ptr()+4,frame.size()-4);
      if (nread!=(frame.size()-4)) {
        GM_DEBUG_PRINT("[mad_reader] truncated frame at end of input.");
        packet->flags|=FLAG_EOS;
        status=ReadError;
        goto done;
        }
      packet->wroteBytes(frame.size());
      stream_position+=frame.nsamples();
      if (input->read(buffer,4)!=4){
        packet->flags|=FLAG_EOS;
        status=ReadError;
        goto done;
        }
      continue;
      }
    else {
      if (lostsync==false) {
        lostsync=true;
        GM_DEBUG_PRINT("[mad_reader] lost frame sync\n");
        }
      if (buffer[0]==0 && buffer[1]==0 && buffer[2]==0 && buffer[3]==0) {
        if (input->read(buffer,4)!=4){
          packet->flags|=FLAG_EOS;
          status=ReadError;
          goto done;
          }
        }
      else {
        buffer[0]=buffer[1];
        buffer[1]=buffer[2];
        buffer[2]=buffer[3];
        if (input->read(&buffer[3],1)!=1){
          packet->flags|=FLAG_EOS;
          status=ReadError;
          goto done;
          }
        }
      }
    }
done:
  if (packet->size() || packet->flags&FLAG_EOS)
    engine->decoder->post(packet);
  else {
    packet->unref();
    packet=NULL;
    }
  return status;
  }
























MadDecoder::MadDecoder(AudioEngine *e) : DecoderPlugin(e), buffer(MAD_BUFFER_MDLEN),flags(0) {
  out=NULL;
  }

MadDecoder::~MadDecoder(){
  if (out) {
    out->unref();
    out=NULL;
    }
  if (flags&FLAG_INIT) {
    mad_synth_finish(&synth);
    mad_stream_finish(&stream);
    mad_frame_finish(&frame);
    }
  }




FXbool MadDecoder::init(ConfigureEvent* event){
  FXASSERT(out==NULL);
  af=event->af;
  stream_offset_start=event->stream_offset_start;
  stream_offset_end=event->stream_offset_end;
  buffer.clear();

  if (out) {
    out->unref();
    out=NULL;
    }

  if (flags&FLAG_INIT) {
    mad_synth_mute(&synth);
    mad_frame_mute(&frame);

    mad_synth_finish(&synth);
    mad_frame_finish(&frame);
    mad_stream_finish(&stream);
    }

  mad_stream_init(&stream);
  mad_frame_init(&frame);
  mad_synth_init(&synth);

  flags|=FLAG_INIT;


  frame_counter=0;
  return true;
  }

#include <limits.h>

#ifdef MAD_FLOAT_OUTPUT
static FXfloat madfixed_to_float(mad_fixed_t fixed) {
//mad_f_todouble
static FXfloat m = exp2f(-MAD_F_FRACBITS);
/*
  FXfloat r1,r2,r3,r4;
  FXlong s1,s2,s3,s4,e1,e2,e3,e4;


  s1 = fxgetticks();
  r1=  (float) ((fixed) / (float) (1L << MAD_F_FRACBITS));
  e1 = fxgetticks();


  s2 = fxgetticks();
  r2=  (float) ((fixed) * exp2f(-MAD_F_FRACBITS));
  e2 = fxgetticks();

  s3 = fxgetticks();
  r3=  (float) ((fixed) * powf(2,-MAD_F_FRACBITS));
  e3 = fxgetticks();

  s4 = fxgetticks();
  r4=  (float) ((fixed) * m);
  e4 = fxgetticks();


  fxmessage("%ld %ld %ld %ld\n",e1-s1,e2-s2,e3-s3,e4-s4);
*/
  return  (float) ((fixed) * m);

//((x) * (double) (1L << MAD_F_FRACBITS) + 0.5))
//  return ((FXfloat)fixed) * (exp2(-MAD_F_FRACBITS));
  }

#else
static FXshort madfixed_to_s16(mad_fixed_t Fixed)
{
  /* A fixed point number is formed of the following bit pattern:
   *
   * SWWWFFFFFFFFFFFFFFFFFFFFFFFFFFFF
   * MSB                          LSB
   * S ==> Sign (0 is positive, 1 is negative)
   * W ==> Whole part bits
   * F ==> Fractional part bits
   *
   * This pattern contains MAD_F_FRACBITS fractional bits, one
   * should alway use this macro when working on the bits of a fixed
   * point number. It is not guaranteed to be constant over the
   * different platforms supported by libmad.
   *
   * The signed short value is formed, after clipping, by the least
   * significant whole part bit, followed by the 15 most significant
   * fractional part bits. Warning: this is a quick and dirty way to
   * compute the 16-bit number, madplay includes much better
   * algorithms.
   */




  /* round */
  Fixed += (1L << (MAD_F_FRACBITS - 16));

  /* clip */
  if (Fixed >= MAD_F_ONE)
    Fixed = MAD_F_ONE - 1;
  else if (Fixed < -MAD_F_ONE)
    Fixed = -MAD_F_ONE;

  /* quantize */
  return Fixed >> (MAD_F_FRACBITS + 1 - 16);

#if 0
  /* Clipping */
  if(Fixed>=MAD_F_ONE)
    return(SHRT_MAX);
  if(Fixed<=-MAD_F_ONE)
    return(-SHRT_MAX);

  /* Conversion. */
  Fixed=Fixed>>(MAD_F_FRACBITS-15);
  return((FXshort)Fixed);
#endif
}
#endif




DecoderStatus MadDecoder::process(Packet*in){
  FXASSERT(in);

  FXint p,s,n;
  FXuint streamid=in->stream;
  FXlong stream_length=in->stream_length;
  FXbool eos=(in->flags&FLAG_EOS);

  if (in->size() || eos){
    if (in->size()) {
      if (buffer.size()) {
        if (stream.next_frame!=NULL)
          buffer.setReadPosition(stream.next_frame);
        }
      else {
        stream_position=in->stream_position;
        }
      buffer.append(in->data(),in->size());
      }
    if (eos) buffer.append((FXchar)0,MAD_BUFFER_GUARD);
    mad_stream_buffer(&stream,buffer.data(),buffer.size());
    }


  if (buffer.size()==0) {
    GM_DEBUG_PRINT("[mad_decoder] empty buffer, nothing to decode\n");
    return DecoderOk;
    }

  stream.error=MAD_ERROR_NONE;

  in->unref();

  do {

    /// Decode a frame
    if (mad_frame_decode(&frame,&stream)) {
      if (MAD_RECOVERABLE(stream.error)) {
        continue;
        }
      else if(stream.error==MAD_ERROR_BUFLEN) {
        if (eos) {
          GM_DEBUG_PRINT("[mad_decoder] post end of stream %d\n",streamid);
          if (out && out->numFrames()) {
             if (stream_offset_end) {
              FXASSERT(out->numFrames()>=stream_offset_end); // FIXME
              out->trimFrames(FXMIN(out->numFrames(),stream_offset_end));
              }
             engine->output->post(out);
             out=NULL;
             }
          engine->output->post(new ControlEvent(End,streamid));
          }
        return DecoderOk;
        }
      else {
        GM_DEBUG_PRINT("[mad_decoder] %s\n",mad_stream_errorstr(&stream));
        return DecoderError;
        }
      }

    mad_synth_frame(&synth,&frame);

    frame_counter++;

    if (frame.header.samplerate!=af.rate) {
      GM_DEBUG_PRINT("[mad_decoder] sample rate changed: %d->%d \n",af.rate,frame.header.samplerate);
      }


    FXint nframes=synth.pcm.length;
    if (nframes==0)  { GM_DEBUG_PRINT("[mad_decoder] nframes == 0 ?\n"); continue; }

    mad_fixed_t * left  = synth.pcm.samples[0];
    mad_fixed_t * right = synth.pcm.samples[1];

    if (stream_position==0) {
      left+=stream_offset_start;
      right+=stream_offset_start;
      nframes-=stream_offset_start;
      GM_DEBUG_PRINT("[mad_decoder] skipping %d frames\n",stream_offset_start);
      }


    do {

      // Get new buffer
      if (out==NULL) {
        out = engine->decoder->get_output_packet();
        if (out==NULL) return DecoderInterrupted; // FIXME
        out->af=af;
        out->stream_position=stream_position;
        out->stream_length=stream_length;
        }

      n = FXMIN(out->availableFrames(),nframes);

#ifdef MAD_FLOAT_OUTPUT
      FXfloat * flt = out->flt();
      if (synth.pcm.channels==2) {
        for (p=0,s=0;s<n;s++) {
          flt[p++] = madfixed_to_float(*left++);
          flt[p++] = madfixed_to_float(*right++);
          }
        }
      else {
        for (p=0,s=0;s<n;s++) {
          flt[p++] = madfixed_to_float(*left++);
          }
        }
#else
      FXshort * buf16 = out->s16();
      if (synth.pcm.channels==2) {
        for (p=0,s=0;s<n;s++) {
          buf16[p++] = madfixed_to_s16(*left++);
          buf16[p++] = madfixed_to_s16(*right++);
          }
        }
      else {
        for (p=0,s=0;s<n;s++) {
          buf16[p++] = madfixed_to_s16(*left++);
          }
        }
#endif
      nframes-=n;
      out->wroteFrames(n);
      stream_position+=n;

      if (out->availableFrames()==0) {
        engine->output->post(out);
        out=NULL;
        }
      }
    while(nframes);
    }
  while(1);




  if (eos && out) {
    engine->output->post(out);
    out=NULL;
    }
  return DecoderOk;
  }

FXbool MadDecoder::flush(){
  if (out) {
    out->unref();
    out=NULL;
    }
  buffer.clear();
  if (flags&FLAG_INIT) {
    mad_synth_finish(&synth);
    mad_stream_finish(&stream);
    mad_frame_finish(&frame);
    }
  mad_synth_init(&synth);
  mad_stream_init(&stream);
  mad_frame_init(&frame);
  flags|=FLAG_INIT;
  return true;
  }



ReaderPlugin * ap_mad_reader(AudioEngine * engine) {
  return new MadReader(engine);
  }

DecoderPlugin * ap_mad_decoder(AudioEngine * engine) {
  return new MadDecoder(engine);
  }



}












