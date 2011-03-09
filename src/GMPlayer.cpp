/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2006-2010 by Sander Jansen. All Rights Reserved      *
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
#include "gmdefs.h"
#include "gmutils.h"
#include "GMTrack.h"
#include "GMTrackDatabase.h"
#include "GMList.h"
#include "GMTrackList.h"
#include "GMPlayerManager.h"
#include "GMPlayer.h"
#include "GMWindow.h"
#include "GMRemote.h"
#include "GMApp.h"

#include <xine.h>



GMEQBands::GMEQBands() {
  for (FXint i=0;i<10;i++)
    bands[i]=0;
  }

GMEQBands::GMEQBands(FXdouble e0,FXdouble e1,FXdouble e2,FXdouble e3,FXdouble e4,FXdouble e5,FXdouble e6,FXdouble e7,FXdouble e8,FXdouble e9){
  bands[0]=e0;
  bands[1]=e1;
  bands[2]=e2;
  bands[3]=e3;
  bands[4]=e4;
  bands[5]=e5;
  bands[6]=e6;
  bands[7]=e7;
  bands[8]=e8;
  bands[9]=e9;
  }

GMEQBands::GMEQBands(const GMEQBands & v){
  for (FXint i=0;i<10;i++)
    bands[i]=v.bands[i];
  }


void GMEQBands::unparse(FXString & preset) const {
#if FOXVERSION < FXVERSION(1,7,0)
#ifdef HAVE_NLS
  /// Change the current locale of the current thread only.
  locale_t old = uselocale(GMApp::instance()->clocale);
#endif

  preset+=GMStringFormat("%lg,%lg,%lg,%lg,%lg,%lg,%lg,%lg,%lg,%lg",bands[0],bands[1],bands[2],bands[3],bands[4],bands[5],bands[6],bands[7],bands[8],bands[9]);

#ifdef HAVE_NLS
  /// Change back to old locale
  uselocale(old);
#endif
#else
  preset+=GMStringFormat("%lg,%lg,%lg,%lg,%lg,%lg,%lg,%lg,%lg,%lg",bands[0],bands[1],bands[2],bands[3],bands[4],bands[5],bands[6],bands[7],bands[8],bands[9]);
#endif
  }

void GMEQBands::parse(const FXString & preset) {
  if (!preset.empty()) {
    for (FXint i=0;i<10;i++)
      bands[i]=gm_parse_number(preset.section(',',i));
    }
  }



GMEqualizer::GMEqualizer() : preamp(0),enabled(false) {
  }

GMEqualizer::GMEqualizer(const GMEQBands & v) : bands(v),preamp(0),enabled(true) {
  }

void GMEqualizer::load(FXSettings & settings) {
  enabled=settings.readBoolEntry("audio-plugins","equalizer",enabled);
  FXString entry = settings.readStringEntry("audio-plugins","equalizer-bands",NULL);
  bands.parse(entry);
  }

void GMEqualizer::save(FXSettings & settings) const {
  settings.writeBoolEntry("audio-plugins","equalizer",enabled);
  FXString entry;
  bands.unparse(entry);
  settings.writeStringEntry("audio-plugins","equalizer-bands",entry.text());
  }


FXDEFMAP(GMPlayer) GMPlayerMap[]={
  FXMAPFUNCS(SEL_UPDATE,GMPlayer::ID_EQ_30HZ,GMPlayer::ID_EQ_16000HZ,GMPlayer::onUpdEqualizer),
  FXMAPFUNC(SEL_UPDATE,GMPlayer::ID_PREAMP,GMPlayer::onUpdPreamp),

  FXMAPFUNC(SEL_UPDATE,GMPlayer::ID_VOLUME,GMPlayer::onUpdVolume),
  FXMAPFUNC(SEL_UPDATE,GMPlayer::ID_MUTE,GMPlayer::onUpdMute),
  FXMAPFUNC(SEL_UPDATE,GMPlayer::ID_UNMUTE,GMPlayer::onUpdUnMute),
  FXMAPFUNC(SEL_UPDATE,GMPlayer::ID_TOGGLE_MUTE,GMPlayer::onUpdToggleMute),
  FXMAPFUNCS(SEL_COMMAND,GMPlayer::ID_EQ_30HZ,GMPlayer::ID_EQ_16000HZ,GMPlayer::onCmdEqualizer),
  FXMAPFUNCS(SEL_CHANGED,GMPlayer::ID_EQ_30HZ,GMPlayer::ID_EQ_16000HZ,GMPlayer::onCmdEqualizer),
  FXMAPFUNC(SEL_COMMAND,GMPlayer::ID_PREAMP,GMPlayer::onCmdPreamp),
  FXMAPFUNC(SEL_CHANGED,GMPlayer::ID_PREAMP,GMPlayer::onCmdPreamp),

  FXMAPFUNC(SEL_COMMAND,GMPlayer::ID_VOLUME,GMPlayer::onCmdVolume),
  FXMAPFUNC(SEL_CHANGED,GMPlayer::ID_VOLUME,GMPlayer::onCmdVolume),
  FXMAPFUNC(SEL_COMMAND,GMPlayer::ID_MUTE,GMPlayer::onCmdMute),
  FXMAPFUNC(SEL_COMMAND,GMPlayer::ID_UNMUTE,GMPlayer::onCmdUnMute),
  FXMAPFUNC(SEL_COMMAND,GMPlayer::ID_TOGGLE_MUTE,GMPlayer::onCmdToggleMute),

  };

