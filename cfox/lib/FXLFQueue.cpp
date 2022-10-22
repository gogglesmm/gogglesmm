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
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "fxmath.h"
#include "FXElement.h"
#include "FXArray.h"
#include "FXPtrList.h"
#include "FXAtomic.h"
#include "FXLFQueue.h"

/*
  Notes:
  - Core idea: reserve a slot, write it (or read it), then commit it.
  - Only works with atomic memory operations; in this case we need compare-and-swap
    on integer data types.
  - Multiple threads can read from the queue, and multiple threads can write to
    the queue.
  - Push may fail if queue is full; pop may fail if queue is empty.
*/


using namespace FX;

/*******************************************************************************/

namespace FX {


// Create initially empty queue
FXLFQueue::FXLFQueue():whead(0),wtail(0),rhead(0),rtail(0){
  }


// Create queue with initial size, which must be a power of two
FXLFQueue::FXLFQueue(FXuint sz):whead(0),wtail(0),rhead(0),rtail(0){
  setSize(sz);
  }


// Change size of queue (must be power of two); return true if success
FXbool FXLFQueue::setSize(FXuint sz){
  if(sz&(sz-1)){ fxerror("FXLFQueue::setSize: bad argument: %u.\n",sz); }
  if(items.no(sz)){
    whead=wtail=rhead=rtail=0;
    return true;
    }
  return false;
  }


// Return used slots
FXuint FXLFQueue::getUsed() const {
  return whead-rtail;
  }


// Return free slots
FXuint FXLFQueue::getFree() const {
  return getSize()+rtail-whead;
  }


// If queue not full, can write if no other producers
FXbool FXLFQueue::isFull() const {
  return (whead-rtail)>=getSize();
  }


// If queue not empty, can read if no other consumers
FXbool FXLFQueue::isEmpty() const {
  return (whead-rtail)<=0;
  }


// Add item to queue, return true if success
FXbool FXLFQueue::push(FXptr ptr){
  FXuint mask=getSize()-1;
  FXuint w;
x:w=whead;
  if(__likely((w-rtail)<=mask)){
    if(__unlikely(!atomicBoolCas(&whead,w,w+1))) goto x;
    items[w&mask]=ptr;
    while(__unlikely(wtail!=w)){}
    wtail=w+1;
    return true;
    }
  return false;
  }


// Remove item from queue, return true if success
FXbool FXLFQueue::pop(FXptr& ptr){
  FXuint mask=getSize()-1;
  FXuint r;
x:r=rhead;
  if(__likely((wtail-r)>=1)){
    if(__unlikely(!atomicBoolCas(&rhead,r,r+1))) goto x;
    ptr=items[r&mask];
    while(__unlikely(rtail!=r)){}
    rtail=r+1;
    return true;
    }
  return false;
  }


// Destroy job queue
FXLFQueue::~FXLFQueue(){
  }

}
