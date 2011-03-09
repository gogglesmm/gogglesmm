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
#include "GMAppStatusNotify.h"
#include "GMTrack.h"
#include "GMPlayerManager.h"

#include "appstatus_xml.h"


#define APPLICATION_STATUS_NAME       "org.kde.StatusNotifierWatcher"
#define APPLICATION_STATUS_PATH      "/StatusNotifierWatcher"
#define APPLICATION_STATUS_INTERFACE "org.kde.StatusNotifierWatcher"

#define APPLICATION_STATUS_ITEM_PATH "/StatusNotifierItem"
#define APPLICATION_STATUS_ITEM_INTERFACE "org.kde.StatusNotifierItem"
#define APPLICATION_STATUS_ITEM_MENU_PATH "/StatusMenu"


#define DBUS_MENU_INTERFACE "org.ayatana.dbusmenu"

#define SESSIONBUS (bus->connection())


DBusHandlerResult dbus_status_item_filter(DBusConnection *connection,DBusMessage * msg,void * data){
  DEBUG_DBUS_MESSAGE(msg);
  DBusMessage *   reply;
  DBusMessageIter iter;
  DBusMessageIter dict;
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
        FXchar * orientation;
        if (dbus_message_get_args(msg,NULL,DBUS_TYPE_INT32,&delta,DBUS_TYPE_STRING,&orientation,DBUS_TYPE_INVALID)) {
          FXint level = GMPlayerManager::instance()->volume();
          level+=(delta/120);
          GMPlayerManager::instance()->volume(level);

 //         fxmessage("Scroll: %d %s %d\n",delta,orientation,level);
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
            gm_dbus_dict_append_string(&dict,"IconName","gogglesmm_status_white");
            gm_dbus_dict_append_string(&dict,"Status","Active");
            gm_dbus_dict_append_uint32(&dict,"WindowId",GMPlayerManager::instance()->getMainWindowId());
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

static const FXchar gogglesmm_menu_xml[]=
  "<menu id=\"0\">"
  "  <menu id=\"1\">"
  "    <menu id=\"2\"/>"
  "    <menu id=\"3\"/>"
  "    <menu id=\"4\"/>"
  "    <menu id=\"5\"/>"
  "    <menu id=\"6\"/>"
  "  </menu>"
  "</menu>";




DBusHandlerResult dbus_dbusmenu_filter(DBusConnection *connection,DBusMessage * msg,void * data){
  GMAppStatusNotify * p = reinterpret_cast<GMAppStatusNotify*>(data);
  DEBUG_DBUS_MESSAGE(msg);
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
        FXint parent;
        if (dbus_message_get_args(msg,NULL,DBUS_TYPE_INT32,&parent,DBUS_TYPE_INVALID)) {
          return gm_dbus_reply_uint_string(connection,msg,0,gogglesmm_menu_xml);
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
  name = "org.kde.StatusNotifierItem-" + GMStringVal(fxgetpid()) + "-1";
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

long GMAppStatusNotify::onSignal(FXObject*,FXSelector,void*ptr) {
  DBusMessage * msg = reinterpret_cast<DBusMessage*>(ptr);
  FXASSERT(msg);
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
