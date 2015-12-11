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
#include "ap_config.h"
#include "ap_pipe.h"
#include "ap_event.h"
#include "ap_format.h"
#include "ap_device.h"
#include "ap_buffer.h"
#include "ap_packet.h"
#include "ap_event_queue.h"
#include "ap_thread_queue.h"
#include "ap_format.h"
#include "ap_engine.h"
#include "ap_thread.h"
#include "ap_input_plugin.h"
#include "ap_output_plugin.h"
#include "ap_decoder_plugin.h"
#include "ap_decoder_thread.h"
#include "ap_jack_plugin.h"

using namespace ap;


extern "C" GMAPI OutputPlugin * ap_load_plugin(OutputThread * output) {
  return new JackOutput(output);
  }

extern "C" GMAPI void ap_free_plugin(OutputPlugin* plugin) {
  delete plugin;
  }

FXuint GMAPI ap_version = AP_VERSION(APPLICATION_MAJOR,APPLICATION_MINOR,APPLICATION_LEVEL);


namespace ap {

JackOutput::JackOutput(OutputThread * output) : OutputPlugin(output) {
  }

JackOutput::~JackOutput() {
  close();
  }

FXbool JackOutput::open() {
  jack = jack_client_open("gap",JackNoStartServer,NULL);
  if (jack==NULL) {
    return false;
    }
  return false;
  }

void JackOutput::close() {
  if (jack) {
    jack_client_close(jack);
    jack=NULL;
    }
  af.reset();
  }


void JackOutput::volume(FXfloat) {
  }

FXint JackOutput::delay() {
  return 0;
  }

void JackOutput::drop() {
  }

void JackOutput::drain() {
  }

void JackOutput::pause(FXbool) {
  }

FXbool JackOutput::configure(const AudioFormat &){
//  af.format = AP_FORMAT_FLOAT;
//  af.rate   = jack_get_sample_rate(jack);
  return false;
  }


FXbool JackOutput::write(const void *,FXuint){
  return false;
  }
}

