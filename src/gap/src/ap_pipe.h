#ifndef AP_PIPE_H
#define AP_PIPE_H

class Event;

class Pipe {
protected:
  FXInputHandle h[2];
private:
  Pipe(const Pipe&);
  Pipe& operator=(const Pipe&);
public:
  /// Constructor
  Pipe();

  /// Create Pipe
  virtual FXbool create();

  /// Close Pipe
  void close();

  /// Return read handle
  FXInputHandle handle() const { return h[0]; }

  virtual ~Pipe();
  };

class EventPipe : public Pipe {
private:
  EventPipe(const EventPipe&);
  EventPipe& operator=(const EventPipe&);
public:
  /// Constructor
  EventPipe();

  /// Push Event
  void push(Event*);

  /// Pop Event
  Event* pop();

  /// Destructor
  virtual ~EventPipe();
  };


class NotifyPipe : public Pipe {
private:
  NotifyPipe(const NotifyPipe&);
  NotifyPipe& operator=(const NotifyPipe&);
public:
  /// Constructor
  NotifyPipe();

  /// Create Pipe
  FXbool create();

  /// Clear Pipe
  void clear();

  /// Signal
  void signal();

  /// Destructor
  virtual ~NotifyPipe();
  };

#endif
