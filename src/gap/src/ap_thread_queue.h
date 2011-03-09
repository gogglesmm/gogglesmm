#ifndef THREAD_QUEUE_H
#define THREAD_QUEUE_H

class ThreadQueue : public EventQueue {
protected:
  FXMutex      mfifo;
  NotifyPipe   pfifo;
public:
  ThreadQueue();

  /// init resources
  FXbool init();

  /// Free resources
  void free();

  /// Post event on queue
  void post(Event*,FXint where=Back);

  /// Get next event.
  Event * pop();

  /// Pop typed event
  Event * pop_if(FXuchar t,FXbool & other);

  /// Pop typed event
  Event * pop_if_not(FXuchar t2,FXuchar t1);

  /// Flush all events.
  void flush();

  /// Peek Event
  FXuchar peek();

  /// Returns the wait fd.
  FXInputHandle handle() const;

  ~ThreadQueue();
  };

#endif
