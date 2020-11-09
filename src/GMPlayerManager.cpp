/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2006-2018 by Sander Jansen. All Rights Reserved      *
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

#include "gmdefs.h"

#include <xincs.h>
#include <X11/XF86keysym.h>


#include "gmutils.h"

#include "GMSession.h"

#include "GMCover.h"
#include "GMTrack.h"
#include "GMTrackDatabase.h"

#include "GMCoverManager.h"

#ifdef HAVE_DBUS
#include "GMDBus.h"
#include "GMNotifyDaemon.h"
#include "GMSettingsDaemon.h"
#include "GMMediaPlayerService.h"
#endif

#include <FXPNGIcon.h>
#include "GMApp.h"
#include "GMTrackList.h"
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
#include "GMPodcastSource.h"

#include "GMFilter.h"
#include "GMFilterSource.h"


#include "GMPlayQueue.h"
#include "GMLocalSource.h"

#include "GMIconTheme.h"
#include "GMPlayerManager.h"
#include "GMTrayIcon.h"

#include "GMWindow.h"
#include "GMRemote.h"

#include "GMCoverCache.h"
#include "GMAudioPlayer.h"

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
  FXMAPFUNC(SEL_TIMEOUT,GMPlayerManager::ID_SLEEP_TIMER,GMPlayerManager::onCmdSleepTimer),
  FXMAPFUNC(SEL_TIMEOUT,GMPlayerManager::ID_PLAY_NOTIFY,GMPlayerManager::onPlayNotify),
  FXMAPFUNC(SEL_IO_READ,GMPlayerManager::ID_DDE_MESSAGE,GMPlayerManager::onDDEMessage),
  FXMAPFUNC(SEL_CLOSE,GMPlayerManager::ID_WINDOW,GMPlayerManager::onCmdCloseWindow),
  FXMAPFUNC(SEL_SIGNAL,GMPlayerManager::ID_CHILD,GMPlayerManager::onCmdChild),

  FXMAPFUNC(SEL_COMMAND,GMPlayerManager::ID_SCROBBLER,GMPlayerManager::onScrobblerError),
  FXMAPFUNC(SEL_OPENED,GMPlayerManager::ID_SCROBBLER,GMPlayerManager::onScrobblerOpen),

#ifdef HAVE_DBUS
  FXMAPFUNC(SEL_KEYPRESS,GMPlayerManager::ID_GNOME_SETTINGS_DAEMON,GMPlayerManager::onCmdSettingsDaemon),
#endif
  FXMAPFUNC(SEL_PLAYER_BOS,GMPlayerManager::ID_AUDIO_PLAYER,GMPlayerManager::onPlayerBOS),
  FXMAPFUNC(SEL_PLAYER_EOS,GMPlayerManager::ID_AUDIO_PLAYER,GMPlayerManager::onPlayerEOS),
  FXMAPFUNC(SEL_PLAYER_TIME,GMPlayerManager::ID_AUDIO_PLAYER,GMPlayerManager::onPlayerTime),
  FXMAPFUNC(SEL_PLAYER_STATE,GMPlayerManager::ID_AUDIO_PLAYER,GMPlayerManager::onPlayerState),
  FXMAPFUNC(SEL_PLAYER_META,GMPlayerManager::ID_AUDIO_PLAYER,GMPlayerManager::onPlayerMeta),
  FXMAPFUNC(SEL_PLAYER_ERROR,GMPlayerManager::ID_AUDIO_PLAYER,GMPlayerManager::onPlayerError),
  FXMAPFUNC(SEL_PLAYER_VOLUME,GMPlayerManager::ID_AUDIO_PLAYER,GMPlayerManager::onPlayerVolume),

  FXMAPFUNC(SEL_COMMAND,GMPlayerManager::ID_CANCEL_TASK,GMPlayerManager::onCancelTask),

  FXMAPFUNC(SEL_TASK_STATUS,GMPlayerManager::ID_TASKMANAGER,GMPlayerManager::onTaskManagerStatus),
  FXMAPFUNC(SEL_TASK_IDLE,GMPlayerManager::ID_TASKMANAGER,GMPlayerManager::onTaskManagerIdle),
  FXMAPFUNC(SEL_TASK_RUNNING,GMPlayerManager::ID_TASKMANAGER,GMPlayerManager::onTaskManagerRunning),

  FXMAPFUNC(SEL_TIMEOUT,GMPlayerManager::ID_TASKMANAGER_SHUTDOWN,GMPlayerManager::onTaskManagerShutdown),

  FXMAPFUNC(SEL_TASK_COMPLETED,GMPlayerManager::ID_IMPORT_TASK,GMPlayerManager::onImportTaskCompleted),
  FXMAPFUNC(SEL_TASK_CANCELLED,GMPlayerManager::ID_IMPORT_TASK,GMPlayerManager::onImportTaskCompleted),
