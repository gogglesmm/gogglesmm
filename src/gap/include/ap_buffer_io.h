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
#ifndef AP_BUFFER_IO_H
#define AP_BUFFER_IO_H

namespace ap {

class BufferIO : public FXIO {
protected:
	FXIO    * io;      // IO
  FXuchar * begptr;  // Begin of buffer
  FXuchar * endptr;  // End of buffer
  FXuchar * wrptr;   // Write pointer
  FXuchar * rdptr;   // Read pointer
	FXuchar   dir;		 // buffer direction
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

	/// Adjust Buffer Size
	FXbool setSpace(FXuval size);

	/// Attach an IO
	void attach(FXIO * io);

  /// Return true if open
  virtual FXbool isOpen() const;

  /// Return true if serial access only
  virtual FXbool isSerial() const;

  /// Get current file position
  virtual FXlong position() const;

  /// Change file position, returning new position from start
  virtual FXlong position(FXlong offset,FXuint from=FXIO::Begin);

	/// Peek block of bytes, return number of bytes peeked
	virtual FXival peekBlock(void* data,FXival count);

  /// Read block of bytes, returning number of bytes read
  virtual FXival readBlock(void* data,FXival count);

  /// Write block of bytes, returning number of bytes written
  virtual FXival writeBlock(const void* data,FXival count);

  /// Truncate file
  virtual FXlong truncate(FXlong size);

  /// Flush
  virtual FXbool flush();

  /// Test if we're at the end
  virtual FXint eof();

  /// Return size of i/o device
  virtual FXlong size();

  /// Close handle
  virtual FXbool close();

  /// Destroy and close
  virtual ~BufferIO();
  };

}
#endif
