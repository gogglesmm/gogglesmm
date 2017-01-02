/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2010-2017 by Sander Jansen. All Rights Reserved      *
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
#ifndef GMAPPSTATUSNOTIFY_H
#define GMAPPSTATUSNOTIFY_H

class GMAppStatusNotify : public GMDBusProxy {
FXDECLARE(GMAppStatusNotify)
protected:
  GMAppStatusNotify();
private:
  GMAppStatusNotify(const GMAppStatusNotify*);
  GMAppStatusNotify& operator=(const GMAppStatusNotify&);
public:
  long onSignal(FXObject*,FXSelector,void*);
public:
  GMAppStatusNotify(GMDBus * bus);
  void show();

  void notify_status_change();
  
  void notify_track_change(const GMTrack & track);

  ~GMAppStatusNotify();
  };
#endif
