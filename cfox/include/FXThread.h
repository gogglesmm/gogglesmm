/********************************************************************************
*                                                                               *
*                          T h r e a d   S u p p o r t                          *
*                                                                               *
*********************************************************************************
* Copyright (C) 2004,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#ifndef FXTHREAD_H
#define FXTHREAD_H

#ifndef FXRUNNABLE_H
#include "FXRunnable.h"
#endif

namespace FX {


/**
* FXThread provides system-independent support for threads.
*
* Subclasses of FXThread must implement the run() function to implement
* the desired functionality of the thread object.  The thread can be
* started by calling start(), passing an optional size to allocate for
* the thread's stack space.
* Each thread can have thread-local storage.  FXThread has at least one
* thread-local variable, a pointer to the FXThread object itself; this
* value can be obtained at any time during the execution of the thread by
* calling self().  The value of self() is automatically set when the thread
* is started.
* Additional thread-local variables may be obtained using FXAutoThreadStorageKey.
* Initially, all signals are masked by newly started threads (only the main thread
* will normally handle signals).
* To reclaim the resources once the thread is completed, a call to join() must be
* made, or the thread must be detached (note however that detaching the thread will
* sever the association between FXThread and the thread).
* The special FXThreadException may be used to terminate a thread gracefully,
* and pass a return code to the corresponding join() operation.  This is preferred
* over the raw FXThread::exit().
* Unknown exceptions cause the program to terminate with an error.
* Calling the destructor from within the thread itself (suicide) is allowed; the
* thread will be detached and terminate immediately.
* Calling the destructor from another thread will cancel() the thread if it is
* still running.  Due to the asynchronous nature of threads, it is usually not a
* good idea to do this; it is recommended that subclasses call join(), and delay
* the execution of the destructor until the thread has completed normally.
*/
class FXAPI FXThread : public FXRunnable {
private:
  volatile FXThreadID tid;      // Handle to thread
  volatile FXbool     busy;     // Thread is running
public:

  /// Thread priority levels
  enum Priority {
    PriorityError=-1,   /// Failed to get priority
    PriorityDefault,  	/// Default scheduling priority
    PriorityMinimum,    /// Minimum scheduling priority
    PriorityLower,      /// Lower scheduling priority
    PriorityMedium,     /// Medium priority
    PriorityHigher,     /// Higher scheduling priority
    PriorityMaximum     /// Maximum scheduling priority
    };

  /// Thread scheduling policies
  enum Policy {
    PolicyError=-1,     /// Failed to get policy
    PolicyDefault,      /// Default scheduling
    PolicyFifo,         /// First in, first out scheduling
    PolicyRoundRobin 	/// Round-robin scheduling
    };

private:
  FXThread(const FXThread&);
  FXThread &operator=(const FXThread&);
private:
  static void self(FXThread* t);
private:
  static FXAutoThreadStorageKey selfKey;
private:
#if defined(WIN32)
  static unsigned int CALLBACK function(void*);
#else
  static void* function(void*);
#endif
public:

  /// Initialize thread object.
  FXThread();

  /**
  * Return handle of this thread object.
  * This handle is valid in the context of the thread which
  * called start().
  */
  FXThreadID id() const;

  /**
  * Return true if this thread is running.
  */
  FXbool running() const;

  /**
  * Start thread; the thread is started as attached.
  * The thread is given stacksize for its stack; a value of
  * zero for stacksize will give it the default stack size.
  * This invokes the run() function in the context of the new
  * thread.
  */
  FXbool start(FXuval stacksize=0);

  /**
  * Suspend calling thread until thread is done.  The FXThreadID is
  * reset back to zero.
  */
  FXbool join();

  /**
  * Suspend calling thread until thread is done, and set code to the
  * return value of run() or the argument passed into exit().  The
  * FXThreadID is reset back to zero.
  * If an exception happened in the thread, return -1.
  */
  FXbool join(FXint& code);

  /**
  * Cancel the thread, stopping it immediately, running or not.
  * If the calling thread is this thread, nothing happens.
  * It is probably better to wait until it is finished, in case the
  * thread currently holds mutexes.
  * The FXThreadID is reset back to zero after the thread has been
  * stopped.
  */
  FXbool cancel();

  /**
  * Detach thread, so that a no join() is necessary to harvest the
  * resources of this thread.  The thread continues to run until
  * normal completion.
  */
  FXbool detach();

  /**
  * Exit the calling thread.
  * No destructors are invoked for objects on thread's stack; to invoke destructors,
  * throw an exception instead; the special FXThreadException causes graceful termination
  * of the calling thread with return of an exit code for join().
  */
  static void exit(FXint code=0);

  /**
  * Make the thread yield its time quantum.
  */
  static void yield();

  /**
  * Processor pause/back-off.
  */
  static void pause();

  /**
  * Return time in nanoseconds since Epoch (Jan 1, 1970).
  */
  static FXTime time();

  /**
  * Get steady time in nanoseconds since some arbitrary start time.
  */
  static FXTime steadytime();

  /**
  * Return time in processor ticks.
  */
  static FXTime ticks();

  /**
  * Make the calling thread sleep for a number of nanoseconds.
  */
  static void sleep(FXTime nsec);

  /**
  * Wake at appointed time specified in nanoseconds since Epoch.
  */
  static void wakeat(FXTime nsec);

  /**
  * Return pointer to the FXThread instance associated
  * with the calling thread; it returns NULL for the main
  * thread and all threads not created by FOX.
  */
  static FXThread* self();

  /**
  * Return thread id of calling thread.
  */
  static FXThreadID current();

  /**
  * Return number of available processors (cores) in the system.
  */
  static FXint processors();

  /**
  * Return processor index of the calling thread; returns a value
  * between [0 ... processors()-1] if successful, and -1 otherwise.
  */
  static FXint processor();

  /**
  * Generate new thread local storage key.
  */
  static FXThreadStorageKey createStorageKey();

  /**
  * Dispose of thread local storage key.
  */
  static void deleteStorageKey(FXThreadStorageKey key);

  /**
  * Get thread local storage pointer using key.
  */
  static void* getStorage(FXThreadStorageKey key);

  /**
  * Set thread local storage pointer using key.
  */
  static void setStorage(FXThreadStorageKey key,void* ptr);

  /**
  * Set thread scheduling priority.
  */
  FXbool priority(Priority prio);

  /**
  * Return thread scheduling priority.
  */
  Priority priority() const;

  /**
  * Set thread scheduling policy.
  */
  FXbool policy(Policy plcy);

  /**
  * Get thread scheduling policy.
  */
  Policy policy() const;

  /**
  * Change thread's processor affinity, the set of
  * processors onto which the thread may be scheduled.
  */
  FXbool affinity(FXulong mask);

  /**
  * Get thread's processor affinity.
  */
  FXulong affinity() const;

  /**
  * Change thread description.
  */
  FXbool description(const FXString& desc);

  /**
  * Return thread description, if any.
  */
  FXString description() const;

  /**
  * Suspend thread; return true if success.
  */
  FXbool suspend();

  /**
  * Resume thread; return true if success.
  */
  FXbool resume();

  /**
  * Destroy the thread immediately, running or not.
  * It is probably better to wait until it is finished, in case
  * the thread currently holds mutexes.
  */
  virtual ~FXThread();
  };

}

#endif
