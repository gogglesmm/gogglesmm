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
public:
  AudioFormat         af;                           // Audio Format
  FXuchar             codec;                        // Audio Codec
  FXArray<FXuint>     stsz;                         // samples size lookup table (in bytes)
  FXArray<FXuint>     stco;                         // chunk offset table
  FXArray<stts_entry> stts;                         // time to sample number lookup table 
  FXArray<stsc_entry> stsc;                         // chunk-to-sample table
  FXuint              fixed_sample_size;            // used if all samples have the same size
  FXuchar*            decoder_specific_info;        // decoder specific info
  FXuint              decoder_specific_info_length; // decoder specific length
public:
  Track() : codec(Codec::Invalid),decoder_specific_info(NULL),decoder_specific_info_length(0) {}
  ~Track() { freeElms(decoder_specific_info); }

public:
  FXlong getChunkOffset(FXuint chunk,FXuint chunk_nsamples,FXuint sample) {
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
  void getChunk(FXuint s,FXuint & chunk,FXuint & chunk_nsamples) {
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

  FXint getSample(FXlong position) {
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

  FXlong getSamplePosition(FXuint s) {
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
 
  FXlong getLength() {
    FXlong length=0;
    for (int i=0;i<stts.no();i++){
      length+=stts[i].delta*stts[i].nsamples;
      }
    return length; 
    }


  FXlong getSampleOffset(FXuint s) {
    FXuint chunk,nsamples;
    getChunk(s,chunk,nsamples);
    return getChunkOffset(chunk,nsamples,s);
    }

  FXlong getSampleSize(FXuint s) {
    if (fixed_sample_size)
      return fixed_sample_size;
    else
      return stsz[s];
    }

  FXuint getNumSamples() {
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
  FXint indent;
#endif  
  FXPtrListOf<Track> tracks;
  Track*             track;
  MetaInfo*          meta;  
protected:
  FXuint read_descriptor_length(FXuint&);
  FXbool atom_parse_esds(FXlong size);
  FXbool atom_parse_mp4a(FXlong size);
  FXbool atom_parse_stsd(FXlong size);
  FXbool atom_parse_stco(FXlong size);
  FXbool atom_parse_stsc(FXlong size);
  FXbool atom_parse_stts(FXlong size);
  FXbool atom_parse_stsz(FXlong size);
  FXbool atom_parse_trak(FXlong size);
  //FXbool atom_parse_freeform(FXlong size);
  //FXbool atom_parse_text(FXlong size,FXString & value);
  FXbool atom_parse_meta(FXlong size);
  FXbool atom_parse_meta_text(FXlong size,FXString &);
  FXbool atom_parse_header(FXuint & atom_type,FXlong & atom_size,FXlong & container);
  FXbool atom_parse(FXlong size);
protected:
  FXuint   sample;      // current sample
  FXuint   nsamples;    // number of samples
protected:
  ReadStatus parse(Packet * p);
  FXbool select_track();
  void clear_tracks();
public:
  MP4Reader(AudioEngine*);

  // Format
  FXuchar format() const { return Format::MP4; };

  // Init
  FXbool init(InputPlugin*);

  // Seekable
  FXbool can_seek() const;

  // Seek
  FXbool seek(FXdouble);

  // Process Packet
  ReadStatus process(Packet*); 

  // Destroy
  ~MP4Reader();
  };


ReaderPlugin * ap_mp4_reader(AudioEngine * engine) {
  return new MP4Reader(engine);
  }



MP4Reader::MP4Reader(AudioEngine* e) : ReaderPlugin(e),track(NULL) {
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
    meta=NULL;
    }
  return true;
  }

FXbool MP4Reader::can_seek() const {
  return true;
  }

FXbool MP4Reader::seek(FXdouble pos){
  if (!input->serial()){
    FXint s = track->getSample(pos*stream_length);
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
    if (size<0) return ReadError;
    if (size>packet->space()){
      engine->decoder->post(packet);
      packet=NULL;
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
      packet=NULL;
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
  track = NULL;      
  }


FXbool MP4Reader::select_track() {
  Track* selected = NULL;
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
  return (track!=NULL);
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
    nsamples = track->getNumSamples();
    sample   = 0;

    af = track->af;
    engine->decoder->post(new ConfigureEvent(af,track->codec));  

    if (meta->title.length()) {
      engine->decoder->post(meta);
      meta = NULL;
      }
    else {
      meta->unref();
      meta = NULL;
      }

    packet->append(track->decoder_specific_info,track->decoder_specific_info_length);
    packet->flags|=AAC_FLAG_CONFIG|AAC_FLAG_FRAME;
    engine->decoder->post(packet);
    flags|=FLAG_PARSED;
    return ReadOk;
    }
  packet->unref();
  meta->unref();
  meta=NULL;
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

  TRAK = DEFINE_ATOM('t','r','a','k'),
  UDTA = DEFINE_ATOM('u','d','t','a'),
  MP4A = DEFINE_ATOM('m','p','4','a'),
  ALAC = DEFINE_ATOM('a','l','a','c'),
  META = DEFINE_ATOM('m','e','t','a'),

  CART = DEFINE_ATOM(169,'A','R','T'),
  CALB = DEFINE_ATOM(169,'a','l','b'),
  CNAM = DEFINE_ATOM(169,'n','a','m')
/*
  MEAN = DEFINE_ATOM('m','e','a','n'),
  NAME = DEFINE_ATOM('n','a','m','e'),
  DATA = DEFINE_ATOM('d','a','t','a'),
  DDDD = DEFINE_ATOM('-','-','-','-')
*/
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

  if (track==NULL)
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


#define ESDescriptorTag            0x3
#define DecoderConfigDescriptorTag 0x4 
#define DecoderSpecificInfoTag     0x5
         
FXbool MP4Reader::atom_parse_esds(FXlong size) {
  FXlong   nbytes = size;
  FXuint   version;
  FXushort esid;
  FXuchar  flags;
  FXuint   length;
  FXuint   l;

  FXlong start = input->position();

  if (track==NULL)
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

    if (!input->read(&flags,1))
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

  length = read_descriptor_length(l);
  nbytes-=(length);


  if (input->read(&tag,1)!=1)
    return false;

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

  length = read_descriptor_length(l);
  nbytes-=(length);

  if (length) {
    allocElms(track->decoder_specific_info,length);
    track->decoder_specific_info_length = length;
    if (input->read(track->decoder_specific_info,length)!=length)
      return false;
    }

  FXlong end = input->position();
 
  if (end-start>0)
    input->position(size-(end-start),FXIO::Current);
 
  return true;
  }
  


FXbool MP4Reader::atom_parse_meta(FXlong size) {
  FXuint   version;

  if (track==NULL)
    return false;

  if (!input->read_uint32_be(version)) 
    return false;

  if (!atom_parse(size-4))
    return false;

  return true;
  }

#if 0
FXbool MP4Reader::atom_parse_text(FXlong size,FXString & value) {
  FXuint version;

  if (!input->read_uint32_be(version)) 
    return false;

  value.length(size-4);
  if (input->read(&value[0],size-4)!=value.length())
    return false;

  return true;
  }


FXbool MP4Reader::atom_parse_freeform(FXlong size) {
#ifdef DEBUG
  indent++;
#endif
  FXString mean,name;
  FXuint version;
  FXuint atom_type;
  FXlong atom_size;

  FXbool ok;
  while(size>=8 && atom_parse_header(atom_type,atom_size,size)){
    switch(atom_type) {
      case MEAN: ok = atom_parse_text(atom_size,mean); fxmessage("mean: %s\n",mean.text()); break;               
      case NAME: ok = atom_parse_text(atom_size,name); fxmessage("name: %s\n",name.text());  break;
      case DATA: input->position(atom_size,FXIO::Current); ok=true; break;
      }
    if (!ok) return false;
    size-=atom_size;    
    }
#ifdef DEBUG
  indent--;
#endif
  return true;
  }
#endif

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

  if (!input->read_int16_be(county))
    return false;

  if (!input->read_int16_be(language))
    return false;

  field.length(length-16);
  if (input->read(&field[0],(length-16))!=(length-16))
    return false;

  return true;
  }

FXbool MP4Reader::atom_parse_stsd(FXlong size) {
  FXuint   version;
  FXuint   nentries;      

  if (track==NULL)
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

  if (track==NULL)
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

  if (track==NULL)
    return false;

  if (input->read(&version,4)!=4) 
    return false;

  if (!input->read_uint32_be(nchunks))
    return false;

  if (nchunks>0) {
    track->stco.no(nchunks);    
    for (FXuint i=0;i<nchunks;i++) {
      input->read_uint32_be(track->stco[i]);
      }
    }
  //FXASSERT(size==((nchunks*4)+8));
  return true;
  }

FXbool MP4Reader::atom_parse_stts(FXlong /*size*/) {
  FXuint   version;
  FXuint   nsize;      

  if (track==NULL)
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
      }
    }
  //FXASSERT(size==((nsize*8)+8));
  return true;
  }


FXbool MP4Reader::atom_parse_stsz(FXlong /*size*/) {
  FXuint   version;
  FXuint   nsamples;      

  if (track==NULL)
    return false;

  if (input->read(&version,4)!=4) 
    return false;

  if (!input->read_uint32_be(track->fixed_sample_size))
    return false;

  if (!input->read_uint32_be(nsamples))
    return false;

  if (track->fixed_sample_size==0 && nsamples>0) {
    track->stsz.no(nsamples);    
    for (FXuint i=0;i<nsamples;i++) {
      input->read_uint32_be(track->stsz[i]);
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
      case MP4A: ok=atom_parse_mp4a(atom_size); break;
      case ESDS: ok=atom_parse_esds(atom_size); break;
      case CART: ok=atom_parse_meta_text(atom_size,meta->artist); break;
      case CALB: ok=atom_parse_meta_text(atom_size,meta->album); break;
      case CNAM: ok=atom_parse_meta_text(atom_size,meta->title); break;
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



