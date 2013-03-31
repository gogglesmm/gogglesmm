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
#include "ap_config.h"
#include "ap_event.h"
#include "ap_pipe.h"
#include "ap_event_queue.h"
#include "ap_thread_queue.h"
#include "ap_format.h"
#include "ap_device.h"
#include "ap_buffer.h"
#include "ap_packet.h"
#include "ap_engine.h"
#include "ap_thread.h"
#include "ap_input_plugin.h"
#include "ap_output_plugin.h"
#include "ap_decoder_plugin.h"
#include "ap_decoder_thread.h"
#include "ap_output_thread.h"

#include "ap_pulse_plugin.h"
#include <poll.h>


using namespace ap;


struct pa_io_event {
  pa_io_event_cb_t         callback;
  pa_io_event_destroy_cb_t destroy_callback;
  void *                   userdata;
  FXint                    fd;
  FXushort                 flags;
  };

struct pa_defer_event {
  pa_defer_event_cb_t         callback;
  pa_defer_event_destroy_cb_t destroy_callback;
  void*                       userdata;
  FXbool                      enabled;
  };

struct pa_time_event {
  pa_time_event*              next;  
  pa_time_event_cb_t          callback;
  pa_time_event_destroy_cb_t  destroy_callback;
  void*                       userdata;
  FXTime                      time;
  };


static short map_flags_to_libc(pa_io_event_flags_t flags) {
    return (short)
        ((flags & PA_IO_EVENT_INPUT ? POLLIN : 0) |
         (flags & PA_IO_EVENT_OUTPUT ? POLLOUT : 0) |
         (flags & PA_IO_EVENT_ERROR ? POLLERR : 0) |
         (flags & PA_IO_EVENT_HANGUP ? POLLHUP : 0));
    }

static pa_io_event_flags_t map_flags_from_libc(short flags) {
  int event=PA_IO_EVENT_NULL;

  if (flags&POLLIN)
    event=PA_IO_EVENT_INPUT;
  if (flags&POLLOUT)
    event|=PA_IO_EVENT_OUTPUT;
  if (flags&POLLERR)
    event|=PA_IO_EVENT_ERROR;
  if (flags&POLLHUP)
    event|=PA_IO_EVENT_HANGUP;
  return (pa_io_event_flags_t)event;

/*
    return (pa_io_event_flags_t)
        (flags & POLLIN ? (pa_io_event_flags_t)PA_IO_EVENT_INPUT : (pa_io_event_flags_t)PA_IO_EVENT_NULL) |
        (flags & POLLOUT ? (pa_io_event_flags_t)PA_IO_EVENT_OUTPUT : (pa_io_event_flags_t)PA_IO_EVENT_NULL) |
        (flags & POLLERR ? (pa_io_event_flags_t)PA_IO_EVENT_ERROR : (pa_io_event_flags_t)PA_IO_EVENT_NULL) |
        (flags & POLLHUP ? (pa_io_event_flags_t)PA_IO_EVENT_HANGUP : (pa_io_event_flags_t)PA_IO_EVENT_NULL);
*/
    }
  


class PulseEventLoop  {
protected:
  static PulseEventLoop * loop;
public: 
  pa_mainloop_api mapi;
public:
  FXPtrListOf<pa_io_event>    watches;
  FXPtrListOf<pa_defer_event> deferred;
  pa_time_event * timers;
public:
  static PulseEventLoop* instance();
public:
  PulseEventLoop();

  pa_mainloop_api * api() { return &mapi; }

  void addTimer(pa_time_event* t){
    pa_time_event**tt;
    for(tt=&timers; *tt && ((*tt)->time < t->time); tt=&(*tt)->next){}
    t->next=*tt;
    *tt=t;
    }

  void removeTimer(pa_time_event*event) {
    if (timers) {
      for (pa_time_event ** node = &timers; *node; node = &(*node)->next) {
        if ((*node)==event) {
          pa_time_event * next = (*node)->next;
          *node = next;
          return;
          }
        }
      }
    }


  FXbool handle_deferred() {
    FXbool handled = false;
    for (FXint d=0;d<deferred.no();d++) {
      if (deferred[d]->enabled && deferred[d]->callback) {
        fxmessage("deferred callback %p\n",deferred[d]);
        deferred[d]->callback(api(),deferred[d],deferred[d]->userdata);
        handled=true;
        }
      }
    return handled;
    }

