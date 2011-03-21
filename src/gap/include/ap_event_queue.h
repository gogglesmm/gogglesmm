#ifndef AP_EVENT_QUEUE_H
#define AP_EVENT_QUEUE_H

namespace ap {

class EventQueue {
protected:
  Event* head;
  Event* tail;
public:
  enum {
    Front,
    Back,
    Flush
    };
public:
  /// Constructor
  EventQueue();

  virtual FXbool init();

  virtual void free();

  /// Post event.
  virtual void post(Event*,FXint where=Back)=0;

  /// Pop event
  virtual Event * pop()=0;

  /// Flush all events.
  virtual void flush()=0;

  /// Destructor
  virtual ~EventQueue();
  };

}
#endif
