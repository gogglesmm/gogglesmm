/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2006-2014 by Sander Jansen. All Rights Reserved      *
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
class GMAudioPlayer;
class GMWindow;
class GMTrackDatabase;
class GMPlayer;
class GMPlayList;
class GMSource;
class GMSourceView;
#ifdef HAVE_DBUS
class GMNotifyDaemon;
class GMAppStatusNotify;
class GMSettingsDaemon;
class GMDBus;
class GMMediaPlayerService1;
class GMMediaPlayerService2;
#endif
class GMAudioScrobbler;
class GMTrayIcon;
class GMPlayQueue;
class GMDatabaseSource;
class GMCoverCache;
class GMCoverManager;
class GMTaskManager;
class GMTask;
class GMSession;

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


enum {
  TRACK_CURRENT = 0,
  TRACK_NEXT,
  TRACK_PREVIOUS
  };


struct TrackTime {
  FXint hours;
  FXint minutes;
  FXint seconds;
  };


class GMPlayerManager : public FXObject {
FXDECLARE(GMPlayerManager)
private:
  static GMPlayerManager * myself;
protected:
  GMPreferences preferences;
  GMSourceList  sources;
  FXString      fifofilename;
  FXFile        fifo;
  FXlong        count_track_remaining;
  FXbool        scheduled_stop;
  GMTaskManager        * taskmanager;
protected:

#ifdef HAVE_DBUS
  GMDBus               * sessionbus;
  GMDBus               * systembus;
  GMNotifyDaemon       * notifydaemon;
  GMAppStatusNotify    * appstatus;
  GMSettingsDaemon     * gsd;
  GMMediaPlayerService1* mpris1;
  GMMediaPlayerService2* mpris2;
#endif
  FXApp 	 				     * application;
  GMSession            * session;
  GMWindow 				     * mainwindow;
  GMAudioPlayer        * player;
  GMTrayIcon           * trayicon;
  GMAudioScrobbler     * scrobbler;
#ifdef HAVE_LIRC
  FXint                  lirc_fd;
  struct lirc_config*    lirc_config;
#endif
protected:
#ifdef HAVE_PLAYQUEUE
  GMPlayQueue          * queue;
#endif
  GMSource             * source;
  GMTrackDatabase      * database;
  GMCoverCache         * covercache;
  GMCoverManager       * covermanager;
  GMTrack    trackinfo;
  FXbool     trackinfoset;
protected:
  FXbool hasSourceWithKey(const FXString & key) const;
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
    ID_SESSION_MANAGER,
    ID_CHILD
    };
public:
  long onUpdTrackDisplay(FXObject*,FXSelector,void*);
  long onCmdCountTrack(FXObject*,FXSelector,void*);
  long onCmdSleepTimer(FXObject*,FXSelector,void*);
  long onDDEMessage(FXObject*,FXSelector,void*);
  long onCmdCloseRemote(FXObject*,FXSelector,void*);
  long onCmdCloseWindow(FXObject*,FXSelector,void*);
  long onPlayNotify(FXObject*,FXSelector,void*);
  long onCmdChild(FXObject*,FXSelector,void*);
  long onScrobblerError(FXObject*,FXSelector,void*);
  long onScrobblerOpen(FXObject*,FXSelector,void*);


  long onImportTaskCompleted(FXObject*,FXSelector,void*);
  long onTaskManagerRunning(FXObject*,FXSelector,void*);
  long onTaskManagerStatus(FXObject*,FXSelector,void*);
  long onTaskManagerIdle(FXObject*,FXSelector,void*);
  long onTaskManagerShutdown(FXObject*,FXSelector,void*);
  long onCancelTask(FXObject*,FXSelector,void*);
  long onCmdQuit(FXObject*,FXSelector,void*);
#ifdef HAVE_LIRC
  long onCmdLirc(FXObject*,FXSelector,void*);
#endif
#ifdef HAVE_DBUS
  long onCmdSettingsDaemon(FXObject*,FXSelector,void*);
#endif

  long onPlayerBOS(FXObject*,FXSelector,void*);
  long onPlayerEOS(FXObject*,FXSelector,void*);
  long onPlayerTime(FXObject*,FXSelector,void*);
  long onPlayerState(FXObject*,FXSelector,void*);
  long onPlayerMeta(FXObject*,FXSelector,void*);
  long onPlayerError(FXObject*,FXSelector,void*);
  long onPlayerVolume(FXObject*,FXSelector,void*);
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

  GMCoverCache * getCoverCache() const { return covercache; }

  GMCoverManager * getCoverManager() const { return covermanager; }



  /// Change Source
  void setSource(FXuint source);


  void update_tray_icon();


  FXint getNumSources() const { return sources.no(); }

  GMSource * getSource(FXint i) const { return sources[i]; }

  void removeSource(GMSource * src);

  void insertSource(GMSource * src) { sources.append(src); }

  void removePlayListSources();

  GMSource * getSource() const { return source; }

#ifdef HAVE_PLAYQUEUE
  GMPlayQueue * getPlayQueue() const { return queue; }
#endif

  GMWindow * getMainWindow() const { return mainwindow; }


  GMTrackView * getTrackView() const;

  GMSourceView * getSourceView() const;

  FXuint getMainWindowId() const;

  GMAudioPlayer * getPlayer() const { return player; }

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




  void playItem(FXuint whence);
  void open(const FXString & mrl);


  void pause();

  void unpause();

  void stop(FXbool closedevice=false);


  void volume(FXint l);
  FXint volume() const;

  void seek(FXdouble pos);

  FXbool playlist_empty();

  void notify_playback_finished();

  void reset_track_display();

  void update_cover_display();

  void update_track_display(FXbool notify=true);

  void update_time_display();

  void load_album_covers();

  void display_track_notification();

  FXint current_position() const;

  FXbool playing() const;

  FXbool audio_device_opened() const;

  FXint get_prev() const;

  FXint get_next() const;

  /// Set Sleep Timer - 0 turns the timer off
  void setSleepTimer(FXlong ns);

  FXbool hasSleepTimer();

  void show_message(const FXchar * title,const FXchar * msg);

  void setStatus(const FXString & msg);

  void register_global_hotkeys();

  FXbool handle_global_hotkeys(FXuint code);


  void cmd_play();
  void cmd_playpause();
  void cmd_pause();
  void cmd_stop();
  void cmd_schedule_stop();
  void cmd_next();
  void cmd_prev();
  void cmd_toggle_shown();
  void cmd_focus_next();
  void cmd_focus_previous();
  void cmd_raise();


#ifdef HAVE_PLAYQUEUE
  void setPlayQueue(FXbool enable);
#endif
  FXint createPlaylist(const FXString & name);

  ~GMPlayerManager();
  };

#endif
