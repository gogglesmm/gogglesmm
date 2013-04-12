/*******************************************************************************
*                         Goggles Audio Player Library                         *
********************************************************************************
*           Copyright (C) 2010-2012 by Sander Jansen. All Rights Reserved      *
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
#include "ap_utils.h"
#include "ap_pipe.h"
#include "ap_event.h"
#include "ap_format.h"
#include "ap_device.h"
#include "ap_event_private.h"
#include "ap_event_queue.h"
#include "ap_thread_queue.h"
#include "ap_buffer.h"
#include "ap_packet.h"
#include "ap_engine.h"
#include "ap_thread.h"
#include "ap_thread_queue.h"
#include "ap_decoder_plugin.h"
#include "ap_decoder_thread.h"
#include "ap_input_thread.h"
#include "ap_output_thread.h"

namespace ap {

DecoderThread::DecoderThread(AudioEngine*e) : EngineThread(e),plugin(NULL) {
  }

DecoderThread::~DecoderThread() {
  }

FXbool DecoderThread::init() {

  if (!EngineThread::init())
    return false;

  if (!packetpool.init(8192,40))
    return false;

  return true;
  }

void DecoderThread::free() {
  packetpool.free();
  EngineThread::free();
  }



void DecoderThread::configure(ConfigureEvent * event) {
  if (plugin) {
    if (plugin->codec() == event->codec) {
      plugin->init(event);
      goto forward;
      }
    delete plugin;
    plugin=NULL;
    }
  plugin = DecoderPlugin::open(engine,event->codec);
  if (plugin) {
    plugin->init(event);
    }
  else {
    engine->input->post(new ControlEvent(Ctrl_Close));
    engine->post(new ErrorMessage(FXString::value("No decoder plugin available for %s",Codec::name(event->codec))));
    event->unref();
    return;
    }

forward:
  /// Forward to output
  if (!event->af.undefined()) {
    engine->output->post(event);
    }
  else {
    event->unref();
    }
  }



FXint DecoderThread::run(){
  Event * event=NULL;

  ap_set_thread_name("ap_decoder");

  for(;;) {
    event = wait_for_packet();
    switch(event->type) {
      case Flush    : GM_DEBUG_PRINT("[decoder] flush\n");
                      if (plugin) plugin->flush();
                      engine->output->post(event,EventQueue::Flush);
                      continue;
                      break;

      case Ctrl_Quit: GM_DEBUG_PRINT("[decoder] quit\n");
                      if (plugin) {
                        delete plugin;
                        plugin=NULL;
                        }
                      /// forward to output thread
                      engine->output->post(event,EventQueue::Flush);
                      return 0;
                      break;

      case Configure: configure(dynamic_cast<ConfigureEvent*>(event));
                      continue;
                      break;

      case End      :
      case Meta     : if (plugin) {
                        engine->output->post(event);
                        continue;
                        }
                      break;
      case Buffer   : if (plugin) {
                        stream=event->stream;
                        switch(plugin->process(dynamic_cast<Packet*>(event))){
                          case DecoderError:
                                                           delete plugin;
                                                           plugin=NULL;
                                                           fxmessage("[decoder] error\n");
                                                           engine->input->post(new ControlEvent(Ctrl_Close));
                                                           engine->post(new ErrorMessage("Fatal decoder error"));
                                                           break;
                          default                        : break;
                          }
                        continue;
                        }
                      break;
      }
    Event::unref(event);
    }
  return 0;
  }


Event * DecoderThread::wait_for_packet() {
  Event * event = fifo.pop();
  if (event==NULL) {
    ap_wait(fifo.handle());
    event = fifo.pop();
    }
  FXASSERT(event);
  return event;
  }

Packet * DecoderThread::get_decoder_packet() {
  Event * event = NULL;
  FXbool other=false;
  do {
    event = fifo.pop_if(Buffer,other);
    if (event)  {
      FXASSERT(event->next==NULL);
      return dynamic_cast<Packet*>(event);
      }

    if (other) return NULL;
    ap_wait(fifo.handle());

/*
    type = fifo.peek();

    if (type==Ctrl_Quit || type==Configure || type==Flush) {
      return NULL;
      }

    if (type!=Invalid) {
      event = fifo.pop();
      if (event) return event;
      }
    ap_wait(fifo.handle());
    */
    }
  while(1);
  return NULL;
  }

Packet * DecoderThread::get_output_packet() {
//  Event * event = NULL;
  FXuchar type;
  do {
    type = fifo.peek();

    if (type!=Buffer && type!=AP_INVALID)
      return NULL;

    Packet * packet = packetpool.pop();
    if (packet) {
      packet->stream=stream;
      return packet;
      }

/*
    event = OutputPacket::get();
    if (event) {
      event->stream=stream;
      return dynamic_cast<OutputPacket*>(event);
      }
*/
    ap_wait(packetpool.handle(),fifo.handle());
    }
  while(1);
  return NULL;
  }

}


