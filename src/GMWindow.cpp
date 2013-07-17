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
#include "gmdefs.h"
#include "gmutils.h"
#include <xincs.h>

#include <FXArray.h>

#include "GMTrack.h"
#include "GMApp.h"
#include "GMAbout.h"
#include "GMTrackList.h"
#include "GMList.h"
#include "GMTaskManager.h"
#include "GMSource.h"
#include "GMFilename.h"
#include "GMPlayerManager.h"
#include "GMWindow.h"
#include "GMRemote.h"
#include "GMCover.h"
#include "GMCoverManager.h"

#include "GMDatabase.h"
#include "GMDatabaseSource.h"
#include "GMTrackView.h"
#include "GMSourceView.h"
#include "GMAudioScrobbler.h"
#include "GMIconTheme.h"
#include "GMImportDialog.h"
#include "GMPreferencesDialog.h"
#include "FXUTF8Codec.h"

#ifdef HAVE_OPENGL_COVERVIEW
#include "GMImageView.h"
#endif

#include "GMAnimImage.h"

#include "GMScanner.h"

#define HIDESOURCES (FX4Splitter::ExpandTopRight)
#define SHOWSOURCES (FX4Splitter::ExpandTopLeft|FX4Splitter::ExpandTopRight)
#define SHOWSOURCES_COVER (FX4Splitter::ExpandTopLeft|FX4Splitter::ExpandTopRight|FX4Splitter::ExpandBottomLeft)

// Define Message Map
FXDEFMAP(GMWindow) GMWindowMap[]={
  //________Message_Type_____________________ID___________________________Message_Handler___
  FXMAPFUNC(SEL_UPDATE,         		GMWindow::ID_PLAYPAUSE,         GMWindow::onUpdPlayPause),
  FXMAPFUNC(SEL_UPDATE,         		GMWindow::ID_PLAYPAUSEMENU,     GMWindow::onUpdPlayPauseMenu),

  FXMAPFUNC(SEL_UPDATE,         		GMWindow::ID_PAUSE,             GMWindow::onUpdPause),
  FXMAPFUNC(SEL_UPDATE,         		GMWindow::ID_STOP,              GMWindow::onUpdStop),
  FXMAPFUNC(SEL_UPDATE,         		GMWindow::ID_NEXT,              GMWindow::onUpdNext),
  FXMAPFUNC(SEL_UPDATE,         		GMWindow::ID_PREV,              GMWindow::onUpdPrev),
  FXMAPFUNC(SEL_UPDATE,         		GMWindow::ID_REPEAT_ALL,       	GMWindow::onUpdRepeatAll),
  FXMAPFUNC(SEL_UPDATE,         		GMWindow::ID_REPEAT,    				GMWindow::onUpdRepeat),
  FXMAPFUNC(SEL_UPDATE,         		GMWindow::ID_REPEAT_AB,    		  GMWindow::onUpdRepeatAB),
  FXMAPFUNC(SEL_UPDATE,         		GMWindow::ID_REPEAT_OFF,    		GMWindow::onUpdRepeatOff),

  FXMAPFUNC(SEL_UPDATE,         		GMWindow::ID_SHUFFLE,    				GMWindow::onUpdShuffle),
  FXMAPFUNC(SEL_UPDATE,							GMWindow::ID_SLEEP,					    GMWindow::onUpdSleepTimer),
  FXMAPFUNC(SEL_UPDATE,					    GMWindow::ID_VOLUME_BUTTON,     GMWindow::onUpdVolumeButton),
  FXMAPFUNC(SEL_UPDATE,        		  GMWindow::ID_SHOW_MINIPLAYER,   GMWindow::onUpdShowMiniPlayer),

  FXMAPFUNC(SEL_COMMAND,						GMWindow::ID_QUIT,							GMWindow::onCmdQuit),
  FXMAPFUNC(SEL_SIGNAL,							GMWindow::ID_QUIT,							GMWindow::onCmdQuit),
  FXMAPFUNC(SEL_COMMAND,						GMWindow::ID_IMPORT_DIRS,				GMWindow::onCmdImport),
  FXMAPFUNC(SEL_COMMAND,						GMWindow::ID_IMPORT_FILES,		  GMWindow::onCmdImport),
  FXMAPFUNC(SEL_COMMAND,						GMWindow::ID_SYNC_DIRS,		      GMWindow::onCmdImport),

  FXMAPFUNC(SEL_COMMAND,						GMWindow::ID_OPEN,							GMWindow::onCmdOpen),

  FXMAPFUNC(SEL_COMMAND,         		GMWindow::ID_REPEAT_ALL,       	GMWindow::onCmdRepeatAll),
  FXMAPFUNC(SEL_COMMAND,         		GMWindow::ID_REPEAT_AB,    			GMWindow::onCmdRepeatAB),
  FXMAPFUNC(SEL_COMMAND,         		GMWindow::ID_REPEAT,    				GMWindow::onCmdRepeat),
  FXMAPFUNC(SEL_COMMAND,         		GMWindow::ID_REPEAT_OFF,    	  GMWindow::onCmdRepeatOff),

  FXMAPFUNC(SEL_COMMAND,         		GMWindow::ID_SHUFFLE,    				GMWindow::onCmdShuffle),

  FXMAPFUNC(SEL_COMMAND,        		GMWindow::ID_ABOUT,             GMWindow::onCmdAbout),

  FXMAPFUNC(SEL_COMMAND,        		GMWindow::ID_PLAYPAUSE,         GMWindow::onCmdPlayPause),
  FXMAPFUNC(SEL_COMMAND,        		GMWindow::ID_PLAYPAUSEMENU,     GMWindow::onCmdPlayPause),

  FXMAPFUNC(SEL_COMMAND,        		GMWindow::ID_PAUSE,             GMWindow::onCmdPause),
  FXMAPFUNC(SEL_COMMAND,        		GMWindow::ID_STOP,              GMWindow::onCmdStop),
  FXMAPFUNC(SEL_COMMAND,        		GMWindow::ID_NEXT,              GMWindow::onCmdNext),
  FXMAPFUNC(SEL_COMMAND,        		GMWindow::ID_PREV,              GMWindow::onCmdPrev),
//  FXMAPFUNCS(SEL_COMMAND,GMWindow::ID_SEEK_FORWARD_10SEC,GMWindow::ID_SEEK_BACKWARD_1MIN,GMWindow::onCmdSeek),

  FXMAPFUNC(SEL_COMMAND,						GMWindow::ID_SHOW_FULLSCREEN,		GMWindow::onCmdShowFullScreen),
  FXMAPFUNC(SEL_COMMAND,        		GMWindow::ID_SHOW_MINIPLAYER,   GMWindow::onCmdShowMiniPlayer),
  FXMAPFUNC(SEL_COMMAND,        		GMWindow::ID_SHOW_BROWSER,      GMWindow::onCmdShowBrowser),

  FXMAPFUNC(SEL_COMMAND,        		GMWindow::ID_PREFERENCES,       GMWindow::onCmdPreferences),

  FXMAPFUNC(SEL_COMMAND,            GMWindow::ID_RESET_COLORS,      GMWindow::onCmdResetColors),

  FXMAPFUNC(SEL_COMMAND,        		GMWindow::ID_TIMESLIDER,        GMWindow::onCmdTimeSlider),

  FXMAPFUNC(SEL_COMMAND,						GMWindow::ID_VOLUME_SLIDER,			GMWindow::onCmdVolume),
  FXMAPFUNC(SEL_CHANGED,						GMWindow::ID_VOLUME_SLIDER,			GMWindow::onCmdVolume),
  FXMAPFUNC(SEL_MOUSEWHEEL,					GMWindow::ID_VOLUME_BUTTON,     GMWindow::onCmdVolumeButton),
  FXMAPFUNC(SEL_MOUSEWHEEL,					0,															GMWindow::onCmdVolumeButton),


  FXMAPFUNC(SEL_COMMAND,						GMWindow::ID_HOMEPAGE,					GMWindow::onCmdHomePage),
  FXMAPFUNC(SEL_COMMAND,						GMWindow::ID_REPORT_ISSUE,	  	GMWindow::onCmdReportIssue),

  FXMAPFUNC(SEL_COMMAND,						GMWindow::ID_JOIN_LASTFM,	  	GMWindow::onCmdJoinLastFM),
  FXMAPFUNC(SEL_COMMAND,						GMWindow::ID_JOIN_GMM_LASTFM,	  	GMWindow::onCmdJoinGMMLastFM),

  FXMAPFUNC(SEL_COMMAND,						GMWindow::ID_SLEEP,					GMWindow::onCmdSleepTimer),

  FXMAPFUNC(SEL_COMMAND,						GMWindow::ID_PLAYQUEUE,					GMWindow::onCmdPlayQueue),
  FXMAPFUNC(SEL_UPDATE,						  GMWindow::ID_PLAYQUEUE,					GMWindow::onUpdPlayQueue),

  FXMAPFUNC(SEL_COMMAND,						GMWindow::ID_NEXT_FOCUS,		GMWindow::onCmdNextFocus),
  FXMAPFUNC(SEL_CONFIGURE,          GMWindow::ID_COVERVIEW,         GMWindow::onConfigureCoverView),
  //FXMAPFUNC(SEL_MAP,                GMWindow::ID_COVERVIEW,         GMWindow::onConfigureCoverView),

  FXMAPFUNC(SEL_TIMEOUT,            GMWindow::ID_REFRESH_COVERVIEW,    GMWindow::onConfigureCoverView),

  FXMAPFUNC(SEL_UPDATE,         		GMWindow::ID_SHOW_SOURCES,       	GMWindow::onUpdShowSources),
  FXMAPFUNC(SEL_COMMAND,         		GMWindow::ID_SHOW_SOURCES,       	GMWindow::onCmdShowSources),
  };


// Object implementation
FXIMPLEMENT(GMWindow,FXMainWindow,GMWindowMap,ARRAYNUMBER(GMWindowMap))