  void run_once() {
    handle_deferred();
    }

  void wait() {

    if (handle_deferred())
      return;

    FXTime wait = -1;
    struct timespec ts;
    struct pollfd * pfd = NULL;
    FXint ntotal = watches.no();
    allocElms(pfd,ntotal);
    FXint i=0;
    
    for (FXint w=0;w<watches.no();w++,i++) {
      pfd[i].fd     = watches[w]->fd;
      pfd[i].events = watches[w]->flags;      
      }



    if (timers) {
      FXTime now = FXThread::time();
      pa_time_event * tt = timers;
      while(tt && tt->time<now) tt=tt->next;
      if (tt) wait = tt->time - now;
      }

    if (wait>0){
      ts.tv_sec = wait / 1000000000;
      ts.tv_nsec = wait % 1000000000;
      fxmessage("ppoll %ld\n",wait);
      int result = ppoll(pfd,i,&ts,NULL);
      fxmessage("result %d\n",result);
      if (result) {
        FXint i=0;
        for (FXint w=0;w<watches.no();w++,i++) {
          if (pfd[i].revents) {
            FXASSERT(pfd[i].fd == watches[w]->fd);
            watches[w]->callback(api(),watches[w],watches[w]->fd,map_flags_from_libc(pfd[i].revents),watches[w]->userdata);
            }
          }
        }
      }
    else if (i) {
      fxmessage("ppoll no timeout\n");
      int result = ppoll(pfd,i,NULL,NULL);
      if (result) {
        i=0;
        for (FXint w=0;w<watches.no();w++,i++) {
          if (pfd[i].revents) {
            FXASSERT(pfd[i].fd == watches[w]->fd);
            watches[w]->callback(api(),watches[w],watches[w]->fd,map_flags_from_libc(pfd[i].revents),watches[w]->userdata);
            }
          }
        }
      fxmessage("result %d\n",result);
      }
    else {
      fxmessage("nothing to do\n");
      }

    freeElms(pfd);
    }

  };






pa_io_event* pulse_io_new(pa_mainloop_api*api, int fd, pa_io_event_flags_t events, pa_io_event_cb_t cb, void *userdata){
  pa_io_event * event = new pa_io_event;
  event->callback         = cb;
  event->destroy_callback = NULL;
  event->userdata         = userdata;
  event->fd               = fd;
  event->flags            = map_flags_to_libc(events);
  GM_DEBUG_PRINT("pulse_io_new %p %d\n",event,fd);
  PulseEventLoop::instance()->watches.append(event);
  return event;
  }

void pulse_io_enable(pa_io_event* event, pa_io_event_flags_t events){
  GM_DEBUG_PRINT("pulse_io_enable %p %d %d\n",event,event->fd,(int)events);
  event->flags = map_flags_to_libc(events);
  }

void pulse_io_free(pa_io_event* event){
  GM_DEBUG_PRINT("pulse_io_free %p %d\n",event,event->fd);

  if (event->destroy_callback)
    event->destroy_callback(PulseEventLoop::instance()->api(),event,event->userdata);

  PulseEventLoop::instance()->watches.remove(event);
  delete event;
  }

void pulse_io_set_destroy(pa_io_event *event, pa_io_event_destroy_cb_t cb){
  GM_DEBUG_PRINT("pulse_io_set_destroy %p %d\n",event,event->fd);
  event->destroy_callback = cb;
  }




pa_defer_event* pulse_defer_new(pa_mainloop_api*api, pa_defer_event_cb_t cb, void *userdata){
  pa_defer_event* event = new pa_defer_event;
  event->callback         = cb;
  event->destroy_callback = NULL;
  event->enabled          = true;
  event->userdata         = userdata;
  GM_DEBUG_PRINT("pulse_defer_new %p\n",event);
  PulseEventLoop::instance()->deferred.append(event);
  return event;
  }

void pulse_defer_enable(pa_defer_event* event, int b){
  GM_DEBUG_PRINT("pulse_defer_enable %p %d\n",event,b);
  event->enabled = (b==1) ? true : false;
  }

void pulse_defer_free(pa_defer_event* event){
  GM_DEBUG_PRINT("pulse_defer_free %p\n",event);

  if (event->destroy_callback)
    event->destroy_callback(PulseEventLoop::instance()->api(),event,event->userdata);

  PulseEventLoop::instance()->deferred.remove(event);
  delete event;
  }

