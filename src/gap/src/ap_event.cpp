#include "ap_defs.h"
#include "ap_event.h"
#include "ap_format.h"
#include "ap_device.h"
#include "ap_event_private.h"

namespace ap {

Event::Event() : next(NULL), type(AP_INVALID),stream(0) {
  }

Event::Event(FXuchar t) : next(NULL), type(t),stream(0) {
  FXASSERT(type!=AP_INVALID);
  }

Event::~Event() {
  next=NULL;
  }

void Event::unref() {
  delete this;
  }

void Event::unref(Event*& event) {
  event->unref();
  event=NULL;
  }



FlushEvent::FlushEvent(FXbool c) : Event(Flush), close(c) {
  }




ControlEvent::ControlEvent(FXuchar t,const FXString & msg) : Event(t),text(msg) {
  }

ControlEvent::ControlEvent(FXuchar t,FXuint id) : Event(t) {
  stream=id;
  }


ControlEvent::~ControlEvent() {
  }

ErrorMessage::ErrorMessage(const FXString & text) : Event(AP_ERROR),msg(text) {
  }
ErrorMessage::~ErrorMessage() {
  }

TimeUpdate::TimeUpdate(FXuint p,FXuint l) : Event(AP_TIMESTAMP), position(p), length(l) {
  }
TimeUpdate::~TimeUpdate() {
  }


CtrlSeekEvent::CtrlSeekEvent(FXdouble p) : Event(Ctrl_Seek), pos(p) {
  }

CtrlSeekEvent::~CtrlSeekEvent() {
  }


CtrlVolumeEvent::CtrlVolumeEvent(FXfloat v) : Event(Ctrl_Volume), vol(v) {
  }

CtrlVolumeEvent::~CtrlVolumeEvent() {
  }



ConfigureEvent::ConfigureEvent(const AudioFormat & fmt,FXuchar c,FXint n) : Event(Configure),
  af(fmt),
  codec(c),
  stream_length(n),
  data(NULL),
  stream_offset_start(0),
  stream_offset_end(0){
  }

ConfigureEvent::~ConfigureEvent(){
  }

}