//----------------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------------
GMWindow::GMWindow(FXApp* a,FXObject*tgt,FXSelector msg) : FXMainWindow(a,"Goggles Music Manager",NULL,NULL,DECOR_ALL,5,5,700,580) {
  flags|=FLAG_ENABLED;

  remote=NULL;

  icontheme = new GMIconTheme(getApp());
  icontheme->load();

  setIcon(icontheme->icon_applogo);
  setMiniIcon(icontheme->icon_applogo_small);

#if APPLICATION_BETA > 0
  setTitle(FXString::value("Goggles Music Manager %d.%d.%d-beta%d",APPLICATION_MAJOR,APPLICATION_MINOR,APPLICATION_LEVEL,APPLICATION_BETA));
#endif

  /// Set myself as the target
  setTarget(tgt);
  setSelector(msg);

  /// Popup Volume Menu
  volumecontrol = new FXPopup(this,POPUP_VERTICAL|FRAME_RAISED|FRAME_THICK|POPUP_SHRINKWRAP);
  volumeslider = new FXSlider(volumecontrol,this,GMWindow::ID_VOLUME_SLIDER,LAYOUT_FIX_HEIGHT|LAYOUT_FIX_WIDTH|SLIDER_VERTICAL|SLIDER_TICKS_RIGHT|SLIDER_TICKS_LEFT|SLIDER_INSIDE_BAR,0,0,20,100);
  volumeslider->setTickDelta(10);
  volumeslider->setRange(0,100);
  volumeslider->setIncrement(10);

  toolbar   = new FXHorizontalFrame(this,LAYOUT_SIDE_TOP|LAYOUT_FILL_X,0,0,0,0,3,3,3,0);
  statusbar = new FXStatusBar(this,LAYOUT_SIDE_BOTTOM|LAYOUT_FILL_X|STATUSBAR_WITH_DRAGCORNER,0,0,0,0,3,3,2,2,3);
  statusbar->getStatusLine()->setFrameStyle(FRAME_NONE);


  progressbar = new FXHorizontalFrame(statusbar,LAYOUT_LEFT|FRAME_NONE,0,0,0,0,3,3,0,0);

  progressbar_animation = new GMAnimImage(progressbar,GMIconTheme::instance()->icon_progress,GMIconTheme::instance()->getSmallSize(),FRAME_NONE|LAYOUT_CENTER_Y);
  progressbar_animation->setBackColor(getApp()->getBaseColor());
  progressbar_label = new FXLabel(progressbar,FXString::null,NULL,LAYOUT_CENTER_Y|JUSTIFY_CENTER_Y);
  new GMButton(progressbar,tr("\tCancel Task\tCancel Task"),GMIconTheme::instance()->icon_close,GMPlayerManager::instance(),GMPlayerManager::ID_CANCEL_TASK,BUTTON_TOOLBAR|FRAME_RAISED);
  new FXSeparator(progressbar,LAYOUT_FILL_Y|SEPARATOR_GROOVE);

  progressbar->reparent(statusbar,statusbar->getStatusLine());
  progressbar_animation->hide();  
  progressbar->hide();

  FXVerticalFrame * mainframe = new FXVerticalFrame(this,LAYOUT_FILL_X|LAYOUT_FILL_Y);

  mainsplitter    = new FX4Splitter(mainframe,LAYOUT_FILL_X|LAYOUT_FILL_Y|FOURSPLITTER_VERTICAL|FOURSPLITTER_TRACKING);
  sourceview      = new GMSourceView(mainsplitter);
  trackview       = new GMTrackView(mainsplitter);
  coverframe      = new GMCoverFrame(mainsplitter);

  mainsplitter->setBarSize(7);

  coverframe->setBackColor(getApp()->getBackColor());
  coverframe->setBorderColor(getApp()->getShadowColor());
  coverframe->hide();
  coverview_x11=NULL;
#ifdef HAVE_OPENGL_COVERVIEW
  coverview_gl=NULL;
  glvisual=NULL;
#endif

  updateCoverView();



  /****************************************************************************/
  /****************************************************************************/

  /// Library Menu
  menu_library = new GMMenuPane(this);
  new GMMenuCommand(menu_library,tr("Import Folder…\tCtrl-O\tImport Music from folder into Library"),icontheme->icon_import,this,GMWindow::ID_IMPORT_DIRS);
  new GMMenuCommand(menu_library,tr("Sync Folder…\t\tSynchronize Folder with Music in Library"),icontheme->icon_sync,this,GMWindow::ID_SYNC_DIRS);
  new FXMenuSeparator(menu_library);
  new GMMenuCommand(menu_library,tr("Play File or Stream…\t\tPlay File or Stream"),NULL,this,GMWindow::ID_OPEN);
  new FXMenuSeparator(menu_library);
  new GMMenuCommand(menu_library,tr("New Playlist…\t\tCreate a new playlist"),NULL,GMPlayerManager::instance()->getDatabaseSource(),GMDatabaseSource::ID_NEW_PLAYLIST);
  new GMMenuCommand(menu_library,tr("Import Playlist…\t\tImport existing playlist"),icontheme->icon_import,GMPlayerManager::instance()->getDatabaseSource(),GMDatabaseSource::ID_IMPORT_PLAYLIST);

//  new GMMenuCommand(menu_library,tr("Sync Folder…\t\tSynchronize Folder with Music in Library"),icontheme->icon_sync,this,GMWindow::ID_SYNC_DIRS);
//  new FXMenuSeparator(menu_library);
//  new GMMenuCommand(menu_library,tr("Add Podcast…\t\tAdd a radio station"),NULL,sourceview,GMSourceView::ID_NEW_STATION);
//  new GMMenuCommand(menu_library,tr("Add Radio Station…\t\tAdd a radio station"),NULL,sourceview,GMSourceView::ID_NEW_STATION);
//  new FXMenuSeparator(menu_library);
//  new FXMenuSeparator(menu_library);


 // new GMMenuCommand(menu_library,tr("New Radio Station…\t\tCreate a new playlist"),NULL,sourceview,GMSourceView::ID_NEW_STATION);
  gm_set_window_cursor(menu_library,getApp()->getDefaultCursor(DEF_ARROW_CURSOR));

  //// Active View Menu
/*
  menu_view    = new GMMenuPane(this);
  new GMMenuCommand(menu_view,tr("&Copy\tCtrl-C\tCopy Selected Tracks"),icontheme->icon_copy,trackview,GMTrackView::ID_COPY,MENU_AUTOGRAY);
  new GMMenuCommand(menu_view,tr("&Cut\tCtrl-X\tCut Selected Tracks"),icontheme->icon_cut,trackview,GMTrackView::ID_CUT,MENU_AUTOGRAY);
  new GMMenuCommand(menu_view,tr("&Paste\tCtrl-V\tPaste Clipboard Selection"),icontheme->icon_paste,trackview,GMTrackView::ID_PASTE);
  new FXMenuSeparator(menu_view);
  new GMMenuCommand(menu_view,tr("Find…\tCtrl-F\tShow search filter."),GMIconTheme::instance()->icon_find,trackview,GMTrackView::ID_TOGGLE_FILTER);
  new FXMenuSeparator(menu_view);
  new GMMenuCommand(menu_view,tr("&Configure Columns""…"),NULL,trackview,GMTrackView::ID_CONFIGURE_COLUMNS);
  new GMMenuCascade(menu_view,tr("&Sort"),icontheme->icon_sort,trackview->getSortMenu());
  new GMMenuCheck(menu_view,tr("&Browse\tCtrl-B\tShow genre artist and album browser."),trackview,GMTrackView::ID_TOGGLE_BROWSER);
  new GMMenuCheck(menu_view,tr("Show &Tags\tCtrl-G\tShow tag browser."),trackview,GMTrackView::ID_TOGGLE_TAGS);
  gm_set_window_cursor(menu_view,getApp()->getDefaultCursor(DEF_ARROW_CURSOR));
*/
  /// Media Controls
  menu_media   = new GMMenuPane(this);
  new GMMenuCheck(menu_media,tr("Queue Play\t\tPlay tracks from queue."),this,ID_PLAYQUEUE);
  new GMMenuCheck(menu_media,tr("Shuffle Play\tAlt-R\tPlay tracks in random order."),this,ID_SHUFFLE);
  new FXMenuSeparator(menu_media);
  new GMMenuRadio(menu_media,tr("Repeat Off\tCtrl-,\tRepeat current track."),this,ID_REPEAT_OFF);
  new GMMenuRadio(menu_media,tr("Repeat Track\tCtrl-.\tRepeat current track."),this,ID_REPEAT);
  new GMMenuRadio(menu_media,tr("Repeat All Tracks\tCtrl-/\tRepeat all tracks."),this,ID_REPEAT_ALL);
//  new GMMenuCheck(menu_media,tr("Repeat A-B\tCtrl-T\tRepeat section of track."),this,ID_REPEAT_AB);
  new FXMenuSeparator(menu_media);
  new GMMenuCheck(menu_media,tr("Sleep Timer\t\tSetup sleeptimer."),this,ID_SLEEP);
  gm_set_window_cursor(menu_media,getApp()->getDefaultCursor(DEF_ARROW_CURSOR));

  /// Program Menu
  menu_gmm     = new GMMenuPane(this);
  new GMMenuCheck(menu_gmm,tr("Show &Sources\tCtrl-S\tShow source browser "),this,ID_SHOW_SOURCES);
  fullscreencheck = new GMMenuCheck(menu_gmm,tr("Show Full Screen\tF12\tToggle fullscreen mode."),this,ID_SHOW_FULLSCREEN);
  new GMMenuCheck(menu_gmm,tr("Show Mini Player\tCtrl-M\tToggle Mini Player."),this,ID_SHOW_MINIPLAYER);
  new FXMenuSeparator(menu_gmm);
  new GMMenuCommand(menu_gmm,tr("Find…\tCtrl-F\tShow search filter."),GMIconTheme::instance()->icon_find,trackview,GMTrackView::ID_TOGGLE_FILTER);
  new FXMenuSeparator(menu_gmm);
  new GMMenuCommand(menu_gmm,tr("Preferences…"),icontheme->icon_settings,this,GMWindow::ID_PREFERENCES);
  new GMMenuCommand(menu_gmm,tr("&About…"),icontheme->icon_info,this,GMWindow::ID_ABOUT);
  new FXMenuSeparator(menu_gmm);
  new GMMenuCommand(menu_gmm,tr("&Quit\tCtrl-Q\tQuit the application."),icontheme->icon_exit,this,GMWindow::ID_QUIT);
  gm_set_window_cursor(menu_gmm,getApp()->getDefaultCursor(DEF_ARROW_CURSOR));

  /****************************************************************************/
  /****************************************************************************/

  controldragcorner = new FXDragCorner(toolbar);

  menubutton_library = new GMMenuButton(toolbar,"\tManage Music Database",icontheme->icon_create,menu_library,MENUBUTTON_TOOLBAR|FRAME_RAISED|LAYOUT_CENTER_Y|ICON_AFTER_TEXT);
  //menubutton_view    = new GMMenuButton(toolbar,"\tEdit View",icontheme->icon_document,menu_view,MENUBUTTON_TOOLBAR|FRAME_RAISED|LAYOUT_CENTER_Y|MENUBUTTON_DOWN|ICON_AFTER_TEXT);
  menubutton_gmm     = new GMMenuButton(toolbar,"\tCustomize and Control GMM",icontheme->icon_customize,menu_gmm,MENUBUTTON_TOOLBAR|FRAME_RAISED|LAYOUT_CENTER_Y|MENUBUTTON_DOWN|ICON_AFTER_TEXT|LAYOUT_RIGHT);
  menubutton_media   = new GMMenuButton(toolbar,"\tMedia Control",icontheme->icon_media,menu_media,MENUBUTTON_TOOLBAR|FRAME_RAISED|LAYOUT_RIGHT|LAYOUT_CENTER_Y|ICON_AFTER_TEXT|LAYOUT_RIGHT);
  volumebutton       = new GMMenuButton(toolbar,"\tAdjust Volume\tAdjust Volume",icontheme->icon_volume_muted_toolbar,volumecontrol,MENUBUTTON_NOARROWS|MENUBUTTON_ATTACH_LEFT|MENUBUTTON_UP|MENUBUTTON_TOOLBAR|FRAME_RAISED|LAYOUT_CENTER_Y|LAYOUT_RIGHT);
  volumebutton->setTarget(this);
  volumebutton->setSelector(ID_VOLUME_BUTTON);

  new FXVerticalSeparator(toolbar,LAYOUT_FILL_Y|SEPARATOR_GROOVE|LAYOUT_RIGHT);
  new FXVerticalSeparator(toolbar,LAYOUT_FILL_Y|SEPARATOR_GROOVE);
  playpausebutton = new FXToggleButton(toolbar,tr("\tStart Playback\tStart Playback"),tr("\tPause\tPause Playback"),icontheme->icon_play_toolbar,icontheme->icon_pause_toolbar,this,ID_PLAYPAUSE,BUTTON_TOOLBAR|FRAME_RAISED|ICON_ABOVE_TEXT|LAYOUT_CENTER_Y);
  stopbutton      = new FXButton(toolbar,tr("\tStop Playback\tStop Playback"),icontheme->icon_stop_toolbar,this,ID_STOP,BUTTON_TOOLBAR|FRAME_RAISED|ICON_ABOVE_TEXT|LAYOUT_CENTER_Y);
  prevbutton      = new FXButton(toolbar,tr("\tPlay Previous Track\tPlay previous track."),icontheme->icon_prev_toolbar,this,ID_PREV,BUTTON_TOOLBAR|FRAME_RAISED|ICON_ABOVE_TEXT|LAYOUT_CENTER_Y);
  nextbutton      = new FXButton(toolbar,tr("\tPlay Next Track\tPlay next track."),icontheme->icon_next_toolbar,this,ID_NEXT,BUTTON_TOOLBAR|FRAME_RAISED|ICON_ABOVE_TEXT|LAYOUT_CENTER_Y);
  new FXVerticalSeparator(toolbar,LAYOUT_FILL_Y|SEPARATOR_GROOVE);

  FXVerticalFrame * timeframe = new FXVerticalFrame(toolbar,LAYOUT_FILL_X|LAYOUT_CENTER_Y,0,0,0,0,0,0,0,0,0,1);
  label_nowplaying = new FXLabel(timeframe," ",NULL,LABEL_NORMAL|LAYOUT_CENTER_Y|LAYOUT_CENTER_X,0,0,0,0,0,0,0,0);
//  new FXLabel(timeframe," ",NULL,LABEL_NORMAL|LAYOUT_CENTER_Y,0,0,0,0,0,0,0,0);

  FXHorizontalFrame *timelabelframe = new FXHorizontalFrame(timeframe,LAYOUT_FILL_X,0,0,0,0,0,0,0,0);
  time_progress   = new FXTextField(timelabelframe,8,NULL,0,LAYOUT_LEFT|LAYOUT_CENTER_Y|TEXTFIELD_READONLY,0,0,0,0,0,0,0,0);
  time_remaining  = new FXTextField(timelabelframe,8,NULL,0,LAYOUT_RIGHT|LAYOUT_CENTER_Y|TEXTFIELD_READONLY,0,0,0,0,0,0,0,0);
  time_progress->disable();
  time_remaining->disable();
  time_progress->setJustify(JUSTIFY_CENTER_X);
  time_remaining->setJustify(JUSTIFY_CENTER_X);


  trackslider = new GMTrackProgressBar(timelabelframe,this,ID_TIMESLIDER,LAYOUT_FILL_X|LAYOUT_CENTER_Y|FRAME_RAISED,0,0,0,0,0,0,0,0);
  trackslider->setTotal(100000);
  trackslider->setDefaultCursor(GMIconTheme::instance()->cursor_hand);
  trackslider->setDragCursor(GMIconTheme::instance()->cursor_hand);

#if 0
  // File Menu
  filemenu=new GMMenuPane(this);
  new GMMenuTitle(menubar,tr("&Music"),NULL,filemenu);
//  new GMMenuCommand(filemenu,tr("Import File(s)…\t\tImport Music into Library"),icontheme->icon_importfile,this,GMWindow::ID_IMPORT_FILES);
  new GMMenuCommand(filemenu,tr("Import Folder…\tCtrl-O\tImport Music from folder into Library"),icontheme->icon_import,this,GMWindow::ID_IMPORT_DIRS);
  new GMMenuCommand(filemenu,tr("Sync Folder…\t\tSynchronize Folder with Music in Library"),icontheme->icon_sync,this,GMWindow::ID_SYNC_DIRS);
  new FXMenuSeparator(filemenu);
  new GMMenuCommand(filemenu,tr("Play File or Stream…\t\tPlay File or Stream"),NULL,this,GMWindow::ID_OPEN);
  new FXMenuSeparator(filemenu);
  new GMMenuCommand(filemenu,tr("New Playlist…\t\tCreate a new playlist"),icontheme->icon_playlist,GMPlayerManager::instance()->getDatabaseSource(),GMDatabaseSource::ID_NEW_PLAYLIST);
  new GMMenuCommand(filemenu,tr("Import Playlist…\t\tImport existing playlist"),icontheme->icon_import,GMPlayerManager::instance()->getDatabaseSource(),GMDatabaseSource::ID_IMPORT_PLAYLIST);
  new GMMenuCommand(filemenu,tr("New Radio Station…\t\tCreate a new playlist"),NULL,sourceview,GMSourceView::ID_NEW_STATION);
  new FXMenuSeparator(filemenu);
  new GMMenuCommand(filemenu,tr("&Quit\tCtrl-Q\tQuit the application."),icontheme->icon_exit,this,GMWindow::ID_QUIT);
  gm_set_window_cursor(filemenu,getApp()->getDefaultCursor(DEF_ARROW_CURSOR));

  // Help Menu
  editmenu=new GMMenuPane(this);
  new GMMenuTitle(menubar,tr("&Edit"),NULL,editmenu);
  new GMMenuCommand(editmenu,tr("&Copy\tCtrl-C\tCopy Selected Tracks"),icontheme->icon_copy,trackview,GMTrackView::ID_COPY,MENU_AUTOGRAY);
  new GMMenuCommand(editmenu,tr("&Cut\tCtrl-X\tCut Selected Tracks"),icontheme->icon_cut,trackview,GMTrackView::ID_CUT,MENU_AUTOGRAY);
  new GMMenuCommand(editmenu,tr("&Paste\tCtrl-V\tPaste Clipboard Selection"),icontheme->icon_paste,trackview,GMTrackView::ID_PASTE,MENU_AUTOGRAY);
  new FXMenuSeparator(editmenu);
  new GMMenuCommand(editmenu,tr("Find…\tCtrl-F\tShow search filter."),GMIconTheme::instance()->icon_find,trackview,GMTrackView::ID_TOGGLE_FILTER);
  new FXMenuSeparator(editmenu);
  new GMMenuCommand(editmenu,tr("Preferences…"),icontheme->icon_settings,this,GMWindow::ID_PREFERENCES);
  gm_set_window_cursor(editmenu,getApp()->getDefaultCursor(DEF_ARROW_CURSOR));

  viewmenu=new GMMenuPane(this);
  new GMMenuTitle(menubar,tr("&View"),NULL,viewmenu);
  new GMMenuCheck(viewmenu,tr("&Browse\tCtrl-B\tShow genre artist and album browser."),trackview,GMTrackView::ID_TOGGLE_BROWSER);
  new GMMenuCheck(viewmenu,tr("Show &Genres\tCtrl-G\tShow genre browser."),trackview,GMTrackView::ID_TOGGLE_GENRES);
  new FXMenuSeparator(viewmenu);
  new GMMenuCheck(viewmenu,tr("Show &Sources\tCtrl-S\tShow source browser "),sourcesplitter,FXWindow::ID_TOGGLESHOWN);
  new FXMenuSeparator(viewmenu);
#if FOXVERSION >= FXVERSION(1,7,11)
  fullscreencheck = new GMMenuCheck(viewmenu,tr("Show Full Screen\tF12\tToggle fullscreen mode."),this,ID_SHOW_FULLSCREEN);
#endif
  new GMMenuCheck(viewmenu,tr("Show Mini Player\tF11\tToggle Mini Player."),this,ID_SHOW_MINIPLAYER);

  new FXMenuSeparator(viewmenu);
  new GMMenuCommand(viewmenu,tr("&Configure Columns""…"),NULL,trackview,GMTrackView::ID_CONFIGURE_COLUMNS);
  new GMMenuCascade(viewmenu,tr("&Sort"),icontheme->icon_sort,trackview->getSortMenu());
  new FXMenuSeparator(viewmenu);
  new GMMenuCommand(viewmenu,tr("&Jump to Current Track\tCtrl-J\tShow current playing track."),NULL,trackview,GMTrackView::ID_SHOW_CURRENT);
  gm_set_window_cursor(viewmenu,getApp()->getDefaultCursor(DEF_ARROW_CURSOR));

  playmenu=new GMMenuPane(this);
  new GMMenuTitle(menubar,tr("&Control"),NULL,playmenu);
  new GMMenuCommand(playmenu,tr("Play\tCtrl-P\tStart playback."),icontheme->icon_play,this,ID_PLAYPAUSEMENU);
  new GMMenuCommand(playmenu,tr("Stop\tCtrl-\\\tStop playback."),icontheme->icon_stop,this,ID_STOP);
  new GMMenuCommand(playmenu,tr("Previous Track\tCtrl-[\tPlay next track."),icontheme->icon_prev,this,ID_PREV);
  new GMMenuCommand(playmenu,tr("Next Track\tCtrl-]\tPlay previous track."),icontheme->icon_next,this,ID_NEXT);
  new FXMenuSeparator(playmenu);
  new GMMenuCheck(playmenu,tr("Queue Play\t\tPlay tracks from queue."),this,ID_PLAYQUEUE);
  new FXMenuSeparator(playmenu);
  new GMMenuRadio(playmenu,tr("Repeat Off\tCtrl-,\tRepeat current track."),this,ID_REPEAT_OFF);
  new GMMenuRadio(playmenu,tr("Repeat Track\tCtrl-.\tRepeat current track."),this,ID_REPEAT);
  new GMMenuRadio(playmenu,tr("Repeat All Tracks\tCtrl-/\tRepeat all tracks."),this,ID_REPEAT_ALL);
  new GMMenuCheck(playmenu,tr("Repeat A-B\tCtrl-T\tRepeat section of track."),this,ID_REPEAT_AB);
  new GMMenuCheck(playmenu,tr("Shuffle Mode\tAlt-R\tPlay tracks in random order."),this,ID_SHUFFLE);
  new FXMenuSeparator(playmenu);
  new GMMenuCommand(playmenu,tr("Equalizer\t\t"),icontheme->icon_equalizer,GMPlayerManager::instance(),GMPlayerManager::ID_EQUALIZER);
  new GMMenuCheck(playmenu,tr("Sleep Timer\t\tSetup sleeptimer."),this,ID_SLEEP);
  gm_set_window_cursor(playmenu,getApp()->getDefaultCursor(DEF_ARROW_CURSOR));

  helpmenu=new GMMenuPane(this);
  new GMMenuTitle(menubar,tr("&Help"),NULL,helpmenu,LAYOUT_RIGHT);
  new GMMenuCommand(helpmenu,tr("&Homepage"),icontheme->icon_homepage,this,GMWindow::ID_HOMEPAGE);
  new GMMenuCommand(helpmenu,tr("&Report Issue…"),NULL,this,GMWindow::ID_REPORT_ISSUE);
  new FXMenuSeparator(helpmenu);
  new GMMenuCommand(helpmenu,tr("&Sign up for last.fm…"),NULL,this,GMWindow::ID_JOIN_LASTFM);
  new GMMenuCommand(helpmenu,tr("&Join GMM on last.fm…\t\tJoin the Goggles Music Manager group on last.fm…"),NULL,this,GMWindow::ID_JOIN_GMM_LASTFM);
  new FXMenuSeparator(helpmenu);
  new GMMenuCommand(helpmenu,tr("&About…"),icontheme->icon_info,this,GMWindow::ID_ABOUT);
  gm_set_window_cursor(helpmenu,getApp()->getDefaultCursor(DEF_ARROW_CURSOR));



  menubar->hide();
#endif

  getAccelTable()->addAccel(parseAccel("F11"),this,FXSEL(SEL_COMMAND,ID_SHOW_MINIPLAYER));
  getAccelTable()->addAccel(parseAccel("Ctrl-W"),this,FXSEL(SEL_CLOSE,0));
  getAccelTable()->addAccel(parseAccel("/"),trackview,FXSEL(SEL_COMMAND,GMTrackView::ID_TOGGLE_FILTER));
  getAccelTable()->addAccel(parseAccel("Ctrl-;"),this,FXSEL(SEL_COMMAND,ID_SEEK_BACKWARD_1MIN));
  getAccelTable()->addAccel(parseAccel("Ctrl-'"),this,FXSEL(SEL_COMMAND,ID_SEEK_FORWARD_1MIN));
  getAccelTable()->addAccel(parseAccel("Ctrl-,"),this,FXSEL(SEL_COMMAND,ID_SEEK_BACKWARD_10SEC));
  getAccelTable()->addAccel(parseAccel("Ctrl-."),this,FXSEL(SEL_COMMAND,ID_SEEK_FORWARD_10SEC));

  getAccelTable()->addAccel(parseAccel("Ctrl-L"),this,FXSEL(SEL_COMMAND,ID_NEXT_FOCUS));

  getAccelTable()->addAccel(parseAccel("Ctrl-C"),trackview,FXSEL(SEL_COMMAND,GMTrackView::ID_COPY));
  getAccelTable()->addAccel(parseAccel("Ctrl-X"),trackview,FXSEL(SEL_COMMAND,GMTrackView::ID_CUT));
  getAccelTable()->addAccel(parseAccel("Ctrl-V"),trackview,FXSEL(SEL_COMMAND,GMTrackView::ID_PASTE));
  getAccelTable()->addAccel(parseAccel("Ctrl-J"),trackview,FXSEL(SEL_COMMAND,GMTrackView::ID_SHOW_CURRENT));
  //getAccelTable()->addAccel(parseAccel("Ctrl-F"),trackview,FXSEL(SEL_COMMAND,GMTrackView::ID_TOGGLE_FILTER));
  getAccelTable()->addAccel(parseAccel("Ctrl-B"),trackview,FXSEL(SEL_COMMAND,GMTrackView::ID_TOGGLE_BROWSER));
  getAccelTable()->addAccel(parseAccel("Ctrl-T"),trackview,FXSEL(SEL_COMMAND,GMTrackView::ID_TOGGLE_TAGS));
  }


