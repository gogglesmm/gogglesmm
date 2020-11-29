/*******************************************************************************
*                         Goggles Audio Player Library                         *
********************************************************************************
*           Copyright (C) 2010-2021 by Sander Jansen. All Rights Reserved      *
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
#ifndef AP_SOCKET_H
#define AP_SOCKET_H

#include "ap_signal.h"

struct sockaddr;

#if defined(HAVE_OPENSSL)
typedef struct ssl_st SSL;
#elif defined(HAVE_GNUTLS)
struct gnutls_session_int;
typedef struct gnutls_session_int *gnutls_session_t;
#endif

namespace ap {


class Socket : public FXIODevice {
#ifdef _WIN32
  FXuint sockethandle = 0;
#endif
private:
  Socket(const Socket&);
  Socket &operator=(const Socket&);
public:
  enum {
    EndOfStream = 2048
    };
public:
  Socket(){}

protected:
  // Wait for specificied event
  virtual WaitEvent wait(WaitMode);
public:
  // Set Receive Timeout
  FXbool setReceiveTimeout(FXTime);

  // Set Send Timeout
  FXbool setSendTimeout(FXTime);

  // Get Pending Error
  FXint getError() const;

public:

#ifdef _WIN32
  FXbool isOpen() const;
#endif

  // Handle eof
  FXint eof() override;

  // Close Socket
  FXbool close() override;

  virtual FXbool shutdown();

  // Read block of bytes, returning number of bytes read
  FXival readBlock(void* data,FXival count) override;

  // Write block of bytes, returning number of bytes written
  FXival writeBlock(const void* data,FXival count) override;

  // Create specific socket type
  virtual FXbool create(FXint domain,FXint type,FXint protocol,FXuint mode);

  // Connect to address
  virtual FXint connect(const struct sockaddr *,FXint sockaddr_length);

  };

#if defined(HAVE_OPENSSL) || defined(HAVE_GNUTLS)

class SecureSocket : public Socket {
protected:
#if defined(HAVE_OPENSSL)
  SSL*     ssl = nullptr;
#elif defined(HAVE_GNUTLS)
  gnutls_session_t session = nullptr;
protected:
  FXint handshake();
#endif
public:
  SecureSocket();

  FXbool shutdown() override;

  // Close SecureSocket
  FXbool close() override;

  // Read block of bytes, returning number of bytes read
  FXival readBlock(void* data,FXival count) override;

  // Write block of bytes, returning number of bytes written
  FXival writeBlock(const void* data,FXival count) override;

  // Create specific socket type
  FXbool create(FXint domain,FXint type,FXint protocol,FXuint mode) override;

  // Connect to address
  FXint connect(const struct sockaddr *,FXint sockaddr_length) override;
  };

#endif

struct IOContext;

class ThreadSocket : public Socket {
private:
  IOContext * context = nullptr;
private:
  ThreadSocket(const ThreadSocket&);
  ThreadSocket &operator=(const ThreadSocket&);
protected:
  // Wait for specified event
  WaitEvent wait(WaitMode) override;
public:
  ThreadSocket(IOContext*);
  };

#if defined(HAVE_OPENSSL) || defined(HAVE_GNUTLS)

class ThreadSecureSocket : public SecureSocket {
private:
  IOContext * context = nullptr;
private:
  ThreadSecureSocket(const ThreadSecureSocket&);
  ThreadSecureSocket &operator=(const ThreadSecureSocket&);
protected:
  // Wait for specified event
  WaitEvent wait(WaitMode) override;
public:
  ThreadSecureSocket(IOContext*);
  };

#endif

}
#endif
