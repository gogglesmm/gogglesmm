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
#ifndef AP_BUFFER_IO_H
#define AP_BUFFER_IO_H

#include "ap_buffer_base.h"

namespace ap {

class GMAPI BufferIO : public FXIO, public BufferBase  {
protected:
  FXIO    * io;     // IO
  FXuchar   dir;    // buffer direction
protected:
  enum { // Buffer Direction
    DirNone  = 0,
    DirWrite = 1,
    DirRead  = 2,
    };
protected:
  FXuval writeBuffer();
  FXuval readBuffer();
  FXbool flushBuffer();
private:
  BufferIO(const BufferIO&);
  BufferIO &operator=(const BufferIO&);
public:
  BufferIO(FXuval size=8192UL);
  BufferIO(FXIO * io,FXuval size=8192UL);

  /// Attach an IO
  void attach(FXIO * io);

  // Return attached io
  FXIO * attached() const;

  /// Return true if open
  FXbool isOpen() const override;

  /// Return true if serial access only
  FXbool isSerial() const override;

  /// Get current file position
  FXlong position() const override;

  /// Change file position, returning new position from start
  FXlong position(FXlong offset,FXuint from=FXIO::Begin) override;

  /// Peek block of bytes, return number of bytes peeked
  virtual FXival peekBlock(void* data,FXival count);

  /// Read block of bytes, returning number of bytes read
  FXival readBlock(void* data,FXival count) override;

  /// Write block of bytes, returning number of bytes written
  FXival writeBlock(const void* data,FXival count) override;

  /// Truncate file
  FXlong truncate(FXlong size) override;

  /// Flush
  FXbool flush() override;

  /// Test if we're at the end
  FXint eof() override;

  /// Return size of i/o device
  FXlong size() override;

  /// Close handle
  FXbool close() override;

  /// Destroy and close
  virtual ~BufferIO();
  };

}
#endif
