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
#include "gmdefs.h"
#include "gmutils.h"

#include "icons.h"

#include <FXPNGIcon.h>

#include "GMTrack.h"
#include "GMApp.h"
#include "GMCoverCache.h"
#include "GMAudioPlayer.h"
#include "GMAlbumList.h"
#include "GMTrackList.h"
#include "GMList.h"
#include "GMSource.h"
#include "GMPlayerManager.h"
#include "GMWindow.h"
#include "GMRemote.h"
#include "GMDatabaseSource.h"
#include "GMPodcastSource.h"
#include "GMTrackView.h"
#include "GMSourceView.h"
#ifdef HAVE_OPENGL
#include "GMImageView.h"
#endif
#include "GMAudioScrobbler.h"
#include "GMIconTheme.h"
#include "GMFontDialog.h"
#include "GMPreferencesDialog.h"
#include "GMTrayIcon.h"


#define MINUTES 60000000000LL


enum {
  ALSA_DRIVER   = 0,
  OSS_DRIVER    = 1,
  PULSE_DRIVER  = 2,
  JACK_DRIVER   = 3,
  OTHER_DRIVER  = 4
  };



ColorTheme::ColorTheme() {
  base      = FXApp::instance()->getBaseColor();
  border    = FXApp::instance()->getBorderColor();
  back      = FXApp::instance()->getBackColor();
  fore      = FXApp::instance()->getForeColor();
  selfore	  = FXApp::instance()->getSelforeColor();
  selback	  = FXApp::instance()->getSelbackColor();
  tipfore	  = FXApp::instance()->getTipforeColor();
  tipback	  = FXApp::instance()->getTipbackColor();
  menufore  = FXApp::instance()->getSelMenuTextColor();
  menuback  = FXApp::instance()->getSelMenuBackColor();
  menubase  = GMPlayerManager::instance()->getPreferences().gui_menu_base_color;
  shadow    = FXApp::instance()->getShadowColor();
  hilite    = FXApp::instance()->getHiliteColor();
  playfore  = GMPlayerManager::instance()->getPreferences().gui_playtext_color;
  playback  = GMPlayerManager::instance()->getPreferences().gui_play_color;
  altback   = GMPlayerManager::instance()->getPreferences().gui_row_color;
  trayback  = GMPlayerManager::instance()->getPreferences().gui_tray_color;
  }

ColorTheme::ColorTheme(const FXchar * _name,FXColor _base,FXColor _border,FXColor _back,FXColor _altback,FXColor _fore,FXColor _selback,FXColor _selfore,FXColor _tipback,FXColor _tipfore,FXColor _psback,FXColor _psfore) :
  name(_name),
  base(_base),
  border(_border),
  back(_back),
  altback(_altback),
  fore(_fore),
  selback(_selback),
  selfore(_selfore),
  tipback(_tipback),
  tipfore(_tipfore),
  menuback(_selback),
  menufore(_selfore),
  menubase(_back),
  playfore(_psfore),
  playback(_psback),
  hilite(makeHiliteColor(base)),
  shadow(makeShadowColor(base)),
  trayback(_base) {
  }


void ColorTheme::save() const {
  FXApp::instance()->setBaseColor(base);
  FXApp::instance()->setBorderColor(border);
  FXApp::instance()->setBackColor(back);
  FXApp::instance()->setForeColor(fore);
  FXApp::instance()->setSelbackColor(selback);
  FXApp::instance()->setSelforeColor(selfore);
  FXApp::instance()->setTipbackColor(tipback);
  FXApp::instance()->setTipforeColor(tipfore);
  FXApp::instance()->setSelMenuBackColor(menuback);
  FXApp::instance()->setSelMenuTextColor(menufore);
  FXApp::instance()->setShadowColor(shadow);
  FXApp::instance()->setHiliteColor(hilite);
  GMPlayerManager::instance()->getPreferences().gui_playtext_color=playfore;
  GMPlayerManager::instance()->getPreferences().gui_play_color=playback;
  GMPlayerManager::instance()->getPreferences().gui_row_color=altback;
  GMPlayerManager::instance()->getPreferences().gui_menu_base_color=menubase;
  GMPlayerManager::instance()->getPreferences().gui_tray_color=trayback;
  }

bool operator==(const ColorTheme& t1,const ColorTheme& t2) {
  return (t1.base==t2.base &&
      t1.border==t2.border &&
      t1.back==t2.back &&
      t1.altback==t2.altback &&
      t1.fore==t2.fore &&
      t1.selback==t2.selback &&
      t1.selfore==t2.selfore &&
      t1.tipback==t2.tipback &&
      t1.tipfore==t2.tipfore &&
      t1.menuback==t2.menuback &&
      t1.menufore==t2.menufore &&
      t1.menubase==t2.menubase &&
      t1.playfore==t2.playfore &&
      t1.playback==t2.playback &&
      t1.hilite==t2.hilite &&
      t1.shadow==t2.shadow &&
      t1.trayback==t2.trayback);
  }


/// Think you have a great color theme, mail them to s.jansen@gmail.com
const ColorTheme ColorThemes[]={
  ColorTheme("FOX", // name
             FXRGB(212,208,200), // base
             FXRGB(  0,  0,  0), // boder
             FXRGB(255,255,255), // back
             FXRGB(240,240,240), // alt back
             FXRGB(  0,  0,  0), // fore
             FXRGB( 10, 36,106), // selback
             FXRGB(255,255,255), // selfore
             FXRGB(255,255,225), // tipback
             FXRGB(0,0,0)), // tipfore

  ColorTheme("Clearlooks"     , // name
             FXRGB(237,236,235), // base
             FXRGB(  0,  0,  0), // boder
             FXRGB(255,255,255), // back
             FXRGB(240,240,240), // alt back
             FXRGB( 26, 26, 25), // fore
             FXRGB(134,171,217), // selback
             FXRGB(255,255,255), // selfore
             FXRGB(245,245,181), // tipback
             FXRGB(  0,  0,  0)),   // tipfore

  ColorTheme("Honeycomb"       , // name
             FXRGB(213,215,209), // base
             FXRGB(  0,  0,  0), // boder
             FXRGB(255,255,255), // back
             FXRGB(238,238,238), // alt back
             FXRGB( 0, 0, 0), // fore
             FXRGB(227,167,  0), // selback
             FXRGB(255,255,255), // selfore
             FXRGB(255,242,153), // tipback
             FXRGB( 64, 48, 0)), // tipfore

  ColorTheme("Norway"          , // name
             FXRGB(235,226,210), // base
             FXRGB(  0,  0,  0), // boder
             FXRGB(253,252,251), // back
             FXRGB(238,238,238), // alt back
             FXRGB(  0,  0,  0), // fore
             FXRGB( 29,135,205), // selback
             FXRGB(255,255,255), // selfore
             FXRGB(253,252,251), // tipback
             FXRGB(  0,  0,  0)),   // tipfore

  ColorTheme("Oxygen"          , // name
             FXRGB(224,223,223), // base
             FXRGB(  0,  0,  0), // boder
             FXRGB(255,255,255), // back
             FXRGB(238,238,238), // alt back
             FXRGB( 20, 19, 18), // fore
             FXRGB( 65,141,212), // selback
             FXRGB(255,255,255), // selfore
             FXRGB(192,218,255), // tipback
             FXRGB( 20, 19, 18)), // tipfore

  ColorTheme("Obsidian Coast" ,  // name
             FXRGB( 48, 47, 47), // base
             FXRGB(  0,  0,  0), // boder
             FXRGB( 32, 31, 31), // back
             FXRGB( 38, 38, 38), // alt back
             FXRGB(224,223,220), // fore
             FXRGB(  24,72,128), // selback
             FXRGB(255,255,255), // selfore
             FXRGB( 16, 48, 80), // tipback
             FXRGB(196,209,224),// tipfore
             FXRGB(24,128,73), // psback
             FXRGB(224,223,220)), // psfore

  ColorTheme("Steel"       , // name
             FXRGB(224,223,216), // base
             FXRGB(  0,  0,  0), // boder
             FXRGB(255,255,255), // back
             FXRGB(238,238,238), // alt back
             FXRGB( 0, 0, 0), // fore
             FXRGB(123,161,173), // selback
             FXRGB(255,255,255), // selfore
             FXRGB(220,231,235), // tipback
             FXRGB( 37, 34, 28)), // tipfore

  ColorTheme("Wonton Soup"     , // name
             FXRGB (71, 76, 86), // base
             FXRGB(  0,  0,  0), // boder
             FXRGB( 58, 62, 70), // back
             FXRGB( 62, 66, 75), // alt back
             FXRGB(182,193,208), // fore
             FXRGB(117,133,153), // selback
             FXRGB(209,225,244), // selfore
             FXRGB(182,193,208), // tipback
             FXRGB(42,44,48),		 // tipfore
             FXRGB(134,147,134), // psback
             FXRGB(209,225,244)),// psfore
  };



