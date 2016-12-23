/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2010-2017 by Sander Jansen. All Rights Reserved      *
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
#include "GMDBus.h"
#include "GMMediaPlayerService.h"
#include "GMSource.h"
#include "GMPlayerManager.h"
#include "GMWindow.h"
#include "GMCoverManager.h"
#include "GMAudioPlayer.h"

#include "mpris1_xml.h"
#include "mpris2_xml.h"


static void gm_mpris_track_to_dict(DBusMessageIter * iter,const GMTrack & track) {
  DBusMessageIter array;
  dbus_message_iter_open_container(iter,DBUS_TYPE_ARRAY,"{sv}",&array);
  gm_dbus_dict_append_string(&array,"title",track.title);
  gm_dbus_dict_append_string(&array,"artist",track.artist);
  gm_dbus_dict_append_string(&array,"album",track.album);
  gm_dbus_dict_append_string(&array,"tracknumber",FXString::value(track.no));
  gm_dbus_dict_append_uint32(&array,"time",track.time);
  gm_dbus_dict_append_uint32(&array,"year",track.year);
  if (track.tags.no())
    gm_dbus_dict_append_string(&array,"genre",track.tags[0]);

  if (dbus_validate_utf8(track.url.text(),NULL))
    gm_dbus_dict_append_string(&array,"location",gm_make_url(track.url));

  const FXString & arturl = GMPlayerManager::instance()->getCoverManager()->getShareFilename();
  if (!arturl.empty())
    gm_dbus_dict_append_string(&array,"arturl",FXURL::fileToURL(arturl));

  dbus_message_iter_close_container(iter,&array);
  }


static const FXchar MPRIS_DBUS_NAME[]="org.mpris.gogglesmm";
static const FXchar MPRIS_DBUS_INTERFACE[]="org.freedesktop.MediaPlayer";
static const FXchar MPRIS_DBUS_PLAYER[]="/Player";
static const FXchar MPRIS_DBUS_ROOT[]="/";
static const FXchar MPRIS_DBUS_TRACKLIST[]="/TrackList";

static FXint mpris_get_caps(GMPlayerManager * p) {
  FXint caps=0;
  enum {
    MPRIS_CAPS_NONE                  = 0,
    MPRIS_CAPS_CAN_GO_NEXT           = 1 << 0,
    MPRIS_CAPS_CAN_GO_PREV           = 1 << 1,
    MPRIS_CAPS_CAN_PAUSE             = 1 << 2,
    MPRIS_CAPS_CAN_PLAY              = 1 << 3,
    MPRIS_CAPS_CAN_SEEK              = 1 << 4,
    MPRIS_CAPS_CAN_PROVIDE_METADATA  = 1 << 5,
    MPRIS_CAPS_CAN_HAS_TRACKLIST     = 1 << 6
   };
  if (p->can_play() || p->can_unpause()) caps|=MPRIS_CAPS_CAN_PLAY;
  if (p->can_pause()) caps|=MPRIS_CAPS_CAN_PAUSE;
  if (p->can_prev()) caps|=MPRIS_CAPS_CAN_GO_PREV;
  if (p->can_next()) caps|=MPRIS_CAPS_CAN_GO_NEXT;
  caps|=MPRIS_CAPS_CAN_PROVIDE_METADATA;
  return caps;
  }


static void gm_mpris_get_status(DBusMessageIter * iter,GMPlayerManager * p) {
  enum {
    MPRIS_STATUS_PLAYING = 0,
    MPRIS_STATUS_PAUSED  = 1,
    MPRIS_STATUS_STOPPED = 2,
    };

  FXint playstatus=0;
  FXint playmode=0;
  FXint playnext=0;
  FXint playrepeat=0;

  if (p->can_unpause())
    playstatus=MPRIS_STATUS_PAUSED;
  else if (p->can_pause())
    playstatus=MPRIS_STATUS_PLAYING;
  else
    playstatus=MPRIS_STATUS_STOPPED;

  DBusMessageIter str;
  dbus_message_iter_open_container(iter,DBUS_TYPE_STRUCT,nullptr,&str);
  dbus_message_iter_append_basic(&str,DBUS_TYPE_INT32,&playstatus);
  dbus_message_iter_append_basic(&str,DBUS_TYPE_INT32,&playmode);
  dbus_message_iter_append_basic(&str,DBUS_TYPE_INT32,&playnext);
  dbus_message_iter_append_basic(&str,DBUS_TYPE_INT32,&playrepeat);
  dbus_message_iter_close_container(iter,&str);
  }

FXIMPLEMENT(GMMediaPlayerService1,FXObject,nullptr,0)

GMMediaPlayerService1::GMMediaPlayerService1(GMDBus * b) : bus(b){

  memset(&root_vtable,0,sizeof(DBusObjectPathVTable));
  root_vtable.message_function=&root_filter;
  memset(&player_vtable,0,sizeof(DBusObjectPathVTable));
  player_vtable.message_function=&player_filter;
  memset(&tracklist_vtable,0,sizeof(DBusObjectPathVTable));
  tracklist_vtable.message_function=&tracklist_filter;

  int result = dbus_bus_request_name(bus->connection(),MPRIS_DBUS_NAME,DBUS_NAME_FLAG_DO_NOT_QUEUE,nullptr);
  if (result == DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER ) {
    dbus_connection_register_object_path(bus->connection(),MPRIS_DBUS_ROOT,&root_vtable,this);
    dbus_connection_register_object_path(bus->connection(),MPRIS_DBUS_PLAYER,&player_vtable,this);
    dbus_connection_register_object_path(bus->connection(),MPRIS_DBUS_TRACKLIST,&tracklist_vtable,this);
    published=true;
    }
  }

