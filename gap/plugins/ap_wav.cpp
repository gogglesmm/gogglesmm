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
#include "ap_event_private.h"
#include "ap_reader_plugin.h"
#include "ap_input_plugin.h"
#include "ap_decoder_thread.h"



static FXshort alaw2lpcm(FXuchar value) {
  FXshort t;
  FXshort seg;

  value ^= 0x55;
  t = (value & 0xf) << 4;
  seg = ((unsigned)value & 0x70) >> 4;
  switch (seg) {
    case 0:
      t += 8;
      break;
    case 1:
      t += 0x108;
      break;
    default:
      t += 0x108;
      t <<= seg - 1;
    }
  return ((value & 0x80) ? t : -t);
  }

static FXshort ulaw2lpcm(FXuchar value) {
  FXshort t;

  value = ~value;
  t = ((value & 0xf) << 3) + 0x84;
  t <<= ((unsigned)value & 0x70) >> 4;

  return ((value & 0x80) ? (0x84 - t) : (t - 0x84));
  }


namespace ap {

class WavReader : public ReaderPlugin {
protected:
  FXlong   datasize;    // size of the data section
  FXlong   input_start;
  FXushort wavformat;
protected:
  ReadStatus parse();
public:
  WavReader(AudioEngine*);
  FXbool init(InputPlugin*);
  ReadStatus process(Packet*);

  FXuchar format() const { return Format::WAV; };

