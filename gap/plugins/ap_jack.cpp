/*******************************************************************************
*                         Goggles Audio Player Library                         *
********************************************************************************
*           Copyright (C) 2010-2021 by Sander Jansen. All Rights Reserved      *
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
#include "ap_output_plugin.h"

#include <jack/jack.h>

using namespace ap;

namespace ap {

class JackOutput : public OutputPlugin {
protected:
  jack_client_t * jack;
protected:
  FXbool open();
public:
  JackOutput(OutputContext* ctx);

  /// Configure
  FXbool configure(const AudioFormat &);

  /// Write frames to playback buffer
  FXbool write(const void*, FXuint);

  /// Return delay in no. of frames
  FXint delay();

  /// Empty Playback Buffer Immediately
  void drop();

  /// Wait until playback buffer is emtpy.
  void drain();

  /// Pause
  void pause(FXbool);

  /// Change Volume
  void volume(FXfloat);

  /// Close Output
  void close();

  /// Get Device Type
  FXchar type() const { return DeviceJack; }

  /// Destructor
  virtual ~JackOutput();
  };


JackOutput::JackOutput(OutputContext * ctx) : OutputPlugin(ctx) {
  }

JackOutput::~JackOutput() {
  close();
  }

FXbool JackOutput::open() {
  jack = jack_client_open("gap",JackNoStartServer,nullptr);
  if (jack==nullptr) {
    return false;
    }
  return false;
  }

void JackOutput::close() {
  if (jack) {
    jack_client_close(jack);
    jack=nullptr;
    }
  af.reset();
  }


void JackOutput::volume(FXfloat) {
  }

FXint JackOutput::delay() {
  return 0;
  }

void JackOutput::drop() {
  }

void JackOutput::drain() {
  }

void JackOutput::pause(FXbool) {
  }

FXbool JackOutput::configure(const AudioFormat &){
//  af.format = AP_FORMAT_FLOAT;
//  af.rate   = jack_get_sample_rate(jack);
  return false;
  }


FXbool JackOutput::write(const void *,FXuint){
  return false;
  }
}


AP_IMPLEMENT_PLUGIN(JackOutput);
