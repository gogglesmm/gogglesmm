/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2009-2010 by Sander Jansen. All Rights Reserved      *
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
#ifndef GMUTILS_H
#define GMUTILS_H


enum {
  DESKTOP_SESSION_X11         = 0,
  DESKTOP_SESSION_KDE_PLASMA  = 1,
  DESKTOP_SESSION_XFCE        = 2,
  DESKTOP_SESSION_GNOME       = 3,
  DESKTOP_SESSION_LXDE        = 4
  };

extern FXuint gm_desktop_session();

extern FXbool gm_open_folder(const FXString & folder);

extern FXbool gm_open_browser(const FXString & folder);

extern FXbool gm_image_search(const FXString & query);

extern FXString gm_url_encode(const FXString& url);

extern FXString gm_make_url(const FXString&);

extern FXdouble gm_parse_number(const FXString &);

extern FXbool gm_buffer_file(const FXString & filename,FXString & buffer);

extern void gm_make_absolute_path(const FXString & path,FXStringList & urls);

extern void gm_set_window_cursor(FXWindow *,FXCursor*);

extern void gm_focus_and_select(FXTextField*);

extern void gm_run_popup_menu(FXMenuPane*,FXint rx,FXint ry);

extern FXbool gm_is_local_file(const FXString & filename);

extern void gm_convert_filenames_to_uri(const FXStringList & filenames,FXString & uri);

extern void gm_convert_filenames_to_gnomeclipboard(const FXStringList & filenames,FXString & clipboard);

extern void gm_convert_gnomeclipboard_to_filenames(FXString & clipboard,FXStringList & filenames);

extern void gm_convert_uri_to_filenames(FXString & uri,FXStringList & filenames);

extern void gm_bgra_to_rgba(FXColor * in,FXColor * out,FXint len);

extern FXImage * gm_load_image_from_data(const void * data,FXuval size,FXint scale,FXint crop=0);
extern FXImage * gm_load_image_from_file(const FXString & filename,FXint scale,FXint crop=0);

extern FXbool gm_decode_base64(FXuchar * buffer,FXint & len);

extern void gm_print_time(FXint nseconds,FXString & result);

#endif
