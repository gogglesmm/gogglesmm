#ifdef HAVE_RSOUND_PLUGIN
#ifndef RSOUND_PLUGIN_H
#define RSOUND_PLUGIN_H

#include <rsound.h>

class RSoundOutput : public OutputPlugin {
protected:
  rsound_t * rsd;
protected:
  FXbool open();
public:
  RSoundOutput();
  
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

  /// Pause
  void pause(FXbool);

  /// Change Volume
  void volume(FXfloat);

  /// Close Output
  void close();
  
  /// Destructor
  virtual ~RSoundOutput();
  };

#endif
#endif
