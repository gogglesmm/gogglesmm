/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2008-2010 by Sander Jansen. All Rights Reserved      *
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
#include "GMTrack.h"
#include "GMAppStatusNotify.h"
#include "GMPlayerManager.h"

#include "appstatus_xml.h"


#define APPLICATION_STATUS_NAME       "org.kde.StatusNotifierWatcher"
#define APPLICATION_STATUS_PATH      "/StatusNotifierWatcher"
#define APPLICATION_STATUS_INTERFACE "org.kde.StatusNotifierWatcher"

#define APPLICATION_STATUS_ITEM_PATH "/StatusNotifierItem"
#define APPLICATION_STATUS_ITEM_INTERFACE "org.kde.StatusNotifierItem"
#define APPLICATION_STATUS_ITEM_MENU_PATH "/StatusMenu"


//#define DBUS_MENU_INTERFACE "org.ayatana.dbusmenu"
#define DBUS_MENU_INTERFACE "com.canonical.dbusmenu"

#define SESSIONBUS (bus->connection())



static void gm_add_tooltip(DBusMessageIter*entry) {
  DBusMessageIter dict;
  DBusMessageIter container;
  DBusMessageIter item;
  DBusMessageIter icondata;

  dbus_message_iter_open_container(entry,DBUS_TYPE_DICT_ENTRY,0,&dict);
    gm_dbus_append_string(&dict,"ToolTip");
    dbus_message_iter_open_container(&dict,DBUS_TYPE_VARIANT,"(sa(iiay)ss)",&container);
      dbus_message_iter_open_container(&container,DBUS_TYPE_STRUCT,0,&item);
        gm_dbus_append_string(&item,"gogglesmm");
        dbus_message_iter_open_container(&item,DBUS_TYPE_ARRAY,"(iiay)",&icondata);
        dbus_message_iter_close_container(&item,&icondata);
        gm_dbus_append_string(&item,"Goggles Music Manager");
        gm_dbus_append_string(&item,"Hello World");
      dbus_message_iter_close_container(&container,&item);
    dbus_message_iter_close_container(&dict,&container);
  dbus_message_iter_close_container(entry,&dict);
  }

DBusHandlerResult dbus_status_item_filter(DBusConnection *connection,DBusMessage * msg,void */* data*/){
//  DEBUG_DBUS_MESSAGE(msg);
  DBusMessage *   reply;
  DBusMessageIter iter;
  DBusMessageIter dict;
  //DBusMessageIter dictentry;

  FXuint serial;
  if (dbus_message_has_path(msg,APPLICATION_STATUS_ITEM_PATH)){
    DEBUG_DBUS_MESSAGE(msg);
    if (dbus_message_has_interface(msg,APPLICATION_STATUS_ITEM_INTERFACE)) {
      if (dbus_message_is_method_call(msg,APPLICATION_STATUS_ITEM_INTERFACE,"Activate")){
        GMPlayerManager::instance()->cmd_toggle_shown();
        return gm_dbus_reply_if_needed(connection,msg);
        }
      else if (dbus_message_is_method_call(msg,APPLICATION_STATUS_ITEM_INTERFACE,"Scroll")){
        FXint delta;
        const FXchar * orientation;
        if (dbus_message_get_args(msg,NULL,DBUS_TYPE_INT32,&delta,DBUS_TYPE_STRING,&orientation,DBUS_TYPE_INVALID)) {

          //FIXME
          FXint level = GMPlayerManager::instance()->volume();
          level+=(delta/120);
          GMPlayerManager::instance()->volume(level);
          }
        return gm_dbus_reply_if_needed(connection,msg);
        }
      }
    else if (dbus_message_has_interface(msg,DBUS_INTERFACE_INTROSPECTABLE)) {
      if (dbus_message_is_method_call(msg,DBUS_INTERFACE_INTROSPECTABLE,"Introspect")){
        return gm_dbus_reply_string(connection,msg,appstatus_xml);
        }
      }
    else if (dbus_message_has_interface(msg,DBUS_INTERFACE_PROPERTIES)) {
      if (dbus_message_is_method_call(msg,DBUS_INTERFACE_PROPERTIES,"Get")){
        fxmessage("get\n");
        }
      else if (dbus_message_is_method_call(msg,DBUS_INTERFACE_PROPERTIES,"GetAll")){
        fxmessage("getall\n");
        if ((reply=dbus_message_new_method_return(msg))!=NULL) {
          dbus_message_iter_init_append(reply,&iter);
          dbus_message_iter_open_container(&iter,DBUS_TYPE_ARRAY,"{sv}",&dict);
            gm_dbus_dict_append_path(&dict,"Menu",APPLICATION_STATUS_ITEM_MENU_PATH);
            gm_dbus_dict_append_string(&dict,"Category","ApplicationStatus");
            gm_dbus_dict_append_string(&dict,"Id","gogglesmm");
            gm_dbus_dict_append_string(&dict,"IconName","gogglesmm");
            gm_dbus_dict_append_string(&dict,"Status","Active");
            gm_dbus_dict_append_uint32(&dict,"WindowId",GMPlayerManager::instance()->getMainWindowId());
            gm_add_tooltip(&dict);




          dbus_message_iter_close_container(&iter,&dict);
          dbus_connection_send(connection,reply,&serial);
          dbus_message_unref(reply);
          }
        return DBUS_HANDLER_RESULT_HANDLED;
        }
      else if (dbus_message_is_method_call(msg,DBUS_INTERFACE_PROPERTIES,"Set")){
        }
      }
    }
  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }

void gm_begin_menu(DBusMessageIter * root,DBusMessageIter * item,FXint id,const FXchar * label,const FXchar * icon=NULL,FXbool enabled=true) {
  DBusMessageIter dict;
  dbus_message_iter_open_container(root,DBUS_TYPE_STRUCT,NULL,item);
    dbus_message_iter_append_basic(item,DBUS_TYPE_INT32,&id);
      dbus_message_iter_open_container(item,DBUS_TYPE_ARRAY,"{sv}",&dict);
        gm_dbus_dict_append_string(&dict,"label",label);
        gm_dbus_dict_append_string(&dict,"icon-name",icon);
        if (!enabled)
          gm_dbus_dict_append_bool(&dict,"enabled",false);
      dbus_message_iter_close_container(item,&dict);
  }

void gm_end_menu(DBusMessageIter * root,DBusMessageIter * item) {
  dbus_message_iter_close_container(root,item);
  }

void gm_begin_menu_item_list(DBusMessageIter * item,DBusMessageIter * list){
  dbus_message_iter_open_container(item,DBUS_TYPE_ARRAY,DBUS_TYPE_VARIANT_AS_STRING,list);
  }

void gm_end_menu_item_list(DBusMessageIter * item,DBusMessageIter * list){
  dbus_message_iter_close_container(item,list);
  }


void gm_make_menu_item(DBusMessageIter * root,FXint id,const FXchar * label,const FXchar * icon,FXbool enabled) {
  DBusMessageIter item,list,variant;
  dbus_message_iter_open_container(root,DBUS_TYPE_VARIANT,"(ia{sv}av)",&variant);
    gm_begin_menu(&variant,&item,id,label,icon,enabled);
      gm_begin_menu_item_list(&item,&list);
      gm_end_menu_item_list(&item,&list);
    gm_end_menu(&variant,&item);
  dbus_message_iter_close_container(root,&variant);
  }


enum {
  ID_ROOT=0,
  ID_PLAY=1,
  ID_STOP,
  ID_PAUSE,
  ID_NEXT,
  ID_PREVIOUS,
  ID_QUIT
  };




