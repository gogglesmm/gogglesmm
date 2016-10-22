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
#ifndef AP_OPUS_H
#define AP_OPUS_H

#include "ap_event_private.h"

namespace ap {

class OpusConfig : public DecoderConfig {
public:
  FXuchar * info        = nullptr;
  FXuint    info_bytes  = 0;
public:

  void setOpusInfo(const FXuchar * data,FXuint len) {
    if (len>0) {
      info_bytes = len;
      allocElms(info,info_bytes);
      memcpy(info,data,info_bytes);
      }
    }

  ~OpusConfig() {
    freeElms(info);
    }
  };

}
#endif