#ifdef HAVE_SESSION
  FXMAPFUNC(SEL_SESSION_CLOSED,GMPlayerManager::ID_SESSION_MANAGER,GMPlayerManager::onCmdQuit)
#endif
  };

FXIMPLEMENT(GMPlayerManager,FXObject,GMPlayerManagerMap,ARRAYNUMBER(GMPlayerManagerMap))

#ifdef HAVE_DBUS

DBusHandlerResult dbus_systembus_filter(DBusConnection *,DBusMessage * msg,void * data){
  FXTRACE((80,"-----dbus_systembus_filter-------\n"));
  FXTRACE((80,"path: %s\n",dbus_message_get_path(msg)));
  FXTRACE((80,"member: \"%s\"\n",dbus_message_get_member(msg)));
  FXTRACE((80,"interface: %s\n",dbus_message_get_interface(msg)));
  FXTRACE((80,"sender: %s\n",dbus_message_get_sender(msg)));

  GMPlayerManager * p = static_cast<GMPlayerManager*>(data);
  if (dbus_message_has_path(msg,"/org/freedesktop/NetworkManager")){
    if (dbus_message_is_signal(msg,"org.freedesktop.NetworkManager","StateChanged") ||
        dbus_message_is_signal(msg,"org.freedesktop.NetworkManager","StateChange")) {

      FXuint state=0;
      enum {
/*
        // Network Manager 0.7 / 0.8

        NM7_STATE_UNKNOWN          =  0,
        NM7_STATE_ASLEEP           =  1,
        NM7_STATE_CONNECTING       =  2,
        NM7_STATE_CONNECTED        =  3,
        NM7_STATE_DISCONNECTED     =  4,

        // Network Manager 0.9
        NM9_STATE_UNKNOWN          =  0,
        NM9_STATE_ASLEEP           = 10,
        NM9_STATE_DISCONNECTED     = 20,
        NM9_STATE_DISCONNECTING    = 30,
        NM9_STATE_CONNECTING       = 40,
        NM9_STATE_CONNECTED_LOCAL  = 50,
        NM9_STATE_CONNECTED_SITE   = 60,
        NM9_STATE_CONNECTED_GLOBAL = 70,

*/
        NM7_STATE_CONNECTED        =  3,
        NM9_STATE_CONNECTED_GLOBAL = 70,
        };

      if (dbus_message_get_args(msg,nullptr,DBUS_TYPE_UINT32,&state,DBUS_TYPE_INVALID)) {
        if (p->getAudioScrobbler() && (state==NM7_STATE_CONNECTED ||
                                       state==NM9_STATE_CONNECTED_GLOBAL))
          p->getAudioScrobbler()->nudge();
        }
      return DBUS_HANDLER_RESULT_HANDLED;
      }
    }
  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }
#endif



long GMPlayerManager::onPlayNotify(FXObject*,FXSelector,void*){

  if (!trackinfo.title.empty() && !trackinfo.artist.empty()) {

    display_track_notification();

    if (scrobbler) scrobbler->nowplaying(trackinfo);
    }
  return 0;
  }

long GMPlayerManager::onUpdTrackDisplay(FXObject*,FXSelector,void*){
  //fxmessage("onUpdTrackDisplay\n");
  update_track_display();
  getTrackView()->showCurrent();
  return 0;
  }


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
    else if (cmd=="--raise") cmd_raise();
    else if (cmd=="--update-podcasts") cmd_update_podcasts();
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



GMPlayerManager * GMPlayerManager::myself = nullptr;

GMPlayerManager* GMPlayerManager::instance() {
  return myself;
  }


/// Constructor
GMPlayerManager::GMPlayerManager() {
  FXASSERT(myself==NULL);
  myself=this;
  }


/// Destructor
GMPlayerManager::~GMPlayerManager() {

  /// Remove Signal Handlers
  if (application) {
    application->removeSignal(SIGINT);
    application->removeSignal(SIGQUIT);
    application->removeSignal(SIGTERM);
    application->removeSignal(SIGHUP);
    application->removeSignal(SIGPIPE);
    application->removeSignal(SIGCHLD);
    }

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

  myself=nullptr;

#ifdef HAVE_SESSION
  delete session;
#endif
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
    ap_set_closeonexec(fifo.handle());
    //fcntl(fifo.handle(),F_SETFD,FD_CLOEXEC);
    }
  else {
    FXFile::remove(fifofilename);
    fifofilename=FXString::null;
    }

  return FIFO_STATUS_OWNER;
  }




