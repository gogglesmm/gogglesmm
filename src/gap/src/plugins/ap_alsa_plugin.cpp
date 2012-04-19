#include "ap_defs.h"
#include "ap_config.h"
#include "ap_utils.h"
#include "ap_pipe.h"
#include "ap_format.h"
#include "ap_event.h"
#include "ap_buffer.h"
#include "ap_packet.h"
#include "ap_event_queue.h"
#include "ap_thread_queue.h"
#include "ap_engine.h"
#include "ap_thread.h"
#include "ap_device.h"
#include "ap_input_plugin.h"
#include "ap_output_plugin.h"
#include "ap_decoder_plugin.h"
#include "ap_decoder_thread.h"
#include "ap_alsa_plugin.h"

#define ALSA_VERSION(major,minor,patch) ((major<<16)|(minor<<8)|patch)

using namespace ap;


extern "C" GMAPI OutputPlugin * ap_load_plugin() {
  return new AlsaOutput();
  }

extern "C" GMAPI void ap_free_plugin(OutputPlugin* plugin) {
  delete plugin;
  }

namespace ap {

static FXbool to_alsa_format(const AudioFormat & af,snd_pcm_format_t & alsa_format) {
  switch(af.format){
    case AP_FORMAT_S8        : alsa_format=SND_PCM_FORMAT_S8;       break;
    case AP_FORMAT_S16_LE    : alsa_format=SND_PCM_FORMAT_S16_LE;   break;
    case AP_FORMAT_S16_BE    : alsa_format=SND_PCM_FORMAT_S16_BE;   break;
    case AP_FORMAT_S24_LE    : alsa_format=SND_PCM_FORMAT_S24_LE;   break;
    case AP_FORMAT_S24_BE    : alsa_format=SND_PCM_FORMAT_S24_BE;   break;
    case AP_FORMAT_S24_3LE   : alsa_format=SND_PCM_FORMAT_S24_3LE;  break;
    case AP_FORMAT_S24_3BE   : alsa_format=SND_PCM_FORMAT_S24_3BE;  break;
    case AP_FORMAT_S32_LE    : alsa_format=SND_PCM_FORMAT_S32_LE;   break;
    case AP_FORMAT_S32_BE    : alsa_format=SND_PCM_FORMAT_S32_BE;   break;
    case AP_FORMAT_FLOAT_LE  : alsa_format=SND_PCM_FORMAT_FLOAT_LE; break;
    case AP_FORMAT_FLOAT_BE  : alsa_format=SND_PCM_FORMAT_FLOAT_BE; break;
    default                  : GM_DEBUG_PRINT("Unhandled format: %d\n", af.format); return false; break;
    }
  return true;
  }
#if 0
static void alsa_list_devices() {
  FXchar ** hints=NULL;

  if (snd_device_name_hint(-1,"pcm",(void***)&hints)<0){
    return;
    }

  for (FXint i=0;hints[i];i++) {
    fxmessage("[%d]=%s\n",i,hints[i]);
    }

  snd_device_name_free_hint((void**)hints);
  }

#endif



AlsaOutput::AlsaOutput() : OutputPlugin(), handle(NULL),mixer(NULL),mixer_element(NULL) {
  }

AlsaOutput::~AlsaOutput() {
  close();
  }


FXbool AlsaOutput::setOutputConfig(const OutputConfig & c) {
  config=c.alsa;
  return true;
  }

FXbool AlsaOutput::open() {
  FXint result;
  if (handle==NULL) {

    if ((result=snd_pcm_open(&handle,config.device.text(),SND_PCM_STREAM_PLAYBACK,0))<0) {
      GM_DEBUG_PRINT("Unable to open device \"%s\": %s\n",config.device.text(),snd_strerror(result));
      return false;
      }

    GM_DEBUG_PRINT("[alsa] opened device \"%s\"\n",config.device.text());

    snd_pcm_info_t* info;
    snd_pcm_info_alloca(&info);

    if (snd_pcm_info(handle,info)>=0) {
      GM_DEBUG_PRINT("alsa device: %s %s %s\n",snd_pcm_info_get_id(info),snd_pcm_info_get_name(info),snd_pcm_info_get_subdevice_name(info));
      if (snd_mixer_open(&mixer,0)<0) {
        GM_DEBUG_PRINT("Unable to open mixer: %s\n",snd_strerror(result));
        return true;
        }

      FXString mixerdevice = snd_pcm_name(handle);

      if ((result=snd_mixer_attach(mixer,mixerdevice.text()))<0) {
        GM_DEBUG_PRINT("Unable to attach mixer: %s\n",snd_strerror(result));
        if (snd_pcm_info_get_card(info)!=-1) {
          mixerdevice.format("hw:%d",snd_pcm_info_get_card(info));
          if ((result=snd_mixer_attach(mixer,mixerdevice.text()))<0) {
            GM_DEBUG_PRINT("Unable to attach mixer: %s\n",snd_strerror(result));
            snd_mixer_close(mixer);
            mixer=NULL;
            return true;
            }
          }
        else {
          snd_mixer_close(mixer);
          mixer=NULL;
          return true;
          }
        }

      GM_DEBUG_PRINT("mixer device: %s\n",mixerdevice.text());

      if ((result=snd_mixer_selem_register(mixer,NULL,NULL))<0){
        GM_DEBUG_PRINT("Unable to register simple mixer: %s\n",snd_strerror(result));
        snd_mixer_close(mixer);
        mixer=NULL;
        return true;
        }

      if ((result=snd_mixer_load(mixer))<0) {
        GM_DEBUG_PRINT("Unable to attach mixer: %s\n",snd_strerror(result));
        snd_mixer_close(mixer);
        mixer=NULL;
        return true;
        }

      for (snd_mixer_elem_t * element = snd_mixer_first_elem(mixer);element;element=snd_mixer_elem_next(element)){
        if (snd_mixer_elem_get_type(element)!=SND_MIXER_ELEM_SIMPLE) continue;
        if (!snd_mixer_selem_is_active(element)) continue;
        if (snd_mixer_selem_has_playback_channel(element,SND_MIXER_SCHN_FRONT_LEFT) &&
            snd_mixer_selem_has_playback_channel(element,SND_MIXER_SCHN_FRONT_RIGHT)) {
          mixer_element=element;
          GM_DEBUG_PRINT("found mixer: %s\n",snd_mixer_selem_get_name(element));
          break;
          }
        }
      }
    }
  return true;
  }

void AlsaOutput::volume(FXfloat v) {
  if (mixer && mixer_element) {
    long min,max;
    snd_mixer_selem_get_playback_volume_range(mixer_element,&min,&max);
    long value = FXLERP(min,max,v);

    snd_mixer_selem_set_playback_volume_all(mixer_element,value);

//    snd_mixer_selem_set_playback_volume(mixer_element,SND_MIXER_SCHN_FRONT_LEFT,volume);
//    snd_mixer_selem_set_playback_volume(mixer_element,SND_MIXER_SCHN_FRONT_RIGHT,volume);
//    fxmessage("vol: %g %d\n",v,value);
    }
  }


FXint AlsaOutput::delay() {
  snd_pcm_sframes_t nframes=0;
  if (handle) {
    if (snd_pcm_delay(handle,&nframes)<0 || nframes<0)
      return 0;
    }
  return nframes;
  }

void AlsaOutput::close() {
  if (handle) {
    snd_pcm_drop(handle);
    if (mixer) {
      snd_mixer_free(mixer);
      snd_mixer_close(mixer);
      mixer=NULL;
      mixer_element=NULL;
      }
    snd_pcm_close(handle);
    handle=NULL;
    }
  af.reset();
  }


void AlsaOutput::drop() {
  if (__likely(handle)) {
    snd_pcm_drop(handle);
//    snd_pcm_prepare(handle);
    }
  }

void AlsaOutput::drain() {
  if (__likely(handle)) {
    snd_pcm_drain(handle);
//    snd_pcm_prepare(handle);
    }
  }

void AlsaOutput::pause(FXbool p) {
  FXint result=-1;
  if (__likely(handle)) {
    if (can_pause) {
      result = snd_pcm_pause(handle,p?1:0);
      if (result==-1 && p==true)
        snd_pcm_drain(handle);
      }
    else {
      if (p) snd_pcm_drain(handle);
      }
    }
  }



FXbool AlsaOutput::configure(const AudioFormat & fmt){
  snd_pcm_uframes_t periodsize;
  snd_pcm_uframes_t buffersize;
  snd_pcm_uframes_t boundary;
  snd_pcm_format_t  format;
  snd_pcm_hw_params_t * hw=NULL;
  snd_pcm_sw_params_t * sw=NULL;

#ifdef DEBUG
  //snd_pcm_uframes_t minperiod,maxperiod;
  //snd_pcm_uframes_t minbuffer,maxbuffer;
  snd_pcm_uframes_t availmin;
  snd_pcm_uframes_t startthreshold,stopthreshold;
  //FXuint buffertime = 500000;
#endif



  snd_pcm_hw_params_alloca(&hw);
  snd_pcm_sw_params_alloca(&sw);

  FXuint num_channels;
  FXint dir=0;
  FXuint sample_rate;



//  bool try_reopen=(handle) ? true : false;


//  do {

  if (__unlikely(handle==NULL)) {
    if (!open()) {
      return false;
      }
    }

  if (fmt==af) {
    return true;
    }
/*
  else {
    close()
    if (!open())
      return false;
    }
*/




  af=fmt;

  if (!to_alsa_format(af,format))
    goto failed;


  /// Make sure we call drain to stop the pcm.
  drain();

  /// Set to blocking
  if (snd_pcm_nonblock(handle,0)<0)
    goto failed;

  /// Initialize to requested value
  num_channels=af.channels;
  sample_rate=af.rate;

  /// Get all pcm configurations
  if (snd_pcm_hw_params_any(handle,hw)<0)
    goto failed;

  /// Select whether we want to allow ALSA to resample
  if (snd_pcm_hw_params_set_rate_resample(handle,hw,(config.flags&AlsaConfig::DeviceNoResample) ? 0 : 1 )<0){
    GM_DEBUG_PRINT("Unable to set hardware sample rate only\n");
    goto failed;
    }

  while(snd_pcm_hw_params_test_format(handle,hw,format)<0) {

    // Try a simple swap
    if (af.swap()) {
      if (to_alsa_format(af,format) && snd_pcm_hw_params_test_format(handle,hw,format)==0)
        break;
      af.swap();
      }

    // Try a compatible format.
    if (!af.compatible() || !to_alsa_format(af,format)) {
      goto failed;
      }

    }

  /// Try to set format
  if ((snd_pcm_hw_params_set_format(handle,hw,format)<0) ||
      (snd_pcm_hw_params_set_channels_near(handle,hw,&num_channels)<0) ||
      (snd_pcm_hw_params_set_rate_near(handle,hw,&sample_rate,&dir)<0)) {
    goto failed;
    }

  /// Try mmap'ed access
  if ((config.flags&AlsaConfig::DeviceMMap) && snd_pcm_hw_params_set_access(handle,hw,SND_PCM_ACCESS_MMAP_INTERLEAVED)<0) {
    config.flags&=~AlsaConfig::DeviceMMap;
    }

  /// Try regular access if not mmap'ed.
  if (!(config.flags&AlsaConfig::DeviceMMap) && snd_pcm_hw_params_set_access(handle,hw,SND_PCM_ACCESS_RW_INTERLEAVED)<0){
    goto failed;
    }

/*
  if (snd_pcm_hw_params_get_buffer_size_max(hw,&maxbuffer)<0)
    goto failed;

  if (snd_pcm_hw_params_set_buffer_size_near(handle,hw,&maxbuffer)<0)
    goto failed;

  dir=0;
  if (snd_pcm_hw_params_get_period_size_max(hw,&maxperiod,&dir)<0)
    goto failed;

  dir=0;
  if (snd_pcm_hw_params_set_period_size_near(handle,hw,&maxperiod,&dir)<0)
    goto failed;

  dir=0;
  if (snd_pcm_hw_params_set_buffer_time_near(handle,hw,&buffertime,&dir)<0)
    goto failed;
*/

  // Configure the hardware
  if (snd_pcm_hw_params(handle,hw)<0)
    goto failed;

  if (snd_pcm_hw_params_current(handle,hw)<0)
    goto failed;

  can_pause  = snd_pcm_hw_params_can_pause(hw);
  can_resume = snd_pcm_hw_params_can_resume(hw);

  dir=0;
  if (snd_pcm_hw_params_get_rate(hw,&sample_rate,&dir)<0)
    goto failed;

  if (snd_pcm_hw_params_get_channels(hw,&num_channels)<0)
    goto failed;


  dir=0;
  if (snd_pcm_hw_params_get_period_size(hw,&periodsize,&dir)<0)
    goto failed;

  if (snd_pcm_hw_params_get_buffer_size(hw,&buffersize)<0)
    goto failed;


  af.rate=sample_rate;
  af.channels=num_channels;

  if (snd_pcm_sw_params_current(handle,sw)<0)
    goto failed;

  if (snd_pcm_sw_params_set_avail_min(handle,sw,periodsize)<0)
    goto failed;

  /// Start when almost full
//  if (snd_pcm_sw_params_set_start_threshold(handle,sw,(buffersize/periodsize)*periodsize)<0)
//    goto failed;

  if (snd_pcm_sw_params_set_stop_threshold(handle,sw,buffersize)<0)
    goto failed;

  if (snd_pcm_sw_params_get_boundary(sw,&boundary)<0)
    goto failed;

  if (snd_pcm_sw_params_set_silence_size(handle,sw,boundary)<0)
    goto failed;

#if SND_LIB_VERSION < ALSA_VERSION(1,0,16)
  if (snd_pcm_sw_params_set_xfer_align(handle,sw,1)<0)
    goto failed;
#endif

  if (snd_pcm_sw_params(handle,sw)<0)
    goto failed;

#ifdef DEBUG
  if (snd_pcm_sw_params_current(handle,sw)<0)
    goto failed;

  if (snd_pcm_sw_params_get_avail_min(sw,&availmin)<0)
    goto failed;
  if (snd_pcm_sw_params_get_start_threshold(sw,&startthreshold)<0)
    goto failed;
  if (snd_pcm_sw_params_get_stop_threshold(sw,&stopthreshold)<0)
    goto failed;

  GM_DEBUG_PRINT("[alsa] configuration\n");
  GM_DEBUG_PRINT("\tstart threshold: %lu\n",startthreshold);
  GM_DEBUG_PRINT("\tstop threshold: %lu\n",stopthreshold);
  GM_DEBUG_PRINT("\tavail min: %lu\n",availmin);
  GM_DEBUG_PRINT("\tperiod size: %lu\n",periodsize);
  GM_DEBUG_PRINT("\tbuffer size: %lu\n",buffersize);
#endif

  if (snd_pcm_nonblock(handle,1)<0)
    goto failed;

  return true;
failed:
  GM_DEBUG_PRINT("[alsa] unsupported configuration\n");
  af.reset();
  return false;
  }

FXbool AlsaOutput::write(const void * buffer,FXuint nframes){
  int result;
  snd_pcm_sframes_t navailable;
  snd_pcm_sframes_t nwritten;
  snd_pcm_state_t state;
  const FXchar * buf = (const FXchar*)buffer;

  if (__unlikely(handle==NULL))
    return false;

  while(nframes>0) {
    state=snd_pcm_state(handle);
    switch(state) {
      /// Failed States
      case SND_PCM_STATE_DRAINING     :
      case SND_PCM_STATE_DISCONNECTED :
      case SND_PCM_STATE_OPEN         : GM_DEBUG_PRINT("[alsa] state is open, draining or disconnected\n");
                                        return false;
                                        break;

      case SND_PCM_STATE_PAUSED       : GM_DEBUG_PRINT("[alsa] state is paused while write is called\n");
                                        return false;
                                        break;

      /// Recoverable States
      case SND_PCM_STATE_XRUN         :
        {
          GM_DEBUG_PRINT("[alsa] xrun\n");
          result = snd_pcm_prepare(handle);
          if (result<0) {
            fxmessage("[alsa] %s",snd_strerror(result));
            return false;
            }
        } break;

      case SND_PCM_STATE_SETUP        :
        {
          result = snd_pcm_prepare(handle);
          if (result<0) {
            GM_DEBUG_PRINT("[alsa] %s",snd_strerror(result));
            return false;
            }

        } break;

      case SND_PCM_STATE_SUSPENDED:
        {
          GM_DEBUG_PRINT("[alsa] suspended\n");
          result=-1;

          if (can_resume) {
            while((result=snd_pcm_resume(handle))==-EAGAIN)
              FXThread::sleep(10000000);
            }

          /// If the hardware cannot resume, we need to call prepare
          if (result!=0)
            result = snd_pcm_prepare(handle);

          if (result!=0) {
            GM_DEBUG_PRINT("[alsa] %s",snd_strerror(result));
            return false;
            }

        } break;

      case SND_PCM_STATE_PREPARED     :
      case SND_PCM_STATE_RUNNING      :
        {
          navailable = snd_pcm_avail_update(handle);
          if (navailable<nframes) {
            result = snd_pcm_wait(handle,500);
            if (result<0) {
              /// Underrun / Suspended
              if (result==-EPIPE || result==-ESTRPIPE) {
                GM_DEBUG_PRINT("[alsa] %s\n",snd_strerror(result));
                continue;
                }
              return false;
              }
            }
        } /// intentionally no break
      default                         :
        {
          if ((config.flags&AlsaConfig::DeviceMMap))
            nwritten = snd_pcm_mmap_writei(handle,buf,nframes);
          else
            nwritten = snd_pcm_writei(handle,buf,nframes);

          if (nwritten==-EAGAIN || nwritten==-EINTR)
            continue;

          if (nwritten<0) {
            GM_DEBUG_PRINT("[alsa] xrun or suspend: %s\n",snd_strerror(nwritten));
            nwritten = snd_pcm_recover(handle,nwritten,1);
            if (nwritten<0) {
              if (nwritten!=-EAGAIN) {
                GM_DEBUG_PRINT("[alsa] fatal write error %ld:  %s\n",nwritten,snd_strerror(nwritten));
                return false;
                }
              }
            }
          if (nwritten>0) {
            buf+=(nwritten*af.framesize());
            nframes-=nwritten;
            if (snd_pcm_state(handle)!=SND_PCM_STATE_RUNNING) {
              GM_DEBUG_PRINT("PCM NOT RUNNING\n");
              }
            }
        } break;
      }
    }
  return true;
  }

}
