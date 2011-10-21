#include "ap_defs.h"
#include "ap_config.h"
#include "ap_utils.h"
#include "ap_pipe.h"
#include "ap_event.h"
#include "ap_format.h"
#include "ap_device.h"
#include "ap_event_private.h"
#include "ap_event_queue.h"
#include "ap_thread_queue.h"
#include "ap_memory_buffer.h"
#include "ap_convert.h"
#include "ap_packet.h"
#include "ap_engine.h"
#include "ap_thread.h"
#include "ap_thread_queue.h"
#include "ap_input_thread.h"
#include "ap_output_plugin.h"
#include "ap_output_thread.h"

#ifndef AP_PLUGIN_PATH
#error "AP_PLUGIN_PATH PATH not defined"
#endif

#ifdef HAVE_SAMPLERATE_PLUGIN
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
    GM_DEBUG_PRINT("timer set to %d\n",nwait);
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
    meta=NULL;
    }
  };


class EOSTimer : public FrameTimer {
  FXint stream;
public:
  EOSTimer(FXint s,FXint n) : FrameTimer(n),stream(s){}
  void execute(AudioEngine* engine) {
    engine->post(new Event(AP_EOS));
    }
  };

OutputThread::OutputThread(AudioEngine*e) : EngineThread(e), plugin(NULL),draining(false) {
  stream=-1;
  stream_remaining=0;
  stream_written=0;
  stream_position=0;
  timestamp=-1;
  }

OutputThread::~OutputThread() {
  FXASSERT(plugin==NULL);
  clear_timers();
  }


void OutputThread::reconfigure() {
  AudioFormat old=af;
  af.reset();
  configure(old);
  }


void OutputThread::notify_position() {
  FXint tm = (FXint) floor((double)stream_position / (double)plugin->af.rate);
  if (tm!=timestamp) {
    timestamp=tm;
    FXuint len =0;
    len = floor((double)stream_length / (double)plugin->af.rate);
    engine->post(new TimeUpdate(timestamp,len));
    }
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




void OutputThread::update_position(FXint sid,FXint position,FXint nframes,FXint length) {
  FXint delay = plugin->delay();

  if (sid!=stream) {
    if (stream_remaining>0) {
      GM_DEBUG_PRINT("stream_remaining already set. probably very short track. let's drain\n");
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
    stream_position = position - delay;
    stream_length   = length;
    }


  update_timers(delay,nframes);


  notify_position();


#if 0


      FXint fs_diff=fs_delay - fs_submitted;
//      fxmessage("fs_diff=%d",fs_diff);
      if (fs_diff>0) {
        if (fs_diff>plugin->af.rate)
          fs_output    += (fs_remaining - fs_diff);
        fs_remaining  = fs_diff;
//        fxmessage("diff: %d => %lg\n",fs_diff,floor((double)fs_output / (double)plugin->af.rate));
//        fxmessage("fs_output=%d\n" ,fs_output);
        }
      else {
        fs_remaining  = 0;
        fs_submitted  = 0;
        stream_length = length;
        engine->post(new NotifyEvent(Notify_Begin_Of_Stream));
        }
      }
    fs_submitted += nframes;
    }
  else {
    fs_output     = position - fs_delay;
    fs_submitted  = nframes;
    stream_length = length;
    }


  FXint s = floor((double)fs_output / (double)plugin->af.rate);
  if (s!=timestamp) {
    timestamp=s;
    FXuint len =0;
    len = floor((double)stream_length / (double)plugin->af.rate);
    engine->post(new TimeUpdate(timestamp,len));
    }
#endif
  }



