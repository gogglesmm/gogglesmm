/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2007-2010 by Sander Jansen. All Rights Reserved      *
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
#include "GMDBus.h"
#include "GMTrack.h"
#include "GMPlayerManager.h"
#include "GMNotifyDaemon.h"

#define GALAGO_NOTIFY_NAME "org.freedesktop.Notifications"
#define GALAGO_NOTIFY_PATH "/org/freedesktop/Notifications"
#define GALAGO_NOTIFY_INTERFACE "org.freedesktop.Notifications"

/*

/// gnome 3.2.0
name: gnome-shell
vendor: GNOME
version: 3.2.0
spec: 1.2
quirks:

/// gnome3
name: gnome-shell
vendor: GNOME
version: 3.0.2
spec: 1.2
quirks: IMAGE_WITHOUT_APPICON


/// gnome-fallback - notification-daemon
name: Notification Daemon
vendor: GNOME
version: 0.7.2
spec: 1.2
quirks:


/// XFCE  xfce4-notifyd
name: Xfce Notify Daemon
vendor: Xfce
version: 0.2.2
spec: 0.9
quirks: IMAGE_WITHOUT_APPICON


/// KDE
name: Plasma
vendor: KDE
version: 1.0
spec: 1.1
quirks: 0


/// Notify-OSD
name: notify-osd
vendor: Canonical Ltd
version: 1.0
spec: 1.1
icondata: image_data
flags: 0


*/


FXDEFMAP(GMNotifyDaemon) GMNotifyDaemonMap[]={
  FXMAPFUNC(SEL_SIGNAL,0,GMNotifyDaemon::onSignal),
  FXMAPFUNC(SEL_COMMAND,0,GMNotifyDaemon::onMethod),
  FXMAPFUNC(SEL_COMMAND,GMNotifyDaemon::ID_NOTIFY_REPLY,GMNotifyDaemon::onNotifyReply),
  FXMAPFUNC(SEL_COMMAND,GMNotifyDaemon::ID_NOTIFY_CAPABILITIES,GMNotifyDaemon::onNotifyCapabilities),
  FXMAPFUNC(SEL_COMMAND,GMNotifyDaemon::ID_NOTIFY_SERVER,GMNotifyDaemon::onNotifyServer)

  };

FXIMPLEMENT(GMNotifyDaemon,GMDBusProxy,GMNotifyDaemonMap,ARRAYNUMBER(GMNotifyDaemonMap));

GMNotifyDaemon::GMNotifyDaemon(){
  }

GMNotifyDaemon::GMNotifyDaemon(GMDBus * b) : GMDBusProxy(b,GALAGO_NOTIFY_NAME,GALAGO_NOTIFY_PATH,GALAGO_NOTIFY_INTERFACE),flags(0),msgid(0),persistent(false){
  appicon="gogglesmm";
  appname="gogglesmm";
  icondata="icon_data";
  }

long GMNotifyDaemon::onSignal(FXObject*,FXSelector,void*ptr){
  DBusMessage * msg = reinterpret_cast<DBusMessage*>(ptr);
  FXuint id,reason;
  FXchar * action;
  if (dbus_message_is_signal(msg,GALAGO_NOTIFY_INTERFACE,"NotificationClosed")){
    if ((dbus_message_has_signature(msg,"u") && dbus_message_get_args(msg,NULL,DBUS_TYPE_UINT32,&id,DBUS_TYPE_INVALID)) ||
        (dbus_message_has_signature(msg,"uu") && dbus_message_get_args(msg,NULL,DBUS_TYPE_UINT32,&id,DBUS_TYPE_UINT32,&reason,DBUS_TYPE_INVALID))) {
      if (id==msgid) {
        msgid=0;
        }
      }
    return 1;
    }
  else if (dbus_message_is_signal(msg,GALAGO_NOTIFY_INTERFACE,"ActionInvoked")){
    if (dbus_message_has_signature(msg,"us") && dbus_message_get_args(msg,NULL,DBUS_TYPE_UINT32,&id,DBUS_TYPE_STRING,&action,DBUS_TYPE_INVALID)) {
      if (compare(action,"media-skip-backward")==0) {
        GMPlayerManager::instance()->cmd_prev();
        }
      else if (compare(action,"media-skip-forward")==0) {
        GMPlayerManager::instance()->cmd_next();
        }
      else if (compare(action,"media-playback-pause")==0) {
        GMPlayerManager::instance()->cmd_pause();
        }
      else if (compare(action,"media-playback-start")==0) {
        GMPlayerManager::instance()->cmd_play();
        }
      else if (compare(action,"media-playback-stop")==0) {
        GMPlayerManager::instance()->cmd_stop();
        }
      else {
        fxmessage("unhandled action: %s\n",action);
        }
      return 1;
      }
    }
  return 0;
  }


long GMNotifyDaemon::onMethod(FXObject*,FXSelector,void*){
  return 1;
  }

