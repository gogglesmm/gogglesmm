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
  virtual FXbool isOpen() const;

  /// Return true if serial access only
  virtual FXbool isSerial() const;

  /// Get current file position
  virtual FXlong position() const;

  /// Change file position, returning new position from start
  virtual FXlong position(FXlong offset,FXuint from=FXIO::Begin);

  /// Read block of bytes, returning number of bytes read
  virtual FXival readBlock(void* data,FXival count);

  /// Write block of bytes, returning number of bytes written
  virtual FXival writeBlock(const void* data,FXival count);

  /// Truncate file
  virtual FXlong truncate(FXlong size);

  /// Flush to disk
  virtual FXbool flush();

  /// Test if we're at the end
  virtual FXbool eof();

  /// Return size of i/o device
  virtual FXlong size();

  /// Close handle
  virtual FXbool close();

	/// Wait until
	FXbool wait(FXuchar mode=WaitIO::Readable);

  /// Destroy and close
  virtual ~WaitIO();
	};
}
#endif