void GMWindow::init(FXuint mode) {
  sourceview->init();

  if (mode==SHOW_NORMAL || mode==SHOW_TRAY) {
    if (getApp()->reg().readBoolEntry("window","remote",false)){
      if (!remote) {
        remote = new GMRemote(getApp(),target,message);
        remote->create();
        }
      }
    }

  switch(mode) {
    case SHOW_TRAY  : break;
    case SHOW_WIZARD: toggleShown();
                      handle(this,FXSEL(SEL_COMMAND,GMWindow::ID_IMPORT_DIRS),NULL);
                      break;
    default         : if (getApp()->reg().readBoolEntry("window","window-show",true)){
                        toggleShown();
                        }
                      break;
    };
  }

//----------------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------------
GMWindow::~GMWindow(){
#ifdef HAVE_OPENGL_COVERVIEW
  if (coverview_gl) {
    coverview_gl->setImage(NULL);
    delete coverview_gl;
    delete glvisual;
    }
#endif
  delete icontheme;
  }



//----------------------------------------------------------------------------------
// Create
//----------------------------------------------------------------------------------
void GMWindow::create(){
  mainsplitter->setHSplit(getApp()->reg().readIntEntry("window","source-list-hsplit",2500));

  configureStatusbar(GMPlayerManager::instance()->getPreferences().gui_show_status_bar);

  /// Set initial Toolbar Configuration
  configureToolbar(GMPlayerManager::instance()->getPreferences().gui_toolbar_docktop,true);

  FXMainWindow::create();

  fix_wm_properties(this);

  ewmh_change_window_type(this,WINDOWTYPE_NORMAL);

  /// Initialize the window size & position
  if (getApp()->reg().readIntEntry("window","x",-1)!=-1) {
    FXint xx=getApp()->reg().readIntEntry("window","x",getX());
    FXint yy=getApp()->reg().readIntEntry("window","y",getY());
    FXint ww=getApp()->reg().readIntEntry("window","width",500);
    FXint hh=getApp()->reg().readIntEntry("window","height",550);
    position(xx,yy,ww,hh);
    }
  else {
    place(PLACEMENT_SCREEN);
    }
  /// Set extended window manager hints on the menus for compositing window managers.
/*
  ewmh_change_window_type(filemenu,WINDOWTYPE_DROPDOWN_MENU);
  ewmh_change_window_type(editmenu,WINDOWTYPE_DROPDOWN_MENU);
  ewmh_change_window_type(viewmenu,WINDOWTYPE_DROPDOWN_MENU);
  ewmh_change_window_type(playmenu,WINDOWTYPE_DROPDOWN_MENU);
  ewmh_change_window_type(helpmenu,WINDOWTYPE_DROPDOWN_MENU);
*/
  ewmh_change_window_type(menu_library,WINDOWTYPE_DROPDOWN_MENU);
  ewmh_change_window_type(menu_media,WINDOWTYPE_DROPDOWN_MENU);
//  ewmh_change_window_type(menu_view,WINDOWTYPE_DROPDOWN_MENU);
  ewmh_change_window_type(menu_gmm,WINDOWTYPE_DROPDOWN_MENU);


  gm_set_application_icon(this);
  }


