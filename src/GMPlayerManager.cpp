/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2006-2010 by Sander Jansen. All Rights Reserved      *
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
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>

#include <xincs.h>
#include <X11/XF86keysym.h>


#include "gmdefs.h"
#include "gmutils.h"

#include "GMTrack.h"
#include "GMTrackDatabase.h"

#ifdef HAVE_DBUS
#include "GMDBus.h"
#include "GMNotifyDaemon.h"
#include "GMSettingsDaemon.h"
#include "GMMediaPlayerService.h"
#include "GMAppStatusNotify.h"
#endif

#ifdef HAVE_LIRC
#include "lirc_client.h"
#endif

#include <FXPNGIcon.h>
#include "GMApp.h"
#include "GMPlayer.h"
#include "GMWindow.h"
#include "GMTrackList.h"
#include "GMRemote.h"
#include "GMTag.h"
#include "GMFilename.h"

#include "GMTaskManager.h"
#include "GMList.h"
#include "GMTrackView.h"
#include "GMSourceView.h"
#include "GMSource.h"
#include "GMDatabaseSource.h"
#include "GMStreamSource.h"
#include "GMPlayListSource.h"
#include "GMPlayQueue.h"
#include "GMLocalSource.h"

#include "GMIconTheme.h"
#include "GMPlayerManager.h"
#include "GMFetch.h"
#include "GMEQDialog.h"
#include "GMTrayIcon.h"

#include "GMCoverThumbs.h"

#ifndef HAVE_XINE_LIB
#include "GMAudioPlayer.h"
#endif

#include "GMAudioScrobbler.h"


#if APPLICATION_BETA_DB > 0
#define DATABASE_FILENAME "goggles_beta.db"
#else
#define DATABASE_FILENAME "goggles.db"
#endif


enum {
  FIFO_STATUS_ERROR  = 0,
  FIFO_STATUS_OWNER  = 1,
  FIFO_STATUS_EXISTS = 2
  };

FXDEFMAP(GMPlayerManager) GMPlayerManagerMap[]={
  FXMAPFUNC(SEL_TIMEOUT,GMPlayerManager::ID_UPDATE_TRACK_DISPLAY,GMPlayerManager::onUpdTrackDisplay),
#ifdef HAVE_XINE_LIB
  FXMAPFUNC(SEL_TIMEOUT,GMPlayerManager::ID_HANDLE_EVENTS,GMPlayerManager::onUpdEvents),
#endif
  FXMAPFUNC(SEL_TIMEOUT,GMPlayerManager::ID_SLEEP_TIMER,GMPlayerManager::onCmdSleepTimer),
  FXMAPFUNC(SEL_TIMEOUT,GMPlayerManager::ID_PLAY_NOTIFY,GMPlayerManager::onPlayNotify),
  FXMAPFUNC(SEL_IO_READ,GMPlayerManager::ID_DDE_MESSAGE,GMPlayerManager::onDDEMessage),
  FXMAPFUNC(SEL_CHORE,GMPlayerManager::ID_PLAYER_ERROR,GMPlayerManager::onPlayerError),
  FXMAPFUNC(SEL_CLOSE,GMPlayerManager::ID_WINDOW,GMPlayerManager::onCmdCloseWindow),
  FXMAPFUNC(SEL_SIGNAL,GMPlayerManager::ID_CHILD,GMPlayerManager::onCmdChild),

  FXMAPFUNC(SEL_COMMAND,GMPlayerManager::ID_SCROBBLER,GMPlayerManager::onScrobblerError),
  FXMAPFUNC(SEL_OPENED,GMPlayerManager::ID_SCROBBLER,GMPlayerManager::onScrobblerOpen),

  FXMAPFUNC(SEL_COMMAND,GMPlayerManager::ID_EQUALIZER,GMPlayerManager::onCmdEqualizer),

#ifdef HAVE_DBUS
  FXMAPFUNC(SEL_KEYPRESS,GMPlayerManager::ID_GNOME_SETTINGS_DAEMON,GMPlayerManager::onCmdSettingsDaemon),
#endif
#ifdef HAVE_LIRC
  FXMAPFUNC(SEL_IO_READ,GMPlayerManager::ID_LIRC,GMPlayerManager::onCmdLirc),
#endif
//#ifndef HAVE_XINE_LIB
  FXMAPFUNC(SEL_PLAYER_BOS,GMPlayerManager::ID_AUDIO_PLAYER,GMPlayerManager::onPlayerBOS),
  FXMAPFUNC(SEL_PLAYER_EOS,GMPlayerManager::ID_AUDIO_PLAYER,GMPlayerManager::onPlayerEOS),
  FXMAPFUNC(SEL_PLAYER_TIME,GMPlayerManager::ID_AUDIO_PLAYER,GMPlayerManager::onPlayerTime),
  FXMAPFUNC(SEL_PLAYER_STATE,GMPlayerManager::ID_AUDIO_PLAYER,GMPlayerManager::onPlayerState),
//#endif
  FXMAPFUNC(SEL_COMMAND,GMPlayerManager::ID_DOWNLOAD_COMPLETE,GMPlayerManager::onCmdDownloadComplete),
  FXMAPFUNC(SEL_COMMAND,GMPlayerManager::ID_CANCEL_TASK,GMPlayerManager::onCancelTask),

  FXMAPFUNC(SEL_TASK_STATUS,GMPlayerManager::ID_TASKMANAGER,GMPlayerManager::onTaskManagerStatus),
  FXMAPFUNC(SEL_TASK_IDLE,GMPlayerManager::ID_TASKMANAGER,GMPlayerManager::onTaskManagerIdle),
  FXMAPFUNC(SEL_TASK_RUNNING,GMPlayerManager::ID_TASKMANAGER,GMPlayerManager::onTaskManagerRunning),

  FXMAPFUNC(SEL_TIMEOUT,GMPlayerManager::ID_TASKMANAGER_SHUTDOWN,GMPlayerManager::onTaskManagerShutdown),

  FXMAPFUNC(SEL_TASK_COMPLETED,GMPlayerManager::ID_IMPORT_TASK,GMPlayerManager::onImportTaskCompleted)

//  FXMAPFUNC(SEL_COMMAND,GMPlayerManager::ID_COVERS_LOADED,GMPlayerManager::onCoversLoaded)
  };

FXIMPLEMENT(GMPlayerManager,FXObject,GMPlayerManagerMap,ARRAYNUMBER(GMPlayerManagerMap))

#ifdef HAVE_DBUS

#include "gogglesmm_xml.h"


#ifdef HAVE_DBUS
#define GOGGLESMM_DBUS_NAME "org.fifthplanet.gogglesmm"
#define GOGGLESMM_DBUS_PATH "/org/fifthplanet/gogglesmm"
#define GOGGLESMM_DBUS_INTERFACE "org.fifthplanet.gogglesmm"
#endif


DBusHandlerResult dbus_systembus_filter(DBusConnection *,DBusMessage * msg,void * data){
  FXTRACE((80,"-----dbus_systembus_filter-------\n"));
  FXTRACE((80,"path: %s\n",dbus_message_get_path(msg)));
  FXTRACE((80,"member: \"%s\"\n",dbus_message_get_member(msg)));
  FXTRACE((80,"interface: %s\n",dbus_message_get_interface(msg)));
  FXTRACE((80,"sender: %s\n",dbus_message_get_sender(msg)));

  GMPlayerManager * p = (GMPlayerManager*)data;
  if (dbus_message_has_path(msg,"/org/freedesktop/NetworkManager")){
    if (dbus_message_is_signal(msg,"org.freedesktop.NetworkManager","StateChanged") ||
        dbus_message_is_signal(msg,"org.freedesktop.NetworkManager","StateChange")) {

      FXuint state=0;
      enum {
        NETWORK_STATE_UNKNOWN      = 0,
        NETWORK_STATE_ASLEEP       = 1,
        NETWORK_STATE_CONNECTING   = 2,
        NETWORK_STATE_CONNECTED    = 3,
        NETWORK_STATE_DISCONNECTED = 4
        };

      if (dbus_message_get_args(msg,NULL,DBUS_TYPE_UINT32,&state,DBUS_TYPE_INVALID)) {
        if (p->getAudioScrobbler() && state==NETWORK_STATE_CONNECTED)
          p->getAudioScrobbler()->nudge();
        }
      return DBUS_HANDLER_RESULT_HANDLED;
      }
    }
  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }



DBusHandlerResult dbus_playermanager_filter(DBusConnection *connection,DBusMessage * msg,void * data){
  FXchar * mrl;
  GMPlayerManager * p = (GMPlayerManager*)data;
  if (dbus_message_has_path(msg,GOGGLESMM_DBUS_PATH)){
    if (dbus_message_is_method_call(msg,GOGGLESMM_DBUS_INTERFACE,"play")){
      p->cmd_play();
      return gm_dbus_reply_if_needed(connection,msg);
      }
    else if (dbus_message_is_method_call(msg,GOGGLESMM_DBUS_INTERFACE,"playpause")){
      p->cmd_playpause();
      return gm_dbus_reply_if_needed(connection,msg);
      }
    else if (dbus_message_is_method_call(msg,GOGGLESMM_DBUS_INTERFACE,"stop")){
      p->cmd_stop();
      return gm_dbus_reply_if_needed(connection,msg);
      }
    else if (dbus_message_is_method_call(msg,GOGGLESMM_DBUS_INTERFACE,"pause")){
      p->cmd_pause();
      return gm_dbus_reply_if_needed(connection,msg);
      }
    else if (dbus_message_is_method_call(msg,GOGGLESMM_DBUS_INTERFACE,"next")){
      p->cmd_next();
      return gm_dbus_reply_if_needed(connection,msg);
      }
    else if (dbus_message_is_method_call(msg,GOGGLESMM_DBUS_INTERFACE,"prev")){
      p->cmd_prev();
      return gm_dbus_reply_if_needed(connection,msg);
      }
    else if (dbus_message_is_method_call(msg,GOGGLESMM_DBUS_INTERFACE,"open")){
      if (dbus_message_get_args(msg,NULL,DBUS_TYPE_STRING,&mrl,DBUS_TYPE_INVALID)) {
        p->open(mrl);
        }
      return gm_dbus_reply_if_needed(connection,msg);
      }
    else if (dbus_message_is_method_call(msg,GOGGLESMM_DBUS_INTERFACE,"raise")){
      if (p->getMainWindow()->shown())
        p->getMainWindow()->raise();
      return gm_dbus_reply_if_needed(connection,msg);
      }
    else if (dbus_message_is_method_call(msg,GOGGLESMM_DBUS_INTERFACE,"notify")){
      p->display_track_notification();
      return gm_dbus_reply_if_needed(connection,msg);
      }
    else if (dbus_message_is_method_call(msg,GOGGLESMM_DBUS_INTERFACE,"toggleshown")){
      p->cmd_toggle_shown();
      return gm_dbus_reply_if_needed(connection,msg);
      }
    else if (dbus_message_is_method_call(msg,GOGGLESMM_DBUS_INTERFACE,"getactions")){
      FXuint actions=0;

      enum {
        CAN_PLAY  		= 0x1,
        CAN_PAUSE 		= 0x2,
        CAN_STOP			= 0x4,
        CAN_NEXT  		= 0x8,
        CAN_PREV			= 0x10,
        };

      if (p->can_play() || p->can_unpause()) actions|=CAN_PLAY;
      if (p->can_stop()) actions|=CAN_STOP;
      if (p->can_pause()) actions|=CAN_PAUSE;
      if (p->can_prev()) actions|=CAN_PREV;
      if (p->can_next()) actions|=CAN_NEXT;
      return gm_dbus_reply_unsigned_int(connection,msg,actions);
      }
    else if (dbus_message_is_method_call(msg,GOGGLESMM_DBUS_INTERFACE,"exit")){
      gm_dbus_reply_if_needed(connection,msg);
      if (p->getMainWindow()) p->getMainWindow()->handle(p,FXSEL(SEL_COMMAND,GMWindow::ID_QUIT),NULL);
      return DBUS_HANDLER_RESULT_HANDLED;
      }
    else if (dbus_message_is_method_call(msg,"org.freedesktop.DBus.Introspectable","Introspect")){
      return gm_dbus_reply_string(connection,msg,gogglesmm_xml);
      }
    }
  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }


static DBusObjectPathVTable org_fifthplanet_gogglesmm={
  NULL,
  &dbus_playermanager_filter,
  NULL,
  NULL,
  NULL,
  NULL
  };




static void dbus_send_to_self(DBusConnection * connection,const FXchar * signal,FXString argument) {
  DBusMessage * msg;
//	FXuint serial=-1;
  msg = dbus_message_new_method_call(GOGGLESMM_DBUS_NAME,GOGGLESMM_DBUS_PATH,GOGGLESMM_DBUS_INTERFACE,signal);
  if (msg){
    if (!argument.empty()) {
      const FXchar * arg=argument.text();
      dbus_message_append_args(msg,DBUS_TYPE_STRING,&arg,DBUS_TYPE_INVALID);
      }
    dbus_message_set_no_reply(msg,true);
    dbus_connection_send(connection,msg,NULL);
    dbus_connection_flush(connection);
//    if (reply) dbus_message_unref(reply);
    dbus_message_unref(msg);
    }
  }


static FXint dbus_send_commands(DBusConnection * connection,int& argc,char** argv){
  FXString cmd="raise";
  FXString url;
  if (argc>1) {
    if (compare(argv[1],"--previous")==0)
      cmd="prev";
    else if (compare(argv[1],"--play")==0)
      cmd="play";
    else if (compare(argv[1],"--play-pause")==0)
      cmd="playpause";
    else if (compare(argv[1],"--pause")==0)
      cmd="pause";
    else if (compare(argv[1],"--next")==0)
      cmd="next";
    else if (compare(argv[1],"--stop")==0)
      cmd="stop";
    else if (compare(argv[1],"--toggle-shown")==0)
      cmd="toggleshown";
    else if (compare(argv[1],"--now-playing")==0)
      cmd="notify";
    else if (compare(argv[1],"--raise")==0)
      cmd="raise";
    else {
      cmd="open";
      url=argv[1];
      if (gm_is_local_file(url)) {
        if (!FXPath::isAbsolute(url)) {
          url=FXPath::absolute(url);
          }
        }
      }
    }
  dbus_send_to_self(connection,cmd.text(),url);
  return 1;
  }




#endif



long GMPlayerManager::onPlayNotify(FXObject*,FXSelector,void*){
  update_cover_display();

  if (!trackinfo.title.empty() && !trackinfo.artist.empty()) {

    display_track_notification();

    if (scrobbler) scrobbler->nowplaying(trackinfo);
    }
  return 0;
  }

long GMPlayerManager::onUpdTrackDisplay(FXObject*,FXSelector,void*){
  update_track_display();
  getTrackView()->showCurrent();
  return 0;
  }

#ifdef HAVE_XINE_LIB
long GMPlayerManager::onUpdEvents(FXObject*,FXSelector,void*){
  handle_async_events();
  application->addTimeout(this,GMPlayerManager::ID_HANDLE_EVENTS,TIME_MSEC(500));
  return 0;
  }
#endif

/* Stop Playback! */
long GMPlayerManager::onCmdSleepTimer(FXObject*,FXSelector,void*){
  if (playing()) {
    stop();
    }
  return 1;
  }


long GMPlayerManager::onDDEMessage(FXObject*,FXSelector,void*){
  FXString cmd;
  FXchar buffer[1024];
  int nread=fifo.readBlock(buffer,1024);
  if (nread>0) {
    cmd=FXString(buffer,nread);
    cmd.trim();
    if (cmd=="--previous") cmd_prev();
    else if (cmd=="--play") cmd_play();
    else if (cmd=="--play-pause") cmd_playpause();
    else if (cmd=="--pause") cmd_pause();
    else if (cmd=="--next") cmd_next();
    else if (cmd=="--stop") cmd_stop();
    else if (cmd=="--toggle-shown") cmd_toggle_shown();
    else if (cmd=="--now-playing") display_track_notification();
    else if (cmd=="--raise") {
      if (mainwindow->shown())
        mainwindow->raise();
      }
    else {
      if (gm_is_local_file(cmd)) {
        if (!FXPath::isAbsolute(cmd)) {
          cmd=FXPath::absolute(cmd);
          }
        }
      if (!cmd.empty())
        open(cmd);
      }
    }
  return 1;
  }



GMPlayerManager * GMPlayerManager::myself = NULL;

GMPlayerManager* GMPlayerManager::instance() {
  return myself;
  }



/// Constructor
GMPlayerManager::GMPlayerManager() :
  count_track_remaining(0),
  taskmanager(NULL),
#ifdef HAVE_DBUS
  sessionbus(NULL),
  systembus(NULL),
  notifydaemon(NULL),
  gsd(NULL),
  mpris(NULL),
#endif
  application(NULL),
  mainwindow(NULL),
  player(NULL),
  trayicon(NULL),
  scrobbler(NULL),
  queue(NULL),
  source(NULL),
  database(NULL),
  thumbs(NULL) {
  FXASSERT(myself==NULL);
  myself=this;
  }


/// Destructor
GMPlayerManager::~GMPlayerManager() {

  /// Remove Signal Handlers
#ifndef DEBUG
  application->removeSignal(SIGINT);
  application->removeSignal(SIGQUIT);
  application->removeSignal(SIGTERM);
  application->removeSignal(SIGHUP);
  application->removeSignal(SIGPIPE);
#endif

  application->removeSignal(SIGCHLD);

  /// Cleanup fifo crap
  if (fifo.isOpen())
    fifo.close();
  if (!fifofilename.empty())
    FXFile::remove(fifofilename);

  delete scrobbler;

  delete database;

#ifdef HAVE_DBUS
  delete sessionbus;
  delete systembus;
#endif

  delete player;
  delete taskmanager;

  /// Clean up global resources
  GMFetch::exit();

  myself=NULL;

  delete application;
  }




