/********************************************************************************
*                                                                               *
*                          E v e n t   D i s p a t c h e r                      *
*                                                                               *
*********************************************************************************
* Copyright (C) 2019,2020 by Jeroen van der Zijp.   All Rights Reserved.        *
********************************************************************************/
#ifndef EVENTDISPATCHER_H
#define EVENTDISPATCHER_H

namespace FX {


/**
* A FXEventDispatcher extends FXDispatcher, adding graphical user interface
* handling and other display-related events.
*/
class FXAPI FXEventDispatcher : public FXDispatcher {
private:
  FXptr   display;              // Display
private:
  FXEventDispatcher(const FXEventDispatcher&);
  FXEventDispatcher &operator=(const FXEventDispatcher&);
public:
  typedef FXCallback<FXbool(FXEventDispatcher*,FXRawEvent& event)> EventCallback;
public:

  /// Construct event dispatcher object.
  FXEventDispatcher();

  /// Dispatch if something happens within given blocking time.
  /// Flags control subsets of events to be dispatched (signals, timers,
  /// idle, and more). Default is to dispatch all events.
  virtual FXbool dispatch(FXTime blocking=forever,FXuint flags=DispatchAll);

  /// Dispatch platform-dependent event
  virtual FXbool dispatchEvent(FXRawEvent& event);

  /// Destroy event dispatcher object.
  virtual ~FXEventDispatcher();
  };

}

#endif
