/********************************************************************************
*                                                                               *
*                            W o r k e r   T h r e a d                          *
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
#ifndef FXWORKER_H
#define FXWORKER_H

#ifndef FXTHREAD_H
#include "FXThread.h"
#endif

namespace FX {


/**
* An FXWorker is a transient thread that performs an FXRunnable task.
* After the worker thread finishes the execution of the task, the worker thread's
* memory is automatically reclaimed.
* The FXRunnable itself is not deleted; it will thus outlive the worker that runs it.
* Any exceptions raised by the task are caught by the worker thread, and will
* be rethrown after the worker thread's memory has been reclaimed.
*/
class FXAPI FXWorker : public FXThread {
private:
  FXRunnable *runnable;
private:
  FXWorker(const FXWorker&);
  FXWorker &operator=(const FXWorker&);
public:

  /// Create worker for runnable
  FXWorker(FXRunnable* task=nullptr);

  /// Change runnable if not started yet
  void setRunnable(FXRunnable* task){ runnable=task; }

  /// Return runnable
  FXRunnable* getRunnable() const { return runnable; }

  /// Run worker
  virtual FXint run();

  /// Create and start a worker on a given runnable.
  static FXWorker* execute(FXRunnable* task,FXuval stacksize=0);

  /// Destroy
  virtual ~FXWorker();
  };

}

#endif
