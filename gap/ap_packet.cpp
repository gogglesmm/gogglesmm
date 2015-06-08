/*******************************************************************************
*                         Goggles Audio Player Library                         *
********************************************************************************
*           Copyright (C) 2010-2015 by Sander Jansen. All Rights Reserved      *
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
#include "ap_pipe.h"
#include "ap_format.h"
#include "ap_device.h"
#include "ap_event.h"
#include "ap_event_private.h"
#include "ap_buffer.h"
#include "ap_packet.h"

namespace ap {

PacketPool::PacketPool() : list(nullptr) {
  }

FXbool PacketPool::init(FXival sz,FXival n) {
  for (FXint i=0;i<n;i++) {
    Packet * packet = new Packet(this,sz);
    packet->next = list;
    list = packet;
    }
  return ppool.create();
  }

void PacketPool::free() {
  fetchEvents();
  while(list) {
    Packet * p = dynamic_cast<Packet*>(list);
    list=list->next;
    delete p;
    }
  ppool.close();
  }

PacketPool::~PacketPool() {
  }


void PacketPool::fetchEvents() {
  Event * event = nullptr;
  while((event=ppool.pop())!=nullptr){
    event->next = list;
    list = event;
    }
  }

Packet * PacketPool::pop() {
  Event * event = nullptr;
  if (!list)
    fetchEvents();

  if (list) {
    event = list;
    list = event->next;
    event->next = nullptr;
    }
  return dynamic_cast<Packet*>(event);
  }

void PacketPool::push(Packet * packet) {
  ppool.push(packet);
  }

FXInputHandle PacketPool::handle() const {
  return ppool.handle();
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