FXDEFMAP(GMPreferencesDialog) GMPreferencesDialogMap[]={
  FXMAPFUNC(SEL_COMMAND,GMPreferencesDialog::ID_LASTFM_SCROBBLE,GMPreferencesDialog::onCmdLastFMScrobble),
  FXMAPFUNC(SEL_COMMAND,GMPreferencesDialog::ID_LASTFM_SERVICE,GMPreferencesDialog::onCmdLastFMService),
  FXMAPFUNC(SEL_COMMAND,GMPreferencesDialog::ID_LASTFM_USERNAME,GMPreferencesDialog::onCmdLastFMUserName),
  FXMAPFUNC(SEL_CHANGED,GMPreferencesDialog::ID_LASTFM_USERNAME,GMPreferencesDialog::onCmdLastFMUserName),
  FXMAPFUNC(SEL_FOCUSIN,GMPreferencesDialog::ID_LASTFM_PASSWORD,GMPreferencesDialog::onFocusLastFMPassWord),
  FXMAPFUNC(SEL_COMMAND,GMPreferencesDialog::ID_LASTFM_PASSWORD,GMPreferencesDialog::onCmdLastFMPassWord),
  FXMAPFUNC(SEL_COMMAND,GMPreferencesDialog::ID_LASTFM_JOIN,GMPreferencesDialog::onCmdLastFMJoin),
  FXMAPFUNCS(SEL_COMMAND,GMPreferencesDialog::ID_BASE_COLOR,GMPreferencesDialog::ID_BORDER_COLOR,GMPreferencesDialog::onCmdElementColor),
  FXMAPFUNCS(SEL_UPDATE,GMPreferencesDialog::ID_BASE_COLOR,GMPreferencesDialog::ID_BORDER_COLOR,GMPreferencesDialog::onUpdElementColor),
  FXMAPFUNC(SEL_COMMAND,GMPreferencesDialog::ID_COLOR_THEME,GMPreferencesDialog::onCmdColorTheme),
  FXMAPFUNC(SEL_UPDATE,GMPreferencesDialog::ID_COLOR_THEME,GMPreferencesDialog::onUpdColorTheme),
  FXMAPFUNC(SEL_UPDATE,GMPreferencesDialog::ID_FONT,GMPreferencesDialog::onUpdFont),
  FXMAPFUNC(SEL_COMMAND,GMPreferencesDialog::ID_CHANGE_FONT,GMPreferencesDialog::onCmdChangeFont),
  FXMAPFUNC(SEL_COMMAND,FXDialogBox::ID_ACCEPT,GMPreferencesDialog::onCmdAccept),
  FXMAPFUNC(SEL_COMMAND,GMPreferencesDialog::ID_AUDIO_DRIVER,GMPreferencesDialog::onCmdAudioDriver),
  FXMAPFUNC(SEL_COMMAND,GMPreferencesDialog::ID_REPLAY_GAIN,GMPreferencesDialog::onCmdReplayGain),
  FXMAPFUNC(SEL_COMMAND,GMPreferencesDialog::ID_TITLE_FORMAT,GMPreferencesDialog::onCmdTitleFormat),
  FXMAPFUNC(SEL_UPDATE,GMPreferencesDialog::ID_TITLE_FORMAT,GMPreferencesDialog::onUpdTitleFormat),
  FXMAPFUNC(SEL_UPDATE,GMPreferencesDialog::ID_TITLE_FORMAT_LABEL,GMPreferencesDialog::onUpdTitleFormat),
  FXMAPFUNC(SEL_COMMAND,GMPreferencesDialog::ID_ICON_THEME,GMPreferencesDialog::onCmdIconTheme),
  FXMAPFUNC(SEL_COMMAND,GMPreferencesDialog::ID_DISPLAY_DPI,GMPreferencesDialog::onCmdDisplayDPI),
  FXMAPFUNC(SEL_COMMAND,GMPreferencesDialog::ID_APPLY_AUDIO,GMPreferencesDialog::onCmdApplyAudio)
  };

FXIMPLEMENT(GMPreferencesDialog,FXDialogBox,GMPreferencesDialogMap,ARRAYNUMBER(GMPreferencesDialogMap))

