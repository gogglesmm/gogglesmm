/*******************************************************************************
*                         Goggles Audio Player Library                         *
********************************************************************************
*           Copyright (C) 2010-2016 by Sander Jansen. All Rights Reserved      *
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
#ifndef AP_H
#define AP_H

#ifdef _WIN32
  #ifdef GAP_DLL
    #define GMAPI __declspec(dllimport)
  #else
    #define GMAPI
  #endif
#else
  #if __GNUC__ >= 4
    #define GMAPI __attribute__ ((visibility("default")))
  #else
    #define GMAPI
  #endif
#endif


#include <ap_event.h>
#include <ap_event_queue.h>
#include <ap_app_queue.h>
#include <ap_device.h>
#include <ap_player.h>
#include <ap_common.h>
#include <ap_http.h>
#include <ap_xml_parser.h>

using namespace ap;

#endif