long GMNotifyDaemon::onNotifyServer(FXObject*,FXSelector,void*ptr){
  DBusMessage * msg = reinterpret_cast<DBusMessage*>(ptr);
  const FXchar * name=NULL;
  const FXchar * vendor=NULL;
  const FXchar * version=NULL;
  const FXchar * spec=NULL;

  if ((dbus_message_get_type(msg)==DBUS_MESSAGE_TYPE_METHOD_RETURN) && dbus_message_get_args(msg,NULL,DBUS_TYPE_STRING,&name,DBUS_TYPE_STRING,&vendor,DBUS_TYPE_STRING,&version,DBUS_TYPE_STRING,&spec,DBUS_TYPE_INVALID)) {

    if (compareversion(spec,"1.1")==0) {
      icondata="image_data";
      }
    else if (compareversion(spec,"1.2")>=0) {
      icondata="image-data";
      }
    else {
      icondata="icon_data";
      }

    if (comparecase(vendor,"xfce")==0 && comparecase(name,"xfce notify daemon")==0) {
      flags|=IMAGE_WITHOUT_APPICON;
      }

    if (comparecase(name,"gnome-shell")==0 && comparecase(vendor,"gnome")==0) {
      GMPlayerManager::instance()->getPreferences().gui_tray_icon_disabled=true;
      flags|=ACTION_ITEMS;
      if (compareversion(version,"3.2.0")<0){
        flags|=IMAGE_WITHOUT_APPICON;
        }
      }
#ifdef DEBUG
    fxmessage("name: %s\n",name);
    fxmessage("vendor: %s\n",vendor);
    fxmessage("version: %s\n",version);
    fxmessage("spec: %s\n",spec);
    fxmessage("icondata: %s\n",icondata.text());
    fxmessage("flags: %x\n",flags);
#endif
    }

  GMPlayerManager::instance()->update_tray_icon();
  return 1;
  }

long GMNotifyDaemon::onNotifyCapabilities(FXObject*,FXSelector,void*ptr){
  DBusMessage * msg = reinterpret_cast<DBusMessage*>(ptr);
  FXchar ** caps = NULL;
  int ncaps;
  if ((dbus_message_get_type(msg)==DBUS_MESSAGE_TYPE_METHOD_RETURN) && dbus_message_get_args(msg,NULL,DBUS_TYPE_ARRAY,DBUS_TYPE_STRING,&caps,&ncaps,DBUS_TYPE_INVALID)) {
    FXbool has_action_icons=false;
    FXbool has_actions=false;
    FXbool has_persistence=false;

    for (FXint i=0;i<ncaps;i++){
#ifdef DEBUG
      fxmessage("caps[%d]=%s\n",i,caps[i]);
#endif
      if (flags&ACTION_ITEMS) {
        if (comparecase(caps[i],"actions")==0)
          has_actions=true;
        else if ((comparecase(caps[i],"action-icons")==0) || (comparecase(caps[i],"x-gnome-icon-buttons")==0))
          has_action_icons=true;
        else if (comparecase(caps[i],"persistence")==0)
          has_persistence=true;
        }
      }

    if (has_actions && has_action_icons && has_persistence) {
      persistent=true;
      reset();
      }

    // Need to delete array of strings returned by dbus_message_get_args
    dbus_free_string_array(caps);
    }
  return 1;
  }


long GMNotifyDaemon::onNotifyReply(FXObject*,FXSelector,void*ptr){
  DBusMessage * msg = reinterpret_cast<DBusMessage*>(ptr);
  FXASSERT(msg);
  if (dbus_message_get_type(msg)==DBUS_MESSAGE_TYPE_METHOD_RETURN) {
    dbus_message_get_args(msg,NULL,DBUS_TYPE_UINT32,&msgid,DBUS_TYPE_INVALID);
    }
  return 1;
  }


void GMNotifyDaemon::reset() {
  if (persistent) {
    notify("Goggles Music Manager","",-1,NULL);
    }
  else {
    close();
    }
  }

void GMNotifyDaemon::close() {
  fxmessage("GMNotifyDaemon::close()\n");
  if (msgid>0) {
    DBusMessage * msg = method("CloseNotification");
    if (msg) {
      dbus_message_append_args(msg,DBUS_TYPE_UINT32,&msgid,DBUS_TYPE_INVALID);
      dbus_message_set_no_reply(msg,true);
      bus->send(msg);
      }
    }
  }

void GMNotifyDaemon::init() {
  {
    DBusMessage * msg = method("GetServerInformation");
    send(msg,this,ID_NOTIFY_SERVER);
  }


  {
    DBusMessage * msg = method("GetCapabilities");
    send(msg,this,ID_NOTIFY_CAPABILITIES);
  }

  }

void GMNotifyDaemon::notify_track_change(const GMTrack & track,FXImage * cover){
  FXString body = FXString::value(fxtrformat("%s\n%s (%d)"),track.artist.text(),track.album.text(),track.year);
  /// Dirty Hack. According to the spec, we shouldn't have to do this,
  /// but try finding a notification notifydaemon that actually implements it...
  /// http://www.galago-project.org/specs/notification/0.9/index.html
  body.substitute("&","&amp;");
  notify(track.title.text(),body.text(),-1,cover);
  }