DBusHandlerResult dbus_dbusmenu_filter(DBusConnection *connection,DBusMessage * msg,void * /*data*/){
  //GMAppStatusNotify * p = reinterpret_cast<GMAppStatusNotify*>(data);

  DEBUG_DBUS_MESSAGE(msg);

  DBusMessage *   reply;
  DBusMessageIter iter;
  DBusMessageIter item;
  DBusMessageIter list;
  FXuint serial;

  if (dbus_message_has_path(msg,APPLICATION_STATUS_ITEM_MENU_PATH)){
    if (dbus_message_has_interface(msg,DBUS_INTERFACE_INTROSPECTABLE)) {
      if (dbus_message_is_method_call(msg,DBUS_INTERFACE_INTROSPECTABLE,"Introspect")){
        return gm_dbus_reply_string(connection,msg,dbusmenu_xml);
        }
      }
    else if (dbus_message_has_interface(msg,DBUS_INTERFACE_PROPERTIES)) {
      if (dbus_message_is_method_call(msg,DBUS_INTERFACE_PROPERTIES,"Get")){
        }
      else if (dbus_message_is_method_call(msg,DBUS_INTERFACE_PROPERTIES,"GetAll")){
        }
      else if (dbus_message_is_method_call(msg,DBUS_INTERFACE_PROPERTIES,"Set")){
        }
      }
    else if (dbus_message_has_interface(msg,DBUS_MENU_INTERFACE)) {
      if (dbus_message_is_method_call(msg,DBUS_MENU_INTERFACE,"GetLayout")){
        FXint parent,depth;
        FXchar ** properties=NULL;
        FXint nprops;
        if (dbus_message_get_args(msg,NULL,DBUS_TYPE_INT32,&parent,DBUS_TYPE_INT32,&depth,DBUS_TYPE_ARRAY,DBUS_TYPE_STRING,&properties,&nprops,DBUS_TYPE_INVALID)) {
          if ((reply=dbus_message_new_method_return(msg))!=NULL) {
            FXuint revision=1;

            dbus_message_iter_init_append(reply,&iter);

            dbus_message_iter_append_basic(&iter,DBUS_TYPE_UINT32,&revision);

            gm_begin_menu(&iter,&item,ID_ROOT,"Goggles Music Manager","gogglesmm");
              gm_begin_menu_item_list(&item,&list);
                if (depth==-1 || depth==1) {
                  gm_make_menu_item(&list,ID_PLAY,"Play","media-playback-start",GMPlayerManager::instance()->can_play());
                  gm_make_menu_item(&list,ID_PAUSE,"Pause","media-playback-pause",GMPlayerManager::instance()->can_pause());
                  gm_make_menu_item(&list,ID_STOP,"Stop","media-playback-stop",GMPlayerManager::instance()->can_stop());
                  gm_make_menu_item(&list,ID_PREVIOUS,"Previous Track","media-skip-backward",GMPlayerManager::instance()->can_prev());
                  gm_make_menu_item(&list,ID_NEXT,"Next Track","media-skip-forward",GMPlayerManager::instance()->can_next());
                  }
              gm_end_menu_item_list(&item,&list);
            gm_end_menu(&iter,&item);
            dbus_connection_send(connection,reply,&serial);
            dbus_message_unref(reply);
            }
          dbus_free_string_array(properties);
          }
        return DBUS_HANDLER_RESULT_HANDLED;
        }
      else if (dbus_message_is_method_call(msg,DBUS_MENU_INTERFACE,"AboutToShow")) {
        FXint event_id;
        if (dbus_message_get_args(msg,NULL,DBUS_TYPE_INT32,&event_id,DBUS_TYPE_INVALID)) {
          gm_dbus_reply_bool(connection,msg,false);
          }
        return DBUS_HANDLER_RESULT_HANDLED;
        }
      else if (dbus_message_is_method_call(msg,DBUS_MENU_INTERFACE,"Event")) {
        FXint event_id;
        const FXchar * event_type=NULL;
        if (dbus_message_get_args(msg,NULL,DBUS_TYPE_INT32,&event_id,DBUS_TYPE_STRING,&event_type,DBUS_TYPE_INVALID)) {
          if (compare(event_type,"clicked")==0) {
            switch(event_id) {
              case ID_PLAY    : GMPlayerManager::instance()->cmd_play(); break;
              case ID_PAUSE   : GMPlayerManager::instance()->cmd_pause(); break;
              case ID_STOP    : GMPlayerManager::instance()->cmd_stop(); break;
              case ID_NEXT    : GMPlayerManager::instance()->cmd_next(); break;
              case ID_PREVIOUS: GMPlayerManager::instance()->cmd_prev(); break;
              }
            }
          return DBUS_HANDLER_RESULT_HANDLED;
          }
        }
      }
    }
  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }







static DBusObjectPathVTable org_freedesktop_statusnotifieritem={
  NULL,
  &dbus_status_item_filter,
  NULL,
  NULL,
  NULL,
  NULL
  };

static DBusObjectPathVTable org_freedesktop_dbusmenu={
  NULL,
  &dbus_dbusmenu_filter,
  NULL,
  NULL,
  NULL,
  NULL
  };


FXDEFMAP(GMAppStatusNotify) GMAppStatusNotifyMap[]={
  FXMAPFUNC(SEL_SIGNAL,0,GMAppStatusNotify::onSignal)
  };

FXIMPLEMENT(GMAppStatusNotify,FXObject,GMAppStatusNotifyMap,ARRAYNUMBER(GMAppStatusNotifyMap))

GMAppStatusNotify::GMAppStatusNotify(){
  }

