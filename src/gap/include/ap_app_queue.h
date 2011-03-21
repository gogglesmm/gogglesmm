#ifndef APPLICATION_QUEUE_H
#define APPLICATION_QUEUE_H

namespace ap {

class GMAPI FXAppQueue : public EventQueue {
protected:
  FXMutex            mfifo;
  FXMessageChannel * channel;
  FXObject         * target;
  FXSelector         message;
protected:
  FXAppQueue();
private:
  FXAppQueue(const FXAppQueue&);
  FXAppQueue& operator=(const FXAppQueue&);
public:
  /// Construct a FXAppQueue
  FXAppQueue(FXApp*,FXObject * tgt,FXSelector sel);

  /// Post event on queue
  void post(Event*,FXint where=Back);

  /// Get next event.
  Event * pop();

  /// Flush all events.
  void flush();

  /// Destructor
  virtual ~FXAppQueue();
  };
  
}  

#endif
