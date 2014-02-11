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


struct stts_entry {
  FXuint nsamples;
  FXuint delta;
  };

class Track {
public:

  struct stsc_entry {
    FXint first;
    FXint nsamples;
    FXint index;
    };

public:
  FXuchar             codec;                  // Audio Codec
  FXArray<FXuint>     stsz;                   // samples size lookup table (in bytes)
  FXArray<FXuint>     stco;                   // chunk offset table
  FXArray<stts_entry> stts;                   // time to sample number lookup table 
  FXArray<stsc_entry> stsc;                   // chunk-to-sample table

  FXuint              fixed_sample_size;
  FXArray<FXuchar>    decoder_specific_info;
  AudioFormat         af;


public:
  FXuint sample_size(FXuint s) {
    if (fixed_sample_size)
      return fixed_sample_size;
    else
      return stsz[s];
    }

  FXuint get_chunk_for_sample(FXuint s) {
    fxmessage("get_chunk_for_sample %d\n",s);
    FXint nchunks,nsamples,ntotal=0;;
    for (FXint i=0;i<stsc.no()-1;i++){
      nchunks  = (stsc[i+1].first - stsc[i].first);
      nsamples = nchunks * stsc[i].nsamples;  
      if (s<ntotal+nsamples) {
        fxmessage("found chunk in range %d-%d=%d = %d\n",stsc[i].first,stsc[i+1].first,stsc[i].nsamples,stsc[i].first + ((s-ntotal) / stsc[i].nsamples));
        return (s-ntotal) / stsc[i].nsamples;
        }
      ntotal+=nsamples;
      }    
    fxmessage("found s in last range %d-%d = %d\n",stsc.tail().first,stsc[stsc.no()-1].nsamples,stsc[stsc.no()-1].first + ((s-ntotal) / stsc[stsc.no()-1].nsamples));
    return stsc[stsc.no()-1].first + ((s-ntotal) / stsc[stsc.no()-1].nsamples);
    }

  FXlong get_chunk_offset(FXuint c) {
    if (stco.no()) {
      return stco[FXMIN(stco.no(),c];
      }
    else {
      return 0;
      }
    }



  };


class MP4Reader : public ReaderPlugin {
protected:
  Packet * packet;

protected:
#ifdef DEBUG
  FXint indent;

#endif  
  FXPtrListOf<Track> tracks;
  Track*           track;  
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
  FXbool atom_parse_header(FXuint & atom_type,FXlong & atom_size);
  FXbool atom_parse(FXlong size);
protected:
  ReadStatus parse();
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



MP4Reader::MP4Reader(AudioEngine* e) : ReaderPlugin(e) {
  }

MP4Reader::~MP4Reader(){
  }

FXbool MP4Reader::init(InputPlugin*plugin) {
  ReaderPlugin::init(plugin);
  flags&=~FLAG_PARSED;
#ifdef DEBUG
  indent=0;
#endif

  for (int i=0;i<tracks.no();i++)
    delete tracks[i];
  tracks.clear();
  track=NULL;

  return true;
  }


FXbool MP4Reader::can_seek() const {
  return false;
  }

FXbool MP4Reader::seek(FXdouble pos){
  return false;
  }

ReadStatus MP4Reader::process(Packet*p) {
  packet = p;
  packet->stream_position=-1;
  packet->stream_length=-1;

  if (!(flags&FLAG_PARSED)) {
    return parse();
    }

  return ReaderPlugin::process(p);
  }



ReadStatus MP4Reader::parse() {
  if (atom_parse(input->size())) {

    if (tracks.no()==0) 
      return ReadError;

    // fixme get best track

    af = track->af;
    engine->decoder->post(new ConfigureEvent(af,track->codec));
  
  //  fxmessage("chunk offset %d\n",track->stco[0]);
    packet->append(&track->decoder_specific_info[0],track->decoder_specific_info.no());
    packet->flags|=AAC_FLAG_CONFIG|AAC_FLAG_FRAME;
    engine->decoder->post(packet);
    input->position(track->stco[0],FXIO::Begin);




    fxmessage("size of sample %d\n",track->sample_size(0));
    track->get_chunk_for_sample(0);

    track->get_chunk_for_sample(9460);





    }
  af = track->af;
  flags|=FLAG_PARSED;
  return ReadOk;
  }



// Defined in reverse so we don't have to byteswap while reading on LE.
#define DEFINE_ATOM(b1,b2,b3,b4) ((b4<<24) | (b3<<16) | (b2<<8) | (b1))

enum Atom {