GMMediaPlayerService1::~GMMediaPlayerService1(){
  if (published) {
    dbus_connection_unregister_object_path(bus->connection(),MPRIS_DBUS_ROOT);
    dbus_connection_unregister_object_path(bus->connection(),MPRIS_DBUS_PLAYER);
    dbus_connection_unregister_object_path(bus->connection(),MPRIS_DBUS_TRACKLIST);
    published=false;
    dbus_bus_release_name(bus->connection(),MPRIS_DBUS_NAME,nullptr);
    }
  }


void GMMediaPlayerService1::notify_track_change(const GMTrack & info) {
  if (published) {
    DBusMessage * msg = dbus_message_new_signal(MPRIS_DBUS_PLAYER,MPRIS_DBUS_INTERFACE,"TrackChange");
    if (msg) {
      DBusMessageIter iter;
      dbus_message_iter_init_append(msg,&iter);
      gm_mpris_track_to_dict(&iter,info);
      bus->send(msg);
      }
    }
  }

void GMMediaPlayerService1::notify_status_change() {
  DBusMessage * msg = dbus_message_new_signal(MPRIS_DBUS_PLAYER,MPRIS_DBUS_INTERFACE,"StatusChange");
  if (msg) {
    DBusMessageIter iter;
    dbus_message_iter_init_append(msg,&iter);
    gm_mpris_get_status(&iter,GMPlayerManager::instance());
    bus->send(msg);
    }
  }

void GMMediaPlayerService1::notify_caps_change() {
  if (published) {
    DBusMessage * msg = dbus_message_new_signal(MPRIS_DBUS_PLAYER,MPRIS_DBUS_INTERFACE,"CapsChange");
    if (msg) {
      FXint caps = mpris_get_caps(GMPlayerManager::instance());
      dbus_message_append_args(msg,DBUS_TYPE_INT32,&caps,DBUS_TYPE_INVALID);
      bus->send(msg);
      }
    }
  }

DBusHandlerResult GMMediaPlayerService1::root_filter(DBusConnection *connection,DBusMessage * msg,void * /*ptr*/){
  DEBUG_DBUS_MESSAGE(msg);
  DBusMessage * reply=nullptr;
  FXuint serial;
  GMPlayerManager      * p     = GMPlayerManager::instance();
  if (dbus_message_is_method_call(msg,"org.freedesktop.DBus.Introspectable","Introspect")){
    FXString xml(mpris_xml);
    char ** children=nullptr;
    if (dbus_connection_list_registered(connection,"/",&children)) {
      for (FXint i=0;children[i]!=nullptr;i++) {
        xml+=FXString::value("\t<node name=\"%s\"/>\n",children[i]);
        }
      dbus_free_string_array(children);
      }
    xml+="</node>";
    return gm_dbus_reply_string(connection,msg,xml.text());
    }
  else if (dbus_message_is_method_call(msg,MPRIS_DBUS_INTERFACE,"Identity")) {
    return gm_dbus_reply_string(connection,msg,"Goggles Music Manager");
    }
  else if (dbus_message_is_method_call(msg,MPRIS_DBUS_INTERFACE,"MprisVersion")) {
    reply = dbus_message_new_method_return(msg);
    if (reply) {
      FXushort major=1,minor=0;
      DBusMessageIter iter;
      DBusMessageIter str;
      dbus_message_iter_init_append(reply,&iter);
      dbus_message_iter_open_container(&iter,DBUS_TYPE_STRUCT,nullptr,&str);
      dbus_message_iter_append_basic(&str,DBUS_TYPE_UINT16,&major);
      dbus_message_iter_append_basic(&str,DBUS_TYPE_UINT16,&minor);
      dbus_message_iter_close_container(&iter,&str);
      dbus_connection_send(connection,reply,&serial);
      dbus_message_unref(reply);
      }
    return DBUS_HANDLER_RESULT_HANDLED;
    }
  if (dbus_message_is_method_call(msg,MPRIS_DBUS_INTERFACE,"Quit")) {
    gm_dbus_reply_if_needed(connection,msg);
    if (p->getMainWindow()) p->getMainWindow()->handle(p,FXSEL(SEL_COMMAND,GMWindow::ID_QUIT),nullptr);
    return DBUS_HANDLER_RESULT_HANDLED;
    }
  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }


