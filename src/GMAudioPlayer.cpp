#ifndef HAVE_XINE_LIB

#include "gmdefs.h"

#include <ap.h>
#include "GMAudioPlayer.h"

FXDEFMAP(GMAudioPlayer) GMAudioPlayerMap[]={
  FXMAPFUNC(SEL_CHANGED,GMAudioPlayer::ID_AUDIO_ENGINE,GMAudioPlayer::onEngineEvents)
  };

FXIMPLEMENT(GMAudioPlayer,AudioPlayer,GMAudioPlayerMap,ARRAYNUMBER(GMAudioPlayerMap));

GMAudioPlayer::GMAudioPlayer(FXApp * app,FXObject * tgt,FXSelector sel) : AudioPlayer(NULL) {
  setEventQueue(new FXAppQueue(app,this,ID_AUDIO_ENGINE));
  target=tgt;
  message=sel;
  state=PLAYER_STOPPED;
  }

GMAudioPlayer::~GMAudioPlayer() {
  }


long GMAudioPlayer::onEngineEvents(FXObject*,FXSelector,void* ptr){
  Event * event=NULL;
  while((event=pop())!=NULL) {
    switch(event->type) {
      case AP_EOS   : if (target) target->handle(this,FXSEL(SEL_PLAYER_EOS,message),NULL); break;
      case AP_BOS : if (target) target->handle(this,FXSEL(SEL_PLAYER_BOS,message),NULL); break;
      case AP_STATE_READY      : state=PLAYER_STOPPED; if (target) target->handle(this,FXSEL(SEL_PLAYER_STATE,message),(void*)(FXival)state);break;
      case AP_STATE_PLAYING    : state=PLAYER_PLAYING; if (target) target->handle(this,FXSEL(SEL_PLAYER_STATE,message),(void*)(FXival)state);break;
      case AP_STATE_PAUSING    : state=PLAYER_PAUSING; if (target) target->handle(this,FXSEL(SEL_PLAYER_STATE,message),(void*)(FXival)state);break;
      case AP_TIMESTAMP              :
        {
          if (target) {
            PlaybackTime tm;
            tm.position = ((TimeUpdate*)event)->position;
            tm.length   = ((TimeUpdate*)event)->length;
            target->handle(this,FXSEL(SEL_PLAYER_TIME,message),&tm);
            }
        } break;

      case AP_ERROR                  :
        {
          ErrorMessage * err = dynamic_cast<ErrorMessage*>(event);
          fxmessage("[ERROR] %s\n",err->msg.text());
        } break;
      default: break;
      }
    Event::unref(event);
    }
  return 1;
  }
#endif

