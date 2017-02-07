/*******************************************************************************
*                         Goggles Audio Player Library                         *
********************************************************************************
*           Copyright (C) 2010-2017 by Sander Jansen. All Rights Reserved      *
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
#include "ap_vorbis.h"
#include "ap_opus.h"
#include "ap_packet.h"
#include "ap_reader_plugin.h"
#include "ap_input_plugin.h"

#include <ogg/ogg.h>

#if defined(HAVE_VORBIS)
#include <vorbis/codec.h>
#elif defined(HAVE_TREMOR)
#include <tremor/ivorbiscodec.h>
#endif

#ifdef HAVE_OPUS
#include <opus/opus.h>
#endif

namespace ap {

class AudioEngine;

#if defined(HAVE_VORBIS) || defined(HAVE_TREMOR)

class VorbisCodec {
protected:
  FXulong blockflag    = 0; // up to 64 flags for each mode
  FXuchar modecount    = 0; // up to 64 entries
  FXuchar modenbits    = 0; // number of bits needed to store modecount.
  FXint   blocksize[2] = {0,0};
public:
  /// Parse Info Header for blocksize
  FXbool parseInfo(const FXuchar * packet,FXuint size);

  // Parse Setup Header
  FXbool parseSetup(const FXuchar * packet,FXuint size);

  // Given the packet data, return blocksize
  FXint getBlockSize(const FXuchar * packet) const;

  // Clear Codec
  void clear();
  };


void VorbisCodec::clear() {
  blockflag    = 0;
  modecount    = 0;
  modenbits    = 0;
  blocksize[0] = 0;
  blocksize[1] = 0;
  }

FXbool VorbisCodec::parseInfo(const FXuchar * packet,FXuint size) {
  if (size>=28) {
    blocksize[0] = 1<<(packet[28]&0xF);
    blocksize[1] = 1<<(packet[28]>>4);
    return true;
    }
  return false;
  }


FXbool VorbisCodec::parseSetup(const FXuchar * packet,FXuint size) {

  // Minimum packet size
  const FXuint MIN_PACKET_SIZE = 7 + // header
                                 1 + // number of codebooks
                                 3 + // codebook sync bytes
                                 5 + // codebook dims + entry counts
                                 2 + // 1 vorbis_time_count value  (at least 1)
                                 2 + // 1 vorbis_floor_types value (at least 1)
                                 2 + // 1 vorbis_residue_types value (at least 1)
                                 2 + // 1 vorbis_mapping_type value (at least 1)
                                 3;  // 4 x 6 bits count fields: vorbis_time_count,vorbis_floor_count,vorbis_residue_count,vorbis_mapping_count
  FXulong bits;
  FXuchar nbits=0;
  FXuchar found=0;
  FXuint  p = size-1;

  // Clear State
  blockflag = 0;
  modecount = 0;
  modenbits = 0;

  // Find the framing bit
  while(nbits==0 && p>MIN_PACKET_SIZE) {
    bits=packet[p--];
    for (nbits=8; nbits && (bits&(1<<(nbits-1)))==0; nbits--);
    }
  if (nbits==0) return false;

  nbits--;

  // read mode entry:
  // vorbis_mode_blockflag:1
  // vorbis_mode_windowtype:16 => should be 0
  // vorbis_mode_transformtype:16 => should be 0
  // vorbis_mode_mapping:8
  //
  // also read potential vorbis_mode_count:6
  for (;nbits<47;nbits+=8){
    bits<<=8;
    bits|=(static_cast<FXulong>(packet[p--]));
    }

  // scan backwards to start of modes section
  while(p>MIN_PACKET_SIZE && found<=64) {

    const FXulong entry = (bits>>(nbits-41));

    // expect 0s for window type and transform type
    if (entry&0x1FFFFFFFELL) break;

    // for now assume this is a valid entry
    found++;

    // Store block size flag
    blockflag <<= 1;
    blockflag  |= (entry&0x1);

    // if this is the first entry, this is preceded by vorbis_mode_count
    const FXuchar size = 1 + ((bits>>(nbits-47))&0x3f);
    if (size==found)
      modecount = found;

    // fill buffer with next entry
    for (nbits-=41;nbits<47;nbits+=8) {
      bits<<=8;
      bits |= static_cast<FXulong>(packet[p--]);
      }
    }

  // calculate modebits
  if (modecount) {
    for (FXint v = modecount-1; v>0; modenbits++) {
      v>>=1;
      }
    return true;
    }
  return false;
  }


FXint VorbisCodec::getBlockSize(const FXuchar * packet) const {
  const FXuchar mode = (packet[0]>>1)&modenbits;
  return blocksize[(blockflag>>mode)&0x1];
  }



#endif






struct OggReaderState {
  FXbool has_stream     = false;
  FXbool has_eos        = false;
  FXbool has_page       = false;
  FXbool has_packet     = false;
  FXbool header_written = false;
  FXuint bytes_written  = 0;
  OggReaderState() {}
  void reset() {has_stream=false;has_eos=false;has_page=false; has_packet=false; header_written=false;bytes_written=0; }
  };

class OggReader : public ReaderPlugin {
protected:
  enum {
    FLAG_OGG_VORBIS            = 0x2,
    FLAG_OGG_FLAC              = 0x4,
    FLAG_OGG_OPUS              = 0x8,
    FLAG_VORBIS_INFO           = 0x10,
    FLAG_VORBIS_COMMENT        = 0x20,
    FLAG_VORBIS_BLOCK          = 0x40,
    FLAG_VORBIS_MASK           = FLAG_VORBIS_INFO|FLAG_VORBIS_COMMENT|FLAG_VORBIS_BLOCK
    };
protected:
  OggReaderState    state;
  ogg_stream_state  stream = {};
  ogg_sync_state    sync = {};
  ogg_page          page = {};
  ogg_packet        op = {};
protected:
  MemoryBuffer      cached_packets;

protected:
#if defined(HAVE_VORBIS) || defined(HAVE_TREMOR)
  VorbisCodec     vorbis;
#endif
protected:
  Packet *        packet = nullptr;
  ReadStatus      status = ReadError;
  FXuchar         codec = Codec::Invalid;
  FXlong          stream_start = 0;
  FXlong          input_position = -1;
  FXushort        stream_offset_start = 0;
  FXushort        stream_offset_end = 0;
  FXlong          stream_position = -1;
protected:
  FXbool match_page();
  FXbool fetch_next_page();
  FXbool fetch_next_packet(FXbool nocache=false);
  void submit_ogg_packet();
  void cache_ogg_packet();
  FXlong find_lastpage_position();

  ReadStatus parse();
#if defined(HAVE_VORBIS) || defined(HAVE_TREMOR)
  ReadStatus parse_vorbis_stream();
  void check_vorbis_length();
  FXbool parse_vorbis_setup();
#endif
#ifdef HAVE_FLAC
  ReadStatus parse_flac_stream();
#endif
#ifdef HAVE_OPUS
  ReadStatus parse_opus_stream();
  void check_opus_length();
#endif
public:
  OggReader(InputContext *);
  FXuchar format() const override { return Format::OGG; };
  FXbool init(InputPlugin*) override;
  FXlong seek_offset(FXdouble) const;
  FXbool seek(FXlong offset) override;
  FXbool can_seek() const override;
  ReadStatus process(Packet*) override;
  virtual ~OggReader();
  };















OggReader::OggReader(InputContext * ctx) : ReaderPlugin(ctx), cached_packets(0) {
  ogg_sync_init(&sync);
  }


OggReader::~OggReader() {
  ogg_sync_clear(&sync);

  if (state.has_stream) {
    ogg_stream_clear(&stream);
    state.has_stream=false;
    }
  }

FXbool OggReader::can_seek() const {
  return stream_length>0;
  }

FXlong OggReader::seek_offset(FXdouble pos) const{
  if (stream_length>0)
    return stream_start + (stream_length*pos);
  else
    return -1;
  }

FXbool OggReader::seek(FXlong target){
 FXASSERT(stream_length>0);

    if (packet) {
      packet->unref();
      packet=nullptr;
      }

    state.has_eos=false;
    state.has_packet=false;
    state.has_page=false;
    state.header_written=false;
    state.bytes_written=0;
    ogg_sync_reset(&sync);
    ogg_stream_reset(&stream);


    /*
      When seeking within an Ogg Opus stream, the decoder should start decoding (and discarding the output) at least 3840 samples (80 ms)
      prior to the seek point in order to ensure that the output audio is correct at the seek point.
    */
    if (codec==Codec::Opus)
      target = FXMAX(0,target-3840);

    FXlong offset  = input->size() * (target/stream_length);
    FXlong lastpos = -1;

    input_position = input->position(offset,FXIO::Begin);

    GM_DEBUG_PRINT("[ogg] target seek %ld / %ld => %ld\n",target,stream_length,offset);

    while(fetch_next_page()) {
      if (ogg_page_granulepos(&page)>target) {
        GM_DEBUG_PRINT("[ogg] found %ld %ld %ld\n",ogg_page_granulepos(&page),lastpos,offset);
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
        stream_position=ogg_page_granulepos(&page);
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
  flags&=~(FLAG_PARSED|FLAG_OGG_VORBIS|FLAG_OGG_FLAC|FLAG_OGG_OPUS|FLAG_VORBIS_MASK);
  status=ReadOk;

  input_position=-1;
  stream_start=0;
  stream_offset_start=0;
  stream_offset_end=0;

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

    GM_DEBUG_PRINT("[ogg] page serial not matching\n");

    if (input->serial() && state.has_eos) {

      GM_DEBUG_PRINT("[ogg] serial input and eos was set\n");

      ogg_stream_clear(&stream);
      state.has_stream=false;

      if (ogg_page_bos(&page)) {
        FXint serial = ogg_page_serialno(&page);
        GM_DEBUG_PRINT("[ogg] begin of new stream %d\n",serial);
        ogg_stream_init(&stream,serial);
        state.has_stream=true;
        state.has_eos=false;
        return true;
        }
      else {
        GM_DEBUG_PRINT("[ogg] expected bos page not found\n");
        }
      }
    else {
      GM_DEBUG_PRINT("non-matching page %d (expected %ld)\n",ogg_page_serialno(&page),stream.serialno);
      }
    }
  else {
    if (ogg_page_bos(&page)) {
      GM_DEBUG_PRINT("[ogg] beginning of stream\n");
      FXint serial = ogg_page_serialno(&page);
      ogg_stream_init(&stream,serial);
      state.has_stream=true;
      return true;
      }
    }
  GM_DEBUG_PRINT("[ogg] failed to match page\n");
  return false;
  }


