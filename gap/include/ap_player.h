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
#ifndef AP_PLAYER_H
#define AP_PLAYER_H

#include "ap_event.h"
#include "ap_device.h"

namespace ap {

class AudioEngine;
class EventQueue;
class Event;

class GMAPI AudioPlayer : public FXObject {
FXDECLARE(AudioPlayer)
private:
  AudioEngine * engine;
private:
  AudioPlayer(const AudioPlayer&);
  AudioPlayer& operator=(const AudioPlayer&);
protected:
  /// Set the event queue.
  void setEventQueue(EventQueue*);
public:
  AudioPlayer();

  /// Init
  FXbool init();

  /// Shutdown
  void exit();

  /// Open url and flush existing stream if true
  void open(const FXString & url,FXbool flush=true);

  /// Pause Stream
  void pause();

  /// Close Stream
  void close();

  /// Seek to Position
  void seek(FXdouble pos);

  /// Change Volume
  void volume(FXfloat v);

  /// Get the output configuration
  void getOutputConfig(OutputConfig & config) const;

  /// Set the output configuration
  void setOutputConfig(const OutputConfig & config);

  /// Set Replay Gain Mode
  void setReplayGain(ReplayGainMode mode);

  /// Set Cross Fade Mode
  void setCrossFade(FXuint seconds);

  /// Get Replay Gain Mode
  ReplayGainMode getReplayGain() const;

  /// Get Cross Fade Mode
  FXuint getCrossFade() const;

  Event * pop();

  ~AudioPlayer();
  };

}
#endif
