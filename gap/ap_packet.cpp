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
#include "ap_defs.h"
#include "ap_event_private.h"
#include "ap_packet.h"

namespace ap {

PacketPool::PacketPool() {}

FXbool PacketPool::init(FXival sz,FXival n) {
  if (n>64) fxerror("fixme");
  packets.setSize(64); //
  for (FXint i=0;i<n;i++) {
    packets.push(new Packet(this,sz));
    }
  return semaphore.create(n);
  }

void PacketPool::free() {
  Packet * packet = nullptr;
  while(packets.pop(packet)) delete packet;
  semaphore.close();
  }

PacketPool::~PacketPool() {
  }


void PacketPool::push(Packet * packet) {
  packets.push(packet);
  semaphore.release();
  }


Packet * PacketPool::wait(const Signal & signal) {
  if (semaphore.wait(signal)){
    Packet * packet = nullptr;
    packets.pop(packet);
    return packet;
    } 
  return nullptr;
  }




Packet::Packet(PacketPool *p,FXival sz) : Event(Buffer), MemoryBuffer(sz), pool(p),flags(0),stream_position(0),stream_length(0) {
  }

Packet::~Packet() {
  }

void Packet::reset() {
  MemoryBuffer::clear();
  flags=0;
  stream=0;
  stream_position=0;
  stream_length=0;
  }

void Packet::unref() {
  reset();
  pool->push(this);
  }

}
