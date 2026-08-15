/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2010-2026 by Sander Jansen. All Rights Reserved      *
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
*                               ---                                            *
* SPDX-License-Identifier: GPL-3.0-or-later                                    *
********************************************************************************/
#ifndef GM_AUDIO_PLAYER_H
#define GM_AUDIO_PLAYER_H

#ifndef AP_H
#include "ap.h"
#endif

#ifndef SEL_PLAYER_OFFSET
#define SEL_PLAYER_OFFSET SEL_LAST
#endif

enum {
  SEL_PLAYER_EOS = SEL_PLAYER_OFFSET,
  SEL_PLAYER_BOS,
  SEL_PLAYER_TIME,
  SEL_PLAYER_STATE,
  SEL_PLAYER_VOLUME,
  SEL_PLAYER_META,
  SEL_PLAYER_ERROR
  };

enum PlayerState {
  PLAYER_STOPPED,
  PLAYER_PLAYING,
  PLAYER_PAUSING
  };


struct PlaybackTime {
  FXuint position = 0;
  FXuint length = 0;
  };

class GMAudioPlayer : public AudioPlayer {
  FXDECLARE(GMAudioPlayer);
private:
  FXAppQueue   * fifo    = nullptr;
  FXObject     * target  = nullptr;
  FXSelector     message = 0;
protected:
  PlayerState    state = PLAYER_STOPPED;
  PlaybackTime   time;
  FXint          vvolume = -1;
protected:
  GMAudioPlayer(){}
private:
  GMAudioPlayer(const GMAudioPlayer&);
  GMAudioPlayer& operator=(const GMAudioPlayer&);
public:
  long onEngineEvents(FXObject*,FXSelector,void*);
public:
  enum {
    ID_AUDIO_ENGINE = 1
    };
public:
  GMAudioPlayer(FXApp * app,FXObject * tgt,FXSelector sel);

  void setPosition(FXuint pos);

  FXuint getPosition() const { return time.position; }

  FXint getVolume() const { return vvolume; }

  void stop() { close(); }

  /// Status
  FXbool playing() const { return state==PLAYER_PLAYING; }

  FXbool pausing() const { return state==PLAYER_PAUSING; }

  void loadSettings();

  void saveSettings();

  virtual ~GMAudioPlayer();
  };


#endif
