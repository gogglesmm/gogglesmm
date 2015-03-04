/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2007-2015 by Sander Jansen. All Rights Reserved      *
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
#ifndef GMSTREAMSOURCE_H
#define GMSTREAMSOURCE_H

class GMSource;

class GMStreamSource : public GMSource {
FXDECLARE(GMStreamSource)
  GMTrackDatabase * db;
protected:
  GMStreamSource();
private:
  GMStreamSource(const GMStreamSource&);
  GMStreamSource& operator=(const GMStreamSource&);
public:
  enum {
    ID_NEW_STATION = GMSource::ID_LAST,
    ID_EDIT_STATION,
    ID_DELETE_STATION,
    ID_LAST
    };
public:
  long onCmdNewStation(FXObject*,FXSelector,void*);
  long onCmdEditStation(FXObject*,FXSelector,void*);
  long onCmdDeleteStation(FXObject*,FXSelector,void*);
  long onUpdExport(FXObject*,FXSelector,void*);
public:
  GMStreamSource(GMTrackDatabase * db);

  virtual void configure(GMColumnList&);

  FXbool hasCurrentTrack(GMSource * ) const;

  virtual FXbool getTrack(GMTrack & info) const;

  virtual FXbool setTrack(GMTrack & info) const;

  FXString getName() const { return fxtr("Internet Radio"); }

  FXint getType() const { return SOURCE_INTERNET_RADIO; }

  FXString settingKey() const { return "internet-radio"; }

  FXint getSortColumn(FXbool) const { return HEADER_TRACK; }

  FXbool canBrowse() const { return false; }

  FXbool defaultBrowse() const { return false; }

  FXbool autoPlay() const { return false; }

  FXbool source_menu(FXMenuPane * pane);

  FXbool source_context_menu(FXMenuPane * pane);

  FXbool track_context_menu(FXMenuPane * pane);

  FXbool listTracks(GMTrackList * tracklist,const FXIntList & albumlist,const FXIntList & genrelist);

  virtual ~GMStreamSource();
  };

#endif
