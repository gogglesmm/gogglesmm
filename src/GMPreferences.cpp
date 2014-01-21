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
#include "GMTrack.h"
#include "GMFilename.h"
#include "GMPreferences.h"

const char section_window[] = "window";
const char section_import[] = "import";
const char section_export[] = "export";
const char section_player[] = "player";
const char section_colors[] = "colors";
const char section_dbus[]   = "dbus";
const char section_app[]    = "application";
const char section_sync[]   = "sync";

const char key_import_default_field[]="default-user-title";
const char key_import_track_from_filelist[]="track-from-filelist";
const char key_import_replace_underscores[]="replace-underscores";
const char key_import_parse_filename_only[]="parse-filename-only";

const char key_import_filename_template[]="filename-template";
const char key_import_parse_method[]="parse-method";
const char key_import_exclude_folder[]="exclude-folder";
const char key_import_exclude_file[]="exclude-file";

const char key_export_format_template[]="format-template";
const char key_export_character_filter[]="character-filter";
const char key_export_encoding[]="encoding";
const char key_export_lowercase[]="lowercase";
const char key_export_lowercase_extension[]="lowercase-extension";
const char key_export_underscore[]="underscore";

const char key_gui_show_status_bar[]="show-statusbar";
const char key_gui_hide_player_when_close[]="hide-player-when-close";
const char key_gui_toolbar_bigicons[]="toolbar-bigicons";
const char key_gui_toolbar_docktop[]="toolbar-docktop";
const char key_gui_toolbar_showlabels[]="toolbar-labels";
const char key_gui_toolbar_labelsabove[]="toolbar-labels-above";
const char key_gui_show_browser_icons[]="browser-icons";
const char key_gui_keywords[]="sort-keywords";
const char key_gui_show_playing_albumcover[]="show-playing-albumcover";
//const char key_gui_show_albumcovers[]="show-albumcovers";
const char key_gui_merge_albums[]="merge-album-names";
const char key_gui_show_opengl_coverview[]="show-opengl-coverview";
const char key_gui_tray_icon[]="tray-icon";
const char key_gui_show_playing_titlebar[]="show-playing-titlebar";
const char key_gui_format_title[]="title-format";

const char key_gui_row_color[]="row-color";
const char key_gui_play_color[]="play-color";
const char key_gui_playtext_color[]="playtext-color";
const char key_gui_sourceselect_color[]="source-select-color";
const char key_gui_sourceselecttext_color[]="source-selecttext-color";
const char key_gui_menu_base_color[]="menu-base-color";
const char key_gui_tray_color[]="tray-back-color";
const char key_gui_coverdisplay_size[]="cover-display-size";

const char key_play_repeat[]="repeat-mode";
const char key_play_replaygain[]="replay-gain";
const char key_play_close_stream[]="close-audio-stream";
const char key_play_pause_close_device[]="pause-close-device";
const char key_play_gapless[]="gapless-playback";
const char key_play_shuffle[]="shuffle";
const char key_play_open_device_on_startup[]="open_audio_device_on_startup";
const char key_play_from_queue[]="play-from-queue";

const char key_dbus_notify_daemon[]="notification-daemon";
const char key_dbus_mpris1[]="mpris";
const char key_dbus_mpris2[]="mpris-2";

const char key_sync_import_new[]="import-new";
const char key_sync_remove_missing[]="remove-missing";
const char key_sync_remove_all[]="remove-all";
const char key_sync_update[]="update";
const char key_sync_update_always[]="update-always";


GMImportOptions::GMImportOptions() :
  default_field("Untitled"),
  filename_template("%P/%A/%N %T"),
  track_from_filelist(false),
  replace_underscores(true),
  fix_album_artist(false),
  parse_method(PARSE_BOTH){
  }

void GMImportOptions::save(FXSettings & reg) const {
  reg.writeBoolEntry(section_import,key_import_track_from_filelist,track_from_filelist);
  reg.writeBoolEntry(section_import,key_import_replace_underscores,replace_underscores);
  reg.writeStringEntry(section_import,key_import_default_field,default_field.text());
  reg.writeStringEntry(section_import,key_import_filename_template,filename_template.text());
  reg.writeStringEntry(section_import,key_import_exclude_folder,exclude_folder.text());
  reg.writeStringEntry(section_import,key_import_exclude_file,exclude_file.text());
  reg.writeUIntEntry(section_import,key_import_parse_method,parse_method);
  }

