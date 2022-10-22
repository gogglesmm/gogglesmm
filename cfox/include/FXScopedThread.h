/********************************************************************************
*                                                                               *
*                    S c o p e d   T h r e a d   S u p p o r t                  *
*                                                                               *
*********************************************************************************
* Copyright (C) 2013,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#ifndef FXSCOPEDTHREAD_H
#define FXSCOPEDTHREAD_H

#ifndef FXTHREAD_H
#include "FXThread.h"
#endif

namespace FX {


/**
* FXScopedThread is a kind of thread which automatically performs a thread
* join to clean up the thread when going out of scope.
* Scoped Threads should not be detached, as this would prevent a successful
* join().
* Note that Scoped Thread does not forcibly terminate the thread, and thus
* allows the thread function run() to conclude gracefully; calling thread
* waits inside join() until thread is done.
*/
class FXAPI FXScopedThread : public FXThread {
private:
  FXScopedThread(const FXScopedThread&);
  FXScopedThread &operator=(const FXScopedThread&);
public:

  /// Initialize thread object
  FXScopedThread();

  /// Destroy thread after joining it
  virtual ~FXScopedThread();
  };

}

#endif

