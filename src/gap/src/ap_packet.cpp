#include "ap_defs.h"
#include "ap_utils.h"
#include "ap_pipe.h"
#include "ap_format.h"
#include "ap_event.h"
#include "ap_event_private.h"
#include "ap_memory_buffer.h"
#include "ap_packet.h"


PacketPool::PacketPool() : list(NULL) {
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
  while(list) {
    Packet * p = dynamic_cast<Packet*>(list);
    list=list->next;
    delete p;
    }
  ppool.close();
  }

PacketPool::~PacketPool() {
  PacketPool::free();
  }


void PacketPool::fetchEvents() {
  Event * event = NULL;
  while((event=ppool.pop())!=NULL){
    event->next = list;
    list = event;
    }
  }

Packet * PacketPool::pop() {
  Event * event = NULL;
  if (!list)
    fetchEvents();

  if (list) {
    event = list;
    list = event->next;
    event->next = NULL;
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

void Packet::clear() {
  MemoryBuffer::clear();
  flags=0;
  stream=0;
  stream_position=0;
  stream_length=0;
  }

void Packet::unref() {
  clear();
  pool->push(this);
  }