void GMWindow::showRemote(){
  if (!remote) {
    remote = new GMRemote(getApp(),target,message);
    remote->update_volume_display(GMPlayerManager::instance()->volume());
    remote->create();
    if (GMPlayerManager::instance()->playing()){
      GMTrack info;
      GMPlayerManager::instance()->getTrackInformation(info);
      remote->update_cover_display();
      remote->display(info);
      }
    remote->show();
    }
  }

void GMWindow::hideRemote(){
  if (remote) {
    remote->writeRegistry();
    delete remote;
    remote = NULL;
    }
  show();
  }


void GMWindow::setFullScreen(FXbool show){
  if (show) {
    if (isMaximized()) restore();
    getApp()->reg().writeIntEntry("window","x",getX());
    getApp()->reg().writeIntEntry("window","y",getY());
    getApp()->reg().writeIntEntry("window","width",getWidth());
    getApp()->reg().writeIntEntry("window","height",getHeight());
    fullScreen();
    statusbar->setCornerStyle(false);
    controldragcorner->hide();
    fullscreencheck->setCheck(true);
    }
  else {
    restore();
    if (!statusbar->shown() &&  GMPlayerManager::instance()->getPreferences().gui_toolbar_docktop==false)
      controldragcorner->show();
    statusbar->setCornerStyle(true);
    fullscreencheck->setCheck(false);
    }
  }


void GMWindow::hide() {
  getApp()->reg().writeBoolEntry("window","fullscreen",isFullScreen());
  getApp()->reg().writeBoolEntry("window","maximized",isMaximized());
  if (isFullScreen() || isMaximized() || isMinimized() )
    restore();
  FXMainWindow::hide();
  }

