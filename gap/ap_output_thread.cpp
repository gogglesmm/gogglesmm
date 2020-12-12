/*******************************************************************************
*                         Goggles Audio Player Library                         *
********************************************************************************
*           Copyright (C) 2010-2021 by Sander Jansen. All Rights Reserved      *
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
#include "ap_convert.h"
#include "ap_engine.h"
#include "ap_crossfader.h"
#include "ap_input_thread.h"
#include "ap_output_thread.h"


#ifndef _WIN32
#include <poll.h>
#include <errno.h>
#endif


#ifndef AP_PLUGIN_PATH
#error "AP_PLUGIN_PATH PATH not defined"
#endif

#ifdef HAVE_SAMPLERATE
#include <samplerate.h>
#endif

namespace ap {


class FrameTimer {
private:
  FXint nold;
  FXint nwait;
  FXint nwritten;
public:
  FrameTimer(FXint n) : nold(n),nwait(n),nwritten(0) {
    GM_DEBUG_PRINT("[output] frame timer set to %d\n",nwait);
    }

  FXbool update(FXint delay,FXint nframes) {
      FXint real = delay - nwritten;
      FXint diff = FXABS(nold-real);

/*
      fxmessage("samples to wait for       : %10d\n",nwait);
      fxmessage("samples left previously   : %10d\n",nold);
      fxmessage("samples left              : %10d\n",delay);
      fxmessage("samples left + new samples: %10d\n",real);
      fxmessage("change in samples         : %10d\n",diff);
*/
      nwait-=diff;
      nold=delay;
      nwritten+=nframes;
      if (nwait<=0) {
        return true;
        }


  #if 0

//    if (nwritten>0) {
      diff = delay - nwritten;

      if (diff<=0) {
#ifdef DEBUG
        fxmessage("timer is expired\n");
#endif
        return true;
        }
      else {
        nwait=diff;
        nwritten+=nframes;
        }
      fxmessage("nwait %d\n",nwait);

#endif
//      }
//    else {
//      nwritten+=nframes;
//      }
    return false;
    }

  virtual void execute(AudioEngine*) {}

  virtual ~FrameTimer() {}
  };


class MetaTimer : public FrameTimer {
public:
  Event  * meta;
public:
  MetaTimer(Event * m,FXint n) : FrameTimer(n), meta(m) {}
  ~MetaTimer() {
    if (meta)
      meta->unref();
    }

  void execute(AudioEngine* engine) {
    engine->post(meta);
    meta=nullptr;
    }
  };


class EOSTimer : public FrameTimer {
  //FXint stream;
public:
  EOSTimer(/*FXint s,*/FXint n) : FrameTimer(n)/*,stream(s)*/{}
  void execute(AudioEngine* engine) {
    engine->input->post(new Event(AP_EOS));
    }
  };


OutputThread::OutputThread(AudioEngine*e) : EngineThread(e), fifoinput(nullptr),plugin(nullptr),draining(false),pausing(false) {
  stream=-1;
  stream_length=0;
  stream_remaining=0;
  stream_written=0;
  stream_position=0;
  timestamp=-1;
  }


FXbool OutputThread::init() {
  if (EngineThread::init()) {
    fifoinput=new Reactor::Input(fifo.signal().handle(),Reactor::Input::Readable);
    reactor.addInput(fifoinput);
    return true;
    }
  return false;
  }


OutputThread::~OutputThread() {
  FXASSERT(plugin==nullptr);
  reactor.removeInput(fifoinput);
  delete fifoinput;
  clear_timers();
  if (crossfader)
    delete crossfader;
  }


void OutputThread::reconfigure() {
  AudioFormat old=af;
  af.reset();
  configure(old);
  }


void OutputThread::notify_position() {
  FXint tm = (FXint) floor((double)stream_position / (double)plugin->af.rate);
  if (tm!=timestamp) {
    FXASSERT(stream_position>=0);
    FXASSERT(tm>=0);
    timestamp=tm;
    FXuint len =0;
    len = floor((double)stream_length / (double)plugin->af.rate);
    engine->post(new TimeUpdate(timestamp,len));
    }
  }


void OutputThread::notify_disable_volume(){
  engine->post(new VolumeNotify());
  }

