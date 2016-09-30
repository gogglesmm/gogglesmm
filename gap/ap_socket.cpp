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


#ifdef _WIN32
#include <WinSock2.h>
#else
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
#ifdef _WIN32
Socket::Socket(FXuint h, FXuint m) : FXIODevice(WSACreateEvent(), m | ReadWrite | OwnHandle), sockethandle(h) {

	if (access&FXIO::NonBlocking) {
		u_long blocking = 1;
		ioctlsocket(sockethandle, FIONBIO, &blocking);
	}

	}
#else
	Socket::Socket(FXInputHandle h, FXuint m) : FXIODevice(h, m | ReadWrite | OwnHandle) {
#ifndef SOCK_CLOEXEC
		if (!ap_set_closeonexec(device)) {
			::close(device);
			return;
		}
#endif

#ifndef SOCK_NONBLOCK
		if (access&FXIO::NonBlocking && !ap_set_nonblocking(device)) {
			::close(device);
			return;
		}
#endif
}
#endif
  


Socket * Socket::create(int domain,int type,int protocol,FXuint mode) {
#ifdef _WIN32
  FXuint handle= socket(domain, type, protocol);
  if (handle != INVALID_SOCKET) {
	return new Socket(handle, mode);
	}
  return nullptr;
#else
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
  handle = socket(domain,type|opts,protocol);
  if (handle!=BadHandle) {
    return new Socket(handle,mode);
    }
  return nullptr;
#endif
  }


FXbool Socket::setKeepAlive(FXbool enable) {
#ifndef _WIN32
  int value = (enable) ? 1 : 0;
  if (setsockopt(device,SOL_SOCKET,SO_KEEPALIVE,&value,sizeof(int))!=0)
    return false;
#endif
  return true;
  }

FXbool Socket::getKeepAlive() const {
#ifndef _WIN32
  int 			 value=0;
  socklen_t length=sizeof(value);
  if (getsockopt(device,SOL_SOCKET,SO_KEEPALIVE,&value,&length)==0)
    return value;
  else
#endif
    return false;
  }

FXbool Socket::setReuseAddress(FXbool enable) {
#ifndef _WIN32
  int value = (enable) ? 1 : 0;
  if (setsockopt(device,SOL_SOCKET,SO_REUSEADDR,&value,sizeof(int))!=0)
    return false;
#endif
  return true;

  }

FXbool Socket::getReuseAddress() const {
#ifndef _WIN32
  int value = 0;
  socklen_t length=sizeof(value);
  if (getsockopt(device,SOL_SOCKET,SO_REUSEADDR,&value,&length)==0)
    return value;
  else
#endif
    return false;
  }

FXbool Socket::setLinger(FXTime tm) {
#ifndef _WIN32
	struct linger l;
  l.l_onoff  = (tm>0) ? 1 : 0;
  l.l_linger = tm;
  if (setsockopt(device,SOL_SOCKET,SO_LINGER,&l,sizeof(struct linger))!=0)
    return false;
#endif
  return true;
  }

FXTime Socket::getLinger() const {
#ifndef _WIN32
	struct linger l;
  socklen_t length=sizeof(struct linger);
  if (getsockopt(device,SOL_SOCKET,SO_LINGER,&l,&length)==0 && (l.l_onoff) ){
    return l.l_linger;
    }
#endif
  return 0;
  }

FXbool Socket::setReceiveTimeout(FXTime tm) {
#ifndef _WIN32
	struct timeval tv;
  tv.tv_sec  = tm / 1000000000;
  tv.tv_usec = (tm % 1000000000) / 1000;
  if (setsockopt(device,SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof(struct timeval))!=0)
    return false;
#endif
  return true;
  }

FXbool Socket::setSendTimeout(FXTime tm) {
#ifndef _WIN32
  struct timeval tv;
  tv.tv_sec  = tm / 1000000000;
  tv.tv_usec = (tm % 1000000000) / 1000;
  if (setsockopt(device,SOL_SOCKET,SO_SNDTIMEO,&tv,sizeof(struct timeval))!=0)
    return false;
#endif  
  return true;
  }

FXint Socket::getError() const {
#ifndef _WIN32
	int value = 0;
  socklen_t length=sizeof(value);
  if (getsockopt(device,SOL_SOCKET,SO_ERROR,&value,&length)==0)
    return value;
  else
#endif
    return -1;
  }

FXival Socket::readBlock(void* data,FXival count){
  FXival nread=-1;
#ifdef _WIN32
  nread = ::recv(sockethandle,(char*)data,count,0);
  if (nread==0 && count>0 || (nread==-1 && WSAGetLastError() != WSAEWOULDBLOCK))
	  access |= EndOfStream;
#else
  do{
    nread=::recv(device,data,count,MSG_NOSIGNAL);
    }
  while(nread<0 && errno==EINTR);

  if ((nread==0 && count>0) || (nread<0 && errno!=EWOULDBLOCK && errno!=EAGAIN))
    access|=EndOfStream;
#endif
  return nread;
  }


FXival Socket::writeBlock(const void* data,FXival count){
  FXival nwritten=-1;
#ifdef _WIN32
  nwritten = ::send(sockethandle, (char*)data, count,0);
  if (nwritten == 0 && count>0 || (nwritten == -1 && WSAGetLastError() != WSAEWOULDBLOCK))
	  access |= EndOfStream;
#else
  do{
    nwritten=::send(device,data,count,MSG_NOSIGNAL);
    }
  while(nwritten<0 && errno==EINTR);

  if ((nwritten==0 && count>0) || (nwritten<0 && errno!=EWOULDBLOCK && errno!=EAGAIN))
    access|=EndOfStream;
#endif
  return nwritten;
  }


FXbool Socket::close() {
#ifdef _WIN32
  if (isOpen()) {
	shutdown(sockethandle, SD_BOTH);
	::closesocket(sockethandle);
	WSACloseEvent(device);
	}
#else
  if (isOpen()) {
    shutdown(device,SHUT_RDWR);
    return FXIODevice::close();
    }
#endif
  return true;
  }


FXint Socket::eof() {
  return (access&EndOfStream) ? 1 : 0;
  }

}
