/*******************************************************************************
*                         Goggles Audio Player Library                         *
********************************************************************************
*           Copyright (C) 2010-2018 by Sander Jansen. All Rights Reserved      *
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
#ifndef DECODER_H
#define DECODER_H

#include "ap_thread.h"
#include "ap_packet.h"
#include "ap_decoder_plugin.h"

namespace ap {

class AudioEngine;
class DecoderPlugin;
class Packet;
class ConfigureEvent;

class DecoderThread : public EngineThread, public DecoderContext {
protected:
  PacketPool      packetpool;
  DecoderPlugin * plugin = nullptr;
protected:
  FXuint stream = 0;
protected:
  void configure(ConfigureEvent*);

public: // DecoderContext Interface

  Packet * get_output_packet() override;

  Packet * get_input_packet() override;

  void post_output_packet(Packet *&,FXbool eos) override;

  void post_configuration(ConfigureEvent*) override;

public:
  DecoderThread(AudioEngine*);

  FXint run() override;

  FXbool init() override;

  void free() override;

  virtual ~DecoderThread();
  };

}
#endif