GMAppStatusNotify::GMAppStatusNotify(GMDBus * b) : GMDBusProxy(b,APPLICATION_STATUS_NAME,APPLICATION_STATUS_PATH,APPLICATION_STATUS_INTERFACE)  {
  name = "org.kde.StatusNotifierItem-" + FXString::value(FXProcess::current()) + "-1";
  int result = dbus_bus_request_name(SESSIONBUS,name.text(),DBUS_NAME_FLAG_DO_NOT_QUEUE,NULL);
  if (result == DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER ) {
    dbus_connection_register_object_path(SESSIONBUS,APPLICATION_STATUS_ITEM_PATH,&org_freedesktop_statusnotifieritem,this);
    dbus_connection_register_object_path(SESSIONBUS,APPLICATION_STATUS_ITEM_MENU_PATH,&org_freedesktop_dbusmenu,this);
    }
  }

GMAppStatusNotify::~GMAppStatusNotify() {
  dbus_connection_unregister_object_path(SESSIONBUS,APPLICATION_STATUS_ITEM_PATH);
  dbus_connection_unregister_object_path(SESSIONBUS,APPLICATION_STATUS_ITEM_MENU_PATH);
  }

long GMAppStatusNotify::onSignal(FXObject*,FXSelector,void*) {
  //DBusMessage * msg = reinterpret_cast<DBusMessage*>(ptr);
  //FXASSERT(msg);
  return 1;
  }


void GMAppStatusNotify::show() {
  DBusMessage * msg;
  FXuint serial;
  if (dbus_bus_name_has_owner(SESSIONBUS,APPLICATION_STATUS_NAME,NULL)) {
    if ((msg = method("RegisterStatusNotifierItem"))!=NULL){
      const FXchar * cname = name.text();
      dbus_message_set_no_reply(msg,true);
      dbus_message_append_args(msg,DBUS_TYPE_STRING,&cname,DBUS_TYPE_INVALID);
      dbus_connection_send(SESSIONBUS,msg,&serial);
      dbus_message_unref(msg);
      }
    }
  }




void gm_enable_menu_item(DBusMessageIter * list,FXint id,FXbool enabled) {
  DBusMessageIter dict;
  DBusMessageIter item;
  dbus_message_iter_open_container(list,DBUS_TYPE_STRUCT,NULL,&item);
    dbus_message_iter_append_basic(&item,DBUS_TYPE_INT32,&id);
    dbus_message_iter_open_container(&item,DBUS_TYPE_ARRAY,"{sv}",&dict);
      gm_dbus_dict_append_bool(&dict,"enabled",enabled);
    dbus_message_iter_close_container(&item,&dict);
  dbus_message_iter_close_container(list,&item);
  }


void GMAppStatusNotify::notify_track_change(const GMTrack &){
  DBusMessage * msg = dbus_message_new_signal(APPLICATION_STATUS_ITEM_PATH,APPLICATION_STATUS_INTERFACE,"NewToolTip");
  if (msg) {
    bus->send(msg);
    }
  }

void GMAppStatusNotify::notify_status_change(){
  DBusMessage * msg = dbus_message_new_signal	(APPLICATION_STATUS_ITEM_MENU_PATH,DBUS_MENU_INTERFACE,"ItemsPropertiesUpdated");
  if (msg) {
    DBusMessageIter iter;
    DBusMessageIter updated;
    DBusMessageIter removed;

    dbus_message_set_no_reply(msg,true);
    dbus_message_iter_init_append(msg,&iter);
    dbus_message_iter_open_container(&iter,DBUS_TYPE_ARRAY,"(ia{sv})",&updated);
      gm_enable_menu_item(&updated,ID_PLAY,GMPlayerManager::instance()->can_play());
      gm_enable_menu_item(&updated,ID_PAUSE,GMPlayerManager::instance()->can_pause());
      gm_enable_menu_item(&updated,ID_STOP,GMPlayerManager::instance()->can_stop());
      gm_enable_menu_item(&updated,ID_NEXT,GMPlayerManager::instance()->can_next());
      gm_enable_menu_item(&updated,ID_PREVIOUS,GMPlayerManager::instance()->can_prev());
    dbus_message_iter_close_container(&iter,&updated);

    dbus_message_iter_open_container(&iter,DBUS_TYPE_ARRAY,"(ias)",&removed);
    dbus_message_iter_close_container(&iter,&removed);

    bus->send(msg);
    }
  }
