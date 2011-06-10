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


OutputThread::OutputThread(AudioEngine*e) : EngineThread(e), plugin(NULL),processing(false) {
  stream=-1;
  stream_remaining=0;
  stream_written=0;
  stream_position=0;
  timestamp=-1;
  }

OutputThread::~OutputThread() {
  FXASSERT(plugin==NULL);
  }


void OutputThread::reconfigure() {
  AudioFormat old=af;
  af.reset();
  configure(old);
  }


void OutputThread::notify_position() {
  FXint time = (FXint) floor((double)stream_position / (double)plugin->af.rate);
  if (time!=timestamp) {
    timestamp=time;
    FXuint len =0;
    len = floor((double)stream_length / (double)plugin->af.rate);
    engine->post(new TimeUpdate(timestamp,len));
    }
  }

void OutputThread::update_position(FXint id,FXint position,FXint nframes,FXint length) {
  FXint delay = plugin->delay();

  if (id!=stream) {
    if (stream_remaining>0) {
      fxmessage("stream_remaining already set. probably very short track. let's drain\n");
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
    stream=id;
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
  fxmessage("drain while updating time\n");
  if (plugin) {
    FXint delay,offset;

    delay=offset=plugin->delay();

    if ((delay<=0) || ((double)delay/(double)plugin->af.rate) < 0.5) {
      fxmessage("Current delay not worth waiting for (%d frames)\n",delay);
      if (flush) plugin->drain();
      return;
      }

    while(1) {
      FXThread::sleep(200000000);
      delay = plugin->delay();

      if (delay < (plugin->af.rate>>1) ) {
        fxmessage("Less than 0.5 left. drain the soundcard\n");
        if (flush) plugin->drain();
        return;
        }

      stream_position += offset - delay;
      offset = delay;

      notify_position();
      }
    }
  }





Event * OutputThread::wait_for_packet() {
  Event * event = fifo.pop();
  if (event==NULL) {
    ap_wait(fifo.handle());
    event=fifo.pop();
    }
  FXASSERT(event);
  return event;
  }

Event * OutputThread::wait_for_event() {
  do {
    Event * event = fifo.pop_if_not(Buffer,Configure);
    if (event) return event;
    ap_wait(fifo.handle());
    }
  while(1);
  }


void OutputThread::load_plugin() {
  typedef OutputPlugin*  (*ap_load_plugin_t)();

  if (output_config.device==DeviceNone) {
    fxmessage("[output] no output plugin defined\n");
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
    fxmessage("[output] unable to load %s\nreason: %s\n",plugin_name.text(),dll.error().text());
    return;
    }

  ap_load_plugin_t ap_load_plugin = (ap_load_plugin_t) dll.address("ap_load_plugin");
  if (ap_load_plugin==NULL || (plugin=ap_load_plugin())==NULL) {
    fxmessage("[output] incompatible plugin\n");
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
  fxmessage("[output] close device\n");
  if (plugin) {
    plugin->close();
    }
  processing=false;
  af.reset();
  }



void OutputThread::configure(const AudioFormat & fmt) {
  if (plugin==NULL) {
    load_plugin();
    if (!plugin) {
      engine->input->post(new ControlEvent(Ctrl_Close));
      processing=false;
      af.reset();
      return;
      }
    af.reset();
    }
  FXASSERT(plugin);

  if (af==fmt || fmt==plugin->af){
    af=fmt;
    processing=true;
    fxmessage("stream ");
    fmt.debug();
    fxmessage("output ");
    plugin->af.debug();
    return;
    }

  // We need to reconfigure the hardware, but first let's drain the existing samples
  fxmessage("Potential new format... let's drain\n");
  drain();

  af=fmt;

  if (!plugin->configure(af)) {
    plugin->drop();
    fxmessage("OutputThread::configure failed\n");
    engine->input->post(new ControlEvent(Ctrl_Close));
    processing=false;
    af.reset();
    return;
    }

  fxmessage("stream ");
  af.debug();
  fxmessage("output ");
  plugin->af.debug();
  processing=true;
  }



static FXbool mono_to_stereo(FXuchar * in,FXuint nsamples,FXuchar bps,MemoryBuffer & out){
  out.clear();
  out.grow(nsamples*bps*2);
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

  converted_samples.grow(framesize*srcdata.output_frames);

  if (src_process(src_state,&srcdata)==0) {
    fxmessage("converted %ld|%ld|%d\n",srcdata.output_frames_gen,srcdata.input_frames_used,packet->numFrames());
    if (srcdata.input_frames_used) {
      if ((srcdata.input_frames_used*framesize)<src_input.size())
        src_input.trimBefore(src_input.data ()+ (srcdata.input_frames_used*framesize));
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
    out[i]*=scale;
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
    fxmessage("[output] write failed\n");
    engine->input->post(new ControlEvent(Ctrl_Close));
    engine->post(new ErrorMessage(FXString::value("Output Error")));
    processing=false;
    }



  return;
mismatch:
  fxmessage("[output] config mismatch\n");
  processing=false;
  engine->input->post(new ControlEvent(Ctrl_Close));
  }

/*
struct output_stream_state {
  FXint  streamid;
  FXlong remaining;
  FXlong total;
  FXlong fs;
  };


void OutputThread::update_position(FXint fs) {
  fs_total_delay = plugin->delay();

  FXint delay=fs_total_delay;

  for (FXint i=0;i<ss.no();i++) {
    if (ss[i].remaining) {
      ss[i].remaining




  if (fs==0) {

    while(fs_total_
    for (FXint i=0;i<nstreams;i++){
      if (ss[i].rem


    if (state.remaining



    fs_remaining=fs_delay;
    if (fs_remaining==0)
      fs_output=0;
    }
  else if (fs


  engine->post(new TimeUpdate(seconds,0));
  }
*/


FXint OutputThread::run(){
  Event * event;
  FXbool pausing=false;

  ap_set_thread_name("ap_output");

  for (;;){

    if (!pausing)
      event = wait_for_packet();
    else
      event = wait_for_event();

    FXASSERT(event);


    switch(event->type) {
      case Flush      :
                        {
                          fxmessage("[output] flush\n");
                          FlushEvent * flush = dynamic_cast<FlushEvent*>(event);
                          if (plugin) {
                            plugin->drop();
                            if (flush->close)
                              close_plugin();
                            }
                          pausing=false;
                          reset_position();
                        }
                        break;

      case Ctrl_EOS   : //engine->post(new Event(AP_EOS));
                        engine->input->post(event);
                        continue;
                        break;
      case Ctrl_Volume: if (plugin) plugin->volume((dynamic_cast<CtrlVolumeEvent*>(event))->vol);
                        break;
      case Ctrl_Pause : pausing=!pausing;
                        if (plugin)
                          plugin->pause(pausing);

                        if (pausing)
                          engine->post(new Event(AP_STATE_PAUSING));
                        else
                          engine->post(new Event(AP_STATE_PLAYING));
                        break;

      case Ctrl_Quit  : fxmessage("[output] quit\n");
                        unload_plugin();
                        Event::unref(event);
                        return 0;

      case Ctrl_Set_Replay_Gain:
                        {
                          SetReplayGain * g = dynamic_cast<SetReplayGain*>(event);
                          fxmessage("[output] set replay gain mode %d\n",g->mode);
                          replaygain.mode = g->mode;
                        } break;

      case Ctrl_Get_Replay_Gain:
                        {
                          GetReplayGain * g = dynamic_cast<GetReplayGain*>(event);
                          fxmessage("[output] get replay gain mode\n");
                          g->mode = replaygain.mode;
                        } break;

      case Ctrl_Get_Output_Config:
                        {
                          GetOutputConfig * out = dynamic_cast<GetOutputConfig*>(event);
                          FXASSERT(out);
                          out->config = output_config;
                          fxmessage("[output] get output config\n");
                          break;
                        }
      case Ctrl_Set_Output_Config:
                        {
                          fxmessage("[output] set ouput config");
                          SetOutputConfig * out = dynamic_cast<SetOutputConfig*>(event);
                          output_config = out->config;
                          if (plugin) {
                            if (plugin->type()==output_config.device) {
                              if (processing) {
                                drain(true);
                                plugin->close();
                                plugin->setOutputConfig(output_config);
                                reconfigure();
                                }
                              else {
                                close_plugin();
                                plugin->setOutputConfig(output_config);
                                }
                              }
                            else {
                              if (processing)
                                drain(true);

                              unload_plugin();

                              if (processing)
                                reconfigure();
                              }
                            }
                        } break;

      case Meta       : engine->post(event);
                        continue;
                        break;
                        
      case Configure  : configure(((ConfigureEvent*)event)->af);
                        replaygain.value = ((ConfigureEvent*)event)->replaygain;
                        break;
      case Buffer     : if (processing)
                          process(dynamic_cast<Packet*>(event));
                        break;
      };
    Event::unref(event);
    }
  return 0;
  }

}