DBusHandlerResult  GMMediaPlayerService1::player_filter(DBusConnection *connection,DBusMessage * msg,void * /*ptr*/){
  DEBUG_DBUS_MESSAGE(msg);
  DBusMessage * reply=nullptr;
  FXuint serial;
  GMPlayerManager      * p     = GMPlayerManager::instance();

  if (dbus_message_is_method_call(msg,"org.freedesktop.DBus.Introspectable","Introspect")){
    return gm_dbus_reply_string(connection,msg,mpris_player_xml);
    }
  else if (dbus_message_is_method_call(msg,MPRIS_DBUS_INTERFACE,"GetCaps")) {
    FXint caps = mpris_get_caps(p);
    return gm_dbus_reply_int(connection,msg,caps);
    }
  else if (dbus_message_is_method_call(msg,MPRIS_DBUS_INTERFACE,"PositionGet")) {
    return gm_dbus_reply_int(connection,msg,p->getPlayer()->getPosition()*1000);
    }
  else if (dbus_message_is_method_call(msg,MPRIS_DBUS_INTERFACE,"VolumeSet")) {
    return gm_dbus_reply_if_needed(connection,msg);
    }
  else if (dbus_message_is_method_call(msg,MPRIS_DBUS_INTERFACE,"VolumeGet")) {
    return gm_dbus_reply_int(connection,msg,p->volume());
    }
  else if (dbus_message_is_method_call(msg,MPRIS_DBUS_INTERFACE,"GetStatus")) {
    reply = dbus_message_new_method_return(msg);
    if (reply) {
      DBusMessageIter iter;
      dbus_message_iter_init_append(reply,&iter);
      gm_mpris_get_status(&iter,p);
      dbus_connection_send(connection,reply,&serial);
      dbus_message_unref(reply);
      }
    return DBUS_HANDLER_RESULT_HANDLED;
    }
  else if (dbus_message_is_method_call(msg,MPRIS_DBUS_INTERFACE,"GetMetadata")) {
    reply = dbus_message_new_method_return(msg);
    if (reply) {
      DBusMessageIter iter;
      GMTrack track;
      p->getTrackInformation(track);
      dbus_message_iter_init_append(reply,&iter);
      gm_mpris_track_to_dict(&iter,track);
      dbus_connection_send(connection,reply,&serial);
      dbus_message_unref(reply);
      }
    return DBUS_HANDLER_RESULT_HANDLED;
    }
  else if (dbus_message_is_method_call(msg,MPRIS_DBUS_INTERFACE,"Play")){
    p->cmd_play();
    return gm_dbus_reply_if_needed(connection,msg);
    }
  else if (dbus_message_is_method_call(msg,MPRIS_DBUS_INTERFACE,"Stop")){
    p->cmd_stop();
    return gm_dbus_reply_if_needed(connection,msg);
    }
  else if (dbus_message_is_method_call(msg,MPRIS_DBUS_INTERFACE,"Pause")){
    p->cmd_pause();
    return gm_dbus_reply_if_needed(connection,msg);
    }
  else if (dbus_message_is_method_call(msg,MPRIS_DBUS_INTERFACE,"Next")){
    p->cmd_next();
    return gm_dbus_reply_if_needed(connection,msg);
    }
  else if (dbus_message_is_method_call(msg,MPRIS_DBUS_INTERFACE,"Prev")){
    p->cmd_prev();
    return gm_dbus_reply_if_needed(connection,msg);
    }
  else if (dbus_message_is_method_call(msg,MPRIS_DBUS_INTERFACE,"Repeat")){
    return gm_dbus_reply_if_needed(connection,msg);
    }
  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }




DBusHandlerResult  GMMediaPlayerService1::tracklist_filter(DBusConnection *connection,DBusMessage * msg,void * /*ptr*/){
  DEBUG_DBUS_MESSAGE(msg);
  DBusMessage * reply=nullptr;
  FXuint serial;
  GMPlayerManager      * p     = GMPlayerManager::instance();

  if (dbus_message_is_method_call(msg,"org.freedesktop.DBus.Introspectable","Introspect")){
    return gm_dbus_reply_string(connection,msg,mpris_tracklist_xml);
    }
  else if (dbus_message_is_method_call(msg,MPRIS_DBUS_INTERFACE,"GetMetadata")) {
    reply = dbus_message_new_method_return(msg);
    if (reply) {
      DBusMessageIter iter;
      GMTrack track;
      p->getTrackInformation(track);
      dbus_message_iter_init_append(reply,&iter);
      gm_mpris_track_to_dict(&iter,track);
      dbus_connection_send(connection,reply,&serial);
      dbus_message_unref(reply);
      }
    return DBUS_HANDLER_RESULT_HANDLED;
    }
  else if (dbus_message_is_method_call(msg,MPRIS_DBUS_INTERFACE,"GetLength")) {
    return gm_dbus_reply_int(connection,msg,0);
    }
  else if (dbus_message_is_method_call(msg,MPRIS_DBUS_INTERFACE,"GetCurrentTrack")) {
    return gm_dbus_reply_int(connection,msg,0);
    }
  else if (dbus_message_is_method_call(msg,MPRIS_DBUS_INTERFACE,"AddTrack")) {
    return gm_dbus_reply_int(connection,msg,1);
    }
  else if (dbus_message_is_method_call(msg,MPRIS_DBUS_INTERFACE,"DelTrack")) {
    return gm_dbus_reply_if_needed(connection,msg);
    }
  else if (dbus_message_is_method_call(msg,MPRIS_DBUS_INTERFACE,"SetLoop")) {
    return gm_dbus_reply_if_needed(connection,msg);
    }
  else if (dbus_message_is_method_call(msg,MPRIS_DBUS_INTERFACE,"SetRandom")) {
    return gm_dbus_reply_if_needed(connection,msg);
    }
  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }




//------------------------------------------------------------------
// MPRIS 2
//------------------------------------------------------------------

