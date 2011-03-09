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
#ifndef GMPLAYERMANAGER_H
#define GMPLAYERMANAGER_H

#ifndef GMPREFERENCES_H
#include "GMPreferences.h"
#endif

#ifndef GMSOURCE_H
#include "GMSource.h"
#endif

class GMPlayList;
class GMTrackList;
class GMTrackView;
#ifdef HAVE_XINE_LIB
class GMPlayer;
#else
class GMAudioPlayer;
#endif
class GMWindow;
class GMRemote;
class GMTrackDatabase;
class GMPlayer;
class GMPlayList;
class GMSource;
class GMSourceView;
class GMFetch;
class GMEQDialog;
#ifdef HAVE_DBUS
class GMNotifyDaemon;
class GMAppStatusNotify;
class GMSettingsDaemon;
class GMDBus;
class GMMediaPlayerService;
#endif
class GMAudioScrobbler;
class GMTrayIcon;
class GMPlayQueue;
class GMDatabaseSource;
class GMCoverThumbs;
class GMTaskManager;
class GMTask;

struct lirc_config;

/*

possible plugin

class GMPlugin {
public:
  void notify_track_changed(const GMTrack &);

  void notify_status_change();

  void notify_caps_change();
  };

*/

class GMPlayerManager : public FXObject {
FXDECLARE(GMPlayerManager)
private:
  static GMPlayerManager * myself;
protected:
  GMPreferences preferences;
  GMSourceList  sources;
  FXString      fifofilename;
  FXFile        fifo;
#if FOXVERSION < FXVERSION(1,7,0)
  FXuint        count_track_remaining;
#else
  FXlong        count_track_remaining;
#endif
  GMTaskManager        * taskmanager;
protected:

#ifdef HAVE_DBUS
  GMDBus               * sessionbus;
  GMDBus               * systembus;

  GMNotifyDaemon       * notifydaemon;
  GMAppStatusNotify    * appstatus;
  GMSettingsDaemon     * gsd;
  GMMediaPlayerService * mpris;
#endif
  FXApp 	 				     * application;
  GMWindow 				     * mainwindow;
#ifdef HAVE_XINE_LIB
  GMPlayer    		     * player;
#else
  GMAudioPlayer        * player;
#endif
  GMTrayIcon           * trayicon;
  GMAudioScrobbler     * scrobbler;
#ifdef HAVE_LIRC
  FXint                  lirc_fd;
  struct lirc_config*    lirc_config;
#endif
protected:
  GMPlayQueue          * queue;
  GMSource             * source;
  GMTrackDatabase      * database;
  GMCoverThumbs        * thumbs;
  GMTrack    trackinfo;
  FXbool     trackinfoset;
protected:
  FXbool hasSourceWithKey(const char * key) const;
  void cleanSourceSettings();
public:
  static GMPlayerManager * instance();
public:
  enum {
    ID_UPDATE_TRACK_DISPLAY = 1,
    ID_HANDLE_EVENTS,
    ID_COUNT_TRACK,
    ID_SLEEP_TIMER,
    ID_SCAN_AUDIOCD,
    ID_DDE_MESSAGE,
    ID_DOWNLOAD_COMPLETE,
    ID_PLAY_NOTIFY,
    ID_WINDOW,
    ID_EQUALIZER,
    ID_SCROBBLER,
    ID_PLAYER_ERROR,
#ifdef HAVE_LIRC
    ID_LIRC,
#endif
#ifdef HAVE_DBUS
    ID_GNOME_SETTINGS_DAEMON,
#endif
    ID_AUDIO_PLAYER,
    ID_IMPORT_TASK,
    ID_CANCEL_TASK,
    ID_TASKMANAGER,
    ID_TASKMANAGER_SHUTDOWN,
    ID_CHILD
    };
public:
  long onUpdTrackDisplay(FXObject*,FXSelector,void*);
#ifdef HAVE_XINE_LIB
  long onUpdEvents(FXObject*,FXSelector,void*);
#endif
  long onCmdCountTrack(FXObject*,FXSelector,void*);
  long onCmdSleepTimer(FXObject*,FXSelector,void*);
  long onDDEMessage(FXObject*,FXSelector,void*);
  long onCmdDownloadComplete(FXObject*,FXSelector,void*);
  long onCmdCloseRemote(FXObject*,FXSelector,void*);
  long onCmdCloseWindow(FXObject*,FXSelector,void*);
  long onPlayNotify(FXObject*,FXSelector,void*);
  long onCmdChild(FXObject*,FXSelector,void*);
  long onCmdEqualizer(FXObject*,FXSelector,void*);
  long onScrobblerError(FXObject*,FXSelector,void*);
  long onScrobblerOpen(FXObject*,FXSelector,void*);

  long onPlayerError(FXObject*,FXSelector,void*);

