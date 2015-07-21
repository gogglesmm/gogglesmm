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
#include "ap_event.h"
#include "ap_event_queue.h"
#include "ap_app_queue.h"

namespace ap {

FXAppQueue::FXAppQueue() : channel(nullptr),target(nullptr),message(0) {
  }

FXAppQueue::FXAppQueue(FXApp * app,FXObject * tgt,FXSelector sel) : target(tgt),message(sel) {
  channel=new FXMessageChannel(app);
  }

FXAppQueue::~FXAppQueue() {
  flush();
  FXASSERT(head==nullptr);
  FXASSERT(tail==nullptr);
  delete channel;
  }

void FXAppQueue::post(Event*event,FXint where) {
  mfifo.lock();
  if (where==Back) {
    if (tail) tail->next = event;
    event->next=nullptr;
    tail = event;
    if (head==nullptr) {
      head=tail;
      mfifo.unlock();
      channel->message(target,FXSEL(SEL_CHANGED,message),nullptr);
      return;
      }
    }
  else {
    event->next=head;
    head=event;
    if (tail==nullptr) {
      tail=head;
      mfifo.unlock();
      channel->message(target,FXSEL(SEL_CHANGED,message),nullptr);
      return;
      }
    }
  mfifo.unlock();
  }

Event * FXAppQueue::pop() {
  Event * event=nullptr;
  mfifo.lock();
//  event = atomicSet(head,head->next);
  if (head) {
    event=head;
    head=head->next;
    event->next=nullptr;
    if (head==nullptr) tail=nullptr;
    }
  mfifo.unlock();
  return event;
  }

void FXAppQueue::flush() {
  mfifo.lock();
  Event * h = head;
  head=tail=nullptr;
  mfifo.unlock();

  while(h) {
    Event * event = h;
    h = h->next;
    event->next = nullptr;
    Event::unref(event);
    }
  }
}
