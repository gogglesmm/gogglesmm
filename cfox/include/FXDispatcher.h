/********************************************************************************
*                                                                               *
*                      C a l l b a c k   D i s p a t c h e r                    *
*                                                                               *
*********************************************************************************
* Copyright (C) 2006,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
********************************************************************************/
#ifndef FXDISPATCHER_H
#define FXDISPATCHER_H

#ifndef FXREACTOR_H
#include "FXReactor.h"
#endif

namespace FX {


/**
* A FXDispatcher watches a number of devices and signals for activity
* and dispatches to the proper function when activity is observed.
*/
class FXAPI FXDispatcher : public FXReactor {
private:
  struct Handle;
  struct Signal;
  struct Idle;
  struct Timer;
private:
  FXHash        handles;                // Handle callbacks
  Signal      **signals;                // Signal callbacks
  Timer        *timers;                 // Timeout callbacks
  Idle         *idles;                  // Idle callbacks
  Timer        *timerrecs;              // Timer records
  Idle         *idlerecs;               // Idle records
private:
  FXDispatcher(const FXDispatcher&);
  FXDispatcher &operator=(const FXDispatcher&);
public:

  /// I/O Handle callback when a handle is raised
  typedef FXCallback<FXbool(FXDispatcher*,FXInputHandle,FXuint,void*)> HandleCallback;

  /// Signal callback when signal has been fired
  typedef FXCallback<FXbool(FXDispatcher*,FXint,void*)> SignalCallback;

  /// Timer callback when timer expired
  typedef FXCallback<FXbool(FXDispatcher*,FXTime,void*)> TimeoutCallback;

  /// Idle callback when dispatcher is about to block
  typedef FXCallback<FXbool(FXDispatcher*,void*)> IdleCallback;

public:

  /// Construct dispatcher object.
  FXDispatcher();

  /// Initialize dispatcher.
  virtual FXbool init();

  /// Add callback cb with new handle hnd to watch-list
  virtual FXbool addHandle(HandleCallback cb,FXInputHandle hnd,FXuint mode=InputRead,void* ptr=nullptr);

  /// Add new handle hnd to watch-list (no callback)
  virtual FXbool addHandle(FXInputHandle hnd,FXuint mode=InputRead);

  /// Remove handle hnd from watch-list
  virtual FXbool remHandle(FXInputHandle hnd);

  /// Return true if handle has been set.
  virtual FXbool hasHandle(FXInputHandle hnd) const;

  /// Dispatch handler when handle index is raised.
  /// Return true if the handle was raised and the callback returned true.
  virtual FXbool dispatchHandle(FXInputHandle hnd,FXuint mode,FXuint flags);

  /// Add (optionally asynchronous) callback cb for signal sig to signal-set
  virtual FXbool addSignal(SignalCallback cb,FXint sig,void* ptr=nullptr,FXbool async=false);

  /// Add (optionally asynchronous) signal sig to signal-set (no callback)
  virtual FXbool addSignal(FXint sig,FXbool async=false);

  /// Remove signal from signal-set
  virtual FXbool remSignal(FXint sig);

  /// Dispatch a signal handler if signal fired.
  /// Return true if the signal was raised and the callback handler returned true.
  virtual FXbool dispatchSignal(FXint sig);

  /// Add timeout callback cb at time due (ns since Epoch).
  /// If callback cb was already set, remove it and return its old
  /// data pointer, then reset it to the new time and data pointer.
  virtual void* addTimeout(TimeoutCallback cb,FXTime due,void* ptr=nullptr);

  /// Add timeout callback cb after time interval (ns).
  /// If callback cb was already set, remove it and return its old
  /// data pointer, then reset it to the new time and data pointer.
  virtual void* addInterval(TimeoutCallback cb,FXTime interval,void* ptr=nullptr);

  /// Remove timeout callback cb.
  /// If callback cb was set, return its data pointer.
  virtual void* remTimeout(TimeoutCallback cb);

  /// Return when timeout callback cb is due.
  /// If callback cb was not set or has expired, return forever.
  virtual FXTime getTimeout(TimeoutCallback cb) const;

  /// Return true if timeout callback cb been set.
  virtual FXbool hasTimeout(TimeoutCallback cb) const;

  /// Return time when first timer callback is due.
  /// If no timeout callback is currently set, return forever.
  virtual FXTime nextTimeout();

  /// Dispatch a timer when due.
  /// Return true if a timer was due and the callback returned true.
  virtual FXbool dispatchTimeout(FXTime due);

  /// Add idle callback be executed when dispatch about to block.
  /// If callback cb was already set, remove it and return its old
  /// data pointer, then reset the callback with the new pointer.
  virtual void* addIdle(IdleCallback cb,void* ptr=nullptr);

  /// Remove idle callback cb.
  /// If callback cb was set, return its data pointer.
  virtual void* remIdle(IdleCallback cb);

  /// Return true if idle callback cb been set.
  virtual FXbool hasIdle(IdleCallback cb) const;

  /// Dispatch one idle callback.
  /// Return true if a chore was set and the callback returned true.
  virtual FXbool dispatchIdle();

  /// Exit dispatcher.
  virtual FXbool exit();

  /// Destroy dispatcher object.
  virtual ~FXDispatcher();
  };


}

#endif
