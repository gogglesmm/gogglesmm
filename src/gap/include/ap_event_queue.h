#ifndef AP_EVENT_QUEUE_H
#define AP_EVENT_QUEUE_H

namespace ap {

class EventQueue {
protected:
  Event* head;
  Event* tail;
public:
  enum {
    Front, /// Add event to front of the queue
    Back,  /// Add event to the back of the queue
    Flush  /// Flush queue, then add event.
    };
private:
  EventQueue(const EventQueue&);
  EventQueue& operator=(const EventQueue&);    
public:
  /// Constructor
  EventQueue() : head(NULL), tail(NULL) {}

  /// Post event.
  virtual void post(Event*,FXint where=Back)=0;

  /// Pop event
  virtual Event * pop()=0;

  /// Flush all events.
  virtual void flush()=0;

  /// Destructor
  virtual ~EventQueue() {}
  };

}
#endif
