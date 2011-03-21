#include "ap_defs.h"
#include "ap_pipe.h"
#include "ap_format.h"
#include "ap_event.h"
#include "ap_event_private.h"
#include "ap_event_queue.h"
#include "ap_thread_queue.h"
#include "ap_thread.h"
#include "ap_memory_buffer.h"
#include "ap_packet.h"
#include "ap_input_thread.h"
#include "ap_decoder_thread.h"
#include "ap_output_thread.h"
#include "ap_engine.h"
#include "ap_player.h"

using namespace ap;

namespace ap {

const FXuchar version[2]={AP_MAJOR,AP_MINOR};

FXIMPLEMENT(AudioPlayer,FXObject,NULL,0);

AudioPlayer::AudioPlayer(EventQueue*fifo) : engine(NULL) {
  engine = new AudioEngine();
  engine->fifo = fifo;
  }

AudioPlayer::~AudioPlayer() {
  delete engine;
  }

void AudioPlayer::setEventQueue(EventQueue* fifo) {
  engine->fifo = fifo;
  }

FXbool AudioPlayer::init() {
  FXASSERT(engine->fifo);

  if (engine->fifo==NULL)
    return false;

  return engine->init();
  }

void AudioPlayer::exit() {
  return engine->exit();
  }

Event* AudioPlayer::pop() {
  FXASSERT(engine->fifo);
  return engine->fifo->pop();
  }

void AudioPlayer::open(const FXString & url,FXbool flush) {
  FXASSERT(engine->input->running());
  engine->input->post(new ControlEvent(flush ? Ctrl_Open_Flush : Ctrl_Open,url));
  }

void AudioPlayer::pause() {
  FXASSERT(engine->input->running());
  engine->output->post(new ControlEvent(Ctrl_Pause),EventQueue::Front);
  }

void AudioPlayer::seek(FXdouble pos) {
  FXASSERT(engine->input->running());
  engine->input->post(new CtrlSeekEvent(pos),EventQueue::Front);
  }

void AudioPlayer::volume(FXfloat vol) {
  FXASSERT(engine->output->running());
  engine->output->post(new CtrlVolumeEvent(vol),EventQueue::Front);
  }

void AudioPlayer::close() {
  FXASSERT(engine->input->running());
  engine->input->post(new ControlEvent(Ctrl_Close),EventQueue::Front);
  }

void AudioPlayer::setOutputPlugin(const FXString & plugin) {
  FXASSERT(engine->input->running());
  engine->output->post(new ControlEvent(Ctrl_Output_Plugin,plugin),EventQueue::Front);
  }

FXString AudioPlayer::getOutputPlugin() const {
  return engine->output->getOutputPlugin();
  }

}