  FXbool can_seek() const;
  FXbool seek(FXlong);
  virtual ~WavReader();
  };


enum {
  WAV_FORMAT_PCM        = 0x0001,
  WAV_FORMAT_FLOAT      = 0x0003,
  WAV_FORMAT_ALAW       = 0x0006,
  WAV_FORMAT_ULAW       = 0x0007,
  WAV_FORMAT_EXTENSIBLE = 0xFFFE
  };


typedef FXuchar ap_guid_t[16];

const ap_guid_t guid_wav_format_pcm={0x01,0x00,0x00,0x00,0x00,0x00,0x10,0x00,0x80,0x00,0x00,0xaa,0x00,0x38,0x9b,0x71};
const ap_guid_t guid_wav_format_float={0x03,0x00,0x00,0x00,0x00,0x00,0x10,0x00,0x80,0x00,0x00,0xaa,0x00,0x38,0x9b,0x71};


WavReader::WavReader(AudioEngine*e) : ReaderPlugin(e),datasize(0),input_start(0) {
  }

WavReader::~WavReader(){
  }

FXbool WavReader::init(InputPlugin*plugin) {
  ReaderPlugin::init(plugin);
  datasize=0;
  input_start=0;
  return true;
  }

FXbool WavReader::can_seek() const {
  return (datasize>0);
  }


FXbool WavReader::seek(FXlong pos){
  FXlong offset=input_start + FXCLAMP(0,pos*af.framesize(),datasize);
  input->position(offset,FXIO::Begin);
  return true;
  }

ReadStatus WavReader::process(Packet*packet) {

  if (!(flags&FLAG_PARSED)) {
    if (parse()!=ReadOk)
      return ReadError;
    }

  packet->af              = af;
  packet->stream_position = static_cast<FXint>( (input->position()-input_start) / af.framesize());
  packet->stream_length   = stream_length;
    

  if (wavformat == WAV_FORMAT_ALAW) {
    FXint nsamples = (packet->space() / af.framesize());
    FXshort * data = packet->s16();
    FXuchar value;FXint s;
    for (s=0;s<nsamples;s++) {
      FXint nread = input->read(&value,1);
      if (nread<0) {
        packet->unref();
        return ReadError;
        }
      else if (nread==0) {
        packet->wroteBytes(s*2);
        packet->flags=FLAG_EOS;
        engine->decoder->post(packet);
        return ReadDone;
        }
      data[s] = alaw2lpcm(value);
      }
    packet->wroteBytes(s*2);
    }
  else if (wavformat == WAV_FORMAT_ULAW) {
    FXint nsamples = (packet->space() / af.framesize());
    FXshort * data = packet->s16();
    FXuchar value;FXint s;
    for (s=0;s<nsamples;s++) {
      FXint nread = input->read(&value,1);
      if (nread<0) {
        packet->unref();
        return ReadError;
        }
      else if (nread==0) {
        packet->wroteBytes(s*2);
        packet->flags=FLAG_EOS;
        engine->decoder->post(packet);
        return ReadDone;
        }
      data[s] = ulaw2lpcm(value);
      }
    packet->wroteBytes(s*2);
    }
  else {
    FXint nbytes = (packet->space() / af.framesize()) * af.framesize();
    FXint nread = input->read(packet->ptr(),nbytes);
    if (nread<0) {
      packet->unref();
      return ReadError;
      }
    else if (nread==0){
      packet->unref();
      return ReadDone;
      }
    packet->wroteBytes(nread);
    }


  if (input->eof())
    packet->flags=FLAG_EOS;
  else
    packet->flags=0;
  engine->decoder->post(packet);
  return ReadOk;
  }


static FXuint get_channel_order(const FXuint wavmask,const FXuint channels){
/*
#define SPEAKER_FRONT_LEFT             0x1
#define SPEAKER_FRONT_RIGHT            0x2
#define SPEAKER_FRONT_CENTER           0x4
#define SPEAKER_LOW_FREQUENCY          0x8
#define SPEAKER_BACK_LEFT              0x10
#define SPEAKER_BACK_RIGHT             0x20
#define SPEAKER_FRONT_LEFT_OF_CENTER   0x40
#define SPEAKER_FRONT_RIGHT_OF_CENTER  0x80
#define SPEAKER_BACK_CENTER            0x100
#define SPEAKER_SIDE_LEFT              0x200
#define SPEAKER_SIDE_RIGHT             0x400
#define SPEAKER_TOP_CENTER             0x800
#define SPEAKER_TOP_FRONT_LEFT         0x1000
#define SPEAKER_TOP_FRONT_CENTER       0x2000
#define SPEAKER_TOP_FRONT_RIGHT        0x4000
#define SPEAKER_TOP_BACK_LEFT          0x8000
#define SPEAKER_TOP_BACK_CENTER        0x10000
#define SPEAKER_TOP_BACK_RIGHT         0x20000
#define SPEAKER_RESERVED               0x80000000
*/

  GM_DEBUG_PRINT("[wav_reader] channel mask %u\n",wavmask);

  static const FXuint wav_channel_order[]={
    Channel::FrontLeft,
    Channel::FrontRight,
    Channel::FrontCenter,
    Channel::LFE,
    Channel::BackLeft,
    Channel::BackRight,
    Channel::None,
    Channel::None,
    Channel::BackCenter,
    Channel::SideLeft,
    Channel::SideRight
    };

  if (channels<1 || channels>8)
    return 0;

  if (wavmask==0) {
    if (channels==1)
      return AP_CMAP1(Channel::Mono);
    else if (channels==2)
      return AP_CMAP2(Channel::FrontLeft,Channel::FrontRight);
    else
      return 0;
    }

  FXuint order=0,cpos=0,cbit=0;
  while(cpos<channels && cbit<11) {
    if ((wavmask>>cbit)&0x1) {
      // unsupported channel
      if (wav_channel_order[cbit]==Channel::None)
        return 0;
      order|=wav_channel_order[cbit]<<(cpos<<2);
      cpos++;
      }
    cbit++;
    }
  return (cpos==channels) ? order : 0;
  }


ReadStatus WavReader::parse() {
  FXchar  chunkid[4];
  FXuint  chunksize;
  FXushort channels;
  FXuint rate;
  FXuint byterate;
  FXushort samplesize;
  FXushort block;

  ap_guid_t  subconfig;
  FXushort validbitspersample;
  FXuint   channelmask  = 0;
  FXuint   channelorder = 0;

  FXbool has_fmt=false;

  GM_DEBUG_PRINT("parsing wav header\n");

  if (input->read(&chunkid,4)!=4)
    return ReadError;
 
  if (input->read(&chunksize,4)!=4)
    return ReadError;

  if (compare(chunkid,"RIFF",4) && compare(chunkid,"RF64",4)) 
    return ReadError;
 
  if (input->read(&chunkid,4)!=4 || compare(chunkid,"WAVE",4)) 
    return ReadError;

  while(1) {

    if (input->read(&chunkid,4)!=4)
      return ReadError;

    if (input->read(&chunksize,4)!=4)
      return ReadError;

    if (compare(chunkid,"data",4)==0) {

      if (has_fmt==false)
        return ReadError;

      input_start = input->position();

      switch(wavformat) {
        case WAV_FORMAT_ALAW:
        case WAV_FORMAT_ULAW: 
          {
            af.set(AP_FORMAT_S16,rate,channels,channelorder);
          } break;

        case WAV_FORMAT_PCM :
          {
            if (samplesize>8)
              af.set(Format::Signed|Format::Little,samplesize,samplesize>>3,rate,channels,channelorder);
            else
              af.set(Format::Unsigned|Format::Little,samplesize,samplesize>>3,rate,channels,channelorder);
          } break;

        case WAV_FORMAT_FLOAT:
          {
            if (samplesize!=32) {
              GM_DEBUG_PRINT("unsupported float format with samplesize %d\n",samplesize);
              return ReadError;
              }
            af.set(Format::Float|Format::Little,samplesize,samplesize>>3,rate,channels,channelorder);
          } break;

        case WAV_FORMAT_EXTENSIBLE: 
          {
            if (memcmp(subconfig,guid_wav_format_pcm,16)==0) {
              wavformat = WAV_FORMAT_PCM;
              if (samplesize>8)
                af.set(Format::Signed|Format::Little,validbitspersample,samplesize>>3,rate,channels,channelorder);
              else  
                af.set(Format::Unsigned|Format::Little,validbitspersample,samplesize>>3,rate,channels,channelorder);
              }
            else if (memcmp(subconfig,guid_wav_format_float,16)==0) {
              wavformat = WAV_FORMAT_FLOAT;
              if (samplesize!=32) {
                GM_DEBUG_PRINT("unsupported float format with samplesize %d\n",samplesize);
                return ReadError;
                }
              af.set(Format::Float|Format::Little,validbitspersample,samplesize>>3,rate,channels,channelorder);
              }
            else
              return ReadError;
          } break;

        default: return ReadError; break;
        }
 
#ifdef DEBUG
     af.debug();
     if (block!=af.framesize())
       GM_DEBUG_PRINT("[wav_reader] warning: blockalign not the same as framesize\n");
#endif
      flags|=FLAG_PARSED;
      stream_length=-1;
      if (!input->serial()) {
        datasize      = FXMIN(input->size() - input_start,chunksize);
        stream_length = (datasize) / af.framesize();
        }
      GM_DEBUG_PRINT("[wav_reader] stream_length %ld\n",stream_length);
      engine->decoder->post(new ConfigureEvent(af,Codec::PCM));
      return ReadOk;
      }
    else if (compare(chunkid,"ds64",4)==0) {
      input->position(chunksize,FXIO::Current);
      }
    else if (compare(chunkid,"fmt ",4)==0) {

      if (input->read(&wavformat,2)!=2 || !(wavformat==WAV_FORMAT_PCM || wavformat==WAV_FORMAT_FLOAT || wavformat==WAV_FORMAT_ALAW || wavformat==WAV_FORMAT_ULAW  || wavformat==WAV_FORMAT_EXTENSIBLE) ) {
        GM_DEBUG_PRINT("[wav_reader] unsupported format: %x\n",wavformat);
        return ReadError;
        }

      if (input->read(&channels,2)!=2)
        return ReadError;

      if (input->read(&rate,4)!=4)
        return ReadError;

      if (input->read(&byterate,4)!=4)
        return ReadError;

      if (input->read(&block,2)!=2)
        return ReadError;

      if (input->read(&samplesize,2)!=2)
        return ReadError;

      chunksize-=16;

      // Require channelmask for channels>2
      if (channels>2 && wavformat!=WAV_FORMAT_EXTENSIBLE) {
        GM_DEBUG_PRINT("[wav_reader] unsupported %d channels without Extensible Format\n",channels);
        return ReadError;
        }

      if (wavformat==WAV_FORMAT_EXTENSIBLE) {

        if (input->read(&validbitspersample,2)!=2)
          return ReadError;

        if (input->read(&validbitspersample,2)!=2)
          return ReadError;

        if (input->read(&channelmask,4)!=4)
          return ReadError;

        if (input->read(&subconfig,16)!=16)
          return ReadError;

        // Make sure it's PCM or Float
        if (memcmp(subconfig,guid_wav_format_pcm,16)!=0 && memcmp(subconfig,guid_wav_format_float,16)!=0)
          return ReadError;

        chunksize-=24;
        }

      GM_DEBUG_PRINT("chunksize left: %d\n",chunksize);
      input->position(chunksize,FXIO::Current);

      // Get the channel order
      channelorder = get_channel_order(channelmask,channels);
      if (channelorder==Channel::None) {
        GM_DEBUG_PRINT("[wav_reader] unknown channel order\n");
        return ReadError;
        }

      has_fmt=true;
      }
    else {
      input->position(chunksize,FXIO::Current);
      }
    }
  return ReadError;
  }


ReaderPlugin * ap_wav_reader(AudioEngine * engine) {
  return new WavReader(engine);
  }
}