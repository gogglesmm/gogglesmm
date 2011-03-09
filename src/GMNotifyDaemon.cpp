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
#include "GMDBus.h"
#include "GMNotifyDaemon.h"

#define GALAGO_NOTIFY_NAME "org.freedesktop.Notifications"
#define GALAGO_NOTIFY_PATH "/org/freedesktop/Notifications"
#define GALAGO_NOTIFY_INTERFACE "org.freedesktop.Notifications"

FXDEFMAP(GMNotifyDaemon) GMNotifyDaemonMap[]={
  FXMAPFUNC(SEL_SIGNAL,0,GMNotifyDaemon::onSignal),
  FXMAPFUNC(SEL_COMMAND,0,GMNotifyDaemon::onMethod),
  FXMAPFUNC(SEL_COMMAND,GMNotifyDaemon::ID_NOTIFY_REPLY,GMNotifyDaemon::onNotifyReply),

  };

FXIMPLEMENT(GMNotifyDaemon,GMDBusProxy,GMNotifyDaemonMap,ARRAYNUMBER(GMNotifyDaemonMap));

GMNotifyDaemon::GMNotifyDaemon(){
  }

GMNotifyDaemon::GMNotifyDaemon(GMDBus * b) : GMDBusProxy(b,GALAGO_NOTIFY_NAME,GALAGO_NOTIFY_PATH,GALAGO_NOTIFY_INTERFACE),msgid(0){
  }

long GMNotifyDaemon::onSignal(FXObject*,FXSelector,void*ptr){
  DBusMessage * msg = reinterpret_cast<DBusMessage*>(ptr);
  FXuint id,reason;
  if (dbus_message_is_signal(msg,GALAGO_NOTIFY_INTERFACE,"NotificationClosed")){
    if ((dbus_message_has_signature(msg,"u") && dbus_message_get_args(msg,NULL,DBUS_TYPE_UINT32,&id,DBUS_TYPE_INVALID)) ||
        (dbus_message_has_signature(msg,"uu") && dbus_message_get_args(msg,NULL,DBUS_TYPE_UINT32,&id,DBUS_TYPE_UINT32,&reason,DBUS_TYPE_INVALID))) {
      if (id==msgid) {
        msgid=0;
        }
      }
    return 1;
    }
  return 0;
  }


long GMNotifyDaemon::onMethod(FXObject*,FXSelector,void*ptr){
  return 1;
  }

long GMNotifyDaemon::onNotifyReply(FXObject*,FXSelector,void*ptr){
  DBusMessage * msg = reinterpret_cast<DBusMessage*>(ptr);
  FXASSERT(msg);
  if (dbus_message_get_type(msg)==DBUS_MESSAGE_TYPE_METHOD_RETURN) {
    dbus_message_get_args(msg,NULL,DBUS_TYPE_UINT32,&msgid,DBUS_TYPE_INVALID);
    fxmessage("id %d\n",msgid);
    }
  return 1;
  }

void GMNotifyDaemon::close() {
  if (msgid>0) {
    DBusMessage * msg = method("CloseNotification");
    if (msg) {
      dbus_message_append_args(msg,DBUS_TYPE_UINT32,&msgid,DBUS_TYPE_INVALID);
      dbus_message_set_no_reply(msg,true);
      bus->send(msg);
      }
    }
  }

