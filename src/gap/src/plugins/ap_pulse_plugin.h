#ifdef HAVE_PULSE_PLUGIN
#ifndef PULSE_PLUGIN_H
#define PULSE_PLUGIN_H

#include <pulse/pulseaudio.h>


class PulseOutput : public OutputPlugin {
protected:
  pa_threaded_mainloop * mainloop;
  pa_context           * context;
  pa_stream            * stream;
protected:
  FXbool open();
public:
  PulseOutput();

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
  virtual ~PulseOutput();
  };

#endif
#endif