FXint GMPlayerManager::init_fifo(int& argc,char** argv){
  FXString fifodir = FXSystem::getHomeDirectory() + PATHSEPSTRING + ".goggles";
  FXStat info;

  if ( (!FXStat::exists(fifodir) && !FXDir::create(fifodir) ) || !FXStat::isDirectory(fifodir) ) {
    FXMessageBox::error(application,MBOX_OK,"Goggles Music Manager",fxtrformat("Unable to create directory %s\n"),fifodir.text());
    return FIFO_STATUS_ERROR;
    }

  fifofilename = fifodir + PATHSEPSTRING + "gmm.dde";

  /// Find existing fifo
  if (FXStat::statFile(fifofilename,info)) {

    /// File exists, but it's not a fifo... try removing it
    if (!info.isFifo() && !FXFile::remove(fifofilename)) {
      fifofilename=FXString::null;
      return FIFO_STATUS_ERROR;
      }

    if (fifo.open(fifofilename,FXIO::WriteOnly|FXIO::NonBlocking)){
      FXString commandline;

      for (FXint i=1;i<argc;i++){
        commandline+=argv[i];
        }

      /// Try raising the window
      if (commandline.empty())
        commandline="--raise";

      /// if write failed or command line was empty, user probably tries to start up normally
      /// and fifo may be stale
      if (fifo.writeBlock(commandline.text(),commandline.length())) {
        fifofilename=FXString::null;
        return FIFO_STATUS_EXISTS;
        }

      /// Most likely left behind by crashed music manager
      fifo.close();
      }

    if (!FXFile::remove(fifofilename))
      return FIFO_STATUS_OWNER;
    }

  /// Create the fifo
  if (mkfifo(fifofilename.text(),S_IWUSR|S_IRUSR)!=0){
      fifofilename=FXString::null;
      return FIFO_STATUS_OWNER;
      }

  /// Try open the fifo
  fifo.open(fifofilename,FXIO::Reading|FXIO::WriteOnly,FXIO::OwnerWrite);

  /// Close fifo on execute or remove fifo if we couldn't open it...
  if (fifo.isOpen()) {
    fcntl(fifo.handle(),F_SETFD,FD_CLOEXEC);
    }
  else {
    FXFile::remove(fifofilename);
    fifofilename=FXString::null;
    }

  return FIFO_STATUS_OWNER;
  }




FXbool GMPlayerManager::init_sources() {

  // Main Database
  database = new GMTrackDatabase;
  thumbs   = new GMCoverThumbs;

  // Make sure we can open it.
  if (!init_database(database)) {
    delete database;
    return false;
    }

  /// Create the main database source
  sources.append(new GMDatabaseSource(database));

  /// Create Play List Sources
  FXIntList playlists;
  if (database->listPlaylists(playlists)) {
    for (FXint i=0;i<playlists.no();i++) {
      sources.append(new GMPlayListSource(database,playlists[i]));
      }
    }

  /// Init Play Queue
  if (preferences.play_from_queue) {
    queue = new GMPlayQueue(database);
    sources.append(queue);
    }

#ifdef HAVE_XINE_LIB
  /// Internet Streams
  sources.append(new GMStreamSource(database));
#endif

  /// File System
  sources.append(new GMLocalSource());

  /// Load Settings
  for (FXint i=0;i<sources.no();i++) {
    sources[i]->load(application->reg());
    }

  return true;
  }


GMDatabaseSource * GMPlayerManager::getDatabaseSource() const {
  return dynamic_cast<GMDatabaseSource*>(sources[0]);
  }

void GMPlayerManager::setPlayQueue(FXbool enable) {
  preferences.play_from_queue=enable;
  if (enable) {
    if (!queue) {
      queue = new GMPlayQueue(database);
      sources.append(queue);
      GMPlayerManager::instance()->getSourceView()->refresh();
      }
    }
  else {
    if (queue) {
      removeSource(queue);
      queue=NULL;
      }
    }
  }



void GMPlayerManager::removeSource(GMSource * src) {

  sources.remove(src);

  GMPlayerManager::instance()->getSourceView()->refresh();

  if (application->hasTimeout(src,GMSource::ID_TRACK_PLAYED))
    application->removeTimeout(src,GMSource::ID_TRACK_PLAYED);

  if (src==source) {
    source->resetCurrent();
    source=NULL;
    }

  delete src;
  }



void GMPlayerManager::init_window(FXbool wizard) {
  const FXint           argc = application->getArgc();
  const FXchar *const * argv = application->getArgv();

  /// Create Main Window
  mainwindow = new GMWindow(application,this,ID_WINDOW);
  mainwindow->create();

  // Register Global Hotkeys
  register_global_hotkeys();

  /// Handle interrupt to save stuff nicely
#ifndef DEBUG
  application->addSignal(SIGINT,mainwindow,GMWindow::ID_QUIT);
  application->addSignal(SIGQUIT,mainwindow,GMWindow::ID_QUIT);
  application->addSignal(SIGTERM,mainwindow,GMWindow::ID_QUIT);
  application->addSignal(SIGHUP,mainwindow,GMWindow::ID_QUIT);
  application->addSignal(SIGPIPE,mainwindow,GMWindow::ID_QUIT);
#endif

  /// Create Tooltip Window
  FXToolTip * tooltip = new FXToolTip(application);
  tooltip->create();
  ewmh_change_window_type(tooltip,WINDOWTYPE_TOOLTIP);

  if (database->getNumTracks()==0 && database->getNumStreams()==0 && wizard) {
    cleanSourceSettings();
    mainwindow->init(SHOW_WIZARD);
    }
  else {
    FXbool start_as_tray=false;
    for(FXint i=1;i<argc;i++) {
      if (comparecase(argv[i],"--tray")==0){
        start_as_tray=true;
        preferences.gui_tray_icon=true;
        break;
        }
      }
    if (start_as_tray)
      mainwindow->init(SHOW_TRAY);
    else
      mainwindow->init(SHOW_NORMAL);
    }
#ifdef HAVE_XINE_LIB
  application->addTimeout(this,GMPlayerManager::ID_HANDLE_EVENTS,TIME_MSEC(500));
#endif
  }


static FXString get_cmdline_url(int& argc,char** argv) {
  FXString url;
  for (FXint i=1;i<argc;i++) {
    if (argv[i][0]!='-') {
      url=argv[i];
      }
    }
  return url;
  }

void GMPlayerManager::init_configuration() {
  FXString xdg_config_home = GMApp::getConfigDirectory(true);
  FXString xdg_data_home = GMApp::getDataDirectory(true);

  /// Check if we need to migrate old files to new directory.
  FXString newbase = xdg_data_home+PATHSEPSTRING;
  FXString oldbase = FXSystem::getHomeDirectory()+PATHSEPSTRING ".goggles" PATHSEPSTRING;

  if ( (FXStat::exists(newbase+DATABASE_FILENAME)==false) && FXStat::exists(oldbase+DATABASE_FILENAME) ) {


    /// Move database (for now disabled untill we have a upgrade path)
    /// FXFile::moveFiles(oldbase+DATABASE_FILENAME,newbase+DATABASE_FILENAME);
    /// FXFile::moveFiles(oldbase+"scrobbler.cache",newbase+"scrobbler.cache");

    newbase = xdg_config_home+PATHSEPSTRING;

    /// Let's move
    FXString oldconfig = FXSystem::getHomeDirectory()+PATHSEPSTRING ".foxrc" PATHSEPSTRING "musicmanager";

    /// Move old files over to new directory;
    FXFile::moveFiles(oldbase+"xineconf",newbase+"xineconf");

    /// Move Settings
    if (FXStat::exists(oldconfig))
      FXFile::moveFiles(oldconfig,newbase+"settings.rc");

    ///FIXME enable this when we release 0.12. Remove the old config directory.
    //FXFile::removeFiles(FXSystem::getHomeDirectory()+PATHSEPSTRING+".goggles",true);
    }
  }


#ifdef HAVE_LIRC

void GMPlayerManager::init_lirc() {

//#ifdef DEBUG
  lirc_fd  = lirc_init("gogglesmm",1);
//#else
//  lirc_fd  = lirc_init("gogglesmm",0);
//#endif
  if (lirc_fd) {
    ap_set_nonblocking(lirc_fd);

    if (lirc_readconfig(NULL,&lirc_config,NULL))
      fxmessage("oops\n");

#if FOXVERSION < FXVERSION(1,7,0)
    application->addInput(lirc_fd,INPUT_READ,this,ID_LIRC);
#else
    application->addInput(this,ID_LIRC,lirc_fd,INPUT_READ);
#endif
    }

  }
#endif

