/*******************************************************************************
*                         Goggles Audio Player Library                         *
********************************************************************************
*           Copyright (C) 2010-2012 by Sander Jansen. All Rights Reserved      *
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
#ifdef HAVE_PULSE_PLUGIN
#ifndef PULSE_PLUGIN_H
#define PULSE_PLUGIN_H

extern "C" {
#include <pulse/pulseaudio.h>
}

namespace ap {

class PulseOutput : public OutputPlugin {
protected:
  pa_threaded_mainloop * mainloop;
  pa_context           * context;
  pa_stream            * stream;
protected:
  FXbool open();
public:
  PulseOutput();

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
  FXchar type() const { return DevicePulse; }

  /// Destructor
  virtual ~PulseOutput();
  };

}

#endif
#endif
