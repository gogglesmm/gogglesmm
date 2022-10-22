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
#include "xincs.h"
#include "fxdefs.h"
#include "fxmath.h"
#include "FXElement.h"
#include "FXArray.h"
#include "FXPtrList.h"
#include "FXAtomic.h"
#include "FXWSQueue.h"

/*
  Notes:
   - A SPMC (Single Producer, Multiple Consumer) queue that minimizes contention
     of shared state.
   - Only a single thread may push() and pop() at the front of the queue, but multiple
     threads may take() (steal) items of the back of the queue.
   - It is lock-free in that no operating-system calls are used; this solution requires
     atomic operations however.
   - This is a version of the Chase-Lev work-stealing queue.
*/

using namespace FX;

/*******************************************************************************/

namespace FX {


// Create a queue and set its size to sz
FXWSQueue::FXWSQueue(FXint sz):top(0),bot(0){
  setSize(sz);
  }


// Change size of the queue
FXbool FXWSQueue::setSize(FXint sz){
  if(sz&(sz-1)){ fxerror("FXWSQueue::setSize: bad argument: %d.\n",sz); }
  if(list.no(sz)){
    top=bot=0;
    return true;
    }
  return false;
  }


// Return used slots
FXint FXWSQueue::getUsed() const {
  return bot-top;
  }


// Return free slots
FXint FXWSQueue::getFree() const {
  return getSize()-bot+top;
  }


// Check if queue is full
FXbool FXWSQueue::isFull() const {
  return (bot-top)>=getSize();
  }


// Check if queue is empty
FXbool FXWSQueue::isEmpty() const {
  return (bot-top)<=0;
  }


// Push task
FXbool FXWSQueue::push(FXptr ptr){
  FXint mask=getSize()-1;
  FXint t=top;
  if(__likely((bot-t)<mask)){
    list[bot&mask]=ptr;
    bot++;
    atomicThreadFence();
    return true;
    }
  return false;
  }


// Pop task
FXbool FXWSQueue::pop(FXptr& ptr){
  FXint mask=getSize()-1;
  FXint b=bot-1;
  bot=b;
  atomicThreadFence();
  FXival t=top;
  if(t<=b){
    ptr=list[b&mask];
    if(t==b){
      if(!atomicBoolCas(&top,t,t+1)){
        ptr=nullptr;
        bot=b+1;
        atomicThreadFence();
        return false;
        }
      bot=b+1;
      atomicThreadFence();
      }
    return true;
    }
  ptr=nullptr;
  bot=b+1;
  atomicThreadFence();
  return false;
  }


// Take (steal) task
FXbool FXWSQueue::take(FXptr& ptr){
  FXint mask=getSize()-1;
  FXint t=top;
  atomicThreadFence();
  FXint b=bot;
  if(__likely(t<b)){
    ptr=list[t&mask];
    if(atomicBoolCas(&top,t,t+1)){
      return true;
      }
    }
  ptr=nullptr;
  return false;
  }


// Delete queue
FXWSQueue::~FXWSQueue(){
  }

}