FXIMPLEMENT(GMPlayer,FXObject,GMPlayerMap,ARRAYNUMBER(GMPlayerMap))

GMPlayer::GMPlayer(){
  }

GMPlayer::GMPlayer(FXApp*,int argc,char** argv,FXObject* tgt,FXSelector sel) : FXObject(), target(tgt), message(sel), xine(NULL),ao(NULL),so(NULL),queue(NULL), post_volume_normalize(NULL) {
  position=0;
  ctime=0;
  ttime=0;
  hours=0;
  minutes=0;
  seconds=0;
  volume=0;
  repeat_a=-1;
  repeat_b=-1;
  ignore_uimsg=false;
  debug=false;

  for (FXint i=1;i<argc;i++){
    if (comparecase(argv[i],"--xine-debug")==0){
      debug=true;
      break;
      }
    }

  /// Load the equalizer from registry
  equalizer.load(FXApp::instance()->reg());
  }


GMPlayer::~GMPlayer(){
  exit();
  }


FXbool GMPlayer::opened() const {
  if (xine && so) return true;
  else return false;
  }


static void gm_xine_event(void *user_data,const xine_event_t *event){
  fxmessage("xine event ");
  switch(event->type){
    case XINE_EVENT_UI_PLAYBACK_FINISHED: fxmessage("playback finished\n"); break;
    case XINE_EVENT_UI_CHANNELS_CHANGED	: fxmessage("channels changed\n"); break;
    case XINE_EVENT_UI_SET_TITLE        : fxmessage("set title\n"); break;
    case XINE_EVENT_UI_MESSAGE          : fxmessage("message\n"); break;
    case XINE_EVENT_FRAME_FORMAT_CHANGE : fxmessage("frame\n"); break;
    case XINE_EVENT_AUDIO_LEVEL         : fxmessage("audio level\n"); break;
    case XINE_EVENT_QUIT								: fxmessage("quit\n"); break;
    case XINE_EVENT_PROGRESS						: fxmessage("progress callback\n"); break;
#ifndef XINE_EVENT_MRL_REFERENCE_EXT
    case XINE_EVENT_MRL_REFERENCE			  : fxmessage("new mrl ref.\n"); break;
#else
    case XINE_EVENT_MRL_REFERENCE_EXT		: fxmessage("new mrl ref.\n"); break;
#endif
    case XINE_EVENT_UI_NUM_BUTTONS			: fxmessage("num buttons\n");break;
    case XINE_EVENT_SPU_BUTTON					: fxmessage("spu buttons\n");break;
    case XINE_EVENT_DROPPED_FRAMES			: fxmessage("drop frames\n");break;
    case XINE_EVENT_AUDIO_AMP_LEVEL			: fxmessage("amp level\n");break;
    default: fxmessage("unhandled event %d\n",event->type); break;
    }
  }















FXbool GMPlayer::init_internal() {
  if (xine == NULL) {

    /// Create Xine
    xine  = xine_new();

    /// load xine config file and init xine
    FXString config = GMApp::getConfigDirectory()+PATHSEPSTRING "xineconf";
    xine_config_load(xine,config.text());

    /// Init the Engine
    xine_init(xine);

    /// Enable Debug Output
    if (debug) xine_engine_set_param(xine,XINE_ENGINE_PARAM_VERBOSITY,XINE_VERBOSITY_DEBUG);
    }

  /// Open Audio Driver
  if (ao == NULL) {

    FXString audio_driver = FXApp::instance()->reg().readStringEntry("xine","driver",NULL);

    if (!audio_driver.empty() && audio_driver!="auto") {
      ao = xine_open_audio_driver(xine,audio_driver.text(),NULL);
      if (ao) FXApp::instance()->reg().writeStringEntry("xine","driver",audio_driver.text());
      }

    if (ao == NULL) {
      const char *const * plugins = xine_list_audio_output_plugins(xine);
      for (FXint i=0;plugins[i]!=NULL;i++){
        if (compare(plugins[i],"none") && compare(plugins[i],"file") ){
          ao = xine_open_audio_driver(xine,plugins[i],NULL);
          if (ao) {
            FXApp::instance()->reg().writeStringEntry("xine","driver",plugins[i]);
            break;
            }
          }
        }
      }

    if (ao == NULL ) {
      msg=fxtr("Unable to initialize audio driver.");
      return false;
      }
   }

  FXASSERT(so==NULL);

  /// Open Default Stream
  so = xine_stream_new(xine,ao,NULL);
  if (so == NULL) {
    msg="Unable to create a new stream for playback.";
    xine_close_audio_driver(xine,ao);
    return false;
    }


  if (FXApp::instance()->reg().readBoolEntry("audio-plugins","volume-normalization",false) &&
    hasVolumeNormalization()) {
    setVolumeNormalization(true);
    }

  /// Create Event Queue for Stream
  queue = xine_event_new_queue(so);

  xine_event_create_listener_thread(queue,gm_xine_event,this);


  /// Init Volume
  volume = xine_get_param(so,XINE_PARAM_AUDIO_VOLUME);

  /// Faster Seeking
  //xine_set_param(so,XINE_PARAM_METRONOM_PREBUFFER,6000);

  /// Ignore Video
  xine_set_param(so,XINE_PARAM_IGNORE_VIDEO,1);

  /// Ignore SPU
  xine_set_param(so,XINE_PARAM_IGNORE_SPU,1);

  /// Init the Equalizer
  setEqualizer(equalizer);

  /// Debug Output
  if (debug) xine_set_param(so,XINE_PARAM_VERBOSITY,XINE_VERBOSITY_DEBUG);

  setupGapless();
  return true;
  }