void GMWindow::show(){
  FXMainWindow::show();

  if (getApp()->reg().readBoolEntry("window","fullscreen",false)){
    getApp()->reg().writeBoolEntry("window","fullscreen",false);
    getApp()->reg().writeBoolEntry("window","maximized",false);

   if (getApp()->reg().readIntEntry("window","x",-1)!=-1) {
     FXint xx=getApp()->reg().readIntEntry("window","x",getX());
     FXint yy=getApp()->reg().readIntEntry("window","y",getY());
     FXint ww=getApp()->reg().readIntEntry("window","width",500);
     FXint hh=getApp()->reg().readIntEntry("window","height",550);
     position(xx,yy,ww,hh);
     }
   else {
     place(PLACEMENT_SCREEN);
     }
    setFullScreen(true);
    }
  else if (getApp()->reg().readBoolEntry("window","maximized",false)){
    getApp()->reg().writeBoolEntry("window","fullscreen",false);
    getApp()->reg().writeBoolEntry("window","maximized",false);
    maximize();
    }
  }

void GMWindow::raiseWindow() {
  if (remote) {
    if (!remote->shown())
      remote->show();
    remote->raise();
    }
  else {
    if (!shown())
      show();
    raise();
    }
  }

void GMWindow::toggleShown() {
  if (remote) {
    if (remote->shown())
      remote->hide();
    else
      remote->show();
    }
  else {
    if (shown())
      hide();
    else
      show();
    }
  }




FXbool GMWindow::question(const FXString & title,const FXString & labeltext,const FXString & accept,const FXString & cancel){
  FXDialogBox dialog(this,title,DECOR_TITLE|DECOR_BORDER|DECOR_RESIZE,0,0,400,0,0,0,0,0,0,0);
  create_dialog_header(&dialog,title,labeltext,NULL);
  FXHorizontalFrame *closebox=new FXHorizontalFrame(&dialog,LAYOUT_BOTTOM|LAYOUT_FILL_X,0,0,0,0);
  new GMButton(closebox,accept,NULL,&dialog,FXDialogBox::ID_ACCEPT,BUTTON_INITIAL|BUTTON_DEFAULT|LAYOUT_RIGHT|FRAME_RAISED|FRAME_THICK,0,0,0,0, 20,20);
  new GMButton(closebox,cancel,NULL,&dialog,FXDialogBox::ID_CANCEL,BUTTON_DEFAULT|LAYOUT_RIGHT|FRAME_RAISED|FRAME_THICK,0,0,0,0, 20,20);
  if (dialog.execute()) {
    return TRUE;
    }
  return FALSE;
  }


void GMWindow::reset() {

  if (remote) remote->reset();

  label_nowplaying->setText(" ");
  time_progress->setText("--:--");
  time_remaining->setText("--:--");

  trackslider->disable();
  trackslider->setProgress(0);

  /// Reset Status Text
  statusbar->getStatusLine()->setNormalText("Ready.");

  /// Clear Cover
  update_cover_display();


#if APPLICATION_BETA > 0
  setTitle(FXString::value("Goggles Music Manager %d.%d.%d-beta%d",APPLICATION_MAJOR,APPLICATION_MINOR,APPLICATION_LEVEL,APPLICATION_BETA));
#else
  setTitle("Goggles Music Manager");
#endif
  }

void GMWindow::display(const GMTrack& info){


  /// Update Ticker Text
 // FXString statustext = FXString("Now playing: " + info.artist + " - " + info.title);
  //statusbar->getStatusLine()->setNormalText(statustext);
  //FXint nchars = nowplaying->getWidth() / nowplaying->getFont()->getTextWidth("8");
  //printf("nchars=%d\n",nchars);
  //nowplaying->setText(info.artist + " - " + info.title);
  FXUTF8Codec codec;
  FXString title = GMFilename::format_track(info,GMPlayerManager::instance()->getPreferences().gui_format_title,FXString::null,0,&codec);
  label_nowplaying->setText(title);

  if (GMPlayerManager::instance()->getPreferences().gui_show_playing_titlebar){

#if APPLICATION_BETA > 0
    setTitle(FXString::value("%s ~ Goggles Music Manager %d.%d.%d-beta%d",title.text(),APPLICATION_MAJOR,APPLICATION_MINOR,APPLICATION_LEVEL,APPLICATION_BETA));
#else
    setTitle(FXString::value("%s ~ Goggles Music Manager",title.text()));
#endif
    }
  else {
#if APPLICATION_BETA > 0
    setTitle(FXString::value("Goggles Music Manager %d.%d.%d-beta%d",APPLICATION_MAJOR,APPLICATION_MINOR,APPLICATION_LEVEL,APPLICATION_BETA));
#else
    setTitle("Goggles Music Manager");
#endif
    }

  if (remote) remote->display(info);
  }

void GMWindow::update_volume_display(FXint level) {
  if (level<=0)
    volumebutton->setIcon(icontheme->icon_volume_muted_toolbar);
  else if (level<=33)
    volumebutton->setIcon(icontheme->icon_volume_low_toolbar);
  else if (level<=66)
    volumebutton->setIcon(icontheme->icon_volume_medium_toolbar);
  else
    volumebutton->setIcon(icontheme->icon_volume_high_toolbar);

  volumeslider->setValue(level);

  if (remote) remote->update_volume_display(level);
  }
#if 0
void GMWindow::update_time_display() {
  FXint display;
  const GMPlayer * player = GMPlayerManager::instance()->getPlayer();
  FXASSERT(player);
  if (player->playing()) {
    display = player->getCurrentTime();

    hours 	  = (FXint) floor((double)display/3600000.0);
    display  -= (FXint) (3600000.0*hours);
    minutes   = (FXint) floor((double)display/60000.0);
    display  -= (FXint) (60000.0*minutes);
    seconds   = (FXint) floor((double)display/1000.0);

    if (hours>0)
      timelabel->setText(FXString::value("%d:%.2d:%.2d",hours,minutes,seconds));
    else
      timelabel->setText(FXString::value("%.2d:%.2d",minutes,seconds));



    }





  }
  #endif

void GMWindow::update_time(const TrackTime & c,const TrackTime & r,FXint position,FXbool playing,FXbool seekable) {
  if (playing) {
    if (c.hours>0)
      time_progress->setText(FXString::value("%d:%.2d:%.2d",c.hours,c.minutes,c.seconds));
    else
      time_progress->setText(FXString::value("%.2d:%.2d",c.minutes,c.seconds));

    if (position) {
      if (r.hours>0)
        time_remaining->setText(FXString::value("-%d:%.2d:%.2d",r.hours,r.minutes,r.seconds));
      else
        time_remaining->setText(FXString::value("-%.2d:%.2d",r.minutes,r.seconds));
      }

    if (seekable) {
      if (!trackslider->grabbed()){
        trackslider->setProgress(position);
        }
      trackslider->enable();
      }
    else {
      trackslider->disable();
      }
    }
  else {
    time_remaining->setText("--:--");
    time_progress->setText("--:--");
    trackslider->disable();
    trackslider->setProgress(0);
    }
  if (remote) remote->update_time(c,r,position,playing,seekable);
  }

long GMWindow::onCmdQuit(FXObject *,FXSelector,void*){
  GM_DEBUG_PRINT("GMWindow::onCmdQuit\n");

  sourceview->saveView();
  trackview->saveView();

  /// Save the Window State
  getApp()->reg().writeBoolEntry("window","remote",(remote==NULL) ? false : true);
  if (remote)
    getApp()->reg().writeBoolEntry("window","window-show",remote->shown());
  else
    getApp()->reg().writeBoolEntry("window","window-show",shown());

  if (!isFullScreen() && !isMaximized() && !isMinimized() &&shown()) {
    getApp()->reg().writeIntEntry("window","x",getX());
    getApp()->reg().writeIntEntry("window","y",getY());
    getApp()->reg().writeIntEntry("window","width",getWidth());
    getApp()->reg().writeIntEntry("window","height",getHeight());
    }
  getApp()->reg().writeBoolEntry("window","fullscreen",isFullScreen());
  getApp()->reg().writeBoolEntry("window","maximized",isMaximized());
  getApp()->reg().writeIntEntry("window","source-list-hsplit",mainsplitter->getHSplit());


  /// Get rid of remote
  if (remote) {
    remote->writeRegistry();
    delete remote;
    remote = NULL;
    }

  volumeslider->setTarget(NULL);
  volumeslider->setSelector(0);
  volumebutton->setMenu(NULL);
  delete volumecontrol;

  GMPlayerManager::instance()->exit();
  return 1;
  }



long GMWindow::onCmdAbout(FXObject *,FXSelector,void*){
  GMAboutDialog dialog(this);
  dialog.execute(PLACEMENT_SCREEN);
  return 1;
  }


long GMWindow::onCmdShowFullScreen(FXObject*,FXSelector,void*){
  if (isFullScreen()) {
    setFullScreen(false);
    }
  else {
    setFullScreen(true);
    }
  return 1;
  }

long GMWindow::onCmdShowBrowser(FXObject*,FXSelector,void*){
  hideRemote();
  return 1;
  }

long GMWindow::onCmdShowMiniPlayer(FXObject*,FXSelector,void*){
  showRemote();
  hide();
  return 1;
  }

long GMWindow::onUpdShowMiniPlayer(FXObject*sender,FXSelector,void*){
  if (remote)
    sender->handle(this,FXSEL(SEL_COMMAND,FXWindow::ID_CHECK),NULL);
  else
    sender->handle(this,FXSEL(SEL_COMMAND,FXWindow::ID_UNCHECK),NULL);
  return 1;
  }

long GMWindow::onCmdResetColors(FXObject*,FXSelector,void*){
  GMPlayerManager::instance()->getPreferences().resetColors();
  return 1;
  }

long GMWindow::onCmdPreferences(FXObject *,FXSelector,void*){
  GMPreferencesDialog dialog(this);
  dialog.execute();
  return 1;
  }



void GMWindow::configureStatusbar(FXbool show){
  if (show) {
    statusbar->show();
    //controlstatusseparator->hide();
    controldragcorner->hide();
    }
  else {
    statusbar->hide();
    //controlstatusseparator->hide();
    if (!GMPlayerManager::instance()->getPreferences().gui_toolbar_docktop){
      controldragcorner->show();
      toolbar->recalc();
      }
    ///ticker->setSpeed(0);
    }
  GMPlayerManager::instance()->getPreferences().gui_show_status_bar=show;
  recalc();
  toolbar->recalc();
  }