void GMImportOptions::load(FXSettings & reg) {
  track_from_filelist    = reg.readBoolEntry(section_import,key_import_track_from_filelist,track_from_filelist);
  replace_underscores    = reg.readBoolEntry(section_import,key_import_replace_underscores,replace_underscores);
  default_field          = reg.readStringEntry(section_import,key_import_default_field,default_field.text());
  filename_template      = reg.readStringEntry(section_import,key_import_filename_template,filename_template.text());
  exclude_folder         = reg.readStringEntry(section_import,key_import_exclude_folder,exclude_folder.text());
  exclude_file           = reg.readStringEntry(section_import,key_import_exclude_file,exclude_file.text());
  parse_method           = FXMIN(reg.readUIntEntry(section_import,key_import_parse_method,parse_method),(FXuint)PARSE_BOTH);
  }


GMSyncOptions::GMSyncOptions() :
  import_new(true),
  remove_missing(true),
  remove_all(false),
  update(false),
  update_always(false) {
  }

void GMSyncOptions::save(FXSettings & reg) const {
  reg.writeBoolEntry(section_sync,key_sync_import_new,import_new);
  reg.writeBoolEntry(section_sync,key_sync_remove_missing,remove_missing);
  reg.writeBoolEntry(section_sync,key_sync_remove_all,remove_all);
  reg.writeBoolEntry(section_sync,key_sync_update,update);
  reg.writeBoolEntry(section_sync,key_sync_update_always,update_always);
  }

void GMSyncOptions::load(FXSettings & reg) {
  import_new     = reg.readBoolEntry(section_sync,key_sync_import_new,import_new);
  remove_missing = reg.readBoolEntry(section_sync,key_sync_remove_missing,remove_missing);
  remove_all     = reg.readBoolEntry(section_sync,key_sync_remove_all,remove_all);
  update         = reg.readBoolEntry(section_sync,key_sync_update,update);
  update_always  = reg.readBoolEntry(section_sync,key_sync_update_always,update_always);
  }


GMPreferences::GMPreferences() :
  export_format_template("%N %T"),
  export_character_filter("\'\\#~!\"$&();<>|`^*?[]/.:"),
  gui_format_title("%N - %T - %P"),

  gui_show_status_bar(true),
  gui_hide_player_when_close(false),
  gui_toolbar_bigicons(true),
  gui_toolbar_docktop(true),
  gui_toolbar_showlabels(true),
  gui_toolbar_labelsabove(true),
  gui_show_browser_icons(true),
  gui_show_playing_albumcover(true),
  //gui_show_albumcovers(false),
  gui_merge_albums(true),
  gui_tray_icon(false),
  gui_tray_icon_disabled(false),
  gui_show_playing_titlebar(false),
  gui_use_opengl(true),
  gui_coverdisplay_size(256),

  play_replaygain(REPLAYGAIN_OFF),
  play_repeat(REPEAT_ALL),
  play_close_stream(false),
  play_pause_close_device(false),
  play_gapless(true),
  play_shuffle(false),
  play_open_device_on_startup(false),
  play_from_queue(false),

  export_encoding(GMFilename::ENCODING_ASCII),
  export_lowercase(false),
  export_lowercase_extension(true),
  export_underscore(false),

  dbus_notify_daemon(true),
  dbus_mpris1(true),
  dbus_mpris2(true){
  resetColors();

  }

