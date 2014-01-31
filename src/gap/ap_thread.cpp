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
#include "ap_defs.h"
#include "ap_pipe.h"
#include "ap_event.h"
#include "ap_event_queue.h"
#include "ap_thread_queue.h"
#include "ap_thread.h"

namespace ap {

EngineThread::EngineThread(AudioEngine * e) : engine(e){
  }

EngineThread::~EngineThread() {
  }

FXbool EngineThread::init() {
  return fifo.init();
  }

void EngineThread::free() {
  fifo.free();
  }

void EngineThread::post(Event * event,FXint where) {
  fifo.post(event,where);
  }

}
