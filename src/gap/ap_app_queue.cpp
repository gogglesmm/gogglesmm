/*******************************************************************************
*                         Goggles Audio Player Library                         *
********************************************************************************
*           Copyright (C) 2010-2016 by Sander Jansen. All Rights Reserved      *
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

FXAppQueue::FXAppQueue() : channel(NULL),target(NULL),message(0) {
  }

FXAppQueue::FXAppQueue(FXApp * app,FXObject * tgt,FXSelector sel) : target(tgt),message(sel) {
  channel=new FXMessageChannel(app);
  }

FXAppQueue::~FXAppQueue() {
  flush();
  FXASSERT(head==NULL);
  FXASSERT(tail==NULL);
  delete channel;
  }

void FXAppQueue::post(Event*event,FXint where) {
  mfifo.lock();
  if (where==Back) {
    if (tail) tail->next = event;
    event->next=NULL;
    tail = event;
    if (head==NULL) {
      head=tail;
      mfifo.unlock();
      channel->message(target,FXSEL(SEL_CHANGED,message),NULL);
      return;
      }
    }
  else {
    event->next=head;
    head=event;
    if (tail==NULL) {
      tail=head;
      mfifo.unlock();
      channel->message(target,FXSEL(SEL_CHANGED,message),NULL);
      return;
      }
    }
  mfifo.unlock();
  }

Event * FXAppQueue::pop() {
  Event * event=NULL;
  mfifo.lock();
//  event = atomicSet(head,head->next);
  if (head) {
    event=head;
    head=head->next;
    event->next=NULL;
    if (head==NULL) tail=NULL;
    }
  mfifo.unlock();
  return event;
  }

void FXAppQueue::flush() {
  mfifo.lock();
  Event * event = NULL;
  Event * h = head;
  head=tail=NULL;
  mfifo.unlock();

  while(h) {
    event = h;
    h = h->next;
    event->next = NULL;
    Event::unref(event);
    }
  }
}