void OutputThread::notify_volume(FXfloat value) {
  engine->post(new VolumeNotify(value));
  }

void OutputThread::clear_timers() {
  for (FXint i=0;i<timers.no();i++){
    delete timers[i];
    }
  timers.clear();
  }

void OutputThread::update_timers(FXint delay,FXint nframes) {
  for (FXint i=timers.no()-1;i>=0;i--){
    if (timers[i]->update(delay,nframes)) {
      timers[i]->execute(engine);
      delete timers[i];
      timers.erase(i);
      }
    }
  }




void OutputThread::update_position(FXint sid,FXlong position,FXint nframes,FXlong length) {
  FXint delay = plugin->delay();
  FXASSERT(position>=0);

  if (sid!=stream) {
    if (stream_remaining>0) {
      GM_DEBUG_PRINT("[output] stream_remaining already set. probably very short track. let's drain\n");
      drain(false);

      stream_remaining = 0;
      stream_written   = 0;
      stream_length    = length;
      engine->post(new Event(AP_BOS));
      }
    else {
      stream_remaining = delay;
      stream_written   = 0;
      if (stream_remaining<=0)
        engine->post(new Event(AP_BOS));
      }
    stream=sid;
    }

  if (stream_remaining>0) {
    if (stream_written>0) {
      FXint diff = delay - stream_written;
      if (diff>0) {
        if ((FXuint)diff>plugin->af.rate)
          stream_position += stream_remaining - diff;
        stream_remaining = diff;
        stream_written += nframes;
        FXASSERT(stream_position>0);
        }
      else {
        stream_remaining = 0;
        stream_written   = 0;
        stream_length    = length;
        engine->post(new Event(AP_BOS));
        }
      }
    else {
      stream_written += nframes;
      }
    }
  else {
    FXASSERT(delay<position);
    stream_position = FXMAX(0,position - delay);
    stream_length   = length;
    }


  update_timers(delay,nframes);
  notify_position();
  }



void OutputThread::drain(FXbool flush) {
  GM_DEBUG_PRINT("[output] drain while updating time\n");
  if (plugin) {
    FXint delay,offset;

    delay=offset=plugin->delay();

    if ((delay<=0) || ((double)delay/(double)plugin->af.rate) < 0.5) {
      GM_DEBUG_PRINT("[output] Current delay not worth waiting for (%d frames)\n",delay);
      if (flush) plugin->drain();
      return;
      }

    while(1) {
      FXThread::sleep(200000000);
      delay = plugin->delay();

      if (delay < (FXint)(plugin->af.rate>>1) ) {
        GM_DEBUG_PRINT("[output] Less than 0.5 left. drain the soundcard\n");
        if (flush) plugin->drain();
        return;
        }

      stream_position += offset - delay;
      offset = delay;
      update_timers(delay,0);
      notify_position();
      }
    }
  }


void OutputThread::wait_plugin_events() {
  reactor.removeInput(fifoinput);
  reactor.runOnce();
  reactor.addInput(fifoinput);
  }


Event * OutputThread::get_next_event() {
  if (draining)
    return wait_drain();
  else if (pausing)
    return wait_pause();
  else
    return wait_event();
  }


Event * OutputThread::wait_event() {
  do {
    Event * event = fifo.pop();
    if (event) {
      reactor.runPending();
      return event;
      }
    // FIXME maybe split into wait & dispatch so we can give higher priority to fifo.
    reactor.runOnce();
    }
  while(1);
  }

/*
  Player is paused and we don't handle any Buffer or Configure
  events until we receive some other command first. Plugin events
  are continueing to be handled.
*/
Event * OutputThread::wait_pause() {
  Event * event = fifo.pop_if_not(Buffer,Configure);
  if (event) {
    reactor.runPending();
    return event;
    }

  do {
    // FIXME maybe split into wait & dispatch so we can give higher priority to fifo.
    reactor.runOnce();
    if (fifoinput->readable()){
      event = fifo.pop_if_not(Buffer,Configure);
      if (event) return event;
      }
    }
  while(1);
  }

