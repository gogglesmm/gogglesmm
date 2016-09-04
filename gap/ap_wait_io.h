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
#ifndef AP_WAIT_IO_H
#define AP_WAIT_IO_H

namespace ap {

/*
  Wraps a non-blocking FXIO and makes it blocking. An optional watch handle and timeout
  can be passed to fall out of the blocking io.
*/
class WaitIO : public FXIO {
protected:
  FXIODevice* 	io;
  FXInputHandle watch;
  FXTime 			  timeout;
public:
  enum {
    Readable = 0,
    Writable = 1
    };
private:
  WaitIO(const WaitIO&);
  WaitIO &operator=(const WaitIO&);
public:
  WaitIO(FXIODevice * io,FXInputHandle watch=BadHandle,FXTime timeout=0);

  /// Return device
  FXIODevice* getDevice() const { return io; }

  /// Return true if open
  FXbool isOpen() const override;

  /// Return true if serial access only
  FXbool isSerial() const override;

  /// Get current file position
  FXlong position() const override;

  /// Change file position, returning new position from start
  FXlong position(FXlong offset,FXuint from=FXIO::Begin) override;

  /// Read block of bytes, returning number of bytes read
  FXival readBlock(void* data,FXival count) override;

  /// Write block of bytes, returning number of bytes written
  FXival writeBlock(const void* data,FXival count) override;

  /// Truncate file
  FXlong truncate(FXlong size) override;

  /// Flush to disk
  FXbool flush() override;

  /// Test if we're at the end
  FXint eof() override;

  /// Return size of i/o device
  FXlong size() override;

  /// Close handle
  FXbool close() override;

  /// Wait until
  virtual FXuint wait(FXuchar mode=WaitIO::Readable);

  /// Destroy and close
  virtual ~WaitIO();
  };

class ThreadQueue;

class ThreadIO : public WaitIO {
protected:
  ThreadQueue * fifo;
public:
  ThreadIO(FXIODevice * io,ThreadQueue*q,FXTime timeout=0);

  FXuint wait(FXuchar mode=WaitIO::Readable) override;
  };







}
#endif