void pulse_defer_set_destroy(pa_defer_event *event, pa_defer_event_destroy_cb_t cb){
  GM_DEBUG_PRINT("pulse_defer_set_destroy %p\n",event);
  event->destroy_callback = cb;
  }



pa_time_event* pulse_time_new(pa_mainloop_api*a, const struct timeval *tv, pa_time_event_cb_t cb, void *userdata){
  pa_time_event * event = new pa_time_event;
  event->next             = NULL;
  event->callback         = cb;
  event->destroy_callback = NULL;
  event->userdata         = userdata;
  event->time             = (tv->tv_sec*1000000000) + (tv->tv_usec*1000);
  GM_DEBUG_PRINT("pulse_time_new %p %ld.%ld\n",event,tv->tv_sec,tv->tv_usec);
  PulseEventLoop::instance()->addTimer(event);
  return event;
  }

void pulse_time_restart(pa_time_event* event, const struct timeval *tv){
  event->time  = (tv->tv_sec*1000000000) + (tv->tv_usec*1000);
  GM_DEBUG_PRINT("pulse_time_restart %p %ld.%ld\n",event,tv->tv_sec,tv->tv_usec);
  }

void pulse_time_free(pa_time_event* event){
  GM_DEBUG_PRINT("pulse_time_free %p\n",event);
  if (event->destroy_callback)
    event->destroy_callback(PulseEventLoop::instance()->api(),event,event->userdata);
  PulseEventLoop::instance()->removeTimer(event);
  delete event;
  }

void pulse_time_set_destroy(pa_time_event *event, pa_time_event_destroy_cb_t cb){
  GM_DEBUG_PRINT("pulse_time_set_destroy %p\n",event);
  event->destroy_callback=cb;
  }



void pulse_quit(pa_mainloop_api*,int){  
  GM_DEBUG_PRINT("pulse_quit\n");
  }





PulseEventLoop* PulseEventLoop::loop = NULL;

PulseEventLoop* PulseEventLoop::instance() {
  return loop;
  }


PulseEventLoop::PulseEventLoop() : timers(NULL) {
  loop = this;
  mapi.userdata = this;
  mapi.io_new            = pulse_io_new;
  mapi.io_free           = pulse_io_free;
  mapi.io_enable         = pulse_io_enable;
  mapi.io_set_destroy    = pulse_io_set_destroy;
  mapi.defer_new         = pulse_defer_new;
  mapi.defer_free        = pulse_defer_free;
  mapi.defer_enable      = pulse_defer_enable;
  mapi.defer_set_destroy = pulse_defer_set_destroy;
  mapi.time_new          = pulse_time_new;
  mapi.time_restart      = pulse_time_restart;
  mapi.time_free         = pulse_time_free;
  mapi.time_set_destroy  = pulse_time_set_destroy;
  mapi.quit              = pulse_quit;
  }

  
























extern "C" GMAPI OutputPlugin * ap_load_plugin(OutputThread * output) {
  return new PulseOutput(output);
  }

extern "C" GMAPI void ap_free_plugin(OutputPlugin* plugin) {
  delete plugin;
  }