static void gm_mpris2_track_to_dict(DBusMessageIter * iter,const GMTrack & track) {
  DBusMessageIter array;
  dbus_message_iter_open_container(iter,DBUS_TYPE_ARRAY,"{sv}",&array);
  gm_dbus_dict_append_long(&array,"mpris:length",track.time*1000000);
  gm_dbus_dict_append_string(&array,"xesam:title",track.title);
  gm_dbus_dict_append_string_list(&array,"xesam:artist",FXStringList(track.artist,1));
  if (!track.album_artist.empty())
    gm_dbus_dict_append_string_list(&array,"xesam:albumArtist",FXStringList(track.album_artist,1));
  gm_dbus_dict_append_string(&array,"xesam:album",track.album);
  if (!track.composer.empty())
    gm_dbus_dict_append_string_list(&array,"xesam:composer",FXStringList(track.composer,1));

  if (dbus_validate_utf8(track.url.text(),NULL))
    gm_dbus_dict_append_string(&array,"xesam:url",gm_make_url(track.url));

  const FXString & arturl = GMPlayerManager::instance()->getCoverManager()->getShareFilename();
  if (!arturl.empty())
    gm_dbus_dict_append_string(&array,"mpris:artUrl",FXURL::fileToURL(arturl));
  dbus_message_iter_close_container(iter,&array);
  }

static void gm_dbus_dict_append_track(DBusMessageIter * iter,const FXchar * key,const GMTrack & track) {
  DBusMessageIter entry;
  DBusMessageIter variant;
  dbus_message_iter_open_container(iter,DBUS_TYPE_DICT_ENTRY,0,&entry);
    dbus_message_iter_append_basic(&entry,DBUS_TYPE_STRING,&key);
    dbus_message_iter_open_container(&entry,DBUS_TYPE_VARIANT,"a{sv}",&variant);
      gm_mpris2_track_to_dict(&variant,track);
    dbus_message_iter_close_container(&entry,&variant);
  dbus_message_iter_close_container(iter,&entry);
  }







static const FXchar MPRIS2_NAME[]="org.mpris.MediaPlayer2.gogglesmm";
static const FXchar MPRIS2_PATH[]="/org/mpris/MediaPlayer2";
static const FXchar MPRIS2_ROOT[]="org.mpris.MediaPlayer2";
static const FXchar MPRIS2_PLAYER[]="org.mpris.MediaPlayer2.Player";
static const FXchar DBUS_INTROSPECTABLE[]="org.freedesktop.DBus.Introspectable";
static const FXchar DBUS_PROPERTIES[]="org.freedesktop.DBus.Properties";


static const FXchar * mpris_play_status(GMPlayerManager * p){
  if (p->can_unpause())
    return "Paused";
  else if (p->can_pause())
    return "Playing";
  else
    return "Stopped";
  }




FXIMPLEMENT(GMMediaPlayerService2,FXObject,nullptr,0)

GMMediaPlayerService2::GMMediaPlayerService2(GMDBus * b) : bus(b){
  memset(&mpris_vtable,0,sizeof(DBusObjectPathVTable));
  mpris_vtable.message_function=&mpris_filter;
  }

FXint GMMediaPlayerService2::create() {
  FXint result = dbus_bus_request_name(bus->connection(),MPRIS2_NAME,DBUS_NAME_FLAG_DO_NOT_QUEUE,nullptr);
  if (result == DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) {
    if (!dbus_connection_register_object_path(bus->connection(),MPRIS2_PATH,&mpris_vtable,this)) {
      // force disable dbus functionality
      return -1;
      }
    registered=true;
    }
  return result;
  }

GMMediaPlayerService2::~GMMediaPlayerService2(){
  if (registered) {
    dbus_connection_unregister_object_path(bus->connection(),MPRIS2_PATH);
    dbus_bus_release_name(bus->connection(),MPRIS2_NAME,nullptr);
    registered=false;
    }
  }

void GMMediaPlayerService2::notify_seek(FXuint position){
  DBusMessage * msg = dbus_message_new_signal(MPRIS2_PATH,MPRIS2_PLAYER,"Seeked");
  if (msg) {
    FXlong p = 1000000 * (FXlong)position;
    dbus_message_append_args(msg,DBUS_TYPE_INT64,&p,DBUS_TYPE_INVALID);
    bus->send(msg);
    }
  }


void GMMediaPlayerService2::notify_track_change(const GMTrack & track){
  DBusMessage * msg = dbus_message_new_signal(MPRIS2_PATH,DBUS_PROPERTIES,"PropertiesChanged");
  if (msg) {
    DBusMessageIter iter,dict,array;
    dbus_message_iter_init_append(msg,&iter);
    gm_dbus_append_string(&iter,MPRIS2_PLAYER);
    dbus_message_iter_open_container(&iter,DBUS_TYPE_ARRAY,"{sv}",&dict);
    gm_dbus_dict_append_track(&dict,"Metadata",track);
    dbus_message_iter_close_container(&iter,&dict);
    dbus_message_iter_open_container(&iter,DBUS_TYPE_ARRAY,"s",&array);
    dbus_message_iter_close_container(&iter,&array);
    bus->send(msg);
    }
  }

