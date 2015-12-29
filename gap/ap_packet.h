/*******************************************************************************
*                         Goggles Audio Player Library                         *
********************************************************************************
*           Copyright (C) 2010-2016 by Sander Jansen. All Rights Reserved      *
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
#ifndef MEM_PACKET_H
#define MEM_PACKET_H

#include "ap_buffer.h"
#include "ap_event.h"
#include "ap_pipe.h"
#include "ap_format.h"

namespace ap {

class Packet;

/*
class Stream {
  FXint  id;
  FXlong length;
  FXlong position;
  };
*/

class PacketPool {
protected:
  EventPipe ppool;
  Event*    list;
protected:
  void fetchEvents();
public:
  /// Constructor
  PacketPool();

  /// Initialize pool
  FXbool init(FXival sz,FXival n);

  /// free pool
  void free();

  /// Get packet out of pool [if any]
  Packet * pop();

  /// Put event back into pool
  void push(Packet*);

  /// Get a handle to the pool
  const EventPipe & notifier() const { return ppool; }

  /// Wait for next packet or return nullptr if notify was signalled.
  Packet * wait(const NotifyPipe & notify);

  /// Destructor
  ~PacketPool();
  };


class Packet : public Event, public MemoryBuffer {
  friend class PacketPool;
protected:
  PacketPool*   pool;
public:
  AudioFormat   af;
  FXuchar       flags;
  FXlong        stream_position;
  FXlong        stream_length;
protected:
  Packet(PacketPool*,FXival sz);
  virtual ~Packet();
public:
  virtual void unref();

  void reset();

  FXbool full() const { return (af.framesize() > space()); }

  FXint numFrames() const { return size() / af.framesize(); }

  FXint availableFrames() const { return space() / af.framesize(); }

  void wroteFrames(FXint nframes) { wroteBytes(nframes*af.framesize()); }

  void appendFrames(const FXuchar * buf,FXival nframes) { append(buf,af.framesize()*nframes); }
  };

}

#endif
