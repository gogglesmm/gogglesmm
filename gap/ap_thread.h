/*******************************************************************************
*                         Goggles Audio Player Library                         *
********************************************************************************
*           Copyright (C) 2010-2017 by Sander Jansen. All Rights Reserved      *
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
#ifndef ENGINE_THREAD_H
#define ENGINE_THREAD_H


#include "ap_thread_queue.h"


namespace ap {

class AudioEngine;
class Event;

class EngineThread : public FXThread {
protected:
  ThreadQueue   fifo;
public:
  AudioEngine * engine;
protected:
  FXuint        stream;
public:
  /// Constructor
  EngineThread(AudioEngine * engine);

  /// Init Thread
  virtual FXbool init();

  /// Free Resource
  virtual void free();

  /// Run thread
  virtual FXint run()=0;

  /// Post event to this thread
  void post(Event * event,FXint where=EventQueue::Back);

  /// Return Fifo
  ThreadQueue & getFifo() { return fifo; }

  /// Destructor
  virtual ~EngineThread();
  };

}


#endif
