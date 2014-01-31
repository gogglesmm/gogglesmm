/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2010-2014 by Sander Jansen. All Rights Reserved      *
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
#include "GMDBus.h"
#include "GMSettingsDaemon.h"

#define GNOME_SETTINGS_DAEMON_PATH        "/org/gnome/SettingsDaemon/MediaKeys"
#define GNOME_SETTINGS_DAEMON_INTERFACE   "org.gnome.SettingsDaemon.MediaKeys"
#define GNOME_SETTINGS_DAEMON_NAME        "org.gnome.SettingsDaemon"

FXDEFMAP(GMSettingsDaemon) GMSettingsDaemonMap[]={
  FXMAPFUNC(SEL_SIGNAL,0,GMSettingsDaemon::onSignal)
  };

FXIMPLEMENT(GMSettingsDaemon,GMDBusProxy,GMSettingsDaemonMap,ARRAYNUMBER(GMSettingsDaemonMap))

GMSettingsDaemon::GMSettingsDaemon(GMDBus*b,FXObject * tgt,FXSelector sel) : GMDBusProxy(b,GNOME_SETTINGS_DAEMON_NAME,GNOME_SETTINGS_DAEMON_PATH,GNOME_SETTINGS_DAEMON_INTERFACE) {
  target=tgt;
  message=sel;
  }

GMSettingsDaemon::~GMSettingsDaemon(){
  }

void GMSettingsDaemon::GrabMediaPlayerKeys(const FXchar * pl,FXuint time) {
  DBusMessage * msg = method("GrabMediaPlayerKeys");
  if (msg) {
    dbus_message_append_args(msg,DBUS_TYPE_STRING,&pl,DBUS_TYPE_UINT32,&time,DBUS_TYPE_INVALID);
    player=pl;
    bus->send(msg);
    }
  }

void GMSettingsDaemon::ReleaseMediaPlayerKeys(const FXchar * pl) {
  DBusMessage * msg = method("ReleaseMediaPlayerKeys");
  if (msg) {
    dbus_message_append_args(msg,DBUS_TYPE_STRING,&pl,DBUS_TYPE_INVALID);
    player.clear();
    bus->send(msg);
    }
  }


long GMSettingsDaemon::onSignal(FXObject*,FXSelector,void*ptr){
  DBusMessage * msg = reinterpret_cast<DBusMessage*>(ptr);
  if (dbus_message_is_signal(msg,GNOME_SETTINGS_DAEMON_INTERFACE,"MediaPlayerKeyPressed")){
    const FXchar * pl=NULL;
    const FXchar * cmd=NULL;
    if (dbus_message_get_args(msg,NULL,DBUS_TYPE_STRING,&pl,DBUS_TYPE_STRING,&cmd,DBUS_TYPE_INVALID)) {
      if (compare(pl,player)==0 && target && cmd) {
        target->handle(this,FXSEL(SEL_KEYPRESS,message),(void*)cmd);
        }
      }
    return 1;
    }
  return 0;
  }
