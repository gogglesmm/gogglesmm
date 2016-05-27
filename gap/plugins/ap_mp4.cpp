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
#include "ap_packet.h"
#include "ap_engine.h"
#include "ap_reader_plugin.h"
#include "ap_input_plugin.h"
#include "ap_decoder_thread.h"


namespace ap {
enum {
  AAC_FLAG_CONFIG = 0x2,
  AAC_FLAG_FRAME  = 0x4
  };


class Track {
  struct stts_entry {
    FXuint nsamples;
    FXuint delta;
    };

  struct stsc_entry {
    FXint first;
    FXint nsamples;
    FXint index;
    };

  struct ctts_entry {
    FXint nsamples;
    FXint offset;
    };

public:
  AudioFormat         af;                           // Audio Format
  FXuchar             codec;                        // Audio Codec
  FXArray<FXuint>     stsz;                         // samples size lookup table (in bytes)
  FXArray<FXuint>     stco;                         // chunk offset table
  FXArray<stts_entry> stts;                         // time to sample number lookup table
  FXArray<stsc_entry> stsc;                         // chunk-to-sample table
  FXArray<ctts_entry> ctts;
  FXuint              fixed_sample_size;            // used if all samples have the same size
  FXuchar*            decoder_specific_info;        // decoder specific info
  FXuint              decoder_specific_info_length; // decoder specific length
public:
  Track() : codec(Codec::Invalid),fixed_sample_size(0),decoder_specific_info(nullptr),decoder_specific_info_length(0) {}
  ~Track() { freeElms(decoder_specific_info); }

public:
  FXlong getChunkOffset(FXuint chunk,FXuint chunk_nsamples,FXuint sample) const {
    FXlong offset;
    if (stco.no())
      offset = stco[FXMIN(chunk,stco.no()-1)];
    else
      offset = 8;

    if (fixed_sample_size) {
      offset += (sample-chunk_nsamples)*fixed_sample_size;
      }
    else {
      for (FXuint i=chunk_nsamples;i<sample;i++) {
        offset+=stsz[i];
        }
      }
    return offset;
    }

  // Find the chunk that contains sample s. Also return nsamples at start of chunk
  void getChunk(FXuint s,FXuint & chunk,FXuint & chunk_nsamples) const{
    FXuint nchunks,nsamples,ntotal=0;
    for (FXint i=0;i<stsc.no()-1;i++) {
      nchunks  = (stsc[i+1].first - stsc[i].first);
      nsamples = nchunks * stsc[i].nsamples;
      if (s<ntotal+nsamples) {
        chunk          = stsc[i].first + ((s-ntotal)/stsc[i].nsamples) - 1;
        chunk_nsamples = ntotal + ((chunk+1)-stsc[i].first) * stsc[i].nsamples;
        return;
        }
      ntotal+=nsamples;
      }

    chunk          = stsc.tail().first + ((s-ntotal) / stsc.tail().nsamples) - 1;
    chunk_nsamples = ntotal + ((chunk+1) - stsc.tail().first) * stsc.tail().nsamples;
    }

  // Sample Offset
  FXint getCompositionOffset(FXlong position) const {
    FXint s = 0;
    for (int i=0;i<ctts.no();i++){
      if (position < s + ctts[i].nsamples){
        return ctts[i].offset;
        }
      s+=ctts[i].nsamples;
      }
    return 0;
    }

  FXint getSample(FXlong position) const {
    FXlong n,ntotal = 0;
    FXint nsamples = 0;
    for (int i=0;i<stts.no();i++){
      n = stts[i].nsamples*stts[i].delta;
      if (position<ntotal+n) {
        return nsamples+((position-ntotal)/stts[i].delta);
        }
      ntotal+=n;
      nsamples+=stts[i].nsamples;
      }
    return -1;
    }

  FXlong getSamplePosition(FXuint s) const {
    FXlong pos=0;
    FXuint nsamples=0;
    for (int i=0;i<stts.no();i++){
      if (s<(stts[i].nsamples+nsamples)){
        pos+=stts[i].delta*(s-nsamples);
        return pos;
        }
      else {
        pos+=stts[i].delta*stts[i].nsamples;
        }
      nsamples+=stts[i].nsamples;
      }
    return 0;
    }

