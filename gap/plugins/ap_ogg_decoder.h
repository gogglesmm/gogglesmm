/*******************************************************************************
*                         Goggles Audio Player Library                         *
********************************************************************************
*           Copyright (C) 2013-2016 by Sander Jansen. All Rights Reserved      *
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
#ifndef OGG_DECODER_PLUGIN_H
#define OGG_DECODER_PLUGIN_H

#include "ap_buffer.h"
#include "ap_decoder_plugin.h"
#include <ogg/ogg.h>


namespace ap {

class Packet;

class OggDecoder : public DecoderPlugin {
private:
  MemoryBuffer  buffer;
  FXuchar*      packet_start_ptr;
protected:
  ogg_packet op = {};
  Packet*    out;
  FXlong     stream_position;
protected:
  FXbool get_next_packet();
  void   push_back_packet();
  const FXuchar* get_packet_offset();
  void set_packet_offset(const FXuchar*);
public:
  OggDecoder(AudioEngine*);

  FXbool init(ConfigureEvent*) override;

  FXbool flush(FXlong) override;

  DecoderStatus process(Packet*) override;

  ~OggDecoder();
  };

}
#endif


