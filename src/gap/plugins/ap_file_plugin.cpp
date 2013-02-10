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
#include "ap_defs.h"
#include "ap_utils.h"
#include "ap_event.h"
#include "ap_pipe.h"
#include "ap_format.h"
#include "ap_buffer.h"
#include "ap_input_plugin.h"
#include "ap_file_plugin.h"

using namespace ap;

namespace ap {


FileInput::FileInput(InputThread * i) : InputPlugin(i) {
  }

FileInput::~FileInput() {
  }

FXbool FileInput::open(const FXString & uri) {
  if (file.open(uri,FXIO::Reading)){
    filename=uri;
    return true;
    }
  return false;
  }

FXival FileInput::io_read(void*data,FXival ncount) {
  return file.readBlock(data,ncount);
  }

FXlong FileInput::position(FXlong offset,FXuint from) {
  return file.position(offset,from);
  }

FXlong FileInput::position() const {
  return file.position();
  }

FXlong FileInput::size() {
  return file.size();
  }

FXbool FileInput::eof()  {
  return file.eof();
  }

FXbool FileInput::serial() const {
  return file.isSerial();
  }

FXuint FileInput::plugin() const {
  FXString extension=FXPath::extension(filename);
  return ap_format_from_extension(extension);
  }


}
