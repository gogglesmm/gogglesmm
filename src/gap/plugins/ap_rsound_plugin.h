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
#ifdef HAVE_RSOUND_PLUGIN
#ifndef RSOUND_PLUGIN_H
#define RSOUND_PLUGIN_H

#include <rsound.h>

namespace ap {

class RSoundOutput : public OutputPlugin {
protected:
  rsound_t * rsd;
protected:
  FXbool open();
public:
  RSoundOutput(OutputThread * output);

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
  FXchar type() const { return DeviceRSound; }

  /// Destructor
  virtual ~RSoundOutput();
  };

}
#endif
#endif
