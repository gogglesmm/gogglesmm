/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2006-2016 by Sander Jansen. All Rights Reserved      *
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
  FXDataTarget target_replaygain;
  FXDataTarget target_show_playing_albumcover;
#ifdef HAVE_DBUS
  FXDataTarget target_dbus_notify_daemon;
  FXDataTarget target_dbus_mpris1;
#endif
  FXDataTarget target_gui_tray_icon;
  FXDataTarget target_gui_show_playing_titlebar;
  FXDataTarget target_gui_format_title;
protected:
  FXString keywords;
protected:
  FXSwitcher  * driverswitcher = nullptr;

  FXLabel     * alsa_device_label = nullptr;
  FXTextField * alsa_device = nullptr;

  FXCheckButton* alsa_hardware_only = nullptr;
  FXFrame * alsa_hardware_only_frame = nullptr;

  FXLabel     * oss_device_label = nullptr;
  FXTextField * oss_device = nullptr;

protected:
  void showDriverSettings(FXuchar driver);
public:
  FXFontPtr     font_fixed;

  FXTextField * lastfm_username = nullptr;
  FXTextField * lastfm_password = nullptr;
  FXLabel * lastfm_password_label = nullptr;
  FXLabel * lastfm_username_label = nullptr;

  FXCheckButton * lastfm_scrobble = nullptr;
  FXButton      * lastfm_join = nullptr;

  FXTextField * iconthemedir = nullptr;
  FXCheckButton * check_audio_normalization = nullptr;
  FXCheckButton * statusbarbutton = nullptr;
  FXTextField   * title_format = nullptr;
  GMList        * colorpresets = nullptr;
  GMListBox     * toolbar_docktop = nullptr;
  GMListBox     * driverlist = nullptr;
  GMListBox     * themelist = nullptr;
  GMListBox     * lastfm_service = nullptr;
  GMListBox     * interval = nullptr;
  FXbool password_set = false;
  FXFont * selectedfont = nullptr;
  FXint theme = 0;
  FXint dpi = 96;
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
