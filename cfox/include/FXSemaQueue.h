/********************************************************************************
*                                                                               *
*                          S e m a p h o r e   Q u e u e                        *
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
#ifndef FXSEMAQUEUE_H
#define FXSEMAQUEUE_H

namespace FX {


/**
* Semaphore protected queue for a single producer and
* a single consumer thread.
* Producer thread will only block in push() when the queue
* is full; likewise, consumer will only block in pop() if
* the queue is empty.
* To avoid blocking the producer, call trypush() instead.
* Without timeout parameter, trypush() will return immediately
* if no space is available.
* When the timeout parameter is passed, the producer will wait
* a finite amount of time before giving up.
* Similarly, to avoid blocking the consumer, call trypop().
* Without the timeout parameter, trypop() will return immediately
* if no items are available.
* When the timeout parameter is passed, the consumer will wait
* a finite amount of time before giving up.
*/
class FXAPI FXSemaQueue {
private:
  FXPtrQueue  queue;    // Queue
  FXSemaphore free;     // Free cells
  FXSemaphore used;     // Used cells
private:
  FXSemaQueue(const FXSemaQueue&);
  FXSemaQueue &operator=(const FXSemaQueue&);
public:

  /// Create initially empty queue of given size sz
  FXSemaQueue(FXival sz=32);

  /// Return size
  FXival getSize() const { return queue.getSize(); }

  /// Add item to queue, return true if success
  FXbool push(FXptr ptr);

  /// Try push object into queue
  FXbool trypush(FXptr obj);

  /// Try push object into queue, waiting up
  /// to nsec for space to become available.
  FXbool trypush(FXptr obj,FXTime nsec);

  /// Remove item from queue, return true if success
  FXbool pop(FXptr& ptr);

  /// Try pop object from queue
  FXbool trypop(FXptr& obj);

  /// Try pop object from queue, waiting up
  /// to nsec for object to become available.
  FXbool trypop(FXptr& obj,FXTime nsec);

  /// Drop item from queue, return true if success
  FXbool pop();

  /// Destroy queue
 ~FXSemaQueue();
  };


// Specialize to pointers to TYPE
template<typename TYPE>
class FXSemaQueueOf : public FXSemaQueue {
public:

  /// Create initially empty queue of given size sz
  FXSemaQueueOf(FXival sz=32):FXSemaQueue(sz){}

  /// Add item to queue, return true if success
  FXbool push(TYPE* obj){ return FXSemaQueue::push((FXptr)obj); }

  /// Try push object into queue, return true if success
  FXbool trypush(TYPE* obj){ return FXSemaQueue::trypush((FXptr)obj); }

  /// Try push object into queue, return true if success
  FXbool trypush(TYPE* obj,FXTime nsec){ return FXSemaQueue::trypush((FXptr)obj,nsec); }

  /// Drop item from queue, return true if success
  FXbool pop(TYPE*& obj){ return FXSemaQueue::pop((FXptr&)obj); }

  /// Try pop object from queue, return true if success
  FXbool trypop(TYPE*& obj){ return FXSemaQueue::trypop((FXptr&)obj); }

  /// Try pop object from queue, return true if success
  FXbool trypop(TYPE*& obj,FXTime nsec){ return FXSemaQueue::trypop((FXptr&)obj,nsec); }
  };


}

#endif
