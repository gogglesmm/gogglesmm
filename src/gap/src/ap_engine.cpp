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

AudioEngine::AudioEngine() : fifo(NULL) {
  input   = new InputThread(this);
  decoder = new DecoderThread(this);
  output  = new OutputThread(this);
  }

AudioEngine::~AudioEngine() {
  delete input;
  delete decoder;
  delete output;
  delete fifo;
  }

FXbool AudioEngine::init() {
  if (!input->running()) {

    if (!input->init()) {
      return false;
      }

    if (!decoder->init()) {
      input->free();
      return false;
      }

    if (!output->init()) {
      input->free();
      decoder->free();
      return false;
      }

    input->start();
    input->policy(FXThread::PolicyFifo);
    input->priority(FXThread::PriorityMinimum);

    decoder->start();
    decoder->policy(FXThread::PolicyFifo);
    decoder->priority(FXThread::PriorityMinimum);

    output->start();
    output->policy(FXThread::PolicyFifo);
    output->priority(FXThread::PriorityMinimum);
    }
  return true;
  }


void AudioEngine::exit() {
  if (input->running()) {

    GM_DEBUG_PRINT("AudioEngine::exit()\n");

    input->post(new ControlEvent(Ctrl_Quit),EventQueue::Front);

    GM_DEBUG_PRINT("Waiting for input\n");
    input->join();
    GM_DEBUG_PRINT("Waiting for decoder\n");
    decoder->join();
    GM_DEBUG_PRINT("Waiting for output\n");
    output->join();
    GM_DEBUG_PRINT("All Joined. Freeing data\n");
    input->free();
    decoder->free();
    output->free();
    }
  }

void AudioEngine::post(Event * event){
  FXASSERT(fifo);
  if (fifo) fifo->post(event);
  }