  FXlong getLength() const {
    FXlong length=0;
    for (int i=0;i<stts.no();i++){
      length+=static_cast<FXlong>(stts[i].delta)*static_cast<FXlong>(stts[i].nsamples);
      }
    return length;
    }

  FXlong getSampleOffset(FXuint s) const {
    FXuint chunk,nsamples;
    getChunk(s,chunk,nsamples);
    return getChunkOffset(chunk,nsamples,s);
    }

  FXlong getSampleSize(FXuint s) const {
    if (fixed_sample_size)
      return fixed_sample_size;
    else
      return stsz[s];
    }

  FXuint getNumSamples() const {
    FXint nsamples = 0;
    for (FXint i=0;i<stts.no();i++) {
      nsamples += stts[i].nsamples;
      }
    return nsamples;
    }
  };


class MP4Reader : public ReaderPlugin {
protected:
#ifdef DEBUG
  FXint indent = 0;
#endif
  FXPtrListOf<Track> tracks;
  Track*             track = nullptr;
  MetaInfo*          meta = nullptr;
  FXushort           padstart = 0;
  FXushort           padend = 0;
protected:
  FXuint read_descriptor_length(FXuint&);
  FXbool atom_parse_asc(const FXuchar*,FXuint size);
  FXbool atom_parse_esds(FXlong size);
  FXbool atom_parse_mp4a(FXlong size);
  FXbool atom_parse_stsd(FXlong size);
  FXbool atom_parse_stco(FXlong size);
  FXbool atom_parse_stsc(FXlong size);
  FXbool atom_parse_stts(FXlong size);
  FXbool atom_parse_stsz(FXlong size);
  FXbool atom_parse_ctts(FXlong size);
  FXbool atom_parse_trak(FXlong size);
  //FXbool atom_parse_freeform(FXlong size);
  //FXbool atom_parse_text(FXlong size,FXString & value);
  FXbool atom_parse_meta(FXlong size);
  FXbool atom_parse_meta_text(FXlong size,FXString &);
  FXbool atom_parse_meta_free(FXlong size);
  FXbool atom_parse_header(FXuint & atom_type,FXlong & atom_size,FXlong & container);
  FXbool atom_parse(FXlong size);
protected:
  FXuint   sample = 0;      // current sample
  FXuint   nsamples = 0;    // number of samples
protected:
  ReadStatus parse(Packet * p);
  FXbool select_track();
  void clear_tracks();
public:
  MP4Reader(AudioEngine*);

  // Format
  FXuchar format() const override { return Format::MP4; };

  // Init
  FXbool init(InputPlugin*) override;

  // Seekable
  FXbool can_seek() const override;

  // Seek
  FXbool seek(FXlong) override;

  // Process Packet
  ReadStatus process(Packet*) override;

