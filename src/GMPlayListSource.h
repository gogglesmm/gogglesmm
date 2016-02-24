/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2007-2016 by Sander Jansen. All Rights Reserved      *
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
#ifndef GMPLAYLISTSOURCE_H
#define GMPLAYLISTSOURCE_H

class GMPlayListSource : public GMDatabaseSource {
FXDECLARE(GMPlayListSource)
protected:
  FXString name;
  FXint    current_queue = -1;
  FXbool   orderchanged  = false;
protected:
  GMPlayListSource(){}
private:
  GMPlayListSource(const GMPlayListSource&);
  GMPlayListSource& operator=(const GMPlayListSource&);
public:
  enum {
    ID_EDIT_NAME = GMDatabaseSource::ID_LAST,
    ID_REMOVE,
    ID_IMPORT,
    ID_LAST
    };
public:
  long onCmdEditName(FXObject*,FXSelector,void*);
  long onCmdRemove(FXObject*,FXSelector,void*);
  long onCmdRemoveInPlaylist(FXObject*,FXSelector,void*);
  long onCmdImport(FXObject*,FXSelector,void*);
public:
  GMPlayListSource(GMTrackDatabase * db,FXint playlist);

  virtual FXbool hasCurrentTrack(GMSource * ) const;

  virtual FXbool findCurrent(GMTrackList * tracklist,GMSource * src);

  virtual void resetCurrent() { current_track=-1; current_queue=-1; }

  virtual void markCurrent(const GMTrackItem*item);

  virtual FXString getName() const { return name.text(); }

  virtual FXint getType() const { return SOURCE_DATABASE_PLAYLIST; }

  virtual FXint getSortColumn(FXbool browse) const { if (browse) return HEADER_BROWSE; else return HEADER_QUEUE; }

  virtual FXbool getQueueColumn(FXbool browse) const { if (browse) return false; else return true; }

  virtual FXbool defaultBrowse() const { return false; }

  virtual FXString settingKey() const { return "database_playlist_" + FXString::value(playlist); }

  virtual void save(GMTrackList*);

  virtual void dragged(GMTrackList*);

  virtual void sorted(GMTrackList*,FXint method);

  virtual FXbool source_menu(FXMenuPane*) { return false; }

  virtual FXbool source_context_menu(FXMenuPane * pane);

  virtual FXbool dnd_accepts(FXDragType*,FXuint);

  virtual ~GMPlayListSource();
  };

#endif
