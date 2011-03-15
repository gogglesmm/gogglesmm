#ifndef AP_PLAYER_H
#define AP_PLAYER_H

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
  AudioPlayer(EventQueue*fifo=NULL);

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

  /// Set the output plugin
  void setOutputPlugin(const FXString &);

  /// Get the output plugin
  FXString getOutputPlugin() const;

  Event * pop();

  ~AudioPlayer();
  };


#endif