#ifdef HAVE_DBUS
FXbool GMPlayerManager::init_dbus(int & argc,char**argv) {

  sessionbus=new GMDBus();
  systembus =new GMDBus();

  FXASSERT(sessionbus);
  FXASSERT(systembus);

  if (!sessionbus->open(DBUS_BUS_SESSION) || !sessionbus->connected()) {
    FXMessageBox::warning(application,MBOX_OK,"Goggles Music Manager",fxtr("Session bus not available. All features requiring dbus are disabled."));
    delete sessionbus;
    sessionbus=NULL;
    }
  else {
    FXint result = dbus_bus_request_name(sessionbus->connection(),GOGGLESMM_DBUS_NAME,DBUS_NAME_FLAG_DO_NOT_QUEUE,NULL);
    switch(result) {
      case DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER:
        {
          if (!dbus_connection_register_object_path(sessionbus->connection(),"/org/fifthplanet/gogglesmm",&org_fifthplanet_gogglesmm,this)){
            FXMessageBox::warning(application,MBOX_OK,"Goggles Music Manager",fxtr("A DBus error occurred. All features requiring sessionbus are disabled."));
            delete sessionbus;
            sessionbus=NULL;
            return true;
            }
        }
        break;
      case DBUS_REQUEST_NAME_REPLY_EXISTS:
        dbus_send_commands(sessionbus->connection(),argc,argv);
        return false;
        break;
      default:
        FXMessageBox::warning(application,MBOX_OK,"Goggles Music Manager",fxtr("Session Bus not available. All features requiring sessionbus are disabled."));
        delete sessionbus;
        sessionbus=NULL;
        return true;
        break;
      }

    if (!systembus->open(DBUS_BUS_SYSTEM) || !systembus->connected()) {
      delete systembus;
      systembus=NULL;
      }

    if (systembus && !dbus_connection_add_filter(systembus->connection(),dbus_systembus_filter,this,NULL)){
      delete systembus;
      systembus=NULL;
      }

    if (systembus) {
      DBusError error;
      dbus_error_init(&error);
      dbus_bus_add_match(systembus->connection(),"type='signal',path='/org/freedesktop/NetworkManager',interface='org.freedesktop.NetworkManager',member='StateChanged'",&error);
      if (dbus_error_is_set(&error)) {
        dbus_error_free(&error);
        delete systembus;
        systembus=NULL;
        }
      }

    if (systembus) {
      DBusError error;
      dbus_error_init(&error);
      dbus_bus_add_match(systembus->connection(),"type='signal',path='/org/freedesktop/NetworkManager',interface='org.freedesktop.NetworkManager',member='StateChange'",&error);
      if (dbus_error_is_set(&error)) {
        dbus_error_free(&error);
        delete systembus;
        systembus=NULL;
        }
      }
    }
  return true;
  }

#endif



FXint GMPlayerManager::run(int& argc,char** argv) {
  FXint result;

  /// Initialize pre-thread libraries.
  GMTag::init();
  GMFetch::init();

  if (!init_gcrypt())
    return 1;

  /// Setup and migrate old config files.
  init_configuration();

  /// Create Application so we can do things like popup dialogs and such
  application = new GMApp();
  application->init(argc,argv);
  application->create();

  /// Keep track of child processes
  application->addSignal(SIGCHLD,this,GMPlayerManager::ID_CHILD);

  /// Make sure we're threadsafe
  if (GMDatabase::threadsafe()==0) {
    FXMessageBox::error(application,MBOX_OK,"Goggles Music Manager",
      fxtrformat("A non threadsafe version of SQLite (%s) is being used.\n"
      "Goggles Music Manager requires a threadsafe SQLite.\n"
      "Please upgrade your SQLite installation."),GMDatabase::version());
    return 1;
    }

  /// Give warning when PNG is not compiled in...
  if (FXPNGIcon::supported==false) {
    FXMessageBox::warning(application,MBOX_OK,"Goggles Music Manager",
      fxtr("For some reason the FOX library was compiled without PNG support.\n"
      "In order to show all icons, Goggles Music Manager requires PNG\n"
      "support in the FOX library. If you've compiled FOX yourself, most\n"
      "likely the libpng header files were not installed on your system."));
    }

  taskmanager = new GMTaskManager(this,ID_TASKMANAGER);

#ifdef HAVE_DBUS

  /// Connect to the dbus...
  if (!init_dbus(argc,argv))
    return 0;

  /// Fallback to fifo method to check for existing instance
  if (sessionbus==NULL) {
    result = init_fifo(argc,argv);
    if (result==FIFO_STATUS_ERROR)
      return 1;
    else if (result==FIFO_STATUS_EXISTS)
      return 0;
    }

#else
  result = init_fifo(argc,argv);
  if (result==FIFO_STATUS_ERROR)
    return 1;
  else if (result==FIFO_STATUS_EXISTS)
    return 0;
#endif

  /// Load Application Preferences
  preferences.load(application->reg());

  /// Check for overrides on the command line
  preferences.parseCommandLine(argc,argv);

  /// Open Database and initialize all sources.
  if (!init_sources())
    return false;

  /// Everything opened succesfully... now create the GUI
#ifdef HAVE_XINE_LIB
  player = new GMPlayer(application,argc,argv,this,ID_AUDIO_PLAYER);
#else
  player = new GMAudioPlayer(application,this,ID_AUDIO_PLAYER);
#endif

  /// Open Audio Device if needed.
#ifdef HAVE_XINE_LIB
  if (preferences.play_open_device_on_startup) {
    if (!player->init()) {
      FXString errormsg;
      player->getErrorMessage(errormsg);
      FXMessageBox::error(application,MBOX_OK,fxtr("Audio Device Error"),"%s",errormsg.text());
      }
    }
#else
  player->init();
#endif

  /// Receive events from fifo
  if (fifo.isOpen()) {
#if FOXVERSION < FXVERSION(1,7,0)
    application->addInput(fifo.handle(),INPUT_READ,this,GMPlayerManager::ID_DDE_MESSAGE);
#else
    application->addInput(this,GMPlayerManager::ID_DDE_MESSAGE,fifo.handle(),INPUT_READ);
#endif
    }

  FXString url = get_cmdline_url(argc,argv);

  /// Show user interface
  init_window(url.empty());

#ifdef HAVE_DBUS
  if (sessionbus) {
    /// Integrate Dbus into FOX Event Loop
    GMDBus::initEventLoop();
    }
#endif

#ifdef HAVE_LIRC
  init_lirc();
#endif

  /// Start Services
  scrobbler    = new GMAudioScrobbler(this,ID_SCROBBLER);
#ifdef HAVE_DBUS
  if (sessionbus) {
    notifydaemon = new GMNotifyDaemon(sessionbus);
    appstatus    = new GMAppStatusNotify(sessionbus);
    appstatus->show();

    /// Grab Media Player Keys
    gsd       = new GMSettingsDaemon(sessionbus,this,ID_GNOME_SETTINGS_DAEMON);
    gsd->GrabMediaPlayerKeys("gogglesmm");

    update_mpris();
    }
#endif

  /// Open url from command line
  if (!url.empty())
    open(url);

  update_album_covers();
  update_tray_icon();

  /// Run the application
  return application->run();
  }


void GMPlayerManager::exit() {

  /// Stop Playing
  stop();

  player->exit();

  /// Save settings
  for (FXint i=0;i<sources.no();i++)
    sources[i]->save(application->reg());


#ifdef HAVE_XINE_LIB
  application->removeTimeout(this,GMPlayerManager::ID_HANDLE_EVENTS);
#endif

  application->removeTimeout(this,GMPlayerManager::ID_UPDATE_TRACK_DISPLAY);

  application->removeTimeout(this,GMPlayerManager::ID_SLEEP_TIMER);

  application->removeTimeout(this,GMPlayerManager::ID_PLAY_NOTIFY);

  application->removeTimeout(this,GMPlayerManager::ID_TASKMANAGER_SHUTDOWN);

  application->removeTimeout(source,GMSource::ID_TRACK_PLAYED);

  preferences.save(application->reg());

  if (scrobbler) scrobbler->shutdown();
  if (taskmanager) taskmanager->shutdown();

#ifdef HAVE_DBUS
  if (sessionbus) {
    dbus_connection_unregister_object_path(sessionbus->connection(),"/org/fifthplanet/gogglesmm");

    gsd->ReleaseMediaPlayerKeys("gogglesmm");
    delete gsd;
    gsd=NULL;

    if (notifydaemon) delete notifydaemon;
    if (mpris) delete mpris;
    }
  if (systembus) {
    dbus_connection_remove_filter(systembus->connection(),dbus_systembus_filter,this);
    }
#endif

  /// Delete Sources
  for (FXint i=0;i<sources.no();i++)
    delete sources[i];

  delete thumbs;

  application->exit(0);
  }

#ifdef HAVE_DBUS
void GMPlayerManager::update_mpris() {
  if (mpris && ( !preferences.dbus_mpris || !sessionbus )) {
    delete mpris;
    mpris=NULL;
    }
  else if (!mpris && preferences.dbus_mpris && sessionbus) {
    mpris = new GMMediaPlayerService(sessionbus);
    }
  }
#endif



void GMPlayerManager::update_tray_icon() {
  if (trayicon && !preferences.gui_tray_icon) {
    delete trayicon;
    trayicon=NULL;
    }
  else if (!trayicon && preferences.gui_tray_icon) {
    trayicon = new GMTrayIcon(application);
    trayicon->create();
    }
  }

void GMPlayerManager::update_album_covers() {
  fxmessage("update_album_covers()\n");
//  if (preferences.gui_show_albumcovers) {
    thumbs->init(database);
//    }
//  else {
//    thumbs->clear();
//    getTrackView()->refresh();
//    }
/*

    if (!thumbs->isLoaded() && !thumbs->load()) {
      GMAlbumPathList list;
      database->listAlbumPaths(list);
      thumbs->generate(list);
      }
    }
  else {
//    GMCoverThumbs::reset();
    if (!thumbs->isLoaded()) {
      thumbs->clear();
      getTrackView()->refresh();
      }
    }
*/
  }



GMTrackView * GMPlayerManager::getTrackView() const {
  return mainwindow->trackview;
  }

GMSourceView * GMPlayerManager::getSourceView() const {
  return mainwindow->sourceview;
  }

FXString GMPlayerManager::getDatabaseFilename() const {
  return GMApp::getDataDirectory() + PATHSEPSTRING + DATABASE_FILENAME;
  }

