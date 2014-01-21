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
#ifndef GMPREFERENCES_H
#define GMPREFERENCES_H

enum {
  REPLAYGAIN_OFF    =0,
  REPLAYGAIN_TRACK  =1,
  REPLAYGAIN_ALBUM  =2
  };

enum {
  REPEAT_OFF        = 0,
  REPEAT_TRACK      = 1,
  REPEAT_ALL        = 2,
  };


struct ColorTheme {
  const FXchar* name;
  FXColor base;
  FXColor border;
  FXColor back;
  FXColor altback;
  FXColor fore;
  FXColor selback;
  FXColor selfore;
  FXColor tipback;
  FXColor tipfore;
  FXColor menuback;
  FXColor menufore;
  FXColor menubase;
  FXColor sourcefore;
  FXColor sourceback;
  FXColor playfore;
  FXColor playback;
  FXColor hilite;
  FXColor shadow;
  FXColor trayback;

  ColorTheme();
  ColorTheme(const FXchar * _name,FXColor _base,FXColor _border,FXColor _back,FXColor _altback,FXColor _fore,FXColor _selback,FXColor _selfore,FXColor _tipback,FXColor _tipfore,FXColor _psback=FXRGB(210,230,210),FXColor _psfore=FXRGB(  0,  0,  0));
  void save() const;

  friend bool operator==(const ColorTheme& t1,const ColorTheme& t2);
  };


class GMImportOptions {
public:
  FXString default_field;
  FXString exclude_folder;
  FXString exclude_file;
  FXString filename_template;
  FXbool   track_from_filelist;
  FXbool   replace_underscores;
  FXbool   fix_album_artist;
  FXuint   parse_method;
public:
  enum {
    PARSE_TAG = 0,
    PARSE_FILENAME = 1,
    PARSE_BOTH = 2
    };
public:
  GMImportOptions();

  /// Save to FXSettings
  void save(FXSettings & settings) const;

  /// Load from FXSettings
  void load(FXSettings & settings);
  };


class GMSyncOptions {
public:
  FXbool import_new;
  FXbool remove_missing;
  FXbool remove_all;
  FXbool update;
  FXbool update_always;
public:
  GMSyncOptions();

  /// Save to FXSettings
  void save(FXSettings & settings) const;

  /// Load from FXSettings
  void load(FXSettings & settings);
  };

class GMPreferences {
public:
  GMImportOptions import;
  GMSyncOptions   sync;

  FXStringList gui_sort_keywords;

  FXString export_format_template;
  FXString export_character_filter;
  FXString gui_format_title;

  FXbool gui_show_status_bar;
  FXbool gui_hide_player_when_close;
  FXbool gui_toolbar_bigicons;
  FXbool gui_toolbar_docktop;
  FXbool gui_toolbar_showlabels;
  FXbool gui_toolbar_labelsabove;
  FXbool gui_show_browser_icons;
  FXbool gui_show_playing_albumcover;
//  FXbool gui_show_albumcovers;
  FXbool gui_merge_albums;
  FXbool gui_tray_icon;
  FXbool gui_tray_icon_disabled;
  FXbool gui_show_playing_titlebar;
  FXbool gui_use_opengl;

  FXColor gui_row_color;
  FXColor gui_play_color;
  FXColor gui_playtext_color;
  FXColor gui_sourceselect_color;
  FXColor gui_sourceselecttext_color;
  FXColor gui_menu_base_color;
  FXColor gui_tray_color;
  FXint   gui_coverdisplay_size;

  FXint  play_replaygain;
  FXuint play_repeat;
  FXbool play_close_stream;
  FXbool play_pause_close_device;
  FXbool play_gapless;
  FXbool play_shuffle;
  FXbool play_open_device_on_startup;
  FXbool play_from_queue;

  FXuint export_encoding;
  FXbool export_lowercase;
  FXbool export_lowercase_extension;
  FXbool export_underscore;

  FXbool dbus_notify_daemon;
#ifdef HAVE_DBUS
  FXbool dbus_mpris1;
  FXbool dbus_mpris2;
#endif
public:
  /// Default Constructor
  GMPreferences();
  GMPreferences(const GMPreferences & p);

  void parseCommandLine(int argc,char **argv);

  void resetColors();

  /// Set Key Words
  void setKeyWords(const FXString & keywords);

  void getKeyWords(FXString & keywords) const;

  /// Save to FXSettings
  void save(FXSettings & settings) const;

  /// Load from FXSettings
  void load(FXSettings & settings);

  };

#endif

