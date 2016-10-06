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
#include "ap_thread_queue.h"
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

using namespace ap;

namespace ap {




FXbool Socket::setReceiveTimeout(FXTime time) {
#ifdef _WIN32
  FXuint value = time / 1000000;
  if (setsockopt(sockethandle, SOL_SOCKET, SO_RCVTIMEO, (char*)&value, sizeof(FXuint))!=0)
    return false;
#else
  struct timeval tv;
  tv.tv_sec  = time / 1000000000;
  tv.tv_usec = (time % 1000000000) / 1000;
  if (setsockopt(device, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(struct timeval))!=0)
    return false;
#endif
  return true;
  }


FXbool Socket::setSendTimeout(FXTime time) {
#ifdef _WIN32
  FXuint value = time / 1000000;
  if (setsockopt(sockethandle, SOL_SOCKET, SO_SNDTIMEO, (char*)&value, sizeof(FXuint))!=0)
    return false;
#else
  struct timeval tv;
  tv.tv_sec  = time / 1000000000;
  tv.tv_usec = (time % 1000000000) / 1000;
  if (setsockopt(device,SOL_SOCKET,SO_SNDTIMEO,&tv,sizeof(struct timeval))!=0)
    return false;
#endif
  return true;
  }


FXint Socket::getError() const {
#ifdef _WIN32
  WSANETWORKEVENTS events;
  if (WSAEnumNetworkEvents(sockethandle, device, &events) == 0) {
    if (events.lNetworkEvents&FD_CONNECT)
      return events.iErrorCode[FD_CONNECT_BIT];
    else if (events.lNetworkEvents&FD_WRITE)
      return events.iErrorCode[FD_WRITE_BIT];
    else if (events.lNetworkEvents&FD_READ)
      return events.iErrorCode[FD_READ_BIT];
    else
      return 0;
    }
  return -1;
#else
  int value = 0;
  socklen_t length=sizeof(value);
  if (getsockopt(device,SOL_SOCKET,SO_ERROR,&value,&length)==0)
    return value;
  else
    return -1;
#endif
  }



#ifdef _WIN32
// Return true if open
FXbool Socket::isOpen() const {
  return sockethandle!=0;
  }
#endif


FXbool Socket::close() {
#ifdef _WIN32
  if (isOpen()) {
    closesocket(sockethandle);
    sockethandle=0;
    return FXIODevice::close();
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




FXbool Socket::create(FXint domain,FXint type,FXint protocol,FXuint mode) {
#ifdef _WIN32
  sockethandle = socket(domain,type,protocol);
  if (sockethandle==INVALID_SOCKET)
    return false;

  if (mode&FXIO::NonBlocking) {
    u_long blocking = 1;
    if (ioctlsocket(sockethandle, FIONBIO, &blocking)!=0){
      closesocket(sockethandle);
      sockethandle=INVALID_SOCKET;
      return false;
      }
    access|=FXIO::NonBlocking;
    }

  device = WSACreateEvent();
  if (device==BadHandle) {
    closesocket(sockethandle);
    return false;
    }
#else
  int options = 0;

#ifdef SOCK_CLOEXEC
  options|=SOCK_CLOEXEC;
#endif

#ifdef SOCK_NONBLOCK
  if (mode&FXIO::NonBlocking){
    access|=FXIO::NonBlocking;
    options|=SOCK_NONBLOCK;
    }
#endif
  device = socket(domain,type|options,protocol);
  if (device==BadHandle)
    return false;

#ifndef SOCK_CLOEXEC
  if (!ap_set_closeonexec(device)){
    ::close(device);
    device=BadHandle;
    return false;
    }
#endif

#ifndef SOCK_NONBLOCK
  if (mode&FXIO::NonBlocking){
    access|=FXIO::NonBlocking;
    if (!ap_set_nonblocking(device)){
      ::close(device);
      device=BadHandle;
      return false;
      }
    }
#endif
#endif
  access|=OwnHandle;
  return true;
  }






// Connect to address
FXint Socket::connect(const struct sockaddr * address,FXint address_length) {
#ifdef _WIN32

  if (::connect(sockethandle,address,address_length)==0) {
    access|=ReadWrite;
    pointer=0;
    return 0;
    }

  if (WSAGetLastError()==WSAEWOULDBLOCK) {
    access|=ReadWrite;
    pointer=0;
    return FXIO::Again;
    }

#else
  if (::connect(device,address,address_length)==0) {
    access|=ReadWrite;
    return 0;
    }
  switch(errno) {
    case EINPROGRESS:
    case EINTR      :
    case EWOULDBLOCK: access|=ReadWrite;
                      pointer=0;
                      return FXIO::Again;
                      break;
    default         : break;
    }
#endif
  return FXIO::Error;
  }


FXival Socket::writeBlock(const void* ptr,FXival count){
  if(__likely(device!=BadHandle) && __likely(access&WriteOnly)){
    FXival nwrote;
#ifdef _WIN32
    nwrote = ::send(sockethandle,static_cast<const char*>(ptr), count,0);
    if (__unlikely(nwrote == SOCKET_ERROR)) {
      if (WSAGetLastError() == WSAEWOULDBLOCK) {
        return FXIO::Again;
        }
      access |= EndOfStream;
      return FXIO::Error;
      }
#else
    
x:  nwrote=::send(device,ptr,count,MSG_NOSIGNAL);
    if(__unlikely(nwrote<0)){
      if(errno==EINTR) goto x;
      if(errno==EAGAIN) return FXIO::Again;
      if(errno==EWOULDBLOCK) return FXIO::Again;
      access|=EndOfStream;
      return FXIO::Error;
      }
#endif
    if (nwrote==0 && count>0)
      access|=EndOfStream;

    return nwrote;
    }
  return FXIO::Error;
  }


FXival Socket::readBlock(void* ptr,FXival count){
  if(__likely(device!=BadHandle) && __likely(access&ReadOnly)){
    FXival nread;
#ifdef _WIN32
    nread = ::recv(sockethandle,static_cast<char*>(ptr), count,0);
    if (__unlikely(nread == SOCKET_ERROR)) {
      if (WSAGetLastError() == WSAEWOULDBLOCK) {
        return FXIO::Again;
        }
      access |= EndOfStream;
      return FXIO::Error;
      }
#else
a:  nread=::recv(device,ptr,count,MSG_NOSIGNAL);
    if(__unlikely(nread<0)){
      if(errno==EINTR) goto a;
      if(errno==EAGAIN) return FXIO::Again;
      if(errno==EWOULDBLOCK) return FXIO::Again;
      access|=EndOfStream;
      return FXIO::Error;
      }
    pointer+=nread;
#endif
    if (nread==0 && count>0)
      access|=EndOfStream;

    return nread;
    }
  return FXIO::Error;
  }



ThreadSocket::ThreadSocket(ThreadQueue * q) : fifo(q) {
  }

WaitEvent ThreadSocket::wait(WaitMode mode) {
#ifdef _WIN32
  if (mode==WaitMode::Read)
    WSAEventSelect(sockethandle,device,FD_READ);
  else if (mode==WaitMode::Write)
    WSAEventSelect(sockethandle,device,FD_WRITE);
  else if (mode==WaitMode::Connect)
    WSAEventSelect(sockethandle,device,FD_CONNECT);
#endif
  return fifo->signal().wait(device,mode);
  }


FXival ThreadSocket::readBlock(void*ptr,FXival count) {
  FXival nread;
x:nread = Socket::readBlock(ptr,count);
  if (nread==FXIO::Again) {
    switch(wait(WaitMode::Read)) {

      case WaitEvent::Signal:
        if (fifo->checkAbort())
          return FXIO::Error;
        goto x;
        break;

      case WaitEvent::Input:
        goto x;
        break;

      default:
        return FXIO::Error;
        break;
      }
    }
  return nread;
  }

FXival ThreadSocket::writeBlock(const void*ptr,FXival count) {
  FXival nwrote;
x:nwrote = Socket::writeBlock(ptr,count);
  if (nwrote==FXIO::Again) {
    switch(wait(WaitMode::Write)) {

      case WaitEvent::Signal:
        if (fifo->checkAbort())
          return FXIO::Error;
        goto x;
        break;

      case WaitEvent::Input:
        goto x;
        break;

      default:
        return FXIO::Error;
        break;
      }
    }
  return nwrote;
  }


FXint ThreadSocket::connect(const struct sockaddr * address,FXint address_length) {
  FXint x = Socket::connect(address,address_length);
  if (x==FXIO::Again) {
a:  switch(wait(WaitMode::Connect)) {

      case WaitEvent::Signal:
        {
          if (fifo->checkAbort()) return ThreadSocket::Signalled;
          goto a;
        } break;

      case WaitEvent::Input:
        {
          if (getError()==0)
            return 0;
        }
      default: break;
      }
    return FXIO::Error;
    }
  return x;
  }

}