void GMNotifyDaemon::notify(const FXchar * summary,const FXchar * body,FXint timeout,FXImage* image){
  FXint iw,ih,is,ibps,ichannels,isize;
  dbus_bool_t ialpha;

  const FXchar * idata=NULL;

  DBusMessage * msg = method("Notify");
  if (msg){
      DBusMessageIter iter;
      DBusMessageIter array;
      DBusMessageIter dict;
      DBusMessageIter value;
      DBusMessageIter variant;
      DBusMessageIter data;

      dbus_message_iter_init_append(msg,&iter);

        gm_dbus_append_string(&iter,appname);
        dbus_message_iter_append_basic(&iter,DBUS_TYPE_UINT32,&msgid);

        if (image && (flags&IMAGE_WITHOUT_APPICON)) {
          FXString empty;
          gm_dbus_append_string(&iter,empty);
          }
        else {
          gm_dbus_append_string(&iter,appicon);
          }

        dbus_message_iter_append_basic(&iter,DBUS_TYPE_STRING,&summary);
        dbus_message_iter_append_basic(&iter,DBUS_TYPE_STRING,&body);

        dbus_message_iter_open_container(&iter,DBUS_TYPE_ARRAY,DBUS_TYPE_STRING_AS_STRING,&array);
        if (persistent) {
          if (GMPlayerManager::instance()->can_prev())
            gm_dbus_append_string_pair(&array,"media-skip-backward","Previous");
          if (GMPlayerManager::instance()->can_pause())
            gm_dbus_append_string_pair(&array,"media-playback-pause","Pause");
          else if (GMPlayerManager::instance()->can_play())
            gm_dbus_append_string_pair(&array,"media-playback-start","Play");
          if (GMPlayerManager::instance()->can_stop())
            gm_dbus_append_string_pair(&array,"media-playback-stop","Stop");
          if (GMPlayerManager::instance()->can_next())
            gm_dbus_append_string_pair(&array,"media-skip-forward","Next");
          }
        dbus_message_iter_close_container(&iter,&array);


        dbus_message_iter_open_container(&iter,DBUS_TYPE_ARRAY,"{sv}",&array);
        if (image && image->getData()) {
//          const FXchar * icon_data="icon_data"; /// spec 0.9 says "image_data". some use "icon_data" though..
//          const FXchar * icon_data="image-data"; /// spec 0.9 says "image_data". some use "icon_data" though..

          ialpha    = true;
          iw        = image->getWidth();
          ih        = image->getHeight();
          is        = iw*4;
          ibps      = 8;
          ichannels = 4;
          isize     = iw*ih*4;

          FXColor * bgra = NULL;
          allocElms(bgra,(iw*ih));
          gm_bgra_to_rgba(image->getData(),bgra,(iw*ih));

          idata     = (const FXchar*)bgra;

          dbus_message_iter_open_container(&array,DBUS_TYPE_DICT_ENTRY,0,&dict);
            gm_dbus_append_string(&dict,icondata);
            dbus_message_iter_open_container(&dict,DBUS_TYPE_VARIANT,"(iiibiiay)",&variant);
              dbus_message_iter_open_container(&variant,DBUS_TYPE_STRUCT,NULL,&value);
                dbus_message_iter_append_basic(&value,DBUS_TYPE_INT32,&iw);
                dbus_message_iter_append_basic(&value,DBUS_TYPE_INT32,&ih);
                dbus_message_iter_append_basic(&value,DBUS_TYPE_INT32,&is);
                dbus_message_iter_append_basic(&value,DBUS_TYPE_BOOLEAN,&ialpha);
                dbus_message_iter_append_basic(&value,DBUS_TYPE_INT32,&ibps);
                dbus_message_iter_append_basic(&value,DBUS_TYPE_INT32,&ichannels);
                dbus_message_iter_open_container(&value,DBUS_TYPE_ARRAY,DBUS_TYPE_BYTE_AS_STRING,&data);
                  dbus_message_iter_append_fixed_array(&data,DBUS_TYPE_BYTE,&idata,isize);
                dbus_message_iter_close_container(&value,&data);
              dbus_message_iter_close_container(&variant,&value);
            dbus_message_iter_close_container(&dict,&variant);
          dbus_message_iter_close_container(&array,&dict);

          freeElms(bgra);
          }

            //gm_dbus_dict_append_bool(&array,"transient",true);

        if (persistent) {
//          if (GMPlayerManager::instance()->playing())
            gm_dbus_dict_append_bool(&array,"resident",true);
//          else
//            gm_dbus_dict_append_bool(&array,"transient",true);
          gm_dbus_dict_append_bool(&array,"action-icons",true);
          }
        dbus_message_iter_close_container(&iter,&array);
        dbus_message_iter_append_basic(&iter,DBUS_TYPE_INT32,&timeout);

      send(msg,this,ID_NOTIFY_REPLY);
      }
    }


