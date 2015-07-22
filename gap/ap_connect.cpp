/*******************************************************************************
*                         Goggles Audio Player Library                         *
********************************************************************************
*           Copyright (C) 2013-2015 by Sander Jansen. All Rights Reserved      *
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
#include "ap_thread_queue.h"


#ifndef WIN32
#include <unistd.h> // for close()
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h> // for getaddrinfo()
#endif

// AI_ADDRCONFIG not available on OpenBSD
#ifndef AI_ADDRCONFIG
#define AI_ADDRCONFIG 0
#endif

#include "ap_connect.h"
#include "ap_wait_io.h"
#include "ap_socket.h"


namespace ap {

ConnectionFactory::ConnectionFactory(){
  }

ConnectionFactory::~ConnectionFactory(){
  }

FXIO* ConnectionFactory::create(FXint domain,FXint type,FXint protocol){
  Socket * s = Socket::create(domain,type,protocol,0);
  if (s) {
    s->setReceiveTimeout(10000000000);
    s->setSendTimeout(10000000000);
    return s;
    }
  return nullptr;
  }

FXuint ConnectionFactory::connect(FXIO * socket,const struct sockaddr * address,FXint address_len) {
  FXIODevice * device  = dynamic_cast<FXIODevice*>(socket);
  if (::connect(device->handle(),address,address_len)==0){
    return Connected;
    }
  return Error;
  }

FXIO* ConnectionFactory::open(const FXString & hostname,FXint port) {
  struct addrinfo   hints;
  struct addrinfo * list=nullptr;
  struct addrinfo * item=nullptr;
  FXint result;

  memset(&hints,0,sizeof(struct addrinfo));
  hints.ai_family=AF_UNSPEC;
  hints.ai_socktype=SOCK_STREAM;
  hints.ai_flags|=(AI_NUMERICSERV|AI_ADDRCONFIG);

  result=getaddrinfo(hostname.text(),FXString::value(port).text(),&hints,&list);
  if (result) return nullptr;

  for (item=list;item;item=item->ai_next){
    FXIO * io = create(item->ai_family,item->ai_socktype,item->ai_protocol);
    if (io==nullptr)
      continue;

    switch(connect(io,item->ai_addr,item->ai_addrlen)){
      case Connected : freeaddrinfo(list); return io; break;
      case Error		 : delete io; break;
      default				 : delete io; freeaddrinfo(list); return nullptr; break;
      }
    }

  if (list) {
    freeaddrinfo(list);
    }
  return nullptr;
  }


NBConnectionFactory::NBConnectionFactory(FXInputHandle w) : watch(w) {
  }


FXIO * NBConnectionFactory::create(FXint domain,FXint type,FXint protocol) {
  Socket * s = Socket::create(domain,type,protocol,FXIO::NonBlocking);
  if (s) {
    s->setReceiveTimeout(10000000000);
    s->setSendTimeout(10000000000);
    return new WaitIO(s,watch,10000000000);
    }
  return nullptr;
  }


FXuint NBConnectionFactory::connect(FXIO * io,const struct sockaddr * address,FXint address_len) {
  WaitIO * wio = dynamic_cast<WaitIO*>(io);
  FXASSERT(wio);
  Socket * sio = dynamic_cast<Socket*>(wio->getDevice());
  if (::connect(sio->handle(),address,address_len)==0) {
    return Connected;
    }
  if (errno==EINPROGRESS || errno==EINTR || errno==EWOULDBLOCK) {
    if (wio->wait(WaitIO::Writable)==0) {
      if (sio->getError()==0)
        return Connected;
      }
    else {
      return Abort;
      }
    }
  return Error;
  }

ThreadConnectionFactory::ThreadConnectionFactory(ThreadQueue * q) : NBConnectionFactory(q->handle()), fifo(q){
  }


FXIO * ThreadConnectionFactory::create(FXint domain,FXint type,FXint protocol) {
  Socket * s = Socket::create(domain,type,protocol,FXIO::NonBlocking);
  if (s) {
    s->setReceiveTimeout(10000000000);
    s->setSendTimeout(10000000000);
    return new ThreadIO(s,fifo,10000000000);
    }
  return nullptr;
  }
}
