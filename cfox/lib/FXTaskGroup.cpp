/********************************************************************************
*                                                                               *
*                        T a s k   G r o u p   C l a s s                        *
*                                                                               *
*********************************************************************************
* Copyright (C) 2012,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "fxmath.h"
#include "FXException.h"
#include "FXElement.h"
#include "FXArray.h"
#include "FXPtrList.h"
#include "FXAtomic.h"
#include "FXSemaphore.h"
#include "FXCompletion.h"
#include "FXRunnable.h"
#include "FXAutoThreadStorageKey.h"
#include "FXThread.h"
#include "FXWorker.h"
#include "FXLFQueue.h"
#include "FXThreadPool.h"
#include "FXTaskGroup.h"


/*
  Notes:

  - FXTaskGroup manages the execution  of a task-set (a set of FXRunnables) on the FXThreadPool.

  - Each FXRunnable is 'wrapped' by FXTaskGroup::Task which updates the completion object
    inside FXTaskGroup.  This way, the FXTaskGroup can monitor the completion of the task-set
    it is monitoring.

  - When waiting for the task-set to finish, the calling thread may join the worker-threads in
    the FXThreadPool temporarily to add additional processing power; only when the FXThreadPoll
    queue is empty will the calling thread drop out of the FXThreadPoll's processing loop to
    actually block for the completion semaphore.

  - Possible improvements:

      o For FXParallel, we could have a special flavor of FXTaskGroup::Task that does not wrap
        a FXRunnable but has a more streamlined implementation, thus avoiding allocs and frees.

      o We would like some kind of early termination capability.  For instance, in a parallel
        search algorithm we would like to stop (or never even start) tasks when one of them is
        successful.

      o Another case of early termination would be the throwing of exceptions from one of the
        tasks.

      o For now, we don't have a good mechanism for this; we do want to keep this light-weight
        since FXTaskGroup is created on the stack in FXParallel.
*/

using namespace FX;

/*******************************************************************************/

namespace FX {


// Create new group of tasks
FXTaskGroup::FXTaskGroup():threadpool(FXThreadPool::instance()){
  if(!threadpool){ fxerror("FXTaskGroup::FXTaskGroup: No thread pool was set."); }
  }


// Create new group of tasks
FXTaskGroup::FXTaskGroup(FXThreadPool* p):threadpool(p){
  if(!threadpool){ fxerror("FXTaskGroup::FXTaskGroup: No thread pool was set."); }
  }


// Construct task group task
FXTaskGroup::Task::Task(FXTaskGroup* g,FXRunnable *r):taskgroup(g),runnable(r){
  taskgroup->completion.increment();
  }


// Destruct task group task
FXTaskGroup::Task::~Task(){
  taskgroup->completion.decrement();
  }


// Process task group task
FXint FXTaskGroup::Task::run(){
  try{
    runnable->run();
    }
  catch(...){
    delete this;
    throw;
    }
/*
  if(!group->isCancelled()){
    try{
      runnable->run();
      }
    catch(const FXResourceException&){         // FIXME perhaps dedicated FXException type to cancel the group, and another FXException type to cancel everything recursively...
      group->setCancelled(true);
      }
    }
*/
  delete this;
  return 0;
  }


// Start task
FXbool FXTaskGroup::execute(FXRunnable* task){
  if(__likely(task)){
    if(threadpool->execute(new FXTaskGroup::Task(this,task))){
      return true;
      }
    }
  return false;
  }


// Start task in this task group, then wait till done
FXbool FXTaskGroup::executeAndWait(FXRunnable* task){
  if(__likely(task)){
    if(threadpool->executeAndWaitFor(new FXTaskGroup::Task(this,task),completion)){
      return true;
      }
    }
  return false;
  }


// Wait for completion
FXbool FXTaskGroup::wait(){
  if(threadpool->waitFor(completion)){
    return true;
    }
  return false;
  }


// Wait for stuff
FXTaskGroup::~FXTaskGroup(){
  wait();
  }

}
