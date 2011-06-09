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
#ifndef GMWINDOW_H
#define GMWINDOW_H

class GMRemote;
class GMTrackView;
class GMSourceView;
class GMIconTheme;
class GMPreferencesDialog;
class GMImageView;

enum {
  SHOW_NORMAL,
  SHOW_WIZARD,
  SHOW_TRAY
  };

class GMWindow : public FXMainWindow {
  FXDECLARE(GMWindow)
friend class GMRemote;
friend class GMPlayerManager;
friend class GMTrackView;
friend class GMPreferencesDialog;
private:
  FXHorizontalFrame * toolbar;
  FXStatusBar       * statusbar;
private:
  FXMenuPtr         filemenu;
  FXMenuPtr         editmenu;
  FXMenuPtr         viewmenu;
  FXMenuPtr         playmenu;
  FXMenuPtr         helpmenu;
  FXMenuButton     *volumebutton;
  FXPopup          *volumecontrol;
  FXSlider         *volumeslider;
private:
  FXMenuPtr         menu_library;
  FXMenuPtr         menu_media;
  FXMenuPtr         menu_view;
  FXMenuPtr         menu_gmm;
  GMMenuButton    * menubutton_library;
  GMMenuButton    * menubutton_media;
  GMMenuButton    * menubutton_view;
  GMMenuButton    * menubutton_gmm;

  GMIconTheme       * icontheme;
  GMTrackView       * trackview;
  GMSourceView      * sourceview;
  FXHorizontalFrame * statusframe;
  GMCoverFrame      * coverframe;
  GMImageView       * coverview_gl;
  FXImageFrame      * coverview_x11;
  FXGLVisual        * glvisual;
  FXToggleButton    * playpausebutton;
  FXButton          * stopbutton;
  FXButton          * nextbutton;
  FXButton          * prevbutton;
  FXLabel           * label_nowplaying;
  FXMenuCheck       * fullscreencheck;
  FXSeparator       * controlseparator;
  FXDragCorner 			* controldragcorner;
  FXSeparator  			* controlstatusseparator;
  FXLabel      			* timelabel;
  GMTrackProgressBar* trackslider;
  FX4Splitter       * mainsplitter;
  FXTextField       * nowplaying;
  FXHorizontalFrame * progressbar;
  FXLabel           * progressbar_label;
  FXHorizontalFrame * progressbar_cancelbutton;
  GMRemote          * remote;
private:
  FXString            coverfile;
private:
  FXImagePtr cover_small;
private:
  void configureToolbar(FXbool docktop,FXbool init=false);
  void configureStatusbar(FXbool show);
  void setFullScreen(FXbool show);
  FXbool showSources() const;
private:
  GMWindow(){}
  GMWindow(const GMWindow&);
  GMWindow& operator=(const GMWindow&);
public: /// Message Handlers
  long onCmdAbout(FXObject*,FXSelector,void*);
  long onCmdQuit(FXObject*,FXSelector,void*);
  long onCmdPreferences(FXObject*,FXSelector,void*);

  long onCmdTimeSlider(FXObject*,FXSelector,void*);
  long onCmdVolume(FXObject*,FXSelector,void*);
  long onCmdVolumeButton(FXObject*,FXSelector,void*);
  long onUpdVolumeButton(FXObject*,FXSelector,void*);

  long onCmdOpen(FXObject*,FXSelector,void*);

  long onCmdImport(FXObject*,FXSelector,void*);
  long onCmdImportFiles(FXObject*,FXSelector,void*);
  long onCmdShowFullScreen(FXObject*,FXSelector,void*);
  long onCmdShowSources(FXObject*,FXSelector,void*);
  long onUpdShowSources(FXObject*,FXSelector,void*);
  long onCmdShowBrowser(FXObject*,FXSelector,void*);
  long onCmdShowMiniPlayer(FXObject*,FXSelector,void*);
  long onUpdShowMiniPlayer(FXObject*,FXSelector,void*);

