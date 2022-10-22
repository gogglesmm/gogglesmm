/********************************************************************************
*                                                                               *
*                          L o c k - F r e e   Q u e u e                        *
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
#ifndef FXLFQUEUE_H
#define FXLFQUEUE_H

namespace FX {


/// Lock-free queue of void pointers
class FXAPI FXLFQueue {
private:
  FXPtrList       items;        // Item buffer
  volatile FXuint whead;        // Head write pointer
  volatile FXuint wtail;        // Tail write pointer
  volatile FXuint rhead;        // Head read pointer
  volatile FXuint rtail;        // Tail read pointer
private:
  FXLFQueue(const FXLFQueue&);
  FXLFQueue &operator=(const FXLFQueue&);
public:

  /// Create initially empty queue
  FXLFQueue();

  /// Create queue with initial size, which must be a power of two
  FXLFQueue(FXuint sz);

  /// Change size of queue (must be power of two); return true if success
  FXbool setSize(FXuint sz);

  /// Return size
  FXuint getSize() const { return (FXuint)items.no(); }

  /// Return number of used slots
  FXuint getUsed() const;

  /// Return number of free slots
  FXuint getFree() const;

  /// If queue not full, can write if no other producers
  FXbool isFull() const;

  /// If queue not empty, can read if no other consumers
  FXbool isEmpty() const;

  /// Add item to queue, return true if success
  FXbool push(FXptr ptr);

  /// Remove item from queue, return true if success
  FXbool pop(FXptr& ptr);

  /// Destroy queue
 ~FXLFQueue();
  };


/// Lock-free queue of pointers to TYPE
template <typename TYPE>
class FXLFQueueOf : public FXLFQueue {
public:
  FXLFQueueOf(){}
  FXLFQueueOf(FXuint sz):FXLFQueue(sz){}
  FXbool push(TYPE* ptr){ return FXLFQueue::push((FXptr)ptr); }
  FXbool pop(TYPE*& ptr){ return FXLFQueue::pop((FXptr&)ptr); }
  };

}

#endif
