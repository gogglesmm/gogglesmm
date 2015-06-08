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
#include "ap_defs.h"
#include "ap_pipe.h"
#include "ap_event.h"
#include "ap_event_queue.h"
#include "ap_thread_queue.h"

namespace ap {

ThreadQueue::ThreadQueue() : EventQueue() {
  }

ThreadQueue::~ThreadQueue() {
  FXASSERT(head==nullptr);
  FXASSERT(tail==nullptr);
  }

FXbool ThreadQueue::init() {
  return pfifo.create();
  }

void ThreadQueue::free() {
  flush();
  FXASSERT(head==nullptr);
  FXASSERT(tail==nullptr);
  pfifo.close();
  }

FXInputHandle ThreadQueue::handle() const {
  return pfifo.handle();
  }

void ThreadQueue::post(Event*event,FXint where) {
  if (where==Flush) {
    mfifo.lock();
      Event * h = head;
      event->next=nullptr;
      head = tail = event;
      pfifo.signal();
    mfifo.unlock();

    /// cleanup outside the fifo lock!
    while(h) {
      event = h;
      h = h->next;
      event->next = nullptr;
      Event::unref(event);
      }
    }
  else if (where==Back) {
    mfifo.lock();

    if (tail) tail->next = event;
    event->next=nullptr;
    tail = event;
    if (head==nullptr) {
      head=tail;
      pfifo.signal();
      }
    mfifo.unlock();
    }
  else {
    //fxmessage("posting event\n");
    mfifo.lock();
    event->next=head;
    head=event;
    pfifo.signal();
    if (tail==nullptr) {
      tail=head;
      }
    mfifo.unlock();
    }
  }

Event * ThreadQueue::pop() {
  Event * event=nullptr;
  mfifo.lock();
  if (head) {
    event=head;
    head=head->next;
    event->next=nullptr;
    if (head==nullptr) {
      tail=nullptr;
      pfifo.clear();
      }
    }
  else {
    pfifo.clear();
    }
  mfifo.unlock();
  return event;
  }

FXbool ThreadQueue::checkAbort() {
  FXScopedMutex lock(mfifo);
  if (head && (head->type&0x80))
    return true;

  pfifo.clear();
  return false;
  }


void ThreadQueue::flush() {
  Event * event = nullptr;
  mfifo.lock();
  Event * h = head;
  head=tail=nullptr;
  pfifo.clear();
  mfifo.unlock();
  while(h) {
    event = h;
    h = h->next;
    event->next = nullptr;
    Event::unref(event);
    }
  }

FXuchar ThreadQueue::peek() {
  FXuchar type=AP_INVALID;
  mfifo.lock();
  pfifo.clear();

  if (head)
    type=head->type;

  mfifo.unlock();
  return type;
  }

/// Pop typed event
Event * ThreadQueue::pop_if(FXuchar requested,FXbool & other){
  Event * event = nullptr;
  mfifo.lock();
  pfifo.clear();
  if (head) {
    if (head->type==requested) {
      event = head;
      head = head->next;
      event->next = nullptr;
      if (head==nullptr) tail=nullptr;
      other=false;
      }
    else {
      other=true;
      }
    }
  mfifo.unlock();
  return event;
  }

/// Pop typed event
Event * ThreadQueue::pop_if_not(FXuchar r1,FXuchar r2){
  Event * event = nullptr;
  mfifo.lock();
  pfifo.clear();
  if (head) {
    if ((head->type!=r1) && (head->type!=r2)) {
      event = head;
      head = head->next;
      event->next = nullptr;
      if (head==nullptr) tail=nullptr;
      }
    }
  mfifo.unlock();
  return event;
  }

}
