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
#include "ap_packet.h"
#include "ap_engine.h"
#include "ap_reader_plugin.h"
#include "ap_input_plugin.h"
#include "ap_decoder_thread.h"

#include "ap_vorbis.h"
#include "ap_opus.h"

#ifdef HAVE_OGG
#include <ogg/ogg.h>
#endif

enum {
  EBML                        = 0x1a45dfa3,
  EBML_VERSION                = 0x4286,
  EBML_READ_VERSION           = 0x42f7,
  EBML_MAX_ID_LENGTH          = 0x42f2,
  EBML_MAX_SIZE_LENGTH        = 0x42f3,
  EBML_DOC_TYPE               = 0x4282,
  EBML_DOC_TYPE_VERSION       = 0x4287,
  EBML_DOC_TYPE_READ_VERSION  = 0x4285,
  SEGMENT                     = 0x18538067,
  SEEK_HEAD                   = 0x114d9b74,
  SEEK                        = 0x4dbb,
  SEEK_ID                     = 0x53ab,
  SEEK_POSITION               = 0x53ac,
  TRACK                       = 0x1654ae6b,
  TRACK_ENTRY                 = 0xae,
  TRACK_TYPE                  = 0x83,
  TRACK_NUMBER                = 0xd7,
  CODEC_ID                    = 0x86,
  CODEC_PRIVATE               = 0x63a2,
  AUDIO                       = 0xe1,
  AUDIO_SAMPLE_RATE           = 0xb5,
  AUDIO_CHANNELS              = 0x9f,
  AUDIO_BITDEPTH              = 0x6264,
  CLUSTER                     = 0x1f43b675,
  CLUSTER_PREVSIZE            = 0xab,
  CLUSTER_POSITION            = 0xa7,
  BLOCKGROUP                  = 0xa0,
  BLOCK                       = 0xa1,
  REFERENCE_BLOCK             = 0xfb,
  SIMPLEBLOCK                 = 0xa3,
  TIMECODE                    = 0xe7,
  SEGMENT_INFO                = 0x1549a966,
  SEGMENT_INFO_TIMECODE_SCALE = 0x2ad7b1,
  SEGMENT_INFO_DURATION       = 0x4489,
  VOID                        = 0xec,
  CUES                        = 0x1c53bb6b,
  CUE_POINT                   = 0xbb,
  CUE_TIME                    = 0xb3,
  CUE_TRACK_POSITIONS         = 0xb7,
  CUE_TRACK                   = 0xf7,
  CUE_CLUSTER_POSITION        = 0xf1,
  CUE_RELATIVE_POSITION       = 0xf0,
  };


namespace ap {

namespace matroska {


struct Element {
  FXuint type   = 0;
  FXlong size   = 0;
  FXlong offset = 0;

  Element(){}
  Element(FXlong sz) : size(sz) {}

  void reset() { type=0; size=0; offset=0; }

  void debug(const FXchar * section) const { fxmessage("%s: 0x%x (%ld bytes)\n",section,type,size); }
  };

struct Block {
  FXlong position;
  FXuint frames[16]={0};
  FXuint nframes = 0;
  inline FXuint next() {
    return frames[--nframes];
    }
  void reset() { nframes=0; }
  };


class Track {
public:


  struct cue_entry {
    FXlong position;
    FXlong cluster;
    };


public:
  AudioFormat af;
  DecoderConfig * dc = nullptr;
  FXuchar     codec  = Codec::Invalid;
  FXulong     number = 0;
  FXArray<cue_entry> cues;
  FXint             ncues=0;

  void add_cue_entry(FXlong pos,FXlong cluster) {
    if (cues.no()>=ncues) {
      cues.no(cues.no()+256);
      }
    cues[ncues].position = pos;
    cues[ncues].cluster  = cluster;
    ncues++;
    }
  };

/* Layout

  Clusters -> Block -> Frame



*/


class MatroskaReader : public ReaderPlugin {
protected:
  FXPtrListOf<Track> tracks;
  Track*             track=nullptr;
protected:
  Block   block;   // current block
  Element cluster; // current cluster
  Element group;   // current group
protected:
  MemoryBuffer data;
protected:
  FXbool is_webm = false;




  FXlong  stream_position = 0;
  FXulong timecode_scale  = 1000000;
  FXulong duration        = 0;

