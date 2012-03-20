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