FXbool GMPlayerManager::init_sources() {

  // Main Database
  database      = new GMTrackDatabase;
  covermanager  = new GMCoverManager;

  // Make sure we can open it.
  if (!init_database(database)) {
    return false;
    }

  /// Create the main database source
  sources.append(new GMDatabaseSource(database));

  /// Load User Queries
  GMFilterSource::init(database,sources);

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

  /// Internet Streams
  sources.append(new GMStreamSource(database));

  /// File System
  sources.append(new GMLocalSource());

  /// Podcast Source
  podcast = new GMPodcastSource(database);
  sources.append(podcast);

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
      queue=nullptr;
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
    source=nullptr;
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
  application->addSignal(SIGINT,mainwindow,GMWindow::ID_QUIT);
  application->addSignal(SIGQUIT,mainwindow,GMWindow::ID_QUIT);
  application->addSignal(SIGTERM,mainwindow,GMWindow::ID_QUIT);
  application->addSignal(SIGHUP,mainwindow,GMWindow::ID_QUIT);
  application->addSignal(SIGPIPE,mainwindow,GMWindow::ID_QUIT);

  /// Create Tooltip Window
  FXToolTip * tooltip = new FXToolTip(application,TOOLTIP_VARIABLE);
  tooltip->create();
  ewmh_change_window_type(tooltip,WINDOWTYPE_TOOLTIP);

  if (database->isEmpty() && wizard) {
    cleanSourceSettings();
    mainwindow->init(SHOW_WIZARD);
    }
  else {
    FXbool start_as_tray=false;
    if (preferences.gui_tray_icon_disabled==false){
      for(FXint i=1;i<argc;i++) {
        if (comparecase(argv[i],"--tray")==0){
          start_as_tray=true;
          preferences.gui_tray_icon=true;
          break;
          }
        }
      }
    if (start_as_tray)
      mainwindow->init(SHOW_TRAY);
    else
      mainwindow->init(SHOW_NORMAL);
    }
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

static FXbool get_cmdline_update_podcasts(int & argc, char ** argv){
  for (FXint i=1;i<argc;i++) {
    if (compare(argv[i],"--update-podcasts")==0)
      return true;
    }
  return false;
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

    /// Copy Settings
    if (FXStat::exists(oldconfig))
      FXFile::copyFiles(oldconfig,newbase+"settings.rc");

    ///FIXME enable this when we release 0.12. Remove the old config directory.
    //FXFile::removeFiles(FXSystem::getHomeDirectory()+PATHSEPSTRING+".goggles",true);
    }
  }



