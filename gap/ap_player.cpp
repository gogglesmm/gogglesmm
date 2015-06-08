/*******************************************************************************
*                         Goggles Audio Player Library                         *
********************************************************************************
*           Copyright (C) 2010-2015 by Sander Jansen. All Rights Reserved      *
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
#include "ap_pipe.h"
#include "ap_format.h"
#include "ap_device.h"
#include "ap_event.h"
#include "ap_reactor.h"
#include "ap_event_private.h"
#include "ap_event_queue.h"
#include "ap_thread_queue.h"
#include "ap_thread.h"
#include "ap_buffer.h"
#include "ap_packet.h"
#include "ap_input_thread.h"
#include "ap_decoder_thread.h"
#include "ap_output_thread.h"
#include "ap_engine.h"
#include "ap_player.h"

using namespace ap;

namespace ap {


FXIMPLEMENT(AudioPlayer,FXObject,nullptr,0);

AudioPlayer::AudioPlayer() : engine(nullptr) {
  engine = new AudioEngine();
  }

AudioPlayer::~AudioPlayer() {
  delete engine;
  }

void AudioPlayer::setEventQueue(EventQueue* fifo) {
  engine->fifo = fifo;
  }

FXbool AudioPlayer::init() {
  FXASSERT(engine->fifo);

  if (engine->fifo==nullptr)
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
  /// EventQueue::Flush => pending commands should not be executed
  engine->input->post(new ControlEvent(flush ? Ctrl_Open_Flush : Ctrl_Open,url),EventQueue::Flush);
  }

void AudioPlayer::close() {
  FXASSERT(engine->input->running());
  /// EventQueue::Flush => pending commands should not be executed
  engine->input->post(new ControlEvent(Ctrl_Close),EventQueue::Flush);
  }

void AudioPlayer::seek(FXdouble pos) {
  FXASSERT(engine->input->running());
  engine->input->post(new CtrlSeekEvent(pos));
  }

void AudioPlayer::pause() {
  FXASSERT(engine->input->running());
  engine->output->post(new ControlEvent(Ctrl_Pause),EventQueue::Front);
  }

void AudioPlayer::volume(FXfloat vol) {
  FXASSERT(engine->output->running());
  engine->output->post(new CtrlVolumeEvent(vol),EventQueue::Front);
  }


void AudioPlayer::getOutputConfig(OutputConfig & config) const{
  FXASSERT(engine->output->running());
  GetOutputConfig event;
  engine->output->post(&event,EventQueue::Front);
  if (event.waitForUnref()) {
    config=event.config;
    }
  }

void AudioPlayer::setOutputConfig(const OutputConfig & config) {
  FXASSERT(engine->output->running());
  engine->output->post(new SetOutputConfig(config),EventQueue::Front);
  }


ReplayGainMode AudioPlayer::getReplayGain() const {
  FXASSERT(engine->output->running());
  GetReplayGain event;
  engine->output->post(&event,EventQueue::Front);
  if (event.waitForUnref()) {
    return event.mode;
    }
  return ReplayGainOff;
  }

void AudioPlayer::setReplayGain(ReplayGainMode mode) {
  FXASSERT(engine->output->running());
  engine->output->post(new SetReplayGain(mode),EventQueue::Front);
  }

}
