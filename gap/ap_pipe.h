/*******************************************************************************
*                         Goggles Audio Player Library                         *
********************************************************************************
*           Copyright (C) 2010-2015 by Sander Jansen. All Rights Reserved      *
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
#ifndef AP_PIPE_H
#define AP_PIPE_H

namespace ap {

class Event;

class Pipe {
protected:
  FXInputHandle h[2];
private:
  Pipe(const Pipe&);
  Pipe& operator=(const Pipe&);
public:
  /// Constructor
  Pipe();

  /// Create Pipe
  virtual FXbool create();

  /// Close Pipe
  void close();

  /// Return read handle
  FXInputHandle handle() const { return h[0]; }

  virtual ~Pipe();
  };

class EventPipe : public Pipe {
private:
  EventPipe(const EventPipe&);
  EventPipe& operator=(const EventPipe&);
public:
  /// Constructor
  EventPipe();

  /// Push Event
  void push(Event*);

  /// Pop Event
  Event* pop();

  /// Destructor
  virtual ~EventPipe();
  };


class NotifyPipe : public Pipe {
private:
  NotifyPipe(const NotifyPipe&);
  NotifyPipe& operator=(const NotifyPipe&);
public:
  /// Constructor
  NotifyPipe();

  /// Create Pipe
  FXbool create();

  /// Clear Pipe
  void clear();

  /// Signal
  void signal();

  /// Destructor
  virtual ~NotifyPipe();
  };

}
#endif