FXbool GMPlayerManager::init_database(GMTrackDatabase * db){
  FXString databasefilename = getDatabaseFilename();

  if (FXStat::exists(databasefilename) && (!FXStat::isWritable(databasefilename) || !FXStat::isReadable(databasefilename)))
    return false;

  /// Init Database
  if (!db->init(databasefilename)) {
    return false;
    }

  return true;
  }

FXbool GMPlayerManager::hasSourceWithKey(const char * key) const{
  for (FXint i=0;i<sources.no();i++){
    if (sources[i]->settingKey()==key)
      return true;
    }
  return false;
  }


void GMPlayerManager::cleanSourceSettings() {
  FXint s;
  FXStringList keys;

  for (s=application->reg().first();s<application->reg().size();s=application->reg().next(s)){
    if (comparecase(application->reg().key(s),"database",8)==0){
      if (!hasSourceWithKey(application->reg().key(s))) {
        keys.append(application->reg().key(s));
        }
      }
    }

  for (s=0;s<keys.no();s++){
    application->reg().deleteSection(keys[s].text());
    }
  }

void GMPlayerManager::removePlayListSources(){
  GMSource * src;
  for (FXint i=sources.no()-1;i>=0;i--){
    src=sources[i];
    if (src->getType()==SOURCE_DATABASE_PLAYLIST) {
      FXApp::instance()->reg().deleteSection(sources[i]->settingKey().text());
      sources.erase(i);
      delete src;
      }
    }
  }

void GMPlayerManager::download(const FXString & filename){
  FXString status = "Downloading " + filename + " ...";
  GMPlayerManager::instance()->setStatus(status);
  GMFetch::download(filename);
  }

FXbool GMPlayerManager::play(const FXString & filename,FXbool flush) {
  player->open(filename,flush);

#if 0
#ifdef HAVE_XINE_LIB
  FXString errormsg;

  /// Open Filename
  if (!player->open(filename)){

    /// Reset Source
    if (source) {
      application->removeTimeout(source,GMSource::ID_TRACK_PLAYED);
      source->resetCurrent();
      source=NULL;
      }

    /// Reset Track Display
    reset_track_display();

    /// Show error dialog once we return to event loop
    application->addChore(this,ID_PLAYER_ERROR);

    /// Close
    if (preferences.play_close_stream)
      player->exit();

    return false;
    }

  /// Start Playback
  if (!player->play()) {

    /// Reset Source
    if (source) {
      application->removeTimeout(source,GMSource::ID_TRACK_PLAYED);
      source->resetCurrent();
      source=NULL;
      }

    /// Reset Track Display
    reset_track_display();

    /// Show error dialog once we return to event loop
    application->addChore(this,ID_PLAYER_ERROR);

    /// Close
    if (preferences.play_close_stream)
      player->exit();

    return false;
    }
#else
  fxmessage("open %s\n",filename.text());
  player->open(filename,flush);
//  player->play();
#endif
#endif
  return true;
  }

#ifdef HAVE_XINE_LIB
FXbool GMPlayerManager::play(const FXStringList & list) {
  FXString errormsg;

  for (FXint i=0;i<list.no();i++) {
    /// Open Filename
    if (!player->open(list[i])){
      continue;
      }
    /// Start Playback
    if (!player->play()) {
      continue;
      }
    return true;
    }

  /// Show error dialog once we return to event loop
  application->addChore(this,ID_PLAYER_ERROR);

  /// Reset Source
  if (source) {
    application->removeTimeout(source,GMSource::ID_TRACK_PLAYED);
    source->resetCurrent();
    source=NULL;
    }

  /// Reset Track Display
  reset_track_display();

  /// Close
  if (preferences.play_close_stream)
    player->exit();

  return false;
  }
#else
FXbool GMPlayerManager::play(const FXStringList &) {
  return false;
  }
#endif




void GMPlayerManager::open(const FXString & filename) {

  /// Stop Current Playback
  player->stop();

  /// Remove Current Timeout
  if (source) {
    application->removeTimeout(source,GMSource::ID_TRACK_PLAYED);
    source->resetCurrent();
    source=NULL;
    }

#ifdef HAVE_XINE_LIB
  /// Check for pls or m3u, since xine cannot handle that
  FXbool local = gm_is_local_file(filename);
  if (!local) {
    FXString extension = FXPath::extension(GMURL::path(filename));
    if ((comparecase(extension,"pls")==0) || (comparecase(extension,"m3u")==0)){
      download(filename);
      return;
      }
    }

  if (local) {
    FXint id;
    if (sources[0]->hasTrack(filename,id)) {
      sources[0]->setCurrentTrack(id);
      source=sources[0];
      getTrackView()->handle(this,FXSEL(SEL_COMMAND,GMTrackView::ID_SHOW_CURRENT),NULL);
      trackinfoset = source->getTrack(trackinfo);
      }
    else {
      /// Reset Active Track
      getTrackView()->mark(-1);
      }
    }
  else {
    /// Reset Active Track
    getTrackView()->mark(-1);
    }


  play(filename);
/*
    /// Play File
  if (play(filename) && local) {
    if (source==NULL) {
      player->getTrackInformation(trackinfo);
      }
    update_track_display();
    }
*/

#else
  FXbool local = gm_is_local_file(filename);
  if (local) {
    FXint id;
    if (sources[0]->hasTrack(filename,id)) {
      sources[0]->setCurrentTrack(id);
      source=sources[0];
//      getTrackView()->handle(this,FXSEL(SEL_COMMAND,GMTrackView::ID_SHOW_CURRENT),NULL);
      trackinfoset = source->getTrack(trackinfo);
      }
    else {
      /// Reset Active Track
      getTrackView()->mark(-1);
      }
    }
  else {
    /// Reset Active Track
    getTrackView()->mark(-1);
    }
  play(filename);
#endif
  }







void GMPlayerManager::play() {
  FXString filename;
  FXint track=-1;

  /// Stop Current Playback
  //player->stop();

  /// Remove Current Timeout
  if (source) {
    application->removeTimeout(source,GMSource::ID_TRACK_PLAYED);
    source->resetCurrent();
    source=NULL;
    }


  if (queue) {

    /// Get the track
    track = queue->getCurrent();
    if (track!=-1) {
      trackinfoset = queue->getTrack(trackinfo);
      source = queue;
      }
    }

  if (track==-1) {

    /// Get Track
    track = getTrackView()->getCurrent();
    if (track==-1)
      return;

    /// Mark Track
    source = getTrackView()->getSource();
    getTrackView()->mark(track);

    /// Get the track info
    trackinfoset = source->getTrack(trackinfo);
    }

  /// Check for pls or m3u, since xine cannot handle that
  FXbool local = gm_is_local_file(trackinfo.mrl);
  if (!local) {
    FXString extension = FXPath::extension(GMURL::path(trackinfo.mrl));
    if ((comparecase(extension,"pls")==0) || (comparecase(extension,"m3u")==0)){
      download(trackinfo.mrl);
      return;
      }
    }

//#ifdef HAVE_GAP
  play(trackinfo.mrl);
//#else
//  if (play(trackinfo.mrl) && local)
//    update_track_display();
//#endif
  }


void GMPlayerManager::stop(FXbool force_close) {

  /// Wait for download to finish.
  GMFetch::cancel_and_wait();

  /// Reset Source
  if (source) {
    application->removeTimeout(source,GMSource::ID_TRACK_PLAYED);
    source->resetCurrent();
    source=NULL;
    }

#ifdef HAVE_XINE_LIB
  if (preferences.play_close_stream || force_close){
    player->stop();
    player->exit();
    }
  else {
    player->stop();
    }
#else
  player->stop();
#endif

  /// Reset Track Display
  reset_track_display();
  }

void GMPlayerManager::next() {
  FXint track=-1;

  player->stop();

  if (source) {
    application->removeTimeout(source,GMSource::ID_TRACK_PLAYED);
    source->resetCurrent();
    source=NULL;
    }

  if (queue) {
    source = queue;
    track = queue->getNext();
    if (track!=-1) {
      trackinfoset = queue->getTrack(trackinfo);
      }
    }

  if (track==-1) {
    track = getTrackView()->getNext(true);
    if (track==-1) {
      reset_track_display();
      return;
      }

    source = getTrackView()->getSource();
    getTrackView()->mark(track);

    /// Get the Track info
    trackinfoset = source->getTrack(trackinfo);
    }

  /// Check for pls or m3u, since xine cannot handle that
  FXbool local = gm_is_local_file(trackinfo.mrl);
  if (!local) {
    FXString extension = FXPath::extension(GMURL::path(trackinfo.mrl));
    if ((comparecase(extension,"pls")==0) || (comparecase(extension,"m3u")==0)){
      download(trackinfo.mrl);
      return;
      }
    }

  play(trackinfo.mrl);
  }


