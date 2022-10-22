/********************************************************************************
*                                                                               *
*                         F O X   E v e n t   L o o p                           *
*                                                                               *
*********************************************************************************
* Copyright (C) 2019,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#ifndef FXEVENTLOOP_H
#define FXEVENTLOOP_H

namespace FX {

// Forward declarations
class FXWindow;
class FXEventLoop;
class FXEventDispatcher;


/// Recursive event loop
class FXAPI FXEventLoop {
private:
  FXEventDispatcher *dispatcher;        // Event dispatcher
  FXEventLoop      **invocation;        // Pointer to variable holding pointer to current invocation
  FXEventLoop       *upper;             // Invocation above this one
  FXWindow          *window;            // Modal window (if any)
  FXuint             modality;          // Modality mode
  FXint              code;              // Return code
  FXbool             done;              // True if breaking out
public:
  enum{
    ModalForNone   = 0,
    ModalForWindow = 1,
    ModalForPopup  = 2
    };
private:
  FXEventLoop(const FXEventLoop&);
  FXEventLoop& operator=(const FXEventLoop&);
public:

  /// Initialize event loop
  FXEventLoop(FXEventLoop** inv,FXWindow* win=nullptr,FXuint mode=0);

  /// Set dispatcher
  void setDispatcher(FXEventDispatcher* disp){ dispatcher=disp; }

  /// Get dispatcher
  FXEventDispatcher* getDispatcher() const { return dispatcher; }

  /// Test if the window is involved in a modal invocation
  FXbool isModal(FXWindow *win) const;

  /// Return window of current modal event loop
  FXWindow* getModalWindow() const;

  /// Return window of this model event loop
  FXWindow* getWindow() const { return window; }

  /// Return mode of this model event loop
  FXuint getModality() const { return modality; }

  /// Break out of topmost event loop, closing all nested loops
  void stop(FXint value);

  /// Break out of modal loop matching window, and all deeper ones
  void stopModal(FXWindow* win,FXint value);

  /// Break out of modal loop, and all deeper non-modal ones
  void stopModal(FXint value);

  /// Destroy event loop
 ~FXEventLoop();
  };

}

#endif
