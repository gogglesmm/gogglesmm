#ifndef AP_PLAYER_H
#define AP_PLAYER_H

namespace ap {

class AudioEngine;
class EventQueue;
class Event;



class GMAPI AudioPlayer : public FXObject {
FXDECLARE(AudioPlayer)
private:
  AudioEngine * engine;
private:
  AudioPlayer(const AudioPlayer&);
  AudioPlayer& operator=(const AudioPlayer&);
protected:
  /// Set the event queue.
  void setEventQueue(EventQueue*);
public:
  AudioPlayer();

  /// Init
  FXbool init();

  /// Shutdown
  void exit();

  /// Open url and flush existing stream if true
  void open(const FXString & url,FXbool flush=true);

  /// Pause Stream
  void pause();

  /// Close Stream
  void close();

  /// Seek to Position
  void seek(FXdouble pos);

  /// Change Volume
  void volume(FXfloat v);

  /// Get the output configuration
  void getOutputConfig(OutputConfig & config) const;

  /// Set the output configuration
  void setOutputConfig(const OutputConfig & config);

  /// Set Replay Gain Mode
  void setReplayGain(ReplayGainMode mode);

  /// Get Replay Gain Mode
  ReplayGainMode getReplayGain() const;

  Event * pop();

  ~AudioPlayer();
  };

}
#endif