Event * OutputThread::wait_drain() {
  FXTime timeout = -1;
  const FXTime interval = 200000000;
  FXTime now = FXThread::time();
  FXTime wakeup = now + interval;

  FXint delay=plugin->delay();
  FXint offset=delay;
  FXint threshold=(FXint)(plugin->af.rate>>2);
  do {

    if (timeout==-1)
      timeout = wakeup-now;
    else
      timeout = FXMIN(timeout,wakeup-now);

    // FIXME maybe split into wait & dispatch so we can give higher priority to fifo.
    // Run reactor
    reactor.runOnce(timeout);

    // Check for event
    if (fifoinput->readable()){
      return fifo.pop();
      }

    // handle position updates
    if (FXThread::time()>=wakeup) {
      delay = plugin->delay();
      if (delay<threshold) {
        plugin->drain();
        update_timers(0,0); /// make sure timers get fired
        close_plugin();
        engine->input->post(new ControlEvent(End,stream));
        draining=false;
        return wait_event();
        }
      else {
        stream_position += offset - delay;
        offset = delay;
        update_timers(delay,0);
        notify_position();
        }
      wakeup = FXThread::time() + interval;
      }

    // set new time
    now = FXThread::time();
    }
  while(1);
  return nullptr;
  }



void OutputThread::load_plugin() {

  if (output_config.device==DeviceNone) {
    GM_DEBUG_PRINT("[output] no output plugin defined\n");
    return;
    }

  FXString plugin_name = ap_get_environment("GOGGLESMM_PLUGIN_PATH",AP_PLUGIN_PATH) + PATHSEPSTRING + FXSystem::dllName("gap_"+output_config.plugin());
  GM_DEBUG_PRINT("[output] loading plugin: %s\n",plugin_name.text());

  if (dll.loaded() && dll.name()!=plugin_name) {
    dll.unload();
    }

  if (!dll.loaded() && !dll.load(plugin_name)) {
    engine->post(new ErrorMessage(FXString::value("Failed to load output plugin: %s.\nReason: %s",plugin_name.text(),dll.error().text())));
    GM_DEBUG_PRINT("[output] unable to load %s\nreason: %s\n",plugin_name.text(),dll.error().text());
    return;
    }

  FXuint * plugin_version = static_cast<FXuint*>(dll.address("ap_version"));
  if (plugin_version==nullptr) {
    GM_DEBUG_PRINT("[output] incompatible plugin: no ap_version found\n");
    engine->post(new ErrorMessage(FXString::value("Failed to load output plugin: %s.\nRequired symbol \'ap_version\' not found.",plugin_name.text())));
    dll.unload();
    return;
    }

  if (*plugin_version!=AP_VERSION(GAP_VERSION_MAJOR,GAP_VERSION_MINOR,GAP_VERSION_PATCH)) {
    GM_DEBUG_PRINT("[output] incompatible plugin: version mismatch.\n");
    engine->post(new ErrorMessage(FXString::value("Failed to load output plugin: %s.\nThis plugin was build for a different version (%u) of gogglesmm.",plugin_name.text(),*plugin_version)));
    dll.unload();
    return;
    }

  ap_load_plugin_t ap_load_plugin = (ap_load_plugin_t) dll.address("ap_load_plugin");
  if (ap_load_plugin==nullptr || (plugin=ap_load_plugin(this))==nullptr) {
    GM_DEBUG_PRINT("[output] incompatible plugin\n");
    engine->post(new ErrorMessage(FXString::value("Failed to load output plugin: %s.\nRequired symbol \'ap_load_plugin\' not found.",plugin_name.text())));
    dll.unload();
    return;
    }

  /// Set Device Config
  if (plugin)
    plugin->setOutputConfig(output_config);
  }

void OutputThread::unload_plugin() {
  if (plugin) {
    FXASSERT(dll.loaded());
    ap_free_plugin_t ap_free_plugin = (ap_free_plugin_t) dll.address("ap_free_plugin");
    FXASSERT(ap_free_plugin!=nullptr);
    if (ap_free_plugin==nullptr) {
      fxwarning("[output] no 'ap_free_plugin' defined\n");
      delete plugin;
      plugin=nullptr;
      }
    else {
      ap_free_plugin(plugin);
      plugin=nullptr;
      }
    dll.unload();
    }
  }


void OutputThread::close_plugin() {
  GM_DEBUG_PRINT("[output] close device\n");
  if (plugin) {
    plugin->close();
    }
  draining=false;
  af.reset();
  }



