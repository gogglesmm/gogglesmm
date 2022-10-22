/********************************************************************************
*                                                                               *
*                            P a r s e - B u f f e r                            *
*                                                                               *
*********************************************************************************
* Copyright (C) 2013,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
*********************************************************************************
* This library is free software; you can redistribute it and/or modify          *
* it under the terms of the GNU Lesser General Public License as published by   *
* the Free Software Foundation; either version 3 of the License, or             *
* (at your option) any later version.                                           *
*                                                                               *
* This library is distributed in the hope that it will be useful,               *
* but WITHOUT ANY WARRANTY; without even the implied warranty of                *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                 *
* GNU Lesser General Public License for more details.                           *
*                                                                               *
* You should have received a copy of the GNU Lesser General Public License      *
* along with this program.  If not, see <http://www.gnu.org/licenses/>          *
********************************************************************************/
#ifndef FXPARSEBUFFER_H
#define FXPARSEBUFFER_H

namespace FX {


/**
* FXParseBuffer manages pointers to a buffer for various file format
* parsers.  It is intended to be subclassed for the particular syntax.
* Additional subclasses are expected to override fill() and flush() to
* read or write to different destinations.  The default implementation
* works on in-memory buffer containing the entire dataset.
*/
class FXAPI FXParseBuffer {
public:
  enum Direction {
    Stop = 0,           /// Not active
    Save = 1,           /// Save to device
    Load = 2            /// Load from device
    };
protected:
  FXchar    *begptr;    // Begin of buffer
  FXchar    *endptr;    // End of buffer
  FXchar    *wptr;      // Write pointer
  FXchar    *rptr;      // Read pointer
  FXchar    *sptr;      // Scan pointer
  Direction  dir;       // Direction
protected:
  FXbool need(FXival count);
  FXbool emit(FXchar ch,FXint count);
  FXbool emit(const FXchar* str,FXint count);
private:
  FXParseBuffer(const FXParseBuffer&);
  FXParseBuffer &operator=(const FXParseBuffer&);
public:

  /**
  * Initialize parse buffer to empty.
  */
  FXParseBuffer();

  /**
  * Initialize parse buffer with given size and direction.
  * Read pointer, scan pointer, and write pointer are set to
  * beginning of buffer, unless direction is Load.  When direction
  * is Load, the write pointer is set to the end of the buffer.
  */
  FXParseBuffer(FXchar* buffer,FXuval sz=4096,Direction d=Load);

  /// Open parse buffer with given size and direction
  FXbool open(FXchar* buffer=nullptr,FXuval sz=4096,Direction d=Load);

  /// Return current direction
  Direction direction() const { return dir; }

  /// Return current buffer size
  FXuval size() const { return endptr-begptr; }

  /// Read at least count bytes into buffer; return bytes available, or -1 for error
  virtual FXival fill(FXival count);

  /// Write at least count bytes from buffer; return space available, or -1 for error
  virtual FXival flush(FXival count);

  /// Close parse buffer
  FXbool close();

  /// Clean up and close buffer
  virtual ~FXParseBuffer();
  };

}

#endif