void GMNotifyDaemon::notify(const FXchar * application,const FXchar * ic,const FXchar * summary,const FXchar * body,FXint timeout,FXImage* image){
  FXint iw,ih,is,ibps,ichannels,isize;
  dbus_bool_t ialpha;
  const FXchar * idata=NULL;
  const FXchar * appicon="";

  if (image==NULL)
    appicon=ic;

  DBusMessage * msg = method("Notify");
  if (msg){
      DBusMessageIter iter;
      DBusMessageIter array;
      DBusMessageIter dict;
      DBusMessageIter value;
      DBusMessageIter variant;
      DBusMessageIter data;

      dbus_message_iter_init_append(msg,&iter);
        dbus_message_iter_append_basic(&iter,DBUS_TYPE_STRING,&application);
        dbus_message_iter_append_basic(&iter,DBUS_TYPE_UINT32,&msgid);
        dbus_message_iter_append_basic(&iter,DBUS_TYPE_STRING,&appicon);
        dbus_message_iter_append_basic(&iter,DBUS_TYPE_STRING,&summary);
        dbus_message_iter_append_basic(&iter,DBUS_TYPE_STRING,&body);
        dbus_message_iter_open_container(&iter,DBUS_TYPE_ARRAY,DBUS_TYPE_STRING_AS_STRING,&array);
        dbus_message_iter_close_container(&iter,&array);
        dbus_message_iter_open_container(&iter,DBUS_TYPE_ARRAY,"{sv}",&array);
        if (image && image->getData()) {
          const FXchar * icon_data="icon_data"; /// spec 0.9 says "image_data". some use "icon_data" though..

          idata     = (const FXchar*)image->getData();
          ialpha    = true;
          iw        = image->getWidth();
          ih        = image->getHeight();
          is        = iw*4;
          ibps      = 8;
          ichannels = 4;
          isize     = iw*ih*4;

          dbus_message_iter_open_container(&array,DBUS_TYPE_DICT_ENTRY,0,&dict);
            dbus_message_iter_append_basic(&dict,DBUS_TYPE_STRING,&icon_data);
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
          }
        dbus_message_iter_close_container(&iter,&array);
        dbus_message_iter_append_basic(&iter,DBUS_TYPE_INT32,&timeout);

      send(msg,this,ID_NOTIFY_REPLY);
      }
    }





#if 0

#define KDE_NOTIFY_NAME "org.kde.VisualNotifications"
#define KDE_NOTIFY_PATH "/VisualNotifications"
#define KDE_NOTIFY_INTERFACE "org.kde.VisualNotifications"

FXDEFMAP(GMNotifyDaemon) GMNotifyDaemonMap[]={
  FXMAPFUNC(SEL_COMMAND,GMNotifyDaemon::ID_NOTIFY_RESULT,GMNotifyDaemon::onCmdNotify),
  FXMAPFUNC(SEL_COMMAND,GMNotifyDaemon::ID_NOTIFY_CLOSE,GMNotifyDaemon::onCmdClose)
  };

FXIMPLEMENT(GMNotifyDaemon,FXObject,GMNotifyDaemonMap,ARRAYNUMBER(GMNotifyDaemonMap));

DBusHandlerResult GMNotifyDaemon::dbus_filter(DBusConnection *,DBusMessage * msg,void * data){
  GMNotifyDaemon * daemon = (GMNotifyDaemon*)data;
  if (dbus_message_has_path(msg,GALAGO_NOTIFY_PATH)){
    FXTRACE((80,"-----GMNotifyDaemon::dbus_filter-------\n"));
    FXTRACE((80,"path: %s\n",dbus_message_get_path(msg)));
    FXTRACE((80,"member: \"%s\"\n",dbus_message_get_member(msg)));
    FXTRACE((80,"interface: %s\n",dbus_message_get_interface(msg)));
    FXTRACE((80,"sender: %s\n",dbus_message_get_sender(msg)));

    if (dbus_message_is_signal(msg,GALAGO_NOTIFY_INTERFACE,"NotificationClosed")){
      FXuint msgid=0,reason=0;
      if (dbus_message_has_signature(msg,"u")) {
        if (dbus_message_get_args(msg,NULL,DBUS_TYPE_UINT32,&msgid,DBUS_TYPE_INVALID)) {
          daemon->closed(msgid);
          }
        }
      else if (dbus_message_has_signature(msg,"uu")) {
        if (dbus_message_get_args(msg,NULL,DBUS_TYPE_UINT32,&msgid,DBUS_TYPE_UINT32,&reason,DBUS_TYPE_INVALID)) {
          daemon->closed(msgid);
          }
        }
      return DBUS_HANDLER_RESULT_HANDLED;
      }
    }
  else if (dbus_message_has_path(msg,"/org/freedesktop/DBus")) {

    FXTRACE((80,"-----GMNotifyDaemon::dbus_filter-------\n"));
    FXTRACE((80,"path: %s\n",dbus_message_get_path(msg)));
    FXTRACE((80,"member: \"%s\"\n",dbus_message_get_member(msg)));
    FXTRACE((80,"interface: %s\n",dbus_message_get_interface(msg)));
    FXTRACE((80,"sender: %s\n",dbus_message_get_sender(msg)));

    if (dbus_message_is_signal(msg,"org.freedesktop.DBus","NameOwnerChanged")){
      FXchar * c_name     = NULL;
      FXchar * c_oldowner = NULL;
      FXchar * c_newowner = NULL;
      if (dbus_message_get_args(msg,NULL,DBUS_TYPE_STRING,&c_name,DBUS_TYPE_STRING,&c_oldowner,DBUS_TYPE_STRING,&c_newowner,DBUS_TYPE_INVALID)) {
        if (compare(c_name,KDE_NOTIFY_NAME)==0) {
          if (c_newowner==NULL || compare(c_newowner,"")==0 )
            daemon->has_kde=false;
          else
            daemon->has_kde=true;
          }
        }
      return DBUS_HANDLER_RESULT_HANDLED;
      }
    }
  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }


GMNotifyDaemon::GMNotifyDaemon()  {
  }

GMNotifyDaemon::GMNotifyDaemon(GMDBus* c) : dbus(c), replace_id(0),has_kde(false),detect(false) {
  gm_dbus_match_signal(c->connection(),GALAGO_NOTIFY_PATH,GALAGO_NOTIFY_INTERFACE,"NotificationClosed");
  gm_dbus_match_signal(c->connection(),KDE_NOTIFY_PATH,KDE_NOTIFY_INTERFACE,"NotificationClosed");
  gm_dbus_match_signal(c->connection(),DBUS_PATH_DBUS,DBUS_INTERFACE_DBUS,"NameOwnerChanged");
  dbus_connection_add_filter(dbus->connection(),dbus_filter,this,NULL);
  }

GMNotifyDaemon::~GMNotifyDaemon() {
  dbus_connection_remove_filter(dbus->connection(),dbus_filter,this);
  }

long GMNotifyDaemon::onCmdClose(FXObject*,FXSelector,void*){
  return 1;
  }

long GMNotifyDaemon::onCmdNotify(FXObject*,FXSelector,void*ptr){
  DBusMessage * msg = (DBusMessage*)ptr;
  if (msg) {
    DBusMessageIter iter;
    dbus_message_iter_init(msg,&iter);
    dbus_message_iter_get_basic(&iter,&replace_id);
    //fxmessage("replace id = %d\n",replace_id);
    }
  return 1;
  }

void GMNotifyDaemon::close() {
  FXTRACE((70,"GMNotifyDaemon::close\n"));
  if (replace_id>0) {
    DBusMessage * msg = dbus_message_new_method_call(GALAGO_NOTIFY_NAME,GALAGO_NOTIFY_PATH,GALAGO_NOTIFY_INTERFACE,"CloseNotification");
    if (msg) {
      dbus_message_append_args(msg,DBUS_TYPE_UINT32,&replace_id,DBUS_TYPE_INVALID);
      dbus_message_set_no_reply(msg,true);
      dbus->sendWithReply(msg,-1,this,ID_NOTIFY_CLOSE);
     // dbus_connection_send(dbus->connection(),msg,NULL);
      //dbus_message_unref(msg);
      }
    }
  }

void GMNotifyDaemon::closed(FXuint id) {
  if (replace_id==id) {
    FXTRACE((70,"GMNotifyDaemon::closed\n"));
    replace_id=0;
    }
  }


