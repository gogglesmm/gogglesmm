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