#ifdef HAVE_DBUS
FXbool GMPlayerManager::init_dbus(int & argc,char**argv) {

  sessionbus=new GMDBus();
  systembus =new GMDBus();

  FXASSERT(sessionbus);
  FXASSERT(systembus);

  if (!sessionbus->open(DBUS_BUS_SESSION) || !sessionbus->connected()) {
    FXMessageBox::warning(application,MBOX_OK,"Goggles Music Manager",fxtr("Session bus not available. All features requiring dbus are disabled."));
    delete sessionbus;
    sessionbus=nullptr;
    }
  else {

    mpris2=new GMMediaPlayerService2(sessionbus);
    FXint result = mpris2->create();
    switch(result) {
      case DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER:
        // success
        break;
      case DBUS_REQUEST_NAME_REPLY_EXISTS:
        if (argc>1 && strlen(argv[1])>0)
          mpris2->request(argv[1]);
        delete mpris2;
        return false;
        break;
      default:
        FXMessageBox::warning(application,MBOX_OK,"Goggles Music Manager",fxtr("Session Bus not available. All features requiring sessionbus are disabled."));
        delete mpris2;
        mpris2=nullptr;
        delete sessionbus;
        sessionbus=nullptr;
        return true;
        break;
      }

    if (!systembus->open(DBUS_BUS_SYSTEM) || !systembus->connected()) {
      delete systembus;
      systembus=nullptr;
      }

    if (systembus && !dbus_connection_add_filter(systembus->connection(),dbus_systembus_filter,this,nullptr)){
      delete systembus;
      systembus=nullptr;
      }

    if (systembus) {
      DBusError error;
      dbus_error_init(&error);
      dbus_bus_add_match(systembus->connection(),"type='signal',path='/org/freedesktop/NetworkManager',interface='org.freedesktop.NetworkManager',member='StateChanged'",&error);
      if (dbus_error_is_set(&error)) {
        dbus_error_free(&error);
        delete systembus;
        systembus=nullptr;
        }
      }

    if (systembus) {
      DBusError error;
      dbus_error_init(&error);
      dbus_bus_add_match(systembus->connection(),"type='signal',path='/org/freedesktop/NetworkManager',interface='org.freedesktop.NetworkManager',member='StateChange'",&error);
      if (dbus_error_is_set(&error)) {
        dbus_error_free(&error);
        delete systembus;
        systembus=nullptr;
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

  // Initialize Crypto Support
  if (!ap_init_crypto()) {
    fxwarning("gogglesmm: failed to initialize ssl support\n");
    return 1;
    }

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
  if (sessionbus==nullptr) {
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
  player = new GMAudioPlayer(application,this,ID_AUDIO_PLAYER);

  /// Open Audio Device if needed.
  player->init();
  player->loadSettings();

  /// FIXME move to player->loadSettings
  switch(preferences.play_replaygain){
    case 0: player->setReplayGain(ReplayGainOff); break;
    case 1: player->setReplayGain(ReplayGainTrack); break;
    case 2: player->setReplayGain(ReplayGainAlbum); break;
    }

  if (preferences.play_crossfade)
    player->setCrossFade(preferences.play_crossfade_duration);


  /// Receive events from fifo
  if (fifo.isOpen()) {
    application->addInput(this,GMPlayerManager::ID_DDE_MESSAGE,fifo.handle(),INPUT_READ);
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

  /// Start Services
  scrobbler    = new GMAudioScrobbler(this,ID_SCROBBLER);
#ifdef HAVE_DBUS
  if (sessionbus) {
    notifydaemon = new GMNotifyDaemon(sessionbus);

    // KDE5 comes with mpris plugin on the toolbar, no need for
    // tray icon
    if (gm_desktop_session()==DESKTOP_SESSION_KDE_PLASMA) {
      preferences.gui_tray_icon_disabled=true;
      }

    /// Grab Media Player Keys
    gsd = new GMSettingsDaemon(sessionbus,this,ID_GNOME_SETTINGS_DAEMON);
    gsd->GrabMediaPlayerKeys("gogglesmm");

    update_mpris();

    notifydaemon->init();
    }
#endif

#ifdef HAVE_SESSION
  /// Connect to the session manager
  session = new GMSession(application,this,GMPlayerManager::ID_SESSION_MANAGER);
  session->init(argc,argv);
#endif

  /// Open url from command line
  if (!url.empty())
    open(url);

#ifndef HAVE_DBUS
  update_tray_icon();
#endif

  // Update Podcasts on startup if desired.
  if (get_cmdline_update_podcasts(argc,argv))
    cmd_update_podcasts();


  /// Run the application
  return application->run();
  }


void GMPlayerManager::exit() {

  player->saveSettings();
  player->stop();
  player->exit();

  /// Save settings
  for (FXint i=0;i<sources.no();i++)
    sources[i]->save(application->reg());

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

    gsd->ReleaseMediaPlayerKeys("gogglesmm");
    delete gsd;
    gsd=nullptr;

    if (notifydaemon) {
      notifydaemon->close();
      delete notifydaemon;
      notifydaemon=nullptr;
      }

    if (mpris1) delete mpris1;
    if (mpris2) delete mpris2;
    }
  if (systembus) {
    dbus_connection_remove_filter(systembus->connection(),dbus_systembus_filter,this);
    }
#endif

  // Destroy shared datastructures between playlists
  getDatabaseSource()->shutdown();

  /// Delete Sources
  for (FXint i=0;i<sources.no();i++)
    delete sources[i];

  delete covermanager;

  ap_free_crypto();

  application->exit(0);
  }

#ifdef HAVE_DBUS
void GMPlayerManager::update_mpris() {
  if (mpris1) {
    if (!sessionbus || !preferences.dbus_mpris1){
      delete mpris1;
      mpris1=nullptr;
      }
    }

  if (!mpris1 && preferences.dbus_mpris1 && sessionbus)
    mpris1=new GMMediaPlayerService1(sessionbus);
  }
#endif



void GMPlayerManager::update_tray_icon() {
  if (trayicon){
    if (!preferences.gui_tray_icon || preferences.gui_tray_icon_disabled) {
      delete trayicon;
      trayicon=nullptr;
      }
    }
  else {
    if (preferences.gui_tray_icon && !preferences.gui_tray_icon_disabled) {
      trayicon = new GMTrayIcon(application);
      trayicon->create();
      }
    }
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

FXbool GMPlayerManager::hasSourceWithKey(const FXString & key) const{
  for (FXint i=0;i<sources.no();i++){
    if (key==sources[i]->settingKey())
      return true;
    }
  return false;
  }


void GMPlayerManager::cleanSourceSettings() {
  FXint s;
  FXStringList keys;

  for (s=0;s<application->reg().no();s++){
    if (!application->reg().empty(s) && comparecase(application->reg().key(s),"database",8)==0){
      if (!hasSourceWithKey(application->reg().key(s))) {
        keys.append(application->reg().key(s));
        }
      }
    }

  for (s=0;s<keys.no();s++){
    application->reg().deleteSection(keys[s]);
    }
  }

void GMPlayerManager::removePlayListSources(){
  for (FXint i=sources.no()-1;i>=0;i--){
    if (sources[i]->getType()==SOURCE_DATABASE_PLAYLIST) {
      FXApp::instance()->reg().deleteSection(sources[i]->settingKey().text());
      GMSource * src = sources[i];
      sources.erase(i);
      delete src;
      }
    }
  }


void GMPlayerManager::open(const FXString & url) {
  FXint id;

  if (source) {
    application->removeTimeout(source,GMSource::ID_TRACK_PLAYED);
    source->resetCurrent();
    source=nullptr;
    }

  trackinfo.url = url;
  if (gm_is_local_file(url) && sources[0]->hasTrack(url,id)) {
    source       = sources[0];
    trackinfoset = source->getTrack(trackinfo);
    sources[0]->setCurrentTrack(id);
    }
  else {
    trackinfoset=false;
    getTrackView()->setActive(-1);
    }

  player->open(url,true);
  }



void GMPlayerManager::playItem(FXuint whence) {
  FXint track=-1;

  // Any scheduled stops should be cancelled
  scheduled_stop = false;

  /// Remove Current Timeout
  if (source) {
    application->removeTimeout(source,GMSource::ID_TRACK_PLAYED);
    source->resetCurrent();
    source=nullptr;
    }

  if (queue) {
    switch(whence) {
      case TRACK_CURRENT :  track = queue->getCurrent();
                            if (track==-1 && queue->canPlaySource(getTrackView()->getSource())) {
                              track = getTrackView()->getCurrent();
                              }
                           break;
      case TRACK_NEXT    : track = queue->getNext(); break;
      default            : FXASSERT(0); break;
      };
    if (track!=-1) {
      source = queue;
      }
    }
  else {
    if (track==-1) {
      switch(whence) {
        case TRACK_CURRENT : track = getTrackView()->getCurrent(); break;
        case TRACK_NEXT    : track = getTrackView()->getNext(true); break;
        case TRACK_PREVIOUS: track = getTrackView()->getPrevious(); break;
        default            : FXASSERT(0); break;
        };
      if (track!=-1) {
        source = getTrackView()->getSource();
        getTrackView()->setActive(track);
        }
      }
    }

  if (source) {
    trackinfoset = source->getTrack(trackinfo);
    player->open(trackinfo.url,true);
    }
  else {
    player->stop();
    }
  }




void GMPlayerManager::stop(FXbool /*force_close*/) {

  // Any scheduled stops should be cancelled
  scheduled_stop = false;

  /// Reset Source
  if (source) {
    application->removeTimeout(source,GMSource::ID_TRACK_PLAYED);
    source->resetCurrent();
    source=nullptr;
    }

  player->stop();

  // We don't call reset_track_display() here, since we'll do that when we get the
  // PLAYER_STATE_STOPPED signal from the audio player.
  }



void GMPlayerManager::seekTime(FXint time) {
  application->removeTimeout(source,GMSource::ID_TRACK_PLAYED);
  player->setPosition(time);
  has_seeked=true;
  }

void GMPlayerManager::seek(FXdouble pos) {
  application->removeTimeout(source,GMSource::ID_TRACK_PLAYED);
  player->seek(pos);
  has_seeked=true;
  }


void GMPlayerManager::pause() {

  // Any scheduled stops should be cancelled
  scheduled_stop = false;

  player->pause();

  if (application->hasTimeout(source,GMSource::ID_TRACK_PLAYED)) {
    count_track_remaining = application->remainingTimeout(source,GMSource::ID_TRACK_PLAYED);
    application->removeTimeout(source,GMSource::ID_TRACK_PLAYED);
    }
  else {
    count_track_remaining=0;
    }
  }

void GMPlayerManager::unpause() {
  player->pause();
 }

FXbool GMPlayerManager::playlist_empty() {
  if (getTrackView()->hasTracks()) return false;
  if (preferences.play_repeat!=REPEAT_OFF) return false;
  if (getTrackView()->getNext()!=-1) return false;
  return true;
  }

void GMPlayerManager::notify_playback_finished() {
  /*
    The current track is still playing (and about to be finished) so don't call reset_track_display if
    there is nothing to play anymore. It will eventually be called by the PLAYER_STATE_STOPPED signal.
  */
  FXString errormsg;
  FXString filename;
  FXint track=-1;

  // Check whether playback should be stopped and reset flag
  FXbool stop_playback = scheduled_stop;
  scheduled_stop=false;

  if (queue) {

    /// Reset Source
    if (source!=queue) {
     source->resetCurrent();
     source=nullptr;
     }

    /// Nothing else to do, mark current as played
    if (stop_playback) {
      queue->getNext();
      return;
      }

    //FIXME handle stop_playback
    track = queue->getNext();
    if (track==-1) {
      source = nullptr;

      if (getTrackView()->getSource()==queue)
        getTrackView()->refresh();

     //reset_track_display();
     return;
     }
    else {
      source = queue;
      }
    trackinfoset = queue->getTrack(trackinfo);
    }
  else {

    /// Don't play anything if we didn't play anything from the library
    if (source==nullptr)
      return;

    /// Can we just start playback without user interaction
    if (!getTrackView()->getSource()->autoPlay() || stop_playback) {

      /// Reset Source
      if (source) {
        source->resetCurrent();
        source=nullptr;
        }

      if(preferences.play_repeat!=REPEAT_TRACK) {
        track = getTrackView()->getNext();
        if (track!=-1) getTrackView()->setCurrent(track);
        }
      return;
      }

    if (source) {
      source->resetCurrent();
      source=nullptr;
      }

    if (preferences.play_repeat==REPEAT_TRACK)
      track = getTrackView()->getActive();
    else
      track = getTrackView()->getNext();

    if (track==-1) {
      //reset_track_display();
      return;
      }

    // FIXME only does source->markCurrent
    getTrackView()->setActive(track,false);

    source = getTrackView()->getSource();
    trackinfoset = source->getTrack(trackinfo);
    }
  player->open(trackinfo.url,false);
  }

FXbool GMPlayerManager::playing() const {
  return player->playing() ;
  }


void GMPlayerManager::reset_track_display() {
  FXTRACE((51,"GMPlayerManager::reset_track_display()\n"));

  /// Reset the cover
  covermanager->clear();

  /// Reset Main Window
  mainwindow->reset();

  if (trayicon) trayicon->reset();

  /// Reset Active Track
  getTrackView()->setActive(-1);

  /// Remove Notify
  application->removeTimeout(this,ID_PLAY_NOTIFY);

#ifdef HAVE_DBUS
  if (notifydaemon && preferences.dbus_notify_daemon)
    notifydaemon->reset();
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
  if (playing()) {

    if (preferences.gui_show_playing_albumcover && covermanager->getCover()==nullptr) {

      covermanager->load(trackinfo.url);

      mainwindow->update_meta_display();

      mainwindow->update_cover_display();

      }

    else if (preferences.gui_show_playing_albumcover==false && covermanager->getCover()) {

      covermanager->clear();

      mainwindow->update_meta_display();

      mainwindow->update_cover_display();

      }

    else {
      mainwindow->update_meta_display();
      }
    }
  else if (covermanager->getCover()) {

    covermanager->clear();

    mainwindow->update_meta_display();

    mainwindow->update_cover_display();
    }
  }

void GMPlayerManager::update_track_display(FXbool notify) {
  FXTRACE((51,"GMPlayerManager::update_track_display()\n"));

  /// If track information is not set, we need to get the latest from the player.
  if (source) {
    FXint time = (FXint) (((double)trackinfo.time) * 0.80);
    if (time <= 5) {
      application->removeTimeout(source,GMSource::ID_TRACK_PLAYED);
      source->handle(this,FXSEL(SEL_TIMEOUT,GMSource::ID_TRACK_PLAYED),nullptr);
      }
    else {
      count_track_remaining=0;
      application->addTimeout(source,GMSource::ID_TRACK_PLAYED,TIME_SEC(time));
      }
    }

  if (preferences.gui_show_playing_albumcover)
    covermanager->load(trackinfo.url);

  mainwindow->display(trackinfo);

  if (notify) application->addTimeout(this,ID_PLAY_NOTIFY,500_ms);

  if (queue) {
    getSourceView()->refresh(queue);
    if (getTrackView()->getSource()==queue)
      getTrackView()->refresh();
    getTrackView()->showCurrent();
    }
  }


FXint GMPlayerManager::volume() const{
  return player->getVolume();
  }

void GMPlayerManager::volume(FXint l) {
  player->volume((FXfloat)l/100.0f);
  }

FXbool GMPlayerManager::can_stop() const {
  return (player->playing() || player->pausing());
  }

FXbool GMPlayerManager::can_play() const {
   return !player->playing() && ((queue==nullptr && getTrackView()->hasTracks()) ||
                                (queue && (queue->getNumTracks() ||  (getTrackView()->hasTracks() && getPlayQueue()->canPlaySource(getTrackView()->getSource())))));
  }

FXbool GMPlayerManager::can_pause() const {
  return (player->playing() && !player->pausing());
  }

FXbool GMPlayerManager::can_unpause() const {
  return player->pausing();
  }

FXbool GMPlayerManager::can_next() const {
  if (player->playing() && !player->pausing()) {
    if (( queue && queue->getNumTracks()>0) || getTrackView()->getNumTracks()>1)
      return true;
    }
  return false;
  }

FXbool GMPlayerManager::can_prev() const {
  if (player->playing() && !player->pausing() && queue==nullptr) {
    return (getTrackView()->getNumTracks()>1);
    }
  return false;
  }

void GMPlayerManager::setSleepTimer(FXlong ns) {
  if (ns==0)
    application->removeTimeout(this,GMPlayerManager::ID_SLEEP_TIMER);
  else
    application->addTimeout(this,GMPlayerManager::ID_SLEEP_TIMER,ns);
  }

FXbool GMPlayerManager::hasSleepTimer() {
  return application->hasTimeout(this,GMPlayerManager::ID_SLEEP_TIMER);
  }

void GMPlayerManager::show_message(const FXchar * title,const FXchar * msg){
  if (application->getActiveWindow() && application->getActiveWindow()->shown()) {
    FXMessageBox::error(application->getActiveWindow(),MBOX_OK,title,"%s",msg);
    }
  else {
    if (mainwindow) {
      if (mainwindow->shown())
        FXMessageBox::error(mainwindow,MBOX_OK,title,"%s",msg);
      else if (mainwindow->getRemote())
        FXMessageBox::error(mainwindow->getRemote(),MBOX_OK,title,"%s",msg);
      else
        FXMessageBox::error(application,MBOX_OK,title,"%s",msg);
      }
    else {
      FXMessageBox::error(application,MBOX_OK,title,"%s",msg);
      }
    }
  }


long GMPlayerManager::onCmdCloseWindow(FXObject*sender,FXSelector,void*){
  FXWindow * window = static_cast<FXWindow*>(sender);
  if (getPreferences().gui_hide_player_when_close && !getPreferences().gui_tray_icon_disabled) {
    window->hide();
    }
  else {
    getMainWindow()->handle(this,FXSEL(SEL_COMMAND,GMWindow::ID_QUIT),nullptr);
    }
  return 1;
  }

long GMPlayerManager::onCmdQuit(FXObject*,FXSelector,void*){
  getMainWindow()->handle(this,FXSEL(SEL_COMMAND,GMWindow::ID_QUIT),nullptr);
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



long GMPlayerManager::onImportTaskCompleted(FXObject*,FXSelector sel,void*ptr){
  GMTask * task = *static_cast<GMTask**>(ptr);
  if (FXSELTYPE(sel)==SEL_TASK_COMPLETED) {
    database->initArtistLookup();
    getDatabaseSource()->updateCovers();
    GMSource * src = getTrackView()->getSource();
    if (src) {
      switch(src->getType()){
        case SOURCE_DATABASE:
        case SOURCE_DATABASE_FILTER:
        case SOURCE_DATABASE_PLAYLIST: getTrackView()->refresh(); break;
        default: break;
        }
      }
    }
  delete task;
  return 0;
  }



void GMPlayerManager::runTask(GMTask * task) {
  application->removeTimeout(this,GMPlayerManager::ID_TASKMANAGER_SHUTDOWN);
  taskmanager->run(task);
  }

long GMPlayerManager::onTaskManagerIdle(FXObject*,FXSelector,void*){
  mainwindow->setStatus(FXString::null);
  GM_DEBUG_PRINT("Schedule taskmanager shutdown in 30s\n");
  application->addTimeout(this,GMPlayerManager::ID_TASKMANAGER_SHUTDOWN,30_s);
  return 0;
  }

long GMPlayerManager::onTaskManagerRunning(FXObject*,FXSelector,void*){
  GM_DEBUG_PRINT("Taskmanager running\n");
  application->removeTimeout(this,GMPlayerManager::ID_TASKMANAGER_SHUTDOWN);
  return 0;
  }


long GMPlayerManager::onTaskManagerShutdown(FXObject*,FXSelector,void*){
  GM_DEBUG_PRINT("Shutdown taskmanager now\n");
  taskmanager->shutdown();
  return 0;
  }


long GMPlayerManager::onTaskManagerStatus(FXObject*,FXSelector,void*ptr){
  FXchar * msg = (FXchar*)ptr;
  mainwindow->setStatus(msg);
  return 0;
  }

long GMPlayerManager::onCancelTask(FXObject*,FXSelector,void*){
  taskmanager->cancelTask();
  return 0;
  }




void GMPlayerManager::cmd_play(){
  if (can_unpause())
    unpause();
  else if (can_play())
    playItem(TRACK_CURRENT);
  }


void GMPlayerManager::cmd_playpause(){
  if (can_pause())
    pause();
  else if (can_unpause())
    unpause();
  else if (can_play())
    playItem(TRACK_CURRENT);
  }


void GMPlayerManager::cmd_pause(){
  if (can_pause())
    pause();
  }

void GMPlayerManager::cmd_schedule_stop(){
  if (scheduled_stop==false) {
    if (can_stop()) {
      GM_DEBUG_PRINT("enable scheduled stop\n");
      scheduled_stop=true;
      }
    }
  else {
    GM_DEBUG_PRINT("disable scheduled stop\n");
    scheduled_stop=false;
    }
  }

FXbool GMPlayerManager::has_scheduled_stop() const {
  if (can_stop() && scheduled_stop)
    return true;
  else
    return false;
  }

void GMPlayerManager::cmd_stop(){
  if (can_stop())
    stop();
  }

void GMPlayerManager::cmd_next(){
  if (can_next())
    playItem(TRACK_NEXT);
  }
void GMPlayerManager::cmd_prev(){
  if (can_prev())
    playItem(TRACK_PREVIOUS);
  }

void GMPlayerManager::cmd_raise() {
  getMainWindow()->raiseWindow();
  }

void GMPlayerManager::cmd_toggle_shown(){
  getMainWindow()->toggleShown();
  }

void GMPlayerManager::cmd_update_podcasts(){
  FXASSERT(podcast);
  podcast->refreshFeeds();
  }



void GMPlayerManager::display_track_notification() {
#ifdef DEBUG
  fxmessage("Track Change Notification\n");
  fxmessage("\t Title: %s\n",trackinfo.title.text());
  fxmessage("\tArtist: %s\n",trackinfo.artist.text());
  fxmessage("\t Album: %s\n",trackinfo.album.text());
#endif
#ifdef HAVE_DBUS
  if (sessionbus) {
    if (preferences.dbus_notify_daemon && notifydaemon) {
      notifydaemon->notify_track_change(trackinfo);
      }
    if (mpris1) mpris1->notify_track_change(trackinfo);
    if (mpris2) mpris2->notify_track_change(trackinfo);
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
  fxwarning("gogglesmm X Error: code %d major %d minor %d: %s.\n",eev->error_code,eev->request_code,eev->minor_code,buf);
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
  GM_DEBUG_PRINT("[player] bos\n");
  update_track_display();
  getTrackView()->showCurrent();
  return 1;
  }

long GMPlayerManager::onPlayerEOS(FXObject*,FXSelector,void*){
  GM_DEBUG_PRINT("[player] eos\n");
  notify_playback_finished();
  return 1;
  }


long GMPlayerManager::onPlayerTime(FXObject*,FXSelector,void* ptr){
  const PlaybackTime * tm = static_cast<const PlaybackTime*>(ptr);

  TrackTime tm_progress;
  TrackTime tm_remaining;

  FXint time,pos=0;

  /// Time progressed
  time                = tm->position;
  tm_progress.hours   = (FXint) floor((double)time/3600.0);
  time               -= (3600*tm_progress.hours);
  tm_progress.minutes = (FXint) floor((double)time/60.0);
  time               -= (FXint) (60*tm_progress.minutes);
  tm_progress.seconds = (FXint) floor((double)time);


  /// Time Remaining
  if (tm->length) {
    time                 = (tm->length-tm->position);
    tm_remaining.hours   = (FXint) floor((double)time/3600.0);
    time                -= (3600*tm_remaining.hours);
    tm_remaining.minutes = (FXint) floor((double)time/60.0);
    time                -= (FXint) (60*tm_remaining.minutes);
    tm_remaining.seconds = (FXint) floor((double)time);

    pos = (FXint)(100000.0 * ( (double)tm->position / (double)tm->length));
    }

  mainwindow->update_time(tm_progress,tm_remaining,pos,true,true);

#ifdef HAVE_DBUS
  if (has_seeked) {
    if (mpris2) mpris2->notify_seek(tm->position);
    has_seeked=false;
    }
#endif
  return 1;
  }

long GMPlayerManager::onPlayerState(FXObject*,FXSelector,void* ptr){
  FXint state = (FXint)(FXival)ptr;
  switch(state){
    case PLAYER_STOPPED: GM_DEBUG_PRINT("[player] stopped\n"); reset_track_display(); break;
    case PLAYER_PLAYING: break;
    case PLAYER_PAUSING: break;
    }
#ifdef HAVE_DBUS
  if (mpris1) mpris1->notify_status_change();
  if (mpris2) mpris2->notify_status_change();
#endif
  return 1;
  }

long GMPlayerManager::onPlayerVolume(FXObject*,FXSelector,void* ptr){
  getMainWindow()->update_volume_display((FXint)(FXival)ptr);
#ifdef HAVE_DBUS
  if (mpris2) mpris2->notify_volume((FXint)(FXival)ptr);
#endif
  return 1;
  }

long GMPlayerManager::onPlayerMeta(FXObject*,FXSelector,void* ptr){
  GMTrack * track = static_cast<GMTrack*>(ptr);
  GM_DEBUG_PRINT("[player] meta\n");
  if (trackinfoset==false) {
    trackinfo.title.adopt(track->title);
    trackinfo.artist.adopt(track->artist);
    trackinfo.album.adopt(track->album);
    update_track_display();
    }
  return 1;
  }


long GMPlayerManager::onPlayerError(FXObject*,FXSelector,void*ptr){
  FXString * msg = (FXString*)ptr;
  show_message(fxtr("Playback Error"),msg->text());
  return 1;
  }


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