FXlong OggReader::find_lastpage_position() {
  FXlong size = input->size();
  FXlong pos = -1;

  /// TODO need a smart way of finding the last page in stream.
  if (size>=0xFFFF) {
    input->position((size-0xFFFF),FXIO::Begin);
    ogg_sync_reset(&sync);
    }

  /// Go to last page of stream to find out last pcm position
  while(fetch_next_page()) {
    pos=ogg_page_granulepos(&page);
    if (ogg_page_eos(&page)) {
      return pos;
      }
    }

  /// eos not found, use last found pos
  GM_DEBUG_PRINT("[ogg] no page found with eos. Truncated file?\n");
  return pos;
  }


#ifdef HAVE_OPUS
void OggReader::check_opus_length() {
  stream_length=0;
  stream_start=0;
  if (!input->serial()) {
    FXlong cpos = input_position;
    FXlong nsamples = 0;

    /// First determine the pcm offset at the start of a stream
    while(fetch_next_packet()) {
      nsamples += opus_packet_get_nb_samples((unsigned char*)op.packet,op.bytes,48000);
      if (op.granulepos!=-1) {
        stream_start=op.granulepos-nsamples;
        GM_DEBUG_PRINT("[ogg] stream start=%ld %ld %ld\n",op.granulepos,nsamples,stream_start);
        break;
        }
      }

    FXASSERT(stream_start>=0);

    /// Find end of stream
    FXlong pos = find_lastpage_position();
    if (pos>0) {
      stream_length = pos - stream_start - stream_offset_start;
      GM_DEBUG_PRINT("[ogg] stream length = %ld\n",stream_length);
      }

    input->position(cpos,FXIO::Begin);
    ogg_sync_reset(&sync);
    ogg_stream_reset(&stream);
    }

  // Mark beginning of stream
  if (stream_start>0)
    stream_position = stream_start;
  else
    stream_position = 0;
  }
