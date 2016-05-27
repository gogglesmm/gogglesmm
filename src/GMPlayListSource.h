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

  FXbool hasCurrentTrack(GMSource * ) const override;

  FXbool findCurrent(GMTrackList * tracklist,GMSource * src) override;

  void resetCurrent() override { current_track=-1; current_queue=-1; }

  void markCurrent(const GMTrackItem*item) override;

  FXString getName() const override { return name.text(); }

  FXint getType() const override { return SOURCE_DATABASE_PLAYLIST; }

  FXint getSortColumn(FXbool browse) const override { if (browse) return HEADER_BROWSE; else return HEADER_QUEUE; }

  FXbool getQueueColumn(FXbool browse) const override { if (browse) return false; else return true; }

  FXbool defaultBrowse() const override { return false; }

  FXString settingKey() const override { return "database_playlist_" + FXString::value(playlist); }

  void save(GMTrackList*) override;

  void dragged(GMTrackList*) override;

  void sorted(GMTrackList*,FXint method) override;

  FXbool source_menu(FXMenuPane*) override { return false; }

  FXbool source_context_menu(FXMenuPane * pane) override;

  FXbool dnd_accepts(FXDragType*,FXuint) override;

  virtual ~GMPlayListSource();
  };

#endif