void GMWindow::configureToolbar(FXbool docktop,FXbool init/*=false*/){
  if ((docktop != GMPlayerManager::instance()->getPreferences().gui_toolbar_docktop) || init) {
    if (docktop) {
      toolbar->setLayoutHints(LAYOUT_FILL_X|LAYOUT_SIDE_TOP);
      controldragcorner->hide();
      menubutton_library->setAttachment(MENUBUTTON_ATTACH_LEFT);
      //menubutton_view->setAttachment(MENUBUTTON_ATTACH_LEFT);
      menubutton_gmm->setAttachment(MENUBUTTON_ATTACH_RIGHT);
      menubutton_media->setAttachment(MENUBUTTON_ATTACH_RIGHT);
      menubutton_library->setPopupStyle(MENUBUTTON_DOWN);
      //menubutton_view->setPopupStyle(MENUBUTTON_DOWN);
      menubutton_gmm->setPopupStyle(MENUBUTTON_DOWN);
      menubutton_media->setPopupStyle(MENUBUTTON_DOWN);
      }
    else {
      toolbar->setLayoutHints(LAYOUT_FILL_X|LAYOUT_SIDE_BOTTOM);
      if (!statusbar->shown()) controldragcorner->show();
      menubutton_library->setAttachment(MENUBUTTON_ATTACH_LEFT);
      //menubutton_view->setAttachment(MENUBUTTON_ATTACH_LEFT);
      menubutton_gmm->setAttachment(MENUBUTTON_ATTACH_RIGHT);
      menubutton_media->setAttachment(MENUBUTTON_ATTACH_RIGHT);
      menubutton_library->setPopupStyle(MENUBUTTON_UP);
      //menubutton_view->setPopupStyle(MENUBUTTON_UP);
      menubutton_gmm->setPopupStyle(MENUBUTTON_UP);
      menubutton_media->setPopupStyle(MENUBUTTON_UP);
      }
    }
  GMPlayerManager::instance()->getPreferences().gui_toolbar_docktop=docktop;
  }


long GMWindow::onCmdImport(FXObject *,FXSelector sel,void*){


  FXuint mode=(FXSELID(sel)==ID_IMPORT_DIRS || FXSELID(sel)==ID_SYNC_DIRS) ? IMPORT_FROMDIR : IMPORT_FROMFILE;

  if (FXSELID(sel)==ID_SYNC_DIRS)
    mode|=IMPORT_SYNC;

  GMImportDialog dialog(this,mode);
  if (dialog.execute()) {
    FXStringList files;
    dialog.getSelectedFiles(files);

    if (FXSELID(sel)==ID_SYNC_DIRS) {
      GMSyncTask * task = new GMSyncTask(GMPlayerManager::instance(),GMPlayerManager::ID_IMPORT_TASK);
      task->setOptions(GMPlayerManager::instance()->getPreferences().import);
      task->setSyncOptions(GMPlayerManager::instance()->getPreferences().sync);
      task->setInput(files);
      GMPlayerManager::instance()->runTask(task);
      }
    else {
      GMImportTask * task = new GMImportTask(GMPlayerManager::instance(),GMPlayerManager::ID_IMPORT_TASK);
      task->setOptions(GMPlayerManager::instance()->getPreferences().import);
      task->setInput(files);
      GMPlayerManager::instance()->runTask(task);
      }
    }
  return 1;
  }

long GMWindow::onCmdPlayPause(FXObject*,FXSelector,void*){
  if (GMPlayerManager::instance()->can_pause())
    GMPlayerManager::instance()->pause();
  else if (GMPlayerManager::instance()->can_unpause())
    GMPlayerManager::instance()->unpause();
  else
    GMPlayerManager::instance()->playItem(TRACK_CURRENT);
  return 1;
  }

long GMWindow::onUpdPlayPause(FXObject*sender,FXSelector,void*){
  if ( GMPlayerManager::instance()->can_play() || GMPlayerManager::instance()->can_unpause() || GMPlayerManager::instance()->can_pause()){
    sender->handle(this,FXSEL(SEL_COMMAND,FXWindow::ID_ENABLE),NULL);
    if (GMPlayerManager::instance()->can_play() || GMPlayerManager::instance()->can_unpause())
      sender->handle(this,FXSEL(SEL_COMMAND,FXWindow::ID_UNCHECK),NULL);
    else
      sender->handle(this,FXSEL(SEL_COMMAND,FXWindow::ID_CHECK),NULL);
    }
  else {
    sender->handle(this,FXSEL(SEL_COMMAND,FXWindow::ID_DISABLE),NULL);
    sender->handle(this,FXSEL(SEL_COMMAND,FXWindow::ID_UNCHECK),NULL);
    }
  return 1;
  }

long GMWindow::onUpdPlayPauseMenu(FXObject*sender,FXSelector,void*){
  FXMenuCommand * menucommand=dynamic_cast<FXMenuCommand*>(sender);
  FXButton * button=dynamic_cast<FXButton*>(sender);

  if ( GMPlayerManager::instance()->can_play() || GMPlayerManager::instance()->can_unpause() || GMPlayerManager::instance()->can_pause()){
    if (GMPlayerManager::instance()->can_play() || GMPlayerManager::instance()->can_unpause()){
      if (button) {
        button->enable();
        button->setHelpText(tr("Start playback."));
        button->setIcon(GMIconTheme::instance()->icon_play);
        }
      else {
        menucommand->enable();
        menucommand->setText(tr("Play"));
        menucommand->setHelpText(tr("Start playback"));
        menucommand->setTipText(tr("Start playback"));
        menucommand->setIcon(GMIconTheme::instance()->icon_play);
        }
      }
    else {
      if (button) {
        button->enable();
        button->setHelpText(tr("Pause playback."));
        button->setTipText(tr("Pause playback"));
        button->setIcon(GMIconTheme::instance()->icon_pause);
        }
      else {
        menucommand->enable();
        menucommand->setText(tr("Pause"));
        menucommand->setHelpText(tr("Pause playback."));
        menucommand->setIcon(GMIconTheme::instance()->icon_pause);
        }
      }
    }
  else {
    if (button) {
      button->setHelpText(tr("Start playback."));
      button->setIcon(GMIconTheme::instance()->icon_play);
      button->disable();
      }
    else {
      menucommand->setText(tr("Play"));
      menucommand->setHelpText(tr("Start playback."));
      menucommand->setIcon(GMIconTheme::instance()->icon_play);
      menucommand->disable();
      }
    }
  return 1;
  }


long GMWindow::onCmdPause(FXObject*,FXSelector,void*){
  GMPlayerManager::instance()->pause();
  return 1;
  }

long GMWindow::onUpdPause(FXObject*sender,FXSelector,void*){
  if (GMPlayerManager::instance()->can_pause())
    sender->handle(this,FXSEL(SEL_COMMAND,FXWindow::ID_SHOW),NULL);
  else
    sender->handle(this,FXSEL(SEL_COMMAND,FXWindow::ID_HIDE),NULL);
  return 1;
  }

long GMWindow::onCmdStop(FXObject*,FXSelector,void*){
  GMPlayerManager::instance()->stop();
  return 1;
  }

long GMWindow::onUpdStop(FXObject*sender,FXSelector,void*){
  if (GMPlayerManager::instance()->can_stop())
    sender->handle(this,FXSEL(SEL_COMMAND,FXWindow::ID_ENABLE),NULL);
  else
    sender->handle(this,FXSEL(SEL_COMMAND,FXWindow::ID_DISABLE),NULL);
  return 1;
  }

long GMWindow::onCmdNext(FXObject*,FXSelector,void*){
  GMPlayerManager::instance()->playItem(TRACK_NEXT);
  return 1;
  }

long GMWindow::onUpdNext(FXObject*sender,FXSelector,void*){
  if (GMPlayerManager::instance()->can_next())
    sender->handle(this,FXSEL(SEL_COMMAND,FXWindow::ID_ENABLE),NULL);
  else
    sender->handle(this,FXSEL(SEL_COMMAND,FXWindow::ID_DISABLE),NULL);
  return 1;
  }

long GMWindow::onCmdPrev(FXObject*,FXSelector,void*){
  GMPlayerManager::instance()->playItem(TRACK_PREVIOUS);
  return 1;
  }

long GMWindow::onUpdPrev(FXObject*sender,FXSelector,void*){
  if (GMPlayerManager::instance()->can_prev())
    sender->handle(this,FXSEL(SEL_COMMAND,FXWindow::ID_ENABLE),NULL);
  else
    sender->handle(this,FXSEL(SEL_COMMAND,FXWindow::ID_DISABLE),NULL);
  return 1;
  }


long GMWindow::onCmdSeek(FXObject*,FXSelector,void*){
#if 0
#ifdef HAVE_XINE_LIB
  if (GMPlayerManager::instance()->getPlayer()->playing() && GMPlayerManager::instance()->getPlayer()->seekable()){
    FXint pos=GMPlayerManager::instance()->getPlayer()->getPositionMS();
    switch(FXSELID(sel)) {
      case ID_SEEK_FORWARD_10SEC  : pos+=10000; break;
      case ID_SEEK_FORWARD_1MIN   : pos+=60000; break;
      case ID_SEEK_BACKWARD_10SEC : pos-=10000; break;
      case ID_SEEK_BACKWARD_1MIN  : pos-=60000; break;
      }
    pos = pos / (double) (GMPlayerManager::instance()->getPlayer()->getTotalTime() / (double) 0xFFFF);
    pos = FXCLAMP(0,pos,0xFFFF);
    GMPlayerManager::instance()->seek(pos);
    }
#endif
#endif
  return 1;
  }


long GMWindow::onCmdTimeSlider(FXObject*,FXSelector,void*ptr){
  FXdouble pos = *(FXdouble*)ptr;
  GMPlayerManager::instance()->seek(pos);
  return 1;
  }

long GMWindow::onCmdVolume(FXObject*,FXSelector,void*ptr){
  FXint level = (FXint)(FXival)ptr;
  GMPlayerManager::instance()->volume(level);
  update_volume_display(level);
  return 1;
  }

long GMWindow::onCmdVolumeButton(FXObject*,FXSelector sel,void*ptr){
  volumeslider->handle(this,FXSEL(FXSELTYPE(sel),0),ptr);
  return 1;
  }

long GMWindow::onUpdVolumeButton(FXObject*sender,FXSelector,void*){
  sender->handle(this,FXSEL(SEL_COMMAND,ID_ENABLE),NULL);
  return 1;
  }

// FIXME
long GMWindow::onCmdRepeatAB(FXObject*,FXSelector,void*){
  return 1;
  }

