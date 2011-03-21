#ifdef HAVE_OSS_PLUGIN
#ifndef OSS_PLUGIN_H
#define OSS_PLUGIN_H

#include <soundcard.h>

namespace ap {

class OSSOutput : public OutputPlugin {
protected:
  FXInputHandle handle;
protected:
  FXString device;
  FXbool   use_hw_samplerate;
  FXbool   use_mmap;
  FXbool   can_pause;
  FXbool   can_resume;
protected:
  FXbool open();
public:
  OSSOutput();

  /// Configure
  FXbool configure(const AudioFormat &);

  /// Write frames to playback buffer
  FXbool write(const void*, FXuint);

  /// Return delay in no. of frames
  FXint delay();

  /// Empty Playback Buffer Immediately
  void drop();

  /// Wait until playback buffer is emtpy.
  void drain();

  /// Pause Playback
  void pause(FXbool t);

  /// Close Output
  void close();

  /// Destructor
  virtual ~OSSOutput();
  };

}
#endif
#endif
