/*******************************************************************************
*                         Goggles Audio Player Library                         *
********************************************************************************
*           Copyright (C) 2010-2018 by Sander Jansen. All Rights Reserved      *
*                               ---                                            *
* This program is free software: you can redistribute it and/or modify         *
* it under the terms of the GNU General Public License as published by         *
* the Free Software Foundation, either version 3 of the License, or            *
* (at your option) any later version.                                          *
*                                                                              *
* This program is distributed in the hope that it will be useful,              *
* but WITHOUT ANY WARRANTY; without even the implied warranty of               *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                *
* GNU General Public License for more details.                                 *
*                                                                              *
* You should have received a copy of the GNU General Public License            *
* along with this program.  If not, see http://www.gnu.org/licenses.           *
********************************************************************************/
#ifndef THREAD_QUEUE_H
#define THREAD_QUEUE_H

#include "ap_event_queue.h"
#include "ap_signal.h"

namespace ap {

class Event;

class ThreadQueue : public EventQueue {
protected:
  FXMutex mfifo;
  Signal  sfifo;
public:
  ThreadQueue();

  /// init resources
  FXbool init();

  /// Free resources
  void free();

  /// Post event on queue
  void post(Event*,FXint where=Back);

  /// Get next event.
  Event * pop();

  /// Wait for next event
  Event * wait();

  /// Wait for specific event. Returns nullptr if other event occurred.
  Event * wait_for(FXuchar t);

  // Check if typed event is in front of the queue
  FXbool peek_if_not(FXuchar t);

  /// Pop typed event
  Event * pop_if_not(FXuchar t2,FXuchar t1);

  /// Flush all events.
  void flush();

  /// Check for abort
  FXbool checkAbort();

  /// Return signal object for this Queue
  const Signal & signal() const { return sfifo; }

  ~ThreadQueue();
  };

}
#endif