#endif

#if defined(HAVE_VORBIS) || defined(HAVE_TREMOR)


void OggReader::check_vorbis_length() {
  stream_length=-1;
  stream_start=0;

  if (!input->serial()) {
    FXlong cpos = input_position;

    /// First Determine the pcm offset at the start of the stream
    FXint cb,lb=-1,tb=0;
    while(fetch_next_packet()) {
      cb=vorbis.getBlockSize(op.packet);
      if (lb!=-1) tb+=(lb+cb)>>2;
      lb=cb;
      if (op.granulepos!=-1) {
        stream_start=op.granulepos-tb;
        GM_DEBUG_PRINT("[ogg] stream offset=%ld %d %ld\n",op.granulepos,tb,stream_start);
        break;
        }
      }

    // handle negative offset
    if (stream_start < 0) {
      stream_offset_start = -stream_start;
      }

    /// Find end of stream
    FXlong pos = find_lastpage_position();
    if (pos>0) {
      stream_length = pos - stream_start;
      GM_DEBUG_PRINT("[ogg] stream length = %ld\n",stream_length);
      }
    input->position(cpos,FXIO::Begin);

    ogg_sync_reset(&sync);
    ogg_stream_reset(&stream);
    }
  else {


    /// First Determine the pcm offset at the start of the stream
    FXint cb,lb=-1,tb=0;
    while(fetch_next_packet(true)) {

      // cache this packet
      cache_ogg_packet();

      cb=vorbis.getBlockSize(op.packet);
      if (lb!=-1) tb+=(lb+cb)>>2;
      lb=cb;
      if (op.granulepos!=-1) {
        stream_start=op.granulepos-tb;
        GM_DEBUG_PRINT("[ogg] stream offset=%ld %d %ld\n",op.granulepos,tb,stream_start);
        break;
        }
      }

    // handle negative offset
    if (stream_start < 0) {
      stream_offset_start = -stream_start;
      }
    }

  // Mark beginning of stream
  if (stream_start>0)
    stream_position = stream_start;
  else
    stream_position = 0;
  }

