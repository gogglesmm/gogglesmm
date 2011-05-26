#ifndef AP_DEVICE_H
#define AP_DEVICE_H

namespace ap {

enum {
  DeviceNone    = -1,
  DeviceAlsa    = 0,
  DeviceOSS     = 1,
  DevicePulse   = 2,
  DeviceRSound  = 3,
  DeviceJack    = 4,
  DeviceWav     = 5,
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
  FXchar      device;
  FXuchar     replaygain;
public:
  OutputConfig();

  FXString plugin() const;

  void load(FXSettings &);

  void save(FXSettings &) const;

  /// Return bitmask of available outputs
  static FXuint devices();
  };

#define AP_HAS_PLUGIN(devices,plugin) (devices&(1<<plugin))


}

#endif

