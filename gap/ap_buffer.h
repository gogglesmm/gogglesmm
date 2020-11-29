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
#ifndef AP_MEMORY_BUFFER_H
#define AP_MEMORY_BUFFER_H

#include "ap_buffer_base.h"

namespace ap {


class MemoryBuffer : public BufferBase {
public:
  // Constructor
  MemoryBuffer(FXival cap=4096);

  // Number of unread bytes
  FXival size() const { return (wrptr-rdptr); }

  // Number of bytes that can be written
  FXival space() const { return (endptr-begptr) - (wrptr-begptr); }

  // Size of the buffer
  FXival capacity() const { return (endptr-begptr); }

  // Read nbytes
  FXival read(void * bytes,FXival nbytes);

  // Read nbytes without advancing the read ptr.
  FXival peek(void * bytes,FXival nbytes);

  // Append bytes of nbytes.
  void append(const void * bytes,FXival nbytes);

  // Append constant nbytes.
  void append(const FXchar c, FXival nbytes=1);

  // Wrote nbytes. Updates the wrptr
  void wroteBytes(FXival nbytes);

  // Read nbytes. Update the rdptr
  void readBytes(FXival nbytes);

  /// Trim nbytes at beginning
  void trimBegin(FXival nbytes);

  /// Trim nbytes at end
  void trimEnd(FXival nbytes);

  /// Return write pointer
  FXuchar* ptr() { return (FXuchar*)wrptr; }
  const FXuchar* ptr() const { return (FXuchar*)wrptr; }

  FXfloat * flt() { return reinterpret_cast<FXfloat*>(wrptr); }
  FXchar  * s8()  { return reinterpret_cast<FXchar*>(wrptr); }
  FXshort * s16() { return reinterpret_cast<FXshort*>(wrptr); }
  FXint   * s32() { return reinterpret_cast<FXint*>(wrptr); }

  /// Return pointer to buffer
  FXuchar* data() { return rdptr; }
  const FXuchar* data() const { return rdptr; }

  void setReadPosition(const FXuchar *p) { rdptr=(FXuchar*)p; }

  // Destructor
  ~MemoryBuffer();
  };

}

#endif

