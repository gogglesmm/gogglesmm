#ifndef ENGINE_THREAD_H
#define ENGINE_THREAD_H

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

  /// Destructor
  virtual ~EngineThread();
  };




#endif
