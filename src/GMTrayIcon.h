/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2008-2010 by Sander Jansen. All Rights Reserved      *
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
#ifndef GMTRAYICON_H
#define GMTRAYICON_H

class GMTrayIcon : public GMPlug {
FXDECLARE(GMTrayIcon)
protected:
  FXID    xtraywindow;
  FXID    xtrayopcode;
  FXID    xtrayorientation;
  FXID    xtrayxfceorientation;
  FXID    xtrayvisual;
protected:
  FXIcon * icon;
  FXbool   opaque;
  FXString tip;
private:
  GMTrayIcon(const GMTrayIcon*);
  GMTrayIcon& operator=(const GMTrayIcon&);
protected:
  GMTrayIcon();
  FXbool findSystemTray();
  void requestDock();
  FXuint getTrayOrientation();
  FXuint getTrayVisual();
public:
  long onPaint(FXObject*,FXSelector,void*);
  long onConfigure(FXObject*,FXSelector,void*);
  long onLeftBtnPress(FXObject*,FXSelector,void*);
  long onMiddleBtnPress(FXObject*,FXSelector,void*);
  long onRightBtnRelease(FXObject*,FXSelector,void*);
  long onMouseWheel(FXObject*,FXSelector,void*);
  long onQueryTip(FXObject*,FXSelector,void*);
public:
  GMTrayIcon(FXApp * app);

  virtual void create();

  void setToolTip(const FXString & t) { tip = t; }

  void display(const GMTrack&);

  void reset();

  void dock();

  void updateIcon();

  virtual ~GMTrayIcon();
  };

#endif