void GMPlayer::getCurrentDriver(FXString & driver) {
  driver=FXApp::instance()->reg().readStringEntry("xine","driver","auto");
  }

FXint GMPlayer::getAvailableDrivers(FXString & drivers) {
  FXint ndrivers=0;
  const char *const * plugins = xine_list_audio_output_plugins(xine);
  for (FXint i=0;plugins[i]!=NULL;i++){
    if (compare(plugins[i],"none") && compare(plugins[i],"file") ){
      if (!drivers.empty()) drivers+="\n";
      drivers+=plugins[i];
      ndrivers++;
      }
    }
  return ndrivers;
/*
  xine_cfg_entry_t entry;
  for (xine_config_get_first_entry(xine,&entry);xine_config_get_next_entry(xine,&entry);){
    if (entry.exp_level==0) fxmessage("key %d: %s\n",entry.exp_level,entry.key);
    }
*/
  }


void GMPlayer::setupGapless() {
  if (so) {
    if (GMPlayerManager::instance()->getPreferences().play_gapless) {
      xine_set_param(so,XINE_PARAM_EARLY_FINISHED_EVENT,1);
      }
    else {
      xine_set_param(so,XINE_PARAM_EARLY_FINISHED_EVENT,0);
      }
    }
  }

FXbool GMPlayer::hasVolumeNormalization() const {
  if (post_volume_normalize) return true;
  const char * const * filters = xine_list_post_plugins_typed(xine,XINE_POST_TYPE_AUDIO_FILTER);
  for (FXint i=0;filters[i];i++){
    if (comparecase(filters[i],"volnorm")==0)
      return true;
    }
  return false;
  }


void GMPlayer::setVolumeNormalization(FXbool enable){
  if (enable) {

    if (post_volume_normalize)
      return;

    xine_audio_port_t * targets[2]={ao,NULL};

    post_volume_normalize = xine_post_init(xine,"volnorm",1,targets,NULL);
    if (post_volume_normalize) {

      xine_post_out_t * source =  xine_get_audio_source(so);

      xine_post_wire_audio_port(source,post_volume_normalize->audio_input[0]);

      FXApp::instance()->reg().writeBoolEntry("audio-plugins","volume-normalization",true);
      }

    }
  else {
    if (!post_volume_normalize)
      return;

    xine_post_out_t * source =  xine_get_audio_source(so);

    xine_post_wire_audio_port(source,ao);

    xine_post_dispose(xine,post_volume_normalize);

    post_volume_normalize = NULL;

    FXApp::instance()->reg().writeBoolEntry("audio-plugins","volume-normalization",false);
    }
  }





void GMPlayer::getErrorMessage(FXString & errormsg) {
  errormsg=msg;
  }

void GMPlayer::check_xine_error() {
  FXint error = xine_get_error(so);
  switch(error) {
    case XINE_ERROR_NO_INPUT_PLUGIN:
      msg="No input plugin found to play:\n" + mrl;
      break;
    case XINE_ERROR_NO_DEMUX_PLUGIN:
      msg="No demux plugin found to play:\n" + mrl;
      break;
    case XINE_ERROR_DEMUX_FAILED:
      msg="Unable to play:\n" + mrl;
      break;
    case XINE_ERROR_INPUT_FAILED:
      msg="Unable to open:\n" + mrl;
      break;
    case XINE_ERROR_MALFORMED_MRL:
      msg="Resource does not exist:\n" + mrl;
      break;
    case XINE_ERROR_NONE:
      msg="Nothing to report\n";
    default:
      msg="Unknown error while trying to play:\n" + mrl;
      break;
    }
  ignore_uimsg=true;
  }


FXbool GMPlayer::checkInitialized() {
  if (!so && !init_internal()) return false;
  return true;
  }

FXbool GMPlayer::init() {
  if (!so && !init_internal()) return false;
  return true;
  }




