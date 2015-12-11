/*******************************************************************************
*                         Goggles Audio Player Library                         *
********************************************************************************
*           Copyright (C) 2013-2016 by Sander Jansen. All Rights Reserved      *
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
#include "ap_socket.h"
#include "ap_utils.h"


#ifndef WIN32
#include <unistd.h> // for close()
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#endif


#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif


namespace ap {

Socket::Socket(FXInputHandle h,FXuint m) : FXIODevice(h,m|ReadWrite|OwnHandle) {
#ifndef SOCK_CLOEXEC
  if (!ap_set_closeonexec(device)){
    ::close(device);
    return;
    }
#endif

#ifndef SOCK_NONBLOCK
  if (access&FXIO::NonBlocking && !ap_set_nonblocking(device)){
    ::close(device);
    return;
    }
#endif
  }


Socket * Socket::create(int domain,int type,int protocol,FXuint mode) {
  // On linux 2.6.27 we can pass additional socket options
  int opts=0;
#ifdef SOCK_CLOEXEC
  opts|=SOCK_CLOEXEC;
#endif

#ifdef SOCK_NONBLOCK
  if (mode&FXIO::NonBlocking) {
    opts|=SOCK_NONBLOCK;
    }
#endif
  FXInputHandle handle = socket(domain,type|opts,protocol);
  if (handle!=BadHandle) {
    return new Socket(handle,mode);
    }
  return NULL;
  }


FXbool Socket::setKeepAlive(FXbool enable) {
  int value = (enable) ? 1 : 0;
  if (setsockopt(device,SOL_SOCKET,SO_KEEPALIVE,&value,sizeof(int))!=0)
    return false;
  return true;
  }

FXbool Socket::getKeepAlive() const {
  int 			 value=0;
  socklen_t length=sizeof(value);
  if (getsockopt(device,SOL_SOCKET,SO_KEEPALIVE,&value,&length)==0)
    return value;
  else
    return false;
  }

FXbool Socket::setReuseAddress(FXbool enable) {
  int value = (enable) ? 1 : 0;
  if (setsockopt(device,SOL_SOCKET,SO_REUSEADDR,&value,sizeof(int))!=0)
    return false;
  return true;
  }

FXbool Socket::getReuseAddress() const {
  int value = 0;
  socklen_t length=sizeof(value);
  if (getsockopt(device,SOL_SOCKET,SO_REUSEADDR,&value,&length)==0)
    return value;
  else
    return false;
  }

FXbool Socket::setLinger(FXTime tm) {
  struct linger l;
  l.l_onoff  = (tm>0) ? 1 : 0;
  l.l_linger = tm;
  if (setsockopt(device,SOL_SOCKET,SO_LINGER,&l,sizeof(struct linger))!=0)
    return false;
  return true;
  }

FXTime Socket::getLinger() const {
  struct linger l;
  socklen_t length=sizeof(struct linger);
  if (getsockopt(device,SOL_SOCKET,SO_LINGER,&l,&length)==0 && (l.l_onoff) ){
    return l.l_linger;
    }
  return 0;
  }

FXbool Socket::setReceiveTimeout(FXTime tm) {
  struct timeval tv;
  tv.tv_sec  = tm / 1000000000;
  tv.tv_usec = (tm % 1000000000) / 1000;
  if (setsockopt(device,SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof(struct timeval))!=0)
    return false;
  return true;
  }

FXbool Socket::setSendTimeout(FXTime tm) {
  struct timeval tv;
  tv.tv_sec  = tm / 1000000000;
  tv.tv_usec = (tm % 1000000000) / 1000;
  if (setsockopt(device,SOL_SOCKET,SO_SNDTIMEO,&tv,sizeof(struct timeval))!=0)
    return false;
  return true;
  }

FXint Socket::getError() const {
  int value = 0;
  socklen_t length=sizeof(value);
  if (getsockopt(device,SOL_SOCKET,SO_ERROR,&value,&length)==0)
    return value;
  else
    return -1;
  }

FXival Socket::readBlock(void* data,FXival count){
  FXival nread=-1;
  do{
    nread=::recv(device,data,count,MSG_NOSIGNAL);
    }
  while(nread<0 && errno==EINTR);


  if ((nread==0 && count>0) || (nread<0 && errno!=EWOULDBLOCK && errno!=EAGAIN))
    access|=EndOfStream;

  return nread;
  }


FXival Socket::writeBlock(const void* data,FXival count){
  FXival nwritten=-1;
  do{
    nwritten=::send(device,data,count,MSG_NOSIGNAL);
    }
  while(nwritten<0 && errno==EINTR);

  if ((nwritten==0 && count>0) || (nwritten<0 && errno!=EWOULDBLOCK && errno!=EAGAIN))
    access|=EndOfStream;

  return nwritten;
  }


FXbool Socket::close() {
  if (isOpen()) {
    shutdown(device,SHUT_RDWR);
    return FXIODevice::close();
    }
  return true;
  }


FXint Socket::eof() {
  return (access&EndOfStream) ? 1 : 0;
  }

}
