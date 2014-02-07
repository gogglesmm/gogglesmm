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
#ifndef GMPREFERENCESDIALOG_H
#define GMPREFERENCESDIALOG_H



class GMPreferencesDialog : public FXDialogBox {
FXDECLARE(GMPreferencesDialog)
protected:
  FXDataTarget target_closeishide;
  FXDataTarget target_keywords;
  FXDataTarget target_close_audio;
  FXDataTarget target_pause_close_device;
  FXDataTarget target_gapless;
  FXDataTarget target_replaygain;
  FXDataTarget target_open_device_on_startup;
  FXDataTarget target_show_playing_albumcover;
#ifdef HAVE_DBUS
  FXDataTarget target_dbus_notify_daemon;
  FXDataTarget target_dbus_mpris1;
  FXDataTarget target_dbus_mpris2;
#endif
  FXDataTarget target_gui_tray_icon;
  FXDataTarget target_gui_show_playing_titlebar;
  FXDataTarget target_gui_format_title;
protected:
  FXString keywords;
protected:
  FXSwitcher  * driverswitcher;

  FXLabel     * alsa_device_label;
  FXTextField * alsa_device;

  FXCheckButton* alsa_hardware_only;
  FXFrame * alsa_hardware_only_frame;
  FXLabel     * alsa_mixer_label;
  FXTextField * alsa_mixer;

  FXLabel     * oss_device_label;
  FXTextField * oss_device;

  FXLabel     * pulse_device_label;
  FXTextField * pulse_device;

  FXLabel     * jack_device_label;
  FXTextField * jack_device;
protected:
  void showDriverSettings(FXuchar driver);
public:
  FXTextField * lastfm_username;
  FXTextField * lastfm_password;
  FXLabel * lastfm_password_label;
  FXLabel * lastfm_username_label;

  FXCheckButton * lastfm_scrobble;
  FXButton      * lastfm_join;

  FXTextField * iconthemedir;
  FXCheckButton * check_audio_normalization;
  FXCheckButton * statusbarbutton;
  FXTextField   * title_format;
  GMList        * colorpresets;
  GMListBox     * toolbar_docktop;
  GMListBox     * driverlist;
  GMListBox     * themelist;
  GMListBox     * lastfm_service;
  FXbool password_set;
  FXFont * selectedfont;
  FXint theme;
  FXint dpi;
  ColorTheme current;
  ColorTheme selected;
public:
  enum {
    ID_LASTFM_USERNAME= FXDialogBox::ID_LAST,
    ID_LASTFM_PASSWORD,
    ID_LASTFM_SCROBBLE,
    ID_LASTFM_SERVICE,
    ID_LASTFM_JOIN,
    ID_BASE_COLOR,
    ID_BACK_COLOR,
    ID_FORE_COLOR,
    ID_SHADOW_COLOR,
    ID_HILITE_COLOR,
    ID_SEL_BACK_COLOR,
    ID_SEL_FORE_COLOR,
    ID_MENU_BACK_COLOR,
    ID_MENU_FORE_COLOR,
    ID_MENU_BASE_COLOR,
    ID_TIP_BACK_COLOR,
    ID_TIP_FORE_COLOR,
    ID_PLAY_BACK_COLOR,
    ID_PLAY_FORE_COLOR,
    ID_ALTERNATIVE_BACK_COLOR,
    ID_TRAY_COLOR,
    ID_BORDER_COLOR,
    ID_COLOR_THEME,
    ID_FONT,
    ID_CHANGE_FONT,
    ID_AUDIO_DRIVER,
    ID_REPLAY_GAIN,
    ID_ICON_THEME,
    ID_TITLE_FORMAT,
    ID_TITLE_FORMAT_LABEL,
    ID_DISPLAY_DPI,
    ID_APPLY_AUDIO
    };
public:
  long onCmdLastFMScrobble(FXObject*,FXSelector,void*);
  long onCmdLastFMUserName(FXObject*,FXSelector,void*);
  long onCmdLastFMPassWord(FXObject*,FXSelector,void*);
  long onFocusLastFMPassWord(FXObject*,FXSelector,void*);
  long onCmdLastFMService(FXObject*,FXSelector,void*);
  long onCmdLastFMJoin(FXObject*,FXSelector,void*);
  long onCmdElementColor(FXObject*,FXSelector,void*);
  long onUpdElementColor(FXObject*,FXSelector,void*);
  long onCmdColorTheme(FXObject*,FXSelector,void*);
  long onUpdColorTheme(FXObject*,FXSelector,void*);
  long onUpdFont(FXObject*,FXSelector,void*);
  long onCmdChangeFont(FXObject*,FXSelector,void*);
  long onCmdAccept(FXObject*,FXSelector,void*);
  long onCmdAudioDriver(FXObject*,FXSelector,void*);
  long onCmdReplayGain(FXObject*,FXSelector,void*);
  long onCmdTitleFormat(FXObject*,FXSelector,void*);
  long onUpdTitleFormat(FXObject*,FXSelector,void*);
  long onCmdDisplayDPI(FXObject*,FXSelector,void*);
  long onCmdIconTheme(FXObject*,FXSelector,void*);
  long onCmdApplyAudio(FXObject*,FXSelector,void*);
protected:
  GMPreferencesDialog(){}
private:
  GMPreferencesDialog(const GMPreferencesDialog&);
  GMPreferencesDialog &operator=(const GMPreferencesDialog&);
public:
  GMPreferencesDialog(FXWindow * p);

  void initColorThemes();
  void updateColorThemes();
  void updateColors();
  void updateFonts();
  void redraw();

  virtual ~GMPreferencesDialog();
  };
#endif