FXbool GMPlayer::open(const FXString & mrl_in){

  if (!so && !init_internal())
    return false;

  mrl=mrl_in;

  /// Make sure we encode #.
  if (mrl[0]=='/') {
    mrl.substitute("#","%23");
    mrl.prepend("file:");
    }

  /// Open Mrl
  if (xine_open(so,mrl.text())==0){
    /// Disable Gapless Playback
    xine_set_param(so,XINE_PARAM_GAPLESS_SWITCH,0);
    check_xine_error();
    return false;
    }
  progress=100;
  return true;
  }







void GMPlayer::open(const FXString & mrl_in,FXbool flush){

  if (!so && !init_internal())
    return;

  mrl=mrl_in;

  /// Make sure we encode #.
  if (mrl[0]=='/') {
    mrl.substitute("#","%23");
    mrl.prepend("file:");
    }

  /// Open Mrl
  if (xine_open(so,mrl.text())==0){
    /// Disable Gapless Playback
    xine_set_param(so,XINE_PARAM_GAPLESS_SWITCH,0);
    check_xine_error();
   // return false;
    }
  else {
    progress=100;
    if (xine_play(so,0,0)){
//      PlayerState state = PLAYER_PLAYING;
      if (target) {
        target->handle(this,FXSEL(SEL_PLAYER_STATE,message),(void*)(FXival)PLAYER_PLAYING);
        target->handle(this,FXSEL(SEL_PLAYER_BOS,message),NULL);
        }
    FXint level = getVolume();
      if (level==0) {
        level=50;
        setVolume(level);
        }
      }
    }
  }


void GMPlayer::seek(FXdouble dpos){
  FXint pos = 0xFFFF * dpos;
  FXASSERT(so);

  if (repeat_a>=0 && repeat_b>=0){
    if (pos>repeat_b || pos<repeat_a)
      pos=repeat_a;
    }

  if (xine_play(so,pos,0)){
    FXint level = getVolume();
    if (level==0) {
      level=50;
      setVolume(level);
      }
    }
  check_xine_error();
  }



























void GMPlayer::close_device(){
  if (xine && so) {
    xine_set_param(so,XINE_PARAM_AUDIO_CLOSE_DEVICE,1);
    }
  }

FXbool GMPlayer::changeDriver(const FXString & driver) {

  if (post_volume_normalize) {

    xine_post_out_t * source =  xine_get_audio_source(so);

    xine_post_wire_audio_port(source,ao);

    xine_post_dispose(xine,post_volume_normalize);

    post_volume_normalize=NULL;
    }

  if (queue) {
    xine_event_dispose_queue(queue);
    queue=NULL;
    }

  if (so) {
    stop();
    xine_dispose(so);
    so=NULL;
    }

  if (ao) {
    xine_close_audio_driver(xine,ao);
    ao=NULL;
    }


  /// Set the new driver
  FXApp::instance()->reg().writeStringEntry("xine","driver",driver.text());

  /// Initialize the system
  return init_internal();
  }


void GMPlayer::exit(){

  if (xine) {
    xine_plugins_garbage_collector(xine);

    FXString config = GMApp::getConfigDirectory()+PATHSEPSTRING "xineconf";
    xine_config_save(xine,config.text());
    }

  if (post_volume_normalize) {

    xine_post_out_t * source =  xine_get_audio_source(so);

    xine_post_wire_audio_port(source,ao);

    xine_post_dispose(xine,post_volume_normalize);

    post_volume_normalize=NULL;
    }

  if (queue) {
    xine_event_dispose_queue(queue);
    queue=NULL;
    }

  if (so) {
    stop();
    xine_dispose(so);
    so=NULL;
    }

  if (ao) {
    xine_close_audio_driver(xine,ao);
    ao=NULL;
    }

  if (xine){
    xine_exit(xine);
    xine=NULL;
    }

  equalizer.save(FXApp::instance()->reg());

  }

FXbool GMPlayer::seekable() const {
  if (xine && so && (xine_get_status(so)==XINE_STATUS_PLAY) && xine_get_stream_info(so,XINE_STREAM_INFO_SEEKABLE))
    return true;
  else
    return false;
  }

void GMPlayer::setRepeatAB() {
  if ((repeat_a>=0 && repeat_b>=0) || !seekable()) {
    repeat_a=-1;
    repeat_b=-1;
    }
  else if (repeat_a==-1 || position==repeat_a)
    repeat_a=position;
  else if (repeat_a<position)
    repeat_b=position;
  else {
    repeat_b=repeat_a;
    repeat_a=position;
    }
  }

FXuint GMPlayer::getRepeatAB() const {
  if (repeat_b!=-1) return REPEAT_AB_B;
  else if (repeat_a!=-1) return REPEAT_AB_A;
  else return REPEAT_AB_OFF;
  }