#endif

extern void ap_replaygain_from_vorbis_comment(ReplayGain & gain,const FXchar * comment,FXint len);
extern void ap_meta_from_vorbis_comment(MetaInfo * meta, const FXchar * comment,FXint len);


#if defined(HAVE_OPUS) || defined(HAVE_VORBIS) || defined(HAVE_TREMOR)

extern const FXuint vorbis_channel_map[];

extern void ap_parse_vorbiscomment(const FXuchar * buffer,FXint len,ReplayGain & gain,MetaInfo * meta);

#endif



#ifdef HAVE_OPUS


ReadStatus OggReader::parse_opus_stream() {
  OpusConfig * opus_config = new OpusConfig;

  if (op.bytes<19) {
    GM_DEBUG_PRINT("[ogg] packet size too small for opushead\n");
    return ReadError;
    }

#if FOX_BIGENDIAN == 0
  stream_offset_start = (op.packet[10] | op.packet[11]<<8);
#else
  stream_offset_start = (op.packet[10]<<8 | op.packet[11]);
#endif

  // channel mapping family
  switch(op.packet[18]) {

    // RTP mapping
    case  0:  af.set(AP_FORMAT_FLOAT,48000,op.packet[9]);
              break;

    // vorbis mapping family
    case  1:
              // Support 1-8 channels
              if (op.packet[9]<1 || op.packet[9]>8)
                return ReadError;

              af.set(AP_FORMAT_FLOAT,48000,op.packet[9],vorbis_channel_map[op.packet[9]-1]);

              // Make sure stream map is correct
              if (af.channels!=op.bytes-21)
                return ReadError;
              break;

    // Undefined, most players shouldn't play this
    default:  return ReadError;
    }

  opus_config->setOpusInfo(op.packet,op.bytes);

  // OpusTags
  if (!fetch_next_packet())
    return ReadError;

  if (compare((const FXchar*)op.packet,"OpusTags",8))
    return ReadError;


  codec=Codec::Opus;

  ConfigureEvent * config = new ConfigureEvent(af,codec);
  MetaInfo       * meta   = new MetaInfo();
  ap_parse_vorbiscomment(op.packet+8,op.bytes-8,config->replaygain,meta);

  config->dc                  = opus_config;
  config->stream_offset_start = stream_offset_start;

  // Now we are ready to init the decoder
  context->post_configuration(config);

  // Send Meta Info
  context->post_meta(meta);

  // find out stream length
  check_opus_length();

  // Success
  flags|=FLAG_PARSED;

  return ReadOk;
  }

#endif

#if defined(HAVE_VORBIS) || defined(HAVE_TREMOR)

