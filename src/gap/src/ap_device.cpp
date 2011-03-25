#include "ap_config.h"
#include "ap_defs.h"
#include "ap_device.h"

namespace ap {

DeviceConfig:: DeviceConfig() {
  }
DeviceConfig::~DeviceConfig(){
  }

static const FXchar * plugin_names[5]={
  "alsa",
  "oss",
  "pulse",
  "rsound",
  "jack"
  };

static FXbool ap_has_plugin(FXuchar device) {
  FXString name = FXSystem::dllName(FXString::value("gap_plugin_%s",plugin_names[device]));
  FXString path = AP_PLUGIN_PATH PATHSEPSTRING + name;

  if (FXStat::exists(path) || FXStat::exists(name))
    return true;

  return false;
  }




AlsaConfig::AlsaConfig() : device("default"), flags(0) {
  }

AlsaConfig::AlsaConfig(const FXString & d,FXuint f) : device(d),flags(0) {
  }

AlsaConfig::~AlsaConfig(){
  }


OSSConfig::OSSConfig() : device("/dev/dsp"), flags(0) {
  }

OSSConfig::OSSConfig(const FXString & d): device(d) {
  }

OSSConfig::~OSSConfig(){
  }


OutputConfig::OutputConfig() {
#if defined(__linux__) && defined(HAVE_ALSA_PLUGIN)
  device=DeviceAlsa;
#elif defined(HAVE_OSS_PLUGIN)
  device=DeviceOSS;
#elif defined(HAVE_ALSA_PLUGIN)
  device=DeviceAlsa;
#elif defined(HAVE_PULSE_PLUGIN)
  device=DevicePulse;
#elif defined(HAVE_JACK_PLUGIN)
  device=DeviceJack;
#elif defined(HAVE_RSOUND_PLUGIN)
  device=DeviceRSound;
#else
  device=DeviceNone;
#endif
  }



FXuint OutputConfig::devices() {
  FXuint plugins=0;
#ifdef HAVE_ALSA_PLUGIN
  if (ap_has_plugin(DeviceAlsa))
    plugins|=(1<<DeviceAlsa);
#endif
#ifdef HAVE_OSS_PLUGIN
  if (ap_has_plugin(DeviceOSS))
    plugins|=(1<<DeviceOSS);
#endif
#ifdef HAVE_PULSE_PLUGIN
  if (ap_has_plugin(DevicePulse))
    plugins|=(1<<DevicePulse);
#endif
#ifdef HAVE_RSOUND_PLUGIN
  if (ap_has_plugin(DeviceRSound))
    plugins|=(1<<DeviceRSound);
#endif
#ifdef HAVE_JACK_PLUGIN
  if (ap_has_plugin(DeviceJack))
    plugins|=(1<<DeviceJack);
#endif
  return plugins;
  }

FXString OutputConfig::plugin() const {
  if (device>=DeviceAlsa && device<=DeviceJack)
    return plugin_names[(FXuchar)device];
  else
    return FXString::null;
  }

}
