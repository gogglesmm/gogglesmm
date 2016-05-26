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
#include "ap_pipe.h"
#include "ap_format.h"
#include "ap_device.h"
#include "ap_event.h"
#include "ap_event_private.h"
#include "ap_event_queue.h"
#include "ap_thread_queue.h"
#include "ap_buffer.h"
#include "ap_packet.h"
#include "ap_engine.h"
#include "ap_reader_plugin.h"
#include "ap_decoder_plugin.h"
#include "ap_thread.h"
#include "ap_input_thread.h"
#include "ap_decoder_thread.h"
#include "ap_output_thread.h"

namespace ap {


class WavPackReader : public ReaderPlugin {
public:
  WavPackReader(AudioEngine*);

  /// Init plugin
  FXbool init(InputPlugin*) override;

  /// Format type
  FXuchar format() const override { return Format::WavPack; }

  /// Process Input
  ReadStatus process(Packet*) override;

  /// Destructor
  virtual ~WavPackReader();
  };


WavPackReader::WavPackReader(AudioEngine*e) : ReaderPlugin(e) {
  }

FXbool WavPackReader::init(InputPlugin*plugin) {
  ReaderPlugin::init(plugin);
  return true;
  }

ReadStatus WavPackReader::process(Packet*) {
  return ReadError;
  }

WavPackReader::~WavPackReader() {
  }

ReaderPlugin * ap_wavpack_reader(AudioEngine * engine) {
  return new WavPackReader(engine);
  }

}

