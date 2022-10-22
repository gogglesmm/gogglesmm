/********************************************************************************
*                                                                               *
*                             T h r e a d   P o o l                             *
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


/*
  Notes:
  - Process tasks from lock-free queue using multiple threads.

  - A semaphore is used to block producing thread in the case the queue is full, for
    arbitrary amounts of time ranging from 0 to forever.

  - Producer ONLY ever blocks if queue is full, i.e. no free slot in the queue for an
    additional task.

  - Multiple producers may add tasks to the queue without lock-contention, if queue is
    not full.

  - Another semaphore is used to block consuming threads.  Permanent worker threads
    block indefinitely; temporary workers, if present, block only for short amount of
    time and exit when no tasks appear within that time.

  - Instead of waiting, a thread can become an additional consumer thread, except that
    it will not block but return if the queue is empty, or count becomes zero.

  - Task groups provide a mechanism to execute tasks which belong together, and allow
    the producing thread to know when the entire group is complete.

  - To this end, a task group maintains a completion object which records the number of
    tasks started versus completed, and which sports a semaphore which can coordinate
    the worker threads with the producer thread.

  - For groups of tasks belonging together, a counter is used to record the completions
    of all the tasks started in the group.  If the producer also participates in the
    processing of of tasks, it exits as soon as the completion count reaches zero or
    the task queue becomes empty.  In either case, it may have to block for a little
    while on the completion semaphore.

  - No new tasks can be posted when about to shut down; so when queue becomes empty, it
    will stay empty.

*/

using namespace FX;

/*******************************************************************************/

