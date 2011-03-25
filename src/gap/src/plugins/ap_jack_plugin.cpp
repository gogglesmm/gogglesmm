#include "ap_defs.h"
#include "ap_config.h"
#include "ap_pipe.h"
#include "ap_event.h"
#include "ap_format.h"
#include "ap_device.h"
#include "ap_memory_buffer.h"
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


extern "C" GMAPI OutputPlugin * ap_load_plugin() {
  return new JackOutput();
  }

extern "C" GMAPI void ap_free_plugin(OutputPlugin* plugin) {
  delete plugin;
  }

namespace ap {

JackOutput::JackOutput() : OutputPlugin() {
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


void JackOutput::volume(FXfloat v) {
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

FXbool JackOutput::configure(const AudioFormat & fmt){
//  af.format = AP_FORMAT_FLOAT;
//  af.rate   = jack_get_sample_rate(jack);
  return false;
  }


FXbool JackOutput::write(const void * b,FXuint nframes){
  return false;
  }
}