  FXlong  first_cluster = 0;
  FXlong  cluster_size = 0;
  FXuint  frame_size = 0;
  //FXlong  packetno = 0;





protected:
  ReadStatus parse(Packet * p);


  FXuchar parse_ebml_uint64(FXlong & value);
  FXbool  parse_element_uint64(Element & container,FXlong & element);
  FXbool  parse_element_int64(Element & container,FXlong & element);
  FXbool  parse_element_id(Element & container,Element & element);

  FXbool parse_element(Element & element);
  FXbool parse_element(Element & parent,Element & element,FXbool allow_unknown_size=false);

  FXbool parse_ebml(Element&);
  FXbool parse_segment(Element &);
  FXbool parse_segment_info(Element &);
  FXbool parse_seekhead(Element&);
  FXbool parse_seek(Element&);
  FXbool parse_track(Element&);
  FXbool parse_track_entry(Element&);
  FXbool parse_track_audio(Element&);
  FXbool parse_track_codec(Element&);


  FXbool parse_uint8(FXuchar & value,const FXlong size);
  FXbool parse_uint64(FXulong & value,const FXlong size);
  FXbool parse_unsigned_int(FXulong & value,const FXlong size);
  FXbool parse_float_as_uint32(FXuint & value,const FXlong size);


  FXbool parse_xiph_lace(Element &, FXuint & framesize);
  FXbool get_next_frame(FXuint & framesize);
  FXbool parse_block_group(Element&);
  FXbool parse_simpleblock(Element&);


  FXbool parse_cues(Element&);
  FXbool parse_cue_point(Element&);
  FXbool parse_cue_track(Element&,FXulong & track,FXulong & cluster);

protected:
  void clear_tracks();
public:
  enum {
    OGG_WROTE_HEADER = 0x2,
    };
public:
  MatroskaReader(AudioEngine*);

  // Format
  FXuchar format() const { return Format::Matroska; };

  // Init
  FXbool init(InputPlugin*);

  // Seekable
  FXbool can_seek() const;

  // Seek
  FXbool seek(FXlong );

  // Process Packet
  ReadStatus process(Packet*);

