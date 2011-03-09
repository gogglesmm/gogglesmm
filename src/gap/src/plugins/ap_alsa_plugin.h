#ifdef HAVE_ALSA_PLUGIN
#ifndef ALSA_PLUGIN_H
#define ALSA_PLUGIN_H

#include <alsa/asoundlib.h>

class AlsaOutput : public OutputPlugin {
protected:
  snd_pcm_t         * handle;
  snd_mixer_t       * mixer;
  snd_mixer_elem_t  * mixer_element;
protected:
  FXString device;
  FXbool   use_hw_samplerate;
  FXbool   use_mmap;
  FXbool   can_pause;
  FXbool   can_resume;
protected:
  FXbool open();
public:
  AlsaOutput();

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

  /// Change Volume
  void volume(FXfloat);
  
  /// Close Output
  void close();

  /// Destructor
  virtual ~AlsaOutput();
  };
#endif
#endif
