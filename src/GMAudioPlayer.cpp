/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2010-2026 by Sander Jansen. All Rights Reserved      *
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
*                               ---                                            *
* SPDX-License-Identifier: GPL-3.0-or-later                                    *
********************************************************************************/
#include "gmdefs.h"
#include "GMApp.h"
#include "GMTrack.h"

#include <ap.h>
#include "GMAudioPlayer.h"

FXDEFMAP(GMAudioPlayer) GMAudioPlayerMap[]={
  FXMAPFUNC(SEL_CHANGED,GMAudioPlayer::ID_AUDIO_ENGINE,GMAudioPlayer::onEngineEvents)
  };

FXIMPLEMENT(GMAudioPlayer,AudioPlayer,GMAudioPlayerMap,ARRAYNUMBER(GMAudioPlayerMap));

GMAudioPlayer::GMAudioPlayer(FXApp * app,FXObject * tgt,FXSelector sel) : target(tgt), message(sel) {
  fifo = new FXAppQueue(app,this,ID_AUDIO_ENGINE);
  setEventQueue(fifo);
  }

GMAudioPlayer::~GMAudioPlayer() {
  delete fifo;
  }


void GMAudioPlayer::setPosition(FXuint position) {
  if (time.length>0) {
    seek(FXMIN(1.0,(position/(double)time.length)));
    }
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
  FXSettings settings;
  if (settings.parseFile(GMApp::getConfigDirectory()+PATHSEPSTRING+"gap.conf")){
    config.load(settings);
    setOutputConfig(config);
    }
  }


long GMAudioPlayer::onEngineEvents(FXObject*,FXSelector,void*){
  Event * event=nullptr;
  while((event=pop())!=nullptr) {
    switch(event->type) {
      case AP_EOS              : if (target) target->handle(this,FXSEL(SEL_PLAYER_EOS,message),nullptr); break;
      case AP_BOS              : if (target) target->handle(this,FXSEL(SEL_PLAYER_BOS,message),nullptr); break;
      case AP_STATE_READY      : state=PLAYER_STOPPED; if (target) target->handle(this,FXSEL(SEL_PLAYER_STATE,message),(void*)(FXival)state);break;
      case AP_STATE_PLAYING    : state=PLAYER_PLAYING; if (target) target->handle(this,FXSEL(SEL_PLAYER_STATE,message),(void*)(FXival)state);break;
      case AP_STATE_PAUSING    : state=PLAYER_PAUSING; if (target) target->handle(this,FXSEL(SEL_PLAYER_STATE,message),(void*)(FXival)state);break;
      case AP_TIMESTAMP        :
        {
            time.position = static_cast<TimeUpdate*>(event)->position;
            time.length   = static_cast<TimeUpdate*>(event)->length;
            if (target) target->handle(this,FXSEL(SEL_PLAYER_TIME,message),&time);
        } break;

      case AP_ERROR                  :
        {
          ErrorMessage * err = static_cast<ErrorMessage*>(event);
          if (target) target->handle(this,FXSEL(SEL_PLAYER_ERROR,message),(void*)&err->msg);
        } break;
      case AP_META_INFO              :
        {
          GMTrack track;
          MetaInfo * info = static_cast<MetaInfo*>(event);

          /// get data
          track.title.adopt(info->title);
          track.artist.adopt(info->artist);
          track.album.adopt(info->album);

          if (target) target->handle(this,FXSEL(SEL_PLAYER_META,message),&track);
        } break;
      case AP_VOLUME_NOTIFY          :
        {
            VolumeNotify * info = static_cast<VolumeNotify*>(event);
            if (info->volume.enabled)
              vvolume = (FXint)(info->volume.value * 100.0f);
            else
              vvolume = -1;

            if (target) target->handle(this,FXSEL(SEL_PLAYER_VOLUME,message),(void*)(FXival)vvolume);
            break;
        }
      default: break;
      }
    Event::unref(event);
    }
  return 1;
  }