ReadStatus OggReader::parse_vorbis_stream() {
  ReplayGain       gain;
  MetaInfo       * meta          = nullptr;
  ConfigureEvent * config        = nullptr;

  // Store Vorbis Config
  VorbisConfig   * vorbis_config = new VorbisConfig();

  //FXASSERT(state.has_packet);
  FXASSERT(op.packet[0]==1);

  // Vorbis Info Packet
#ifdef FOX_BIGENDIAN
  FXuint rate = (op.packet[15] << 24 | op.packet[14] << 16 | op.packet[13] << 8 | op.packet[12]);
#else
  FXuint rate = (op.packet[12] << 24 | op.packet[13] << 16 | op.packet[14] << 8 | op.packet[15]);
#endif
  FXuchar channels = op.packet[11];

  if (channels<1 || channels>8) {
    goto x;
    }

  if (!vorbis.parseInfo(op.packet,op.bytes)) {
    goto x;
    }

  vorbis_config->setVorbisInfo(op.packet,op.bytes);

  // Vorbis Comment Packet
  if (!fetch_next_packet()) {
    goto x;
    }

  // Check make sure its a comment packet
  if (compare((const FXchar*)op.packet,"\x03""vorbis",7)) {
    goto x;
    }

  // Parse the vorbis comments for useful information
  meta = new MetaInfo();
  ap_parse_vorbiscomment(op.packet+8,op.bytes-8,gain,meta);

  // Vorbis Setup Packet
  if (!fetch_next_packet()) {
    goto x;
    }

  // Check make sure its the correct packet
  if (compare((const FXchar*)op.packet,"\x05""vorbis",7)) {
    goto x;
    }

  // Parse Setup Packet
  if (!vorbis.parseSetup(op.packet,op.bytes)) {
    goto x;
    }

  vorbis_config->setVorbisSetup(op.packet,op.bytes);

  af.set(AP_FORMAT_FLOAT,rate,channels,vorbis_channel_map[channels-1]);

  codec=Codec::Vorbis;

  config = new ConfigureEvent(af,codec);
  config->dc         = vorbis_config;
  config->replaygain = gain;

  // Check vorbis length
  check_vorbis_length();

  config->stream_offset_start = stream_offset_start;
  config->stream_offset_end = stream_offset_end;

  // Initialize decode
  context->post_configuration(config);

  // Post meta information
  context->post_meta(meta);

  // Success
  flags|=FLAG_PARSED;

  return ReadOk;

x:
  GM_DEBUG_PRINT("[ogg] failed to parse vorbis stream\n");
  vorbis.clear();
  if (meta) meta->unref();
  delete vorbis_config;
  return ReadError;
  }

#endif

#ifdef HAVE_FLAC

extern void flac_parse_vorbiscomment(const FXuchar * buffer,FXint len,ReplayGain & gain,MetaInfo * meta);
extern FXbool flac_parse_streaminfo(const FXuchar * buffer,AudioFormat & config,FXlong & nframes);
extern FXbool flac_audioformat(const FXuchar * info,AudioFormat & af,FXlong & stream_length);



ReadStatus OggReader::parse_flac_stream() {

  // First packet with StreamInfo
  if (op.bytes!=51)
    return ReadError;

  // Packet Type
  if (op.packet[0]!=0x7f)
    return ReadError;

  // Check Ogg Flac mapping version
  if (op.packet[5]!=0x01 || op.packet[6]!=0x00)
    return ReadError;

  // FLAC Signature
  if (op.packet[9]!='f' || op.packet[10]!='L' || op.packet[11]!='a' || op.packet[12]!='C')
    return ReadError;

  // Check for Metadata Block
  if ((op.packet[13]&0x7f)!=0)
    return ReadError;

  // Check metablock size
  if (34!=((FXuint)op.packet[14]<<16 | ((FXuint)op.packet[15]<<8) | ((FXuint)op.packet[16])))
    return ReadError;

  // Get audio format from header
  if (!flac_audioformat(op.packet+27,af,stream_length))
    return ReadError;

  // Get VorbisComments
  if (!fetch_next_packet())
    return ReadError;

  codec=Codec::FLAC;

  ConfigureEvent * config = new ConfigureEvent(af,codec,stream_length);
  MetaInfo * meta         = new MetaInfo;

  flac_parse_vorbiscomment(op.packet,op.bytes,config->replaygain,meta);

  // Initialize decode
  context->post_configuration(config);

  // Post meta information
  context->post_meta(meta);

  flags|=FLAG_PARSED;
  return ReadOk;
  }

#endif

ReadStatus OggReader::parse() {
  if (input_position==-1)
    input_position = input->position();

  if (!state.has_packet && !fetch_next_packet()){
    GM_DEBUG_PRINT("[ogg] parse failure\n");
    return ReadError;
    }

#if defined(HAVE_VORBIS) || defined(HAVE_TREMOR)
  if (compare((const FXchar*)op.packet,"\x01""vorbis",7)==0) {
    return parse_vorbis_stream();
    }
#endif
#ifdef HAVE_OPUS
  if (compare((const FXchar*)op.packet,"OpusHead",8)==0){
    return parse_opus_stream();
    }
#endif
#ifdef HAVE_FLAC
  if (compare((const FXchar*)op.packet,"\x7f""FLAC",5)==0) {
    return parse_flac_stream();
    }
#endif
  GM_DEBUG_PRINT("[ogg] unknown ogg header %hhd -> %s\n",op.packet[0],(const FXchar*)&op.packet[1]);
  return ReadError;
  }



