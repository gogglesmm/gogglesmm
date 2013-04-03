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









#if 0

FXuint ap_wait(FXInputHandle handle,FXlong timeout) {
#ifndef WIN32
  struct timespec delta;

  fd_set rd;
  fd_set wr;
  fd_set er;

  FD_ZERO(&rd);
  FD_ZERO(&wr);
  FD_ZERO(&er);

  FD_SET((int)handle,&rd);
  FD_SET((int)handle,&er);
  int maxfds=(int)handle;

  delta.tv_nsec=blocking%1000000000;
  delta.tv_sec=blocking/1000000000;

  if (pselect(maxfds+1,&rd,&wr,&er,&delta,NULL)>0) {
    if (FD_ISSET(handle,&rd) || FD_ISSET(handle,&er)) {
      return 1;
      }
    }
#endif
  return 0;
  }
#endif

FXuint ap_wait(FXInputHandle handle,FXTime timeout) {
#ifndef WIN32
  struct timespec delta;
  FXuint result=WIO_TIMEOUT;

  fd_set rd;
  fd_set wr;
  fd_set er;

  FD_ZERO(&rd);
  FD_ZERO(&wr);
  FD_ZERO(&er);

  FD_SET((int)handle,&rd);
  FD_SET((int)handle,&er);
  int maxfds=(int)handle;

  if (timeout) {
    delta.tv_nsec=timeout%1000000000;
    delta.tv_sec=timeout/1000000000;
    if (pselect(maxfds+1,&rd,&wr,&er,&delta,NULL)>0) {
      if (FD_ISSET(handle,&rd) || FD_ISSET(handle,&er)) {
        result|=WIO_HANDLE;
        }
      }
    }
  else {
    if (pselect(maxfds+1,&rd,&wr,&er,NULL,NULL)>0) {
      if (FD_ISSET(handle,&rd) || FD_ISSET(handle,&er)) {
        result|=WIO_HANDLE;
        }
      }
    }
  #endif
  return result;
  }

//FXuint ap_wait(FXInputHandle h1,FXInputHandle h2) {
//  return ap_wait_read(h1,h2,0);
 // }


FXuint ap_wait_write(FXInputHandle interrupt,FXInputHandle handle,FXTime timeout) {
//  fxmessage("Wait for write\n");
#ifndef WIN32
  FXuint result=WIO_TIMEOUT;
  int maxfds=FXMAX(interrupt,handle);
  struct timespec delta;

  fd_set rd;
  fd_set wr;
  fd_set er;

  FD_ZERO(&rd);
  FD_ZERO(&wr);
  FD_ZERO(&er);

  FD_SET((int)interrupt,&rd);
  FD_SET((int)interrupt,&er);
  FD_SET((int)handle,&wr);
  FD_SET((int)handle,&er);

  if (timeout) {
    delta.tv_nsec=timeout%1000000000;
    delta.tv_sec=timeout/1000000000;
    if (pselect(maxfds+1,&rd,&wr,&er,&delta,NULL)) {
      if (FD_ISSET((int)interrupt,&rd) || FD_ISSET((int)interrupt,&er) )
        result|=WIO_INTERRUPT;

      if(FD_ISSET((int)handle,&wr) || FD_ISSET((int)handle,&er))
        result|=WIO_HANDLE;
      }
    }
  else {
    if (pselect(maxfds+1,&rd,&wr,&er,NULL,NULL)) {
      if (FD_ISSET((int)interrupt,&rd) || FD_ISSET((int)interrupt,&er) )
        result|=WIO_INTERRUPT;

      if(FD_ISSET((int)handle,&wr) || FD_ISSET((int)handle,&er))
        result|=WIO_HANDLE;
      }
    }
  return result;
#endif
  }



FXuint ap_wait_read(FXInputHandle interrupt,FXInputHandle handle,FXTime timeout){
//  fxmessage("Wait for read\n");
#ifndef WIN32
  FXuint result=WIO_TIMEOUT;
  int maxfds=FXMAX(interrupt,handle);
  struct timespec delta;

  fd_set rd;
  fd_set wr;
  fd_set er;

  FD_ZERO(&rd);
  FD_ZERO(&wr);
  FD_ZERO(&er);

  FD_SET((int)interrupt,&rd);
  FD_SET((int)interrupt,&er);
  FD_SET((int)handle,&rd);
  FD_SET((int)handle,&er);

  if (timeout) {
    delta.tv_nsec=timeout%1000000000;
    delta.tv_sec=timeout/1000000000;
    if (pselect(maxfds+1,&rd,&wr,&er,&delta,NULL)) {
      if (FD_ISSET((int)interrupt,&rd) || FD_ISSET((int)interrupt,&er)){
        result|=WIO_INTERRUPT;
        }
      if(FD_ISSET((int)handle,&rd) || FD_ISSET((int)handle,&er)) {
        result|=WIO_HANDLE;
        }
      }
    }
  else {
    if (pselect(maxfds+1,&rd,&wr,&er,NULL,NULL)) {
      if (FD_ISSET((int)interrupt,&rd) || FD_ISSET((int)interrupt,&er)){
        result|=WIO_INTERRUPT;
        }
      if(FD_ISSET((int)handle,&rd) || FD_ISSET((int)handle,&er)) {
        result|=WIO_HANDLE;
        }
      }
    }
  return result;
#endif
  }



}