void GMMediaPlayerService2::notify_status_change(){
  DBusMessage * msg = dbus_message_new_signal(MPRIS2_PATH,DBUS_PROPERTIES,"PropertiesChanged");
  GMPlayerManager * p = GMPlayerManager::instance();
  if (msg) {
    DBusMessageIter iter,dict,array;
    dbus_message_iter_init_append(msg,&iter);
    gm_dbus_append_string(&iter,MPRIS2_PLAYER);
    dbus_message_iter_open_container(&iter,DBUS_TYPE_ARRAY,"{sv}",&dict);
    gm_dbus_dict_append_string(&dict,"PlaybackStatus",mpris_play_status(p));
    gm_dbus_dict_append_bool(&dict,"CanGoNext",p->can_next());
    gm_dbus_dict_append_bool(&dict,"CanGoPrevious",p->can_prev());
    gm_dbus_dict_append_bool(&dict,"CanPlay",p->can_play());
    gm_dbus_dict_append_bool(&dict,"CanPause",p->can_pause());
    dbus_message_iter_close_container(&iter,&dict);
    dbus_message_iter_open_container(&iter,DBUS_TYPE_ARRAY,"s",&array);
    dbus_message_iter_close_container(&iter,&array);
    bus->send(msg);
    }
  }

void GMMediaPlayerService2::notify_caps_change(){
  }

void GMMediaPlayerService2::notify_volume(FXint volume){
  if (volume>=0) {
    DBusMessage * msg = dbus_message_new_signal(MPRIS2_PATH,DBUS_PROPERTIES,"PropertiesChanged");
    if (msg) {
      DBusMessageIter iter,dict,array;
      dbus_message_iter_init_append(msg,&iter);
      gm_dbus_append_string(&iter,MPRIS2_PLAYER);
      dbus_message_iter_open_container(&iter,DBUS_TYPE_ARRAY,"{sv}",&dict);
      gm_dbus_dict_append_double(&dict,"Volume",volume/100.0f);
      dbus_message_iter_close_container(&iter,&dict);
      dbus_message_iter_open_container(&iter,DBUS_TYPE_ARRAY,"s",&array);
      dbus_message_iter_close_container(&iter,&array);
      bus->send(msg);
      }
    }
  }


static void mpris_root_property_set(const FXchar * prop,FXVariant & value) {
  if (compare(prop,"Fullscreen")==0) {
    if (value.isBool()) GMPlayerManager::instance()->getMainWindow()->setFullScreen(value.toBool());
    }
  }


static DBusHandlerResult mpris_root_property_get(DBusConnection *connection,DBusMessage * msg,const FXchar * prop){
  static const FXchar * schemes[]={"file","http",NULL};
  static const FXchar * mimetypes[]={"audio/flac","audio/ogg","audio/opus","audio/mpeg","audio/mp4a-latm",NULL};
  static const FXchar prop_identity[]="Goggles Music Manager";
  static const FXchar prop_desktopentry[]="gogglesmm";
  if (prop==nullptr) {
    FXuint serial;
    DBusMessage *   reply;
    DBusMessageIter iter;
    DBusMessageIter dict;
    if ((reply=dbus_message_new_method_return(msg))!=nullptr) {
      dbus_message_iter_init_append(reply,&iter);
      dbus_message_iter_open_container(&iter,DBUS_TYPE_ARRAY,"{sv}",&dict);
      gm_dbus_dict_append_string(&dict,"Identity",prop_identity);
      gm_dbus_dict_append_string(&dict,"DesktopEntry",prop_desktopentry);
      gm_dbus_dict_append_bool(&dict,"CanQuit",true);
      gm_dbus_dict_append_bool(&dict,"Fullscreen",GMPlayerManager::instance()->getMainWindow()->isFullScreen());
      gm_dbus_dict_append_bool(&dict,"CanSetFullscreen",true);
      gm_dbus_dict_append_bool(&dict,"CanRaise",true);
      gm_dbus_dict_append_bool(&dict,"HasTrackList",false);
      gm_dbus_dict_append_string_list(&dict,"SupportedUriSchemes",schemes);
      gm_dbus_dict_append_string_list(&dict,"SupportedMimeTypes",mimetypes);
      dbus_message_iter_close_container(&iter,&dict);
      dbus_connection_send(connection,reply,&serial);
      dbus_message_unref(reply);
      }
    return DBUS_HANDLER_RESULT_HANDLED;
    }
  else if (compare(prop,"Identity")==0)             return gm_dbus_property_string(connection,msg,prop_identity);
  else if (compare(prop,"DesktopEntry")==0)         return gm_dbus_property_string(connection,msg,prop_desktopentry);
  else if (compare(prop,"CanQuit")==0)              return gm_dbus_property_bool(connection,msg,true);
  else if (compare(prop,"Fullscreen")==0)           return gm_dbus_property_bool(connection,msg,GMPlayerManager::instance()->getMainWindow()->isFullScreen());
  else if (compare(prop,"CanSetFullscreen")==0)     return gm_dbus_property_bool(connection,msg,true);
  else if (compare(prop,"CanRaise")==0)             return gm_dbus_property_bool(connection,msg,true);
  else if (compare(prop,"HasTrackList")==0)         return gm_dbus_property_bool(connection,msg,false);
  else if (compare(prop,"SupportedUriSchemes")==0)  return gm_dbus_property_string_list(connection,msg,schemes);
  else if (compare(prop,"SupportedMimeTypes")==0)   return gm_dbus_property_string_list(connection,msg,mimetypes);
  else return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }


static const FXchar * mpris_loop_status(GMPlayerManager * player) {
  if (!player->getPlayQueue()) {
    FXuint repeat = player->getPreferences().play_repeat;
    if (repeat == REPEAT_TRACK)
      return "Track";
    else if (repeat == REPEAT_ALL)
      return "Playlist";
    }
  return "None";
  }


