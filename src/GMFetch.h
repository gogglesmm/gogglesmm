/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2007-2010 by Sander Jansen. All Rights Reserved      *
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
#ifndef GMFETCH_H
#define GMFETCH_H

struct GMFetchResponse {
  FXString url;
  FXString data;
  FXString content_type;
  };

class GMFetch : public FXThread {
protected:
  static GMFetch * fetch;
public:
  FXMessageChannel gui;
  FXStringList     mrl;
  FXString         url;
  FXString         errormsg;
  volatile FXbool  runstatus;
protected:
  FXint run();
protected:
  GMFetch();
public:
  static void download(const FXString & url);

  static void init();

  static void cancel_and_wait();

  static FXbool busy();

  static void exit();
  };

#endif



