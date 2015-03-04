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
#ifndef AP_UTILS_H
#define AP_UTILS_H

#include "ap_common.h"

namespace ap {


// Get textcodec for given mime or name
extern const FXTextCodec * ap_get_textcodec(const FXString &);

extern FXString ap_get_environment(const FXchar * key,const FXchar * def=NULL);

extern FXbool ap_set_closeonexec(FXInputHandle fd);


enum {
  WaitReadable = 0,
  WaitWritable = 1,
  };

enum {
  WaitHasIO        = 0,
  WaitHasTimeout   = 1,
  WaitHasError     = 2,
  WaitHasInterrupt = 3,
  };

// Wait for IO
extern FXuint ap_wait(FXInputHandle io,FXInputHandle watch=BadHandle,FXTime timeout=0,FXuchar mode=WaitReadable);


}
#endif

