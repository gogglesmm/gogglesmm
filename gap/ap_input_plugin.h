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
#ifndef INPUT_PLUGIN_H
#define INPUT_PLUGIN_H

#include "ap_format.h"
#include "ap_signal.h"

namespace ap {

class MetaInfo;

struct IOContext {

  // Access to User Signal
  virtual const Signal & signal() = 0;

  // check if IO was aborted by user
  virtual FXbool aborted() = 0;

  // post meta data from io layer
  virtual void post_meta(MetaInfo*) = 0;

};

class InputPlugin {
protected:
  IOContext * context;
private:
  InputPlugin(const InputPlugin&);
  InputPlugin &operator=(const InputPlugin&);
public:
  InputPlugin(IOContext * ctx) : context(ctx) {}

  virtual FXbool open(const FXString & uri) = 0;

  FXbool read_int16_be(FXshort&);
  FXbool read_uint16_be(FXushort&);
  FXbool read_uint24_be(FXuint&);
  FXbool read_int32_be(FXint &);
  FXbool read_uint32_le(FXuint &);
  FXbool read_uint32_be(FXuint &);
  FXbool read_int64_be(FXlong &);
  FXbool read_uint64_be(FXulong &);
  FXbool read_float_be(FXfloat&);
  FXbool read_double_be(FXdouble&);

  /// Read ncount bytes, returns -1 for error, -2 for interrupted
  virtual FXival read(void*data,FXival ncount) = 0;

  //// Read ncount preview bytes. Position of stream doesn't change
  virtual FXival preview(void*data,FXival ncount) = 0;

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

  /// Open plugin for given url
  static InputPlugin* open(IOContext * ctx,const FXString & url);

  /// Destructor
  virtual ~InputPlugin() {}
  };


}
#endif