void OggReader::cache_ogg_packet() {
  FXuint nbytes = op.bytes;
  cached_packets.append(&nbytes,4);
  cached_packets.append(op.packet,op.bytes);
  }


void OggReader::submit_ogg_packet() {
  FXuint nbytes;

  state.has_packet=true;

  // Make sure data fits into packet or allow partial if it exceeds capacity
  if ((packet->space()>=(op.bytes+4)) ||
      ((packet->space()>=4) && ((op.bytes+4)>packet->capacity()))) {

    // write packet size
    if (state.header_written==false) {
      if (codec==Codec::Vorbis || codec==Codec::Opus) {
        nbytes = op.bytes;
        packet->append(&nbytes,4);
        state.header_written=true;
        if (stream_position!=-1) {
          packet->stream_position=stream_position;
          stream_position=-1;
          }
        }
      else {
        // no
        state.header_written=true;
        }
      }

    // write packet data
    nbytes = FXMIN((op.bytes-state.bytes_written),packet->space());
    if (nbytes) {
      packet->append(&op.packet[state.bytes_written],nbytes);
      state.bytes_written+=nbytes;
      }

    // Check if we're done with the packet.
    if (state.header_written && state.bytes_written==op.bytes) {
      state.header_written=false;
      state.bytes_written=0;
      state.has_packet=false;
      if (op.e_o_s) {
        packet->flags|=FLAG_EOS;
        state.has_eos=true;
        }
      }

    if (packet->flags&FLAG_EOS) {
      packet->af=af;
      packet->stream_length=stream_length;
      context->post_packet(packet);
      packet=nullptr;
      }
    }
  else {
    packet->af=af;
    packet->stream_length=stream_length;
    context->post_packet(packet);
    packet=nullptr;
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
      if (match_page()) {
        return true;
        }
      }
    else if (result==0) { /// Need more bytes
      FXchar * buffer = ogg_sync_buffer(&sync,BUFFERSIZE);
      if (buffer==nullptr) return false;
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


FXbool OggReader::fetch_next_packet(FXbool nocache/*=false*/) {
  FXint result;
  while(1) {

    if (__unlikely(cached_packets.size() && nocache==false)) {
      op.packetno   = 0;
      op.granulepos = -1;
      op.b_o_s      = 0;
      op.e_o_s      = 0;
      FXuint nbytes;
      cached_packets.read(&nbytes,4);
      op.bytes=nbytes;
      cached_packets.read(op.packet,op.bytes);
      return true;
      }

    if (state.has_page==false) {

      if (!fetch_next_page()){
        return false;
        }

      if (ogg_stream_pagein(&stream,&page))
        return false;

      // For streaming check for eos and determine stream_length)
      if (stream_length==-1 && ogg_page_eos(&page)) {
        stream_length = ogg_page_granulepos(&page);
        }

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
  packet = p;
  packet->stream_position = -1;

  if (__unlikely((flags&FLAG_PARSED)==0)) {

    if (parse()!=ReadOk)
      return ReadError;

    if (__unlikely((flags&FLAG_PARSED)==0))
      return ReadOk;
    }

  if (state.has_packet)
    submit_ogg_packet();

  while(packet) {
    if (!fetch_next_packet()){
      GM_DEBUG_PRINT("[ogg] unexpected end of stream\n");
      packet->flags|=FLAG_EOS;
      state.has_eos=true;
      context->post_packet(packet);
      return ReadDone;
      }
    submit_ogg_packet();
    }


  if (input->serial() && state.has_eos && !input->eof()) {
    flags&=~(FLAG_PARSED|FLAG_OGG_VORBIS|FLAG_OGG_FLAC|FLAG_OGG_OPUS|FLAG_VORBIS_MASK);
    if (state.has_stream) {
      ogg_stream_clear(&stream);
      state.has_stream=false;
      }
    state.has_eos=false;
    return ReadOk;
    }


  if (state.has_eos)
    return ReadDone;
  else
    return ReadOk;
  }


ReaderPlugin * ap_ogg_reader(InputContext * ctx) {
  return new OggReader(ctx);
  }


}