GMPreferencesDialog::GMPreferencesDialog(FXWindow * p) : FXDialogBox(p,FXString::null,DECOR_BORDER|DECOR_TITLE,0,0,0,0,0,0,0,0,0,0),
  password_set(false) {

  dpi=getApp()->reg().readIntEntry("SETTINGS","screenres",96);

  setTitle(tr("Preferences"));
  target_closeishide.connect(GMPlayerManager::instance()->getPreferences().gui_hide_player_when_close);
  target_keywords.connect(keywords);

  target_close_audio.connect(GMPlayerManager::instance()->getPreferences().play_close_stream);
  target_pause_close_device.connect(GMPlayerManager::instance()->getPreferences().play_pause_close_device);
  target_gapless.connect(GMPlayerManager::instance()->getPreferences().play_gapless);
  target_show_playing_albumcover.connect(GMPlayerManager::instance()->getPreferences().gui_show_playing_albumcover);

#ifdef HAVE_DBUS
  target_dbus_notify_daemon.connect(GMPlayerManager::instance()->getPreferences().dbus_notify_daemon);
  target_dbus_mpris1.connect(GMPlayerManager::instance()->getPreferences().dbus_mpris1);
  target_dbus_mpris2.connect(GMPlayerManager::instance()->getPreferences().dbus_mpris2);
#endif
  target_gui_tray_icon.connect(GMPlayerManager::instance()->getPreferences().gui_tray_icon);
  target_replaygain.connect(GMPlayerManager::instance()->getPreferences().play_replaygain,this,ID_REPLAY_GAIN);
  target_gui_show_playing_titlebar.connect(GMPlayerManager::instance()->getPreferences().gui_show_playing_titlebar);
  target_gui_format_title.connect(GMPlayerManager::instance()->getPreferences().gui_format_title);

  GMPlayerManager::instance()->getPreferences().getKeyWords(keywords);

  const FXuint labelstyle=LAYOUT_CENTER_Y|LABEL_NORMAL|LAYOUT_RIGHT;
  const FXuint textfieldstyle=TEXTFIELD_ENTER_ONLY|LAYOUT_FILL_X|FRAME_SUNKEN|FRAME_THICK|LAYOUT_FILL_COLUMN;

  FXGroupBox      * grpbox;
  FXMatrix        * matrix;
  FXVerticalFrame * vframe;
  FXVerticalFrame * vframe2;

  FXVerticalFrame * main=new FXVerticalFrame(this,LAYOUT_FILL_X|LAYOUT_FILL_Y);
  GMTabBook * tabbook=new GMTabBook(main,NULL,0,LAYOUT_FILL_X|LAYOUT_FILL_Y|LAYOUT_RIGHT,0,0,0,0,0,0,0,0);


  new GMTabItem(tabbook,tr("&General"),NULL,TAB_TOP_NORMAL,0,0,0,0,5,5);
  vframe = new GMTabFrame(tabbook);

  grpbox =  new FXGroupBox(vframe,tr("Sort Options"),FRAME_NONE|LAYOUT_FILL_X,0,0,0,0,20);
  grpbox->setFont(GMApp::instance()->getThickFont());

  matrix = new FXMatrix(grpbox,2,MATRIX_BY_COLUMNS|LAYOUT_FILL_X,0,0,0,0,0,0,0,0);
  new FXLabel(matrix,tr("Ignore leading words"),NULL,labelstyle);
  new GMTextField(matrix,10,&target_keywords,FXDataTarget::ID_VALUE,textfieldstyle);

  grpbox =  new FXGroupBox(vframe,tr("Album Covers"),FRAME_NONE|LAYOUT_FILL_X,0,0,0,0,20);
  grpbox->setFont(GMApp::instance()->getThickFont());

  new GMCheckButton(grpbox,tr("Show album cover of playing track\tShow album cover of playing track"),&target_show_playing_albumcover,FXDataTarget::ID_VALUE);

  grpbox =  new FXGroupBox(vframe,tr("System Tray"),FRAME_NONE|LAYOUT_FILL_X,0,0,0,0,20);
  grpbox->setFont(GMApp::instance()->getThickFont());

  if (!GMPlayerManager::instance()->getPreferences().gui_tray_icon_disabled)
    new GMCheckButton(grpbox,tr("Show Tray Icon\tShow tray icon in the system tray."),&target_gui_tray_icon,FXDataTarget::ID_VALUE);
#ifdef HAVE_DBUS
  if (GMPlayerManager::instance()->hasSessionBus()) {
    new GMCheckButton(grpbox,tr("Show Track Change Notifications\tInform notification daemon of track changes."),&target_dbus_notify_daemon,FXDataTarget::ID_VALUE);
    new GMCheckButton(grpbox,tr("MPRIS v1 Connectivity\tEnable MPRIS v1 connectivity"),&target_dbus_mpris1,FXDataTarget::ID_VALUE);
    new GMCheckButton(grpbox,tr("MPRIS v2 Connectivity\tEnable MPRIS v2 connectivity"),&target_dbus_mpris2,FXDataTarget::ID_VALUE);
    }
#endif

  grpbox =  new FXGroupBox(vframe,tr("Last.fm"),FRAME_NONE|LAYOUT_FILL_X,0,0,0,0,23,3,0,0,0,0);
  grpbox->setFont(GMApp::instance()->getThickFont());


  if (GMPlayerManager::instance()->getAudioScrobbler()->isBanned()) {
    new FXLabel(grpbox,tr("This version of Goggles Music Manager is\n"
                       "not supported by Last-FM. Please upgrade\n"
                       "to a newer version of GMM."),NULL,JUSTIFY_LEFT);
    }
  else {
    matrix = new FXMatrix(grpbox,2,MATRIX_BY_COLUMNS|LAYOUT_FILL_X,0,0,0,0);
    new FXLabel(matrix,tr("Service:"),NULL,labelstyle);
    FXHorizontalFrame * hframe = new FXHorizontalFrame(matrix,FRAME_NONE,0,0,0,0,0,0,0,0);
    FXuint current_service = GMPlayerManager::instance()->getAudioScrobbler()->getService();

    lastfm_service = new GMListBox(hframe,this,ID_LASTFM_SERVICE,FRAME_SUNKEN|FRAME_THICK);
    lastfm_service->appendItem("Last.fm");
    lastfm_service->appendItem("Libre.fm");

    if (current_service==SERVICE_CUSTOM) {
      lastfm_service->appendItem("Custom");
      lastfm_service->setNumVisible(3);
      }
    else {
      lastfm_service->setNumVisible(2);
      }
    lastfm_service->setCurrentItem(current_service);

    lastfm_join = new FXButton(hframe,tr("&Sign up…"),NULL,this,ID_LASTFM_JOIN,ICON_AFTER_TEXT|FRAME_RAISED|JUSTIFY_CENTER_Y|JUSTIFY_LEFT|BUTTON_TOOLBAR,0,0,0,0,7);
    lastfm_join->setTextColor(FXRGB(0,0,255));
    lastfm_join->setDefaultCursor(GMIconTheme::instance()->cursor_hand);
    if (current_service==SERVICE_CUSTOM) lastfm_join->hide();


    lastfm_username_label = new FXLabel(matrix,tr("Username:"),NULL,labelstyle);
    lastfm_username = new GMTextField(matrix,20,this,ID_LASTFM_USERNAME,FRAME_SUNKEN|FRAME_THICK);
    lastfm_password_label = new FXLabel(matrix,tr("Password:"),NULL,labelstyle);
    lastfm_password = new GMTextField(matrix,20,this,ID_LASTFM_PASSWORD,FRAME_SUNKEN|FRAME_THICK|TEXTFIELD_PASSWD);

    new FXFrame(matrix,FRAME_NONE);
    lastfm_scrobble = new GMCheckButton(grpbox,tr("Scrobble"),this,ID_LASTFM_SCROBBLE,LAYOUT_FIX_X|LAYOUT_FIX_Y|CHECKBUTTON_NORMAL,6,0);
    lastfm_scrobble->setFont(GMApp::instance()->getThickFont());

    lastfm_username->setText(GMPlayerManager::instance()->getAudioScrobbler()->getUsername());
    if (GMPlayerManager::instance()->getAudioScrobbler()->hasPassword())
      lastfm_password->setText("1234567890");

    lastfm_scrobble->setCheck(GMPlayerManager::instance()->getAudioScrobbler()->isEnabled());

    if (current_service==SERVICE_LASTFM){
      lastfm_username->hide();
      lastfm_username_label->hide();
      lastfm_password->hide();
      lastfm_password_label->hide();
      }
    else {
      lastfm_username->show();
      lastfm_username_label->show();
      lastfm_password->show();
      lastfm_password_label->show();
      }

    }

  new GMTabItem(tabbook,tr("&Window"),NULL,TAB_TOP_NORMAL,0,0,0,0,5,5);
  vframe = new GMTabFrame(tabbook);

  grpbox =  new FXGroupBox(vframe,tr("Window"),FRAME_NONE|LAYOUT_FILL_X,0,0,0,0,20);
  grpbox->setFont(GMApp::instance()->getThickFont());
  if (!GMPlayerManager::instance()->getPreferences().gui_tray_icon_disabled)
    new GMCheckButton(grpbox,tr("Close button minimizes to tray"),&target_closeishide,FXDataTarget::ID_VALUE);
  statusbarbutton = new GMCheckButton(grpbox,tr("Show Status Bar"),NULL,0);
  statusbarbutton->setCheck(GMPlayerManager::instance()->getPreferences().gui_show_status_bar);

  new GMCheckButton(grpbox,tr("Display playing track in title bar"),&target_gui_show_playing_titlebar,FXDataTarget::ID_VALUE);


  grpbox =  new FXGroupBox(vframe,tr("Toolbar"),FRAME_NONE|LAYOUT_FILL_X,0,0,0,0,20);
  grpbox->setFont(GMApp::instance()->getThickFont());

  matrix = new FXMatrix(grpbox,2,MATRIX_BY_COLUMNS);
  new FXLabel(matrix,tr("Location:"),NULL,labelstyle);
  toolbar_docktop = new GMListBox(matrix,NULL,0,FRAME_SUNKEN|FRAME_THICK|LAYOUT_FILL_COLUMN|LAYOUT_FILL_X);
  toolbar_docktop->appendItem(tr("Top"));
  toolbar_docktop->appendItem(tr("Bottom"));
  toolbar_docktop->setNumVisible(2);

  new FXLabel(matrix,tr("Title Format:"),NULL,labelstyle);
  new GMTextField(matrix,20,&target_gui_format_title,FXDataTarget::ID_VALUE,LAYOUT_FILL_COLUMN|LAYOUT_FILL_X);

  if (GMPlayerManager::instance()->getPreferences().gui_toolbar_docktop)
    toolbar_docktop->setCurrentItem(0);
  else
    toolbar_docktop->setCurrentItem(1);

  new GMTabItem(tabbook,tr("A&ppearance"),NULL,TAB_TOP_NORMAL,0,0,0,0,5,5);
  vframe = new GMTabFrame(tabbook);
  grpbox =  new FXGroupBox(vframe,tr("Colors"),FRAME_NONE|LAYOUT_FILL_X,0,0,0,0,2,2,0,2);
  grpbox->setFont(GMApp::instance()->getThickFont());

  matrix = new FXMatrix(grpbox,6,MATRIX_BY_COLUMNS|LAYOUT_SIDE_LEFT,0,0,0,0,0,0,0,0,0,0);

  new FXFrame(matrix,FRAME_NONE);
  new FXLabel(matrix,tr("fg\tForeground Color"),NULL,LAYOUT_CENTER_Y|LABEL_NORMAL|LAYOUT_CENTER_X);
  new FXLabel(matrix,tr("bg\tBackground Color"),NULL,LAYOUT_CENTER_Y|LABEL_NORMAL|LAYOUT_CENTER_X);
  new FXLabel(matrix,tr("alt bg\tAlternative Background Color"),NULL,LAYOUT_CENTER_Y|LABEL_NORMAL|LAYOUT_CENTER_X);
  new FXFrame(matrix,FRAME_NONE);
  new FXFrame(matrix,FRAME_NONE);

  new FXFrame(matrix,FRAME_NONE);
  new FXSeparator(matrix,SEPARATOR_LINE|LAYOUT_FILL_X);
  new FXSeparator(matrix,SEPARATOR_LINE|LAYOUT_FILL_X);
  new FXSeparator(matrix,SEPARATOR_LINE|LAYOUT_FILL_X);
  new FXFrame(matrix,FRAME_NONE);
  new FXFrame(matrix,FRAME_NONE);

  new FXLabel(matrix,tr("Normal\tNormal Text Color"),NULL,LAYOUT_CENTER_Y|LABEL_NORMAL|LAYOUT_RIGHT);
  new FXColorWell(matrix,0,this,ID_FORE_COLOR,COLORWELL_OPAQUEONLY|FRAME_LINE|LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT|LAYOUT_FILL_ROW|LAYOUT_CENTER_Y,0,0,40,24);
  new FXColorWell(matrix,0,this,ID_BACK_COLOR,COLORWELL_OPAQUEONLY|FRAME_LINE|LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT|LAYOUT_FILL_ROW|LAYOUT_CENTER_Y,0,0,40,24);
  new FXColorWell(matrix,0,this,ID_ALTERNATIVE_BACK_COLOR,COLORWELL_OPAQUEONLY|FRAME_LINE|LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT|LAYOUT_FILL_ROW|LAYOUT_CENTER_Y,0,0,40,24);
  new FXLabel(matrix,tr("Base\tBase Color"),NULL,LAYOUT_CENTER_Y|LABEL_NORMAL|LAYOUT_RIGHT);
  new FXColorWell(matrix,0,this,ID_BASE_COLOR,COLORWELL_OPAQUEONLY|FRAME_LINE|LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT|LAYOUT_FILL_ROW|LAYOUT_CENTER_Y,0,0,40,24);

  new FXLabel(matrix,tr("Selected\tSelected Text Color"),NULL,LAYOUT_CENTER_Y|LABEL_NORMAL|LAYOUT_RIGHT);
  new FXColorWell(matrix,0,this,ID_SEL_FORE_COLOR,COLORWELL_OPAQUEONLY|FRAME_LINE|LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT|LAYOUT_FILL_ROW|LAYOUT_CENTER_Y,0,0,40,24);
  new FXColorWell(matrix,0,this,ID_SEL_BACK_COLOR,COLORWELL_OPAQUEONLY|FRAME_LINE|LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT|LAYOUT_FILL_ROW|LAYOUT_CENTER_Y,0,0,40,24);
  new FXFrame(matrix,FRAME_NONE);
  new FXLabel(matrix,tr("Menu\tMenu Base Color"),NULL,LAYOUT_CENTER_Y|LABEL_NORMAL|LAYOUT_RIGHT);
  new FXColorWell(matrix,0,this,ID_MENU_BASE_COLOR,COLORWELL_OPAQUEONLY|FRAME_LINE|LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT|LAYOUT_FILL_ROW|LAYOUT_CENTER_Y,0,0,40,24);


  new FXLabel(matrix,tr("Menu\tMenu Text Color"),NULL,LAYOUT_CENTER_Y|LABEL_NORMAL|LAYOUT_RIGHT);
  new FXColorWell(matrix,0,this,ID_MENU_FORE_COLOR,COLORWELL_OPAQUEONLY|FRAME_LINE|LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT|LAYOUT_FILL_ROW|LAYOUT_CENTER_Y,0,0,40,24);
  new FXColorWell(matrix,0,this,ID_MENU_BACK_COLOR,COLORWELL_OPAQUEONLY|FRAME_LINE|LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT|LAYOUT_FILL_ROW|LAYOUT_CENTER_Y,0,0,40,24);
  new FXFrame(matrix,FRAME_NONE);
  new FXLabel(matrix,tr("Border\tBorder Color"),NULL,LAYOUT_CENTER_Y|LABEL_NORMAL|LAYOUT_RIGHT);
  new FXColorWell(matrix,0,this,ID_BORDER_COLOR,COLORWELL_OPAQUEONLY|FRAME_LINE|LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT|LAYOUT_FILL_ROW|LAYOUT_CENTER_Y,0,0,40,24);


  new FXLabel(matrix,tr("Tooltip\tTooltip Color"),NULL,LAYOUT_CENTER_Y|LABEL_NORMAL|LAYOUT_RIGHT);
  new FXColorWell(matrix,0,this,ID_TIP_FORE_COLOR,COLORWELL_OPAQUEONLY|FRAME_LINE|LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT|LAYOUT_FILL_ROW|LAYOUT_CENTER_Y,0,0,40,24);
  new FXColorWell(matrix,0,this,ID_TIP_BACK_COLOR,COLORWELL_OPAQUEONLY|FRAME_LINE|LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT|LAYOUT_FILL_ROW|LAYOUT_CENTER_Y,0,0,40,24);
  new FXFrame(matrix,FRAME_NONE);
  new FXLabel(matrix,tr("Hilite\tHilite Color"),NULL,LAYOUT_CENTER_Y|LABEL_NORMAL|LAYOUT_RIGHT);
  new FXColorWell(matrix,0,this,ID_HILITE_COLOR,COLORWELL_OPAQUEONLY|FRAME_LINE|LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT|LAYOUT_FILL_ROW|LAYOUT_CENTER_Y,0,0,40,24);

  new FXFrame(matrix,FRAME_NONE);
  new FXFrame(matrix,FRAME_NONE);
  new FXFrame(matrix,FRAME_NONE);
  new FXFrame(matrix,FRAME_NONE);
  new FXLabel(matrix,tr("Shadow\tShadow Color"),NULL,LAYOUT_CENTER_Y|LABEL_NORMAL|LAYOUT_RIGHT);
  new FXColorWell(matrix,0,this,ID_SHADOW_COLOR,COLORWELL_OPAQUEONLY|FRAME_LINE|LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT|LAYOUT_FILL_ROW|LAYOUT_CENTER_Y,0,0,40,24);

  new FXLabel(matrix,tr("Playing\tPlaying Track Color"),NULL,LAYOUT_CENTER_Y|LABEL_NORMAL|LAYOUT_RIGHT);
  new FXColorWell(matrix,0,this,ID_PLAY_FORE_COLOR,COLORWELL_OPAQUEONLY|FRAME_LINE|LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT|LAYOUT_FILL_ROW|LAYOUT_CENTER_Y,0,0,40,24);
  new FXColorWell(matrix,0,this,ID_PLAY_BACK_COLOR,COLORWELL_OPAQUEONLY|FRAME_LINE|LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT|LAYOUT_FILL_ROW|LAYOUT_CENTER_Y,0,0,40,24);
  new FXFrame(matrix,FRAME_NONE);
  new FXLabel(matrix,tr("Tray\tTray Background Color"),NULL,LAYOUT_CENTER_Y|LABEL_NORMAL|LAYOUT_RIGHT);
  new FXColorWell(matrix,0,this,ID_TRAY_COLOR,COLORWELL_OPAQUEONLY|FRAME_LINE|LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT|LAYOUT_FILL_ROW|LAYOUT_CENTER_Y,0,0,40,24);


  new FXFrame(matrix,FRAME_NONE);
  new FXFrame(matrix,FRAME_NONE);

  new FXSeparator(grpbox,SEPARATOR_GROOVE|LAYOUT_FILL_Y|LAYOUT_SIDE_LEFT);

  vframe2 = new FXVerticalFrame(grpbox,LAYOUT_SIDE_RIGHT|LAYOUT_FILL_X|LAYOUT_FILL_Y,0,0,0,0,0,0,0,0,0,0);
  new FXLabel(vframe2,tr("Presets:"));
  GMScrollFrame * sunken = new GMScrollFrame(vframe2);
  colorpresets = new GMList(sunken,this,ID_COLOR_THEME,LIST_BROWSESELECT|LAYOUT_FILL_X|LAYOUT_FILL_Y);
  colorpresets->setNumVisible(9);
  colorpresets->setScrollStyle(HSCROLLING_OFF);

  initColorThemes();

  grpbox =  new FXGroupBox(vframe,tr("Font & Icons"),FRAME_NONE|LAYOUT_FILL_X,0,0,0,0);
  grpbox->setFont(GMApp::instance()->getThickFont());

  matrix = new FXMatrix(grpbox,3,MATRIX_BY_COLUMNS|LAYOUT_FILL_X,0,0,0,0,20);

  new FXLabel(matrix,tr("Default Font"),NULL,LAYOUT_RIGHT|LAYOUT_CENTER_Y);
  new GMTextField(matrix,20,this,ID_FONT,LAYOUT_CENTER_Y|LABEL_NORMAL|LAYOUT_FILL_X|LAYOUT_FILL_COLUMN|TEXTFIELD_NORMAL|TEXTFIELD_READONLY);
  new GMButton(matrix,tr("Change…"),NULL,this,ID_CHANGE_FONT,BUTTON_NORMAL|LAYOUT_CENTER_Y);

  new FXLabel(matrix,tr("Display DPI"),NULL,LAYOUT_RIGHT|LAYOUT_CENTER_Y);
  GMSpinner * dpi_spinner = new GMSpinner(matrix,4,this,ID_DISPLAY_DPI,LAYOUT_FILL_COLUMN);
  dpi_spinner->setValue(dpi);
  dpi_spinner->setRange(72,200);
  new FXFrame(matrix,FRAME_NONE);


  new FXLabel(matrix,tr("Icons"),NULL,LAYOUT_RIGHT|LAYOUT_CENTER_Y);

  themelist = new GMListBox(matrix,this,ID_ICON_THEME,FRAME_SUNKEN|FRAME_THICK|LAYOUT_FILL_COLUMN|LAYOUT_FILL_X);
  themelist->appendItem("Standard",NULL,(void*)(FXival)-1);
    for (FXint i=0;i<GMIconTheme::instance()->getNumThemes();i++) {
      themelist->appendItem(GMIconTheme::instance()->getThemeName(i),NULL,(void*)(FXival)i);
      }
  themelist->setSortFunc(FXList::ascending);
  themelist->sortItems();
  themelist->setNumVisible(FXMIN(9,themelist->getNumItems()));
  for (FXint i=0;i<themelist->getNumItems();i++) {
    if (GMIconTheme::instance()->getCurrentTheme()==(FXint)(FXival)themelist->getItemData(i)) {
      themelist->setCurrentItem(i);
      break;
      }
    }

  new FXFrame(matrix,FRAME_NONE);

  new GMTabItem(tabbook,tr("&Audio"),NULL,TAB_TOP_NORMAL,0,0,0,0,5,5);
  vframe = new GMTabFrame(tabbook);

  grpbox =  new FXGroupBox(vframe,tr("Output"),FRAME_NONE|LAYOUT_FILL_X,0,0,0,0,20);
  grpbox->setFont(GMApp::instance()->getThickFont());

  matrix = new FXMatrix(grpbox,2,MATRIX_BY_COLUMNS|LAYOUT_SIDE_TOP,0,0,0,0,0,0,0,0);
  new FXLabel(matrix,tr("Driver:"),NULL,labelstyle);

  driverlist = new GMListBox(matrix,this,ID_AUDIO_DRIVER);

  OutputConfig config;
  GMPlayerManager::instance()->getPlayer()->getOutputConfig(config);

  FXStringList drivers;
  FXuint devices=OutputConfig::devices();

  if (AP_HAS_PLUGIN(devices,DeviceAlsa))
    driverlist->appendItem("Advanced Linux Sound Architecture",NULL,(void*)DeviceAlsa);

  if (AP_HAS_PLUGIN(devices,DeviceOSS))
    driverlist->appendItem("Open Sound System",NULL,(void*)DeviceOSS);

  if (AP_HAS_PLUGIN(devices,DevicePulse))
    driverlist->appendItem("PulseAudio",NULL,(void*)DevicePulse);

  if (AP_HAS_PLUGIN(devices,DeviceJack))
    driverlist->appendItem("Jack",NULL,(void*)DeviceJack);

  if (AP_HAS_PLUGIN(devices,DeviceRSound))
    driverlist->appendItem("RSound",NULL,(void*)DeviceRSound);

  if (AP_HAS_PLUGIN(devices,DeviceWav))
    driverlist->appendItem("Wave File Output",NULL,(void*)DeviceWav);

  if (driverlist->getNumItems()) {
    driverlist->setCurrentItem(driverlist->findItemByData((void*)(FXival)config.device));
    driverlist->setNumVisible(FXMIN(9,driverlist->getNumItems()));
    }
  else {
    driverlist->disable();
    }
  /// Alsa
  alsa_device_label = new FXLabel(matrix,tr("Device:"),NULL,labelstyle);
  alsa_device = new GMTextField(matrix,20);
  alsa_device->setText(config.alsa.device);

  alsa_mixer_label = new FXLabel(matrix,tr("Mixer:"),NULL,labelstyle);
  alsa_mixer = new GMTextField(matrix,20);
  alsa_mixer->setText(config.alsa.mixer);

  alsa_hardware_only_frame = new FXFrame(matrix,FRAME_NONE);
  alsa_hardware_only = new GMCheckButton(matrix,"No resampling");

  /// OSS
  oss_device_label = new FXLabel(matrix,tr("Device:"),NULL,labelstyle);
  oss_device = new GMTextField(matrix,20);
  oss_device->setText(config.oss.device);

  /// Pulse
  pulse_device_label = new FXLabel(matrix,tr("Device:"),NULL,labelstyle);
  pulse_device = new GMTextField(matrix,20);

  /// Jack
  jack_device_label = new FXLabel(matrix,tr("Device:"),NULL,labelstyle);
  jack_device = new GMTextField(matrix,20);

  showDriverSettings(config.device);

  new FXFrame(matrix,FRAME_NONE);
  new GMButton(matrix,tr("Apply Changes"),NULL,this,ID_APPLY_AUDIO);

  grpbox =  new FXGroupBox(vframe,tr("Playback"),FRAME_NONE|LAYOUT_FILL_X,0,0,0,0,20);
  grpbox->setFont(GMApp::instance()->getThickFont());

  matrix = new FXMatrix(grpbox,2,MATRIX_BY_COLUMNS|LAYOUT_SIDE_TOP,0,0,0,0,0,0,0,0);

  new FXLabel(matrix,tr("Replay Gain:"),NULL,labelstyle);

  GMListBox * gainlist = new GMListBox(matrix,&target_replaygain,FXDataTarget::ID_VALUE);
  gainlist->appendItem(tr("Off"));
  gainlist->appendItem(tr("Track"));
  gainlist->appendItem(tr("Album"));
  gainlist->setNumVisible(3);


  // Podcast Settings
  new GMTabItem(tabbook,tr("&Podcasts"),NULL,TAB_TOP_NORMAL,0,0,0,0,5,5);
  vframe = new GMTabFrame(tabbook);

  grpbox =  new FXGroupBox(vframe,tr("Updates"),FRAME_NONE|LAYOUT_FILL_X,0,0,0,0,20);
  grpbox->setFont(GMApp::instance()->getThickFont());

  matrix = new FXMatrix(grpbox,2,MATRIX_BY_COLUMNS|LAYOUT_SIDE_TOP,0,0,0,0);
  new FXLabel(matrix,tr("Update Interval"),NULL,labelstyle);
  interval  = new GMListBox(matrix);
  interval->appendItem(tr("Disabled"));
  interval->appendItem(tr("10 minutes"));
  interval->appendItem(tr("20 minutes"));
  interval->appendItem(tr("30 minutes"));
  interval->appendItem(tr("1 hour"));
  interval->appendItem(tr("2 hours"));
  interval->appendItem(tr("6 hours"));
  interval->appendItem(tr("12 hours"));
  interval->setNumVisible(FXMIN(interval->getNumItems(),9));

  FXlong update_interval =  GMPlayerManager::instance()->getPodcastSource()->getUpdateInterval();
  if (update_interval<=0)
    interval->setCurrentItem(0);
  else if (update_interval<=10*MINUTES)
    interval->setCurrentItem(1);
  else if (update_interval<=20*MINUTES)
    interval->setCurrentItem(2);
  else if (update_interval<=30*MINUTES)
    interval->setCurrentItem(3);
  else if (update_interval<=60*MINUTES)
    interval->setCurrentItem(4);
  else if (update_interval<=120*MINUTES)
    interval->setCurrentItem(5);
  else if (update_interval<=360*MINUTES)
    interval->setCurrentItem(6);
  else
    interval->setCurrentItem(7);

  FXHorizontalFrame *closebox=new FXHorizontalFrame(main,LAYOUT_BOTTOM|LAYOUT_FILL_X|PACK_UNIFORM_WIDTH,0,0,0,0,0,0,0,0);
  new GMButton(closebox,tr("&Close"),NULL,this,FXDialogBox::ID_ACCEPT,BUTTON_INITIAL|BUTTON_DEFAULT|LAYOUT_RIGHT|FRAME_RAISED|FRAME_THICK,0,0,0,0, 20,20);
  }

