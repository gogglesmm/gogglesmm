/*******************************************************************************
*                         Goggles Audio Player Library                         *
********************************************************************************
*           Copyright (C) 2010-2017 by Sander Jansen. All Rights Reserved      *
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
  FXint eof();

  // Close Socket
  FXbool close() override;

  // Read block of bytes, returning number of bytes read
  FXival readBlock(void* data,FXival count) override;

  // Write block of bytes, returning number of bytes written
  FXival writeBlock(const void* data,FXival count) override;

  // Create specific socket type
  virtual FXbool create(FXint domain,FXint type,FXint protocol,FXuint mode);

  // Connect to address
  virtual FXint connect(const struct sockaddr *,FXint sockaddr_length);
  };


class ThreadQueue;

class ThreadSocket : public Socket {
private:
  ThreadQueue * fifo = nullptr;
private:
  ThreadSocket(const ThreadSocket&);
  ThreadSocket &operator=(const ThreadSocket&);
public:
  enum {
    Signalled = 1
    };
public:
  ThreadSocket(ThreadQueue*);

  /// Read block of bytes, returning number of bytes read
  FXival readBlock(void* data,FXival count) override;

  /// Write block of bytes, returning number of bytes written
  FXival writeBlock(const void* data,FXival count) override;

  // Connect to address
  FXint connect(const struct sockaddr *,FXint sockaddr_length) override;

  // Wait for specified event
  WaitEvent wait(WaitMode);
  };

}
#endif
