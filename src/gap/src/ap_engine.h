#ifndef ENGINE_H
#define ENGINE_H

namespace ap {

class InputThread;
class DecoderThread;
class OutputThread;
class EventQueue;

class AudioEngine {
public:
  EventQueue    * fifo;
  InputThread   * input;
  DecoderThread * decoder;
  OutputThread  * output;
public:
  AudioEngine();

  void post(Event*);

  FXbool init();

  void exit();

  ~AudioEngine();
  };
}
#endif