void OutputThread::drain(FXbool flush) {
  GM_DEBUG_PRINT("drain while updating time\n");
  if (plugin) {
    FXint delay,offset;

    delay=offset=plugin->delay();

    if ((delay<=0) || ((double)delay/(double)plugin->af.rate) < 0.5) {
      GM_DEBUG_PRINT("Current delay not worth waiting for (%d frames)\n",delay);
      if (flush) plugin->drain();
      return;
      }

    while(1) {
      FXThread::sleep(200000000);
      delay = plugin->delay();

      if (delay < (FXint)(plugin->af.rate>>1) ) {
        GM_DEBUG_PRINT("Less than 0.5 left. drain the soundcard\n");
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


Event * OutputThread::wait_for_event() {
  FXint offset,delay;

  if (draining && plugin) {
    offset=delay=plugin->delay();
    }

  do {
    if (pausing) {
      Event * event = fifo.pop_if_not(Buffer,Configure);
      if (event) return event;
      ap_wait(fifo.handle());
      }
    else if (draining) {

      Event * event = fifo.pop();
      if (event) return event;

      if (ap_wait(fifo.handle(),200000000)){
        return fifo.pop();
        }

      if (plugin) {
        delay = plugin->delay();
        if (delay < (FXint)(plugin->af.rate>>2) ) {
          plugin->drain();
          update_timers(0,0); /// make sure timers get fired
          draining=false;
          close_plugin();
          engine->input->post(new ControlEvent(End,stream));
          continue;
          }
        else {
          stream_position += offset - delay;
          offset = delay;
          update_timers(delay,0);
          notify_position();
          }
        }
      else {
        draining=false;
        }
      }
    else {
      Event * event = fifo.pop();
      if (event) return event;
      ap_wait(fifo.handle());
      }
    }
  while(1);
  }






void OutputThread::load_plugin() {
  typedef OutputPlugin*  (*ap_load_plugin_t)();

  if (output_config.device==DeviceNone) {
    GM_DEBUG_PRINT("[output] no output plugin defined\n");
    return;
    }

  FXString plugin_name = AP_PLUGIN_PATH;
  FXString plugin_dll = "gaplugin_" + output_config.plugin();

  plugin_name += PATHSEPSTRING + FXSystem::dllName(plugin_dll);
  if (!FXStat::exists(plugin_name))
    plugin_name = FXSystem::dllName(plugin_dll);

  if (dll.loaded() && dll.name()!=plugin_name) {
    dll.unload();
    }

  if (!dll.loaded() && !dll.load(plugin_name)) {
    GM_DEBUG_PRINT("[output] unable to load %s\nreason: %s\n",plugin_name.text(),dll.error().text());
    return;
    }

  ap_load_plugin_t ap_load_plugin = (ap_load_plugin_t) dll.address("ap_load_plugin");
  if (ap_load_plugin==NULL || (plugin=ap_load_plugin())==NULL) {
    GM_DEBUG_PRINT("[output] incompatible plugin\n");
    dll.unload();
    }

  /// Set Device Config
  if (plugin)
    plugin->setOutputConfig(output_config);
  }

void OutputThread::unload_plugin() {
  typedef void (*ap_free_plugin_t)(OutputPlugin*);
  if (plugin) {
    FXASSERT(dll.loaded());
    ap_free_plugin_t ap_free_plugin = (ap_free_plugin_t) dll.address("ap_free_plugin");
    FXASSERT(ap_free_plugin!=NULL);
    if (ap_free_plugin==NULL) {
      fxwarning("[output] no 'ap_free_plugin' defined\n");
      delete plugin;
      plugin=NULL;
      }
    else {
      ap_free_plugin(plugin);
      plugin=NULL;
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
  if (plugin==NULL) {
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
    af=fmt;
    draining=false;

    fxmessage("stream ");
    fmt.debug();
    fxmessage("output ");
    plugin->af.debug();
    return;
    }

  // We need to reconfigure the hardware, but first let's drain the existing samples
  GM_DEBUG_PRINT("Potential new format... let's drain\n");
  drain();

  af=fmt;

  if (!plugin->configure(af)) {
    plugin->drop();
    close_plugin();
    engine->input->post(new ControlEvent(Ctrl_Close));

//    fxmessage("OutputThread::configure failed\n");
//    processing=false;
//    af.reset();
    return;
    }

  fxmessage("stream ");
  af.debug();
  fxmessage("output ");
  plugin->af.debug();
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



/*
static void softvol_float(FXuchar * buffer,FXuint nsamples,FXfloat vol) {
  FXfloat * input  = reinterpret_cast<FXfloat*>(buffer);
  for (FXint i=0;i<nsamples;i++) {
    input[i]*=vol;
    }
  }


static void softvol_s16(FXuchar * buffer,FXuint nsamples,FXfloat vol) {
  FXshort * input  = reinterpret_cast<FXshort*>(buffer);
  for (FXint i=0;i<nsamples;i++) {
    input[i]*=vol;
    }
  }
*/

/*
static void s8_to_float(FXchar * in,FXfloat * out,FXuint nsamples){
  for (FXint i=0;i<nsamples;i++) {
    out[i]=(((FXfloat)in[i])/127.0f);
    }
  }

static void s16_to_float(FXshort * in,FXfloat * out,FXuint nsamples){
  for (FXint i=0;i<nsamples;i++) {
    out[i]=(((FXfloat)in[i])/32767.0f);
    }
  }


static FXbool convert_to_float(OutputPacket * packet,MemoryBuffer & output) {
  const FXint nbytes  = packet->nframes*packet->af.channels*4;
  const FXint nsamples= packet->nframes*packet->af.channels;

  /// Make sure we have enough space
  output.grow_by(nbytes);

  switch(packet->af.format){
    case AP_FORMAT_S8 : s8_to_float(reinterpret_cast<FXchar*>(packet->data),
                                    reinterpret_cast<FXfloat*>(output.ptr()),
                                    nsamples);
                        output.data_size+=nbytes;
                        break;

    case AP_FORMAT_S16: s16_to_float(reinterpret_cast<FXshort*>(packet->data),
                                     reinterpret_cast<FXfloat*>(output.ptr()),
                                     nsamples);
                        output.data_size+=nbytes;
                        break;
    default           : return false; break;
    }
  return true;
  }

*/



/*
void OutputThread::convert_and_resample(OutputPacket * packet) {

  if (src_state==NULL) {
    int error;
    src_state = src_new(SRC_SINC_BEST_QUALITY,packet->af.channels,&error);
    }

  if (packet->af.datatype()!=Format::Float) {
    convert_to_float(packet,src_input);
    }
  else {
    src_input.append(packet->data,packet->nframes*framesize);
    }







  }

*/

#ifdef HAVE_SAMPLERATE_PLUGIN
void OutputThread::resample(Packet * packet,FXint & nframes) {
  static SRC_STATE * src_state = NULL;
  SRC_DATA srcdata;

  FXint framesize = packet->af.framesize();

  if (src_state==NULL) {
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


void OutputThread::process(Packet * packet) {

//  GM_TICKS_START();
  FXASSERT(packet);
  FXbool use_buffer=false;
  FXbool result=false;
  FXint nframes=packet->numFrames();
/*
  if (packet->af.format==AP_FORMAT_FLOAT) {
    softvol_float(packet->data,packet->nframes*packet->af.channels,softvol);
    }
  else if (packet->af.format==AP_FORMAT_S16){
    softvol_s16(packet->data,packet->nframes*packet->af.channels,softvol);
    }
*/

  if (replaygain.mode!=ReplayGainOff) {
    FXdouble gain  = replaygain.gain();
    if(!isnan(gain)) {
      FXdouble peak  = replaygain.peak();
      FXdouble scale = pow(10.0,(gain / 20.0));

      /// Avoid clipping
      if (!isnan(peak) && peak!=0.0 && (scale*peak)>1.0)
        scale = 1.0 / peak;

      switch(packet->af.format) {
        case AP_FORMAT_FLOAT: apply_scale_float(packet->data(),packet->numFrames()*packet->af.channels,scale); break;
        case AP_FORMAT_S16  : apply_scale_s16(packet->data(),packet->numFrames()*packet->af.channels,scale);   break;
        }
      }
    }



  if (packet->af!=plugin->af) {


    if (plugin->af.format!=packet->af.format) {

      if (plugin->af.format==AP_FORMAT_S16) {
        switch(packet->af.format) {
          case AP_FORMAT_FLOAT: float_to_s16(packet->data(),packet->numFrames()*packet->af.channels); break;
          case AP_FORMAT_S24_3: s24le3_to_s16(packet->data(),packet->numFrames()*packet->af.channels); break;
          default             : goto mismatch; break;
          }
        }
      else if (plugin->af.format==AP_FORMAT_S32) {
        switch(packet->af.format) {
          case AP_FORMAT_FLOAT: float_to_s32(packet->data(),packet->numFrames()*packet->af.channels); break;
          case AP_FORMAT_S24_3: s24le3_to_s32(packet->data(),packet->numFrames()*packet->af.channels,converted_samples); use_buffer=true; break;
          default             : goto mismatch; break;
          }
        }

      }

    if (packet->af.channels!=plugin->af.channels) {

      /// FIXME
      if (use_buffer==true)
        goto mismatch;

      if (packet->af.channels==1 && plugin->af.channels==2 ) {
        mono_to_stereo(packet->data(),packet->numFrames(),packet->af.packing(),converted_samples);
        use_buffer=true;
        }
      else {
        goto mismatch;
        }
      }
    if (packet->af.rate!=plugin->af.rate) {
      goto mismatch;
      }
    }

  update_position(packet->stream,packet->stream_position,packet->numFrames(),packet->stream_length);

  if (use_buffer)
    result = plugin->write(converted_samples.data(),nframes);
  else
    result = plugin->write(packet->data(),nframes);

  if (result==false) {
    GM_DEBUG_PRINT("[output] write failed\n");
    engine->input->post(new ControlEvent(Ctrl_Close));
    engine->post(new ErrorMessage(FXString::value("Output Error")));
    close_plugin();
    }
  return;
mismatch:
  GM_DEBUG_PRINT("[output] config mismatch\n");
  draining=false;
  engine->input->post(new ControlEvent(Ctrl_Close));
  close_plugin();
  }



FXint OutputThread::run(){
  Event * event;
  pausing=false;
  draining=false;
  ap_set_thread_name("ap_output");

  for (;;){
    event = wait_for_event();
    FXASSERT(event);

    switch(event->type) {
      case Buffer     :
        {
          if (__likely(af.set()))
            process(dynamic_cast<Packet*>(event));
        } break;


      case Flush      :
        {
          FlushEvent * flush = dynamic_cast<FlushEvent*>(event);
          GM_DEBUG_PRINT("[output] flush %d\n",flush->close);
          if (plugin) {
            plugin->drop();
            if (flush->close)
              close_plugin();
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
          ConfigureEvent * cfg = dynamic_cast<ConfigureEvent*>(event);
          configure(cfg->af);
          replaygain.value = cfg->replaygain;
        } break;


      case End   :
        {
          if (af.set()) {
            FXint wait = plugin->delay();
            FXint rate = plugin->af.rate;

            if (wait<=rate)
              engine->post(new Event(AP_EOS));
            else
              timers.append(new EOSTimer(event->stream,wait-rate));

            draining=true;
            }

        } break;

      case Ctrl_Volume:
        {
          if (plugin) plugin->volume((dynamic_cast<CtrlVolumeEvent*>(event))->vol);
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
          SetReplayGain * g = dynamic_cast<SetReplayGain*>(event);
          GM_DEBUG_PRINT("[output] set replay gain mode %d\n",g->mode);
          replaygain.mode = g->mode;
        } break;

      case Ctrl_Get_Replay_Gain:
        {
          GetReplayGain * g = dynamic_cast<GetReplayGain*>(event);
          GM_DEBUG_PRINT("[output] get replay gain mode\n");
          g->mode = replaygain.mode;
        } break;

      case Ctrl_Get_Output_Config:
        {
          GetOutputConfig * out = dynamic_cast<GetOutputConfig*>(event);
          FXASSERT(out);
          out->config = output_config;
          GM_DEBUG_PRINT("[output] get output config\n");

        } break;

      case Ctrl_Set_Output_Config:
        {
          GM_DEBUG_PRINT("[output] set output config");
          SetOutputConfig * out = dynamic_cast<SetOutputConfig*>(event);
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
