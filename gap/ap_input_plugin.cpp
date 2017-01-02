/*******************************************************************************
*                         Goggles Audio Player Library                         *
********************************************************************************
*           Copyright (C) 2010-2017 by Sander Jansen. All Rights Reserved      *
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
#include "ap_input_plugin.h"


using namespace ap;

namespace ap {


extern InputPlugin * ap_file_plugin(InputThread*);
extern InputPlugin * ap_http_plugin(InputThread*);
#ifdef HAVE_SMB
extern InputPlugin * ap_smb_plugin(InputThread*);
#endif
#ifdef HAVE_CDDA
extern InputPlugin * ap_cdda_plugin(InputThread*);
#endif


InputPlugin::InputPlugin(InputThread * i) : input(i){
  }


InputPlugin::~InputPlugin() {
  }

  /// Open plugin for given url
InputPlugin* InputPlugin::open(InputThread * input,const FXString & url) {
  FXString scheme = FXURL::scheme(url);

  if (scheme.empty() || scheme=="file") {
    InputPlugin * file = ap_file_plugin(input);
    if (!file->open(url)){
      delete file;
      return nullptr;
      }
    return file;
    }

  // We do not support https yet, but hopefully we get a redirect
  else if (scheme=="http" || scheme=="https") {
    InputPlugin * http = ap_http_plugin(input);
    if (!http->open(url)){
      delete http;
      return nullptr;
      }
    return http;
    }
#ifdef HAVE_CDDA
  else if (scheme=="cdda") {
    InputPlugin * cdda = ap_cdda_plugin(input);
    if (!cdda->open(url)) {
      delete cdda;
      return nullptr;
      }
    return cdda;
    }
#endif
#ifdef HAVE_SMB
  else if (scheme=="smb") {
    InputPlugin * smb = ap_smb_plugin(input);
    if (!smb->open(url)) {
      delete smb;
      return nullptr;
      }
    return smb;
    }
#endif
  else {
    return nullptr;
    }
  }




FXbool InputPlugin::read_uint24_be(FXuint & value) {
  FXuchar v[3];
  if (read(&v,3)==3) {
#if FOX_BIGENDIAN == 0
    value = (v[0]<<16) | (v[1]<<8) | v[2];
#else
    value = v[0] | (v[1]<<8) | (v[2]<<16);
#endif
    return true;
    }
  return false;
  }


FXbool InputPlugin::read_uint32_le(FXuint & value) {
  if (read(&value,4)==4) {
#if FOX_BIGENDIAN == 1
    value = swap32(value);
#endif
    return true;
    }
  return false;
  }


FXbool InputPlugin::read_uint32_be(FXuint & value) {
  if (read(&value,4)==4) {
#if FOX_BIGENDIAN == 0
    value = swap32(value);
#endif
    return true;
    }
  return false;
  }


FXbool InputPlugin::read_uint64_be(FXulong & value) {
  if (read(&value,8)==8) {
#if FOX_BIGENDIAN == 0
    value = swap64(value);
#endif
    return true;
    }
  return false;
  }


FXbool InputPlugin::read_int64_be(FXlong & value) {
  if (read(&value,8)==8) {
#if FOX_BIGENDIAN == 0
    value = swap64(value);
#endif
    return true;
    }
  return false;
  }


FXbool InputPlugin::read_int32_be(FXint & value) {
  if (read(&value,4)==4) {
#if FOX_BIGENDIAN == 0
    value = swap32(value);
#endif
    return true;
    }
  return false;
  }


FXbool InputPlugin::read_uint16_be(FXushort & value) {
  if (read(&value,2)==2) {
#if FOX_BIGENDIAN == 0
    value = swap16(value);
#endif
    return true;
    }
  return false;
  }


FXbool InputPlugin::read_int16_be(FXshort & value) {
  if (read(&value,2)==2) {
#if FOX_BIGENDIAN == 0
    value = swap16(value);
#endif
    return true;
    }
  return false;
  }


FXbool InputPlugin::read_float_be(FXfloat & value) {
  FXuchar v[4];
  if (read(&v,4)==4) {
#if FOX_BIGENDIAN == 0
    reinterpret_cast<FXuchar*>(&value)[0] = v[3];
    reinterpret_cast<FXuchar*>(&value)[1] = v[2];
    reinterpret_cast<FXuchar*>(&value)[2] = v[1];
    reinterpret_cast<FXuchar*>(&value)[3] = v[0];
#else
    value = 0;
#endif
    return true;
    }
  return false;
  }


FXbool InputPlugin::read_double_be(FXdouble & value) {
  FXuchar v[8];
  if (read(&v,8)==8) {
#if FOX_BIGENDIAN == 0
    reinterpret_cast<FXuchar*>(&value)[0] = v[7];
    reinterpret_cast<FXuchar*>(&value)[1] = v[6];
    reinterpret_cast<FXuchar*>(&value)[2] = v[5];
    reinterpret_cast<FXuchar*>(&value)[3] = v[4];
    reinterpret_cast<FXuchar*>(&value)[4] = v[3];
    reinterpret_cast<FXuchar*>(&value)[5] = v[2];
    reinterpret_cast<FXuchar*>(&value)[6] = v[1];
    reinterpret_cast<FXuchar*>(&value)[7] = v[0];
#else
    value = 0;
#endif
    return true;
    }
  return false;
  }









}
