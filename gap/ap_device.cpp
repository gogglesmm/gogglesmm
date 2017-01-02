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
#include "ap_device.h"
#include "ap_utils.h"

namespace ap {
#if 0
DeviceConfig:: DeviceConfig() {
  }
DeviceConfig::~DeviceConfig(){
  }
#endif


static const FXchar * const plugin_names[DeviceLast]={
  "none",
  "alsa",
  "oss",
  "pulse",
  "jack",
  "wmm",
  "directx",
  "wav"
  };

static FXString ap_plugin_path(const FXuchar device) {
#ifdef _WIN32
  return FXPath::directory(FXSystem::getExecFilename()) + PATHSEPSTRING + FXSystem::dllName(FXString::value("gap_%s",plugin_names[device]));
#else
  return FXPath::search(ap_get_environment("GOGGLESMM_PLUGIN_PATH",AP_PLUGIN_PATH),FXSystem::dllName(FXString::value("gap_%s",plugin_names[device])));
#endif
  }

static FXbool ap_has_plugin(FXuchar device) {
  return FXStat::exists(ap_plugin_path(device));
  }

OutputConfig::OutputConfig() {
#if defined(__linux__) && defined(HAVE_ALSA)
  device=DeviceAlsa;
#elif defined(HAVE_OSS)
  device=DeviceOSS;
#elif defined(HAVE_PULSE)
  device=DevicePulse;
#elif defined(HAVE_JACK)
  device=DeviceJack;
#else
  device=DeviceWav;
#endif
  }


#define AP_ENABLE_PLUGIN(plugins,device) (plugins|=(1<<(device-1)))


FXuint OutputConfig::devices() {
  FXuint plugins=0;
#ifdef HAVE_ALSA
  if (ap_has_plugin(DeviceAlsa))
    AP_ENABLE_PLUGIN(plugins,DeviceAlsa);
#endif
#ifdef HAVE_OSS
  if (ap_has_plugin(DeviceOSS))
    AP_ENABLE_PLUGIN(plugins,DeviceOSS);
#endif
#ifdef HAVE_PULSE
  if (ap_has_plugin(DevicePulse))
    AP_ENABLE_PLUGIN(plugins,DevicePulse);
#endif
#ifdef HAVE_JACK
  if (ap_has_plugin(DeviceJack))
    AP_ENABLE_PLUGIN(plugins,DeviceJack);
#endif
#ifdef _WIN32
  if (ap_has_plugin(DeviceWindowsMultimedia))
	  AP_ENABLE_PLUGIN(plugins, DeviceWindowsMultimedia);

  if (ap_has_plugin(DeviceDirectSound))
	  AP_ENABLE_PLUGIN(plugins, DeviceDirectSound);
#endif
  if (ap_has_plugin(DeviceWav))
    AP_ENABLE_PLUGIN(plugins,DeviceWav);
  return plugins;
  }

FXString OutputConfig::plugin() const {
  if (device>=DeviceAlsa && device<DeviceLast)
    return ap_plugin_path(device);
  else
    return FXString::null;
  }


void OutputConfig::load(FXSettings & settings) {
  FXString output=settings.readStringEntry("engine","output",plugin_names[device]);
  for (FXint i=DeviceAlsa;i<DeviceLast;i++) {
    if (output==plugin_names[i]){
      device=i;
      break;
      }
    }

  alsa.device = settings.readStringEntry("alsa", "device", alsa.device.text());

  if (settings.readBoolEntry("alsa", "use-mmap", false))
	  alsa.flags |= AlsaConfig::DeviceMMap;
  else
	  alsa.flags &= ~AlsaConfig::DeviceMMap;

  if (settings.readBoolEntry("alsa", "no-resample", false))
	  alsa.flags |= AlsaConfig::DeviceNoResample;
  else
	  alsa.flags &= ~AlsaConfig::DeviceNoResample;

  oss.device = settings.readStringEntry("oss", "device", oss.device.text());
  }

void OutputConfig::save(FXSettings & settings) const {
/*
  FXuchar major,minor;
  ap_get_version(major,minor);
  settings.writeIntEntry("version","major",major);
  settings.writeIntEntry("version","minor",minor);
*/

  if (device>=DeviceAlsa && device<DeviceLast)
    settings.writeStringEntry("engine","output",plugin_names[device]);
  else
    settings.deleteEntry("engine","output");
  }


}
