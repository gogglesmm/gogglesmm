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
#ifndef GMICONTHEME_H
#define GMICONTHEME_H

struct GMIconSet {
  FXString name;
  FXString dir;
  FXString small;
  FXString medium;
  FXString large;
  void load(FXStream & store);
  void save(FXStream & store);
  };

typedef FXArray<GMIconSet> GMIconSetList;

class GMIconTheme {
private:
  FXApp * app;
private:
  GMIconSetList iconsets;
  FXStringList  basedirs;
  FXint         set;
  FXint         smallsize;
  FXint         mediumsize;
  FXint         largesize;
  FXbool        rsvg;
protected:
  FXIcon  * loadIcon(const FXString & filename);
  FXImage * loadImage(const FXString & filename);
protected:
  void loadIcon(FXIconPtr & icon,const FXString &pathlist,FXint size,const char * value,const FXColor blend);
  void loadResource(FXIconPtr & icon,const void * data,const FXColor blend,const char * type="png");
protected:
  FXbool load_cache();
  void   save_cache();
  void   build();
  FXString get_svg_cache();
  void clear_svg_cache();
public:
  FXIconPtr icon_applogo;
  FXIconPtr icon_applogo_small;
public:
  FXIconPtr icon_play;
  FXIconPtr icon_pause;
  FXIconPtr icon_stop;
  FXIconPtr icon_next;
  FXIconPtr icon_prev;
  FXIconPtr icon_play_toolbar;
  FXIconPtr icon_pause_toolbar;
  FXIconPtr icon_stop_toolbar;
  FXIconPtr icon_next_toolbar;
  FXIconPtr icon_prev_toolbar;
  FXIconPtr icon_copy;
  FXIconPtr icon_cut;
  FXIconPtr icon_paste;
  FXIconPtr icon_exit;
  FXIconPtr icon_close;
  FXIconPtr icon_settings;
  FXIconPtr icon_import;
  FXIconPtr icon_delete;
  FXIconPtr icon_edit;
  FXIconPtr icon_undo;
  FXIconPtr icon_info;
  FXIconPtr icon_sort;
  FXIconPtr icon_album;
  FXIconPtr icon_artist;
  FXIconPtr icon_genre;
  FXIconPtr icon_source_library;
  FXIconPtr icon_source_playlist;
  FXIconPtr icon_source_playqueue;
  FXIconPtr icon_source_local;
  FXIconPtr icon_source_internetradio;
  FXIconPtr icon_source_podcast;
  FXIconPtr icon_playqueue;
  FXIconPtr icon_volume_high;
  FXIconPtr icon_volume_medium;
  FXIconPtr icon_volume_low;
  FXIconPtr icon_volume_muted;
  FXIconPtr icon_volume_high_toolbar;
  FXIconPtr icon_volume_medium_toolbar;
  FXIconPtr icon_volume_low_toolbar;
  FXIconPtr icon_volume_muted_toolbar;
  FXIconPtr icon_export;
  FXIconPtr icon_find;
  FXIconPtr icon_sync;
  FXIconPtr icon_nocover;
  FXIconPtr icon_customize;
  FXIconPtr icon_document;
  FXIconPtr icon_create;
  FXIconPtr icon_media;
  FXIconPtr icon_home;
  FXIconPtr icon_file_small;
  FXIconPtr icon_file_big;
  FXIconPtr icon_audio_small;
  FXIconPtr icon_audio_big;
  FXIconPtr icon_folder_open_small;
  FXIconPtr icon_folder_small;
  FXIconPtr icon_folder_big;
  FXIconPtr icon_image_small;
  FXIconPtr icon_image_big;
  FXIconPtr icon_progress;
public:
  FXCursorPtr cursor_hand;
private:
  static GMIconTheme * me;
public:
  static GMIconTheme * instance();
public:
  GMIconTheme(FXApp * app);

  void loadSmall(FXIconPtr & icon,const char * value,const FXColor blend);

  void loadMedium(FXIconPtr & icon,const char * value,const FXColor blend);

  void loadLarge(FXIconPtr & icon,const char * value,const FXColor blend);

  FXImage * loadSmall(const char * value);

  FXint getSmallSize() const { return smallsize; }

  FXint getMediumSize() const { return mediumsize; }

  FXint getLargeSize() const { return largesize; }

  FXint getNumThemes() const;

  void setCurrentTheme(FXint i);

  FXint getCurrentTheme() const;

  FXString getThemeName(FXint i);

  void load();

  void loadExternal();

  void loadInternal();

  ~GMIconTheme();
  };

extern void gm_set_application_icon(FXWindow*);


#endif