GMPreferencesDialog::~GMPreferencesDialog(){
  }


void GMPreferencesDialog::updateFonts() {
  GMPlayerManager::instance()->getMainWindow()->getTrackView()->updateFont();
  redraw();
  FXint nw =getDefaultWidth();
  FXint nh = getDefaultHeight();
  resize(nw,nh);
  }

long GMPreferencesDialog::onCmdDisplayDPI(FXObject*,FXSelector,void*ptr){
  dpi = (FXint)(FXival)ptr;
  getApp()->reg().writeIntEntry("SETTINGS","screenres",dpi);
  GMApp::instance()->updateFont();
  updateFonts();
  return 1;
  }

void GMPreferencesDialog::showDriverSettings(FXuchar driver) {
  switch(driver) {
    case DeviceAlsa:
      {
        alsa_device_label->show();
        alsa_device->show();
        alsa_hardware_only->show();
        alsa_hardware_only_frame->show();
        alsa_mixer_label->show();
        alsa_mixer->show();
        oss_device->hide();
        oss_device_label->hide();
        pulse_device->hide();
        pulse_device_label->hide();
        jack_device->hide();
        jack_device_label->hide();
      } break;

    case DeviceOSS:
      {
        alsa_device_label->hide();
        alsa_device->hide();
        alsa_hardware_only->hide();
        alsa_hardware_only_frame->hide();
        alsa_mixer_label->hide();
        alsa_mixer->hide();
        oss_device->show();
        oss_device_label->show();
        pulse_device->hide();
        pulse_device_label->hide();
        jack_device->hide();
        jack_device_label->hide();
      } break;

    case DevicePulse:
      {
        alsa_device_label->hide();
        alsa_device->hide();
        alsa_hardware_only->hide();
        alsa_hardware_only_frame->hide();
        alsa_mixer_label->hide();
        alsa_mixer->hide();
        oss_device->hide();
        oss_device_label->hide();
        pulse_device->hide();
        pulse_device_label->hide();
        jack_device->hide();
        jack_device_label->hide();
      } break;

    case DeviceJack:
      {
        alsa_device_label->hide();
        alsa_device->hide();
        alsa_hardware_only->hide();
        alsa_hardware_only_frame->hide();
        alsa_mixer_label->hide();
        alsa_mixer->hide();
        oss_device->hide();
        oss_device_label->hide();
        pulse_device->hide();
        pulse_device_label->hide();
        jack_device->hide();
        jack_device_label->hide();
      } break;
    default:
      {
        alsa_device_label->hide();
        alsa_device->hide();
        alsa_hardware_only->hide();
        alsa_hardware_only_frame->hide();
        alsa_mixer_label->hide();
        alsa_mixer->hide();
        oss_device->hide();
        oss_device_label->hide();
        pulse_device->hide();
        pulse_device_label->hide();
        jack_device->hide();
        jack_device_label->hide();
      } break;

    }

  }


long GMPreferencesDialog::onCmdApplyAudio(FXObject*,FXSelector,void*){

  OutputConfig config;
  GMPlayerManager::instance()->getPlayer()->getOutputConfig(config);

  config.device = (FXuchar)(FXival)driverlist->getItemData(driverlist->getCurrentItem());

  /// Alsa Settings
  config.alsa.device = alsa_device->getText();
  config.alsa.mixer  = alsa_mixer->getText();

  if (alsa_hardware_only->getCheck())
    config.alsa.flags|=AlsaConfig::DeviceNoResample;
  else
    config.alsa.flags&=~AlsaConfig::DeviceNoResample;

  config.oss.device = oss_device->getText();

  GMPlayerManager::instance()->getPlayer()->setOutputConfig(config);
  return 1;
  }

long GMPreferencesDialog::onCmdAudioDriver(FXObject*,FXSelector,void*){
  FXuchar device=(FXuchar)(FXival)driverlist->getItemData(driverlist->getCurrentItem());
  showDriverSettings(device);
  return 1;
  }

long GMPreferencesDialog::onCmdReplayGain(FXObject*,FXSelector,void*){
  switch(GMPlayerManager::instance()->getPreferences().play_replaygain){
    case 0: GMPlayerManager::instance()->getPlayer()->setReplayGain(ReplayGainOff); break;
    case 1: GMPlayerManager::instance()->getPlayer()->setReplayGain(ReplayGainTrack); break;
    case 2: GMPlayerManager::instance()->getPlayer()->setReplayGain(ReplayGainAlbum); break;
    }
  return 1;
  }


long GMPreferencesDialog::onCmdAccept(FXObject*,FXSelector,void*) {
  getApp()->stopModal(this,true);
  hide();

  FXlong update_interval = 0;
  switch(interval->getCurrentItem()){
    case 0: update_interval = 0;             break;
    case 1: update_interval = 10  * MINUTES; break;
    case 2: update_interval = 20  * MINUTES; break;
    case 3: update_interval = 30  * MINUTES; break;
    case 4: update_interval = 60  * MINUTES; break;
    case 5: update_interval = 120 * MINUTES; break;
    case 6: update_interval = 360 * MINUTES; break;
    case 7: update_interval = 720 * MINUTES; break;
    default: break;
    }
  GMPlayerManager::instance()->getPodcastSource()->setUpdateInterval(update_interval);

  GMWindow * mainwindow = GMPlayerManager::instance()->getMainWindow();

  if (!GMPlayerManager::instance()->getAudioScrobbler()->isBanned() && password_set) {
    GMPlayerManager::instance()->getAudioScrobbler()->login(lastfm_username->getText(),lastfm_password->getText());
    }

  GMPlayerManager::instance()->getPreferences().setKeyWords(keywords);

  if (!(selected==current)) {
    GMIconTheme::instance()->load();
    redraw();
    }

  mainwindow->configureToolbar((toolbar_docktop->getCurrentItem()==0));
  mainwindow->configureStatusbar(statusbarbutton->getCheck());

  if (GMPlayerManager::instance()->getPreferences().gui_show_playing_albumcover) {
    if (GMPlayerManager::instance()->playing())
      GMPlayerManager::instance()->update_cover_display();
    }
  else {
    mainwindow->clearCover();
    }

  GMPlayerManager::instance()->update_tray_icon();
#ifdef HAVE_DBUS
  GMPlayerManager::instance()->update_mpris();
#endif

  // Key Words may have changed.
  mainwindow->getTrackView()->resort();
  mainwindow->getSourceView()->resort();
  return 1;
  }




void GMPreferencesDialog::initColorThemes() {
  for(FXuint i=0;i<ARRAYNUMBER(ColorThemes);i++){
    colorpresets->appendItem(ColorThemes[i].name,NULL,(void*)&ColorThemes[i]);
    }

  theme=-1;
  for(FXuint i=0;i<ARRAYNUMBER(ColorThemes);i++){
    if (current==ColorThemes[i]){
      theme=i;
      break;
      }
    }

  if (theme==-1){
    colorpresets->prependItem(tr("Current"),NULL,(void*)&current);
    theme=0;
    }
  }


void GMPreferencesDialog::updateColorThemes() {
  theme=-1;

  for (FXint i=0;i<colorpresets->getNumItems();i++){
    if (selected==*(ColorTheme*)colorpresets->getItemData(i)){
      theme=i;
      return;
      }
    }

  if (theme==-1) {
    theme=colorpresets->getNumItems();
    colorpresets->appendItem(tr("Custom"),NULL,(void*)&selected);
    }
  else if (theme<colorpresets->getNumItems()-1) {
    if (colorpresets->getItemData(colorpresets->getNumItems()-1)==&selected)
      colorpresets->removeItem(colorpresets->getNumItems()-1);
    }
  }



long GMPreferencesDialog::onCmdIconTheme(FXObject*,FXSelector,void*){
  GMIconTheme::instance()->setCurrentTheme((FXint)(FXival)themelist->getItemData(themelist->getCurrentItem()));
  GMIconTheme::instance()->load();
  redraw();
  return 1;
  }






long GMPreferencesDialog::onCmdLastFMScrobble(FXObject*,FXSelector,void*){
  if (lastfm_scrobble->getCheck())
    GMPlayerManager::instance()->getAudioScrobbler()->enable();
  else
    GMPlayerManager::instance()->getAudioScrobbler()->disable();
  return 1;
  }


long GMPreferencesDialog::onCmdLastFMService(FXObject*,FXSelector,void*){
  if (lastfm_service->getCurrentItem()!=SERVICE_CUSTOM) {


    GMPlayerManager::instance()->getAudioScrobbler()->service(lastfm_service->getCurrentItem());
    lastfm_username->setText(FXString::null,false);
    lastfm_password->setText(FXString::null,false);
    password_set=false;
    if (lastfm_service->getNumItems()==3) {
      lastfm_service->removeItem(2);
      lastfm_service->setNumVisible(2);
      }
    lastfm_join->show();
    }

  if (lastfm_service->getCurrentItem()==SERVICE_LASTFM){
    lastfm_username->hide();
    lastfm_username_label->hide();
    lastfm_password->hide();
    lastfm_password_label->hide();
    }
  else {
    lastfm_username->show();
    lastfm_username_label->show();
    lastfm_password->show();
    lastfm_password_label->show();
    }
  return 1;
  }


long GMPreferencesDialog::onCmdLastFMPassWord(FXObject*,FXSelector,void*){
  if (!GMPlayerManager::instance()->getAudioScrobbler()->isBanned() && password_set) {
    lastfm_scrobble->setCheck(true);
    GMPlayerManager::instance()->getAudioScrobbler()->login(lastfm_username->getText(),lastfm_password->getText());
    }
  return 1;
  }

long GMPreferencesDialog::onFocusLastFMPassWord(FXObject*,FXSelector,void*){
  if (password_set==false) {
    password_set=true;
    lastfm_password->setText(FXString::null,false);
    }
  return 0;
  }

long GMPreferencesDialog::onCmdLastFMUserName(FXObject*,FXSelector sel,void*){
  lastfm_password->setText(FXString::null,false);
  password_set=true;
  if (FXSELTYPE(sel)==SEL_COMMAND) lastfm_password->setFocus();
  return 1;
  }

long GMPreferencesDialog::onCmdLastFMJoin(FXObject*,FXSelector,void*){
  FXuint service = GMPlayerManager::instance()->getAudioScrobbler()->getService();
  if (service==SERVICE_LASTFM) {
    if (!gm_open_browser("https://www.last.fm/join/")){
      FXMessageBox::error(this,MBOX_OK,tr("Unable to launch webbrowser"),"Goggles Music Manager was unable to launch a webbrowser.\nPlease visit https://www.last.fm/join/");
      }
    }
  else if (service==SERVICE_LIBREFM) {
   if (!gm_open_browser("http://alpha.libre.fm/register.php")){
      FXMessageBox::error(this,MBOX_OK,tr("Unable to launch webbrowser"),"Goggles Music Manager was unable to launch a webbrowser.\nPlease visit http://alpha.libre.fm/register.php");
      }
    }
  return 1;
  }



long GMPreferencesDialog::onCmdElementColor(FXObject*,FXSelector sel,void* rgba) {
  FXColor color = (FXColor)(FXuval)rgba;

  switch(FXSELID(sel)){
    case ID_BASE_COLOR : selected.base = color;
                         selected.shadow = makeShadowColor(selected.base);
                         selected.hilite = makeHiliteColor(selected.base);
                         break;
    case ID_BORDER_COLOR : selected.border = color; break;
    case ID_BACK_COLOR   : selected.back = color; break;
    case ID_FORE_COLOR  : selected.fore = color; break;
    case ID_SHADOW_COLOR : selected.shadow=color; break;
    case ID_HILITE_COLOR : selected.hilite=color; break;


    case ID_SEL_BACK_COLOR: selected.selback=color; break;
    case ID_SEL_FORE_COLOR: selected.selfore=color; break;
    case ID_MENU_BACK_COLOR: selected.menuback=color; break;
    case ID_MENU_FORE_COLOR: selected.menufore=color; break;
    case ID_MENU_BASE_COLOR: selected.menubase=color; break;
    case ID_TIP_BACK_COLOR: selected.tipback=color; break;
    case ID_TIP_FORE_COLOR: selected.tipfore=color; break;
    case ID_PLAY_BACK_COLOR: selected.playback=color; break;
    case ID_PLAY_FORE_COLOR: selected.playfore=color; break;
    case ID_ALTERNATIVE_BACK_COLOR: selected.altback=color; break;
    case ID_TRAY_COLOR: selected.trayback=color; break;

     }

  updateColors();

  updateColorThemes();
  return 1;
  }

long GMPreferencesDialog::onUpdElementColor(FXObject*sender,FXSelector sel,void*) {
  FXuval rgba = 0;
  switch(FXSELID(sel)){
    case ID_BASE_COLOR : rgba=selected.base; break;
    case ID_BORDER_COLOR : rgba=selected.border; break;
    case ID_BACK_COLOR : rgba=selected.back; break;
    case ID_FORE_COLOR : rgba=selected.fore; break;
    case ID_SHADOW_COLOR : rgba=selected.shadow; break;
    case ID_HILITE_COLOR : rgba=selected.hilite; break;

    case ID_SEL_BACK_COLOR: rgba=selected.selback; break;
    case ID_SEL_FORE_COLOR: rgba=selected.selfore; break;
    case ID_MENU_BACK_COLOR: rgba=selected.menuback; break;
    case ID_MENU_FORE_COLOR: rgba=selected.menufore; break;
    case ID_MENU_BASE_COLOR: rgba=selected.menubase; break;
    case ID_TIP_BACK_COLOR: rgba=selected.tipback; break;
    case ID_TIP_FORE_COLOR: rgba=selected.tipfore; break;
    case ID_PLAY_BACK_COLOR: rgba=selected.playback; break;
    case ID_PLAY_FORE_COLOR: rgba=selected.playfore; break;
    case ID_ALTERNATIVE_BACK_COLOR: rgba=selected.altback; break;
    case ID_TRAY_COLOR: rgba=selected.trayback; break;
    }
  sender->tryHandle(this,FXSEL(SEL_COMMAND,FXColorWell::ID_SETINTVALUE),&rgba);
  return 1;
  }
long GMPreferencesDialog::onCmdColorTheme(FXObject*,FXSelector,void*ptr) {
  theme=(FXint)(FXival)ptr;
  ColorTheme *theme_selected=reinterpret_cast<ColorTheme*>(colorpresets->getItemData(theme));

  selected.base = theme_selected->base;
  selected.border = theme_selected->border;
  selected.back = theme_selected->back;
  selected.altback = theme_selected->altback;
  selected.fore = theme_selected->fore;
  selected.selfore	= theme_selected->selfore;
  selected.selback	= theme_selected->selback;
  selected.tipfore	= theme_selected->tipfore;
  selected.tipback	= theme_selected->tipback;
  selected.menufore= theme_selected->menufore;
  selected.menuback= theme_selected->menuback;
  selected.menubase= theme_selected->menubase;
  selected.shadow = makeShadowColor(selected.base);
  selected.hilite   = makeHiliteColor(selected.base);
  selected.playback = theme_selected->playback;
  selected.playfore = theme_selected->playfore;
  selected.trayback = theme_selected->trayback;

  updateColors();
  return 1;
  }

