/*******************************************************************************
*                         Goggles Audio Player Library                         *
********************************************************************************
*           Copyright (C) 2010-2018 by Sander Jansen. All Rights Reserved      *
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
#ifndef AP_CROSSFADER_H
#define AP_CROSSFADER_H

#include "ap_buffer.h"
#include "ap_format.h"

namespace ap {

class CrossFader {
public:
  AudioFormat  af;
  MemoryBuffer buffer;
  FXlong position=-1;
  FXlong length=0;
  FXuint stream=0;
  FXint  nframes=0;   // number of frames stored in buffer
  FXint  rframes=0;   // number of readable frames
  FXuint duration;    // duration in milliseconds
  FXbool recording=true;
public:
  CrossFader(FXuint d=5000) : duration(d) {}

  void start_recording(AudioFormat & fmt, FXuint stream, FXlong stream_position, FXlong stream_length);

  void flush();

  FXlong start_offset() const;

  FXint readable_frames() const;

  FXint total_frames() const;

  FXlong min_stream_length() const;

  void writeFrames(const FXuchar * data, FXint n);

  void readFrames(FXint n);

  FXbool convert(const AudioFormat & target);
};

}
#endif
