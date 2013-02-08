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
#include "ap_format.h"

namespace ap {

static const FXchar * const codecs[]={
  "Invalid",
  "PCM",
  "FLAC",
  "Vorbis",
  "Musepack",
  "MPEG",
  "AAC",
  "Opus",
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

void AudioFormat::set(FXushort dt,FXushort bits,FXushort pack,FXuint r,FXuchar nc) {
  format=dt|((bits-1)<<Format::Bits_Shift)|((pack-1)<<Format::Pack_Shift);
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


extern FXuint ap_format_from_mime(const FXString & mime) {
  if (comparecase(mime,"audio/mpeg")==0) {
    return Format::MP3;
    }
  else if (comparecase(mime,"audio/ogg")==0){
    return Format::OGG;
    }
  else if (comparecase(mime,"audio/aacp")==0){
    return Format::AAC;
    }
  else if (comparecase(mime,"audio/x-mpegurl")==0 ||
           comparecase(mime,"audio/mpegurl")==0) {
    return Format::M3U;
    }
  else if ((comparecase(mime,"application/pls+xml")==0) || /// wrong mimetype, but NPR actually returns this: http://www.npr.org/streams/mp3/nprlive24.pls
           (comparecase(mime,"audio/x-scpls")==0)){
    return Format::PLS;
    }
  else if (comparecase(mime,"application/xspf+xml")==0){
    return Format::XSPF;
    }
  else if (comparecase(mime,"video/x-ms-asf")==0) { /// Either ASF or ASX...
    return Format::ASFX;
    }
  else if (comparecase(mime,"audio/x-ms-wax")==0) {
    return Format::ASX;
    }
  else {
    return Format::Unknown;
    }
  }

extern FXuint ap_format_from_extension(const FXString & extension) {
  if (comparecase(extension,"wav")==0)
    return Format::WAV;
  else if (comparecase(extension,"flac")==0)
    return Format::FLAC;
  else if (comparecase(extension,"ogg")==0 || comparecase(extension,"oga")==0 || comparecase(extension,"opus")==0)
    return Format::OGG;
  else if (comparecase(extension,"mp3")==0)
    return Format::MP3;
  else if (comparecase(extension,"mpc")==0)
    return Format::Musepack;
  else if (comparecase(extension,"mp4")==0 ||
           comparecase(extension,"m4a")==0 ||
           comparecase(extension,"m4p")==0 ||
           comparecase(extension,"m4b")==0 )
    return Format::MP4;
  else if (comparecase(extension,"aac")==0)
    return Format::AAC;
  else if (comparecase(extension,"wv")==0)
    return Format::WavPack;
  else if (comparecase(extension,"m3u")==0)
    return Format::M3U;
  else if (comparecase(extension,"pls")==0)
    return Format::PLS;
  else if (comparecase(extension,"xspf")==0)
    return Format::XSPF;
  else if (comparecase(extension,"asx")==0)
    return Format::ASX;
  else if (comparecase(extension,"asf")==0)
    return Format::ASF;
  else
    return Format::Unknown;
  }


extern FXuint ap_format_from_buffer(const FXchar * buffer,FXival size) {
  if (size>=4 && comparecase(buffer,"<ASX",4)==0)
    return Format::ASX;
  else
    return Format::Unknown;
  }



}
