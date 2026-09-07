/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2007-2026 by Sander Jansen. All Rights Reserved      *
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
*                               ---                                            *
* SPDX-License-Identifier: GPL-3.0-or-later                                    *
********************************************************************************/
#ifndef GMDATABASESOURCE_H
#define GMDATABASESOURCE_H

#ifndef GMCLIPBOARD_H
#include "GMClipboard.h"
#endif

class GMSource;
class GMTrackDatabase;

class GMDatabaseClipboardData : public GMClipboardData {
public:
  GMTrackDatabase * db;
  FXIntList         tracks;
public:
  FXbool request(FXDragType target,GMClipboard * clipboard) override;

  ~GMDatabaseClipboardData() override {
    db=nullptr;
    }
  };




class GMDatabaseSource : public GMSource {
FXDECLARE(GMDatabaseSource)
protected:
  static GMDatabaseSource * filterowner;
  static GMCoverCache     * covercache;
protected:
  GMTrackDatabase   * db         = nullptr;
  FXint               playlist   = 0;
  FXString            filter;
  FXuint              filtermask = FILTER_DEFAULT;
  FXbool              hasfilter  = false;
  FXbool              hasview    = false;
  FXString            dndfiles;
protected:
  GMDatabaseSource() = default;
protected:
  void removeFiles(const FXStringList & files);
  [[nodiscard]] FXbool hasFilter() const { return hasfilter; }
public:
  enum {
    ID_NEW_PLAYLIST = GMSource::ID_LAST,
    ID_IMPORT_PLAYLIST,
    ID_IMPORT_FILES,
    ID_CLEAR,
    ID_COLUMN_TRACK,
    ID_COLUMN_QUEUE,
    ID_COLUMN_ARTIST,
    ID_COLUMN_ALBUM,
    ID_COLUMN_GENRE,
    ID_COLUMN_TIME,
    ID_LOAD_ALBUM_ICONS,
    ID_OPEN_FOLDER,
    ID_EDIT_RATING,
    ID_ADD_COVER,
    ID_SEARCH_COVER,
    ID_SEARCH_COVER_ALBUM,
    ID_LOAD_COVERS,
    ID_NEW_FILTER,
    ID_LAST
    };
public:
  long onCmdEditTrack(FXObject*,FXSelector,void*);
  long onCmdDelete(FXObject*,FXSelector,void*);
  long onCmdCopyArtistAlbum(FXObject*,FXSelector,void*);
  long onCmdCopyTrack(FXObject*,FXSelector,void*);
  long onCmdRequestArtistAlbum(FXObject*,FXSelector,void*);
  long onCmdRequestTrack(FXObject*,FXSelector,void*);
  long onCmdExport(FXObject*,FXSelector,void*);
  long onUpdExport(FXObject*,FXSelector,void*);
  long onCmdDrop(FXObject*,FXSelector,void*);
  long onCmdPaste(FXObject*,FXSelector,void*);
  long onUpdPaste(FXObject*,FXSelector,void*);
  long onCmdNewPlayList(FXObject*,FXSelector,void*);
  long onCmdImportPlayList(FXObject*,FXSelector,void*);
  long onCmdClear(FXObject*,FXSelector,void*);
  long onCmdTrackPlayed(FXObject*,FXSelector,void*);
  long onQueryTip(FXObject*,FXSelector,void*);
  long onCmdOpenFolder(FXObject*,FXSelector,void*);
  long onCmdEditRating(FXObject*,FXSelector,void*);
  long onDndImportFiles(FXObject*,FXSelector,void*);
  long onCmdAddCover(FXObject*,FXSelector,void*);
  long onCmdSearchCover(FXObject*,FXSelector,void*);
  long onCmdLoadCovers(FXObject*,FXSelector,void*);
  long onCmdNewFilter(FXObject*,FXSelector,void*);
public:
  explicit GMDatabaseSource(GMTrackDatabase * db);

  void shutdown();


  [[nodiscard]] GMTrackListSortFunc getSortBrowse(FXbool album_list_mode) const override;


  virtual void addTracks(GMSource * src,const FXIntList & tracks);

  [[nodiscard]] GMCoverCache* getCoverCache() const override { return covercache; }

  void loadCovers() override;

  void updateCovers() override;

  [[nodiscard]] FXbool canFilter() const override { return true; }

  void shuffle(GMTrackList*,FXuint) const override;

  void configure(GMColumnList&) override;

  FXbool hasCurrentTrack(GMSource * ) const override;

  FXbool hasTrack(const FXString & mrl,FXint & id) override;

  FXbool findCurrent(GMTrackList * tracklist,GMSource * src) override;

  FXbool findCurrentArtist(GMList * tracklist,GMSource * src) override;

  FXbool findCurrentAlbum(GMAlbumList *,GMSource * src) override;

  [[nodiscard]] FXString getName() const override { return fxtr("Music Library"); }

  [[nodiscard]] FXint getNumTracks() const override;

  FXbool getTrack(GMTrack & info) const override;

  [[nodiscard]] FXint getType() const override { return SOURCE_DATABASE; }

  [[nodiscard]] FXString settingKey() const override { return "database"; }

  FXbool setFilter(const FXString&,FXuint) override;

  FXbool listTags(GMList * taglist,FXIcon * icon) override;

  FXbool listArtists(GMList * artistlist,FXIcon * icon,const FXIntList & taglist) override;

  FXbool listAlbums(GMAlbumList *,const FXIntList &,const FXIntList &) override;

  FXbool listTracks(GMTrackList * tracklist,const FXIntList & albumlist,const FXIntList & taglist) override;

  FXbool updateSelectedTracks(GMTrackList*) override;

  FXbool genre_context_menu(FXMenuPane * pane) override;

  FXbool artist_context_menu(FXMenuPane * pane) override;

  FXbool album_context_menu(FXMenuPane * pane) override;

  FXbool track_context_menu(FXMenuPane * pane) override;

  FXbool source_context_menu(FXMenuPane * pane) override;

  FXbool source_menu(FXMenuPane * pane) override;

  FXbool dnd_accepts(FXDragType*,FXuint) override;

  FXuint dnd_provides(FXDragType types[]) override;

  ~GMDatabaseSource() override = default;
  };


extern void gm_import_files(const FXStringList & files,FXint playlist,FXuint whence);

#endif
