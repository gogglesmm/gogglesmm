/*******************************************************************************
*                         Goggles Audio Player Library                         *
********************************************************************************
*           Copyright (C) 2013-2017 by Sander Jansen. All Rights Reserved      *
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


#ifndef _WIN32
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
#include "ap_socket.h"


namespace ap {


ConnectionFactory::ConnectionFactory(){
  }


ConnectionFactory::~ConnectionFactory(){
  }


Socket * ConnectionFactory::create(FXint domain,FXint type,FXint protocol) {
  Socket * io = nullptr;

#if defined(HAVE_OPENSSL) || defined(HAVE_GNUTLS)
  if (use_ssl)
    io = new SecureSocket();
  else
    io = new Socket();
#else
  io = new Socket();
#endif


  if (io->create(domain,type,protocol,0)==false){
    delete io;
    return nullptr;
    }
  io->setSendTimeout(10_s);
  io->setReceiveTimeout(10_s);
  return io;
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

  // Automatically enable ssl for 443
#if defined(HAVE_OPENSSL) || defined(HAVE_GNUTLS)
  use_ssl = (port==443);
  if (use_ssl) GM_DEBUG_PRINT("[connection] using SSL on port %d\n",port);
#endif

  result=getaddrinfo(hostname.text(),FXString::value(port).text(),&hints,&list);
  if (result) return nullptr;

  for (item=list;item;item=item->ai_next){
    Socket * io = create(item->ai_family,item->ai_socktype,item->ai_protocol);
    if (io==nullptr)
      continue;

    switch(io->connect((const struct sockaddr*)item->ai_addr,(FXint)item->ai_addrlen)){
      case  0: // connected
        freeaddrinfo(list);
        return io;
        break;

      case 1:  // user interrupt, give up
        delete io;
        freeaddrinfo(list);
        return nullptr;
        break;

      default: // try next
        delete io;
        break;
      }
    }
  if (list) freeaddrinfo(list);
  return nullptr;
  }

//------------------------------------------------------------------------------

ThreadConnectionFactory::ThreadConnectionFactory(ThreadQueue * q) : fifo(q) {
  }


Socket * ThreadConnectionFactory::create(FXint domain,FXint type,FXint protocol) {
  Socket * io = nullptr;

#if defined(HAVE_OPENSSL) || defined(HAVE_GNUTLS)
  if (use_ssl)
    io = new ThreadSecureSocket(fifo);
  else
    io = new ThreadSocket(fifo);
#else
  io = new ThreadSocket(fifo);
#endif

  if (io->create(domain,type,protocol,FXIO::NonBlocking)==false){
    delete io;
    return nullptr;
    }
  io->setSendTimeout(10_s);
  io->setReceiveTimeout(10_s);
  return io;
  }

}