long GMWindow::onUpdRepeatAB(FXObject*,FXSelector,void*){
#if 0
#ifdef HAVE_XINE_LIB
  if (GMPlayerManager::instance()->getPlayer()->playing() && GMPlayerManager::instance()->getPlayer()->seekable()) {
    const FXString state_off = tr("Repeat A-B");
    const FXString state_a = tr("Repeat A");
    const FXString state_b = tr("Repeat A-B");
    sender->handle(this,FXSEL(SEL_COMMAND,ID_ENABLE),NULL);
    switch(GMPlayerManager::instance()->getPlayer()->getRepeatAB()){
      case REPEAT_AB_OFF: sender->handle(this,FXSEL(SEL_COMMAND,ID_UNCHECK),NULL);
                          sender->handle(this,FXSEL(SEL_COMMAND,ID_SETSTRINGVALUE),(void*)&state_off);
                          break;
      case REPEAT_AB_A  : sender->handle(this,FXSEL(SEL_COMMAND,ID_CHECK),NULL);
                          sender->handle(this,FXSEL(SEL_COMMAND,ID_SETSTRINGVALUE),(void*)&state_a);
                          break;
      case REPEAT_AB_B  : sender->handle(this,FXSEL(SEL_COMMAND,ID_CHECK),NULL);
                          sender->handle(this,FXSEL(SEL_COMMAND,ID_SETSTRINGVALUE),(void*)&state_b);
                          break;
      }
    }
  else {
    sender->handle(this,FXSEL(SEL_COMMAND,ID_DISABLE),NULL);
    }
#else
  sender->handle(this,FXSEL(SEL_COMMAND,ID_DISABLE),NULL);
#endif
#endif
  return 1;
  }



long GMWindow::onCmdRepeatOff(FXObject*,FXSelector,void*){
  GMPlayerManager::instance()->getPreferences().play_repeat=REPEAT_OFF;
  return 1;
  }

long GMWindow::onUpdRepeatOff(FXObject*sender,FXSelector,void*){
  if (GMPlayerManager::instance()->getPreferences().play_repeat==REPEAT_OFF)
    sender->handle(this,FXSEL(SEL_COMMAND,ID_CHECK),NULL);
  else
    sender->handle(this,FXSEL(SEL_COMMAND,ID_UNCHECK),NULL);
  return 1;
  }

long GMWindow::onCmdRepeat(FXObject*,FXSelector,void*){
  GMPlayerManager::instance()->getPreferences().play_repeat=REPEAT_TRACK;
  return 1;
  }

long GMWindow::onUpdRepeat(FXObject*sender,FXSelector,void*){
  if (GMPlayerManager::instance()->getPreferences().play_repeat==REPEAT_TRACK)
    sender->handle(this,FXSEL(SEL_COMMAND,ID_CHECK),NULL);
  else
    sender->handle(this,FXSEL(SEL_COMMAND,ID_UNCHECK),NULL);
  return 1;
  }


long GMWindow::onCmdRepeatAll(FXObject*,FXSelector,void*){
  GMPlayerManager::instance()->getPreferences().play_repeat=REPEAT_ALL;
  return 1;
  }

long GMWindow::onUpdRepeatAll(FXObject*sender,FXSelector,void*){
  if (GMPlayerManager::instance()->getPreferences().play_repeat==REPEAT_ALL)
    sender->handle(this,FXSEL(SEL_COMMAND,ID_CHECK),NULL);
  else
    sender->handle(this,FXSEL(SEL_COMMAND,ID_UNCHECK),NULL);
  return 1;
  }


long GMWindow::onCmdShuffle(FXObject*,FXSelector,void*ptr){
  GMPlayerManager::instance()->getPreferences().play_shuffle = (FXbool)(FXival)(ptr);
  return 1;
  }

long GMWindow::onUpdShuffle(FXObject*sender,FXSelector,void*){
  if (GMPlayerManager::instance()->getPreferences().play_shuffle)
    sender->handle(this,FXSEL(SEL_COMMAND,ID_CHECK),NULL);
  else
    sender->handle(this,FXSEL(SEL_COMMAND,ID_UNCHECK),NULL);
  return 1;
  }

long GMWindow::onCmdHomePage(FXObject*,FXSelector,void*){
  if (!gm_open_browser("http://code.google.com/p/gogglesmm")){
    FXMessageBox::error(this,MBOX_OK,tr("Unable to launch webbrowser"),"Goggles Music Manager was unable to launch a webbrowser.\nPlease visit http://code.google.com/p/gogglesmm for the official homepage.");
    }
  return 1;
  }

long GMWindow::onCmdReportIssue(FXObject*,FXSelector,void*){
  if (!gm_open_browser("http://code.google.com/p/gogglesmm/issues/list")){
    FXMessageBox::error(this,MBOX_OK,tr("Unable to launch webbrowser"),"Goggles Music Manager was unable to launch a webbrowser.\nPlease visit http://code.google.com/p/gogglesmm/issues/list to report an issue.");
    }
  return 1;
  }

long GMWindow::onCmdJoinLastFM(FXObject*,FXSelector,void*){
  if (!gm_open_browser("https://www.last.fm/join/")){
    FXMessageBox::error(this,MBOX_OK,tr("Unable to launch webbrowser"),"Goggles Music Manager was unable to launch a webbrowser.\nPlease visit https://www.last.fm/join/");
    }
  return 1;
  }

long GMWindow::onCmdJoinGMMLastFM(FXObject*,FXSelector,void*){
  if (!gm_open_browser("http://www.last.fm/group/Goggles+Music+Manager")){
    FXMessageBox::error(this,MBOX_OK,tr("Unable to launch webbrowser"),"Goggles Music Manager was unable to launch a webbrowser.\nPlease visit http://www.last.fm/group/Goggles+Music+Manager");
    }
  return 1;
  }

extern const FXchar gmfilepatterns[];

class GMOpenDialog : public FXDialogBox {
FXDECLARE(GMOpenDialog)
protected:
  GMTextField * input;
protected:
  GMOpenDialog(){}
private:
  GMOpenDialog(const GMOpenDialog&);
  GMOpenDialog &operator=(const GMOpenDialog&);
public:
  enum {
    ID_BROWSE = FXDialogBox::ID_LAST,
    ID_LAST
    };
public:
  long onCmdBrowse(FXObject*,FXSelector,void*);
  long onCmdAccept(FXObject*,FXSelector,void*);
public:
  GMOpenDialog(FXWindow*);

  FXString getFilename() const;
  };

FXDEFMAP(GMOpenDialog) GMOpenDialogMap[]={
  FXMAPFUNC(SEL_COMMAND,GMOpenDialog::ID_BROWSE,GMOpenDialog::onCmdBrowse),
  FXMAPFUNC(SEL_COMMAND,GMOpenDialog::ID_ACCEPT,GMOpenDialog::onCmdAccept)
  };

FXIMPLEMENT(GMOpenDialog,FXDialogBox,GMOpenDialogMap,ARRAYNUMBER(GMOpenDialogMap));


GMOpenDialog::GMOpenDialog(FXWindow*p) : FXDialogBox(p,fxtr("Play File or Stream"),DECOR_TITLE|DECOR_BORDER,0,0,0,0,4,4,4,4) {
  FXHorizontalFrame *closebox=new FXHorizontalFrame(this,LAYOUT_SIDE_BOTTOM|LAYOUT_FILL_X|PACK_UNIFORM_WIDTH,0,0,0,0,0,0,0,0);
  new GMButton(closebox,tr("&Play"),NULL,this,FXDialogBox::ID_ACCEPT,BUTTON_INITIAL|BUTTON_DEFAULT|LAYOUT_RIGHT|FRAME_RAISED|FRAME_THICK,0,0,0,0, 15,15);
  new GMButton(closebox,tr("&Cancel"),NULL,this,FXDialogBox::ID_CANCEL,BUTTON_DEFAULT|LAYOUT_RIGHT|FRAME_RAISED|FRAME_THICK,0,0,0,0, 15,15);
  new FXSeparator(this,SEPARATOR_GROOVE|LAYOUT_FILL_X|LAYOUT_SIDE_BOTTOM);

  FXVerticalFrame * vframe = new FXVerticalFrame(this,LAYOUT_FILL_X|LAYOUT_FILL_Y);
  new FXLabel(vframe,tr("Please specify a file or url to play:"),NULL);

  FXHorizontalFrame *inputframe=new FXHorizontalFrame(vframe,LAYOUT_FILL_X,0,0,0,0,0,0,0,0);
  input = new GMTextField(inputframe,40,NULL,0,TEXTFIELD_NORMAL|LAYOUT_FILL_X|LAYOUT_CENTER_Y);
  new GMButton(inputframe,tr("…"),NULL,this,ID_BROWSE);
  input->setText(getApp()->reg().readStringEntry("GMOpenDialog","url",NULL));
  }


long GMOpenDialog::onCmdBrowse(FXObject*,FXSelector,void*){
  GMFileDialog filedialog(this,tr("Select File"));
  if (!input->getText().empty())
    filedialog.setFilename(input->getText());
  filedialog.setPatternList(gmfilepatterns);
  if (filedialog.execute())
    input->setText(filedialog.getFilename());
  return 1;
  }

long GMOpenDialog::onCmdAccept(FXObject*sender,FXSelector sel,void*ptr){
  getApp()->reg().writeStringEntry("GMOpenDialog","url",input->getText().text());
  return FXDialogBox::onCmdAccept(sender,sel,ptr);
  }

FXString GMOpenDialog::getFilename() const {
  return input->getText();
  }

long GMWindow::onCmdOpen(FXObject*,FXSelector,void*){
  GMOpenDialog dialog(this);
  if (dialog.execute()) {
    FXString filename = dialog.getFilename();
    GMPlayerManager::instance()->open(filename);
    }
  return 1;
  }


