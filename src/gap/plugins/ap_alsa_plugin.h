/*******************************************************************************
*                         Goggles Audio Player Library                         *
********************************************************************************
*           Copyright (C) 2010-2014 by Sander Jansen. All Rights Reserved      *
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
#ifdef HAVE_ALSA_PLUGIN
#ifndef ALSA_PLUGIN_H
#define ALSA_PLUGIN_H

#include <alsa/asoundlib.h>


namespace ap {

class AlsaMixer;

class AlsaOutput : public OutputPlugin {
protected:
  snd_pcm_t*        handle;
  snd_pcm_uframes_t period_size;
  snd_pcm_uframes_t period_written;
  FXuchar*          silence;
  

  AlsaMixer * mixer;
protected:
  AlsaConfig config;
  FXbool   can_pause;
  FXbool   can_resume;  
protected:
  FXbool open();
public:
  AlsaOutput(OutputThread*);

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

  /// Pause Playback
  void pause(FXbool t);

  /// Change Volume
  void volume(FXfloat);

  /// Close Output
  void close();

  /// Get Device Type
  FXchar type() const { return DeviceAlsa; }

  /// Set Device Configuration
  FXbool setOutputConfig(const OutputConfig &);

  /// Destructor
  virtual ~AlsaOutput();
  };

}
#endif
#endif
