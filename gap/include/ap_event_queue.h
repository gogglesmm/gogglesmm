/*******************************************************************************
*                         Goggles Audio Player Library                         *
********************************************************************************
*           Copyright (C) 2010-2016 by Sander Jansen. All Rights Reserved      *
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
#ifndef AP_EVENT_QUEUE_H
#define AP_EVENT_QUEUE_H

namespace ap {

class Event;

class GMAPI EventQueue {
protected:
  Event* head;
  Event* tail;
public:
  enum {
    Front, /// Add event to front of the queue
    Back,  /// Add event to the back of the queue
    Flush  /// Flush queue, then add event.
    };
private:
  EventQueue(const EventQueue&);
  EventQueue& operator=(const EventQueue&);
public:
  /// Constructor
  EventQueue() : head(nullptr), tail(nullptr) {}

  /// Post event.
  virtual void post(Event*,FXint where=Back)=0;

  /// Pop event
  virtual Event * pop()=0;

  /// Flush all events.
  virtual void flush()=0;

  /// Destructor
  virtual ~EventQueue() {}
  };

}
#endif
