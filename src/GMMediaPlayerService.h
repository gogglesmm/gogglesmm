/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2010-2010 by Sander Jansen. All Rights Reserved      *
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
#ifndef GMMPRISSERVICE_H
#define GMMPRISSERVICE_H

#if 0
/*
  MPRIS v1
*/
class GMMediaPlayerService1 : public FXObject {
FXDECLARE(GMMediaPlayerService1)
protected:
  GMDBus * bus;
  FXbool   published;
protected:
  DBusObjectPathVTable root_vtable;
  DBusObjectPathVTable player_vtable;
  DBusObjectPathVTable tracklist_vtable;
protected:
  static DBusHandlerResult root_filter(DBusConnection*,DBusMessage*,void *);
  static DBusHandlerResult player_filter(DBusConnection*,DBusMessage*,void *);
  static DBusHandlerResult tracklist_filter(DBusConnection*,DBusMessage*,void *);
protected:
  GMMediaPlayerService1(){}
private:
  GMMediaPlayerService1(const GMMediaPlayerService1&);
  GMMediaPlayerService1& operator=(const GMMediaPlayerService1&);
public:
  GMMediaPlayerService1(GMDBus*);

  void notify_track_change(const GMTrack &);

  void notify_status_change();

  void notify_caps_change();

  virtual ~GMMediaPlayerService1();
  };


#endif

/*
  MPRIS v2 
*/
class GMMediaPlayerService : public FXObject {
FXDECLARE(GMMediaPlayerService)
protected:
  GMDBus * bus;
  FXbool   published;
protected:
  DBusObjectPathVTable mpris_vtable;
protected:
  static DBusHandlerResult mpris_filter(DBusConnection*,DBusMessage*,void *);
protected:
  GMMediaPlayerService(){}
private:
  GMMediaPlayerService(const GMMediaPlayerService&);
  GMMediaPlayerService& operator=(const GMMediaPlayerService&);
public:
  GMMediaPlayerService(GMDBus*);

  void notify_track_change(const GMTrack &);

  void notify_status_change();

  void notify_caps_change();

  virtual ~GMMediaPlayerService();
  };

#endif
