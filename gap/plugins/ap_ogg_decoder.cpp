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
#include "ap_packet.h"
#include "ap_ogg_decoder.h"

namespace ap {

OggDecoder::OggDecoder(AudioEngine*e) : DecoderPlugin(e),
  buffer(32768),
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

FXbool OggDecoder::get_next_packet() {
  if (buffer.size() && buffer.size()>=(FXival)sizeof(ogg_packet)) {
    buffer.read((FXuchar*)&op,sizeof(ogg_packet));
    if (buffer.size()<op.bytes) {
      buffer.readBytes(-sizeof(ogg_packet));
      return false;
      }
    op.packet=(FXuchar*)buffer.data();
    buffer.readBytes(op.bytes);
    return true;
    }
  return false;
  }

const FXuchar* OggDecoder::get_packet_offset() {
  return buffer.data();
  }

void OggDecoder::set_packet_offset(const FXuchar * offset) {
  buffer.setReadPosition(offset);
  }

void OggDecoder::push_back_packet() {
  if (packet_start_ptr) {
    buffer.setReadPosition(packet_start_ptr);
    packet_start_ptr=nullptr;
    }
  }

DecoderStatus OggDecoder::process(Packet* packet){
  buffer.append(packet->data(),packet->size());
  packet->unref();
  return DecoderOk;
  }



}