  long onCmdPlayPause(FXObject*,FXSelector,void*);
  long onUpdPlayPause(FXObject*,FXSelector,void*);
  long onUpdPlayPauseMenu(FXObject*,FXSelector,void*);
  long onCmdPlay(FXObject*,FXSelector,void*);
  long onUpdPlay(FXObject*,FXSelector,void*);
  long onCmdPause(FXObject*,FXSelector,void*);
  long onUpdPause(FXObject*,FXSelector,void*);
  long onCmdStop(FXObject*,FXSelector,void*);
  long onUpdStop(FXObject*,FXSelector,void*);
  long onCmdNext(FXObject*,FXSelector,void*);
  long onUpdNext(FXObject*,FXSelector,void*);
  long onCmdPrev(FXObject*,FXSelector,void*);
  long onUpdPrev(FXObject*,FXSelector,void*);
  long onCmdRepeatAll(FXObject*,FXSelector,void*);
  long onUpdRepeatAll(FXObject*,FXSelector,void*);
  long onCmdRepeatAB(FXObject*,FXSelector,void*);
  long onUpdRepeatAB(FXObject*,FXSelector,void*);
  long onCmdRepeatOff(FXObject*,FXSelector,void*);
  long onUpdRepeatOff(FXObject*,FXSelector,void*);
  long onCmdRepeat(FXObject*,FXSelector,void*);
  long onUpdRepeat(FXObject*,FXSelector,void*);
  long onCmdSleepTimer(FXObject*,FXSelector,void*);
  long onUpdSleepTimer(FXObject*,FXSelector,void*);
  long onCmdShuffle(FXObject*,FXSelector,void*);
  long onUpdShuffle(FXObject*,FXSelector,void*);
  long onCmdHomePage(FXObject*,FXSelector,void*);
  long onCmdReportIssue(FXObject*,FXSelector,void*);
  long onCmdJoinLastFM(FXObject*,FXSelector,void*);
  long onCmdJoinGMMLastFM(FXObject*,FXSelector,void*);

  long onCmdResetColors(FXObject*,FXSelector,void*);

  long onCmdPlayQueue(FXObject*,FXSelector,void*);
  long onUpdPlayQueue(FXObject*,FXSelector,void*);


  long onCmdChangeCoverView(FXObject*,FXSelector,void*);
  long onCmdCoverView(FXObject*,FXSelector,void*);
  long onCmdCoverSize(FXObject*,FXSelector,void*);
  long onUpdCoverSize(FXObject*,FXSelector,void*);

  long onCmdSeek(FXObject*,FXSelector,void*);
  long onCmdNextFocus(FXObject*,FXSelector,void*);

public:
  enum{
    ID_ABOUT=FXMainWindow::ID_LAST,
    ID_QUIT,

    ID_OPEN,

    ID_HOMEPAGE,
    ID_REPORT_ISSUE,
    ID_JOIN_LASTFM,
    ID_JOIN_GMM_LASTFM,

    ID_PAUSE,
    ID_PLAYPAUSE,
    ID_PLAYPAUSEMENU,
    ID_STOP,
    ID_NEXT,
    ID_PREV,

    ID_SEEK_FORWARD_10SEC,
    ID_SEEK_FORWARD_1MIN,
    ID_SEEK_BACKWARD_10SEC,
    ID_SEEK_BACKWARD_1MIN,

    ID_REPEAT,
    ID_REPEAT_ALL,
    ID_REPEAT_AB,
    ID_REPEAT_OFF,
    ID_SHUFFLE,

    ID_TIMESLIDER,
    ID_VOLUME_BUTTON,
    ID_VOLUME_SLIDER,

    ID_DISPLAYMODE,

    ID_DATABASE_CLEAR,

    ID_IMPORT_DIRS,
    ID_IMPORT_FILES,
    ID_SYNC_DIRS,


    ID_PREFERENCES,
    ID_SHOW_TRACK,
    ID_SHOW_FULLSCREEN,
    ID_SHOW_SOURCES,
    ID_SHOW_MINIPLAYER,
    ID_SHOW_BROWSER,

    ID_OPEN_DIR,


    ID_RESET_COLORS,

    ID_DDE_MESSAGE,
    ID_SLEEP,

    ID_COVERVIEW,
    ID_CHANGE_COVERVIEW,
    ID_COVERSIZE_SMALL,
    ID_COVERSIZE_MEDIUM,
    ID_COVERSIZE_LARGE,
    ID_COVERSIZE_EXTRALARGE,
    ID_PLAYQUEUE,


    ID_NEXT_FOCUS,

    ID_LAST
    };
public:
  GMWindow(FXApp* a,FXObject*tgt,FXSelector sel);


  GMRemote * getRemote() const { return remote; }


  void updateCoverView();

  void create_dialog_header(FXDialogBox * dialog,const FXString & title,const FXString & label,FXIcon * icon=NULL);

  FXbool question(const FXString & title,const FXString & label,const FXString & accept,const FXString & cancel);


  void reset();
  void display(const GMTrack&);

  void showRemote();
  void hideRemote();
  void init(FXuint);
  void toggleShown();

  /// Create window
  virtual void create();
  virtual void show();
  virtual void hide();

  GMTrackView * getTrackView() const { return trackview; }
  GMSourceView  * getSourceView() const { return sourceview; }


  void loadCover(const FXString & filename);


  void update_elapsed_time(FXint hours,FXint minutes,FXint seconds,FXint position,FXbool playing,FXbool seekable);
  void update_volume_display(FXint level);


  FXImage * getSmallCover() const { return cover_small; }


  void setStatus(const FXString& status);


  void focusNext();
  void focusPrevious();


  /// Destructor
  virtual ~GMWindow();
  };

#endif