void OutputThread::configure(const AudioFormat & fmt) {
  if (plugin==nullptr) {
    load_plugin();
    if (!plugin) {
      engine->input->post(new ControlEvent(Ctrl_Close));
      draining=false;
      af.reset();
      return;
      }
    af.reset();
    }
  FXASSERT(plugin);

  if (af==fmt || fmt==plugin->af){
    if (crossfader && crossfader->readable_frames()) {
      GM_DEBUG_PRINT("[crossfader] has buffer\n");
      if (af != fmt && !crossfader->convert(fmt)) {
        GM_DEBUG_PRINT("[crossfader] cannot crossfade, play remaining samples\n");
        drain_crossfader();
        reset_crossfader();
        }
      else {
        GM_DEBUG_PRINT("[crossfader] switch to playback\n");
        crossfader->recording = false;
        }
      }
    af=fmt;
    draining=false;

#ifdef DEBUG
    fxmessage("[output] stream ");
    fmt.debug();
    fxmessage("[output] plugin ");
    plugin->af.debug();
#endif

    return;
    }

  // We need to reconfigure the hardware, but first let's drain the existing samples
  GM_DEBUG_PRINT("[output] Potential new format... let's drain\n");

  if (crossfader && crossfader->readable_frames()) {
    GM_DEBUG_PRINT("[crossfader] cannot crossfade, play remaining samples\n");
    drain_crossfader();
    reset_crossfader();
    }

  drain();

  af=fmt;

  if (!plugin->configure(af)) {
    plugin->drop();
    close_plugin();
    engine->input->post(new ControlEvent(Ctrl_Close));
    return;
    }

#ifdef DEBUG
  fxmessage("[output] stream ");
  af.debug();
  fxmessage("[output] plugin ");
  plugin->af.debug();
#endif
  //crossfader->plugin = plugin;

  draining=false;
  }

static FXbool mono_to_stereo(FXuchar * in,FXuint nsamples,FXuchar bps,MemoryBuffer & out){
  out.clear();
  out.reserve(nsamples*bps*2);
  for (FXuint i=0;i<nsamples*bps;i+=bps) {
    out.append(&in[i],bps);
    out.append(&in[i],bps);
    }
  return true;
  }


#ifdef HAVE_SAMPLERATE
void OutputThread::resample(Packet * packet,FXint & nframes) {
  static SRC_STATE * src_state = nullptr;
  SRC_DATA srcdata;

  FXint framesize = packet->af.framesize();

  if (src_state==nullptr) {
    int error;
    src_state = src_new(SRC_SINC_BEST_QUALITY,packet->af.channels,&error);
    }

//  src_input.clear();
  src_input.append(packet->data(),packet->size());

  srcdata.data_in      = reinterpret_cast<FXfloat*>(src_input.data());
  srcdata.input_frames = src_input.size() / packet->af.framesize();
  srcdata.data_out     = reinterpret_cast<FXfloat*>(converted_samples.data());
  srcdata.src_ratio    = (FXdouble)plugin->af.rate / (FXdouble)packet->af.rate;
  srcdata.output_frames= (FXint)((FXdouble)srcdata.input_frames*srcdata.src_ratio);
  srcdata.end_of_input = 0;

  converted_samples.reserve(framesize*srcdata.output_frames);

  if (src_process(src_state,&srcdata)==0) {
    fxmessage("converted %ld|%ld|%d\n",srcdata.output_frames_gen,srcdata.input_frames_used,packet->numFrames());
    if (srcdata.input_frames_used) {
      if ((srcdata.input_frames_used*framesize)<src_input.size())
        src_input.trimBegin((srcdata.input_frames_used*framesize));
      else
        src_input.clear();
      }
    if (srcdata.output_frames_gen) {
      nframes=srcdata.output_frames_gen;
      }
    else {
      nframes=0;
      }
    }
  }
#endif

void OutputThread::reset_position() {
  stream_position=0;
  stream_written=0;
  stream_remaining=0;
  timestamp=-1;
  }


static void apply_scale_float(FXuchar * buffer,FXuint nsamples,FXdouble scale) {
  FXfloat * out = reinterpret_cast<FXfloat*>(buffer);
  for (FXuint i=0;i<nsamples;i++) {
    out[i]*=(FXfloat)scale;
    }
  }