  // Destroy
  ~MatroskaReader();
  };


MatroskaReader::MatroskaReader(AudioEngine* e) : ReaderPlugin(e) {
  }

MatroskaReader::~MatroskaReader(){
  clear_tracks();
  }


void MatroskaReader::clear_tracks(){
  for (int i=0;i<tracks.no();i++)
    if (tracks[i]!=track)
      delete tracks[i];
  tracks.clear();
  delete track;
  track = nullptr;
  }


FXbool MatroskaReader::init(InputPlugin*plugin) {
  ReaderPlugin::init(plugin);
  flags&=~FLAG_PARSED;
  clear_tracks();

  timecode_scale  = 1000000;
  first_cluster   = 0;
  frame_size      = 0;
  stream_position = 0;
  duration        = 0;
  cluster.reset();
  block.reset();
  group.reset();
  return true;
  }

FXbool MatroskaReader::can_seek() const {
  return true;
  }

FXbool MatroskaReader::seek(FXlong offset){
  if (track->codec==Codec::Opus)
    offset = FXMAX(0,offset-3840);

  FXlong target  = (offset * 1000000000) /  af.rate;
  FXlong lastpos = 0;
  FXlong timestamp = 0;
  for (FXint i=0;i<track->ncues;i++) {
    fxmessage("target %ld vs %ld\n",target,track->cues[i].position*timecode_scale);
    if (target<track->cues[i].position*timecode_scale) {
      lastpos = track->cues[i].cluster;
      timestamp = track->cues[i].position*timecode_scale;
      continue;
      }
    break;
    }
  input->position(first_cluster+lastpos,FXIO::Begin);
  stream_position = (timestamp * track->af.rate) / 1000000000;

  frame_size=0;
  cluster.reset();
  block.reset();
  group.reset();
  return true;
  }

ReadStatus MatroskaReader::process(Packet*packet) {

  if (!(flags&FLAG_PARSED)) {
    return parse(packet);
    }

  //packet->stream_position=stream_position;
  packet->stream_length=stream_length;

  FXuint element_type;

  packet->af=af;

  while(packet->space()) {

    if (frame_size) {

      switch(track->codec) {
        case Codec::MPEG:
          {
            if(frame_size > packet->space())
              break;
            if (input->read(packet->ptr(),frame_size)!=frame_size)
              return ReadError;
            packet->wroteBytes(frame_size);
            frame_size-=frame_size;
            break;
          }
        case Codec::PCM:
          {
            FXint n = FXMIN(frame_size,packet->availableFrames()*af.framesize());
            if (input->read(packet->ptr(),n)!=n)
              return ReadError;
            packet->wroteBytes(n);
            frame_size-=n;
            break;
          }
        case Codec::AAC:
          {
            if(frame_size > packet->space())
              break;
            if (input->read(packet->ptr(),frame_size)!=frame_size)
              return ReadError;
            packet->wroteBytes(frame_size);
            frame_size-=frame_size;
            break;
          }
        case Codec::Vorbis:
        case Codec::Opus:
          {
            if ( ((frame_size+4)<=packet->space()) ||
                 ((packet->space()>=4) && ((frame_size+4)>packet->capacity()))) {

              if (0==(flags&OGG_WROTE_HEADER)) {
                packet->append(&frame_size,4);
                flags|=OGG_WROTE_HEADER;
                }

              FXint n = FXMIN(frame_size,packet->space());
              if (input->read(packet->ptr(),n)!=n)
                return ReadError;
              packet->wroteBytes(n);
              frame_size-=n;
              }
            break;
          }
        default:
          {
            FXint n = FXMIN(frame_size,packet->space());
            if (input->read(packet->ptr(),n)!=n)
              return ReadError;
            packet->wroteBytes(n);
            frame_size-=n;
            break;
          }
        }

      // Still bytes left so pass packet to decoder
      if (frame_size) {
        engine->decoder->post(packet);
        return ReadOk;
        }
      }

    FXASSERT(frame_size==0);

    if (!get_next_frame(frame_size)) {
      return ReadError;
      }

    flags&=~OGG_WROTE_HEADER;

    if (frame_size==0) {
      packet->flags|=FLAG_EOS;
      engine->decoder->post(packet);
      return ReadDone;
      }
    }
  return ReadOk;
  }


enum {
  AAC_FLAG_CONFIG = 0x2,
  AAC_FLAG_FRAME  = 0x4
  };


ReadStatus MatroskaReader::parse(Packet * packet) {
  Element element;

  // Get first element
  if (!parse_element(element))
    return ReadError;

  // Make sure it is a matroska file
  if (element.type!=EBML || !parse_ebml(element)) {
    return ReadError;
    }

  // Find First Segment
  while(parse_element(element)) {
    if (element.type==SEGMENT) {
      if (!parse_segment(element)) {
        return ReadError;
        }
      break;
      }
    input->position(element.size,FXIO::Current);
    }

  if (tracks.no()) {
    track=nullptr;

    for (FXint i=0;i<tracks.no();i++) {
      if (tracks[i]->codec) {
        if (track==nullptr) track=tracks[i];
        }
      }

    if (track) {
      fxmessage("Codec: %s\n",Codec::name(track->codec));
      track->af.debug();
      input->position(first_cluster,FXIO::Begin);
      af=track->af;
      ConfigureEvent * cfg = new ConfigureEvent(track->af,track->codec);
      cfg->dc = track->dc;
      engine->decoder->post(cfg);
      stream_length = (duration * timecode_scale * track->af.rate )  / 1000000000;
      flags|=FLAG_PARSED;
/*
      if (data.size()) {
        if (track->codec==Codec::AAC) {
          packet->flags|=AAC_FLAG_CONFIG|AAC_FLAG_FRAME;
          }
        packet->af=af;
        packet->append(data.data(),data.size());
        engine->decoder->post(packet);
        data.clear();
        }
*/
      return ReadOk;
      }
    }
  GM_DEBUG_PRINT("[matroska] no supported tracks found\n");
  return ReadError;
  }




FXbool MatroskaReader::parse_block_group(Element & parent) {
  Element element;
  while(parse_element(parent,element)) {
    switch(element.type) {
      case REFERENCE_BLOCK:
        {
          input->position(element.size,FXIO::Current);
          break;
        }
      case BLOCK:
        {
          if (!parse_simpleblock(element))
            return false;

          if (block.nframes)
            return true;

        } break;
      default               :
        element.debug("Cluster.BlockGroup");
        input->position(element.size,FXIO::Current);
        break;
      }
    }
  return true;
  }



FXbool MatroskaReader::parse_simpleblock(Element & element) {
  FXshort timecode;
  FXuchar flags;
  FXlong  tracknumber;

  if (!parse_element_uint64(element,tracknumber)) {
    return false;
    }

  if (track->number != tracknumber) {
    block.nframes=0;
    input->position(element.size,FXIO::Current);
    return true;
    }

  if (!input->read_int16_be(timecode)){
    return false;
    }

  block.position += timecode;

  if (input->read(&flags,1)!=1){
    return false;
    }

  switch((flags>>1)&0x3) {

    case 0: // No lacing
      {
        block.nframes   = 1;
        block.frames[0] = element.size-3;
        break;
      }

    case 1:
      {
        FXuint total=0;

        // total frames - 1
        if (input->read(&block.nframes,1)!=1)
          return false;

        // Adjust frame count
        block.nframes+=1;

        // Read frame sizes
        for (FXint i=block.nframes-1;i>0;i--) {
          if (!parse_xiph_lace(element,block.frames[i]))
            return false;
          total+=block.frames[i];
          }
        block.frames[0] = element.size - total - 4;
        break;
      }

    case 2: // fixed
      {
        // total frames - 1
        if (input->read(&block.nframes,1)!=1)
          return false;

        // Adjust frame count
        block.nframes+=1;

        // size per frame
        const FXuint size = (element.size-4) / block.nframes;
        for (FXint i=block.nframes-1;i>=0;i--) {
          block.frames[i] = size;
          }
        block.frames[0] += ((element.size-4) % size);
        break;
      }

    case 3: // ebml
      {
        // total frames - 1
        if (input->read(&block.nframes,1)!=1)
          return false;

        // Adjust frame count
        block.nframes+=1;

        FXlong size,total=0;

        // first frame size
        if (!parse_element_uint64(element,size)) {
          return false;
          }

        // read remaining
        block.frames[block.nframes-1] = total = size;
        for (FXint i=block.nframes-2;i>0;i--) {
          if (!parse_element_int64(element,size))
            return false;
          block.frames[i]=block.frames[i+1] + size;
          total+=block.frames[i];
          }
        block.frames[0] = (element.size-4) - total;
        break;
      }
    default: return false; break;
    }
/*
  fxmessage("simpleblock\n");
  for(FXint i=block.nframes-1;i>=0;i--) {
    fxmessage("frame[%d]=%u\n",block.nframes-i-1,block.frames[i]);
    }
*/
  return true;
  }

FXbool MatroskaReader::get_next_frame(FXuint & framesize) {

  framesize = 0;

  // First check for any frames left in current block
  if (block.nframes) {
    framesize = block.next();
    return true;
    }

  // Get next block from Block Group [if any]
  if (group.size) {

    if (!parse_block_group(group))
      return false;

    if (block.nframes) {
      framesize = block.next();
      return true;
      }
    }

  // Get next block from cluster
  do {

    Element element;

    // Get next block or simple block
    while(parse_element(cluster,element)) {

      switch(element.type) {
        case CLUSTER_PREVSIZE:
        case CLUSTER_POSITION:
          {
            input->position(element.size,FXIO::Current);
            break;
          }
        case TIMECODE:
          {
            FXulong timecode=0;
            if (!parse_unsigned_int(timecode,element.size)) return false;
            fxmessage("timecode %ld = %ld seconds @ %ld\n",timecode*timecode_scale,(timecode*timecode_scale) / 1000000000,input->position());
            block.position = (timecode*timecode_scale*track->af.rate) / 1000000000;
            break;
          }
        case SIMPLEBLOCK:
          {

            if (!parse_simpleblock(element))
              return false;

            if (block.nframes) {
              framesize = block.next();
              return true;
              }

          } break;

        case BLOCKGROUP:
          {
            group=element; // store for next iteration

            if (!parse_block_group(group))
              return false;

            if (block.nframes) {
              framesize = block.next();
              return true;
              }

          } break;
        default:
          element.debug("Cluster");
          input->position(element.size,FXIO::Current);
          break;
        }
      }

    // Get the next cluster
    while(parse_element(cluster))  {

      if (cluster.type==CLUSTER) {
        break;
        }
      else if (cluster.type==SEGMENT) {
        return true;
        }
      input->position(cluster.size,FXIO::Current);
      }

    if (cluster.size<=0) {
      return true;
      }

    }
  while(1);
  return true;
  }



FXbool MatroskaReader::parse_track_audio(Element & container) {
  FXASSERT(track);
  Element element;
  while(parse_element(container,element)) {
    switch(element.type) {

      case AUDIO_CHANNELS   :
        {
          FXuchar channels;
          if (!parse_uint8(channels,element.size))
            return false;
          track->af.setChannels(channels);
        } break;

      case AUDIO_BITDEPTH   :
        {
          FXuchar bitdepth;
          if (!parse_uint8(bitdepth,element.size))
            return false;
          track->af.setBits(bitdepth);
        } break;

      case AUDIO_SAMPLE_RATE:
        {
          if (!parse_float_as_uint32(track->af.rate,element.size))
            return false;
        } break;

      default:
        {
          element.debug("Track.Audio");
          input->position(element.size,FXIO::Current);
          break;
        }
      }
    }
  return true;
  }


FXbool MatroskaReader::parse_xiph_lace(Element & container,FXuint & value) {
  FXuchar byte;
  value=0;
  do {
    if (input->read(&byte,1)!=1)
      return false;
    value+=byte;
    container.size-=1;
    if (byte<255)
      return true;
    }
  while(1);
  }


FXbool MatroskaReader::parse_track_codec(Element & element) {

  switch(track->codec) {

    case Codec::Opus:
      {
        if (element.size<19)
          return false;

        OpusConfig * oc = new OpusConfig();

        oc->info_bytes = element.size;
        allocElms(oc->info,oc->info_bytes);
        if (input->read(oc->info,oc->info_bytes)!=oc->info_bytes) {
          delete oc;
          return false;
          }

        track->dc = oc;
        break;
      }

    case Codec::Vorbis:
      {
        FXuchar npackets=0;
        FXuint frames[3];

        if (input->read(&npackets,1)!=1)
          return false;

        if ((npackets+1)!=3)
          return false;

        if (!parse_xiph_lace(element,frames[0]))
          return false;

        if (!parse_xiph_lace(element,frames[1]))
          return false;

        frames[2]=element.size - frames[0] - frames[1] - 1;


        VorbisConfig * vc = new VorbisConfig();

        vc->info_bytes = frames[0];
        allocElms(vc->info,vc->info_bytes);
        if (input->read(vc->info,vc->info_bytes)!=vc->info_bytes) {
          delete vc;
          return false;
          }

        input->position(frames[1],FXIO::Current);

        vc->setup_bytes = frames[2];
        allocElms(vc->setup,vc->setup_bytes);
        if (input->read(vc->setup,vc->setup_bytes)!=vc->setup_bytes){
          delete vc;
          return false;
          }

        track->dc = vc;
        break;
      }
    case Codec::AAC:
      {
        data.resize(element.size);
        if (input->read(data.ptr(),element.size)!=element.size)
          return false;
        data.wroteBytes(element.size);
        break;
      }
    case Codec::Invalid:
      {
        input->position(element.size,FXIO::Current);
        break;
      }
    default:
      {
        element.debug("Track.Codec");
        input->position(element.size,FXIO::Current);
        break;
      }
    }
  return true;
  }

FXbool MatroskaReader::parse_track_entry(Element & container) {
  Element element;

  track = new Track();

  while(parse_element(container,element)) {

    switch(element.type) {

      case TRACK_NUMBER:
        {

          if (!parse_uint64(track->number,element.size)){
            delete track;
            track=nullptr;
            return false;
            }

          // Ignore invalid tracks
          if (track->number==0) {
            delete track;
            track=nullptr;
            return true;
            }

          break;
        }

      case TRACK_TYPE :
        {
          FXuchar track_type=0;

          if (input->read(&track_type,1)!=1) {
            delete track;
            track=nullptr;
            return false;
            }

          // Skip rest of container if this is a non audio track
          if (track_type!=2) {
            input->position(container.size,FXIO::Current);
            delete track;
            track=nullptr;
            return true;
            }
          break;
        }

      case CODEC_ID :
        {
          FXString codec;
          codec.length(element.size);
          input->read(codec.text(),element.size);

          fxmessage("found codec: '%s'\n",codec.text());
#ifdef HAVE_OGG
          if (comparecase(codec,"A_VORBIS")==0) {
            track->codec      = Codec::Vorbis;
            track->af.format |= (Format::Float|Format::Little);
            track->af.setBits(32);
            }
          else if (comparecase(codec,"A_OPUS")==0) {
            track->codec      = Codec::Opus;
            track->af.format |= (Format::Float|Format::Little);
            track->af.setBits(32);
            }
#endif
          if (!is_webm) {

            if (comparecase(codec,"A_PCM/INT/LIT")==0) {
              track->codec   = Codec::PCM;
              track->af.format |= (Format::Signed|Format::Little);
              }
            else if (comparecase(codec,"A_PCM/INT/BIG")==0) {
              track->codec   = Codec::PCM;
              track->af.format |= (Format::Signed|Format::Big);
              }
            else if (comparecase(codec,"A_PCM/FLOAT/IEEE")==0) {
              track->codec   = Codec::PCM;
              track->af.format |= (Format::Float|Format::Little);
              }
            else if (comparecase(codec,"A_DTS")==0) {
              track->codec      = Codec::DCA;
              track->af.format |= (Format::Float|Format::Little);
              track->af.setBits(32);
              }
            else if (comparecase(codec,"A_AC3")==0) {
              track->codec      = Codec::A52;
              track->af.format |= (Format::Float|Format::Little);
              track->af.setBits(32);
              }
            else if (comparecase(codec,"A_MPEG/L3")==0) {
              track->codec      = Codec::MPEG;
              track->af.format |= (Format::Signed|Format::Little);
              track->af.setBits(16);
              }
            else if (comparecase(codec,"A_FLAC")==0) {
              track->codec      = Codec::FLAC;
              track->af.format |= (Format::Signed|Format::Little);
              //track->af.setBits(16);
              }
            else if (comparecase(codec,"A_AAC")==0) {
              track->codec      = Codec::AAC;
              track->af.format |= (Format::Signed|Format::Little);
              track->af.setBits(16);
              }

            }
          break;
        }
      case CODEC_PRIVATE:
        {
          if (!parse_track_codec(element)) {
            delete track;
            track=nullptr;
            return false;
            }
          break;
        }
      case AUDIO:
        {
          if (!parse_track_audio(element)){
            delete track;
            track=nullptr;
            return false;
            }
        } break;
      default:
        {
          element.debug("Track");
          input->position(element.size,FXIO::Current);
          break;
        }
      }
    }

  // Fixup channel maps
  if (track->codec == Codec::DCA) {
    if (track->af.channels==6)
      track->af.channelmap = AP_CMAP6(Channel::FrontCenter,Channel::FrontLeft,Channel::FrontRight,Channel::BackLeft,Channel::BackRight,Channel::LFE);
    }
  else if (track->codec == Codec::A52) {
    if (track->af.channels==6)
      track->af.channelmap = AP_CMAP6(Channel::LFE,Channel::FrontLeft,Channel::FrontCenter,Channel::FrontRight,Channel::BackLeft,Channel::BackRight);
    }
  else if (track->codec == Codec::Opus){
    track->af.rate = 48000;
    }
  tracks.append(track);
  return true;
  }


FXbool MatroskaReader::parse_track(Element & container) {
  Element element;
  while(parse_element(container,element)) {
    switch(element.type) {
      case TRACK_ENTRY:
        {
          if (!parse_track_entry(element))
            return false;
          break;
        }
      default:
        {
          element.debug("TrackList");
          input->position(element.size,FXIO::Current);
          break;
        }
      }
    }
  return true;
  }


FXbool MatroskaReader::parse_cue_track(Element & container,FXulong & cue_track,FXulong & cluster_position) {
  cue_track=0;
  cluster_position=0;
  Element element;
  while(parse_element(container,element)) {
    switch(element.type) {
      case CUE_TRACK:
        {
          if (!parse_uint64(cue_track,element.size))
            return false;
          break;
        }
      case CUE_CLUSTER_POSITION:
        {
          if (!parse_uint64(cluster_position,element.size))
            return false;
        } break;
      case CUE_RELATIVE_POSITION:
        {
          input->position(element.size,FXIO::Current);
          break;
        }

      //  {
      //    if (!parse_uint64(relative_position,element.size))
      //      return false;
      //  } break;
      default:
        {
          element.debug("Cues.CuePoint.Track");
          input->position(element.size,FXIO::Current);
          break;
        }
      }
    }
  return true;
  }


FXbool MatroskaReader::parse_cue_point(Element & container) {
  FXulong cuetime;
  FXulong  cluster_position;
  FXulong cuetrack;

  Element element;
  while(parse_element(container,element)) {
    switch(element.type) {
      case CUE_TIME:
        {
          if (!parse_uint64(cuetime,element.size))
            return false;
          fxmessage("cuetime %ld\n",cuetime);
          break;
        }
      case CUE_TRACK_POSITIONS:
        {
          if (!parse_cue_track(element,cuetrack,cluster_position))
            return false;
        } break;
      default:
        {
          element.debug("Cues.CuePoint");
          input->position(element.size,FXIO::Current);
          break;
        }
      }
    }
  for (FXint i=0;i<tracks.no();i++) {
    if (tracks[i]->number==cuetrack) {
      tracks[i]->add_cue_entry(cuetime,cluster_position);
      break;
      }
    }
  return true;
  }


FXbool MatroskaReader::parse_cues(Element & container) {
  Element element;
  while(parse_element(container,element)) {
    switch(element.type) {
      case CUE_POINT:
        {
          if (!parse_cue_point(element))
            return false;
          break;
        }
      default:
        {
          element.debug("Cues");
          input->position(element.size,FXIO::Current);
          break;
        }
      }
    }
  return true;
  }

FXbool MatroskaReader::parse_segment_info(Element & container) {
  Element element;
  while(parse_element(container,element)) {
    switch(element.type) {

      case SEGMENT_INFO_DURATION:
        {
          if (element.size==4) {
            FXfloat value;
            input->read_float_be(value);
            duration=lrintf(value);
            }
          else if (element.size==8) {
            FXdouble value;
            input->read_double_be(value);
            duration=lrint(value);
            }
          break;
        }

      case SEGMENT_INFO_TIMECODE_SCALE:
        {

          if (!parse_unsigned_int(timecode_scale,element.size))
            return false;

          fxmessage("timecode scale %ld\n",timecode_scale);
          break;
        }
      default:
        {
          element.debug("Segment.Info");
          input->position(element.size,FXIO::Current);
          break;
        }
      }
    }
  return true;
  }


FXbool MatroskaReader::parse_segment(Element & container) {
  Element element;

  while(parse_element(container,element,true)) {
    switch(element.type) {

      case SEGMENT_INFO:
        {
          if (!parse_segment_info(element))
            return false;
          break;
        }

      case SEEK_HEAD:
        {
          if (!parse_seekhead(element))
            return false;
          break;
        }

      case TRACK    :
        {

          if (!parse_track(element))
            return false;

          if (tracks.no()==0)
            return true;

        } break;

      case CLUSTER  :
        {

          if (first_cluster==0)
            first_cluster=element.offset;

          if (input->serial())
            return true;

          input->position(element.size,FXIO::Current);

        } break;

      case CUES:
        {
          if (!parse_cues(element))
            return false;

          break;
        }
      default       :
        element.debug("Segment");
        input->position(element.size,FXIO::Current);
        break;
      }
    }
  return true;
  }


FXbool MatroskaReader::parse_seekhead(Element & container) {
  Element element;
  while(parse_element(container,element)) {
    switch(element.type) {
      case SEEK   :
        parse_seek(element);
        break;
      default     :
        element.debug("Seek");
        input->position(element.size,FXIO::Current);
        break;
      }
    }
  return true;
  }

FXbool MatroskaReader::parse_seek(Element & container) {
  Element element;
  while(parse_element(container,element)) {
    switch(element.type) {
      case SEEK_ID:
        input->position(element.size,FXIO::Current);
        break;
      case SEEK_POSITION:
        input->position(element.size,FXIO::Current);
        break;
      default:
        element.debug("Seek.Entry");
        input->position(element.size,FXIO::Current);
        break;
      }
    }
  return true;
  }








FXbool MatroskaReader::parse_element(Element & element) {
  Element container(12);

  element.offset = input->position();
  element.size   = 0;

  if (!parse_element_id(container,element))
    return false;

  if (!parse_element_uint64(container,element.size))
    return false;

  return true;
  }

FXbool MatroskaReader::parse_element(Element & container,Element & element,FXbool allow_unknown_size) {
  if (container.size > 2) {

    element.offset = input->position();
    element.size   = 0;

    if (!parse_element_id(container,element))
      return false;

    if (!parse_element_uint64(container,element.size))
      return false;

    if (element.size>0)
      container.size-=element.size;

    return true;
    }
  else if (container.size == -1 && allow_unknown_size) {
    return parse_element(element);
    }
  return false;
  }


FXbool MatroskaReader::parse_ebml(Element & container) {
  Element  element;
  FXString doctype;

  while(parse_element(container,element)) {

    switch(element.type) {
      case EBML_DOC_TYPE:
        doctype.length(element.size);
        input->read(doctype.text(),element.size);
        break;

      case EBML_VERSION:
      case EBML_READ_VERSION:
      case EBML_MAX_ID_LENGTH:
      case EBML_MAX_SIZE_LENGTH:
      case EBML_DOC_TYPE_VERSION:
      case EBML_DOC_TYPE_READ_VERSION:
        input->position(element.size,FXIO::Current);
        break;
      default:
        element.debug("ebml");
        input->position(element.size,FXIO::Current);
        break;
      }
    }

  if (doctype=="webm") {
    is_webm = true;
    return true;
    }

  if (doctype=="matroska"){
    is_webm = false;
    return true;
    }

  GM_DEBUG_PRINT("[matroska] unknown doctype \"%s\"\n",doctype.text());
  return false;
  }






FXbool MatroskaReader::parse_element_id(Element & container,Element & element) {
  FXuchar buffer[3]={0};
  FXuchar size;


  // Read first byte
  if (input->read(buffer,1)!=1 || (buffer[0]<=0x7)) {
    return false;
    }

  element.type=buffer[0];

  // Get size of id
  size=clz32(element.type)-24;

  // Read remaining bytes
  if (size) {

    if (input->read(buffer,size)!=size) {
      return false;
      }

    for (FXint i=0;i<size;i++) {
      element.type=(element.type<<8)|static_cast<FXuint>(buffer[i]);
      }
    }
  container.size-=(size+1);
  return true;
  }

FXbool MatroskaReader::parse_element_uint64(Element & container,FXlong & value) {
  FXuchar n = parse_ebml_uint64(value);
  if (n==0) return false;
  container.size-=n;
  return true;
  }

FXbool MatroskaReader::parse_element_int64(Element & container,FXlong & value) {
  FXuchar n = parse_ebml_uint64(value);
  if (n==0) return false;
  value = value - ((FXULONG(1)<<((7*n)-1))-1);
  container.size-=n;
  return true;
  }




FXuchar MatroskaReader::parse_ebml_uint64(FXlong & value) {
  FXuchar buffer[7];
  FXuchar n;

  // Read first byte
  if (input->read(buffer,1)!=1 || (buffer[0]==0)) {
    return 0;
    }

  // Store first byte value
  value=buffer[0];

  // Get size of id
  n=clz32(value)-24;

  // Apply mask
  value=value&((1<<(7-n))-1);

  // Read remaining bytes
  if (n) {

    if (input->read(buffer,n)!=n) {
      return 0;
      }

    for (FXint i=0;i<n;i++) {
      value=(value<<8)|static_cast<FXlong>(buffer[i]);
      }
    }

  // Check for unknown sizes
  if (value == ((FXULONG(1)<<((7-n)+(n*8)))-1)) {
    fxwarning("matroska: unknown element size not supported");
    value=-1;
    }
  return (n+1);
  }





FXbool MatroskaReader::parse_uint8(FXuchar & value,const FXlong size) {
  if (size==1 && input->read(&value,1)==1)
    return true;
  else
    return false;
  }

FXbool MatroskaReader::parse_uint64(FXulong & value,const FXlong size) {
  FXuchar byte;
  value = 0;
  for (FXlong i=0;i<size;i++) {
    if (input->read(&byte,1)!=1) return false;
    value = value<<8 | static_cast<FXulong>(byte);
    }
  return true;
  }



FXbool MatroskaReader::parse_unsigned_int(FXulong & value,const FXlong size) {
  FXuchar byte;
  value = 0;
  for (FXlong i=0;i<size;i++) {
    if (input->read(&byte,1)!=1) return false;
    value = value<<8 | static_cast<FXulong>(byte);
    }
  return true;
  }

FXbool MatroskaReader::parse_float_as_uint32(FXuint & value,const FXlong size) {
  if (size==4) {
    FXfloat val;
    if (!input->read_float_be(val)) return false;
    value = lrintf(val);
    }
  else if (size==8) {
    FXdouble val;
    if (!input->read_double_be(val)) return false;
    value = rintf(val);
    }
  else {
    return false;
    }
  return true;
  }

}


ReaderPlugin * ap_matroska_reader(AudioEngine * engine) {
  return new matroska::MatroskaReader(engine);
  }

}



