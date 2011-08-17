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