long GMPreferencesDialog::onUpdColorTheme(FXObject*sender,FXSelector,void*) {
  sender->tryHandle(this,FXSEL(SEL_COMMAND,FXWindow::ID_SETINTVALUE),&theme);
  return 1;
  }



void GMPreferencesDialog::updateColors(){
  register FXWindow *w=FXApp::instance()->getRootWindow();

  FX7Segment * sevensegment;
  FXTextField * gmtextfield;
  FXIconList * iconlist;
  FXList * list;
  FXListBox * listbox;
  FXTreeList * treelist;
  FXComboBox * combobox;
  FXButton * button;
  FXFrame * frame;
  FXLabel * label;
  FXPopup * popup;
  FXMenuTitle * menutitle;
  FXMenuCheck * menucheck;
  FXMenuRadio * menuradio;
  FXMenuCaption * menucaption;
  FXMenuSeparator * menuseparator;
  FXText * text;
  FXFoldingList * foldinglist;
  FXMDIChild * mdichild;
  FXTable * table;
  FXDockTitle * docktitle;
  FXPacker * packer;
  FXHeader * header;
  FXGroupBox * groupbox;
  FXScrollBar * scrollbar;
  FXSlider * slider;
  FXStatusLine * statusline;
  FXDragCorner * dragcorner;
  GMTreeList * gmtreelist;
  GMTrackList * gmtracklist;
  FXRadioButton * radiobutton;
  GMCheckButton * checkbutton;
  FXToolTip * tooltip;

  GMList        * gmlist;
  GMListBox     * gmlistbox;
  GMComboBox     * gmcombobox;
  GMScrollFrame * gmscrollframe;
  GMTabFrame    * gmtabframe;
  GMImageFrame  * gmimageframe;
  GMCoverFrame  * gmcoverframe;
#ifdef HAVE_OPENGL
  GMImageView   * gmimageview;
#endif
  GMMenuPane    * gmmenupane;
  GMProgressBar * gmprogressbar;
  GMTrackProgressBar * gmtrackprogressbar;
  GMSpinner 		* gmspinner;
  GMAlbumList   * gmalbumlist;

  while(w){
    w->setBackColor(selected.base);
    if ((frame=dynamic_cast<FXFrame*>(w))) {

      frame->setBaseColor(selected.base);
      frame->setBackColor(selected.base);
      frame->setShadowColor(selected.shadow);
      frame->setHiliteColor(selected.hilite);
      frame->setBorderColor(selected.border);

      if ((label=dynamic_cast<FXLabel*>(w))) {
        label->setTextColor(selected.fore);
        if ((button=dynamic_cast<FXButton*>(w))) {
          if (dynamic_cast<GMListBox*>(button->getParent())){
            w->setBackColor(selected.back);
            }
          else {
            w->setBackColor(selected.base);
            }
          }
        else if ((checkbutton=dynamic_cast<GMCheckButton*>(w))) {
          checkbutton->setCheckColor(selected.fore);
          checkbutton->setBoxColor(selected.back);
          }
        else if ((radiobutton=dynamic_cast<FXRadioButton*>(w))) {
          radiobutton->setRadioColor(selected.fore);
          radiobutton->setDiskColor(selected.back);
          }
        }
      else if ((gmtextfield=dynamic_cast<FXTextField*>(w))) {
        w->setBackColor(selected.back);
        gmtextfield->setTextColor(selected.fore);
        gmtextfield->setSelTextColor(selected.selfore);
        gmtextfield->setSelBackColor(selected.selback);
        gmtextfield->setBorderColor(selected.shadow);
        }
      else if ((docktitle=dynamic_cast<FXDockTitle*>(w))) {
        docktitle->setCaptionColor(selected.selfore);
        docktitle->setBackColor(selected.selback);
        }
      else if ((header=dynamic_cast<FXHeader*>(w))) {
        header->setTextColor(selected.fore);
        }
      else if ((statusline=dynamic_cast<FXStatusLine*>(w))) {
        statusline->setTextColor(selected.fore);
        }
      else if ((sevensegment=dynamic_cast<FX7Segment*>(w))) {
        sevensegment->setTextColor(selected.fore);
        }
      else if ((slider=dynamic_cast<FXSlider*>(w))) {
        slider->setSlotColor(selected.back);
        }
     else if ((gmimageframe=dynamic_cast<GMImageFrame*>(w))) {
        gmimageframe->setBorderColor(selected.shadow);
        gmimageframe->setBackColor(selected.back); /// fixme, only for coverframe in mainwindow
        }
     else if ((gmprogressbar=dynamic_cast<GMProgressBar*>(w))) {
        gmprogressbar->setBorderColor(selected.shadow);
        gmprogressbar->setBarColor(selected.selback);
        gmprogressbar->setTextAltColor(selected.selfore);
        }
     else if ((gmtrackprogressbar=dynamic_cast<GMTrackProgressBar*>(w))) {
        gmtrackprogressbar->setBorderColor(selected.shadow);
        gmtrackprogressbar->setBarColor(selected.selback);
        gmtrackprogressbar->setTextAltColor(selected.selfore);
        }

     }
   else if ((packer=dynamic_cast<FXPacker*>(w))) {
      packer->setBaseColor(selected.base);
      packer->setBackColor(selected.base);
      packer->setShadowColor(selected.shadow);
      packer->setHiliteColor(selected.hilite);
      packer->setBorderColor(selected.border);
      if ((gmscrollframe=dynamic_cast<GMScrollFrame*>(w))){
        gmscrollframe->setBorderColor(selected.shadow);
        }
      else if ((gmtabframe=dynamic_cast<GMTabFrame*>(w))){
        gmtabframe->setBorderColor(selected.shadow);
        }
       else if ((gmcoverframe=dynamic_cast<GMCoverFrame*>(w))){
        gmcoverframe->setBorderColor(selected.shadow);
        gmcoverframe->setBackColor(selected.back);
        }
      else if ((combobox=dynamic_cast<FXComboBox*>(w))) {
        w->setBackColor(selected.back);
        }
      else if ((listbox=dynamic_cast<FXListBox*>(w))) {
        //w->setBackColor(selected.back);
        if ((gmlistbox=dynamic_cast<GMListBox*>(w))) {
          gmlistbox->setBorderColor(selected.shadow);
          }
        }
      else if ((groupbox=dynamic_cast<FXGroupBox*>(w))) {
        groupbox->setTextColor(selected.fore);
        }
      else if ((gmspinner=dynamic_cast<GMSpinner*>(w))) {
        gmspinner->setBorderColor(selected.shadow);
        gmspinner->setUpArrowColor(selected.fore);
        gmspinner->setDownArrowColor(selected.fore);
        }
      }
    else if ((popup=dynamic_cast<FXPopup*>(w))){
      popup->setBaseColor(selected.base);
      popup->setShadowColor(selected.shadow);
      popup->setHiliteColor(selected.hilite);
      popup->setBorderColor(selected.border);
      if ((gmmenupane=dynamic_cast<GMMenuPane*>(w)) || (gmlistbox=dynamic_cast<GMListBox*>(w->getParent())) ||  (gmcombobox=dynamic_cast<GMComboBox*>(w->getParent()))){
        popup->setBorderColor(selected.shadow);
        }
      }
    else if ((menucaption=dynamic_cast<FXMenuCaption*>(w))) {
      w->setBackColor(selected.menubase);
      menucaption->setTextColor(selected.fore);
      menucaption->setSelTextColor(selected.menufore);
      menucaption->setSelBackColor(selected.menuback);
      menucaption->setShadowColor(makeShadowColor(selected.menubase));
      menucaption->setHiliteColor(makeHiliteColor(selected.menubase));

      if ((menucheck=dynamic_cast<FXMenuCheck*>(w))) {
        menucheck->setBoxColor(selected.back);
        }
      else if ((menuradio=dynamic_cast<FXMenuRadio*>(w))) {
        menuradio->setRadioColor(selected.back);
        }
      else if ((menutitle=dynamic_cast<FXMenuTitle*>(w))) {
        w->setBackColor(selected.base);
        menutitle->setTextColor(selected.fore);
        menutitle->setSelTextColor(selected.menufore);
        menutitle->setSelBackColor(selected.menuback);
        menutitle->setShadowColor(selected.shadow);
        menutitle->setHiliteColor(selected.hilite);
        }
      }
    else if ((menuseparator=dynamic_cast<FXMenuSeparator*>(w))) {
      menuseparator->setShadowColor(makeShadowColor(selected.menubase));
      menuseparator->setHiliteColor(makeHiliteColor(selected.menubase));
      }
    else if ((scrollbar=dynamic_cast<FXScrollBar*>(w))) {
      scrollbar->setShadowColor(selected.shadow);
      scrollbar->setHiliteColor(selected.hilite);
      scrollbar->setBorderColor(selected.border);
      scrollbar->setArrowColor(selected.fore);
      }
    else if ((dragcorner=dynamic_cast<FXDragCorner*>(w))) {
      dragcorner->setShadowColor(selected.shadow);
      dragcorner->setHiliteColor(selected.hilite);
      }
    else if (dynamic_cast<FXScrollArea*>(w)) {
      if ((text=dynamic_cast<FXText*>(w))) {
        w->setBackColor(selected.back);
        text->setTextColor(selected.fore);
        text->setSelTextColor(selected.selfore);
        text->setSelBackColor(selected.selback);
        }
      else if ((list=dynamic_cast<FXList*>(w))) {
        w->setBackColor(selected.back);
        list->setTextColor(selected.fore);
        list->setSelTextColor(selected.selfore);
        list->setSelBackColor(selected.selback);
        if ((gmlist=dynamic_cast<GMList*>(w))) {
          gmlist->setRowColor(selected.altback);
         ((FXFrame*)gmlist->getParent())->setBorderColor(selected.shadow);
          }
        }
      else if ((treelist=dynamic_cast<FXTreeList*>(w))) {
        w->setBackColor(selected.back);
        treelist->setTextColor(selected.fore);
        treelist->setLineColor(selected.shadow);
        treelist->setSelTextColor(selected.selfore);
        treelist->setSelBackColor(selected.selback);
        if ((gmtreelist=dynamic_cast<GMTreeList*>(w))) {
         gmtreelist->setRowColor(selected.altback);
         ((FXFrame*)gmtreelist->getParent())->setBorderColor(selected.shadow);
          }
        else {
          treelist->setSelTextColor(selected.selfore);
          treelist->setSelBackColor(selected.selback);
          }
        }
      else if ((iconlist=dynamic_cast<FXIconList*>(w))) {
        w->setBackColor(selected.back);
        iconlist->setTextColor(selected.fore);
        iconlist->setSelTextColor(selected.selfore);
        iconlist->setSelBackColor(selected.selback);
        }
      else if ((gmalbumlist=dynamic_cast<GMAlbumList*>(w))) {
        w->setBackColor(selected.back);
        gmalbumlist->setTextColor(selected.fore);
        gmalbumlist->setSelTextColor(selected.selfore);
        gmalbumlist->setSelBackColor(selected.selback);
        gmalbumlist->setAltBackColor(selected.altback);
        }
      else if ((gmtracklist=dynamic_cast<GMTrackList*>(w))) {
        w->setBackColor(selected.back);
        //((FXFrame*)gmtracklist->getParent())->setBorderColor(selected.shadow);
        gmtracklist->setTextColor(selected.fore);
        gmtracklist->setSelTextColor(selected.selfore);
        gmtracklist->setSelBackColor(selected.selback);
        gmtracklist->setRowColor(selected.altback);
        gmtracklist->setActiveTextColor(selected.playfore);
        gmtracklist->setActiveColor(selected.playback);
        }
      else if ((foldinglist=dynamic_cast<FXFoldingList*>(w))) {
        w->setBackColor(selected.back);
        foldinglist->setTextColor(selected.fore);
        foldinglist->setSelTextColor(selected.selfore);
        foldinglist->setSelBackColor(selected.selback);
        foldinglist->setLineColor(selected.shadow);
        }
      else if ((table=dynamic_cast<FXTable*>(w))) {
        w->setBackColor(selected.back);
        table->setTextColor(selected.fore);
        table->setSelTextColor(selected.selfore);
        table->setSelBackColor(selected.selback);
        }
      }
    else if ((mdichild=dynamic_cast<FXMDIChild*>(w))) {
      mdichild->setBackColor(selected.base);
      mdichild->setBaseColor(selected.base);
      mdichild->setShadowColor(selected.shadow);
      mdichild->setHiliteColor(selected.hilite);
      mdichild->setBorderColor(selected.border);
      mdichild->setTitleColor(selected.selfore);
      mdichild->setTitleBackColor(selected.selback);
      }
    else if ((tooltip=dynamic_cast<FXToolTip*>(w))){
      tooltip->setTextColor(selected.tipfore);
      tooltip->setBackColor(selected.tipback);
      }
#ifdef HAVE_OPENGL
    else if ((gmimageview=dynamic_cast<GMImageView*>(w))){
      gmimageview->setBackColor(selected.back);
      }
#endif
    w->update();
    if(w->getFirst()){
      w=w->getFirst();
      continue;
      }
    while(!w->getNext() && w->getParent()){
      w=w->getParent();
      }
    w=w->getNext();
    }
  selected.save();

  if (GMPlayerManager::instance()->getTrayIcon())
    GMPlayerManager::instance()->getTrayIcon()->updateIcon();
  }


