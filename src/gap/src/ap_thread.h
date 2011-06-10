#ifndef ENGINE_THREAD_H
#define ENGINE_THREAD_H

namespace ap {

class AudioEngine;

class EngineThread : public FXThread {
protected:
  ThreadQueue  fifo;
public:
  AudioEngine * engine;
protected:
  virtual FXint process(Event*);
public:
  /// Constructor
  EngineThread(AudioEngine * engine);

  /// Init Thread
  virtual FXbool init();

  /// Free Resource
  virtual void free();

  /// Run thread
  virtual FXint run()=0;

  /// Post event to this thread
  void post(Event * event,FXint where=EventQueue::Back);

  /// Return 
  FXInputHandle getFifoHandle() const { return fifo.handle(); }

  /// Destructor
  virtual ~EngineThread();
  };

}


#endif
