#include "gmdefs.h"
#include "GMApp.h"
#include "GMTrack.h"

#include <ap.h>
#include "GMAudioPlayer.h"

FXDEFMAP(GMAudioPlayer) GMAudioPlayerMap[]={
  FXMAPFUNC(SEL_CHANGED,GMAudioPlayer::ID_AUDIO_ENGINE,GMAudioPlayer::onEngineEvents)
  };

FXIMPLEMENT(GMAudioPlayer,AudioPlayer,GMAudioPlayerMap,ARRAYNUMBER(GMAudioPlayerMap));

GMAudioPlayer::GMAudioPlayer(FXApp * app,FXObject * tgt,FXSelector sel) {
  fifo = new FXAppQueue(app,this,ID_AUDIO_ENGINE);
  setEventQueue(fifo);
  target=tgt;
  message=sel;
  state=PLAYER_STOPPED;
  }

GMAudioPlayer::~GMAudioPlayer() {
  delete fifo;
  }


void GMAudioPlayer::saveSettings() {
  OutputConfig config;
  getOutputConfig(config);
  FXSettings settings;
  config.save(settings);
  settings.unparseFile(GMApp::getConfigDirectory(true)+PATHSEPSTRING+"gap.conf");
  }

void GMAudioPlayer::loadSettings() {
  OutputConfig config;
//  getOutputConfig(config);
  FXSettings settings;
  if (settings.parseFile(GMApp::getConfigDirectory()+PATHSEPSTRING+"gap.conf")){
    config.load(settings);
    setOutputConfig(config);
    }
  }


long GMAudioPlayer::onEngineEvents(FXObject*,FXSelector,void*){
  Event * event=NULL;
  while((event=pop())!=NULL) {
    switch(event->type) {
      case AP_EOS              : if (target) target->handle(this,FXSEL(SEL_PLAYER_EOS,message),NULL); break;
      case AP_BOS              : if (target) target->handle(this,FXSEL(SEL_PLAYER_BOS,message),NULL); break;
      case AP_STATE_READY      : state=PLAYER_STOPPED; if (target) target->handle(this,FXSEL(SEL_PLAYER_STATE,message),(void*)(FXival)state);break;
      case AP_STATE_PLAYING    : state=PLAYER_PLAYING; if (target) target->handle(this,FXSEL(SEL_PLAYER_STATE,message),(void*)(FXival)state);break;
      case AP_STATE_PAUSING    : state=PLAYER_PAUSING; if (target) target->handle(this,FXSEL(SEL_PLAYER_STATE,message),(void*)(FXival)state);break;
      case AP_TIMESTAMP        :
        {
            time.position = ((TimeUpdate*)event)->position;
            time.length   = ((TimeUpdate*)event)->length;
            if (target) {
              target->handle(this,FXSEL(SEL_PLAYER_TIME,message),&time);
              }
        } break;

      case AP_ERROR                  :
        {
          ErrorMessage * err = dynamic_cast<ErrorMessage*>(event);
          //fxmessage("[ERROR] %s\n",err->msg.text());
          if (target) target->handle(this,FXSEL(SEL_PLAYER_ERROR,message),(void*)&err->msg);
        } break;
      case AP_META_INFO              :
        {
          GMTrack track;
          MetaInfo * info = dynamic_cast<MetaInfo*>(event);

          /// get data
          track.title.adopt(info->title);
          track.artist.adopt(info->artist);
          track.album.adopt(info->album);

          target->handle(this,FXSEL(SEL_PLAYER_META,message),&track);
        } break;
      case AP_VOLUME_NOTIFY          :
        {
            VolumeNotify * info = dynamic_cast<VolumeNotify*>(event);
            FXint value;
            if (info->volume.enabled)
              value = (FXint)(info->volume.value * 100.0f);
            else
              value = -1;

            target->handle(this,FXSEL(SEL_PLAYER_VOLUME,message),(void*)(FXival)value);
            break;
        }
      default: break;
      }
    Event::unref(event);
    }
  return 1;
  }