void GMPreferences::save(FXSettings & reg) const {
  FXString keywords;
  getKeyWords(keywords);

  /// Write out version information
  reg.writeIntEntry(section_app,"major-version",APPLICATION_MAJOR);
  reg.writeIntEntry(section_app,"minor-version",APPLICATION_MINOR);
  reg.writeIntEntry(section_app,"level-version",APPLICATION_LEVEL);

  import.save(reg);
  sync.save(reg);

  /// Export
  reg.writeBoolEntry(section_export,key_export_lowercase,export_lowercase);
  reg.writeBoolEntry(section_export,key_export_lowercase_extension,export_lowercase_extension);
  reg.writeBoolEntry(section_export,key_export_underscore,export_underscore);
  reg.writeStringEntry(section_export,key_export_format_template,export_format_template.text());
  reg.writeStringEntry(section_export,key_export_character_filter,export_character_filter.text());
  reg.writeUIntEntry(section_export,key_export_encoding,export_encoding);

  /// Colors
  reg.writeColorEntry(section_colors,key_gui_row_color,gui_row_color);
  reg.writeColorEntry(section_colors,key_gui_play_color,gui_play_color);
  reg.writeColorEntry(section_colors,key_gui_playtext_color,gui_playtext_color);
  reg.writeColorEntry(section_colors,key_gui_sourceselect_color,gui_sourceselect_color);
  reg.writeColorEntry(section_colors,key_gui_sourceselecttext_color,gui_sourceselecttext_color);
  reg.writeColorEntry(section_colors,key_gui_menu_base_color,gui_menu_base_color);
  reg.writeColorEntry(section_colors,key_gui_tray_color,gui_tray_color);

  /// Window
  reg.writeBoolEntry(section_window,key_gui_hide_player_when_close,gui_hide_player_when_close);
  reg.writeBoolEntry(section_window,key_gui_show_status_bar,gui_show_status_bar);
  reg.writeBoolEntry(section_window,key_gui_toolbar_bigicons,gui_toolbar_bigicons);
  reg.writeBoolEntry(section_window,key_gui_toolbar_docktop,gui_toolbar_docktop);
  reg.writeBoolEntry(section_window,key_gui_toolbar_showlabels,gui_toolbar_showlabels);
  reg.writeBoolEntry(section_window,key_gui_toolbar_labelsabove,gui_toolbar_labelsabove);
  reg.writeBoolEntry(section_window,key_gui_show_browser_icons,gui_show_browser_icons);
  reg.writeStringEntry(section_window,key_gui_keywords,keywords.text());
  reg.writeBoolEntry(section_window,key_gui_show_playing_albumcover,gui_show_playing_albumcover);
//  reg.writeBoolEntry(section_window,key_gui_show_albumcovers,gui_show_albumcovers);
  reg.writeBoolEntry(section_window,key_gui_merge_albums,gui_merge_albums);
  reg.writeBoolEntry(section_window,key_gui_tray_icon,gui_tray_icon);
  reg.writeBoolEntry(section_window,key_gui_show_playing_titlebar,gui_show_playing_titlebar);
  reg.writeBoolEntry(section_window,key_gui_show_opengl_coverview,gui_use_opengl);
  reg.writeStringEntry(section_window,key_gui_format_title,gui_format_title.text());
  reg.writeIntEntry(section_window,key_gui_coverdisplay_size,gui_coverdisplay_size);

  /// Player
  reg.writeIntEntry(section_player,key_play_repeat,play_repeat);
  reg.writeIntEntry(section_player,key_play_replaygain,play_replaygain);
  reg.writeBoolEntry(section_player,key_play_close_stream,play_close_stream);
  reg.writeBoolEntry(section_player,key_play_pause_close_device,play_pause_close_device);
  reg.writeBoolEntry(section_player,key_play_gapless,play_gapless);
  reg.writeBoolEntry(section_player,key_play_shuffle,play_shuffle);
  reg.writeBoolEntry(section_player,key_play_open_device_on_startup,play_open_device_on_startup);
  reg.writeBoolEntry(section_player,key_play_from_queue,play_from_queue);

  /// Dbus
  reg.writeBoolEntry(section_dbus,key_dbus_notify_daemon,dbus_notify_daemon);
  reg.writeBoolEntry(section_dbus,key_dbus_mpris1,dbus_mpris1);
  reg.writeBoolEntry(section_dbus,key_dbus_mpris2,dbus_mpris2);

  }