  // Destroy
  ~MP4Reader();
  };


ReaderPlugin * ap_mp4_reader(AudioEngine * engine) {
  return new MP4Reader(engine);
  }



MP4Reader::MP4Reader(AudioEngine* e) : ReaderPlugin(e),track(nullptr),meta(nullptr) {
  }

MP4Reader::~MP4Reader(){
  clear_tracks();
  }

FXbool MP4Reader::init(InputPlugin*plugin) {
  ReaderPlugin::init(plugin);
  flags&=~FLAG_PARSED;
#ifdef DEBUG
  indent=0;
#endif
  clear_tracks();
  nsamples=0;
  sample=0;
  if (meta) {
    meta->unref();
    meta=nullptr;
    }
  padstart=0;
  padend=0;
  return true;
  }

FXbool MP4Reader::can_seek() const {
  return true;
  }

FXbool MP4Reader::seek(FXlong offset){
  if (!input->serial()){
    FXint s = track->getSample(offset);
    if (s>=0) {
      sample = s;
      return true;
      }
    }
  return false;
  }

ReadStatus MP4Reader::process(Packet*packet) {
  packet->stream_position=-1;
  packet->stream_length=stream_length;
  packet->flags=AAC_FLAG_FRAME;

  if (!(flags&FLAG_PARSED)) {
    return parse(packet);
    }

  packet->stream_position = track->getSamplePosition(sample);

  for (;sample<nsamples;sample++) {
    FXlong size = track->getSampleSize(sample);
    if (size<0 || size>packet->capacity()) return ReadError;
    if (size>packet->space()){
      engine->decoder->post(packet);
      packet=nullptr;
      return ReadOk;
      }
    FXlong offset = track->getSampleOffset(sample);
    input->position(offset,FXIO::Begin);
    if (input->read(packet->ptr(),size)!=size){
      packet->unref();
      return ReadError;
      }
    packet->wroteBytes(size);

    if (sample==nsamples-1) {
      packet->flags|=FLAG_EOS;
      engine->decoder->post(packet);
      packet=nullptr;
      return ReadDone;
      }
    }
  return ReadOk;
  }


void MP4Reader::clear_tracks(){
  for (int i=0;i<tracks.no();i++)
    if (tracks[i]!=track)
      delete tracks[i];
  tracks.clear();
  delete track;
  track = nullptr;
  }


FXbool MP4Reader::select_track() {
  Track* selected = nullptr;
  for (FXint i=0;i<tracks.no();i++) {
    if (tracks[i]->codec!=Codec::Invalid) {
      selected = tracks[i];
      }
    }
  for (FXint i=0;i<tracks.no();i++){
    if (tracks[i]!=selected)
      delete tracks[i];
    }
  tracks.clear();
  track = selected;
  return (track!=nullptr);
  }


ReadStatus MP4Reader::parse(Packet * packet) {
  meta = new MetaInfo();

  if (atom_parse(input->size())) {

    if (!select_track()) {
      packet->unref();
      return ReadError;
      }

    FXASSERT(track);

    stream_length = track->getLength();
    nsamples      = track->getNumSamples();
    sample        = 0;

    af = track->af;

    ConfigureEvent * cfg = new ConfigureEvent(af,track->codec);


    if (track->codec==Codec::AAC) {
      /* FAAD seem to handle the encoder delay just fine, so all we need to do
         is adjust the stream length to account for the encoder delay. The offset end
         is already taken care of by the track->getLength() call. */
      if (padstart || padend) {
        stream_length -= padstart;
        }
      else if (stream_length && track->stts.no() && track->ctts.no()) {
        stream_length -= track->getCompositionOffset(0);
        }
      }

    engine->decoder->post(cfg);

    if (meta->title.length()) {
      engine->decoder->post(meta);
      meta = nullptr;
      }
    else {
      meta->unref();
      meta = nullptr;
      }

    packet->append(track->decoder_specific_info,track->decoder_specific_info_length);
    packet->flags|=AAC_FLAG_CONFIG|AAC_FLAG_FRAME;
    engine->decoder->post(packet);
    flags|=FLAG_PARSED;
    return ReadOk;
    }
  packet->unref();
  meta->unref();
  meta=nullptr;
  return ReadError;
  }



// Defined in reverse so we don't have to byteswap while reading on LE.
#define DEFINE_ATOM(b1,b2,b3,b4) ((b4<<24) | (b3<<16) | (b2<<8) | (b1))


enum Atom {

  ESDS = DEFINE_ATOM('e','s','d','s'),

  FREE = DEFINE_ATOM('f','r','e','e'),

  FTYP = DEFINE_ATOM('f','t','y','p'),

  ILST = DEFINE_ATOM('i','l','s','t'),

  MDAT = DEFINE_ATOM('m','d','a','t'),

  MOOV = DEFINE_ATOM('m','o','o','v'),

  MVHD = DEFINE_ATOM('m','v','h','d'),
  MDIA = DEFINE_ATOM('m','d','i','a'),
  MDHD = DEFINE_ATOM('m','d','h','d'),
  MINF = DEFINE_ATOM('m','i','n','f'),

  STBL = DEFINE_ATOM('s','t','b','l'),
  STSD = DEFINE_ATOM('s','t','s','d'),
  STSC = DEFINE_ATOM('s','t','s','c'),
  STSZ = DEFINE_ATOM('s','t','s','z'),
  STCO = DEFINE_ATOM('s','t','c','o'),
  STTS = DEFINE_ATOM('s','t','t','s'),
  CTTS = DEFINE_ATOM('c','t','t','s'),