FXbool GMPlayer::play(FXint pos){
  FXASSERT(so);


  if (repeat_a>=0 && repeat_b>=0){
    if (pos>repeat_b || pos<repeat_a)
      pos=repeat_a;
    }

  if (xine_play(so,pos,0)){
    FXint level = getVolume();
    if (level==0) {
      level=50;
      setVolume(level);
      }
    return true;
    }
  check_xine_error();
  return false;
  }


void GMPlayer::stop() {
  if (so) {
    xine_close(so);
    repeat_a=repeat_b=-1;
    }
  }


void GMPlayer::pause(){
  setSpeed(XINE_SPEED_PAUSE);
  }

void GMPlayer::unpause(){
  setSpeed(XINE_SPEED_NORMAL);
  volume = xine_get_param(so,XINE_PARAM_AUDIO_VOLUME);
  }

FXbool GMPlayer::pausing(){
  if (getSpeed()==XINE_SPEED_PAUSE && progress>=100)
    return true;
  else
    return false;
  }

FXbool GMPlayer::playing() const {
  return (so && xine_get_status(so)==XINE_STATUS_PLAY);
  }

void GMPlayer::setSpeed(FXint speed){
  if (so) xine_set_param(so,XINE_PARAM_SPEED,speed);
  }

FXint GMPlayer::getSpeed() const {
  return so ? xine_get_param(so,XINE_PARAM_SPEED) : 0;
  }


void GMPlayer::incSpeed(){
  FXint oldspeed=getSpeed();
  if (oldspeed>=XINE_SPEED_FAST_4) return;
  setSpeed(oldspeed*2);
  }

void GMPlayer::decSpeed(){
  FXint oldspeed=getSpeed();
  if (oldspeed<=XINE_SPEED_SLOW_4) return;
  setSpeed(oldspeed/2);
  }


void GMPlayer::handle_async_events(){
#if 0
  if (queue == NULL) return;

  xine_event_t * event				  =NULL;
  xine_ui_message_data_t * data =NULL;
  FXString uimsg;
static FXString newmrl;


  /// First Get All Pending Events
  while((event = xine_event_get(queue))!=NULL){
    switch(event->type){
      case XINE_EVENT_AUDIO_LEVEL					:
        volume=static_cast<xine_audio_level_data_t*>(event->data)->left;
        GMPlayerManager::instance()->getMainWindow()->update_volume_display(volume);
        if (GMPlayerManager::instance()->getRemote()) GMPlayerManager::instance()->getRemote()->update_volume_display(volume);
        break;
      case XINE_EVENT_UI_CHANNELS_CHANGED	:
        break;
      case XINE_EVENT_UI_SET_TITLE				:
        //fxmessage("Title Changed\n");
        GMPlayerManager::instance()->update_track_display();
        break;
      case XINE_EVENT_FRAME_FORMAT_CHANGE : break;
      case XINE_EVENT_QUIT								: break;
      case XINE_EVENT_PROGRESS						:
        //fxmessage("progress callback %d\n",xine_get_status(so));
        if (xine_get_status(so)==XINE_STATUS_PLAY) {
          progress=static_cast<xine_progress_data_t*>(event->data)->percent;
          if (progress>=100){
            //fxmessage("Update track display %d\n",progress);
            GMPlayerManager::instance()->update_track_display();
            }
          else {
            FXString status;
            status.format("%s - %d%%\n",static_cast<xine_progress_data_t*>(event->data)->description,static_cast<xine_progress_data_t*>(event->data)->percent);
            GMPlayerManager::instance()->setStatus(status);
            }
          }
        break;
      case XINE_EVENT_MRL_REFERENCE_EXT  	:
        newmrl=static_cast<xine_mrl_reference_data_ext_t*>(event->data)->mrl;
//        fxmessage("Received MRL reference: %s\n",static_cast<xine_mrl_reference_data_ext_t*>(event->data)->mrl);
//        GMPlayerManager::instance()->play(static_cast<xine_mrl_reference_data_ext_t*>(event->data)->mrl);
        break;
      case XINE_EVENT_UI_NUM_BUTTONS			: break;
      case XINE_EVENT_SPU_BUTTON					: break;
      case XINE_EVENT_DROPPED_FRAMES			: break;
      case XINE_EVENT_UI_PLAYBACK_FINISHED:

//        fxmessage("playback finished %d\n",xine_get_status(so));
        if (!newmrl.empty()) {
          if (xine_get_status(so)==XINE_STATUS_STOP) {
            GMPlayerManager::instance()->play(newmrl);
            }
          newmrl=FXString::null;
          }
        else {

        if (GMPlayerManager::instance()->getPreferences().play_gapless && !GMPlayerManager::instance()->playlist_empty() && gm_is_local_file(mrl) ){
          xine_set_param(so,XINE_PARAM_GAPLESS_SWITCH,1);
          }

        /// For very short tracks we need to get the latest time.
        xine_get_pos_length(so,&position,&ctime,&ttime);

        GMPlayerManager::instance()->notify_playback_finished();
        /*
          Return here so we don't want to update the time. Xine will report 0,
          if we call it too soon which we don't want.
        */
        xine_event_free(event);
        return;
        }
        break;

      case XINE_EVENT_UI_MESSAGE					:
        //fxmessage("UI message\n");
        if (!ignore_uimsg) {
          data = (xine_ui_message_data_t*)event->data;
          switch(data->type) {
            case XINE_MSG_NO_ERROR							: break;
            case XINE_MSG_ENCRYPTED_SOURCE			: break;
            case XINE_MSG_UNKNOWN_HOST					: uimsg = fxtr("Unknown host.");   							break;
            case XINE_MSG_UNKNOWN_DEVICE				: uimsg = fxtr("Unknown device"); 							break;
            case XINE_MSG_NETWORK_UNREACHABLE		: uimsg = fxtr("Network not reachable."); 			break;
            case XINE_MSG_AUDIO_OUT_UNAVAILABLE	: uimsg = fxtr("Audio output unavailable."); 		break;
            case XINE_MSG_CONNECTION_REFUSED		: uimsg = fxtr("Connection Refused.");					break;
            case XINE_MSG_FILE_NOT_FOUND				: uimsg = fxtr("File not found.");							break;
            case XINE_MSG_PERMISSION_ERROR			: uimsg = fxtr("Resource not accessible. Check permissions");	break;
            case XINE_MSG_READ_ERROR						: uimsg = fxtr("Read Error");										break;
            case XINE_MSG_LIBRARY_LOAD_ERROR		: uimsg = fxtr("Error while loading library/plugin");	break;
            case XINE_MSG_GENERAL_WARNING				: uimsg = fxtr("Warning");											break;
            case XINE_MSG_SECURITY							: uimsg = fxtr("Security Warning");							break;
            default															: uimsg = fxtr("Unknown Error"); 								break;
            }
          }
        ignore_uimsg=false;
        break;
      default: /*unhandled event*/ break;
      }
    xine_event_free(event);
    event=NULL;
    }


  if (xine_get_status(so)==XINE_STATUS_PLAY) {
    if (xine_get_param(so,XINE_PARAM_SPEED)!=XINE_SPEED_PAUSE) {
      if (xine_get_pos_length(so,&position,&ctime,&ttime)){
        FXint time=ctime;
        hours 	= (FXint) floor((double)time/3600000.0);
        time   -= (FXint) (3600000.0*hours);
        minutes = (FXint) floor((double)time/60000.0);
        time   -= (FXint) (60000.0*minutes);
        seconds = (FXint) floor((double)time/1000.0);
        }
      }

    if (repeat_b>=0 && repeat_a>=0 && repeat_b>repeat_a && (position>repeat_b || position<repeat_a))
      xine_play(so,repeat_a,0);

    }
  else {
    hours 	= 0;
    minutes = 0;
    seconds = 0;
    }

  if (!uimsg.empty()){
    GMPlayerManager::instance()->stop();
    GMPlayerManager::instance()->show_message(fxtr("Error"),uimsg.text());
    }
#endif
  }