static void apply_scale_s16(FXuchar * buffer,FXuint nsamples,FXdouble scale) {
  FXshort * out = reinterpret_cast<FXshort*>(buffer);
  for (FXuint i=0;i<nsamples;i++) {
    out[i]*=scale;
    }
  }


static void apply_crossfade_s16(MemoryBuffer & fade_in, MemoryBuffer & fade_out, FXint nframes, FXuint nchannels, FXlong position, FXdouble step) {
  auto in = reinterpret_cast<FXshort*>(fade_in.data());
  auto out = reinterpret_cast<FXshort*>(fade_out.data());
  double fade = position * step;
  for (int f = 0, s = 0; f < nframes; f++, fade += step, s += nchannels) {
    double fi = pow(fade, 3);
    double fo = 1.0 - fi;
    for (FXuint c = 0; c < nchannels; c++) {
      in[s + c] = lrint(((double)in[s + c]) * fi) + (((double)out[s + c]) * fo);

      }
    }
  }


static void apply_crossfade_float(MemoryBuffer & fade_in, MemoryBuffer & fade_out, FXint nframes, FXuint nchannels, FXlong position, FXfloat step) {
  auto in = reinterpret_cast<FXfloat*>(fade_in.data());
  auto out = reinterpret_cast<FXfloat*>(fade_out.data());
  float fade = position * step;
  for (int f = 0, s = 0; f < nframes; f++, fade += step, s += nchannels) {
    float fi = powf(fade, 3);
    float fo = 1.0 - fi;
    for (FXuint c = 0; c < nchannels; c++) {
      in[s + c] = (((float)in[s + c]) * fi) + (((float)out[s + c]) * fo);
      }
    }
  }



void OutputThread::reset_crossfader() {
  FXASSERT(crossfader);
  crossfader->flush();
  if (crossfader->duration == 0) {
    GM_DEBUG_PRINT("[crossfader] discard\n");
    delete crossfader;
    crossfader = nullptr;
    }
  }


void OutputThread::drain_crossfader() {
  FXASSERT(crossfader);
  if (crossfader->recording && crossfader->rframes) {
    GM_DEBUG_PRINT("[crossfader] drain %d\n", crossfader->rframes);
    init_crossfade_samples();
    process_samples();
    }
  }


void OutputThread::init_samples(Packet * packet) {
  samples.buffer = packet;
  samples.position = packet->stream_position;
  samples.length = packet->stream_length;
  samples.nframes = packet->numFrames();
  samples.stream = packet->stream;
  samples.crossfade = true;
  }


void OutputThread::init_crossfade_samples() {
  FXASSERT(crossfader);
  samples.buffer = &crossfader->buffer;
  samples.position = crossfader->position;
  samples.length = crossfader->length;
  samples.nframes = crossfader->buffer.size() / crossfader->af.framesize();
  samples.stream = crossfader->stream;
  samples.crossfade = false;
  }


void OutputThread::process_samples() {

  replay_gain();

  if (crossfader && samples.crossfade) {
    crossfade_samples();
    if (samples.nframes == 0)
      return;
    }

  if (!convert_samples())
    return;

  if (!write_samples())
    return;
  }

void OutputThread::replay_gain() {
  if (replaygain.mode != ReplayGainOff) {
    FXdouble gain  = replaygain.gain();
    if(!isnan(gain)) {
      FXdouble peak  = replaygain.peak();
      FXdouble scale = pow(10.0,(gain / 20.0));

      /// Avoid clipping
      if (!isnan(peak) && peak!=0.0 && (scale*peak)>1.0)
        scale = 1.0 / peak;

      switch(af.format) {
        case AP_FORMAT_FLOAT: apply_scale_float(samples.data(), samples.nframes * af.channels, scale); break;
        case AP_FORMAT_S16  : apply_scale_s16(samples.data(), samples.nframes * af.channels, scale); break;
        }
      }
    }
  }


