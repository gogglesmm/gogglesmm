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
#ifndef FXTHREADPOOL_H
#define FXTHREADPOOL_H

#ifndef FXRUNNABLE_H
#include "FXRunnable.h"
#endif

namespace FX {


class FXWorker;

/// Task queue
typedef FXLFQueueOf<FXRunnable> FXTaskQueue;


/**
* A Thread Pool manages execution of tasks on a number of worker-threads.
*
* A task executed by the thread pool is queued up until a worker-thread becomes available
* to run it.
* To accomodate fluctuations in workloads and minimize resources, the number of worker-
* threads can be allowed to vary between a minimum and a maximum number.
* Idle worker-threads which receive no assignments within a specified amount of time will
* automatically terminate, thereby reduce system-resources used by the program.
* By default, the minimum number of worker-threads is 1, and the maximum number of worker-
* threads is equal to the number of processors in the system.
* During peak workloads, when the task queue may start to fill up, and no new worker-
* threads can be created, the calling thread to block until there is room in the queue
* for more tasks.
* However, if a non-default value is passed for the blocking-parameter to execute(), the
* calling thread will be blocked for only a finite amount of time (non-zero blocking value)
* or return immediately (zero blocking value).
* Failure to queue up a new task will result in execute() returning a false.
* The tasks which are passed to the thread pool are derived from FXRunnable.  In order
* to perform some useful function, subclasses of FXRunnable should overload the run()
* function.
* Uncaught exceptions thrown by a task are intercepted by the thread pool and rethrown
* after the necessary cleanup, and cause the worker thread to end prematurely.
* When the thread pool is stopped, it will wait until all tasks are finished, and then
* cause all worker-threads to terminate.
* The thread pool becomes associated (through a thread-local variable) with the calling
* thread when start() is called; this association lasts until stop() is called.
* In addition, each worker will similarly be associated with the thread pool.
* Thus both the main thread as well as the worker threads can easily find the thread
* pool through the member-function instance().
*/
class FXAPI FXThreadPool : public FXRunnable {
private:
  FXTaskQueue     queue;        // Task queue
  FXCompletion    tasks;        // Active tasks
  FXCompletion    threads;      // Active threads
  FXSemaphore     freeslots;    // Free slots in queue
  FXSemaphore     usedslots;    // Used slots in queue
  FXuval          stacksize;    // Stack size
  FXTime          expiration;   // Quit if no task within this time
  volatile FXuint maximum;      // Maximum threads
  volatile FXuint minimum;      // Minimum threads
  volatile FXuint workers;      // Working threads
  volatile FXuint running;      // Context is running
private:
  static FXAutoThreadStorageKey reference;
private:
  FXbool startWorker();
  void runWhile(FXCompletion& comp,FXTime timeout);
  virtual FXint run();
private:
  FXThreadPool(const FXThreadPool&);
  FXThreadPool &operator=(const FXThreadPool&);
public:

  /**
  * Construct an empty thread pool, with given task-queue size.
  */
  FXThreadPool(FXuint sz=256);

  /// Return true if running
  FXbool active() const { return running==1; }

  /// Change task queue size, return true if success
  FXbool setSize(FXuint sz);

  /// Return task queue size
  FXuint getSize() const { return queue.getSize(); }

  /// Return number of tasks
  FXuint getRunningTasks() const { return tasks.count(); }

  /// Return number of threads
  FXuint getRunningThreads() const { return threads.count(); }

  /// Change minimum number of worker threads; default is 1
  FXbool setMinimumThreads(FXuint n);

  /// Return minimum number of worker threads
  FXuint getMinimumThreads() const { return minimum; }

  /// Change maximum number of worker threads; default is #processors
  FXbool setMaximumThreads(FXuint n);

  /// Return maximum number of worker threads
  FXuint getMaximumThreads() const { return maximum; }

  /// Change expiration time for excess workers to terminate
  FXbool setExpiration(FXTime ns=forever);

  /// Get expiration time
  FXTime getExpiration() const { return expiration; }

  /// Change stack size (0 for default stack size)
  FXbool setStackSize(FXuval sz);

  /// Get stack size
  FXuval getStackSize() const { return stacksize; }

  /// Return calling thread's thread pool
  static FXThreadPool* instance();

  /// Change calling thread's thread pool
  static void instance(FXThreadPool *pool);

  /**
  * Start the thread pool with an initial number of threads equal to count.
  * Returns the number of threads actually started.
  * An association will be established between the calling thread and the thread pool.
  * This association lasts until stop() is called.  If another threadpool was already started
  * before by the calling thread, no new association will be established.
  */
  FXuint start(FXuint count=0);

  /**
  * Execute a task on the thread pool by entering it into the queue.
  * If a spot becomes available in the task queue within the timeout interval,
  * add the task to the queue and return true.
  * Return false if the task could not be added within the given time interval.
  * Possibly starts additional worker threads if the maximum number of worker
  * threads has not yet been exceeded.
  */
  FXbool execute(FXRunnable* task,FXTime blocking=forever);

  /**
  * Execute task on the thread pool by entering int into the queue.
  * If the task was successfully added, the calling thread will temporarily enter
  * the task-processing loop, and help out the worker-threads until all tasks
  * have finished processing.
  * Return false if the task could not be added within the given time interval.
  * Possibly starts additional worker threads if the maximum number of worker
  * threads has not yet been exceeded.
  */
  FXbool executeAndWait(FXRunnable* task,FXTime blocking=forever);

  /**
  * Execute task on the thread pool by entering int into the queue.
  * If the task was successfully added, the calling thread will temporarily enter
  * the task-processing loop, and help out the worker-threads until the completion
  * becomes signaled.
  * Return false if the task could not be added within the given time interval.
  * Possibly starts additional worker threads if the maximum number of worker
  * threads has not yet been exceeded.
  */
  FXbool executeAndWaitFor(FXRunnable* task,FXCompletion& comp,FXTime blocking=forever);

  /**
  * Wait until task queue becomes empty and all tasks are finished, and process tasks
  * to help the worker threads in the meantime.
  * If the thread pool was not running, return immediately with false; otherwise,
  * return when the queue is empty and all tasks have finished.
  */
  FXbool wait();

  /**
  * Wait until completion becomes signaled, and process tasks to help the worker
  * threads in the meantime.
  * If the thread pool was not running, return immediately with false; otherwise,
  * return when the completion becomes signaled, or when the thread pool is stopped.
  * Immediately return with false if the thread pool wasn't running.
  */
  FXbool waitFor(FXCompletion& comp);

  /**
  * Stop thread pool, and block posting of any new tasks to the queue.
  * Enter the task-processing loop and help the worker-threads until the task queue is
  * empty, and all tasks have finished executing.
  * The association between the calling thread, established when start() was called,
  * will hereby be dissolved, if the calling thread was associated with this thread pool.
  * Return false if the thread pool wasn't running.
  */
  FXbool stop();

  /**
  * Stop the thread pool and then delete it.
  */
  virtual ~FXThreadPool();
  };


}

#endif