  long onImportTaskCompleted(FXObject*,FXSelector,void*);
  long onTaskManagerRunning(FXObject*,FXSelector,void*);
  long onTaskManagerStatus(FXObject*,FXSelector,void*);
  long onTaskManagerIdle(FXObject*,FXSelector,void*);
  long onTaskManagerShutdown(FXObject*,FXSelector,void*);
  long onCancelTask(FXObject*,FXSelector,void*);
#ifdef HAVE_LIRC
  long onCmdLirc(FXObject*,FXSelector,void*);
#endif
#ifdef HAVE_DBUS
  long onCmdSettingsDaemon(FXObject*,FXSelector,void*);
#endif

//#ifndef HAVE_XINE_LIB
  long onPlayerBOS(FXObject*,FXSelector,void*);
  long onPlayerEOS(FXObject*,FXSelector,void*);
  long onPlayerTime(FXObject*,FXSelector,void*);
  long onPlayerState(FXObject*,FXSelector,void*);
//#endif
protected:
  FXint  init_fifo(int & argc,char**argv);
  FXbool init_database(GMTrackDatabase *);
  FXbool init_sources();
  void   init_window(FXbool wizard);
  void   init_configuration();
#ifdef HAVE_DBUS
  FXbool init_dbus(int & argc,char**argv);
#endif
#ifdef HAVE_LIRC
  void   init_lirc();
#endif
public:
  GMPlayerManager();

  FXint run(int & argc,char**argv);

  FXString getDatabaseFilename() const;


  /// Return the track database.
  GMTrackDatabase * getTrackDatabase() const { return database; }

  GMDatabaseSource * getDatabaseSource() const;

  GMCoverThumbs * getCoverThumbs() const { return thumbs; }



  /// Change Source
  void setSource(FXuint source);


  void update_tray_icon();


  FXint getNumSources() const { return sources.no(); }

  GMSource * getSource(FXint i) const { return sources[i]; }

  void removeSource(GMSource * src);

  void insertSource(GMSource * src) { sources.append(src); }

  void removePlayListSources();

  GMSource * getSource() const { return source; }

  GMPlayQueue * getPlayQueue() const { return queue; }

  GMWindow * getMainWindow() const { return mainwindow; }


  GMTrackView * getTrackView() const;

  GMSourceView * getSourceView() const;

  FXuint getMainWindowId() const;

#ifdef HAVE_XINE_LIB
  GMPlayer * getPlayer() const { return player; }
#else
  GMAudioPlayer * getPlayer() const { return player; }
#endif

  GMPreferences & getPreferences() { return preferences; }

  GMAudioScrobbler * getAudioScrobbler() { return scrobbler; }

  GMTrayIcon * getTrayIcon() { return trayicon; }

#ifdef HAVE_DBUS
  GMNotifyDaemon * getNotify() { return notifydaemon; }

  FXbool hasSessionBus() const { return (sessionbus!=NULL) ; }

  void update_mpris();
#endif


  /// Run a background task
  void runTask(GMTask * task);


  void getTrackInformation(GMTrack & t) { t=trackinfo; }

  void exit();

  FXbool can_play() const;

  FXbool can_pause() const;

  FXbool can_unpause() const;

  FXbool can_next() const;

  FXbool can_prev() const;

  FXbool can_stop() const;

  void play();

  FXbool play(const FXString & mrl,FXbool flush=true);
  FXbool play(const FXStringList & mrl);

  void download(const FXString & mrl);

  void open(const FXString & mrl);


  void pause();

  void unpause();

  void stop(FXbool closedevice=false);

  void next();

  void prev();

  void volume(FXint l);
  FXint volume() const;

  void seek(FXdouble pos);

  FXbool playlist_empty();

  void notify_playback_finished();

  void reset_track_display();

  void update_cover_display();

  void update_track_display(FXbool notify=true);

  void update_time_display();

  void update_replay_gain();

  void update_album_covers();

  void display_track_notification();

  FXint current_position() const;

  FXbool playing() const;

  FXbool audio_device_opened() const;

  FXint get_prev() const;

  FXint get_next() const;

  /// Set Sleep Timer - 0 turns the timer off
#if FOXVERSION < FXVERSION(1,7,0)
  void setSleepTimer(FXuint ms);
#else
  void setSleepTimer(FXlong ns);
#endif

  FXbool hasSleepTimer();

  void handle_async_events();

  void show_message(const FXchar * title,const FXchar * msg);

  void setStatus(const FXString & msg);

  void register_global_hotkeys();

  FXbool handle_global_hotkeys(FXuint code);


  void cmd_play();
  void cmd_playpause();
  void cmd_pause();
  void cmd_stop();
  void cmd_next();
  void cmd_prev();
  void cmd_toggle_shown();



  void setPlayQueue(FXbool enable);




  FXint createPlaylist(const FXString & name);





  ~GMPlayerManager();
  };

#endif
