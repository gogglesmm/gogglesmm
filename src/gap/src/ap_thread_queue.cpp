#include "ap_defs.h"
#include "ap_pipe.h"
#include "ap_event.h"
#include "ap_event_queue.h"
#include "ap_thread_queue.h"

namespace ap {

ThreadQueue::ThreadQueue() : EventQueue() {
  }

ThreadQueue::~ThreadQueue() {
  FXASSERT(head==NULL);
  FXASSERT(tail==NULL);
  }

FXbool ThreadQueue::init() {
  return pfifo.create();
  }

void ThreadQueue::free() {
  flush();
  FXASSERT(head==NULL);
  FXASSERT(tail==NULL);
  pfifo.close();
  }

FXInputHandle ThreadQueue::handle() const {
  return pfifo.handle();
  }

void ThreadQueue::post(Event*event,FXint where) {
  mfifo.lock();
  if (where==Flush) {
    while(head) {
      Event * ev = head;
      head = head->next;
      ev->next = NULL;
      Event::unref(ev);
      }
    event->next=NULL;
    head = tail = event;
    pfifo.signal();
    }
  else if (where==Back) {
    if (tail) tail->next = event;
    event->next=NULL;
    tail = event;
    if (head==NULL) {
      head=tail;
      pfifo.signal();
      }
    }
  else {
    event->next=head;
    head=event;
    pfifo.signal();
    if (tail==NULL) {
      tail=head;
      }
    }
  mfifo.unlock();
  }

Event * ThreadQueue::pop() {
  Event * event=NULL;
  mfifo.lock();
  if (head) {
    event=head;
    head=head->next;
    event->next=NULL;
    if (head==NULL) {
      tail=NULL;
      pfifo.clear();
      }
    }
  else {
    pfifo.clear();
    }
  mfifo.unlock();
  return event;
  }

void ThreadQueue::flush() {
  Event * event = NULL;
  mfifo.lock();
  Event * h = head;
  head=tail=NULL;
  pfifo.clear();  
  mfifo.unlock();
  while(h) {
    event = h;
    h = h->next;
    event->next = NULL;
    Event::unref(event);
    }
  }

FXuchar ThreadQueue::peek() {
  FXuchar type=AP_INVALID;
  mfifo.lock();
  pfifo.clear();

  if (head)
    type=head->type;
//  else
//    ap_pipe_clear(pfifo[0]);

  mfifo.unlock();
  return type;
  }

/// Pop typed event
Event * ThreadQueue::pop_if(FXuchar requested,FXbool & other){
  Event * event = NULL;
  mfifo.lock();
  pfifo.clear();
  if (head) {
    if (head->type==requested) {
      event = head;
      head = head->next;
      event->next = NULL;
      if (head==NULL) tail=NULL;
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
  Event * event = NULL;
  mfifo.lock();
  pfifo.clear();
  if (head) {
    if ((head->type!=r1) && (head->type!=r2)) {
      event = head;
      head = head->next;
      event->next = NULL;
      if (head==NULL) tail=NULL;
      }
    }
  mfifo.unlock();
  return event;
  }

}
