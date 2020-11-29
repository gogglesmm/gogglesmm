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
#ifndef AP_VORBIS_H
#define AP_VORBIS_H

#include "ap_event_private.h"

namespace ap {

class VorbisConfig : public DecoderConfig {
public:
  FXuchar * info        = nullptr;
  FXuchar * setup       = nullptr;
  FXuint    info_bytes  = 0;
  FXuint    setup_bytes = 0;
public:

  void setVorbisInfo(const FXuchar * data,FXuint len) {
    if (len>0) {
      info_bytes = len;
      allocElms(info,info_bytes);
      memcpy(info,data,info_bytes);
      }
    }

  void setVorbisSetup(const FXuchar * data,FXuint len) {
    if (len>0) {
      setup_bytes = len;
      allocElms(setup,setup_bytes);
      memcpy(setup,data,setup_bytes);
      }
    }

  ~VorbisConfig() {
    freeElms(info);
    freeElms(setup);
    }
  };

}
#endif

