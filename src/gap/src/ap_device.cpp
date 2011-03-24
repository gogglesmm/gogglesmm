#include "ap_config.h"
#include "ap_defs.h"
#include "ap_device.h"

namespace ap {

DeviceConfig:: DeviceConfig() {
  }
DeviceConfig::~DeviceConfig(){
  }

FXuint DeviceConfig::devices() {
  FXuint plugins=0;
#ifdef HAVE_ALSA_PLUGIN
  plugins|=(1<<DeviceAlsa);
#endif
#ifdef HAVE_OSS_PLUGIN
  plugins|=(1<<DeviceOSS);
#endif
#ifdef HAVE_PULSE_PLUGIN
  plugins|=(1<<DevicePulse);
#endif
#ifdef HAVE_RSOUND_PLUGIN
  plugins|=(1<<DeviceRSound);
#endif
#ifdef HAVE_JACK_PLUGIN
  plugins|=(1<<DeviceJack);
#endif
  return plugins;
  }



AlsaConfig::AlsaConfig() : device("default"), flags(0) {
  }

AlsaConfig::AlsaConfig(const FXString & d,FXuint f) : device(d),flags(0) {
  }

AlsaConfig::~AlsaConfig(){
  }


OSSConfig::OSSConfig() : device("/dev/dsp") {
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
  device=DeviceOSS;
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


FXString OutputConfig::plugin() const {
  switch(device) {
    case DeviceAlsa  : return "alsa";         break;
    case DeviceOSS   : return "oss";          break;
    case DevicePulse : return "pulse";        break;
    case DeviceJack  : return "jack";         break;
    case DeviceRSound: return "rsound";       break;
    default          : return FXString::null; break;
    }
  return FXString::null;
  }

}
