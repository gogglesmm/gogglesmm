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
#ifdef HAVE_OSS_PLUGIN
#ifndef OSS_PLUGIN_H
#define OSS_PLUGIN_H

#include <soundcard.h>

namespace ap {

class OSSOutput : public OutputPlugin {
protected:
  FXInputHandle handle;
protected:
  OSSConfig config;
protected:
  FXbool open();
public:
  OSSOutput(OutputThread * output);

  /// Configure
  FXbool configure(const AudioFormat &) override;

  /// Write frames to playback buffer
  FXbool write(const void*, FXuint) override;

  /// Return delay in no. of frames
  FXint delay() override;

  /// Empty Playback Buffer Immediately
  void drop() override;

  /// Wait until playback buffer is emtpy.
  void drain() override;

  /// Pause Playback
  void pause(FXbool t) override;

  /// Close Output
  void close() override;

  /// Get Device Type
  FXchar type() const override { return DeviceOSS; }

  /// Set Device Configuration
  FXbool setOutputConfig(const OutputConfig &) override;

  /// Destructor
  virtual ~OSSOutput();
  };

}
#endif
#endif