  TRAK = DEFINE_ATOM('t','r','a','k'),
  UDTA = DEFINE_ATOM('u','d','t','a'),
  MP4A = DEFINE_ATOM('m','p','4','a'),
  ALAC = DEFINE_ATOM('a','l','a','c'),
  META = DEFINE_ATOM('m','e','t','a'),

  CART = DEFINE_ATOM(169,'A','R','T'),
  CALB = DEFINE_ATOM(169,'a','l','b'),
  CNAM = DEFINE_ATOM(169,'n','a','m'),
  CTOO = DEFINE_ATOM(169,'t','o','o'),

  MEAN = DEFINE_ATOM('m','e','a','n'),
  NAME = DEFINE_ATOM('n','a','m','e'),
  DATA = DEFINE_ATOM('d','a','t','a'),
  DDDD = DEFINE_ATOM('-','-','-','-')
  };


FXbool MP4Reader::atom_parse_trak(FXlong /*size*/) {
  track = new Track();
  tracks.append(track);
  return true;
  }


FXbool MP4Reader::atom_parse_mp4a(FXlong size) {
  FXuchar  mp4a_reserved[16];
  FXushort index;
  FXushort version;
  FXushort channels;
  FXushort samplesize;
  FXuint   samplerate;

  FXlong nbytes = size;

  if (track==nullptr)
    return false;

  if (input->read(&mp4a_reserved,6)!=6)
    return false;

  if (!input->read_uint16_be(index))
    return false;

  if (!input->read_uint16_be(version))
    return false;

  if (input->read(&mp4a_reserved,6)!=6)
    return false;

  if (!input->read_uint16_be(channels))
    return false;

  if (!input->read_uint16_be(samplesize))
    return false;

  if (input->read(&mp4a_reserved,4)!=4)
    return false;

  if (!input->read_uint32_be(samplerate))
    return false;

  samplerate = samplerate >> 16;

  track->af.set(AP_FORMAT_S16,samplerate,channels);
  track->codec = Codec::AAC;

  if (version==1) {
    if (input->read(&mp4a_reserved,16)!=16)
      return false;
    nbytes -= 16;
    }
  else if (version==2) {
    input->position(36,FXIO::Current);
    nbytes -= 36;
    }

  if (nbytes-28>0) {
    return atom_parse(nbytes-28);
    }
  return true;
  }


FXuint MP4Reader::read_descriptor_length(FXuint & length) {
  FXuchar b,nbytes=0;
  length=0;
  do {
    if (input->read(&b,1)!=1)
      return 0;
    nbytes++;
    length = (length<<7) | (b&0x7f);
    }
  while(b&0x80);
  return nbytes;
  }



class BitReader {
private:
  const FXuchar* buffer;
  FXuint   length;
  FXuint   position;
public:
  BitReader(const FXuchar * data,FXuint l) : buffer(data), length(l),position(0) {}