long GMWindow::onCmdSleepTimer(FXObject*,FXSelector,void*ptr){
  if (((FXint)(FXival)ptr)==1) {
    FXDialogBox dialog(this,tr("Sleep Timer"),DECOR_TITLE|DECOR_BORDER,0,0,0,0,0,0,0,0,0,0);
    create_dialog_header(&dialog,tr("Setup sleep timer"),tr("Stop playback within a certain time"),NULL);
    FXHorizontalFrame *closebox=new FXHorizontalFrame(&dialog,LAYOUT_SIDE_BOTTOM|LAYOUT_FILL_X,0,0,0,0);
    new GMButton(closebox,tr("&Start Timer"),NULL,&dialog,FXDialogBox::ID_ACCEPT,BUTTON_INITIAL|BUTTON_DEFAULT|LAYOUT_RIGHT|FRAME_RAISED|FRAME_THICK,0,0,0,0, 15,15);
    new GMButton(closebox,tr("&Cancel"),NULL,&dialog,FXDialogBox::ID_CANCEL,BUTTON_DEFAULT|LAYOUT_RIGHT|FRAME_RAISED|FRAME_THICK,0,0,0,0, 15,15);
    new FXSeparator(&dialog,SEPARATOR_GROOVE|LAYOUT_FILL_X|LAYOUT_SIDE_BOTTOM);

    FXHorizontalFrame * main = new FXHorizontalFrame(&dialog,LAYOUT_FILL_X,0,0,0,0,40,40,10,10);
    new FXLabel(main,tr("Sleep in"),NULL,LABEL_NORMAL|LAYOUT_CENTER_Y);
    FXSpinner* hours = new FXSpinner(main,2,NULL,0,FRAME_SUNKEN|FRAME_THICK);
    new FXLabel(main,tr("hours and"),NULL,LABEL_NORMAL|LAYOUT_CENTER_Y);
    FXSpinner* minutes = new FXSpinner(main,2,NULL,0,FRAME_SUNKEN|FRAME_THICK);
    new FXLabel(main,tr("minutes."),NULL,LABEL_NORMAL|LAYOUT_CENTER_Y);
    minutes->setRange(0,59);
    hours->setRange(0,24);

    minutes->setValue(FXCLAMP(0,getApp()->reg().readIntEntry("player","sleeptimer-minutes",0),59));
    hours->setValue(FXCLAMP(0,getApp()->reg().readIntEntry("player","sleeptimer-hours",2),24));

    if (dialog.execute()) {
      GMPlayerManager::instance()->setSleepTimer(TIME_HOUR(hours->getValue())+TIME_MIN(minutes->getValue()));
      getApp()->reg().writeIntEntry("player","sleeptimer-minutes",minutes->getValue());
      getApp()->reg().writeIntEntry("player","sleeptimer-hours",hours->getValue());
      }
    }
  else {
    GMPlayerManager::instance()->setSleepTimer(0);
    }
  return 1;
  }
long GMWindow::onUpdSleepTimer(FXObject*sender,FXSelector,void*){
  if (GMPlayerManager::instance()->hasSleepTimer())
    sender->handle(this,FXSEL(SEL_COMMAND,ID_CHECK),NULL);
  else
    sender->handle(this,FXSEL(SEL_COMMAND,ID_UNCHECK),NULL);
  return 1;
  }


void GMWindow::clearCover() {
  if (coverview_x11 && coverview_x11->getImage()) {
    delete coverview_x11->getImage();
    coverview_x11->setImage(NULL);
    }
#ifdef HAVE_OPENGL_COVERVIEW
  else if (coverview_gl) {
    coverview_gl->setImage(NULL);
    }
#endif
  }


void GMWindow::update_cover_display() {
  GMCover * cover = GMPlayerManager::instance()->getCoverManager()->getCover();
  if (cover) {
    if (!coverframe->shown()) {
      if (coverview_x11) {
          coverview_x11->setTarget(NULL);
          }

      coverframe->show();
      coverframe->recalc();

      layout();
      updateCover();// gets called by SEL_CONFIGURE event

      if (coverview_x11) {
          coverview_x11->setTarget(this);
          }
      }
    else {
      updateCover();
      }
    }
  else {
    clearCover();
    if (coverframe->shown()) {
      coverframe->hide();
      coverframe->recalc();
      }
    }

  if (remote)
    remote->update_cover_display();
  }


void GMWindow::updateCover() {
  GMCover * cover = GMPlayerManager::instance()->getCoverManager()->getCover();

  // clear old
  clearCover();

  // load new
  if (cover) {
    FXint size ;

    if (coverview_x11)
      size = FXCLAMP(64,FXMIN(coverview_x11->getWidth(),coverview_x11->getHeight()),500);
    else
      size = 0;

    FXImage * image = GMCover::copyToImage(cover,size);
    if (coverview_x11) {
      image->create();
      coverview_x11->setImage(image);
      }
#ifdef HAVE_OPENGL_COVERVIEW
    else {
      coverview_gl->setImage(image);
      delete image;
      }
#endif
    }
  }

void GMWindow::updateCoverView() {
#ifdef HAVE_OPENGL_COVERVIEW
  if (GMPlayerManager::instance()->getPreferences().gui_show_opengl_coverview && FXGLVisual::hasOpenGL(getApp())) {

    if (coverview_x11) {
      clearCover();
      delete coverview_x11;
      coverview_x11=NULL;
      }

    if (!coverview_gl) {
      glvisual     = new FXGLVisual(getApp(),VISUAL_DOUBLE_BUFFER);
      coverview_gl = new GMImageView(coverframe,glvisual,LAYOUT_FILL_X|LAYOUT_FILL_Y);
      coverview_gl->setTarget(this);
      coverview_gl->setSelector(ID_COVERVIEW);
      coverview_gl->enable();
      if (coverframe->id()) coverview_gl->create();
      }

    }
  else {

    if (coverview_gl) {
      clearCover();
      delete coverview_gl;
      coverview_gl=NULL;
      delete glvisual;
      glvisual=NULL;
      }
#endif
    if (!coverview_x11) {
      coverview_x11 = new GMImageFrame(coverframe,NULL,FRAME_NONE|JUSTIFY_CENTER_X|JUSTIFY_CENTER_Y|LAYOUT_FILL_X|LAYOUT_FILL_Y,0,0,0,0,0,0,0,0);
      coverview_x11->setBackColor(getApp()->getBackColor());
      coverview_x11->setTarget(this);
      coverview_x11->setSelector(ID_COVERVIEW);
      coverview_x11->enable();
      if (coverframe->id()) coverview_x11->create();
      }
#ifdef HAVE_OPENGL_COVERVIEW
    }
#endif
  }

long GMWindow::onConfigureCoverView(FXObject*,FXSelector sel,void*){
  if (coverview_x11 && GMPlayerManager::instance()->playing()) {
    if (FXSELID(sel)==ID_COVERVIEW && coverview_x11->getUserData()==NULL) {
      getApp()->addTimeout(this,ID_REFRESH_COVERVIEW,TIME_MSEC(50));
      }
    else {
      GM_DEBUG_PRINT("configure cover view %d %d\n",coverview_x11->getWidth(),coverview_x11->getHeight());
      updateCover();
      }
    }
  return 1;
  }



long GMWindow::onCmdPlayQueue(FXObject*,FXSelector,void*){
  GMPlayerManager::instance()->setPlayQueue(GMPlayerManager::instance()->getPlayQueue()==NULL);
  return 1;
  }

long GMWindow::onUpdPlayQueue(FXObject*sender,FXSelector,void*){
  if (GMPlayerManager::instance()->getPlayQueue())
    sender->handle(this,FXSEL(SEL_COMMAND,ID_CHECK),NULL);
  else
    sender->handle(this,FXSEL(SEL_COMMAND,ID_UNCHECK),NULL);
  return 1;
  }


void GMWindow::setStatus(const FXString & msg){
  if (!msg.empty()) {
    if (!progressbar->shown()) {
      progressbar->show();
      progressbar_animation->show();
      }
    progressbar_label->setText(msg);
    }
  else {
    progressbar->hide();
    progressbar_animation->hide();
    progressbar_label->setText(FXString::null);
    }
  }



void GMWindow::create_dialog_header(FXDialogBox * dialog,const FXString & title,const FXString & subtitle,FXIcon * icon) {
  FXHorizontalFrame * header = new FXHorizontalFrame(dialog,LAYOUT_FILL_X,0,0,0,0,0,0,0,0,0,0);
  header->setBackColor(getApp()->getBackColor());
  FXLabel * label = new FXLabel(header,FXString::null,icon,LABEL_NORMAL|LAYOUT_CENTER_Y,0,0,0,0,10,0,0,0);
  label->setBackColor(getApp()->getBackColor());
  FXVerticalFrame * frame = new FXVerticalFrame(header,LAYOUT_FILL_X,0,0,0,0,0,0,0,0,0,0);
  label = new FXLabel(frame,title,NULL,LAYOUT_FILL_X|JUSTIFY_LEFT|TEXT_AFTER_ICON,0,0,0,0,10,10,10,0);
  label->setBackColor(getApp()->getBackColor());
  label->setFont(GMApp::instance()->getThickFont());
  label = new FXLabel(frame,subtitle,NULL,LAYOUT_FILL_X|JUSTIFY_LEFT|TEXT_AFTER_ICON,0,0,0,0,10+30,10,0,10);
  label->setBackColor(getApp()->getBackColor());
  new FXSeparator(dialog,LAYOUT_FILL_X|SEPARATOR_GROOVE|LAYOUT_SIDE_TOP);
  }


long GMWindow::onCmdShowSources(FXObject*,FXSelector,void*){
  if (mainsplitter->getExpanded()==HIDESOURCES) {
    if (GMPlayerManager::instance()->getPreferences().gui_show_playing_albumcover && GMPlayerManager::instance()->getCoverManager()->getCover())
      mainsplitter->setExpanded(SHOWSOURCES_COVER);
    else
      mainsplitter->setExpanded(SHOWSOURCES);
    }
  else
    mainsplitter->setExpanded(HIDESOURCES);
  return 1;
  }

long GMWindow::onUpdShowSources(FXObject*sender,FXSelector,void*){
  FXuint exp = mainsplitter->getExpanded();
  if (exp==SHOWSOURCES || exp==SHOWSOURCES_COVER)
    sender->handle(this,FXSEL(SEL_COMMAND,ID_CHECK),NULL);
  else
    sender->handle(this,FXSEL(SEL_COMMAND,ID_UNCHECK),NULL);
  return 1;
  }



FXbool GMWindow::showSources() const {
  FXuint exp = mainsplitter->getExpanded();
  if (exp==SHOWSOURCES || exp==SHOWSOURCES_COVER)
    return true;
  else
    return false;
  }


/*
  sourcelist
  genrelist
  artistlist
  albumlist
  tracklist
*/


long GMWindow::onCmdNextFocus(FXObject*,FXSelector,void*){
  focusNext();
  return 1;
  }

void GMWindow::focusNext() {
  if (!trackview->focusNext() && showSources())
    sourceview->focusNext();
  }

void GMWindow::focusPrevious() {
  if (!trackview->focusPrevious() && showSources())
    sourceview->focusPrevious();
  }



