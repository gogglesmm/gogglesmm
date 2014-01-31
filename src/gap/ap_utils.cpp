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
#include "ap_utils.h"
#include "ap_format.h"
#include "ap_event.h"


// for prctl
#ifdef __linux__
#include <sys/prctl.h>
#endif

// for fcntl
#ifndef WIN32
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>
#endif

namespace ap {


// PUBLIC API
//----------------------------------------------------

void ap_set_thread_name(const FXchar * name) {
#ifdef __linux__
  prctl(PR_SET_NAME,(unsigned long)name,0,0,0);
#endif
  }

FXString ap_get_environment(const FXchar * key,const FXchar * def) {
  FXString value = FXSystem::getEnvironment(key);
  if (value.empty())
    return def;
  else
    return value;
  }


FXbool ap_set_nonblocking(FXInputHandle fd) {
#ifndef WIN32
  FXint flags = fcntl(fd,F_GETFL);
  if (flags==-1 || fcntl(fd,F_SETFL,(flags|O_NONBLOCK))==-1)
    return false;
#endif
  return true;
  }

FXbool ap_set_closeonexec(FXInputHandle fd) {
#ifndef WIN32
  FXint flags;
  flags = fcntl(fd,F_GETFD);
  if (flags==-1 || fcntl(fd,F_SETFD,(flags|FD_CLOEXEC))==-1)
    return false;
#endif
  return true;
  }


// PRIVATE API
//----------------------------------------------------



void ap_meta_from_vorbis_comment(MetaInfo * meta, const FXchar * comment,FXint len) {
  FXASSERT(meta);
  if (comparecase(comment,"TITLE=",6)==0){
    if (!meta->title.empty()) meta->title.append(' ');
    meta->title.append(comment+6,len-6);
    }
  else if (meta->artist.empty() && comparecase(comment,"ARTIST=",7)==0){
    meta->artist.assign(comment+7,len-7);
    }
  else if (meta->album.empty() && comparecase(comment,"ALBUM=",6)==0){
    meta->album.assign(comment+6,len-6);
    }
  }


void ap_replaygain_from_vorbis_comment(ReplayGain & gain,const FXchar * comment,FXint len) {
  if (len>22) {
    if (comparecase(comment,"REPLAYGAIN_TRACK_GAIN=",22)==0){
      FXString tag(comment+22,len-22);
      tag.scan("%lg",&gain.track);
      }
    else if (comparecase(comment,"REPLAYGAIN_TRACK_PEAK=",22)==0){
      FXString tag(comment+22,len-22);
      tag.scan("%lg",&gain.track_peak);
      }
    else if (comparecase(comment,"REPLAYGAIN_ALBUM_GAIN=",22)==0){
      FXString tag(comment+22,len-22);
      tag.scan("%lg",&gain.album);
      }
    else if (comparecase(comment,"REPLAYGAIN_ALBUM_PEAK=",22)==0){
      FXString tag(comment+22,len-22);
      tag.scan("%lg",&gain.album_peak);
      }
    }
  }


void ap_parse_vorbiscomment(const FXchar * buffer,FXint len,ReplayGain & gain,MetaInfo * meta) {
  FXString comment;
  FXint size=0;
  FXint ncomments=0;
  const FXchar * end = buffer+len;

  /// Vendor string
  size = INT32_LE(buffer);
  if (size) buffer+=4+size;
  if (buffer>=end) return;

  /// Number of user comments
  ncomments = INT32_LE(buffer);
  buffer+=4;

  for (FXint i=0;i<ncomments && (buffer<=end);i++) {
    size = INT32_LE(buffer);
    if (buffer+size+4>end)
      return;
    ap_replaygain_from_vorbis_comment(gain,buffer+4,size);
    ap_meta_from_vorbis_comment(meta,buffer+4,size);
    buffer+=4+size;
    }
  }






const FXchar Base64Encoder::base64[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";


Base64Encoder::Base64Encoder(FXint source_length) : nbuffer(0), index(0){
  if (source_length)
    out.length(4*(source_length/3));
  }

FXString Base64Encoder::encodeString(const FXString & source) {
  Base64Encoder base64(source.length());
  base64.encode(source);
  base64.finish();
  return base64.getOutput();
  }

void Base64Encoder::encode(FXuint value) {
  encode((const FXuchar*)&value,4);
  }

void Base64Encoder::encode(const FXString & str) {
  encode((const FXuchar*)str.text(),str.length());
  }

void Base64Encoder::encodeChunks(const FXuchar * in,FXint len) {

  // resize buffer if needed
  FXint needed = 4*(len/3);
  if (index+needed>=out.length()) {
    out.length(out.length()+needed-(out.length()-index));
    }

  for (int i=0;i<len;i+=3) {
    out[index++]=base64[(in[i]>>2)];
    out[index++]=base64[((in[i]&0x3)<<4)|(in[i+1]>>4)];
    out[index++]=base64[((in[i+1]&0xf)<<2)|(in[i+2]>>6)];
    out[index++]=base64[(in[i+2]&0x3f)];
    }
  }

void Base64Encoder::finish() {
  if (nbuffer) {
    if (index+4>=out.length()) {
      out.length(out.length()+4-(out.length()-index));
      }
    out[index++]=base64[(buffer[0]>>2)];
    if (nbuffer>1) {
      out[index++]=base64[((buffer[0]&0x3)<<4)|(buffer[1]>>4)];
      out[index++]=base64[((buffer[1]&0xf)<<2)];
      out[index++]='=';
      }
    else {
      out[index++]=base64[((buffer[0]&0x3)<<4)];
      out[index++]='=';
      out[index++]='=';
      }
    }
  }

void Base64Encoder::encode(const FXuchar * in,FXint len) {
  if (len) {
    FXint rindex=0;

    if (nbuffer) {
      for (rindex=0;(nbuffer<3)&&(rindex<len);rindex++)
        buffer[nbuffer++]=in[rindex];

      if (nbuffer<3)
        return;

      encodeChunks(buffer,3);
      len-=rindex;
      nbuffer=0;
      }

    FXint r = len % 3;
    FXint n = len - r;
    if (n) encodeChunks(in+rindex,n);

    for (int i=0;i<r;i++)
      buffer[i]=in[rindex+n+i];

    nbuffer=r;
    }
  }



FXuint ap_wait(FXInputHandle io,FXInputHandle watch,FXTime timeout,FXuchar mode){
#ifndef WIN32
  FXint n,nfds=1;
  struct pollfd fds[2];
  fds[0].fd    	= io;
  fds[0].events = (mode==WaitReadable) ? POLLIN : POLLOUT;
  if (watch!=BadHandle) {
    fds[1].fd 	  = watch;
    fds[1].events = POLLIN;
    nfds=2;
    }
  if (timeout) {
    struct timespec ts;
    ts.tv_sec  = (timeout / 1000000000);
    ts.tv_nsec = (timeout % 1000000000);
    do {
      n=ppoll(fds,nfds,&ts,NULL);
      }
    while(n==-1 && (errno==EAGAIN || errno==EINTR));
    }
  else {
    do {
      n=ppoll(fds,nfds,NULL,NULL);
      }
    while(n==-1 && (errno==EAGAIN || errno==EINTR));
    }
  if (0<n) {
    if (watch!=BadHandle && fds[1].revents)
      return WaitHasInterrupt;
    else
      return WaitHasIO;
    }
  else if (n==0)
    return WaitHasTimeout;
  else
    return WaitHasError;
#endif
  }

}