void GMPlayerManager::prev() {
  FXString filename;
  FXint track=-1;

  /// Stop Current Playback
  player->stop();

  /// Remove Current Timeout
  if (source) {
    application->removeTimeout(source,GMSource::ID_TRACK_PLAYED);
    source->resetCurrent();
    source=NULL;
    }

  if (queue) {
    source = queue;
    track = queue->getPrev();
    if (track!=-1)
      trackinfoset = queue->getTrack(trackinfo);
    }

  if (track==-1) {
    track = getTrackView()->getPrevious();
    if (track==-1) {
      reset_track_display();
      return;
      }

    /// Mark Track
    source = getTrackView()->getSource();
    getTrackView()->mark(track);

    /// Get the Track info
    trackinfoset = source->getTrack(trackinfo);
    }

  /// Check for pls or m3u, since xine cannot handle that
  FXbool local = gm_is_local_file(trackinfo.mrl);
  if (!local) {
    FXString extension = FXPath::extension(GMURL::path(trackinfo.mrl));
    if ((comparecase(extension,"pls")==0) || (comparecase(extension,"m3u")==0)){
      download(trackinfo.mrl);
      return;
      }
    }

  play(trackinfo.mrl);
  }



void GMPlayerManager::seek(FXdouble pos) {
  application->removeTimeout(source,GMSource::ID_TRACK_PLAYED);
  player->seek(pos);
#ifdef HAVE_XINE_LIB
  application->addTimeout(this,GMPlayerManager::ID_HANDLE_EVENTS,TIME_MSEC(500));
#endif
  }


void GMPlayerManager::pause() {
  if (preferences.play_pause_close_device){
    player->pause();
#ifdef HAVE_XINE_LIB
    player->close_device();
#endif
    }
  else {
    player->pause();
    }

  if (application->hasTimeout(source,GMSource::ID_TRACK_PLAYED)) {
//    count_track_remaining=(((FXuint)(((double)trackinfo.time) * 0.80)) * 1000) - player->getPositionMS();

    count_track_remaining = application->remainingTimeout(source,GMSource::ID_TRACK_PLAYED);
//    fxmessage("timeout remaining to %u ms (%u s)\n",count_track_remaining,count_track_remaining/1000);
    application->removeTimeout(source,GMSource::ID_TRACK_PLAYED);
    }
  else {
    count_track_remaining=0;
    }
  }

void GMPlayerManager::unpause() {
#ifdef HAVE_XINE_LIB
  player->unpause();

  if (player->getVolume()==0)
    player->setVolume(50);

  if (count_track_remaining && source){
    application->addTimeout(source,GMSource::ID_TRACK_PLAYED,count_track_remaining);
    }
  count_track_remaining=0;

  mainwindow->update_volume_display(player->getVolume());
#else
 //fxmessage("unpause()\n");
  player->pause();
#endif
 }

FXbool GMPlayerManager::playlist_empty() {
  if (getTrackView()->hasTracks()) return false;
  if (preferences.play_repeat!=REPEAT_OFF) return false;
  if (getTrackView()->getNext()!=-1) return false;
  return true;
  }

void GMPlayerManager::notify_playback_finished() {
  FXString errormsg;
  FXString filename;
  FXint track=-1;
#ifdef HAVE_XINE_LIB
  FXint remaining = player->remaining();
#endif

  if (queue) {

    /// Reset Source
    if (source!=queue) {
     source->resetCurrent();
     source=NULL;
     }

    track = queue->getNext();
    if (track==-1) {
      source = NULL;

      if (getTrackView()->getSource()==queue)
        getTrackView()->refresh();

     reset_track_display();
     return;
     }


    trackinfoset = queue->getTrack(trackinfo);
    }
  else {
    /// Can we just start playback without user interaction
    if (!getTrackView()->getSource()->autoPlay()) {

      /// Reset Source
      if (source) {
         source->resetCurrent();
         source=NULL;
         }

       reset_track_display();
       return;
       }

    if (source) {
      source->resetCurrent();
      source=NULL;
      }

    if (preferences.play_repeat==REPEAT_TRACK)
      track = getTrackView()->getCurrent();
    else
      track = getTrackView()->getNext();

    if (track==-1) {
      reset_track_display();
      return;
      }
//(remaining<=0)
    getTrackView()->mark(track,false);

    source = getTrackView()->getSource();
    trackinfoset = source->getTrack(trackinfo);
    }


#ifdef HAVE_XINE_LIB
  if (play(trackinfo.mrl,false)) {
    if (remaining>0)
      application->addTimeout(this,GMPlayerManager::ID_UPDATE_TRACK_DISPLAY,TIME_SEC(remaining),(void*)(FXival)track);
    else
      update_track_display();
    }
#else
  play(trackinfo.mrl,false);
#endif
  }

FXbool GMPlayerManager::playing() const {
  return player->playing() || GMFetch::busy() ;
  }

FXbool GMPlayerManager::audio_device_opened() const{
#ifdef HAVE_XINE_LIB
  return player->opened();
#else
  return true;
#endif
  }

FXint GMPlayerManager::current_position() const {
#ifdef HAVE_XINE_LIB
  return player->getPosition();
#else
  return 0;
#endif
  }

void GMPlayerManager::reset_track_display() {
  FXTRACE((51,"GMPlayerManager::reset_track_display()\n"));

  /// Reset Main Window
  mainwindow->reset();

  if (trayicon) trayicon->reset();

  /// Reset Active Track
  getTrackView()->mark(-1);

  /// Remove Notify
  application->removeTimeout(this,ID_PLAY_NOTIFY);


#ifdef HAVE_DBUS
  if (notifydaemon && preferences.dbus_notify_daemon)
    notifydaemon->close();
#endif

  /// Update View in queue play.
  if (queue) {
    getSourceView()->refresh(queue);
    if (getTrackView()->getSource()==queue) {
      getTrackView()->refresh();
      }
    }

  /// Schedule a GUI update
  application->refresh();
  }


void GMPlayerManager::setStatus(const FXString & text){
  mainwindow->statusbar->getStatusLine()->setNormalText(text);
  }

void GMPlayerManager::update_cover_display() {
  if (preferences.gui_show_playing_albumcover && gm_is_local_file(trackinfo.mrl))
    mainwindow->loadCover(trackinfo.mrl);
  }


void GMPlayerManager::update_track_display(FXbool notify) {
  FXTRACE((51,"GMPlayerManager::update_track_display()\n"));

  /// If track information is not set, we need to get the latest from the player.
#ifdef HAVE_XINE_LIB
  if (!trackinfoset && player->playing()) {
    player->getTrackInformation(trackinfo);
    if (source) source->setTrack(trackinfo);
    }
#endif

  if (source) {
    FXint time = (FXint) (((double)trackinfo.time) * 0.80);
    if (time <= 5) {
      application->removeTimeout(source,GMSource::ID_TRACK_PLAYED);
      source->handle(this,FXSEL(SEL_TIMEOUT,GMSource::ID_TRACK_PLAYED),NULL);
      }
    else {
      count_track_remaining=0;
      application->addTimeout(source,GMSource::ID_TRACK_PLAYED,TIME_SEC(time));
      }
    }

#ifdef HAVE_XINE_LIB
  if (trayicon && player->playing()) {
    trayicon->display(trackinfo);
    }
#endif

  mainwindow->display(trackinfo);

#ifdef HAVE_XINE_LIB
  /// Make sure Volume Level is up 2 date
  mainwindow->update_volume_display(player->getVolume());
#endif

  update_replay_gain();

  if (notify) application->addTimeout(this,ID_PLAY_NOTIFY,TIME_MSEC(500));


  if (queue) {
    getSourceView()->refresh(queue);
    if (getTrackView()->getSource()==queue)
      getTrackView()->refresh();
    getTrackView()->showCurrent();
    }

  }

void GMPlayerManager::update_replay_gain() {
#ifdef HAVE_XINE_LIB
  switch(preferences.play_replaygain){
    case REPLAYGAIN_OFF   : player->setReplayGain(NAN,NAN); break;
    case REPLAYGAIN_TRACK : player->setReplayGain(trackinfo.track_gain,trackinfo.track_peak); break;
    case REPLAYGAIN_ALBUM : player->setReplayGain(trackinfo.album_gain,trackinfo.album_peak); break;
    }
#endif
  }


void GMPlayerManager::handle_async_events() {
#ifdef HAVE_XINE_LIB
  /// Get Events from player
  player->handle_async_events();

  /// Mark Time
  mainwindow->update_elapsed_time(player->getHours(),player->getMinutes(),player->getSeconds(),player->getPosition(),player->playing(),player->seekable());
#endif
  }

FXint GMPlayerManager::volume() const{
#ifdef HAVE_XINE_LIB
  return player->getVolume();
#else
  return 0;
#endif
  }

void GMPlayerManager::volume(FXint l) {
#ifdef HAVE_XINE_LIB
  player->setVolume(l);
#else
  player->volume((FXfloat)l/100.0f);
#endif
  }

FXbool GMPlayerManager::can_stop() const {
#ifdef HAVE_XINE_LIB
  if (player->playing() || GMFetch::busy() ) return true;
  return false;
#else
  if (player->playing() || GMFetch::busy() ) return true;
  return false;
#endif
  }

FXbool GMPlayerManager::can_play() const {
#ifdef HAVE_XINE_LIB

//  if (queue) {
    return (!player->playing() && ((queue && queue->getNumTracks()>0) ||  getTrackView()->hasTracks())  && !GMFetch::busy());
//    }
//  else {
//    return (!player->playing() && getTrackView()->hasTracks() && !GMFetch::busy());
//    }
  return false;

//  return false;
#else
  return (!player->playing() && ((queue && queue->getNumTracks()>0) ||  getTrackView()->hasTracks())  && !GMFetch::busy());


//  return true;
#endif
  }

