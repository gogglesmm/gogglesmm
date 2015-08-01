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
#ifndef AP_DEVICE_H
#define AP_DEVICE_H

namespace ap {

enum {
  DeviceNone    = 0,
  DeviceAlsa    = 1,
  DeviceOSS     = 2,
  DevicePulse   = 3,
  DeviceRSound  = 4,
  DeviceJack    = 5,
  DeviceWav     = 6,
  DeviceLast,
  };

class GMAPI DeviceConfig {
public:
  DeviceConfig();

  virtual void load(FXSettings &)=0;
  virtual void save(FXSettings &) const=0;

  virtual ~DeviceConfig();
  };

class GMAPI AlsaConfig : public DeviceConfig {
public:
  enum {
    DeviceMMap        = 0x1,
    DeviceNoResample  = 0x2
    };
public:
  FXString device;
  FXuint   flags;
public:
  AlsaConfig();
  AlsaConfig(const FXString & d,FXuint f=0);

  void load(FXSettings &);

  void save(FXSettings &) const;

  ~AlsaConfig();
  };


class GMAPI OSSConfig : public DeviceConfig {
public:
  enum {
    DeviceNoResample = 0x2,
    };
public:
  FXString device;
  FXuint   flags;
public:
  OSSConfig();
  OSSConfig(const FXString & d);

  void load(FXSettings &);

  void save(FXSettings &) const;

  virtual ~OSSConfig();
  };



class GMAPI OutputConfig {
public:
  AlsaConfig  alsa;
  OSSConfig   oss;
  FXuchar     device;
public:
  OutputConfig();

  FXString plugin() const;

  void load(FXSettings &);

  void save(FXSettings &) const;

  /// Return bitmask of available outputs
  static FXuint devices();
  };

#define AP_HAS_PLUGIN(devices,plugin) (devices&(1<<(plugin-1)))


}

#endif

