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
#include "xincs.h"
#include "fxdefs.h"
#include "FXSemaphore.h"
#include "FXElement.h"
#include "FXPtrList.h"
#include "FXPtrQueue.h"
#include "FXSemaQueue.h"


/*
  Notes:
  - Fixed sized, circular buffer protected by semaphores.
  - The semaphores synchronize producer and consumer; if multiple
    consumers or multiple producers are present then these must be
    deconflicted with some other mechanism (e.g. mutex around the
    push() or pop() commands).
*/

using namespace FX;

/*******************************************************************************/

namespace FX {

// Create a queue and set its size to sz
FXSemaQueue::FXSemaQueue(FXival sz):queue(sz),free(sz),used(0){
  }


// Push object into queue
FXbool FXSemaQueue::push(FXptr obj){
  free.wait();
  queue.push(obj);
  used.post();
  return true;
  }


// Try push object into queue
FXbool FXSemaQueue::trypush(FXptr obj){
  if(free.trywait()){
    queue.push(obj);
    used.post();
    return true;
    }
  return false;
  }


// Try push object into queue, waiting up
// to nsec for space to become available.
FXbool FXSemaQueue::trypush(FXptr obj,FXTime nsec){
  if(free.wait(nsec)){
    queue.push(obj);
    used.post();
    return true;
    }
  return false;
  }


// Pop onject from queue
FXbool FXSemaQueue::pop(FXptr& obj){
  used.wait();
  queue.pop(obj);
  free.post();
  return true;
  }


// Try pop object from queue
FXbool FXSemaQueue::trypop(FXptr& obj){
  if(used.trywait()){
    queue.pop(obj);
    free.post();
    return true;
    }
  return false;
  }


// Try pop object from queue, waiting up
// to nsec for object to become available.
FXbool FXSemaQueue::trypop(FXptr& obj,FXTime nsec){
  if(used.wait(nsec)){
    queue.pop(obj);
    free.post();
    return true;
    }
  return false;
  }


// Pop onject from queue
FXbool FXSemaQueue::pop(){
  used.wait();
  queue.pop();
  free.post();
  return true;
  }


// Delete queue
FXSemaQueue::~FXSemaQueue(){
  }

}