void GMNotifyDaemon::notify(const FXchar * application,
                            const FXchar * icon,
                            const FXchar * summary,
                            const FXchar * body,
                            FXint timeout,
                            FXImage      * image) {

  FXint iw,ih,is,ibps,ichannels,isize;
  const FXchar * eventid="Now Playing";
  dbus_bool_t ialpha;
  const FXchar * idata=NULL;
  const FXchar * appicon = "";

  if (image==NULL && icon)
    appicon=icon;

  DBusMessage * msg = NULL;

  if (has_kde==false && detect==false) {
    has_kde  = dbus_bus_name_has_owner(dbus->connection(),KDE_NOTIFY_NAME,NULL);
    detect=true;
    }

  if (has_kde) {





    }
  else {
    if ((msg = dbus_message_new_method_call(GALAGO_NOTIFY_NAME,GALAGO_NOTIFY_PATH,GALAGO_NOTIFY_INTERFACE,"Notify"))!=NULL){
      DBusMessageIter iter;
      DBusMessageIter array;
      DBusMessageIter dict;
      DBusMessageIter value;
      DBusMessageIter variant;
      DBusMessageIter data;

      dbus_message_iter_init_append(msg,&iter);
        dbus_message_iter_append_basic(&iter,DBUS_TYPE_STRING,&application);
        dbus_message_iter_append_basic(&iter,DBUS_TYPE_UINT32,&replace_id);
        dbus_message_iter_append_basic(&iter,DBUS_TYPE_STRING,&appicon);
        dbus_message_iter_append_basic(&iter,DBUS_TYPE_STRING,&summary);
        dbus_message_iter_append_basic(&iter,DBUS_TYPE_STRING,&body);
        dbus_message_iter_open_container(&iter,DBUS_TYPE_ARRAY,DBUS_TYPE_STRING_AS_STRING,&array);
        dbus_message_iter_close_container(&iter,&array);
        dbus_message_iter_open_container(&iter,DBUS_TYPE_ARRAY,"{sv}",&array);
        if (image && image->getData()) {
          const FXchar * icon_data="icon_data"; /// spec 0.9 says "image_data". some use "icon_data" though..

          idata     = (const FXchar*)image->getData();
          ialpha    = true;
          iw        = image->getWidth();
          ih        = image->getHeight();
          is        = iw*4;
          ibps      = 8;
          ichannels = 4;
          isize     = iw*ih*4;

          dbus_message_iter_open_container(&array,DBUS_TYPE_DICT_ENTRY,0,&dict);
            dbus_message_iter_append_basic(&dict,DBUS_TYPE_STRING,&icon_data);
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
          }
        dbus_message_iter_close_container(&iter,&array);
        dbus_message_iter_append_basic(&iter,DBUS_TYPE_INT32,&timeout);
      dbus->sendWithReply(msg,-1,this,ID_NOTIFY_RESULT);
      }
    }



#if 0
  msg = dbus_message_new_method_call(GALAGO_NOTIFY_NAME,GALAGO_NOTIFY_PATH,GALAGO_NOTIFY_INTERFACE,"Notify");
  if (msg) {
    DBusMessageIter iter;
    DBusMessageIter array;
    DBusMessageIter dict;
    DBusMessageIter value;
    DBusMessageIter variant;
    DBusMessageIter data;
//Notify(QString app_name, uint replaces_id, QString event_id, QString app_icon, QString summary, QString body, QStringList actions, QVariantMap hints, int timeout)



    dbus_message_iter_init_append(msg,&iter);
      dbus_message_iter_append_basic(&iter,DBUS_TYPE_STRING,&application);
      dbus_message_iter_append_basic(&iter,DBUS_TYPE_UINT32,&replace_id);
      dbus_message_iter_append_basic(&iter,DBUS_TYPE_STRING,&eventid);
      dbus_message_iter_append_basic(&iter,DBUS_TYPE_STRING,&appicon);
      dbus_message_iter_append_basic(&iter,DBUS_TYPE_STRING,&summary);
      dbus_message_iter_append_basic(&iter,DBUS_TYPE_STRING,&body);
      dbus_message_iter_open_container(&iter,DBUS_TYPE_ARRAY,DBUS_TYPE_STRING_AS_STRING,&array);
      dbus_message_iter_close_container(&iter,&array);
      dbus_message_iter_open_container(&iter,DBUS_TYPE_ARRAY,"{sv}",&array);
      if (image && image->getData()) {
//        const FXchar * icon_data="icon_data"; /// spec 0.9 says "image_data". some use "icon_data" though..
        const FXchar * icon_data="image_data"; /// spec 0.9 says "image_data". some use "icon_data" though..

        idata     = (const FXchar*)image->getData();
        ialpha    = true;
        iw        = image->getWidth();
        ih        = image->getHeight();
        is        = iw*4;
        ibps      = 8;
        ichannels = 4;
        isize     = iw*ih*4;

        dbus_message_iter_open_container(&array,DBUS_TYPE_DICT_ENTRY,0,&dict);
          dbus_message_iter_append_basic(&dict,DBUS_TYPE_STRING,&icon_data);
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
        }
      dbus_message_iter_close_container(&iter,&array);
      dbus_message_iter_append_basic(&iter,DBUS_TYPE_INT32,&timeout);
    dbus->sendWithReply(msg,-1,this,ID_NOTIFY_RESULT);
    }
#endif
  }
  #endif
