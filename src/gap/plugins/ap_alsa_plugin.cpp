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

//#define DEBUG 1

using namespace ap;


extern "C" GMAPI OutputPlugin * ap_load_plugin() {
  return new AlsaOutput();
  }

extern "C" GMAPI void ap_free_plugin(OutputPlugin* plugin) {
  delete plugin;
  }

namespace ap {


static snd_pcm_format_t alsaformat(const AudioFormat & af) {
  switch(af.format){
    case AP_FORMAT_S8        : return SND_PCM_FORMAT_S8;       break;
    case AP_FORMAT_S16_LE    : return SND_PCM_FORMAT_S16_LE;   break;
    case AP_FORMAT_S16_BE    : return SND_PCM_FORMAT_S16_BE;   break;
    case AP_FORMAT_S24_LE    : return SND_PCM_FORMAT_S24_LE;   break;
    case AP_FORMAT_S24_BE    : return SND_PCM_FORMAT_S24_BE;   break;
    case AP_FORMAT_S24_3LE   : return SND_PCM_FORMAT_S24_3LE;  break;
    case AP_FORMAT_S24_3BE   : return SND_PCM_FORMAT_S24_3BE;  break;
    case AP_FORMAT_S32_LE    : return SND_PCM_FORMAT_S32_LE;   break;
    case AP_FORMAT_S32_BE    : return SND_PCM_FORMAT_S32_BE;   break;
    case AP_FORMAT_FLOAT_LE  : return SND_PCM_FORMAT_FLOAT_LE; break;
    case AP_FORMAT_FLOAT_BE  : return SND_PCM_FORMAT_FLOAT_BE; break;
    default                  : break;
    }
  return SND_PCM_FORMAT_UNKNOWN;
  }






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




#ifdef DEBUG


#define DISPLAY_DIR(d) ((d==0) ? '=' : (d==-1) ? '<' : '>')
#define DISPLAY_YESNO(d) ((d==0) ? "no" : "yes")
#define DISPLAY_MINMAX_U(dir,value) ((dir==0) ?  fxmessage("%u",value) : fxmessage("%c%u",(dir==-1) ? '<' : '>',value))
#define DISPLAY_MINMAX_LU(dir,value) ((dir==0) ?  fxmessage("%lu",value) : fxmessage("%c%lu",(dir==-1) ? '<' : '>',value))

static void debug_hw_check_rate(snd_pcm_t * pcm,snd_pcm_hw_params_t * hw,unsigned int rate)
{
  if (snd_pcm_hw_params_test_rate(pcm,hw,rate,0)==0)
    fxmessage("%u ",rate);
}


static void debug_hw_minmax(int s1,int s2,int mindir,int maxdir,unsigned int min,unsigned int max){
  if (s1==0 && s2==0) {
    if (min!=max){
      DISPLAY_MINMAX_U(mindir,min);
      fxmessage(" - ");
      DISPLAY_MINMAX_U(maxdir,max);
      }
    else {
      DISPLAY_MINMAX_U(mindir,min);
      }
    }
  else if (s1==0) {
    DISPLAY_MINMAX_U(mindir,min);
    }
  else if (s2==0) {
    DISPLAY_MINMAX_U(maxdir,max);
    }
  else {
    fxmessage(" - ");
    }
  fxmessage("\n");
  }


static void debug_hw_minmax(int s1,int s2,int mindir,int maxdir,snd_pcm_uframes_t min,snd_pcm_uframes_t max){
  if (s1==0 && s2==0) {
    if (min!=max){
      DISPLAY_MINMAX_LU(mindir,min);
      fxmessage(" - ");
      DISPLAY_MINMAX_LU(maxdir,max);
      }
    else {
      DISPLAY_MINMAX_LU(mindir,min);
      }
    }
  else if (s1==0) {
    DISPLAY_MINMAX_LU(mindir,min);
    }
  else if (s2==0) {
    DISPLAY_MINMAX_LU(maxdir,max);
    }
  else {
    fxmessage(" - ");
    }
  fxmessage("\n");
  }

#endif

class AlsaSetup {
protected:
  snd_pcm_t           * pcm;
  snd_pcm_hw_params_t * hw;
  snd_pcm_sw_params_t * sw;
  snd_pcm_format_t      format;
  snd_pcm_uframes_t     buffer_size;
  snd_pcm_uframes_t     period_size;
  unsigned int          channels;
  unsigned int          rate;
protected:

  void debug_hw_caps(){
#ifdef DEBUG
    int s1,s2,mindir,maxdir;
    unsigned int minus,maxus;
    snd_pcm_uframes_t minframes,maxframes;

    fxmessage("[alsa] Hardware Caps\n");

    fxmessage("\tsample formats     ");
    for (int i=0;i<=(int)SND_PCM_FORMAT_LAST;i++){
      if (snd_pcm_hw_params_test_format(pcm,hw,(snd_pcm_format_t)i)==0)
        fxmessage("%s ",snd_pcm_format_name((snd_pcm_format_t)i));
      }
    fxmessage("\n");

    fxmessage("\tsample rates       ");
    debug_hw_check_rate(pcm,hw,44100);
    debug_hw_check_rate(pcm,hw,48000);
    debug_hw_check_rate(pcm,hw,96000);
    fxmessage("\n");

    fxmessage("\tsample rates (all) ");
    s1=snd_pcm_hw_params_get_rate_min(hw,&minus,&mindir);
    s2=snd_pcm_hw_params_get_rate_max(hw,&maxus,&maxdir);
    debug_hw_minmax(s1,s2,mindir,maxdir,minus,maxus);

    fxmessage("\tchannels           ");
    s1=snd_pcm_hw_params_get_channels_min(hw,&minus);
    s2=snd_pcm_hw_params_get_channels_max(hw,&maxus);
    debug_hw_minmax(s1,s2,0,0,minus,maxus);

    fxmessage("\tbuffer size        ");
    mindir=snd_pcm_hw_params_get_buffer_size_min(hw,&minframes);
    maxdir=snd_pcm_hw_params_get_buffer_size_max(hw,&maxframes);
    debug_hw_minmax(0,0,mindir,maxdir,minframes,maxframes);

    fxmessage("\tbuffer time        ");
    s1=snd_pcm_hw_params_get_buffer_time_min(hw,&minus,&mindir);
    s2=snd_pcm_hw_params_get_buffer_time_max(hw,&maxus,&maxdir);
    debug_hw_minmax(s1,s2,mindir,maxdir,minus,maxus);

    fxmessage("\tperiod size        ");
    s1=snd_pcm_hw_params_get_period_size_min(hw,&minframes,&mindir);
    s2=snd_pcm_hw_params_get_period_size_max(hw,&maxframes,&maxdir);
    debug_hw_minmax(s1,s2,mindir,maxdir,minframes,maxframes);

    fxmessage("\tperiod time        ");
    s1=snd_pcm_hw_params_get_period_time_min(hw,&minus,&mindir);
    s2=snd_pcm_hw_params_get_period_time_max(hw,&maxus,&maxdir);
    debug_hw_minmax(s1,s2,mindir,maxdir,minus,maxus);

    fxmessage("\tperiod count       ");
    s1=snd_pcm_hw_params_get_periods_min(hw,&minus,&mindir);
    s2=snd_pcm_hw_params_get_periods_max(hw,&maxus,&maxdir);
    debug_hw_minmax(s1,s2,0,0,minus,maxus);
#endif
    }