/// Audio Controls

FXint GMPlayer::remaining() const {
  FXint r = ttime-ctime;
  return (FXint) floor((double)r/1000.0);
  }

FXint GMPlayer::getVolume() const {
  return volume;
  }

void GMPlayer::setVolume(FXint level){
  if (so == NULL) return;
  volume=level;

  //xine_set_param( so, XINE_PARAM_AUDIO_AMP_LEVEL,(FXuint)(level*preamp));


  xine_set_param(so,XINE_PARAM_AUDIO_VOLUME,level);
  if (level==0 && !pausing() ) pause();
  if (level>0 && pausing() )  unpause();
  }

FXbool GMPlayer::isMute() const {
  return so ? xine_get_param(so,XINE_PARAM_AUDIO_MUTE) : true;
  }

void GMPlayer::mute(){
  if (so) xine_set_param(so,XINE_PARAM_AUDIO_MUTE,1);
  }

void GMPlayer::unmute(){
  if (so) xine_set_param(so,XINE_PARAM_AUDIO_MUTE,0);
  }


void GMPlayer::disableEqualizer() {
  equalizer.enabled=false;
  if (so) {
    for (FXint i=0;i<10;i++){
      xine_set_param(so,XINE_PARAM_EQ_30HZ+i,0);
      }
    }
  }

void GMPlayer::getEqualizer(GMEqualizer & eq){
  eq=equalizer;
  }

void GMPlayer::setEqualizer(const GMEqualizer & eq) {
  FXint oldamp=equalizer.preamp;
  equalizer=eq;
  equalizer.preamp=oldamp;
  if (so) {
    if (equalizer.enabled) {
      for (FXint i=0;i<10;i++){
        xine_set_param(so,XINE_PARAM_EQ_30HZ+i,equalizer.to_xine(i));
        }
      }
    else {
      for (FXint i=0;i<10;i++){
        xine_set_param(so,XINE_PARAM_EQ_30HZ+i,0);
        }
      }
    }
  }


