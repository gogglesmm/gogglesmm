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
#ifndef DECODER_PLUGIN_H
#define DECODER_PLUGIN_H

namespace ap {

class ConfigureEvent;
class DecoderPacket;

enum DecoderStatus {
  DecoderError,
  DecoderOk,
  DecoderDone,
  DecoderInterrupted
  };

class DecoderPlugin {
protected:
  AudioEngine * engine;
  AudioFormat   af;
public:
public:
  DecoderPlugin(AudioEngine*);

  virtual FXuchar codec() const { return Codec::Invalid; }

  virtual FXbool init(ConfigureEvent*)=0;

  virtual DecoderStatus process(Packet*)=0;

  virtual FXbool flush()=0;

  static DecoderPlugin* open(AudioEngine * engine,FXuchar codec);

  virtual ~DecoderPlugin() {}
  };

}
#endif

