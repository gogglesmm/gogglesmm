/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2006-2018 by Sander Jansen. All Rights Reserved      *
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
#ifndef GMCLIPBOARD_H
#define GMCLIPBOARD_H

class GMClipboard;

class GMClipboardData{
public:
  GMClipboardData(){}
  virtual FXbool request(FXDragType dragtype,GMClipboard*)=0;
  virtual ~GMClipboardData() {}
  };


class GMClipboard : public FXShell {
FXDECLARE(GMClipboard)
private:
  static GMClipboard * me;
public:
  static FXDragType theclipboard;
  static FXDragType kdeclipboard;
  static FXDragType gnomeclipboard;
  static FXDragType gnomedragndrop;
  static FXDragType trackdatabase;
  static FXDragType selectedtracks;
  static FXDragType alltracks;
protected:
  GMClipboardData * clipdata = nullptr;
  FXObject        * clipowner = nullptr;
protected:
  GMClipboard();
  virtual bool doesOverrideRedirect() const;
private:
  GMClipboard(const GMClipboard&);
  GMClipboard& operator=(const GMClipboard&);
public:
  long onClipboardLost(FXObject*,FXSelector,void*);
  long onClipboardGained(FXObject*,FXSelector,void*);
  long onClipboardRequest(FXObject*,FXSelector,void*);
public:
  static GMClipboard * instance();
public:
  GMClipboard(FXApp * app);

  FXbool acquire(FXObject * owner,const FXDragType * types,FXuint num_types,GMClipboardData * data);

  FXbool owned(FXObject * obj);

  FXbool release();

  GMClipboardData * getClipData() const { return clipdata; }

  void saveClipboard();

  virtual void create();

  ~GMClipboard();
  };

#endif