void GMPlayer::setReplayGain(FXdouble gain,FXdouble peak){
  if (replaygain.gain!=gain || replaygain.peak!=peak) {
    replaygain.gain=gain;
    replaygain.peak=peak;
    GM_DEBUG_PRINT("replay gain: %g %g\n",gain,peak);
    set_preamp();
    }
  }


void GMPlayer::set_preamp() {
  FXdouble scale;
  FXdouble gain_scale;

  if (!isnan(replaygain.gain)){

    gain_scale=pow(10.0,replaygain.gain/20.0);

    if (equalizer.enabled) { /// with preamp

      scale=(equalizer.preamp_scale()*gain_scale);

      /// avoid clipping
      if (!isnan(replaygain.peak) && replaygain.peak!=0.0) {
        if ((scale*replaygain.peak)>1.0){
          if ((gain_scale*replaygain.peak)>1.0)
            scale=1.0 / replaygain.peak;
          else
            scale=gain_scale;
          }
        }
      }
    else { /// no preamp
      scale = gain_scale;

      /// avoid clipping
      if (!isnan(replaygain.peak) && replaygain.peak!=0.0 && (gain_scale*replaygain.peak)>1.0)
        scale=1.0 / replaygain.peak;
      }
    xine_set_param(so,XINE_PARAM_AUDIO_AMP_LEVEL,(FXint)(100.0*scale));
    }
  else if (equalizer.enabled) { /// preamp only
    //fxmessage("preamp: %d\n",equalizer.to_xine_preamp());
    xine_set_param(so,XINE_PARAM_AUDIO_AMP_LEVEL,equalizer.to_xine_preamp());
    }
  else { /// disabled
    xine_set_param(so,XINE_PARAM_AUDIO_AMP_LEVEL,100);
    }
  }


long GMPlayer::onCmdPreamp(FXObject*sender,FXSelector,void*){
  if (so) {
    FXdouble value;
    sender->handle(this,FXSEL(SEL_COMMAND,FXWindow::ID_GETREALVALUE),(void*)&value);
    equalizer.preamp=value;//FXCLAMP(-100,value,100);
    set_preamp();
    }
  return 1;
  }


long GMPlayer::onUpdPreamp(FXObject*sender,FXSelector,void*){
  if (equalizer.enabled) {
    sender->handle(this,FXSEL(SEL_COMMAND,FXWindow::ID_ENABLE),NULL);
    sender->handle(this,FXSEL(SEL_COMMAND,FXWindow::ID_SETREALVALUE),(void*)&equalizer.preamp);
    }
  else {
    sender->handle(this,FXSEL(SEL_COMMAND,FXWindow::ID_DISABLE),NULL);
    sender->handle(this,FXSEL(SEL_COMMAND,FXWindow::ID_SETVALUE),NULL); /// for textfield NULL is string, for slider, NULL will be 0
    }
  return 1;
  }

long GMPlayer::onUpdEqualizer(FXObject*sender,FXSelector sel,void*){
  if (equalizer.enabled) {
    sender->handle(this,FXSEL(SEL_COMMAND,FXWindow::ID_ENABLE),NULL);
    sender->handle(this,FXSEL(SEL_COMMAND,FXWindow::ID_SETREALVALUE),(void*)&equalizer.bands[FXSELID(sel)-ID_EQ_30HZ]);
    }
  else {
    sender->handle(this,FXSEL(SEL_COMMAND,FXWindow::ID_DISABLE),NULL);
    sender->handle(this,FXSEL(SEL_COMMAND,FXWindow::ID_SETVALUE),NULL); /// for textfield NULL is string, for slider, NULL will be 0
    }
  return 1;
  }

long GMPlayer::onCmdEqualizer(FXObject*sender,FXSelector sel,void*){
  /// don't bother reading EQ values from xine.
  /// due to rounding errors, the correct value won't be returned.
  FXdouble value;
  FXint which=FXSELID(sel)-ID_EQ_30HZ;
  sender->handle(this,FXSEL(SEL_COMMAND,FXWindow::ID_GETREALVALUE),(void*)&value);
  equalizer.bands[which]=FXCLAMP(-12,value,12);
  if (so) {
    xine_set_param(so,XINE_PARAM_EQ_30HZ+which,equalizer.to_xine(which));
    }
  return 1;
  }


const char * GMPlayer::getVersion() const{
  return xine_get_version_string();
  }

/// Message Handlers

long GMPlayer::onCmdVolume(FXObject*,FXSelector,void*ptr){
  FXint level = (FXint)(FXival)ptr;
  setVolume(level);
  return 1;
  }

long GMPlayer::onUpdVolume(FXObject*sender,FXSelector,void*){
  FXWindow * window = (FXWindow*)sender;
  if (window->getShell()->shown()){
    sender->handle(this,FXSEL(SEL_COMMAND,FXWindow::ID_SETINTVALUE),&volume);
    return 1;
    }
  return 0;
  }

long GMPlayer::onCmdMute(FXObject*,FXSelector,void*){
  mute();
  return 1;
  }