  FXuint read(FXint nbits) {
    FXuint value  = 0;
    FXuint offset,shift;
    FXuchar r;
    while(nbits) {
      offset = position / 8;
      shift  = position % 8;
      r=FXMIN(nbits,8);
      value<<=r;
      value|=(buffer[offset]<<shift)>>(8-r);
      nbits-=r;
      position+=r;
      }
    return value;
    }
  };


static const FXuint mp4_channel_map[]={
  Channel::FrontCenter, // maybe mono?

  AP_CHANNELMAP_STEREO,

  AP_CMAP3(Channel::FrontCenter,
           Channel::FrontLeft,
           Channel::FrontRight),

  AP_CMAP4(Channel::FrontCenter,
           Channel::FrontLeft,
           Channel::FrontRight,
           Channel::BackCenter),

  AP_CMAP5(Channel::FrontCenter,
           Channel::FrontLeft,
           Channel::FrontRight,
           Channel::BackLeft,
           Channel::BackRight),

  AP_CMAP6(Channel::FrontCenter,
           Channel::FrontLeft,
           Channel::FrontRight,
           Channel::BackLeft,
           Channel::BackRight,
           Channel::LFE),

  AP_CMAP8(Channel::FrontCenter,
           Channel::FrontLeft,
           Channel::FrontRight,
           Channel::SideLeft,
           Channel::SideRight,
           Channel::BackLeft,
           Channel::BackRight,
           Channel::LFE)
  };


FXbool MP4Reader::atom_parse_asc(const FXuchar * data,FXuint length) {
  BitReader bit(data,length);

  if (track==nullptr)
    return false;

  FXuint objtype = bit.read(5);

  if (objtype==31)
    objtype = 32 + bit.read(6);

  FXuint index = bit.read(4);
  if (index == 15)
    bit.read(24);

  FXuint channelconfig = bit.read(4);
  if (channelconfig>0 && channelconfig<8) {
    track->af.channelmap = mp4_channel_map[channelconfig-1];
    }
  return true;
  }


#define ESDescriptorTag            0x3
#define DecoderConfigDescriptorTag 0x4
#define DecoderSpecificInfoTag     0x5

FXbool MP4Reader::atom_parse_esds(FXlong size) {
  FXlong   nbytes = size;
  FXuint   version;
  FXushort esid;
  FXuchar  esflags;
  FXuint   length;
  FXuint   l;

  FXlong start = input->position();

  if (track==nullptr)
    return false;

  if (!input->read_uint32_be(version))
    return false;

  FXuchar tag;

  if (input->read(&tag,1)!=1)
    return false;

  if (tag==ESDescriptorTag) {

    length = read_descriptor_length(l);

    if (!input->read_uint16_be(esid))
      return false;

    if (!input->read(&esflags,1))
      return false;

    nbytes-=(length);
    }
  else {
    // fixme
    return false;
    }

  if (input->read(&tag,1)!=1)
    return false;

  if (tag!=DecoderConfigDescriptorTag)
    return false;

  nbytes -= read_descriptor_length(l);

  if (input->read(&tag,1)!=1)
    return false;

  if (tag==0x40 || tag==0x67)
    track->codec = Codec::AAC;

  if (!input->read_uint32_be(version))
    return false;

  FXuint avgbitrate;
  FXuint maxbitrate;

  if (!input->read_uint32_be(maxbitrate))
    return false;

  if (!input->read_uint32_be(avgbitrate))
    return false;

  if (input->read(&tag,1)!=1)
    return false;

  if (tag!=DecoderSpecificInfoTag)
    return false;

  nbytes -= read_descriptor_length(track->decoder_specific_info_length);

  if (track->decoder_specific_info_length > 0) {

    allocElms(track->decoder_specific_info,track->decoder_specific_info_length);

    if (input->read(track->decoder_specific_info,track->decoder_specific_info_length)!=track->decoder_specific_info_length)
      return false;

    if (!atom_parse_asc(track->decoder_specific_info,track->decoder_specific_info_length))
      return false;
    }

  FXlong end = input->position();

  if (end-start>0)
    input->position(size-(end-start),FXIO::Current);

  return true;
  }



FXbool MP4Reader::atom_parse_meta(FXlong size) {
  FXuint   version;

  if (track==nullptr)
    return false;

  if (!input->read_uint32_be(version))
    return false;

  if (!atom_parse(size-4))
    return false;

  return true;
  }



FXbool MP4Reader::atom_parse_meta_free(FXlong size) {
#ifdef DEBUG
  indent++;
#endif
  FXuint atom_type;
  FXlong atom_size;
  FXint length;

  FXint   type;
  FXshort county;
  FXshort language;

  FXString mean;
  FXString name;
  FXString data;

  while(size>=8 && atom_parse_header(atom_type,atom_size,size)){
    switch(atom_type){

      case MEAN:
          if (atom_size <= 4)
            return false;

          if (!input->read_int32_be(length))
            return false;

          mean.length(atom_size-4);
          if (input->read(mean.text(),atom_size-4)!=atom_size-4)
            return false;

          break;

      case NAME:

          if (atom_size <= 4)
            return false;

          if (!input->read_int32_be(length))
            return false;

          name.length(atom_size-4);
          if (input->read(name.text(),atom_size-4)!=atom_size-4)
            return false;

          break;

      case DATA:

          if (!input->read_int32_be(type))
            return false;

          if (type==1) { // UTF-8

            if (!input->read_int16_be(county))
              return false;

            if (!input->read_int16_be(language))
              return false;

            //if (type==1) {
              data.length(atom_size-8);
              if (input->read(data.text(),(atom_size-8))!=(atom_size-8))
                return false;
             // }
            }
          else {
            input->position(atom_size-4,FXIO::Current);
            }
          break;

      default : input->position(atom_size,FXIO::Current);
                break;
      }
    size-=atom_size;
    }

  GM_DEBUG_PRINT("%s.%s = %s\n",mean.text(),name.text(),data.text());
  if (name=="iTunSMPB") {
    FXlong duration;
    data.simplify().scan("%*x %hx %hx %lx",&padstart,&padend,&duration);
    }

#ifdef DEBUG
  indent--;
#endif
  return true;
  }



FXbool MP4Reader::atom_parse_meta_text(FXlong size,FXString & field) {
  FXint  length;
  FXchar id[4];
  FXint  type;
  FXshort  county;
  FXshort  language;

  if (!input->read_int32_be(length))
    return false;

  if (size!=length)
    return false;

  if (input->read(&id,4)!=4)
    return false;

  if (!input->read_int32_be(type))
    return false;

  if (type==1) {

    if (!input->read_int16_be(county))
      return false;

    if (!input->read_int16_be(language))
      return false;

    field.length(length-16);
    if (input->read(&field[0],(length-16))!=(length-16))
      return false;
#ifdef DEBUG
    fxmessage("%s\n",field.text());
#endif
    }
  else {
    input->position(length-12,FXIO::Current);
    }
  return true;
  }

FXbool MP4Reader::atom_parse_stsd(FXlong size) {
  FXuint   version;
  FXuint   nentries;

  if (track==nullptr)
    return false;

  if (!input->read_uint32_be(version))
    return false;

  if (!input->read_uint32_be(nentries))
    return false;

  if (!atom_parse(size-8))
    return false;

  return true;
  }


FXbool MP4Reader::atom_parse_stsc(FXlong /*size*/) {
  FXuint version;
  FXuint nentries;

  if (track==nullptr)
    return false;

  if (!input->read_uint32_be(version))
    return false;

  if (!input->read_uint32_be(nentries))
    return false;

  if (nentries) {
    track->stsc.no(nentries);
    for (FXuint i=0;i<nentries;i++) {
      if (!input->read_int32_be(track->stsc[i].first))
        return false;
      if (!input->read_int32_be(track->stsc[i].nsamples))
        return false;
      if (!input->read_int32_be(track->stsc[i].index))
        return false;
      }
    }
  //FXASSERT(size==((nentries*12)+8));
  return true;
  }



FXbool MP4Reader::atom_parse_stco(FXlong/*size*/) {
  FXuint   version;
  FXuint   nchunks;

  if (track==nullptr)
    return false;

  if (input->read(&version,4)!=4)
    return false;

  if (!input->read_uint32_be(nchunks))
    return false;

  if (nchunks>0) {
    track->stco.no(nchunks);
    for (FXuint i=0;i<nchunks;i++) {
      if (!input->read_uint32_be(track->stco[i]))
        return false;
      }
    }
  //FXASSERT(size==((nchunks*4)+8));
  return true;
  }

FXbool MP4Reader::atom_parse_stts(FXlong /*size*/) {
  FXuint   version;
  FXuint   nsize;

  if (track==nullptr)
    return false;

  if (input->read(&version,4)!=4)
    return false;

  if (!input->read_uint32_be(nsize))
    return false;

  if (nsize>0) {
    track->stts.no(nsize);
    for (FXuint i=0;i<nsize;i++) {
      if (!input->read_uint32_be(track->stts[i].nsamples)) return false;
      if (!input->read_uint32_be(track->stts[i].delta)) return false;
      GM_DEBUG_PRINT("stts %d: %d %d\n",i,track->stts[i].nsamples,track->stts[i].delta);
      }

    }
  //FXASSERT(size==((nsize*8)+8));
  return true;
  }


FXbool MP4Reader::atom_parse_ctts(FXlong /*size*/) {
  FXuint version;
  FXuint nentries;

  if (track==NULL)
    return false;

  if (!input->read_uint32_be(version))
    return false;

  if (!input->read_uint32_be(nentries))
    return false;

  if (nentries) {
    track->ctts.no(nentries);
    for (FXuint i=0;i<nentries;i++) {
      if (!input->read_int32_be(track->ctts[i].nsamples))
        return false;
      if (!input->read_int32_be(track->ctts[i].offset))
        return false;
      GM_DEBUG_PRINT("ctts %d: %d %d\n",i,track->ctts[i].nsamples,track->ctts[i].offset);
      }
    }
  //FXASSERT(size==((nentries*12)+8));
  return true;
  }



FXbool MP4Reader::atom_parse_stsz(FXlong /*size*/) {
  FXuint version;
  FXuint samplecount;

  if (track==nullptr)
    return false;

  if (input->read(&version,4)!=4)
    return false;

  if (!input->read_uint32_be(track->fixed_sample_size))
    return false;

  if (!input->read_uint32_be(samplecount))
    return false;

  if (track->fixed_sample_size==0 && samplecount>0) {
    track->stsz.no(samplecount);
    for (FXuint i=0;i<samplecount;i++) {
      if (!input->read_uint32_be(track->stsz[i]))
        return false;
      }
    }
  //FXASSERT(size==((nsamples*4)+12));
  return true;
  }

FXbool MP4Reader::atom_parse_header(FXuint & type,FXlong & size,FXlong & container) {
  FXuint sz;

  if (!input->read_uint32_be(sz))
    return false;

  if (input->read(&type,4)!=4)
    return false;

  if (sz==1) {
    if (!input->read_int64_be(size))
      return false;
    size -= 16;
    container -= 16;
    }
  else {
    size = sz - 8;
    container -= 8;
    }
#ifdef DEBUG
  for (int i=0;i<indent;i++) fxmessage("  ");
  fxmessage("%d-%c%c%c%c-%ld\n",indent,(type)&0xFF,(type>>8)&0xFF,(type>>16)&0xFF,(type>>24),size);
#endif
  return true;
  }



FXbool MP4Reader::atom_parse(FXlong size) {
#ifdef DEBUG
  indent++;
#endif
  FXuint atom_type;
  FXlong atom_size;
  FXbool ok;
  while(size>=8 && atom_parse_header(atom_type,atom_size,size)){
    switch(atom_type){

      // Don't go any further than the mdat atom in serial streams.
      case MDAT: if (input->serial())
                   return true;
                 input->position(atom_size,FXIO::Current); ok=true;
                 break;

      case TRAK: ok=atom_parse_trak(atom_size); // intentionally no break
                 if(!ok) return false;
      case MDIA:
      case MINF:
      case STBL:
      case UDTA:
      case ILST:
      case MOOV: ok=atom_parse(atom_size);
                 break;
      case META: ok=atom_parse_meta(atom_size); break;
      case STSZ: ok=atom_parse_stsz(atom_size); break;
      case STCO: ok=atom_parse_stco(atom_size); break;
      case STSD: ok=atom_parse_stsd(atom_size); break;
      case STSC: ok=atom_parse_stsc(atom_size); break;
      case STTS: ok=atom_parse_stts(atom_size); break;
      case CTTS: ok=atom_parse_ctts(atom_size); break;
      case MP4A: ok=atom_parse_mp4a(atom_size); break;
      case ESDS: ok=atom_parse_esds(atom_size); break;
      case DDDD: ok=atom_parse_meta_free(atom_size); break;
      case CART: ok=atom_parse_meta_text(atom_size,meta->artist); break;
      case CALB: ok=atom_parse_meta_text(atom_size,meta->album); break;
      case CNAM: ok=atom_parse_meta_text(atom_size,meta->title); break;
      //case CTOO: ok=atom_parse_meta_text(atom_size,meta->title); break;
      default  : input->position(atom_size,FXIO::Current); ok=true;
                 break;
      }
    if (!ok) return false;
    size-=atom_size;
    }
#ifdef DEBUG
  indent--;
#endif
  return true;
  }




}



