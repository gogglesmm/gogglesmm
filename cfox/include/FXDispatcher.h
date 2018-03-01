/********************************************************************************
*                                                                               *
*                         E v e n t   D i s p a t c h e r                       *
*                                                                               *
*********************************************************************************
* Copyright (C) 2006,2018 by Jeroen van der Zijp.   All Rights Reserved.        *
*********************************************************************************
* This library is free software; you can redistribute it and/or modify          *
* it under the terms of the GNU Lesser General Public License as published by   *
* the Free Software Foundation; either version 3 of the License, or             *
* (at your option) any later version.                                           *
*                                                                               *
* This library is distributed in the hope that it will be useful,               *
* but WITHOUT ANY WARRANTY; without even the implied warranty of                *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                 *
* GNU Lesser General Public License for more details.                           *
*                                                                               *
* You should have received a copy of the GNU Lesser General Public License      *
* along with this program.  If not, see <http://www.gnu.org/licenses/>          *
********************************************************************************/
#ifndef FXDISPATCHER_H
#define FXDISPATCHER_H

#ifndef FXOBJECT_H
#include "FXObject.h"
#endif

//////////////////////////////  UNDER DEVELOPMENT  //////////////////////////////


namespace FX {


/**
* A Dispatcher watches a number of devices and signals for activity
* and dispatches to the proper function when activity is observed.
*/
class FXAPI FXDispatcher {
protected:
  struct FXHandles;
protected:
  FXHandles      *handles;              // Handle to watch
  volatile FXint  signotified[64];      // Signal notify flag
  FXint           sigreceived;          // Most recent received signal
  FXint           numhandles;           // Number of handles
  FXint           numwatched;           // Number of watched
  FXint           numraised;            // Number of raised handles
  FXint           current;              // Current handle
  FXbool          initialized;          // Is initialized
protected:
  static FXDispatcher* volatile sigmanager[64]; // Signals managed flag
protected:
  FXDispatcher(const FXDispatcher&);
  FXDispatcher &operator=(const FXDispatcher&);
private:
  static CDECL void signalhandler(FXint sig);
  static CDECL void signalhandlerasync(FXint sig);
public:
  enum {
    InputNone   = 0,            /// Inactive handle
    InputRead   = 1,            /// Read input handle
    InputWrite  = 2,            /// Write input handle
    InputExcept = 4             /// Except input handle
    };
  enum {
    DispatchAll     = 0xffffffff,       /// Dispatch all events
    DispatchSignals = 0x00000001,       /// Dispatch signals
    DispatchTimers  = 0x00000002,       /// Dispatch timers
    DispatchIdle    = 0x00000004,       /// Dispatch idle processing
    DispatchInputs  = 0x00000008        /// Dispatch i/o handles
    };
public:

  /// Construct dispatcher object.
  FXDispatcher();

  /// Initialize dispatcher.
  virtual FXbool init();

  /// Is dispatcher initialized
  FXbool isInitialized() const { return initialized; }


  /// Return timeout when something needs to happen
  virtual FXTime getTimeout();


  /// Check if dispatcher handles given signal
  virtual FXbool hasSignal(FXint sig);

  /// Append signal to signal-set
  virtual FXbool addSignal(FXint sig,FXbool async=false);

  /// Remove signal from signal-set
  virtual FXbool remSignal(FXint sig);

  /// Remvoe all signals
  virtual FXbool remAllSignals();


  /// Append handle hnd to watch-list.
  virtual FXbool addHandle(FXInputHandle hnd,FXuint mode=InputRead);

  /// Remove handle hnd from watch-list.
  virtual FXbool remHandle(FXInputHandle hnd);


  /// Dispatch if something happens within given timeout
  virtual FXbool dispatch(FXTime blocking=forever,FXuint flags=DispatchAll);


  /// Dispatch when a signal was fired
  virtual FXbool dispatchSignal(FXint sig);

  /// Dispatch when handle with given mode becomes active
  virtual FXbool dispatchHandle(FXInputHandle hnd,FXuint mode);

  /// Dispatch when timeout expires
  virtual FXbool dispatchTimeout(FXTime due);

  /// Dispatch when idle
  virtual FXbool dispatchIdle();


  /// Exit dispatcher.
  virtual FXbool exit();

  /// Destroy dispatcher object.
  virtual ~FXDispatcher();
  };

}

#endif
