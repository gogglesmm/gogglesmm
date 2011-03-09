#include "ap_defs.h"
#include "ap_event.h"
#include "ap_event_queue.h"

EventQueue::EventQueue() : head(NULL), tail(NULL) {
  }

FXbool EventQueue::init() {
  return false;
  }

void EventQueue::free() {
   }

EventQueue::~EventQueue() {
  }