namespace ap {





PulseOutput::PulseOutput(OutputThread * thread) : OutputPlugin(thread),eventloop(NULL),context(NULL),stream(NULL) {
  }

PulseOutput::~PulseOutput() {
  close();
  }


void PulseOutput::ev_handle_pending(){
  GM_DEBUG_PRINT("EV_HANDLE_PENDING\n");
  eventloop->handle_deferred();
  }

FXint PulseOutput::ev_num_poll(){
  GM_DEBUG_PRINT("EV_NUM_POLL\n");
  FXint n=0;
  for (FXint i=0;i<eventloop->watches.no();i++) {
    if (eventloop->watches[i]->fd && eventloop->watches[i]->flags)
      n++;
    }
  return n;
  }

void PulseOutput::ev_prepare_poll(struct ::pollfd* pfd,FXint n,FXTime & wakeup){
  GM_DEBUG_PRINT("EV_PREPARE_POLL\n");

  for (FXint i=0,w=0;w<eventloop->watches.no();w++) {
    if (eventloop->watches[w]->fd && eventloop->watches[w]->flags){
      pfd[i].fd     = eventloop->watches[w]->fd;
      pfd[i].events = eventloop->watches[w]->flags;
      pfd[i].revents = 0;
      i++;
      }
    }

  if (eventloop->timers) {
    FXTime now = FXThread::time();
    pa_time_event * tt = eventloop->timers;
    while(tt && tt->time<now) tt=tt->next;
    if (tt) wakeup = tt->time - now;
    }

  if (wakeup<0) wakeup=0;
  }

void PulseOutput::ev_handle_poll(struct ::pollfd* pfd,FXint n,FXTime now){
  for (FXint w=0;w<eventloop->watches.no();w++) {
    if (pfd[w].revents) {
      GM_DEBUG_PRINT("EV_HANDLE_POLL\n");  
      FXASSERT(pfd[w].fd == eventloop->watches[w]->fd);
      eventloop->watches[w]->callback(eventloop->api(),eventloop->watches[w],eventloop->watches[w]->fd,map_flags_from_libc(pfd[w].revents),eventloop->watches[w]->userdata);
      pfd[w].revents=0;
      }
    }
  }
























static FXbool to_pulse_format(const AudioFormat & af,pa_sample_format & pulse_format){
  switch(af.format) {
    case AP_FORMAT_S16_LE   : pulse_format=PA_SAMPLE_S16LE; break;
    case AP_FORMAT_S16_BE   : pulse_format=PA_SAMPLE_S16BE; break;
    case AP_FORMAT_S24_LE   : pulse_format=PA_SAMPLE_S24_32LE;  break;
    case AP_FORMAT_S24_BE   : pulse_format=PA_SAMPLE_S24_32BE;  break;
    case AP_FORMAT_S24_3LE  : pulse_format=PA_SAMPLE_S24LE;  break;
    case AP_FORMAT_S24_3BE  : pulse_format=PA_SAMPLE_S24BE;  break;
    case AP_FORMAT_FLOAT_LE : pulse_format=PA_SAMPLE_FLOAT32LE;  break;
    case AP_FORMAT_FLOAT_BE : pulse_format=PA_SAMPLE_FLOAT32BE;  break;
    default                 : return false; break;
    }
  return true;
  }


static FXbool to_gap_format(pa_sample_format pulse_format,AudioFormat & af){
  switch(pulse_format) {
    case PA_SAMPLE_U8       : af.format = AP_FORMAT_U8;       break;
    case PA_SAMPLE_S16LE    : af.format = AP_FORMAT_S16_LE;   break;
    case PA_SAMPLE_S16BE    : af.format = AP_FORMAT_S16_BE;   break;
    case PA_SAMPLE_S24LE    : af.format = AP_FORMAT_S24_3LE;   break;
    case PA_SAMPLE_S24BE    : af.format = AP_FORMAT_S24_3BE;   break;
    case PA_SAMPLE_S24_32LE : af.format = AP_FORMAT_S24_LE;  break;
    case PA_SAMPLE_S24_32BE : af.format = AP_FORMAT_S24_BE;  break;
    case PA_SAMPLE_FLOAT32LE: af.format = AP_FORMAT_FLOAT_LE; break;
    case PA_SAMPLE_FLOAT32BE: af.format = AP_FORMAT_FLOAT_BE; break;
    default                 : return false;
    }
  return true;
  }

static void context_state_callback(pa_context *c,void*ptr){
  GM_DEBUG_PRINT("context_state_callback %d\n",pa_context_get_state(c));
  //pa_threaded_mainloop * mainloop = (pa_threaded_mainloop*)ptr;
  switch (pa_context_get_state(c)) {
    case PA_CONTEXT_READY:
    case PA_CONTEXT_TERMINATED:
    case PA_CONTEXT_FAILED:
      //pa_threaded_mainloop_signal(mainloop,0);
      break;
    default: break;
    }
  }

static void stream_state_callback(pa_stream *s,void*ptr){
  GM_DEBUG_PRINT("stream_state_callback %d\n",pa_stream_get_state(s));
  //pa_threaded_mainloop * mainloop = (pa_threaded_mainloop*)ptr;
  switch (pa_stream_get_state(s)) {
    case PA_STREAM_READY:
    case PA_STREAM_TERMINATED:
    case PA_STREAM_FAILED:
      //pa_threaded_mainloop_signal(mainloop, 0);
      break;
    default: break;
    }
  }

static void stream_write_callback(pa_stream*stream,size_t bs,void *ptr){
  PulseOutput * pulse = (PulseOutput*)ptr;
  if (pulse->af.framesize()==0) {
    //pa_stream_write(stream,NULL,0,NULL,0,PA_SEEK_RELATIVE);
    return;
    }

  const void * buffer = NULL;
  FXuint nframes = bs / pulse->af.framesize();
  GM_DEBUG_PRINT("stream_write_callback requested %ld\n",nframes);
  pulse->output->getSamples(buffer,nframes);
  GM_DEBUG_PRINT("stream_write_callback got %ld\n",nframes);

  if (buffer && nframes) //pulse->write(buffer,nframes);
    pa_stream_write(stream,(const FXchar*)buffer,nframes*pulse->af.framesize(),NULL,0,PA_SEEK_RELATIVE);




  





  //pa_threaded_mainloop * mainloop = (pa_threaded_mainloop*)ptr;
  //pa_threaded_mainloop_signal(mainloop,0);
  }



FXbool PulseOutput::open() {

  /// Start the mainloop
  if (eventloop==NULL) {
    eventloop = new PulseEventLoop();
    //mainloop = eventloop->api();

    //mainloop = pa_threaded_mainloop_new();
    //pa_threaded_mainloop_start(mainloop);

    

    }

  //pa_threaded_mainloop_lock(mainloop);

  /// Get a context
  if (context==NULL) {
    //context = pa_context_new(pa_threaded_mainloop_get_api(mainloop),"Goggles Music Manager");
    context = pa_context_new(eventloop->api(),"Goggles Music Manager");
    //pa_context_set_state_callback(context,context_state_callback,mainloop);
    pa_context_set_state_callback(context,context_state_callback,eventloop);
    }

  /// Try connecting
  GM_DEBUG_PRINT("pa_context_connect()\n");
  if (pa_context_get_state(context)==PA_CONTEXT_UNCONNECTED) {
    if (pa_context_connect(context,NULL,PA_CONTEXT_NOFLAGS,NULL)<0) {
      GM_DEBUG_PRINT("pa_context_connect failed\n");
      //pa_threaded_mainloop_unlock(mainloop);
      return false;
      }
    }

  /// Wait until we're connected to the pulse daemon
  GM_DEBUG_PRINT("wait for connection\n");
  pa_context_state_t state;
  while((state=pa_context_get_state(context))!=PA_CONTEXT_READY) {
    if (state==PA_CONTEXT_FAILED || state==PA_CONTEXT_TERMINATED){
      GM_DEBUG_PRINT("Unable to connect to pulsedaemon\n");
      //pa_threaded_mainloop_unlock(mainloop);
      return false;
      }
    //FXThread::sleep(10000000);
    eventloop->wait();
    //pa_threaded_mainloop_wait(mainloop);
    }

  GM_DEBUG_PRINT("ready()\n");
  //pa_threaded_mainloop_unlock(mainloop);
  return true;
  }

void PulseOutput::close() {
  //if (mainloop)
  //  pa_threaded_mainloop_lock(mainloop);
 // if (eventloop)
  //  delete eventloop;

  if (stream) {
    GM_DEBUG_PRINT("disconnecting stream\n");
    pa_stream_disconnect(stream);
    pa_stream_unref(stream);
    stream=NULL;
    }

  if (context) {
    GM_DEBUG_PRINT("disconnecting context\n");
    pa_context_disconnect(context);
    pa_context_unref(context);
    context=NULL;
    }

/*
  if (mainloop) {
    GM_DEBUG_PRINT("disconnecting mainloop\n");    
    pa_threaded_mainloop_unlock(mainloop);
    pa_threaded_mainloop_stop(mainloop);
    pa_threaded_mainloop_free(mainloop);
    mainloop=NULL;
    }
*/
  af.reset();
  }




void PulseOutput::volume(FXfloat v) {
//  if (mainloop && context && stream) {
  if (eventloop && context && stream) {
    //pa_threaded_mainloop_lock(mainloop);
    pa_cvolume cvol;
    pa_cvolume_set(&cvol,af.channels,pa_sw_volume_from_linear((FXdouble)v));
    pa_operation* operation = pa_context_set_sink_input_volume(context,pa_stream_get_index(stream),&cvol,NULL,NULL);
    pa_operation_unref(operation);
    //pa_threaded_mainloop_unlock(mainloop);
    }
  }

FXint PulseOutput::delay() {
  FXint value=0;
  if (stream) {
    pa_usec_t latency;
    int negative;
    //pa_threaded_mainloop_lock(mainloop);
    if (pa_stream_get_latency(stream,&latency,&negative)>=0){
      value = (latency*af.rate) / 1000000;
      }
    //pa_threaded_mainloop_unlock(mainloop);
    }
  return value;
  }

void PulseOutput::drop() {
  if (stream) {
    //pa_threaded_mainloop_lock(mainloop);
    pa_operation* operation = pa_stream_flush(stream,NULL,0);
    pa_operation_unref(operation);
    //pa_threaded_mainloop_unlock(mainloop);
    }
  }


static void drain_callback(pa_stream*,int,void *ptr) {
  //pa_threaded_mainloop * mainloop = (pa_threaded_mainloop*)ptr;
  //pa_threaded_mainloop_signal(mainloop,0);
  }


void PulseOutput::drain() {
  if (stream) {
    //pa_threaded_mainloop_lock(mainloop);
    //pa_operation * operation = pa_stream_drain(stream,drain_callback,mainloop);
    pa_operation * operation = pa_stream_drain(stream,drain_callback,eventloop);
    //while (pa_operation_get_state(operation) == PA_OPERATION_RUNNING)
    //  pa_threaded_mainloop_wait(mainloop);
    pa_operation_unref(operation);
    //pa_threaded_mainloop_unlock(mainloop);
    }
  }

void PulseOutput::pause(FXbool) {
  }

FXbool PulseOutput::configure(const AudioFormat & fmt){
  const pa_sample_spec * config=NULL;

  if (!open())
    return false;

  if (stream && fmt==af)
    return true;

  //pa_threaded_mainloop_lock(mainloop);

  if (stream) {
    pa_stream_disconnect(stream);
    pa_stream_unref(stream);
    stream=NULL;
    }

  pa_sample_spec spec;

  if (!to_pulse_format(fmt,spec.format))
    goto failed;

  spec.rate     = fmt.rate;
  spec.channels = fmt.channels;

  stream = pa_stream_new(context,"Goggles Music Manager",&spec,NULL);
  pa_stream_set_state_callback(stream,stream_state_callback,eventloop);
  //pa_stream_set_write_callback(stream,stream_write_callback,this);

  if (pa_stream_connect_playback(stream,NULL,NULL,PA_STREAM_NOFLAGS,NULL,NULL)<0)
    goto failed;

  /// Wait until stream is ready
  pa_stream_state_t state;
  while((state=pa_stream_get_state(stream))!=PA_STREAM_READY) {
    if (state==PA_STREAM_FAILED || state==PA_STREAM_TERMINATED){
      goto failed;
      }
    eventloop->wait();
    //pa_threaded_mainloop_wait(mainloop);
    }

  /// Get Actual Format
  config = pa_stream_get_sample_spec(stream);
  if (!to_gap_format(config->format,af))
    goto failed;
  af.channels=config->channels;
  af.rate=config->rate;

 // pa_threaded_mainloop_unlock(mainloop);
  return true;
failed:
  GM_DEBUG_PRINT("Unsupported pulse configuration:\n");
  af.debug();
  //pa_threaded_mainloop_unlock(mainloop);
  return false;
  }

void PulseOutput::start() {
  if (pa_stream_writable_size(stream)>1) {
    GM_DEBUG_PRINT("PulseOutput::start() %ld\n",pa_stream_writable_size(stream));
    stream_write_callback(stream,pa_stream_writable_size(stream),this);
    }
  }

FXbool PulseOutput::write(const void * b,FXuint nframes){
/*
  FXASSERT(stream);
  const FXchar * buffer = reinterpret_cast<const FXchar*>(b);






 // pa_threaded_mainloop_lock(mainloop);
  FXuint total = nframes*af.framesize();
  while(total) {
    size_t nbytes = pa_stream_writable_size(stream);
    size_t n = FXMIN(total,nbytes);
    GM_DEBUG_PRINT("WRITE-%ld------\n",n);
    if (n<=0) {
      GM_DEBUG_PRINT("WRITE-WAIT-------\n");
      eventloop->wait();
      //pa_threaded_mainloop_wait(mainloop);
      continue;
      }
    pa_stream_write(stream,buffer,n,NULL,0,PA_SEEK_RELATIVE);
    total-=n;
    buffer+=n;
    if (total)
      eventloop->wait();
    }
 // pa_threaded_mainloop_unlock(mainloop);
*/
  return true;
  }

}
