/********************************************************************************
*                                                                               *
*                        I / O   B u f f e r   C l a s s                        *
*                                                                               *
*********************************************************************************
* Copyright (C) 2005,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#ifndef FXIOBUFFER_H
#define FXIOBUFFER_H

#ifndef FXIO_H
#include "FXIO.h"
#endif

namespace FX {


/**
* IOBuffer provides a file-interface to a memory-buffer of a given size.
*/
class FXAPI FXIOBuffer : public FXIO {
protected:
  FXuchar *buffer;      // Memory buffer
  FXuval   pointer;     // Stream pointer
  FXuval   space;       // Space in buffer
private:
  FXIOBuffer(const FXIOBuffer&);
  FXIOBuffer &operator=(const FXIOBuffer&);
public:

  /// Construct
  FXIOBuffer();

  /// Construct and open
  FXIOBuffer(FXuchar* ptr,FXuval sz);

  /// Open buffer
  virtual FXbool open(FXuchar* ptr,FXuval sz);

  /// Obtain pointer to buffer
  FXuchar* data() const { return buffer; }

  /// Return true if open
  virtual FXbool isOpen() const;

  /// Return true if serial access only
  virtual FXbool isSerial() const;

  /// Get current buffer position
  virtual FXlong position() const;

  /// Change buffer position, returning new position from start
  virtual FXlong position(FXlong offset,FXuint from=FXIO::Begin);

  /// Read block of bytes, returning number of bytes read
  virtual FXival readBlock(void* ptr,FXival count);

  /// Write block of bytes, returning number of bytes written
  virtual FXival writeBlock(const void* ptr,FXival count);

  /// Truncate size of the buffer
  virtual FXlong truncate(FXlong sz);

  /// Flush to disk
  virtual FXbool flush();

  /// Test if we're at the end; -1 if error
  virtual FXint eof();

  /// Return size of the buffer
  virtual FXlong size();

  /// Close handle
  virtual FXbool close();

  /// Destroy and close
  virtual ~FXIOBuffer();
  };

}

#endif
