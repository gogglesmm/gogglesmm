#ifdef HAVE_JACK_PLUGIN
#ifndef JACK_PLUGIN_H
#define JACK_PLUGIN_H

#include <jack/jack.h>

class JackOutput : public OutputPlugin {
protected:
  jack_client_t * jack;
protected:
  FXbool open();
public:
  JackOutput();

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
  virtual ~JackOutput();
  };

#endif
#endif
