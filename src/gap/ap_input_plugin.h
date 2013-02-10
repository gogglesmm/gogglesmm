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
#ifndef INPUT_PLUGIN_H
#define INPUT_PLUGIN_H

namespace ap {

class InputThread;

enum {
  AP_IO_ERROR = -1, // error occured
  AP_IO_BLOCK = -2  // nothing available
  };

class InputPlugin {
protected:
  InputThread*  input;
  MemoryBuffer  buffer;
private:
  InputPlugin(const InputPlugin&);
  InputPlugin &operator=(const InputPlugin&);
protected:
  virtual FXival        io_read(void*,FXival)=0;
  virtual FXival        io_read_block(void*,FXival);
  virtual FXival        io_write(const void*,FXival) { return -1; }
  virtual FXival        io_write_block(const void*,FXival);
  virtual FXival        io_buffer(FXival);
  FXbool                io_wait_read();
  FXbool                io_wait_write();
  virtual FXInputHandle io_handle() const { return BadHandle; }
protected:
  InputPlugin(InputThread*,FXival size);
public:
  InputPlugin(InputThread*);

  /// Read ncount bytes, returns -1 for error, -2 for interrupted
  virtual FXival read(void*data,FXival ncount);

  //// Read ncount preview bytes. Position of stream doesn't change
  virtual FXival preview(void*data,FXival ncount);

  /// Set Position
  virtual FXlong position(FXlong offset,FXuint from)=0;

  /// Get Position
  virtual FXlong position() const=0;

  /// Size
  virtual FXlong size()=0;

  /// End of Input
  virtual FXbool eof()=0;

  /// Serial
  virtual FXbool serial() const=0;

  /// Get plugin type
  virtual FXuint plugin() const { return Format::Unknown; }

  /// Destructor
  virtual ~InputPlugin();
  };



}
#endif