namespace FX {


// Locate thread pool to which worker thread belongs
FXAutoThreadStorageKey FXThreadPool::reference;


// Create thread pool
FXThreadPool::FXThreadPool(FXuint sz):queue(sz),freeslots(sz),usedslots(0),stacksize(0),expiration(forever),maximum(FXThread::processors()),minimum(1),workers(0),running(0){
  FXTRACE((100,"FXThreadPool::FXThreadPool(%d)\n",sz));
  }


// Change task queue size, return true if success
FXbool FXThreadPool::setSize(FXuint sz){
  if((sz<8) || (sz&(sz-1))){ fxerror("FXThreadPool::setSize: bad argument: %u.\n",sz); }
  if(atomicBoolCas(&running,0,2)){
    FXuint osz=queue.getSize();
    if(setSize(sz)){
      while(osz<sz){ ++osz; freeslots.post(); }
      while(osz>sz){ --osz; freeslots.wait(); }
      running=0;
      return true;
      }
    running=0;
    }
  return false;
  }


// Change minimum number of worker threads
FXbool FXThreadPool::setMinimumThreads(FXuint n){
  if(atomicBoolCas(&running,0,2)){
    minimum=n;
    running=0;
    return true;
    }
  return false;
  }


// Change maximum number of worker threads
FXbool FXThreadPool::setMaximumThreads(FXuint n){
  if(atomicBoolCas(&running,0,2)){
    maximum=FXMAX(n,1);
    running=0;
    return true;
    }
  return false;
  }


// Change expiration time
FXbool FXThreadPool::setExpiration(FXTime ns){
  if(atomicBoolCas(&running,0,2)){
    expiration=ns;
    running=0;
    return true;
    }
  return false;
  }


// Change stack size
FXbool FXThreadPool::setStackSize(FXuval sz){
  if(atomicBoolCas(&running,0,2)){
    stacksize=sz;
    running=0;
    return true;
    }
  return false;
  }


// Return calling thread's thread pool
FXThreadPool* FXThreadPool::instance(){
  return (FXThreadPool*)reference.get();
  }


// Change calling thread's thread pool
void FXThreadPool::instance(FXThreadPool *pool){
  reference.set(pool);
  }


// Start a worker and reset semaphore
FXbool FXThreadPool::startWorker(){
  threads.increment();
  if(FXWorker::execute(this,stacksize)){
    return true;
    }
  threads.decrement();
  return false;
  }


// Start thread pool
FXuint FXThreadPool::start(FXuint count){
  FXuint result=0;
  FXTRACE((150,"FXThreadPool::start(%u)\n",count));
  if(atomicBoolCas(&running,0,2)){

    // Start number of workers
    while(result<count && startWorker()){
      result++;
      }

    // Set context reference if not set yet
    if(instance()==nullptr) instance(this);

    // Start running
    running=1;
    }
  return result;
  }


// Wait until counter becomes zero, return if no new tasks posted within timeout
void FXThreadPool::runWhile(FXCompletion& comp,FXTime timeout){
  FXRunnable* task;
  while(!comp.done() && usedslots.wait(timeout) && queue.pop(task)){
    freeslots.post();
    try{
      task->run();
      }
    catch(...){
      tasks.decrement();
      throw;
      }
    tasks.decrement();
    }
  }


// Process tasks from the queue using multiple worker threads.
// When queue becomes empty, extra workers will exit if no work arrives within
// a set amount of time; the last worker to terminate will signal the semaphore.
// Any exceptions raised during task processing will be rethrown after adjusting
// the current count of workers.
FXint FXThreadPool::run(){
  FXuint w=atomicAdd(&workers,1);
  instance(this);
  try{
    runWhile(threads,(w<minimum)?forever:expiration);
    }
  catch(...){
    instance(nullptr);
    atomicAdd(&workers,-1);
    threads.decrement();
    throw;
    }
  instance(nullptr);
  atomicAdd(&workers,-1);
  threads.decrement();
  return 0;
  }


// Try to add new task to the queue, waiting for space if necessary
FXbool FXThreadPool::execute(FXRunnable* task,FXTime blocking){
  if(__likely(running==1 && task)){
    if(tasks.count()<threads.count() || maximum<=threads.count() || startWorker()){
      if(freeslots.wait(blocking)){
        tasks.increment();
        queue.push(task);
        usedslots.post();
        return true;
        }
      }
    }
  return false;
  }


// Execute task, and wait for all completion of all tasks
FXbool FXThreadPool::executeAndWait(FXRunnable* task,FXTime blocking){
  if(execute(task,blocking)){
    runWhile(tasks,0);
    tasks.wait();
    return true;
    }
  return false;
  }


// Execute task, and wait for completion
FXbool FXThreadPool::executeAndWaitFor(FXRunnable* task,FXCompletion& comp,FXTime blocking){
  if(execute(task,blocking)){
    runWhile(comp,0);
    comp.wait();
    return true;
    }
  return false;
  }


// Wait for completion of all tasks
FXbool FXThreadPool::wait(){
  if(__likely(running)){
    runWhile(tasks,0);
    tasks.wait();
    return true;
    }
  return false;
  }


// Wait for completion
FXbool FXThreadPool::waitFor(FXCompletion& comp){
  if(__likely(running)){
    runWhile(comp,0);
    comp.wait();
    return true;
    }
  return false;
  }


// Stop thread pool
FXbool FXThreadPool::stop(){
  FXTRACE((150,"FXThreadPool::stop()\n"));
  if(atomicBoolCas(&running,1,2)){
    FXint w=threads.count();

    // Help out processing tasks while waiting
    wait();

    // Queue empty now
    FXASSERT(queue.isEmpty());

    // Force all workers to stop
    while(w){ usedslots.post(); --w; }

    // Wait till last worker is done
    threads.wait();

    // Reset usedslots semaphore to zero
    while(usedslots.trywait()){ }

    // Unset context reference if set to this context
    if(instance()==this) instance(nullptr);

    // Stop running
    running=0;
    return true;
    }
  return false;
  }


// Delete thread pool
FXThreadPool::~FXThreadPool(){
  FXTRACE((100,"FXThreadPool::~FXThreadPool()\n"));
  stop();
  }

}