FXbool GMPlayerManager::can_pause() const {
#ifdef HAVE_XINE_LIB
  if (player->playing() && !player->pausing())
    return true;
  return false;
#else
  if (player->playing() && !player->pausing())
    return true;
  return false;
#endif
  }

FXbool GMPlayerManager::can_unpause() const {
#ifdef HAVE_XINE_LIB
  if (player->playing() && player->pausing())
    return true;
  return false;
#else
  if (player->pausing())
    return true;
  return false;
#endif
  }

FXbool GMPlayerManager::can_next() const {
  if (player->playing() && !player->pausing()) {
    if (( queue && queue->getNumTracks()>1) || getTrackView()->getNumTracks()>1)
      return true;
//      return (queue->getNumTracks()>1);
//    else
//      return (getTrackView()->getNumTracks()>1);
    }
  return false;
  }

FXbool GMPlayerManager::can_prev() const {
  if (player->playing() && !player->pausing()) {
    return (getTrackView()->getNumTracks()>1);
    }
  return false;
  }

#if FOXVERSION < 107000
void GMPlayerManager::setSleepTimer(FXuint ms) {
  if (ms==0)
    application->removeTimeout(this,GMPlayerManager::ID_SLEEP_TIMER);
  else
    application->addTimeout(this,GMPlayerManager::ID_SLEEP_TIMER,ms);
  }
#else
void GMPlayerManager::setSleepTimer(FXlong ns) {
  if (ns==0)
    application->removeTimeout(this,GMPlayerManager::ID_SLEEP_TIMER);
  else
    application->addTimeout(this,GMPlayerManager::ID_SLEEP_TIMER,ns);
  }
#endif

FXbool GMPlayerManager::hasSleepTimer() {
  return application->hasTimeout(this,GMPlayerManager::ID_SLEEP_TIMER);
  }

void GMPlayerManager::show_message(const FXchar * title,const FXchar * msg){
  if (application->getActiveWindow() && application->getActiveWindow()->shown()) {
    FXMessageBox::error(application->getActiveWindow(),MBOX_OK,title,"%s",msg);
    }
  else {
    if (mainwindow && mainwindow->shown())
      FXMessageBox::error(mainwindow,MBOX_OK,title,"%s",msg);
    else if (mainwindow->getRemote())
      FXMessageBox::error(mainwindow->getRemote(),MBOX_OK,title,"%s",msg);
    else
      FXMessageBox::error(application,MBOX_OK,title,"%s",msg);
    }
  }


long GMPlayerManager::onCmdCloseWindow(FXObject*sender,FXSelector,void*){
  FXWindow * window = reinterpret_cast<FXWindow*>(sender);
  if (getPreferences().gui_hide_player_when_close) {
    window->hide();
    }
  else {
    getMainWindow()->handle(this,FXSEL(SEL_COMMAND,GMWindow::ID_QUIT),NULL);
    }
  return 1;
  }


long GMPlayerManager::onCmdChild(FXObject*,FXSelector,void*){
  FXint pid;
  FXint status;
  while(1) {
    pid = waitpid(-1,&status,WNOHANG);
    if (pid>0) {
      continue;
      }
    break;
    }
  return 1;
  }

long GMPlayerManager::onScrobblerError(FXObject*,FXSelector,void*ptr){
  show_message(fxtr("Last.FM Error"),(const FXchar*)ptr);
  return 1;
  }

long GMPlayerManager::onScrobblerOpen(FXObject*,FXSelector,void*ptr){
  gm_open_browser((const FXchar*)ptr);
  return 1;
  }


long GMPlayerManager::onPlayerError(FXObject*,FXSelector,void*){
#ifdef HAVE_XINE_LIB
  FXString errormsg;
  player->getErrorMessage(errormsg);
  show_message(fxtr("Playback Error"),errormsg.text());
#endif
  return 1;
  }










long GMPlayerManager::onCmdDownloadComplete(FXObject*,FXSelector,void*ptr){
  GMFetchResponse * response = *((GMFetchResponse**)ptr);
  if (response) {
    if (response->url == trackinfo.mrl ) {
      FXStringList list;
      FXString extension = FXPath::extension(GMURL::path(response->url));

      if (comparecase(extension,"pls")==0)
        gm_parse_pls(response->data,list);
      else if (comparecase(extension,"m3u")==0)
        gm_parse_m3u(response->data,list);

      if (list.no())
        play(list);
      }
    delete response;
    }
  return 1;
  }

long GMPlayerManager::onImportTaskCompleted(FXObject*,FXSelector,void*ptr){
  ///fxmessage("TASK COMPLETED\n");
  GMTask * task = *((GMTask**)ptr);
  delete task;

  /// Update the covers
//  if (preferences.gui_show_albumcovers) {
    thumbs->refresh(database);
 //   }

  //FIXME  only refresh when we have the music database open.
  getTrackView()->refresh();
  return 0;
  }



void GMPlayerManager::runTask(GMTask * task) {
  application->removeTimeout(this,GMPlayerManager::ID_TASKMANAGER_SHUTDOWN);
  taskmanager->run(task);
  }

long GMPlayerManager::onTaskManagerIdle(FXObject*,FXSelector,void*){
  mainwindow->setStatus(FXString::null);
  fxmessage("Schedule taskmanager shutdown in 30s\n");
  application->addTimeout(this,GMPlayerManager::ID_TASKMANAGER_SHUTDOWN,TIME_SEC(30));
  return 0;
  }

long GMPlayerManager::onTaskManagerRunning(FXObject*,FXSelector,void*){
  fxmessage("Taskmanager running\n");
  application->removeTimeout(this,GMPlayerManager::ID_TASKMANAGER_SHUTDOWN);
  return 0;
  }


long GMPlayerManager::onTaskManagerShutdown(FXObject*,FXSelector,void*){
  fxmessage("Shutdown taskmanager now\n");
  taskmanager->shutdown();
  return 0;
  }


long GMPlayerManager::onTaskManagerStatus(FXObject*,FXSelector,void*ptr){



  FXchar * msg = (FXchar*)ptr;
//  fxmessage("msg: %s\n",msg);
//  free(msg);

  mainwindow->setStatus(msg);
  return 0;
  }

long GMPlayerManager::onCancelTask(FXObject*,FXSelector,void*){
  taskmanager->cancelTask();
  return 0;
  }


long GMPlayerManager::onCmdEqualizer(FXObject *,FXSelector,void*){
#ifdef HAVE_XINE_LIB
  GMEQDialog * eqdialog = GMEQDialog::instance();
  if (eqdialog==NULL) {
    eqdialog = new GMEQDialog(mainwindow);
    eqdialog->create();
    }
  eqdialog->show();
#endif
  return 1;
  }


void GMPlayerManager::cmd_play(){
  if (can_unpause())
    unpause();
  else if (can_play())
    play();
  }


void GMPlayerManager::cmd_playpause(){
  if (can_pause())
    pause();
  else if (can_unpause())
    unpause();
  else if (can_play())
    play();
  }


void GMPlayerManager::cmd_pause(){
  if (can_pause())
    pause();
  else if (can_unpause())
    unpause();
  }

void GMPlayerManager::cmd_stop(){
  if (can_stop())
    stop();
  }

void GMPlayerManager::cmd_next(){
  if (can_next())
    next();
  }
void GMPlayerManager::cmd_prev(){
  if (can_prev())
    prev();
  }

void GMPlayerManager::cmd_toggle_shown(){
  getMainWindow()->toggleShown();
  }




void GMPlayerManager::display_track_notification() {
#ifdef HAVE_DBUS
  if (sessionbus && !trackinfo.title.empty() && !trackinfo.artist.empty() && preferences.dbus_notify_daemon) {
    if (notifydaemon) {
      FXString body = GMStringFormat(fxtrformat("%s\n%s (%d)"),trackinfo.artist.text(),trackinfo.album.text(),trackinfo.year);
      /// Dirty Hack. According to the spec, we shouldn't have to do this,
      /// but try finding a notification notifydaemon that actually implements it...
      /// http://www.galago-project.org/specs/notification/0.9/index.html
      body.substitute("&","&amp;");
      notifydaemon->notify("gogglesmm","gogglesmm_status",trackinfo.title.text(),body.text(),-1,mainwindow->getSmallCover());
      }
    if (mpris) mpris->notify_track_change(trackinfo);
    }
#endif
  }


#ifdef HAVE_DBUS
long GMPlayerManager::onCmdSettingsDaemon(FXObject*,FXSelector,void*ptr){
  const FXchar * cmd = (const FXchar*)ptr;
  if (comparecase(cmd,"play")==0) cmd_playpause();
  else if (comparecase(cmd,"pause")==0) cmd_playpause();
  else if (comparecase(cmd,"stop")==0) cmd_stop();
  else if (comparecase(cmd,"previous")==0) cmd_prev();
  else if (comparecase(cmd,"next")==0) cmd_next();
  else {
    GM_DEBUG_PRINT("Unknown or unhandled key press: %s\n",cmd);
    }
  return 1;
  }
#endif


