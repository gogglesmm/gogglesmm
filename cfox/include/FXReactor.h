/********************************************************************************
*                                                                               *
*                            R e a c t o r   C l a s s                          *
*                                                                               *
*********************************************************************************
* Copyright (C) 2006,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#ifndef FXREACTOR_H
#define FXREACTOR_H

namespace FX {


/**
* FXReactor implements the reactor pattern.
* Given a list of file descriptors (handles), calling thread is blocked until
* any of the following happens:
*
*   - One or more of the file descriptors becomes active;
*   - A timeout occurs;
*   - The maximum blocking time is exceeded;
*   - An interrupt (signal) occurs.
*   - One or more of the handles develops an error and an FXFatalException
*     is thrown.
*
* Depending on what occurred, FXReactor dispatches to one of the handling
* API's: dispatchHandle(), dispatchTimeout(), or dispatchSignal().
* Prior to blocking, if nothing demands immediate attention, the special
* handler dispatchIdle() is called.
*
* If dispatchHandle(), dispatchTimeout(), dispatchSignal() or dispatchIdle()
* returned true, FXReactor will return with true; otherwise, it will continue
* to loop, decreasing the initial blocking time until eventually returning false
* when nothing happened during the entire blocking interval.
*
* Each FXReactor can manage a set of signals, but note that only one single
* FXReactor is allowed to manage a particular signal; other signals may be
* left unassigned, or managed by other FXReactors.
*
* If waiting for a very long but finite amount of time, FXReactor will wake up
* after at most FXReactor::maxwait, which is about 1 day.
* It will continue to loop and wait for additional intervals, decreasing the initial
* blocking time until eventually returning false when nothing happened.
*/
class FXAPI FXReactor {
  friend class FXEventDispatcher;
private:
  struct Internals;
private:
  Internals      *internals;            // Internals
  FXint           sigreceived;          // Most recent received signal
  FXint           numhandles;           // Number of handles
  FXint           numwatched;           // Number of watched
  FXint           numraised;            // Number of raised handles
  FXint           current;              // Current handle
private:
  static FXReactor* volatile sigmanager[64];
private:
  FXReactor(const FXReactor&);
  FXReactor &operator=(const FXReactor&);
private:
  static void CDECL signalhandler(FXint sig);
  static void CDECL signalhandlerasync(FXint sig);
public:

  /// Modes
  enum {
    InputNone   = 0,                    /// Inactive handle
    InputRead   = 1,                    /// Read input handle
    InputWrite  = 2,                    /// Write input handle
    InputExcept = 4                     /// Except input handle
    };

  /// Dispatch flags
  enum {
    DispatchAll     = 0xffffffff,       /// Dispatch all events
    DispatchSignals = 0x00000001,       /// Dispatch signals
    DispatchTimers  = 0x00000002,       /// Dispatch timers
    DispatchIdle    = 0x00000004,       /// Dispatch idle processing
    DispatchEvents  = 0x00000008,       /// Dispatch events
    DispatchOther   = 0x00000010        /// Dispatch other i/o
    };

  /// Sleep no longer than this
  static const FXTime maxwait;

public:

  /// Construct reactor.
  FXReactor();

  /// Initialize reactor.
  virtual FXbool init();

  /// Is reactor initialized.
  FXbool isInitialized() const { return (internals!=nullptr); }

  /// Dispatch if something happens within given blocking time.
  /// Flags control subsets of events to be dispatched (signals, timers,
  /// idle, and more). Default is to dispatch all events.
  virtual FXbool dispatch(FXTime blocking=forever,FXuint flags=DispatchAll);

  /// Add new handle hnd to watch-list
  virtual FXbool addHandle(FXInputHandle hnd,FXuint mode=InputRead);

  /// Remove handle hnd from watch-list
  virtual FXbool remHandle(FXInputHandle hnd);

  /// Dispatch handler for handle hnd.
  /// Return true if the callback returned true.
  virtual FXbool dispatchHandle(FXInputHandle hnd,FXuint mode,FXuint flags);

  /// Add (optionally asynchronous) signal sig to signal-set
  virtual FXbool addSignal(FXint sig,FXbool=false);

  /// Remove signal sig from signal-set
  virtual FXbool remSignal(FXint sig);

  /// Return true if signal sig is handled by this dispatcher.
  virtual FXbool hasSignal(FXint sig) const;

  /// Dispatch when a signal was fired; return true when handled.
  virtual FXbool dispatchSignal(FXint sig);

  /// Return time when first timer callback is due; the special
  /// value forever is returned when no timer is in effect.
  virtual FXTime nextTimeout();

  /// Dispatch when timeout expires; return true when handled.
  virtual FXbool dispatchTimeout(FXTime due);

  /// Dispatch when idle; return true when handled.
  virtual FXbool dispatchIdle();

  /// Exit reactor.
  virtual FXbool exit();

  /// Destroy reactor object.
  virtual ~FXReactor();
  };

}

#endif
