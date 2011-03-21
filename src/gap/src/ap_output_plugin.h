#ifndef OUTPUT_PLUGIN_H
#define OUTPUT_PLUGIN_H

namespace ap {

class GMAPI OutputPlugin {
public:
  AudioFormat af;
public:
  /// Constructor
  OutputPlugin() {}

  /// Configure
  virtual FXbool configure(const AudioFormat &)=0;

  /// Write frames to playback buffer
  virtual FXbool write(const void*, FXuint)=0;

  /// Return delay in no. of frames
  virtual FXint delay() { return 0; }

  /// Empty Playback Buffer Immediately
  virtual void drop()=0;

  /// Wait until playback buffer is emtpy.
  virtual void drain()=0;

  /// Pause Playback
  virtual void pause(FXbool t)=0;

  /// Change Volume
  virtual void volume(FXfloat) { }

  /// Close Output
  virtual void close() {}

  /// Destructor
  virtual ~OutputPlugin() {}
  };

}
#endif
