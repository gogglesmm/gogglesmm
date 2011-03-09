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
  FXint msgid;
protected:
  GMNotifyDaemon();
private:
  GMNotifyDaemon(const GMNotifyDaemon&);
  GMNotifyDaemon& operator=(const GMNotifyDaemon&);
public:
  enum {
    ID_NOTIFY_REPLY=1
    };
public:
  long onSignal(FXObject*,FXSelector,void*);
  long onMethod(FXObject*,FXSelector,void*);
  long onNotifyReply(FXObject*,FXSelector,void*);
public:
  GMNotifyDaemon(GMDBus*);
  void close();
  void notify(const FXchar * app,const FXchar * icon,const FXchar * summary,const FXchar * body,FXint timeout,FXImage* img);
  };

#endif



