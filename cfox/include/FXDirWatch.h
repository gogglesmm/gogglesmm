/********************************************************************************
*                                                                               *
*           W a t c h   D i r e c t o r i e s   f o r   C h a n g e s           *
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
#ifndef FXDIRWATCH_H
#define FXDIRWATCH_H

#ifndef FXOBJECT_H
#include "FXObject.h"
#endif

namespace FX {


class FXApp;

#define DIRWATCH 1

/**
* Watches one or more directories (on Linux, also files..).
* Issue messages when one of them was changed.
* Each path has a corresponding handle which is used to interact with
* the operating system.
*/
class FXAPI FXDirWatch : public FXObject {
  FXDECLARE(FXDirWatch)
private:
  FXApp*              app;              // Back link to application object
protected:
  FXInputHandle       hnd;              // Handle
  FXDictionary        pathToHandle;     // Path to handle map
  FXReverseDictionary handleToPath;     // Handle to path map
  FXTime              timestamp;        // Time stamp
  FXObject*           target;           // Target object
  FXSelector          message;          // Message ID
private:
  FXDirWatch(const FXDirWatch&);
  FXDirWatch& operator=(const FXDirWatch&);
public:
  enum{
    ID_CHANGE=1,
    ID_LAST
    };
public:
  long onMessage(FXObject*,FXSelector,void*);
public:

  /// Initialize directory watcher
  FXDirWatch(FXApp* a=nullptr,FXObject* tgt=nullptr,FXSelector sel=0);

  /// Get application pointer
  FXApp* getApp() const { return app; }

  /// Set the message target object
  void setTarget(FXObject *t){ target=t; }

  /// Get the message target object
  FXObject* getTarget() const { return target; }

  /// Set the message identifier
  void setSelector(FXSelector sel){ message=sel; }

  /// Get the message identifier
  FXSelector getSelector() const { return message; }

  /// Add path to watch; return true if added
  FXbool addWatch(const FXString& path);

  /// Remove path to watch; return true if removed
  FXbool remWatch(const FXString& path);

  /// Clear all watches
  FXbool clearAll();

  /// Clean up directory watcher
  virtual ~FXDirWatch();
  };

}

#endif