void OutputThread::crossfade_samples() {
  FXASSERT(crossfader);
  if (crossfader->recording) {

    // Initialize Crossfader if we haven't done so.
    if (crossfader->position == -1) {
      crossfader->position = samples.length - (af.rate * crossfader->duration) / 1000;
      }

    // Store the end of stream samples into the crossfader
    if (samples.position + samples.nframes >= crossfader->position) {

      if (crossfader->nframes == 0) {  // Check if we record anything in the crossfader buffer
        if (samples.position >= crossfader->position)  // already passed the record position (due to seeking)
          crossfader->start_recording(af, samples.stream, samples.position, samples.length);
        else  // still before the record_position
          crossfader->start_recording(af, samples.stream, crossfader->position, samples.length);
        }

      if (samples.position < crossfader->position) {
        FXint skip = crossfader->position - samples.position;
        crossfader->writeFrames(samples.data() + (af.framesize() * skip), samples.nframes - skip);
        samples.nframes = skip;
        }
      else {
        crossfader->writeFrames(samples.data(), samples.nframes);
        samples.nframes = 0;
        }
      }

    }
  else if (crossfader->readable_frames()) {  // Apply crossfader buffer to the start of the stream
    FXASSERT(samples.position < crossfader->start_offset());
    FXint nframes = FXMIN(samples.nframes, crossfader->readable_frames());
    switch(af.format) {
      case AP_FORMAT_S16:
        apply_crossfade_s16(*samples.buffer, crossfader->buffer, nframes, af.channels, samples.position, 1.0 / crossfader->total_frames());
        crossfader->readFrames(nframes);
        break;
      case AP_FORMAT_FLOAT:
        apply_crossfade_float(*samples.buffer, crossfader->buffer, nframes, af.channels, samples.position, 1.0f / crossfader->total_frames());
        crossfader->readFrames(nframes);
        break;
      default:
        FXASSERT(0);
        break;
      }
    if (crossfader->rframes == 0 && crossfader->duration == 0) {
      delete crossfader;
      crossfader = nullptr;
      }
    }
  }


bool OutputThread::convert_samples() {
  if (af.format != plugin->af.format) {
    switch(plugin->af.format) {
      case AP_FORMAT_S16:
        {
          switch(af.format) {
            case AP_FORMAT_FLOAT:
              float_to_s16(samples.data(), samples.nframes * af.channels);
              break;
            case AP_FORMAT_S24_3:
              s24le3_to_s16(samples.data(), samples.nframes * af.channels);
              break;
            default:
              goto mismatch;
              break;
            }
        } break;
      case AP_FORMAT_S32:
        {
          switch(af.format) {
            case AP_FORMAT_FLOAT:
              float_to_s32(samples.data(), samples.nframes * af.channels);
              break;
            case AP_FORMAT_S24_3:
              s24le3_to_s32(samples.data(), samples.nframes * af.channels, samples.formatted);
              samples.buffer = &samples.formatted;
              break;
            default:
              goto mismatch;
              break;
            }
        } break;
      }
    }
  if (af.channels != plugin->af.channels) {
    if (af.channels == 1 && plugin->af.channels == 2) {
      mono_to_stereo(samples.data(), samples.nframes, af.packing(), samples.remapped);
      samples.buffer = &samples.remapped;
      }
    else {
      goto mismatch;
      }
    }
  return true;
mismatch:
  GM_DEBUG_PRINT("[output] config mismatch\n");
  draining=false;
  engine->input->post(new ControlEvent(Ctrl_Close));
  close_plugin();
  return false;
  }


FXbool OutputThread::write_samples() {
  FXuint nframes;
  while (samples.nframes) {
    nframes = FXMIN(FXint(plugin->af.rate >> 1), samples.nframes);
    update_position(samples.stream, samples.position, nframes, samples.length);
    if (!plugin->write(samples.data(), nframes)) {
      GM_DEBUG_PRINT("[output] write failed\n");
      engine->input->post(new ControlEvent(Ctrl_Close));
      engine->post(new ErrorMessage(FXString::value("Output Error")));
      close_plugin();
      return false;
      }
    samples.buffer->readBytes(plugin->af.framesize() * nframes);
    samples.nframes -= nframes;
    }
  return true;
  }



