/*******************************************************************************
*                         Goggles Audio Player Library                         *
********************************************************************************
*           Copyright (C) 2010-2014 by Sander Jansen. All Rights Reserved      *
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
#ifndef APPLICATION_QUEUE_H
#define APPLICATION_QUEUE_H

namespace ap {

class FXAppQueue : public EventQueue {
protected:
  FXMutex            mfifo;
  FXMessageChannel * channel;
  FXObject         * target;
  FXSelector         message;
protected:
  FXAppQueue();
private:
  FXAppQueue(const FXAppQueue&);
  FXAppQueue& operator=(const FXAppQueue&);
public:
  /// Construct a FXAppQueue
  FXAppQueue(FXApp*,FXObject * tgt,FXSelector sel);

  /// Post event on queue
  void post(Event*,FXint where=Back);

  /// Get next event.
  Event * pop();

  /// Flush all events.
  void flush();

  /// Destructor
  virtual ~FXAppQueue();
  };

}

#endif