void GMPreferences::load(FXSettings & reg) {
  FXString keywords="a;an;the";

  /// Remove Keys that interfere with new ones..
  if (reg.readIntEntry(section_app,"major-version",0)==0 && reg.readIntEntry(section_app,"minor-version",8)<9) {
    FXint s;
    for (s = 0; s < reg.no(); s++){
      if (!reg.empty(s)) {
        reg.data(s).remove("browse-showcolumn-no");
        reg.data(s).remove("list-showcolumn-no");
        }
      }
    }

  /// Write out version information
  reg.writeIntEntry(section_app,"major-version",APPLICATION_MAJOR);
  reg.writeIntEntry(section_app,"minor-version",APPLICATION_MINOR);
  reg.writeIntEntry(section_app,"level-version",APPLICATION_LEVEL);


  import.load(reg);
  sync.load(reg);

  /// Export
  export_lowercase              = reg.readBoolEntry(section_export,key_export_lowercase,export_lowercase);
  export_lowercase_extension    = reg.readBoolEntry(section_export,key_export_lowercase_extension,export_lowercase_extension);
  export_underscore             = reg.readBoolEntry(section_export,key_export_underscore,export_underscore);
  export_format_template        = reg.readStringEntry(section_export,key_export_format_template,export_format_template.text());
  export_character_filter       = reg.readStringEntry(section_export,key_export_character_filter,export_character_filter.text());
  export_encoding 							= FXMIN(GMFilename::ENCODING_LAST-1,reg.readUIntEntry(section_export,key_export_encoding,export_encoding));

  /// Colors
  gui_row_color                 = reg.readColorEntry(section_colors,key_gui_row_color,gui_row_color);
  gui_play_color                = reg.readColorEntry(section_colors,key_gui_play_color,gui_play_color);
  gui_playtext_color            = reg.readColorEntry(section_colors,key_gui_playtext_color,gui_playtext_color);
  gui_sourceselect_color        = reg.readColorEntry(section_colors,key_gui_sourceselect_color,gui_sourceselect_color);
  gui_sourceselecttext_color    = reg.readColorEntry(section_colors,key_gui_sourceselecttext_color,gui_sourceselecttext_color);
  gui_menu_base_color    				= reg.readColorEntry(section_colors,key_gui_menu_base_color,gui_menu_base_color);
  gui_tray_color                = reg.readColorEntry(section_colors,key_gui_tray_color,FXApp::instance()->getBaseColor());

  /// Window
  gui_hide_player_when_close    = reg.readBoolEntry(section_window,key_gui_hide_player_when_close,gui_hide_player_when_close);
  gui_show_status_bar           = reg.readBoolEntry(section_window,key_gui_show_status_bar,gui_show_status_bar);
  gui_toolbar_bigicons          = reg.readBoolEntry(section_window,key_gui_toolbar_bigicons,gui_toolbar_bigicons);
  gui_toolbar_docktop           = reg.readBoolEntry(section_window,key_gui_toolbar_docktop,gui_toolbar_docktop);
  gui_toolbar_showlabels        = reg.readBoolEntry(section_window,key_gui_toolbar_showlabels,gui_toolbar_showlabels);
  gui_toolbar_labelsabove       = reg.readBoolEntry(section_window,key_gui_toolbar_labelsabove,gui_toolbar_labelsabove);
  gui_show_browser_icons        = reg.readBoolEntry(section_window,key_gui_show_browser_icons,gui_show_browser_icons);
  keywords                      = reg.readStringEntry(section_window,key_gui_keywords,keywords.text());
  gui_show_playing_albumcover   = reg.readBoolEntry(section_window,key_gui_show_playing_albumcover,gui_show_playing_albumcover);
//  gui_show_albumcovers          = reg.readBoolEntry(section_window,key_gui_show_albumcovers,gui_show_albumcovers);
  gui_tray_icon                 = reg.readBoolEntry(section_window,key_gui_tray_icon,gui_tray_icon);
  gui_show_playing_titlebar     = reg.readBoolEntry(section_window,key_gui_show_playing_titlebar,gui_show_playing_titlebar);
  gui_format_title              = reg.readStringEntry(section_window,key_gui_format_title,gui_format_title.text());
  gui_use_opengl                = reg.readBoolEntry(section_window,key_gui_show_opengl_coverview,gui_use_opengl);
  gui_coverdisplay_size         = reg.readIntEntry(section_window,key_gui_coverdisplay_size,gui_coverdisplay_size);

  /// Player
  play_repeat                   = reg.readIntEntry(section_player,key_play_repeat,play_repeat);
  play_replaygain               = reg.readIntEntry(section_player,key_play_replaygain,play_replaygain);
  play_close_stream             = reg.readBoolEntry(section_player,key_play_close_stream,play_close_stream);
  play_pause_close_device       = reg.readBoolEntry(section_player,key_play_pause_close_device,play_pause_close_device);
  play_gapless                  = reg.readBoolEntry(section_player,key_play_gapless,play_gapless);
  play_shuffle                  = reg.readBoolEntry(section_player,key_play_shuffle,play_shuffle);
  play_open_device_on_startup   = reg.readBoolEntry(section_player,key_play_open_device_on_startup,play_open_device_on_startup);
  play_from_queue               = reg.readBoolEntry(section_player,key_play_from_queue,play_from_queue);

  /// Dbus
  dbus_notify_daemon            = reg.readBoolEntry(section_dbus,key_dbus_notify_daemon,dbus_notify_daemon);
  dbus_mpris1                   = reg.readBoolEntry(section_dbus,key_dbus_mpris1,dbus_mpris1);
  dbus_mpris2                   = reg.readBoolEntry(section_dbus,key_dbus_mpris2,dbus_mpris2);

  setKeyWords(keywords);
  }



