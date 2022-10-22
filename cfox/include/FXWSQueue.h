/********************************************************************************
*                                                                               *
*                      W o r k - S t e a l i n g   Q u e u e                    *
*                                                                               *
*********************************************************************************
* Copyright (C) 2016,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#ifndef FXWSQUEUE_H
#define FXWSQUEUE_H

namespace FX {


/// Work-stealing queue
class FXAPI FXWSQueue {
private:
  FXPtrList      list;
  volatile FXint top;
  volatile FXint bot;
private:
  FXWSQueue(const FXWSQueue&);
  FXWSQueue &operator=(const FXWSQueue&);
public:

  /// Create a queue and set its size to sz
  FXWSQueue(FXint sz=256);

  /// Change size of the queue
  FXbool setSize(FXint sz);

  /// Return size
  FXint getSize() const { return list.no(); }

  /// Return number of used slots
  FXint getUsed() const;

  /// Return number of free slots
  FXint getFree() const;

  /// Check if queue is full
  FXbool isFull() const;

  /// Check if queue is empty
  FXbool isEmpty() const;

  /// Push pointer
  FXbool push(FXptr ptr);

  /// Pop pointer
  FXbool pop(FXptr& ptr);

  /// Take (steal) pointer
  FXbool take(FXptr& ptr);

  /// Delete queue
  virtual ~FXWSQueue();
  };

}

#endif

