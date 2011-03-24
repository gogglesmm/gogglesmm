#ifndef AP_DEVICE_H
#define AP_DEVICE_H

namespace ap {

enum {
  DeviceNone    = -1,
  DeviceAlsa    = 0,
  DeviceOSS     = 1,
  DevicePulse   = 2,
  DeviceRSound  = 3,
  DeviceJack    = 4
  };

class GMAPI DeviceConfig {
public:
  DeviceConfig();
  virtual ~DeviceConfig();
public:
  static FXuint devices();
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
  virtual ~AlsaConfig();
  };


class GMAPI OSSConfig : public DeviceConfig {
public:
  FXString device;
public:
  OSSConfig();
  OSSConfig(const FXString & d);
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
  };


}

#endif