  void debug_hw_parameters(){
#ifdef DEBUG
    int dir;
    snd_pcm_uframes_t frames;
    unsigned int us;

    fxmessage("[alsa] Hardware Parameters\n");

    dir = snd_pcm_hw_params_can_mmap_sample_resolution(hw);
    fxmessage("\tcan mmap sample resolution %s\n",DISPLAY_YESNO(dir));

    dir = snd_pcm_hw_params_can_overrange(hw);
    fxmessage("\thas overrange detection %s\n",DISPLAY_YESNO(dir));

    dir = snd_pcm_hw_params_can_pause(hw);
    fxmessage("\tcan pause %s\n",DISPLAY_YESNO(dir));

    dir = snd_pcm_hw_params_can_resume(hw);
    fxmessage("\tcan resume %s\n",DISPLAY_YESNO(dir));

    dir = snd_pcm_hw_params_can_sync_start(hw);
    fxmessage("\tcan sync start %s\n",DISPLAY_YESNO(dir));

    dir = snd_pcm_hw_params_is_batch(hw);
    fxmessage("\tis batch transfer %s\n",DISPLAY_YESNO(dir));

    dir = snd_pcm_hw_params_is_block_transfer(hw);
    fxmessage("\tis block transfer %s\n",DISPLAY_YESNO(dir));

    dir = snd_pcm_hw_params_is_double(hw);
    fxmessage("\tis double buffer %s\n",DISPLAY_YESNO(dir));

    dir = snd_pcm_hw_params_is_half_duplex(hw);
    fxmessage("\tis half duplex %s\n",DISPLAY_YESNO(dir));

    dir = snd_pcm_hw_params_is_joint_duplex(hw);
    fxmessage("\tis joint duplex %s\n",DISPLAY_YESNO(dir));

    dir = snd_pcm_hw_params_is_monotonic(hw);
    fxmessage("\tmonotonic timestamps %s\n",DISPLAY_YESNO(dir));

    if (snd_pcm_hw_params_get_channels(hw,&us)==0)
      fxmessage("\tchannel count  = %u\n",us);

    if (snd_pcm_hw_params_get_buffer_size(hw,&frames)==0)
      fxmessage("\tbuffer size    %lu frames\n",frames);

    if (snd_pcm_hw_params_get_buffer_time(hw,&us,&dir)==0)
      fxmessage("\tbuffer time    %c %u us\n",DISPLAY_DIR(dir),us);

    dir = snd_pcm_hw_params_get_fifo_size(hw);
    fxmessage("\tfifo size  = %d frames\n",dir);

    if (snd_pcm_hw_params_get_min_align(hw,&frames)==0)
      fxmessage("\tmin transfer align = %lu samples\n",frames);

    if (snd_pcm_hw_params_get_period_size(hw,&frames,&dir)==0)
      fxmessage("\tperiod size    %c %lu frames\n",DISPLAY_DIR(dir),frames);

    if (snd_pcm_hw_params_get_period_time(hw,&us,&dir)==0)
      fxmessage("\tperiod time    %c %u us\n",DISPLAY_DIR(dir),us);

    if (snd_pcm_hw_params_get_periods(hw,&us,&dir)==0)
      fxmessage("\tperiod count   %c %u\n",DISPLAY_DIR(dir),us);

    if (snd_pcm_hw_params_get_period_wakeup(pcm,hw,&us)==0)
      fxmessage("\tperiod wakeup  %s\n",DISPLAY_YESNO(us));

    if (snd_pcm_hw_params_get_rate(hw,&us,&dir)==0)
      fxmessage("\tsample rate   %c %u\n",DISPLAY_DIR(dir),us);

    if (snd_pcm_hw_params_get_rate_resample(pcm,hw,&us)==0)
      fxmessage("\tsample rate resample  %s\n",DISPLAY_YESNO(us));
#endif
    }

  void debug_sw_parameters(){
#ifdef DEBUG
    snd_pcm_uframes_t frames;
    int dir;

    fxmessage("[alsa] Software Parameters\n");

    if (snd_pcm_sw_params_get_avail_min(sw,&frames)==0)
      fxmessage("\tmin available %lu frames\n",frames);

    if (snd_pcm_sw_params_get_boundary(sw,&frames)==0)
      fxmessage("\tboundary %lu frames\n",frames);

    if (snd_pcm_sw_params_get_period_event(sw,&dir)==0)
      fxmessage("\tperiod event %d\n",dir);

    if (snd_pcm_sw_params_get_silence_size(sw,&frames)==0)
      fxmessage("\tsilence size %lu frames\n",frames);

    if (snd_pcm_sw_params_get_silence_threshold(sw,&frames)==0)
      fxmessage("\tsilence threshold %lu frames\n",frames);

    if (snd_pcm_sw_params_get_start_threshold(sw,&frames)==0)
      fxmessage("\tstart threshold %lu frames\n",frames);

    if (snd_pcm_sw_params_get_stop_threshold(sw,&frames)==0)
      fxmessage("\tstop threshold %lu frames\n",frames);
#endif
    }

protected:
  AlsaSetup(snd_pcm_t*p) : pcm(p),hw(NULL),sw(NULL) {
    }

  ~AlsaSetup() {
    snd_pcm_hw_params_free(hw);
    snd_pcm_sw_params_free(sw);
    }

  FXbool init() {
    snd_pcm_hw_params_malloc(&hw);
    snd_pcm_sw_params_malloc(&sw);

    /// blocking while configuring
    if (snd_pcm_nonblock(pcm,0)<0)
      return false;

    /// Get all configurations
    if (snd_pcm_hw_params_any(pcm,hw)<0)
      return false;

    debug_hw_caps();

    return true;
    }