void GMPreferences::resetColors(){
  gui_row_color=FXRGB(240,240,240);
  gui_play_color=FXRGB(210,230,210);
  gui_playtext_color=FXRGB(0,0,0);
  gui_sourceselect_color=FXRGB(210,230,210);
  gui_sourceselecttext_color=FXRGB(0,0,0);
  gui_menu_base_color=FXRGB(255,255,255);
  gui_tray_color=FXRGB(0,0,0);
  }


void gm_make_string_list(const FXString & input,const FXchar sep,FXStringList & output) {
  FXint s=0,e,n;
//  FXint len = input.length();
//  for (i=0;i<len;i++) {

  while(s<input.length()) {

//    fxmessage("s: %d\n",s);
    /// Skip white space.
    while(input[s]==' ') s++;
    e=s;

    /// End of delimiter
    while(input[e]!=sep && input[e]!='\0') e++;

    n=e+1;
    while(input[e-1]==' ' && (e-1)>s) e--;

    if (e-s) output.append(input.mid(s,e-s));
    s=n;
    }



/*
    if (skip && input[i]==' ')
      s++;
    else if (input[i]==sep) {
      FXString item = input.mid(s,i-s).trim();
      if (!item.empty()) output.append(item);
      s=i+1;
      }
    }
  if ((len-s)>0) {
    FXString item = input.mid(s,i-s).trim();
    if (!item.empty()) output.append(item);
    }
*/
  }


void GMPreferences::setKeyWords(const FXString & keywords) {
  gui_sort_keywords.clear();

//  fxmessage("input: \"%s\"\n",keywords.text());
  gm_make_string_list(keywords,';',gui_sort_keywords);
  for (FXint i=0;i<gui_sort_keywords.no();i++){
    gui_sort_keywords[i]+=' ';
    }

//    fxmessage("key: \"%s\"\n",gui_sort_keywords[i].text());
 //   }
  }

void GMPreferences::getKeyWords(FXString & keywords) const {
  if (gui_sort_keywords.no()){
    keywords+=FXString(gui_sort_keywords[0]).trim();
    for (FXint i=1;i<gui_sort_keywords.no();i++) {
      keywords+=";" + FXString(gui_sort_keywords[i]).trim();
      }
 //   fxmessage("get %s\n",keywords.text());
    }
  }

void GMPreferences::parseCommandLine(int argc,char **argv){
  for (FXint i=0;i<argc;i++){
    if (comparecase(argv[i],"--disable-opengl")==0) {
      gui_use_opengl=false;
      }
    }
  }












