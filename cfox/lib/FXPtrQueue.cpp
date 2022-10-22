/********************************************************************************
*                                                                               *
*                         Q u e u e   O f   P o i n t e r s                     *
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
#include "fxmath.h"
#include "FXElement.h"
#include "FXPtrList.h"
#include "FXPtrQueue.h"


/*
  Notes:
  - Fixed-sized, circular buffer of pointers to stuff.
  - A building block for all sorts of useful things.
  - The head and tail pointer can assume arbitrary values,
    with constraint that 0 <= head-tail <= N.
  - Access slots by taking modulo of head or tail pointers.
  - Thus all N (not N-1) slots can be filled.
*/

using namespace FX;

/*******************************************************************************/

namespace FX {


// Create initially empty queue
FXPtrQueue::FXPtrQueue():head(0),tail(0){
  }


// Create queue with initial size
FXPtrQueue::FXPtrQueue(FXival sz):list((FXptr)nullptr,sz),head(0),tail(0){
  }


// Change size of queue; return true if success
FXbool FXPtrQueue::setSize(FXival sz){
  if(list.no(sz)){
    head=0;
    tail=0;
    return true;
    }
  return false;
  }


// Return used slots
FXival FXPtrQueue::getUsed() const {
  return head-tail;
  }


// Return free slots
FXival FXPtrQueue::getFree() const {
  return getSize()+tail-head;
  }


// Check if queue is full
FXbool FXPtrQueue::isFull() const {
  return (head-tail)>=getSize();
  }


// Check if queue is empty
FXbool FXPtrQueue::isEmpty() const {
  return (head-tail)<=0;
  }


// Peek for item
FXbool FXPtrQueue::peek(FXptr& ptr){
  if(__likely((head-tail)>0)){
    ptr=list[tail%getSize()];
    return true;
    }
  return false;
  }


// Add item to queue, return true if success
FXbool FXPtrQueue::push(FXptr ptr){
  if(__likely((head-tail)<getSize())){
    list[head%getSize()]=ptr;
    head++;
    return true;
    }
  return false;
  }


// Remove item from queue, return true if success
FXbool FXPtrQueue::pop(FXptr& ptr){
  if(__likely((head-tail)>0)){
    ptr=list[tail%getSize()];
    tail++;
    return true;
    }
  return false;
  }


// Pop onject from queue
FXbool FXPtrQueue::pop(){
  if(__likely((head-tail)>0)){
    tail++;
    return true;
    }
  return false;
  }


// Destroy job queue
FXPtrQueue::~FXPtrQueue(){
  }

}