long GMPlayer::onUpdMute(FXObject*sender,FXSelector,void*){
  if (isMute())
    sender->handle(this,FXSEL(SEL_COMMAND,FXWindow::ID_DISABLE),NULL);
  else
    sender->handle(this,FXSEL(SEL_COMMAND,FXWindow::ID_ENABLE),NULL);
  return 1;
  }

long GMPlayer::onCmdUnMute(FXObject*,FXSelector,void*){
  unmute();
  return 1;
  }

long GMPlayer::onUpdUnMute(FXObject*sender,FXSelector,void*){
  if (isMute())
    sender->handle(this,FXSEL(SEL_COMMAND,FXWindow::ID_ENABLE),NULL);
  else
    sender->handle(this,FXSEL(SEL_COMMAND,FXWindow::ID_DISABLE),NULL);
  return 1;
  }


long GMPlayer::onCmdToggleMute(FXObject*,FXSelector,void*){
  if (isMute())
    unmute();
  else
    mute();
  return 1;
  }

long GMPlayer::onUpdToggleMute(FXObject*sender,FXSelector,void*){
  if (isMute())
    sender->handle(this,FXSEL(SEL_COMMAND,FXWindow::ID_CHECK),NULL);
  else
    sender->handle(this,FXSEL(SEL_COMMAND,FXWindow::ID_UNCHECK),NULL);
  return 1;
  }


void GMPlayer::getTrackInformation(GMTrack & info){
  info.clear();
  info.mrl      = mrl;
  info.artist   = xine_get_meta_info(so,XINE_META_INFO_ARTIST);
  info.album    = xine_get_meta_info(so,XINE_META_INFO_ALBUM);
  info.title    = xine_get_meta_info(so,XINE_META_INFO_TITLE);
  info.tags.append(xine_get_meta_info(so,XINE_META_INFO_GENRE));
#if FOXVERSION >= FXVERSION(1,7,12)
  info.year     = FXString(xine_get_meta_info(so,XINE_META_INFO_YEAR)).toInt();
  info.no       = FXString(xine_get_meta_info(so,XINE_META_INFO_TRACK_NUMBER)).toInt();
#else
  info.year     = FXIntVal(xine_get_meta_info(so,XINE_META_INFO_YEAR));
  info.no       = FXIntVal(xine_get_meta_info(so,XINE_META_INFO_TRACK_NUMBER));
#endif
  info.time     = 0;
  if (xine_get_pos_length(so,NULL,NULL,&info.time)){
     info.time/=1000;
     }

  info.bitrate=xine_get_stream_info(so,XINE_STREAM_INFO_BITRATE);
  if (info.bitrate==0)
    info.bitrate=xine_get_stream_info(so,XINE_STREAM_INFO_AUDIO_BITRATE);


  //fxmessage("Playing %s - %s\n",info.artist.text(),info.title.text());
  }

FXbool GMPlayer::setStringValue(const FXString & entry,const FXString & value){
  xine_cfg_entry_t config;
  if (xine_config_lookup_entry(xine,entry.text(),&config)) {
    config.str_value = (FXchar*)value.text();
    xine_config_update_entry (xine,&config);
    return true;
    }
  return false;
  }
#if 0
void GMPlayer::list_cda_tracks(GMTrackDatabase * db,const FXString & device) {
  GMTrack track;
  FXchar ** tracks;
  FXint num_tracks;
  FXint id;

  if (!setStringValue("media.audio_cd.device",device))
    return;

  tracks = xine_get_autoplay_mrls(xine,"CD",&num_tracks);
  if (num_tracks) {
    for (int i=0;i<num_tracks;i++) {
      if (xine_open(so,tracks[i])) {
        track.mrl		 = tracks[i];
        track.artist = xine_get_meta_info(so,XINE_META_INFO_ARTIST);
        track.album  = xine_get_meta_info(so,XINE_META_INFO_ALBUM);
        track.title  = xine_get_meta_info(so,XINE_META_INFO_TITLE);
        track.genre  = xine_get_meta_info(so,XINE_META_INFO_GENRE);
        //list[i].year   = xine_get_meta_info(so,XINE_META_INFO_YEAR);
        track.no   	 = FXIntVal(xine_get_meta_info(so,XINE_META_INFO_TRACK_NUMBER));
        track.time   = 0;
        if (xine_get_pos_length(so,NULL,NULL,&track.time)){
          track.time/=1000;
          }

        track.title.trim();
        track.artist.trim();
        track.album.trim();
        track.genre.trim();

        if (track.title.empty())  track.title = GMPlayerManager::instance()->getPreferences().import_default_user_title;
        if (track.artist.empty()) track.artist = GMPlayerManager::instance()->getPreferences().import_default_user_artist;
        if (track.album.empty())  track.album = GMPlayerManager::instance()->getPreferences().import_default_user_album;
        if (track.genre.empty())  track.genre = GMPlayerManager::instance()->getPreferences().import_default_user_genre;
        if (track.no==0) track.no=i+1;
        db->insertTrack(track,id);
        }
      }
    }
  }
#endif