  FXbool matchFormat(const AudioFormat & in,AudioFormat & out,AlsaConfig & config) {
    int dir  = 0;
    out      = in;
    channels = out.channels;
    rate     = out.rate;
    format   = alsaformat(out);

    if (format==SND_PCM_FORMAT_UNKNOWN)
      return false;

    /// Find closest matching format based on what we can handle and what alsa offers
    while(snd_pcm_hw_params_test_format(pcm,hw,format)<0) {

      // Try a simple swap
      if (out.swap()) {
        if (to_alsa_format(out,format) && snd_pcm_hw_params_test_format(pcm,hw,format)==0)
          break;
        out.swap();
        }

      // Try a compatible format.
      if (!out.compatible() || (format=alsaformat(out))==SND_PCM_FORMAT_UNKNOWN)
        return false;

      }

    if (snd_pcm_hw_params_set_format(pcm,hw,format)<0)
      return false;

    if (snd_pcm_hw_params_set_channels_near(pcm,hw,&channels)<0)
      return false;

    if (snd_pcm_hw_params_set_rate_near(pcm,hw,&rate,&dir)<0)
      return false;

    if (config.flags&AlsaConfig::DeviceMMap) {
      if (snd_pcm_hw_params_set_access(pcm,hw,SND_PCM_ACCESS_MMAP_INTERLEAVED)<0) {
        if (snd_pcm_hw_params_set_access(pcm,hw,SND_PCM_ACCESS_RW_INTERLEAVED)<0)
          return false;

        config.flags&=~AlsaConfig::DeviceMMap;
        }
      }
    else {
      if (snd_pcm_hw_params_set_access(pcm,hw,SND_PCM_ACCESS_RW_INTERLEAVED)<0)
        return false;
      }

    return true;
    }


  FXbool finish(AudioFormat & af,FXbool & can_pause,FXbool & can_resume) {
    af.rate     = rate;
    af.channels = channels;
    can_pause   = snd_pcm_hw_params_can_pause(hw);
    can_resume  = snd_pcm_hw_params_can_resume(hw);

    debug_hw_parameters();
    debug_sw_parameters();

    if (snd_pcm_nonblock(pcm,1)<0)
      return false;

    return true;
    }

  FXbool setupHardware() {
    //unsigned int buffer_time = 1000000; // 1 sec
    //unsigned int periods     = 5;       // periods every 200ms
    //int dir=0;

    //if (snd_pcm_hw_params_set_buffer_time_near(pcm,hw,&buffer_time,&dir)<0)
    //  return false;

    //if (snd_pcm_hw_params_set_periods_near(pcm,hw,&periods,&dir)<0)
    //  return false;

    if (snd_pcm_hw_params(pcm,hw)<0)
      return false;

    if (snd_pcm_hw_params_current(pcm,hw)<0)
      return false;

    return getHardware();
    }

  FXbool getHardware() {
    int dir=0;

    if (snd_pcm_hw_params_get_rate(hw,&rate,&dir)<0)
      return false;

    if (snd_pcm_hw_params_get_channels(hw,&channels)<0)
      return false;

    if (snd_pcm_hw_params_get_period_size(hw,&period_size,&dir)<0)
      return false;

    if (snd_pcm_hw_params_get_buffer_size(hw,&buffer_size)<0)
      return false;

    return true;
    }

  FXbool setupSoftware() {

    if (snd_pcm_sw_params_set_avail_min(pcm,sw,period_size)<0)
      return false;

    if (snd_pcm_sw_params_set_start_threshold(pcm,sw,period_size)<0)
      return false;

    if (snd_pcm_sw_params_set_stop_threshold(pcm,sw,buffer_size)<0)
      return false;

#if SND_LIB_VERSION < ALSA_VERSION(1,0,16)
    if (snd_pcm_sw_params_set_xfer_align(pcm,sw,1)<0)
      return false;
#endif

    return true;
    }

public:

