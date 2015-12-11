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
#ifndef OUTPUT_PLUGIN_H
#define OUTPUT_PLUGIN_H

#include "ap_event_private.h"
#include "ap_format.h"

namespace ap {

class OutputThread;

class GMAPI OutputPlugin {
public:
  OutputThread* output;
  AudioFormat   af;
private:
  OutputPlugin(){}
public:
  /// Constructor
  OutputPlugin(OutputThread * o) : output(o) {}

  /// Output Plugin Type
  virtual FXchar type() const=0;

  /// Set Device Configuration
  virtual FXbool setOutputConfig(const OutputConfig &) { return false; }

  /// Configure the device for the requested format. If requested
  /// format is not available it should return something similar. Only
  /// return false if no format is available at all.
  virtual FXbool configure(const AudioFormat &)=0;

  /// Write frames to playback buffer
  virtual FXbool write(const void*, FXuint)=0;

  /// Return delay in no. of frames
  virtual FXint delay() { return 0; }

  /// Empty Playback Buffer Immediately
  virtual void drop()=0;

  /// Wait until playback buffer is emtpy.
  virtual void drain()=0;

  virtual void start() {}

  /// Pause Playback
  virtual void pause(FXbool t)=0;

  /// Change Volume
  virtual void volume(FXfloat) {}

  /// Get Volume
  virtual FXfloat volume() {return 1.0f;}

  /// Close Output
  virtual void close() {}

  /// Destructor
  virtual ~OutputPlugin() {}
  };

}
#endif