static void mpris_player_property_set(const FXchar * prop,FXVariant & value) {
  if (compare(prop,"LoopStatus")==0) {
    if (GMPlayerManager::instance()->getPlayQueue()) return;
    if (!value.isString()) return;
    FXString state = value.toString();
    if (state=="None")
      GMPlayerManager::instance()->getPreferences().play_repeat = REPEAT_OFF;
    else if (state=="Track")
      GMPlayerManager::instance()->getPreferences().play_repeat = REPEAT_TRACK;
    else if (state=="Playlist")
      GMPlayerManager::instance()->getPreferences().play_repeat = REPEAT_ALL;
    }
  else if (compare(prop,"Shuffle")==0){
    if (!value.isBool()) return;
    GMPlayerManager::instance()->getPreferences().play_shuffle = value.toBool();
    }
  else if (compare(prop,"Position")==0){
    GMPlayerManager::instance()->seekTime((FXint)(value.asLong()/1000000));
    }
  else if (compare(prop,"Volume")==0){
    GMPlayerManager::instance()->volume(FXCLAMP(0,(FXint)(value.asDouble()*100),100));
    }
  /*
    Not implemented:
      Rate -> can not be changed, can only be 1.0. Will ignore it because spec is unclear what to do about 0.0:
              The value must fall in the range described by MinimumRate and MaximumRate, and must not be 0.0
              A value of 0.0 should not be set by the client. If it is, the media player should act as though Pause was called.
  */
  return;
  }

static DBusHandlerResult mpris_player_property_get(DBusConnection *c,DBusMessage * msg,const FXchar * prop){
  FXuint serial;
  GMPlayerManager * p = GMPlayerManager::instance();
  if (prop==nullptr) {
    DBusMessage *   reply;
    DBusMessageIter iter;
    DBusMessageIter dict;
    if ((reply=dbus_message_new_method_return(msg))!=nullptr) {
      GMTrack track;
      p->getTrackInformation(track);
      dbus_message_iter_init_append(reply,&iter);
      dbus_message_iter_open_container(&iter,DBUS_TYPE_ARRAY,"{sv}",&dict);
      gm_dbus_dict_append_string(&dict,"PlaybackStatus",mpris_play_status(p));
      gm_dbus_dict_append_string(&dict,"LoopStatus",mpris_loop_status(p));
      gm_dbus_dict_append_bool(&dict,"Shuffle",p->getPreferences().play_shuffle);
      gm_dbus_dict_append_track(&dict,"Metadata",track);
      gm_dbus_dict_append_double(&dict,"Volume",p->volume()>=0 ? (p->volume()/100.0f) : 0.0f);
      gm_dbus_dict_append_long(&dict,"Position",((FXlong)p->getPlayer()->getPosition())*1000000);
      gm_dbus_dict_append_double(&dict,"Rate",1.0);
      gm_dbus_dict_append_double(&dict,"MinimumRate",1.0);
      gm_dbus_dict_append_double(&dict,"MaximumRate",1.0);
      gm_dbus_dict_append_bool(&dict,"CanGoNext",p->can_next());
      gm_dbus_dict_append_bool(&dict,"CanGoPrevious",p->can_prev());
      gm_dbus_dict_append_bool(&dict,"CanPlay",p->can_play());
      gm_dbus_dict_append_bool(&dict,"CanPause",p->can_pause());
      gm_dbus_dict_append_bool(&dict,"CanSeek",true);
      gm_dbus_dict_append_bool(&dict,"CanControl",true);
      dbus_message_iter_close_container(&iter,&dict);
      dbus_connection_send(c,reply,&serial);
      dbus_message_unref(reply);
      }
    }
  else if (compare(prop,"Metadata")==0) {
    DBusMessage * reply;
    if ((reply=dbus_message_new_method_return(msg))!=nullptr) {
      DBusMessageIter iter;
      DBusMessageIter container;
      dbus_message_iter_init_append(reply,&iter);

      dbus_message_iter_open_container(&iter,DBUS_TYPE_VARIANT,"a{sv}",&container);
      GMTrack track;
      p->getTrackInformation(track);

      gm_mpris2_track_to_dict(&container,track);
      dbus_message_iter_close_container(&iter,&container);
      dbus_connection_send(c,reply,&serial);
      dbus_message_unref(reply);
      }
    return DBUS_HANDLER_RESULT_HANDLED;
    }
  else if (compare(prop,"PlaybackStatus")==0) return gm_dbus_property_string(c,msg,mpris_play_status(p));
  else if (compare(prop,"LoopStatus")==0)     return gm_dbus_property_string(c,msg,mpris_loop_status(p));
  else if (compare(prop,"Shuffle")==0)        return gm_dbus_property_bool(c,msg,p->getPreferences().play_shuffle);
  else if (compare(prop,"Volume")==0)         return gm_dbus_property_double(c,msg,p->volume()>=0 ? (p->volume()/100.0f) : 0.0f);
  else if (compare(prop,"Position")==0)       return gm_dbus_property_long(c,msg,((FXlong)p->getPlayer()->getPosition())*1000000);
  else if (compare(prop,"Rate")==0)           return gm_dbus_property_double(c,msg,1.0);
  else if (compare(prop,"MinimumRate")==0)    return gm_dbus_property_double(c,msg,1.0);
  else if (compare(prop,"MaximumRate")==0)    return gm_dbus_property_double(c,msg,1.0);
  else if (compare(prop,"CanGoNext")==0)      return gm_dbus_property_bool(c,msg,p->can_next());
  else if (compare(prop,"CanGoPrevious")==0)  return gm_dbus_property_bool(c,msg,p->can_prev());
  else if (compare(prop,"CanPlay")==0)        return gm_dbus_property_bool(c,msg,p->can_play());
  else if (compare(prop,"CanPause")==0)       return gm_dbus_property_bool(c,msg,p->can_pause());
  else if (compare(prop,"CanControl")==0)     return gm_dbus_property_bool(c,msg,true);
  else if (compare(prop,"CanSeek")==0)        return gm_dbus_property_bool(c,msg,true);
  return DBUS_HANDLER_RESULT_HANDLED;
  }



