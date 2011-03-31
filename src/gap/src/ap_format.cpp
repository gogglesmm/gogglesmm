#include "ap_defs.h"
#include "ap_format.h"

namespace ap {

static const FXchar * const codecs[]={
  "Invalid",
  "PCM",
  "FLAC",
  "Vorbis",
  "Musepack",
  "MPEG",
  "AAC"
  };

static const FXchar * const byteorders[]={
  "le",
  "be"
  };

static const FXchar * const formats[]={
  "s",
  "u",
  "f",
  "iec958_frame"
  "Reserved1"
  "Reserved2"
  "Reserved3"
  "Reserved4"
  };


const FXchar * Codec::name(FXuchar c){
  return codecs[c];
  }



AudioFormat::AudioFormat() : rate(0),format(0),channels(0) {
  }
  

AudioFormat::AudioFormat(const AudioFormat & af) {
  format  =af.format;
  rate    =af.rate;
  channels=af.channels;
  }

void AudioFormat::reset() {
  format=0;
  rate=0;
  channels=0;
  }

void AudioFormat::set(FXushort dt,FXushort bps,FXushort pack,FXuint r,FXuchar nc) {
  format=dt|((bps-1)<<Format::Bits_Shift)|((pack-1)<<Format::Pack_Shift);
  rate=r;
  channels=nc;
  }

void AudioFormat::set(FXushort fmt,FXuint r,FXuchar nc) {
  format=fmt;
  rate=r;
  channels=nc;
  }


FXbool AudioFormat::swap() {
  if (packing()>Format::Pack_1) {
    format^=(1<<Format::Order_Shift);
    return true;
    }
  else {
    return false;
    }
  }


/*
  24 -> 32 -> 16
  flt -> 32 -> 16
*/

FXbool AudioFormat::compatible() {
  switch(format){
    case AP_FORMAT_S24_3BE  : format=AP_FORMAT_S24_BE; break;
    case AP_FORMAT_S24_3LE  : format=AP_FORMAT_S24_LE; break;
    case AP_FORMAT_S24_LE   : format=AP_FORMAT_S32_LE; break;
    case AP_FORMAT_S24_BE   : format=AP_FORMAT_S32_BE; break;
    case AP_FORMAT_S32_LE   : format=AP_FORMAT_S16_LE; break;
    case AP_FORMAT_S32_BE   : format=AP_FORMAT_S16_BE; break;
    case AP_FORMAT_FLOAT_LE : format=AP_FORMAT_S32_LE; break;
    case AP_FORMAT_FLOAT_BE : format=AP_FORMAT_S32_BE; break;
    default                 : return false;            break;
    }
  return true;
  }


void AudioFormat::debug() const {
  fxmessage("format: %6dHz, %dch, %s%2d%s%d\n",rate,channels,formats[datatype()],bps(),byteorders[byteorder()],packing());
  }


FXbool operator!=(const AudioFormat& af1,const AudioFormat& af2){
  if ( (af1.format!=af2.format) ||
       (af1.rate!=af2.rate) ||
       (af1.channels!=af2.channels) )
    return true;
  else
    return false;
  }

FXbool operator==(const AudioFormat& af1,const AudioFormat& af2){
  if ( (af1.format!=af2.format) ||
       (af1.rate!=af2.rate) ||
       (af1.channels!=af2.channels) )
    return false;
  else
    return true;
  }
}
