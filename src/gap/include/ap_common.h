/*******************************************************************************
*                         Goggles Audio Player Library                         *
********************************************************************************
*           Copyright (C) 2010-2012 by Sander Jansen. All Rights Reserved      *
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
#ifndef AP_COMMON_H
#define AP_COMMON_H

namespace ap {

extern GMAPI void ap_parse_pls(const FXString & data,FXStringList & mrl);

extern GMAPI void ap_parse_m3u(const FXString & data,FXStringList & mrl);

extern GMAPI void ap_parse_xspf(const FXString & data,FXStringList & mrl,FXString & title);

extern GMAPI FXbool ap_set_nonblocking(FXInputHandle fd);

extern GMAPI void ap_set_thread_name(const FXchar *);

}
#endif