  static FXbool configure(snd_pcm_t * pcm,AlsaConfig & config,const AudioFormat & in,AudioFormat & out,FXbool & can_pause,FXbool & can_resume) {
    AlsaSetup alsa(pcm);

    // Init structures
    if (!alsa.init())
      return true;

    /// Match Format
    if (!alsa.matchFormat(in,out,config))
      return false;

    /// Configure Device
    if (!alsa.setupHardware())
      return false;

    /// Set the software parameters
    if (!alsa.setupSoftware())
      return false;

    /// Finish up and get the Configured Format
    if (!alsa.finish(out,can_pause,can_resume))
      return false;


    return true;
    }

  };




AlsaOutput::AlsaOutput() : OutputPlugin(), handle(NULL),mixer(NULL),mixer_element(NULL),can_pause(false),can_resume(false) {
  }

AlsaOutput::~AlsaOutput() {
  close();
  }


FXbool AlsaOutput::setOutputConfig(const OutputConfig & c) {
  config=c.alsa;
  return true;
  }


static snd_mixer_elem_t * find_mixer_element_by_name(snd_mixer_t * mixer,const FXchar * name){
  long volume;
  for (snd_mixer_elem_t * element = snd_mixer_first_elem(mixer);element;element=snd_mixer_elem_next(element)){

    /* Filter out the obvious ones */
    if (!snd_mixer_selem_is_active(element) ||
         snd_mixer_elem_get_type(element)!=SND_MIXER_ELEM_SIMPLE ||
        !snd_mixer_selem_has_playback_volume(element))
      continue;

    /* Check if we can query the volume */
    if (snd_mixer_selem_get_playback_volume(element,SND_MIXER_SCHN_FRONT_LEFT,&volume)<0 ||
        snd_mixer_selem_get_playback_volume(element,SND_MIXER_SCHN_FRONT_RIGHT,&volume)<0 ){
      continue;
      }

    /* If we don't know what we're looking for, return first one found */
    if (name==NULL)
      return element;

    /* Check if this is the one we want */
    if (comparecase(snd_mixer_selem_get_name(element),name)==0)
      return element;

    }
  return NULL;
  }

FXbool AlsaOutput::open() {
  FXint result;
  if (handle==NULL) {

    if ((result=snd_pcm_open(&handle,config.device.text(),SND_PCM_STREAM_PLAYBACK,0))<0) {
      GM_DEBUG_PRINT("[alsa] Unable to open device \"%s\": %s\n",config.device.text(),snd_strerror(result));
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

      /* Yay... let's guess what mixer we want */
      mixer_element = find_mixer_element_by_name(mixer,"PCM");
      if (mixer_element==NULL) {
        mixer_element = find_mixer_element_by_name(mixer,"MASTER");
        if (mixer_element==NULL) {
          mixer_element = find_mixer_element_by_name(mixer,NULL);
          }
        }

#ifdef DEBUG
      if (mixer_element)
        GM_DEBUG_PRINT("[alsa] Using mixer element: %s\n",snd_mixer_selem_get_name(mixer_element));
#endif
      if (mixer_element==NULL) {
        snd_mixer_close(mixer);
        mixer=NULL;
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
    if (snd_pcm_delay(handle,&nframes)!=0){
      GM_DEBUG_PRINT("[alsa] failed to get delay\n");
      return 0;
      }
    if (nframes<0) {
      GM_DEBUG_PRINT("[alsa] delay was negative\n");
      return 0;
      }
    }
  return nframes;
  }

void AlsaOutput::close() {
  GM_DEBUG_PRINT("[alsa] closing device\n");
  if (handle) {
    snd_pcm_drop(handle);
    if (mixer) {
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
    snd_pcm_reset(handle);
    snd_pcm_drop(handle);
    }
  }

void AlsaOutput::drain() {
  if (__likely(handle)) {
    snd_pcm_drain(handle);
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
  if (__unlikely(handle==NULL)) {
    if (!open()) {
      return false;
      }
    }

  if (fmt==af) {
    return true;
    }

  if (!AlsaSetup::configure(handle,config,fmt,af,can_pause,can_resume)) {
    GM_DEBUG_PRINT("[alsa] error configuring device\n");
    af.reset();
    return false;
    }
  return true;
  }


FXbool AlsaOutput::write(const void * buffer,FXuint nframes){
  int result;
  snd_pcm_sframes_t navailable;
  snd_pcm_sframes_t nwritten;
  snd_pcm_state_t   state;
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
          if (navailable<nframes /*&& navailable<(snd_pcm_sframes_t)periodsize*/) {
            result = snd_pcm_wait(handle,500);
            if (result<0) {
              /// Underrun / Suspended
              if (result==-EPIPE || result==-ESTRPIPE) {
                GM_DEBUG_PRINT("[alsa] %s\n",snd_strerror(result));
                continue;
                }
              return false;
              }
            navailable = snd_pcm_avail_update(handle);
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
            //if (snd_pcm_state(handle)!=SND_PCM_STATE_RUNNING) {
            //  GM_DEBUG_PRINT("[alsa] not running\n");
            //  }
            }
        } break;
      }
    }
  return true;
  }

}
