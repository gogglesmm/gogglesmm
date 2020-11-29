/*******************************************************************************
*                         Goggles Audio Player Library                         *
********************************************************************************
*           Copyright (C) 2020-2021 by Sander Jansen. All Rights Reserved      *
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
#include "ap_convert.h"
#include "ap_crossfader.h"

namespace ap {

// Start recording samples at stream position
void CrossFader::start_recording(AudioFormat & fmt, FXuint stream, FXlong stream_position, FXlong stream_length) {
  GM_DEBUG_PRINT("[crossfader] start recording at %lld for %lld frames (%g secs)\n", stream_position, stream_length - stream_position,  (stream_length - stream_position) / (double)fmt.rate);
  position = stream_position;
  length = stream_length;
  af = fmt;
  nframes = 0;
  rframes = 0;
  }


void CrossFader::flush(){
  GM_DEBUG_PRINT("[crossfader] flush\n");
  buffer.clear();
  length = 0;
  position = -1;
  nframes = 0;
  rframes = 0;
  recording = true;
  }


FXlong CrossFader::start_offset() const {
  return nframes;
  }

FXint CrossFader::readable_frames() const {
  return rframes;
  }

FXint CrossFader::total_frames() const {
  return nframes;
  }

FXlong CrossFader::min_stream_length() const {
  return (duration << 2) * af.rate;
  }


void CrossFader::writeFrames(const FXuchar * data, FXint n) {
  buffer.append(data, n * af.framesize());
  nframes += n;
  rframes += n;
  }

void CrossFader::readFrames(FXint n) {
  buffer.readBytes(n * af.framesize());
  rframes -= n;
  if (rframes == 0)
    flush();
  }

FXbool CrossFader::convert(const AudioFormat & target) {
  GM_DEBUG_PRINT("[crossfade] converting samples\n");
#ifdef DEBUG
  printf("[crossfade] source ");
  af.debug();
  printf("[crossfade] target ");
  target.debug();
#endif

  if (af.channels != target.channels) {
    GM_DEBUG_PRINT("[crossfade] channel mismatch\n");
    return false;
    }

  if (af.channelmap != target.channelmap) {
    GM_DEBUG_PRINT("[crossfade] channelmap mismatch\n");
    return false;
    }

  if (af.rate != target.rate) {
    GM_DEBUG_PRINT("[crossfade] rate mismatch\n");
    return false;
    }

  if (target.format == AP_FORMAT_S16) {
    switch(af.format) {
      case AP_FORMAT_FLOAT: float_to_s16(buffer.data(), buffer.size() / af.packing()); break;
      case AP_FORMAT_S24_3: s24le3_to_s16(buffer.data(), buffer.size() / af.packing()); break;
      default             :  GM_DEBUG_PRINT("[crossfade] conversion not implemented\n"); return false; break;
      }
    af.format = target.format;
    return true;
    }
  else if (target.format == AP_FORMAT_FLOAT) {
    switch(af.format) {
      case AP_FORMAT_S16:
        {
          MemoryBuffer out;
          s16_to_float(buffer.data(), buffer.size() / af.packing(), out);
          buffer.adopt(out);
          break;
        }
      case AP_FORMAT_S24_3:
        {
          MemoryBuffer out;
          s24le3_to_float(buffer.data(), buffer.size() / af.packing(), out);
          buffer.adopt(out);
          break;
        }
      default             : return false; break;
      }
    af.format = target.format;
    return true;
    }

  }

}

