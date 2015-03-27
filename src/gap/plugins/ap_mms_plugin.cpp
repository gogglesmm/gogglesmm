/*******************************************************************************
*                         Goggles Audio Player Library                         *
********************************************************************************
*           Copyright (C) 2010-2015 by Sander Jansen. All Rights Reserved      *
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
#include "ap_mms_plugin.h"

#include <libmms/mms.h>

using namespace ap;

namespace ap {


MMSInput::MMSInput(InputThread* i) : InputPlugin(i),mms(nullptr) {
  }

MMSInput::~MMSInput() {
  }

FXbool MMSInput::open(const FXString & uri) {
  mms=mms_connect(nullptr,nullptr,uri.text(),128*1024);
  if (!mms) {
    GM_DEBUG_PRINT("failed to connect\n");
    return false;
    }
  GM_DEBUG_PRINT("mms connected\n");
  return true;
  }

FXival MMSInput::io_read(void*data,FXival ncount) {
  FXint result = mms_read(nullptr,(mms_t*)mms,(FXchar*)data,ncount);
  if (result<0) return -1;
  return result;
  }

FXlong MMSInput::position(FXlong /*offset*/,FXuint /*from*/) {
  return -1;
  }

FXlong MMSInput::position() const {
  return -1;
  }

FXlong MMSInput::size() {
  return -1;
  }

FXbool MMSInput::eof()  {
  return -1;
  }

FXbool MMSInput::serial() const {
  return true;
  }

FXuint MMSInput::plugin() const {
  return Format::Unknown;
  }


}