void GMPreferencesDialog::redraw(){
  register FXWindow *w=GMPlayerManager::instance()->getMainWindow();
  while(w){
    w->recalc();
    w->update();
    if(w->getFirst()){
      w=w->getFirst();
      continue;
      }
    while(!w->getNext() && w->getParent()){
      w=w->getParent();
      }
    w=w->getNext();
    }
  }




static void fancyfontname(FXFont * font,FXString & name) {
  name = font->getActualName().before('[').trimEnd();
  const FXchar *wgt=NULL,*slt=NULL,*wid=NULL;
  const FXint size=font->getActualSize()/10;

  switch(font->getActualSetWidth()){
     case FXFont::UltraCondensed: wid="Ultra Condensed"; break;
     case FXFont::ExtraCondensed: wid="Extra Condensed"; break;
     case FXFont::Condensed:      wid="Condensed"; break;
     case FXFont::SemiCondensed:  wid="Semi Condensed"; break;
     case FXFont::NonExpanded:    wid=NULL; break;
     case FXFont::SemiExpanded:   wid="Semi Expanded"; break;
     case FXFont::Expanded:       wid="Expanded"; break;
     case FXFont::ExtraExpanded:  wid="Extra Expanded"; break;
     case FXFont::UltraExpanded:  wid="Ultra Expanded"; break;
     default: wid=NULL; break;
     }

  switch(font->getActualWeight()){
    case FXFont::Thin      : wgt=fxtr("Thin"); break;
    case FXFont::ExtraLight: wgt=fxtr("Extra Light"); break;
    case FXFont::Light     : wgt=fxtr("Light"); break;
    case FXFont::Normal    : wgt=NULL; break;
    case FXFont::Medium    : wgt=fxtr("Medium"); break;
    case FXFont::DemiBold  : wgt=fxtr("Demibold"); break;
    case FXFont::Bold      : wgt=fxtr("Bold"); break;
    case FXFont::ExtraBold : wgt=fxtr("Extra Bold"); break;
    case FXFont::Black     : wgt=fxtr("Heavy"); break;
    default: wgt=NULL; break;
    }

  switch(font->getActualSlant()){
    case FXFont::ReverseOblique: slt="Reverse Oblique"; break;
    case FXFont::ReverseItalic: slt="Reverse Italic"; break;
    case FXFont::Straight: slt=NULL; break;
    case FXFont::Italic: slt="Italic"; break;
    case FXFont::Oblique: slt="Oblique"; break;
    default: slt=NULL; break;
    }

  if (wgt && slt && wid)
    name+=FXString::value(", %s %s %s, %d",wgt,wid,slt,size);
  else if (wgt && slt)
    name+=FXString::value(", %s %s, %d",wgt,slt,size);
  else if (wgt && wid)
    name+=FXString::value(", %s %s, %d",wgt,wid,size);
  else if (wid && slt)
    name+=FXString::value(", %s %s, %d",wid,slt,size);
  else if (slt)
    name+=FXString::value(", %s, %d",slt,size);
  else if (wgt)
    name+=FXString::value(", %s, %d",wgt,size);
  else if (wid)
    name+=FXString::value(", %s, %d",wid,size);
  else
    name+=FXString::value(", %d",size);
  }





long GMPreferencesDialog::onCmdChangeFont(FXObject*,FXSelector,void*){
  GMFontDialog dialog(this,tr("Select Normal Font"));
  FXFont * font = FXApp::instance()->getNormalFont();
  FXFontDesc fontdescription;
  fontdescription =  font->getActualFontDesc();
  strncpy(fontdescription.face,font->getActualName().text(),sizeof(fontdescription.face));
  dialog.setFontDesc(fontdescription);
  if(dialog.execute(PLACEMENT_SCREEN)){
    GMApp::instance()->setFont(dialog.getFontDesc());
    updateFonts();
    }
  return 1;
  }

long GMPreferencesDialog::onUpdFont(FXObject*sender,FXSelector,void*){
  FXString fontname;
  FXFont * font = FXApp::instance()->getNormalFont();
  fancyfontname(font,fontname);
  sender->tryHandle(this,FXSEL(SEL_COMMAND,FXWindow::ID_SETSTRINGVALUE),&fontname);
  return 1;
  }

long GMPreferencesDialog::onCmdTitleFormat(FXObject*sender,FXSelector,void*){
  sender->handle(this,FXSEL(SEL_COMMAND,ID_GETSTRINGVALUE),&GMPlayerManager::instance()->getPreferences().gui_format_title);
  return 1;
  }

long GMPreferencesDialog::onUpdTitleFormat(FXObject*sender,FXSelector sel,void*){
  if (GMPlayerManager::instance()->getPreferences().gui_show_playing_titlebar)
    sender->handle(this,FXSEL(SEL_COMMAND,ID_ENABLE),NULL);
  else
    sender->handle(this,FXSEL(SEL_COMMAND,ID_DISABLE),NULL);

  if (FXSELID(sel)==ID_TITLE_FORMAT)
    sender->handle(this,FXSEL(SEL_COMMAND,ID_SETSTRINGVALUE),&GMPlayerManager::instance()->getPreferences().gui_format_title);

  return 1;
  }






