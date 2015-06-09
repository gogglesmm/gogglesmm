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
#include "ap_config.h"
#include "ap_defs.h"
#include "ap_device.h"
#include "ap_utils.h"

namespace ap {

DeviceConfig:: DeviceConfig() {
  }
DeviceConfig::~DeviceConfig(){
  }

static const FXchar * const plugin_names[DeviceLast]={
  "none",
  "alsa",
  "oss",
  "pulse",
  "jack",
  "wav"
  };

static FXbool ap_has_plugin(FXuchar device) {
  FXString path = FXPath::search(ap_get_environment("GOGGLESMM_PLUGIN_PATH",AP_PLUGIN_PATH),FXSystem::dllName(FXString::value("gap_%s",plugin_names[device])));
  if (FXStat::exists(path) /*|| FXStat::exists(name)*/)
    return true;
  return false;
  }




AlsaConfig::AlsaConfig() : device("default"), flags(0) {
  }

AlsaConfig::AlsaConfig(const FXString & d,FXuint f) : device(d),flags(f) {
  }

AlsaConfig::~AlsaConfig(){
  }

void AlsaConfig::load(FXSettings & settings) {
  device=settings.readStringEntry("alsa","device",device.text());

  if (settings.readBoolEntry("alsa","use-mmap",false))
    flags|=DeviceMMap;
  else
    flags&=~DeviceMMap;

  if (settings.readBoolEntry("alsa","no-resample",false))
    flags|=DeviceNoResample;
  else
    flags&=~DeviceNoResample;
  }

void AlsaConfig::save(FXSettings & settings) const {
  settings.writeStringEntry("alsa","device",device.text());
  settings.writeBoolEntry("alsa","use-mmap",flags&DeviceMMap);
  settings.writeBoolEntry("alsa","no-resample",flags&DeviceNoResample);
  }

OSSConfig::OSSConfig() : device("/dev/dsp"), flags(0) {
  }

OSSConfig::OSSConfig(const FXString & d): device(d) {
  }

OSSConfig::~OSSConfig(){
  }

void OSSConfig::load(FXSettings & settings) {
  device=settings.readStringEntry("oss","device",device.text());
  }

void OSSConfig::save(FXSettings & settings) const {
  settings.writeStringEntry("oss","device",device.text());
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
  if (ap_has_plugin(DeviceWav))
    AP_ENABLE_PLUGIN(plugins,DeviceWav);
  return plugins;
  }

FXString OutputConfig::plugin() const {
  if (device>=DeviceAlsa && device<DeviceLast)
    return plugin_names[device];
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
  alsa.load(settings);
  oss.load(settings);
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

  alsa.save(settings);
  oss.save(settings);
  }


}
