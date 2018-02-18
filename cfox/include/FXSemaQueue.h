/********************************************************************************
*                                                                               *
*                          S e m a p h o r e   Q u e u e                        *
*                                                                               *
*********************************************************************************
* Copyright (C) 2006,2017 by Jeroen van der Zijp.   All Rights Reserved.        *
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


/// Semaphore protected queue
class FXAPI FXSemaQueue {
private:
  FXPtrQueue  list;     // Queue
  FXSemaphore free;     // Free cells
  FXSemaphore used;     // Used cells
private:
  FXSemaQueue(const FXSemaQueue&);
  FXSemaQueue &operator=(const FXSemaQueue&);
public:

  /// Create initially empty queue
  FXSemaQueue(FXuint sz=32);

  /// Try push object into queue
  FXbool trypush(FXptr obj);

  /// Add item to queue, return true if success
  FXbool push(FXptr ptr);

  /// Try pop object from queue
  FXbool trypop(FXptr& obj);

  /// Remove item from queue, return true if success
  FXbool pop(FXptr& ptr);

  /// Drop item from queue, return true if success
  FXbool pop();

  /// Destroy queue
 ~FXSemaQueue();
  };


// Specialize to pointers to TYPE
template<typename TYPE>
class FXSemaQueueOf : public FXSemaQueue {
public:
  FXSemaQueueOf(){}
  FXSemaQueueOf(FXuint sz):FXSemaQueue(sz){}
  FXbool trypush(TYPE* obj){ return FXSemaQueue::trypush((FXptr)obj); }
  FXbool push(TYPE* obj){ return FXSemaQueue::push((FXptr)obj); }
  FXbool trypop(TYPE*& obj){ return FXSemaQueue::trypop((FXptr&)obj); }
  FXbool pop(TYPE*& obj){ return FXSemaQueue::pop((FXptr&)obj); }
  };


}

#endif
