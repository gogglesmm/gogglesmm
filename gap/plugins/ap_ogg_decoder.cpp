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
#include "ap_ogg_decoder.h"

namespace ap {

OggDecoder::OggDecoder(AudioEngine*e) : DecoderPlugin(e),
  buffer(0),
  packet_start_ptr(nullptr),
  out(nullptr),
  stream_position(-1) {
  }

OggDecoder::~OggDecoder() {
  if (out) {
    out->unref();
    out=nullptr;
    }
  }

FXbool OggDecoder::init(ConfigureEvent*event){
  DecoderPlugin::init(event);
  buffer.clear();
  stream_position=-1;
  stream_offset_start=event->stream_offset_start;
  return true;
  }

FXbool OggDecoder::flush(FXlong offset) {
  DecoderPlugin::flush(offset);
  buffer.clear();
  stream_position=-1;
  if (out) {
    out->unref();
    out=nullptr;
    }
  return true;
  }


FXbool OggDecoder::get_next_packet(Packet * packet) {
  FXuint nbytes;

  op.packetno   = 0;
  op.granulepos = -1;
  op.b_o_s      = 0;
  op.e_o_s      = 0;

  // Partial data from previous packet(s)
  if (__unlikely(buffer.size())) {

    FXASSERT(buffer.size()>=4);
    buffer.read(&nbytes,4);
    op.bytes = nbytes;

    // data in buffer
    if (op.bytes<=buffer.size()) {
      op.packet = (FXuchar*)buffer.data();
      buffer.readBytes(op.bytes);
      return true;
      }

    // data partial in buffer and in packet
    else if (buffer.size()+packet->size()>=op.bytes) {
      nbytes = op.bytes - buffer.size();
      FXASSERT(op.bytes>buffer.size());
      buffer.append(packet->data(),nbytes);
      packet->readBytes(nbytes);
      op.packet = (FXuchar*)buffer.data();
      buffer.readBytes(op.bytes);
      return true;
      }

    // not enough data, buffer data from packet.
    else {
      buffer.readBytes(-4);
      buffer.append(packet->data(),packet->size());
      packet->unref();
      return false;
      }
    }

  if (__likely(packet->size())) {
    FXASSERT(packet->size()>=4);
    packet->read(&nbytes,4);
    op.bytes = nbytes;
    if (op.bytes>packet->size()) {
      packet->readBytes(-4);
      buffer.append(packet->data(),packet->size());
      packet->unref();
      return false;
      }
    else {
      op.packet = (FXuchar*)packet->data();
      packet->readBytes(op.bytes);
      return true;
      }
    }
  packet->unref();
  return false;
  }


DecoderStatus OggDecoder::process(Packet* packet){
  if (packet->stream_position>=0 && buffer.size()==0) {
    FXASSERT(stream_position==-1);
    stream_position=packet->stream_position;
    fxmessage("[ogg] new stream position %ld\n",packet->stream_position);
    }
  return DecoderOk;
  }



}