#ifdef HAVE_LIRC
long GMPlayerManager::onCmdLirc(FXObject*,FXSelector,void*){
  FXchar * code=NULL;
  FXchar * action=NULL;
  FXint result;
  while(lirc_nextcode(&code)==0 && code) {
//    fxmessage("code: %s\n",code);
    while((result=lirc_code2char(lirc_config,code,&action))==0 && action) {
      fxmessage("Action: %s\n",action);
      if (comparecase(action,"play")==0)
        cmd_play();
      else if (comparecase(action,"pause")==0)
        cmd_pause();
      else if (comparecase(action,"prev")==0)
        cmd_prev();
      else if (comparecase(action,"next")==0)
        cmd_next();
      else if (comparecase(action,"volup")==0){
        fxmessage("vol %d\n",volume());
        volume(FXMIN(volume()+1,100));
        }
      else if (comparecase(action,"voldown")==0){
        fxmessage("vol %d\n",volume());
        volume(FXMAX(volume()-1,0));
        }
      else if (comparecase(action,"mute")==0){
        volume(0);
        }
      else if (comparecase(action,"right")==0){
        FXWindow * window = application->getFocusWindow();
        if (window) {
          if (dynamic_cast<GMTrackList*>(window)) {
            getSourceView()->getSourceList()->setFocus();
            }
          }
        }
      else if (comparecase(action,"ok")==0){
        FXWindow * window = application->getFocusWindow();
        if (window) {
          GMTrackList * tracklist;
          if ((tracklist=dynamic_cast<GMTrackList*>(window))!=NULL) {
            play();
            }
          }
        }
      else if (comparecase(action,"down")==0 || comparecase(action,"up")==0){
        FXWindow * window = application->getFocusWindow();
        if (window) {
          GMTrackList * tracklist;
          if ((tracklist=dynamic_cast<GMTrackList*>(window))!=NULL) {
            FXint index=tracklist->getCurrentItem();
            if (comparecase(action,"down")==0) {
              if (index<tracklist->getNumItems()-1)
                index++;
              else
                index=0;
              }
            else {
             if (index>0)
                index--;
              else
                index=tracklist->getNumItems()-1;
              }
            tracklist->setCurrentItem(index,false);
            tracklist->makeItemVisible(index);
            tracklist->killSelection(false);
            tracklist->selectItem(index,false);
            tracklist->setAnchorItem(index);
            }
          }
        }


      }
    free(code);
    if (result==-1) return 0;
    }
  return 1;
  }
#endif

// Perhaps should do something else...
static int xregisterhotkeys(Display* dpy,XErrorEvent* eev){
  char buf[256];

  if(eev->error_code==BadAccess && eev->request_code==33) return 0;

  // A BadWindow due to X_SendEvent is likely due to XDND
  if(eev->error_code==BadWindow && eev->request_code==25) return 0;

  // WM_TAKE_FOCUS causes sporadic errors for X_SetInputFocus
  if(eev->request_code==42) return 0;

  // Get error codes
  XGetErrorText(dpy,eev->error_code,buf,sizeof(buf));

  // Print out meaningful warning
  fxwarning("GMM X Error: code %d major %d minor %d: %s.\n",eev->error_code,eev->request_code,eev->minor_code,buf);
  return 1;
  }


void GMPlayerManager::register_global_hotkeys() {
  Window root   = application->getRootWindow()->id();
  Display * display = (Display*) application->getDisplay();
  KeyCode keycode;

  XErrorHandler previous = XSetErrorHandler(xregisterhotkeys);

  /// Only register hotkeys on the rootwindow.
#ifdef XF86XK_AudioPlay
  keycode = XKeysymToKeycode(display,XF86XK_AudioPlay);
  if (keycode) XGrabKey(display,keycode,AnyModifier,root,False,GrabModeAsync,GrabModeAsync);
#endif
#ifdef XF86XK_AudioPause
  keycode = XKeysymToKeycode(display,XF86XK_AudioPause);
  if (keycode) XGrabKey(display,keycode,AnyModifier,root,False,GrabModeAsync,GrabModeAsync);
#endif
#ifdef XF86XK_AudioStop
  keycode = XKeysymToKeycode(display,XF86XK_AudioStop);
  if (keycode) XGrabKey(display,keycode,AnyModifier,root,False,GrabModeAsync,GrabModeAsync);
#endif
#ifdef XF86XK_AudioNext
  keycode = XKeysymToKeycode(display,XF86XK_AudioNext);
  if (keycode) XGrabKey(display,keycode,AnyModifier,root,False,GrabModeAsync,GrabModeAsync);
#endif
#ifdef XF86XK_AudioPrev
  keycode = XKeysymToKeycode(display,XF86XK_AudioPrev);
  if (keycode) XGrabKey(display,keycode,AnyModifier,root,False,GrabModeAsync,GrabModeAsync);
#endif
//#ifdef XF86XK_AudioMute
//  keycode = XKeysymToKeycode(display,XF86XK_AudioMute);
//  if (keycode) XGrabKey(display,keycode,AnyModifier,root,False,GrabModeAsync,GrabModeAsync);
//#endif
//#ifdef XF86XK_AudioLowerVolume
//  keycode = XKeysymToKeycode(display,XF86XK_AudioLowerVolume);
//  if (keycode) XGrabKey(display,keycode,AnyModifier,root,False,GrabModeAsync,GrabModeAsync);
//#endif
//#ifdef XF86XK_AudioRaiseVolume
//  keycode = XKeysymToKeycode(display,XF86XK_AudioRaiseVolume);
//  if (keycode) XGrabKey(display,keycode,AnyModifier,root,False,GrabModeAsync,GrabModeAsync);
//#endif
//#ifdef XF86XK_AudioRepeat
//  keycode = XKeysymToKeycode(display,XF86XK_AudioRepeat);
//  if (keycode) XGrabKey(display,keycode,AnyModifier,root,False,GrabModeAsync,GrabModeAsync);
//#endif
//#ifdef XF86XK_AudioRandomPlay
//  keycode = XKeysymToKeycode(display,XF86XK_AudioRandomPlay);
//  if (keycode) XGrabKey(display,keycode,AnyModifier,root,False,GrabModeAsync,GrabModeAsync);
//#endif

  XSync (display,False);
  XSetErrorHandler(previous);
  }

FXbool GMPlayerManager::handle_global_hotkeys(FXuint code) {
  switch(code) {
//    case XF86XK_AudioMute	        : break;
//    case XF86XK_AudioRaiseVolume	: break;
//    case XF86XK_AudioLowerVolume	: break;
#ifdef XF86XK_AudioPlay
    case XF86XK_AudioPlay         : cmd_playpause(); break;
#endif
#ifdef XF86XK_AudioPause
    case XF86XK_AudioPause	      : cmd_playpause(); break;
#endif
#ifdef XF86XK_AudioStop
    case XF86XK_AudioStop	        : cmd_stop(); break;
#endif
#ifdef XF86XK_AudioPrev
    case XF86XK_AudioPrev	        : cmd_prev(); break;
#endif
#ifdef XF86XK_AudioNext
    case XF86XK_AudioNext	        : cmd_next(); break;
#endif
//    case XF86XK_AudioRepeat	      : break;
//    case XF86XK_AudioRandomPlay	  : break;
    default                       : return false; break;
    }
  return true;
  }

//#ifndef HAVE_XINE_LIB
long GMPlayerManager::onPlayerBOS(FXObject*,FXSelector,void*){
  fxmessage("playback started\n");
  update_track_display();
  getTrackView()->showCurrent();
  return 1;
  }

long GMPlayerManager::onPlayerEOS(FXObject*,FXSelector,void*){
  fxmessage("playback finished\n");
  notify_playback_finished();
  return 1;
  }

long GMPlayerManager::onPlayerTime(FXObject*,FXSelector,void* ptr){

  PlaybackTime * tm = (PlaybackTime*)(ptr);
  FXint time = tm->position;
  FXint pos=0;

  if (tm->length) {
//    fxmessage("%d/%d -> %g -> %d\n",tm->position,tm->length,( (double)tm->position / (double)tm->length),(FXint)(100000.0 * ( (double)tm->position / (double)tm->length)));
    pos = (FXint)(100000.0 * ( (double)tm->position / (double)tm->length));
    }
  FXint  hours  = (FXint) floor((double)time/3600.0);
    time  -= (FXint) (3600*hours);
  FXint minutes= (FXint) floor((double)time/60.0);
    time  -= (FXint) (60*minutes);
  FXint  seconds= (FXint) floor((double)time);

  /// Mark Time
  mainwindow->update_elapsed_time(hours,minutes,seconds,pos,true,true);
  return 1;
  }

long GMPlayerManager::onPlayerState(FXObject*,FXSelector,void* ptr){
  FXint state = (FXint)(FXival)ptr;
  switch(state){
    case PLAYER_STOPPED: reset_track_display(); break;
    case PLAYER_PLAYING: break;
    case PLAYER_PAUSING: break;
    }
#ifdef HAVE_DBUS
  if (mpris) mpris->notify_status_change();
#endif
  return 1;
  }

//#endif



FXint GMPlayerManager::createPlaylist(const FXString & name) {
  FXint playlist=-1;
  if (database->insertPlaylist(name,playlist)) {
    insertSource(new GMPlayListSource(database,playlist));
    getSourceView()->refresh();
    }
  return playlist;
  }



FXuint GMPlayerManager::getMainWindowId() const {
  return mainwindow->id();
  }