FXint OutputThread::run(){
  pausing=false;
  draining=false;
  ap_set_thread_name("ap_output");

  for (;;){
    Event * event = get_next_event();
    FXASSERT(event);

    switch(event->type) {
      case Buffer     :
        {
          if (__likely(af.set())) {
            auto packet = static_cast<Packet*>(event);
            init_samples(packet);
            process_samples();
            }
        } break;


      case Flush      :
        {
          FlushEvent * flush = static_cast<FlushEvent*>(event);
          GM_DEBUG_PRINT("[output] flush %d\n",flush->close);
          if (plugin) {
            plugin->drop();
            if (flush->close)
              close_plugin();
            }
          if (crossfader) {
            reset_crossfader();
            }
          pausing=false;
          draining=false;
          reset_position();
          clear_timers();
        } break;

      case Meta       :
        {
          if (__likely(af.set())) {
            FXASSERT(plugin);
            timers.append(new MetaTimer(event,plugin->delay()));
            continue;
            }
        } break;

      case Configure  :
        {
          ConfigureEvent * cfg = static_cast<ConfigureEvent*>(event);
          configure(cfg->af);
          replaygain.value = cfg->replaygain;
        } break;


      case End   :
        {
          if (af.set()) {
            FXint wait = plugin->delay();
            FXint rate = plugin->af.rate;

            if (wait<=rate)
              engine->input->post(new Event(AP_EOS));
            else
              timers.append(new EOSTimer(/*event->stream,*/wait-rate));

            draining=true;
            }

        } break;

      case Ctrl_Volume:
        {
					FXfloat volume=(static_cast<CtrlVolumeEvent*>(event))->vol;
          if (plugin) plugin->volume(volume);
        } break;

      case Ctrl_Pause :
        {
          pausing=!pausing;
          if (plugin)
            plugin->pause(pausing);

          if (pausing)
            engine->post(new Event(AP_STATE_PAUSING));
          else
            engine->post(new Event(AP_STATE_PLAYING));
        } break;

      case Ctrl_Quit  :
        {
          GM_DEBUG_PRINT("[output] quit\n");
          unload_plugin();
          Event::unref(event);
          clear_timers();
          return 0;
        } break;

      case Ctrl_Set_Replay_Gain:
        {
          SetReplayGain * g = static_cast<SetReplayGain*>(event);
          GM_DEBUG_PRINT("[output] set replay gain mode %d\n",g->mode);
          replaygain.mode = g->mode;
        } break;

      case Ctrl_Get_Replay_Gain:
        {
          GetReplayGain * g = static_cast<GetReplayGain*>(event);
          GM_DEBUG_PRINT("[output] get replay gain mode\n");
          g->mode = replaygain.mode;
        } break;


      case Ctrl_Set_Cross_Fade:
        {
          auto g = static_cast<SetCrossFade*>(event);
          GM_DEBUG_PRINT("[output] set cross fade %d ms\n",g->duration);
          if (g->enabled()) {
            if (crossfader == nullptr)
              crossfader = new CrossFader(g->duration);
            else
              crossfader->duration = g->duration;
            }
          else {
            if (crossfader) {
              crossfader->duration = 0;
              if (crossfader->recording) {
                drain_crossfader();
                reset_crossfader();
                }
              }
            }
        } break;

      case Ctrl_Get_Cross_Fade:
        {
          auto g = static_cast<GetCrossFade*>(event);
          GM_DEBUG_PRINT("[output] get cross fade\n");
          if (crossfader) {
            g->duration = crossfader->duration;
            }
          else {
            g->duration = 0;
            }
        } break;

      case Ctrl_Get_Output_Config:
        {
          GetOutputConfig * out = static_cast<GetOutputConfig*>(event);
          FXASSERT(out);
          out->config = output_config;
          GM_DEBUG_PRINT("[output] get output config\n");

        } break;

      case Ctrl_Set_Output_Config:
        {
          GM_DEBUG_PRINT("[output] set output config");
          SetOutputConfig * out = static_cast<SetOutputConfig*>(event);
          output_config = out->config;
          if (plugin) {
            if (plugin->type()==output_config.device) {
              if (af.set()) {
                drain(true);
                plugin->close();
                plugin->setOutputConfig(output_config);
                reconfigure();
                }
              else {
                plugin->close();
                plugin->setOutputConfig(output_config);
                }
              }
            else {
              if (af.set()) {
                drain(true);
                unload_plugin();
                reconfigure();
                }
              else {
                unload_plugin();
                }
              }
            }
        } break;
      };
    Event::unref(event);
    }
  return 0;
  }

}
