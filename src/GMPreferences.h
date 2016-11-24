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
#ifndef GMPREFERENCES_H
#define GMPREFERENCES_H

#include "GMFilename.h"

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
  FXString exclude_folder;
  FXString exclude_file;
  FXString filename_template     = "%P/%A/%N %T";
  FXuint   parse_method          = PARSE_BOTH;
  FXuint   id3v1_encoding        = GMFilename::ENCODING_8859_1;
  FXbool   track_from_filelist   = false;
  FXbool   replace_underscores   = true;
  FXbool   fix_album_artist      = false;
  FXbool   album_format_grouping = true;
  FXbool   detect_compilation    = false;
  FXbool   fetch_lyrics          = false;
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
  FXbool import_new     = true;
  FXbool remove_missing = false;
  FXbool update         = false;
  FXbool update_always  = false;
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

  FXString export_format_template       = "%N %T";
  FXString export_character_filter      = "\'\\#~!\"$&();<>|`^*?[]/.:";
  FXString gui_format_title             = "%P - %T";

  FXbool gui_show_status_bar            = true;
  FXbool gui_hide_player_when_close     = false;
  FXbool gui_toolbar_bigicons           = true;
  FXbool gui_toolbar_docktop            = true;
  FXbool gui_toolbar_showlabels         = true;
  FXbool gui_toolbar_labelsabove        = true;
  FXbool gui_show_browser_icons         = true;
  FXbool gui_show_playing_albumcover    = true;
  FXbool gui_tray_icon                  = false;
  FXbool gui_tray_icon_disabled         = false;
  FXbool gui_show_playing_titlebar      = false;
  FXbool gui_use_opengl                 = true;

  FXColor gui_row_color                 = 0;
  FXColor gui_play_color                = 0;
  FXColor gui_playtext_color            = 0;
  FXColor gui_sourceselect_color        = 0;
  FXColor gui_sourceselecttext_color    = 0;
  FXColor gui_menu_base_color           = 0;
  FXColor gui_tray_color                = 0;
  FXint   gui_coverdisplay_size         = 128;

  FXint  play_replaygain                = REPLAYGAIN_OFF;
  FXuint play_repeat                    = REPEAT_ALL;
  FXbool play_shuffle                   = false;
  FXbool play_from_queue                = false;

  FXuint export_encoding                = GMFilename::ENCODING_ASCII;
  FXbool export_lowercase               = false;
  FXbool export_lowercase_extension     = true;
  FXbool export_underscore              = false;

#ifdef HAVE_DBUS
  FXbool dbus_notify_daemon             = true;
  FXbool dbus_mpris1                    = true;
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