  ESDS = DEFINE_ATOM('e','s','d','s'),

  FREE = DEFINE_ATOM('f','r','e','e'),
  
  FTYP = DEFINE_ATOM('f','t','y','p'),
  
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
  };


FXbool MP4Reader::atom_parse_trak(FXlong size) {
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
  FXuint   atom_type;
  FXlong   atom_size;

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


#define ES_DescrTag           0x3
#define DecoderConfigDescrTag 0x4 
#define DecSpecificInfoTag    0x5
         
FXbool MP4Reader::atom_parse_esds(FXlong size) {
  FXlong   nbytes = size;
  FXuint   version;
  FXushort esid;
  FXuchar  flags;
  FXuchar  reserved[3];
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

  if (tag==ES_DescrTag) {

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

  if (tag!=DecoderConfigDescrTag)
    return false;

  length = read_descriptor_length(l);
  nbytes-=(length);

  fxmessage("len %d\n",length); 

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

  if (tag!=DecSpecificInfoTag)
    return false;

  length = read_descriptor_length(l);
  nbytes-=(length);

  track->decoder_specific_info.no(length);
  if (input->read(&track->decoder_specific_info[0],length)!=length)
    return false;
    
  FXlong end = input->position();
  fxmessage("%ld / %ld / %ld\n",size-nbytes,end-start,size);
 
  if (end-start>0)
    input->position(size-(end-start),FXIO::Current);
 
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


FXbool MP4Reader::atom_parse_stsc(FXlong size) {
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
  FXASSERT(size==((nentries*12)+8));
  return true;   
  }



FXbool MP4Reader::atom_parse_stco(FXlong size) {
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
  FXASSERT(size==((nchunks*4)+8));
  return true;
  }

FXbool MP4Reader::atom_parse_stts(FXlong size) {
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
      input->read_uint32_be(track->stts[i].nsamples);
      input->read_uint32_be(track->stts[i].delta);
      }
    }
  FXASSERT(size==((nsize*8)+8));
  return true;
  }


FXbool MP4Reader::atom_parse_stsz(FXlong size) {
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
  FXASSERT(size==((nsamples*4)+12));
  return true;
  }

FXbool MP4Reader::atom_parse_header(FXuint & type,FXlong & size) {
  FXuint sz;

  if (!input->read_uint32_be(sz)) 
    return false;

  if (input->read(&type,4)!=4)
    return false;

  if (sz==1) {
    if (!input->read_int64_be(size))
      return false;
    size -= 16;
    }
  else {
    size = sz - 8;
    }
#ifdef DEBUG
  for (int i=0;i<indent;i++) fxmessage("  ");
  fxmessage("%d-%c%c%c%c-%ld\n",indent,(type)&0xFF,(type>>8)&0xFF,(type>>16)&0xFF,(type>>24),size);
#endif 
  return true;
  }



FXbool MP4Reader::atom_parse(FXlong size) {
  indent++;
  fxmessage("atom_parse %d\n",indent);
  FXuint atom_type;
  FXlong atom_size;
  while(size>=8 && atom_parse_header(atom_type,atom_size)){
    switch(atom_type){
      case TRAK: atom_parse_trak(atom_size); // intentionally no break
      case MDIA:
      case MINF: 
      case STBL:
      case MOOV: atom_parse(atom_size);      
                 break;
      case STSZ: atom_parse_stsz(atom_size); break;
      case STCO: atom_parse_stco(atom_size); break;
      case STSD: atom_parse_stsd(atom_size); break;
      case STSC: atom_parse_stsc(atom_size); break;
      case STTS: atom_parse_stts(atom_size); break;
      case MP4A: atom_parse_mp4a(atom_size); break;
      case ESDS: atom_parse_esds(atom_size); break;
      default  : input->position(atom_size,FXIO::Current); break;
      }
    for (int i=0;i<indent;i++) fxmessage("  ");
    fxmessage("%d-%ld\n",indent,atom_size);
    size-=atom_size;
    //fxmessage("size left: %ld\n",size);
    }
  indent--;
  return true;
  }




}



