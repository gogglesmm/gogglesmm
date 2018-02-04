/*******************************************************************************
*                         Goggles Audio Player Library                         *
********************************************************************************
*           Copyright (C) 2010-2018 by Sander Jansen. All Rights Reserved      *
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
#include "ap_event_private.h"

namespace ap {

Event::Event() : next(nullptr), type(AP_INVALID),stream(0) {
  }

Event::Event(FXuchar t) : next(nullptr), type(t),stream(0) {
  FXASSERT(type!=AP_INVALID);
  }

Event::~Event() {
  next=nullptr;
  }

void Event::unref() {
  delete this;
  }

void Event::unref(Event*& event) {
  event->unref();
  event=nullptr;
  }


FlushEvent::FlushEvent(FXlong o) : Event(Flush), offset(o), close(false) {
  }

FlushEvent::FlushEvent(FXbool c) : Event(Flush), offset(0), close(c) {
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

MetaInfo::MetaInfo() : Event(AP_META_INFO) {
  }

MetaInfo::~MetaInfo() {
  }

VolumeNotify::VolumeNotify() : Event(AP_VOLUME_NOTIFY) {
  }

VolumeNotify::VolumeNotify(FXfloat v) : Event(AP_VOLUME_NOTIFY),volume(v) {
  }

VolumeNotify::~VolumeNotify() {
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
  data(nullptr),
  stream_offset_start(0),
  stream_offset_end(0){
  }

ConfigureEvent::~ConfigureEvent(){
  delete dc;
  }

}