static FXVariant get_property(DBusMessageIter * iter) {
  DBusMessageIter subiter;
  dbus_message_iter_recurse(iter,&subiter);
  switch(dbus_message_iter_get_arg_type(&subiter)){
    case DBUS_TYPE_INT64:
      {
        FXlong value;
        dbus_message_iter_get_basic(&subiter,&value);
        return FXVariant(value);
      }
      break;
    case DBUS_TYPE_DOUBLE:
      {
        FXdouble volume;
        dbus_message_iter_get_basic(&subiter,&volume);
        return FXVariant(volume);
      }
      break;
    case DBUS_TYPE_BOOLEAN:
      {
        dbus_bool_t condition;
        dbus_message_iter_get_basic(&subiter,&condition);
        return FXVariant(static_cast<FXbool>(condition));
      }
      break;
    case DBUS_TYPE_STRING:
      {
        const FXchar * value=nullptr;
        dbus_message_iter_get_basic(&subiter,&value);
        return FXVariant(value);
      }
      break;
    default: break;
    }
  return FXVariant();
  }


DBusHandlerResult GMMediaPlayerService2::mpris_filter(DBusConnection * c,DBusMessage * msg,void*){
  GMPlayerManager * p = GMPlayerManager::instance();

  if (dbus_message_has_interface(msg,DBUS_INTROSPECTABLE)) {
    if (dbus_message_is_method_call(msg,DBUS_INTROSPECTABLE,"Introspect")){
      return gm_dbus_reply_string(c,msg,mpris2_xml);
      }
    return DBUS_HANDLER_RESULT_HANDLED;
    }
  else if (dbus_message_has_interface(msg,DBUS_PROPERTIES)) {
    if (dbus_message_is_method_call(msg,DBUS_PROPERTIES,"Get")){
      const FXchar * interface=nullptr;
      const FXchar * property=nullptr;
      if (dbus_message_get_args(msg,nullptr,DBUS_TYPE_STRING,&interface,DBUS_TYPE_STRING,&property,DBUS_TYPE_INVALID)) {
        if (compare(interface,MPRIS2_ROOT)==0) {
          return mpris_root_property_get(c,msg,property);
          }
        else if (compare(interface,MPRIS2_PLAYER)==0) {
          return mpris_player_property_get(c,msg,property);
          }
        }
      return DBUS_HANDLER_RESULT_HANDLED;
      }
    else if (dbus_message_is_method_call(msg,DBUS_PROPERTIES,"GetAll")){
      const FXchar * interface=nullptr;
      if (dbus_message_get_args(msg,nullptr,DBUS_TYPE_STRING,&interface,DBUS_TYPE_INVALID)) {
        if (compare(interface,MPRIS2_ROOT)==0) {
          return mpris_root_property_get(c,msg,nullptr);
          }
        else if (compare(interface,MPRIS2_PLAYER)==0) {
          return mpris_player_property_get(c,msg,nullptr);
          }
        }
      return DBUS_HANDLER_RESULT_HANDLED;
      }
    else if (dbus_message_is_method_call(msg,DBUS_PROPERTIES,"Set")){
      const FXchar * interface=nullptr;
      const FXchar * property=nullptr;
      DBusMessageIter iter;
      dbus_message_iter_init(msg,&iter);
      if (dbus_message_iter_get_arg_type(&iter)==DBUS_TYPE_STRING) {
        dbus_message_iter_get_basic	(&iter,&interface);
        if (dbus_message_iter_next(&iter) && dbus_message_iter_get_arg_type(&iter)==DBUS_TYPE_STRING) {
          dbus_message_iter_get_basic	(&iter,&property);
          dbus_message_iter_next(&iter);
          if (dbus_message_iter_get_arg_type(&iter)==DBUS_TYPE_VARIANT) {
            if (compare(interface,MPRIS2_PLAYER)==0) {
              FXVariant v = get_property(&iter);
              mpris_player_property_set(property,v);
              }
            else if (compare(interface,MPRIS2_ROOT)==0) {
              FXVariant v = get_property(&iter);
              mpris_root_property_set(property,v);
              }
            }
          }
        }
      return gm_dbus_reply_if_needed(c,msg);
      }
    }
  else if (dbus_message_has_interface(msg,MPRIS2_ROOT)) {

    if (dbus_message_is_method_call(msg,MPRIS2_ROOT,"Raise")) {
      p->cmd_raise();
      return gm_dbus_reply_if_needed(c,msg);
      }
    else if (dbus_message_is_method_call(msg,MPRIS2_ROOT,"Quit")) {
      gm_dbus_reply_if_needed(c,msg);
      if (p->getMainWindow()) p->getMainWindow()->handle(p,FXSEL(SEL_COMMAND,GMWindow::ID_QUIT),nullptr);
      return DBUS_HANDLER_RESULT_HANDLED;
      }

    // Undocumented gogglesmm specific call
    else if (dbus_message_is_method_call(msg,MPRIS2_ROOT,"Notify")) {
      p->display_track_notification();
      return gm_dbus_reply_if_needed(c,msg);
      }

    // Undocumented gogglesmm specific call
    else if (dbus_message_is_method_call(msg,MPRIS2_ROOT,"ToggleShown")) {
      p->cmd_toggle_shown();
      return gm_dbus_reply_if_needed(c,msg);
      }

    return DBUS_HANDLER_RESULT_HANDLED;
    }
  else if (dbus_message_has_interface(msg,MPRIS2_PLAYER)) {
    if (dbus_message_is_method_call(msg,MPRIS2_PLAYER,"Next")) {
      p->cmd_next();
      return gm_dbus_reply_if_needed(c,msg);
      }
    else if (dbus_message_is_method_call(msg,MPRIS2_PLAYER,"Previous")) {
      p->cmd_prev();
      return gm_dbus_reply_if_needed(c,msg);
      }
    else if (dbus_message_is_method_call(msg,MPRIS2_PLAYER,"Pause")) {
      p->cmd_pause();
      return gm_dbus_reply_if_needed(c,msg);
      }
    else if (dbus_message_is_method_call(msg,MPRIS2_PLAYER,"PlayPause")) {
      p->cmd_playpause();
      return gm_dbus_reply_if_needed(c,msg);
      }
    else if (dbus_message_is_method_call(msg,MPRIS2_PLAYER,"Stop")) {
      p->cmd_stop();
      return gm_dbus_reply_if_needed(c,msg);
      }
    else if (dbus_message_is_method_call(msg,MPRIS2_PLAYER,"Play")) {
      p->cmd_play();
      return gm_dbus_reply_if_needed(c,msg);
      }
    else if (dbus_message_is_method_call(msg,MPRIS2_PLAYER,"Seek")) {
      FXlong offset=0;
      if (dbus_message_get_args(msg,nullptr,DBUS_TYPE_INT64,&offset,DBUS_TYPE_INVALID)) {
        p->seekTime(p->getPlayer()->getPosition()+(FXuint)(offset/1000000));
        }
      return gm_dbus_reply_if_needed(c,msg);
      }
    else if (dbus_message_is_method_call(msg,MPRIS2_PLAYER,"SetPosition")) {
      const FXchar * trackid=nullptr;
      FXlong position=0;
      if (dbus_message_get_args(msg,nullptr,DBUS_TYPE_OBJECT_PATH,&trackid,DBUS_TYPE_INT64,&position,DBUS_TYPE_INVALID)) {
        FXASSERT(0);
        }
      p->seekTime((FXuint)position/1000000);
      return gm_dbus_reply_if_needed(c,msg);
      }
    else if (dbus_message_is_method_call(msg,MPRIS2_PLAYER,"OpenUri")) {
      const FXchar * uri=nullptr;
      if (dbus_message_get_args(msg,nullptr,DBUS_TYPE_STRING,&uri,DBUS_TYPE_INVALID)) {
        p->open(uri);
        }
      return gm_dbus_reply_if_needed(c,msg);
      }
    return DBUS_HANDLER_RESULT_HANDLED;
    }
  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }


void GMMediaPlayerService2::request(const FXchar * command) {
  const FXchar * interface = nullptr;
  const FXchar * method = nullptr;
  FXString       argument;

  if (compare(command,"--previous")==0) {
    interface = MPRIS2_PLAYER;
    method = "Previous";
    }
  else if (compare(command,"--play")==0) {
    interface = MPRIS2_PLAYER;
    method = "Play";
    }
  else if (compare(command,"--play-pause")==0) {
    interface = MPRIS2_PLAYER;
    method = "PlayPause";
    }
  else if (compare(command,"--pause")==0) {
    interface = MPRIS2_PLAYER;
    method = "Pause";
    }
  else if (compare(command,"--next")==0) {
    interface = MPRIS2_PLAYER;
    method = "Next";
    }
  else if (compare(command,"--stop")==0) {
    interface = MPRIS2_PLAYER;
    method = "Stop";
    }
  else if (compare(command,"--toggle-shown")==0) {
    interface = MPRIS2_ROOT;
    method = "ToggleShown";
    }
  else if (compare(command,"--now-playing")==0) {
    interface = MPRIS2_ROOT;
    method = "Notify";
    }
  else if (compare(command,"--raise")==0) {
    interface = MPRIS2_ROOT;
    method = "Raise";
    }
  else {
    interface = MPRIS2_PLAYER;
    method = "OpenUri";
    argument = command;
    if (gm_is_local_file(argument)) {
      if (!FXPath::isAbsolute(argument))
        argument=FXPath::absolute(argument);
      }
    }

  if (interface && method) {
    DBusMessage * msg;
    msg = dbus_message_new_method_call(MPRIS2_NAME,MPRIS2_PATH,interface,method);
    if (msg){
      if (!argument.empty()){
        const FXchar * arg=argument.text();
        dbus_message_append_args(msg,DBUS_TYPE_STRING,&arg,DBUS_TYPE_INVALID);
        }
      dbus_message_set_no_reply(msg,true);
      bus->send(msg);
      bus->flush();
      }
    }
  }
