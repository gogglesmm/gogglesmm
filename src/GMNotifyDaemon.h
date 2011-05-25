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
#ifndef GMNOTIFY_H
#define GMNOTIFY_H

class GMNotifyDaemon : public GMDBusProxy {
FXDECLARE(GMNotifyDaemon)
protected:
  FXString appname;
  FXString appicon;
  FXint msgid;
  FXbool persistent;
protected:
  GMNotifyDaemon();
private:
  GMNotifyDaemon(const GMNotifyDaemon&);
  GMNotifyDaemon& operator=(const GMNotifyDaemon&);
public:
  enum {
    ID_NOTIFY_REPLY=1,
    ID_NOTIFY_CAPABILITIES
    };
public:
  long onSignal(FXObject*,FXSelector,void*);
  long onMethod(FXObject*,FXSelector,void*);
  long onNotifyReply(FXObject*,FXSelector,void*);
  long onNotifyCapabilities(FXObject*,FXSelector,void*);
public:
  GMNotifyDaemon(GMDBus*);
  void init();
  void close();

  void reset();
  void notify(const FXchar * summary,const FXchar * body,FXint timeout,FXImage* img);



public:
  void notify_track_change(const GMTrack & track,FXImage * cover);




  };

#endif



