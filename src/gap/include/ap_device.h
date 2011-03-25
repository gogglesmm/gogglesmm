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
  enum {
    DeviceNoResample = 0x2,
    };
public:
  FXString device;
  FXuint   flags;
public:
  OSSConfig();
  OSSConfig(const FXString & d);
  virtual ~OSSConfig();
  };

class GMAPI OutputConfig {
public:
  AlsaConfig  alsa;
  OSSConfig   oss;
  FXchar      device;
public:
  OutputConfig();

  FXString plugin() const;

  /// Return bitmask of available outputs
  static FXuint devices();
  };


}

#endif

