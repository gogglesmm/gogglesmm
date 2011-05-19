#ifndef OUTPUT_H
#define OUTPUT_H

namespace ap {

class AudioEngine;
class OutputPlugin;
class Packet;





struct ReplayGainConfig {
  ReplayGainMode  mode;
  ReplayGain      value;

  ReplayGainConfig() : mode(ReplayGainOff) {}

  FXdouble gain() const { return (mode==ReplayGainAlbum) ? value.album      : value.track; }
  FXdouble peak() const { return (mode==ReplayGainAlbum) ? value.album_peak : value.track_peak; }
  };

class OutputThread : public EngineThread {
protected:
  Event * wait_for_packet();
  Event * wait_for_event();
protected:
  OutputConfig   output_config;
public:
  AudioFormat    af;
  OutputPlugin * plugin;
  FXDLL          dll;
  MemoryBuffer   converted_samples;
  MemoryBuffer   src_input;
  MemoryBuffer   src_output;
  ReplayGainConfig  replaygain;
protected:
  FXbool processing;
protected:
  FXint     stream;
  FXint     stream_length;
  FXint     stream_remaining;
  FXint     stream_written;
  FXint     stream_position;
  FXint     timestamp;
protected:
  void configure(const AudioFormat&);
  void load_plugin();
  void unload_plugin();
  void close_plugin();
  void process(Packet*);

#ifdef HAVE_SAMPLERATE_PLUGIN
  void resample(Packet*,FXint & nframes);
#endif

  void drain(FXbool flush=true);

  void update_position(FXint stream,FXint position,FXint nframes,FXint length);
  void notify_position();
  void reset_position();

  void reconfigure();
public:
  OutputThread(AudioEngine*);
  virtual FXint run();
  virtual ~OutputThread();
public:
  void getOutputConfig(OutputConfig & config);

  /// Get the replay gain mode
  ReplayGainMode getReplayGain() const { return replaygain.mode; }
  };

}
#endif

