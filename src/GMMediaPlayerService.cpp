#include "gmdefs.h"
#include "GMTrack.h"
#include "GMDBus.h"
#include "GMMediaPlayerService.h"
#include "GMSource.h"
#include "GMWindow.h"


#ifdef HAVE_XINE_LIB
#include "GMPlayer.h"
#endif
#include "GMPlayerManager.h"


static void gm_mpris2_track_to_dict(DBusMessageIter * iter,const GMTrack & track) {
  DBusMessageIter array;
  dbus_message_iter_open_container(iter,DBUS_TYPE_ARRAY,"{sv}",&array);
  gm_dbus_dict_append_string(&array,"xesam:title",track.title);
  gm_dbus_dict_append_string_list(&array,"xesam:artist",FXStringList(track.artist,1));
  if (!track.album_artist.empty())
    gm_dbus_dict_append_string_list(&array,"xesam:albumArtist",FXStringList(track.album_artist,1));
  gm_dbus_dict_append_string(&array,"xesam:album",track.album);
  gm_dbus_dict_append_string_list(&array,"xesam:composer",FXStringList(track.composer,1));

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


#if 0

#include "mpris_xml.h"


static void gm_mpris_track_to_dict(DBusMessageIter * iter,const GMTrack & track) {
  DBusMessageIter array;
  dbus_message_iter_open_container(iter,DBUS_TYPE_ARRAY,"{sv}",&array);
  gm_dbus_dict_append_string(&array,"title",track.title);
  gm_dbus_dict_append_string(&array,"artist",track.artist);
  gm_dbus_dict_append_string(&array,"album",track.album);
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
  dbus_message_iter_open_container(iter,DBUS_TYPE_STRUCT,NULL,&str);
  dbus_message_iter_append_basic(&str,DBUS_TYPE_INT32,&playstatus);
  dbus_message_iter_append_basic(&str,DBUS_TYPE_INT32,&playmode);
  dbus_message_iter_append_basic(&str,DBUS_TYPE_INT32,&playnext);
  dbus_message_iter_append_basic(&str,DBUS_TYPE_INT32,&playrepeat);
  dbus_message_iter_close_container(iter,&str);
  }

FXIMPLEMENT(GMMediaPlayerService1,FXObject,NULL,0)

GMMediaPlayerService1::GMMediaPlayerService1(GMDBus * b) : bus(b),published(false){

  memset(&root_vtable,0,sizeof(DBusObjectPathVTable));
  root_vtable.message_function=&root_filter;
  memset(&player_vtable,0,sizeof(DBusObjectPathVTable));
  player_vtable.message_function=&player_filter;
  memset(&tracklist_vtable,0,sizeof(DBusObjectPathVTable));
  tracklist_vtable.message_function=&tracklist_filter;

  int result = dbus_bus_request_name(bus->connection(),MPRIS_DBUS_NAME,DBUS_NAME_FLAG_DO_NOT_QUEUE,NULL);
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
    dbus_bus_release_name(bus->connection(),MPRIS_DBUS_NAME,NULL);
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
  DBusMessage * reply=NULL;
  FXuint serial;
  GMPlayerManager      * p     = GMPlayerManager::instance();
  if (dbus_message_is_method_call(msg,"org.freedesktop.DBus.Introspectable","Introspect")){
    FXString xml(mpris_xml);
    char ** children=NULL;
    if (dbus_connection_list_registered(connection,"/",&children)) {
      for (FXint i=0;children[i]!=NULL;i++) {
        xml+=GMStringFormat("\t<node name=\"%s\"/>\n",children[i]);
        }
      dbus_free_string_array(children);
      }
    xml+="</node>";
    fxmessage("%s\n",xml.text());
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
      dbus_message_iter_open_container(&iter,DBUS_TYPE_STRUCT,NULL,&str);
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
    if (p->getMainWindow()) p->getMainWindow()->handle(p,FXSEL(SEL_COMMAND,GMWindow::ID_QUIT),NULL);
    return DBUS_HANDLER_RESULT_HANDLED;
    }
  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }


DBusHandlerResult  GMMediaPlayerService1::player_filter(DBusConnection *connection,DBusMessage * msg,void * /*ptr*/){
  DEBUG_DBUS_MESSAGE(msg);
  DBusMessage * reply=NULL;
  FXuint serial;
  GMPlayerManager      * p     = GMPlayerManager::instance();

  if (dbus_message_is_method_call(msg,"org.freedesktop.DBus.Introspectable","Introspect")){
    return gm_dbus_reply_string(connection,msg,mpris_player_xml);
    }
  else if (dbus_message_is_method_call(msg,MPRIS_DBUS_INTERFACE,"GetCaps")) {
    FXint caps = mpris_get_caps(p);
    return gm_dbus_reply_int(connection,msg,caps);
    }
#ifdef HAVE_XINE_LIB
  else if (dbus_message_is_method_call(msg,MPRIS_DBUS_INTERFACE,"PositionGet")) {
    return gm_dbus_reply_int(connection,msg,p->getPlayer()->getPositionMS());
    }
#else
  else if (dbus_message_is_method_call(msg,MPRIS_DBUS_INTERFACE,"PositionGet")) {
    return gm_dbus_reply_int(connection,msg,0);
    }
#endif
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
  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }




DBusHandlerResult  GMMediaPlayerService1::tracklist_filter(DBusConnection *connection,DBusMessage * msg,void * /*ptr*/){
  DEBUG_DBUS_MESSAGE(msg);
  DBusMessage * reply=NULL;
  FXuint serial;

  GMPlayerManager      * p     = GMPlayerManager::instance();
//  GMMediaPlayerService1 * mpris = reinterpret_cast<GMMediaPlayerService1*>(ptr);

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

#endif

#include "mpris2_xml.h"


static const FXchar MPRIS2_NAME[]="org.mpris.MediaPlayer2.gogglesmm";
static const FXchar MPRIS2_PATH[]="/org/mpris/MediaPlayer2";
static const FXchar MPRIS2_ROOT[]="org.mpris.MediaPlayer2";
static const FXchar MPRIS2_PLAYER[]="org.mpris.MediaPlayer2.Player";
static const FXchar DBUS_INTROSPECTABLE[]="org.freedesktop.DBus.Introspectable";
static const FXchar DBUS_PROPERTIES[]="org.freedesktop.DBus.Properties";

FXIMPLEMENT(GMMediaPlayerService,FXObject,NULL,0)

GMMediaPlayerService::GMMediaPlayerService(GMDBus * b) : bus(b),published(false){
  memset(&mpris_vtable,0,sizeof(DBusObjectPathVTable));
  mpris_vtable.message_function=&mpris_filter;
  int result = dbus_bus_request_name(bus->connection(),MPRIS2_NAME,DBUS_NAME_FLAG_DO_NOT_QUEUE,NULL);
  if (result == DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER ) {
    dbus_connection_register_object_path(bus->connection(),MPRIS2_PATH,&mpris_vtable,this);
    published=true;
    }
  }

GMMediaPlayerService::~GMMediaPlayerService(){
  if (published) {
    dbus_connection_unregister_object_path(bus->connection(),MPRIS2_PATH);
    published=false;
    dbus_bus_release_name(bus->connection(),MPRIS2_NAME,NULL);
    }
  }

void GMMediaPlayerService::notify_track_change(const GMTrack &){
  /// TODO
  }

void GMMediaPlayerService::notify_status_change(){
  /// TODO
  }

void GMMediaPlayerService::notify_caps_change(){
  /// TODO
  }

static DBusHandlerResult mpris_root_property_get(DBusConnection *connection,DBusMessage * msg,const FXchar * prop){
  static const FXchar * schemes[]={"file","http",NULL};
  static const FXchar * mimetypes[]={"audio/flac","audio/ogg","audio/mpeg","audio/mp4a-latm","audio/x-musepack",NULL};
  static const FXchar prop_identity[]="Goggles Music Manager";
  static const FXchar prop_desktopentry[]="gogglesmm";
  if (prop==NULL) {
    FXuint serial;
    DBusMessage *   reply;
    DBusMessageIter iter;
    DBusMessageIter dict;
    if ((reply=dbus_message_new_method_return(msg))!=NULL) {
      dbus_message_iter_init_append(reply,&iter);
      dbus_message_iter_open_container(&iter,DBUS_TYPE_ARRAY,"{sv}",&dict);
      gm_dbus_dict_append_string(&dict,"Identity",prop_identity);
      gm_dbus_dict_append_string(&dict,"DesktopEntry",prop_desktopentry);
      gm_dbus_dict_append_bool(&dict,"CanQuit",true);
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
  else if (compare(prop,"Identity")==0)             return gm_dbus_reply_string(connection,msg,prop_identity);
  else if (compare(prop,"DesktopEntry")==0)         return gm_dbus_reply_string(connection,msg,prop_desktopentry);
  else if (compare(prop,"CanQuit")==0)              return gm_dbus_reply_bool(connection,msg,true);
  else if (compare(prop,"CanRaise")==0)             return gm_dbus_reply_bool(connection,msg,true);
  else if (compare(prop,"HasTrackList")==0)         return gm_dbus_reply_bool(connection,msg,false);
  else if (compare(prop,"SupportedUriSchemes")==0)  return gm_dbus_reply_string_list(connection,msg,schemes);
  else if (compare(prop,"SupportedMimeTypes")==0)   return gm_dbus_reply_string_list(connection,msg,mimetypes);
  else return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }



static const FXchar * mpris_play_status(GMPlayerManager * p){
  if (p->can_unpause())
    return "Paused";
  else if (p->can_pause())
    return "Playing";
  else
    return "Stopped";
  }

static DBusHandlerResult mpris_player_property_set(DBusMessageIter*,const FXchar * prop) {
  if (compare(prop,"LoopStatus")==0) {
    }
  else if (compare(prop,"Rate")==0){
    }
  else if (compare(prop,"Shuffle")==0){
    }
  else if (compare(prop,"Volume")==0){
    }
  return DBUS_HANDLER_RESULT_HANDLED;
  }

static DBusHandlerResult mpris_player_property_get(DBusConnection *c,DBusMessage * msg,const FXchar * prop){
  FXuint serial;
  GMPlayerManager * p = GMPlayerManager::instance();
  if (prop==NULL) {
    DBusMessage *   reply;
    DBusMessageIter iter;
    DBusMessageIter dict;
    if ((reply=dbus_message_new_method_return(msg))!=NULL) {
      GMTrack track;
      p->getTrackInformation(track);
      dbus_message_iter_init_append(reply,&iter);
      dbus_message_iter_open_container(&iter,DBUS_TYPE_ARRAY,"{sv}",&dict);
      gm_dbus_dict_append_string(&dict,"PlaybackStatus",mpris_play_status(p));
      gm_dbus_dict_append_string(&dict,"LoopStatus","None"); /// None, Track, Playlist
      gm_dbus_dict_append_bool(&dict,"Shuffle",false);
      gm_dbus_dict_append_track(&dict,"Metadata",track);
      gm_dbus_dict_append_double(&dict,"Volume",0.0);
      gm_dbus_dict_append_long(&dict,"Position",0);
      gm_dbus_dict_append_double(&dict,"Rate",1.0);
      gm_dbus_dict_append_double(&dict,"MinimumRate",1.0);
      gm_dbus_dict_append_double(&dict,"MaximumRate",1.0);
      gm_dbus_dict_append_bool(&dict,"CanGoNext",p->can_next());
      gm_dbus_dict_append_bool(&dict,"CanGoPrevious",p->can_prev());
      gm_dbus_dict_append_bool(&dict,"CanPlay",p->can_play());
      gm_dbus_dict_append_bool(&dict,"CanPause",p->can_pause());
      gm_dbus_dict_append_bool(&dict,"CanSeek",false);
      gm_dbus_dict_append_bool(&dict,"CanControl",true);
      dbus_message_iter_close_container(&iter,&dict);
      dbus_connection_send(c,reply,&serial);
      dbus_message_unref(reply);
      }
    }
  else if (compare(prop,"Metadata")==0) {
    DBusMessage * reply;
    if ((reply=dbus_message_new_method_return(msg))!=NULL) {
      DBusMessageIter iter;
      GMTrack track;
      p->getTrackInformation(track);
      dbus_message_iter_init_append(reply,&iter);
      gm_mpris2_track_to_dict(&iter,track);
      dbus_connection_send(c,reply,&serial);
      dbus_message_unref(reply);
      }
    return DBUS_HANDLER_RESULT_HANDLED;
    }
  else if (compare(prop,"PlaybackStatus")==0) return gm_dbus_reply_string(c,msg,mpris_play_status(p));
  else if (compare(prop,"LoopStatus")==0)     return gm_dbus_reply_string(c,msg,"None");
  else if (compare(prop,"Shuffle")==0)        return gm_dbus_reply_bool(c,msg,false);
  else if (compare(prop,"Volume")==0)         return gm_dbus_reply_double(c,msg,0.0);
  else if (compare(prop,"Position")==0)       return gm_dbus_reply_long(c,msg,0);
  else if (compare(prop,"Rate")==0)           return gm_dbus_reply_double(c,msg,1.0);
  else if (compare(prop,"MinimumRate")==0)    return gm_dbus_reply_double(c,msg,1.0);
  else if (compare(prop,"MaximumRate")==0)    return gm_dbus_reply_double(c,msg,1.0);
  else if (compare(prop,"CanGoNext")==0)      return gm_dbus_reply_bool(c,msg,p->can_next());
  else if (compare(prop,"CanGoPrevious")==0)  return gm_dbus_reply_bool(c,msg,p->can_prev());
  else if (compare(prop,"CanPlay")==0)        return gm_dbus_reply_bool(c,msg,p->can_play());
  else if (compare(prop,"CanPause")==0)       return gm_dbus_reply_bool(c,msg,p->can_pause());
  else if (compare(prop,"CanControl")==0)     return gm_dbus_reply_bool(c,msg,true);
  return DBUS_HANDLER_RESULT_HANDLED;
  }









DBusHandlerResult GMMediaPlayerService::mpris_filter(DBusConnection * c,DBusMessage * msg,void*){
  GMPlayerManager * p = GMPlayerManager::instance();
  if (dbus_message_has_interface(msg,DBUS_INTROSPECTABLE)) {
    if (dbus_message_is_method_call(msg,DBUS_INTROSPECTABLE,"Introspect")){
      return gm_dbus_reply_string(c,msg,mpris2_xml);
      }
    return DBUS_HANDLER_RESULT_HANDLED;
    }
  else if (dbus_message_has_interface(msg,DBUS_PROPERTIES)) {
    if (dbus_message_is_method_call(msg,DBUS_PROPERTIES,"Get")){
      const FXchar * interface=NULL;
      const FXchar * property=NULL;
      if (dbus_message_get_args(msg,NULL,DBUS_TYPE_STRING,&interface,DBUS_TYPE_STRING,&property,DBUS_TYPE_INVALID)) {
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
      const FXchar * interface=NULL;
      if (dbus_message_get_args(msg,NULL,DBUS_TYPE_STRING,&interface,DBUS_TYPE_INVALID)) {
        if (compare(interface,MPRIS2_ROOT)==0) {
          return mpris_root_property_get(c,msg,NULL);
          }
        else if (compare(interface,MPRIS2_PLAYER)==0) {
          return mpris_player_property_get(c,msg,NULL);
          }
        }
      return DBUS_HANDLER_RESULT_HANDLED;
      }
    else if (dbus_message_is_method_call(msg,DBUS_PROPERTIES,"Set")){
      const FXchar * interface=NULL;
      const FXchar * property=NULL;
      DBusMessageIter iter;
      dbus_message_iter_init(msg,&iter);
      if (dbus_message_iter_get_arg_type(&iter)==DBUS_TYPE_STRING) {
        dbus_message_iter_get_basic	(&iter,&interface);
        if (dbus_message_iter_next(&iter) && dbus_message_iter_get_arg_type(&iter)==DBUS_TYPE_STRING) {
          dbus_message_iter_get_basic	(&iter,&property);
          if (compare(interface,MPRIS2_PLAYER)==0) {
            return mpris_player_property_set(&iter,property);
            }
          }
        }
      return DBUS_HANDLER_RESULT_HANDLED;
      }
    }
  else if (dbus_message_has_interface(msg,MPRIS2_ROOT)) {
    if (dbus_message_is_method_call(msg,MPRIS2_ROOT,"Raise")) {
      FXASSERT(0);
      return gm_dbus_reply_if_needed(c,msg);
      }
    else if (dbus_message_is_method_call(msg,MPRIS2_ROOT,"Quit")) {
      FXASSERT(0);
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
      if (dbus_message_get_args(msg,NULL,DBUS_TYPE_INT64,&offset,DBUS_TYPE_INVALID)) {
        FXASSERT(0);
        }
      return gm_dbus_reply_if_needed(c,msg);
      }
    else if (dbus_message_is_method_call(msg,MPRIS2_PLAYER,"SetPosition")) {
      const FXchar * trackid=NULL;
      FXlong position=0;
      if (dbus_message_get_args(msg,NULL,DBUS_TYPE_OBJECT_PATH,&trackid,DBUS_TYPE_INT64,&position,DBUS_TYPE_INVALID)) {
        FXASSERT(0);
        }
      return gm_dbus_reply_if_needed(c,msg);
      }
    else if (dbus_message_is_method_call(msg,MPRIS2_PLAYER,"OpenUri")) {
      const FXchar * uri=NULL;
      if (dbus_message_get_args(msg,NULL,DBUS_TYPE_STRING,&uri,DBUS_TYPE_INVALID)) {
        FXASSERT(0);
        }
      return gm_dbus_reply_if_needed(c,msg);
      }
    return DBUS_HANDLER_RESULT_HANDLED;
    }
  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }
