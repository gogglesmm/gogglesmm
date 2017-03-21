/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2007-2017 by Sander Jansen. All Rights Reserved      *
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
#ifndef GMPODCASTSOURCE_H
#define GMPODCASTSOURCE_H


/* Feed Item Flags */
enum {
  ITEM_FLAG_QUEUE  = 0,
  ITEM_FLAG_LOCAL  = 1,
  ITEM_FLAG_PLAYED = 2,
  ITEM_FLAG_DOWNLOAD_FAILED = 3,

  ITEM_QUEUE  = (1<<ITEM_FLAG_QUEUE),
  ITEM_LOCAL  = (1<<ITEM_FLAG_LOCAL),
  ITEM_PLAYED = (1<<ITEM_FLAG_PLAYED),
  ITEM_FAILED = (1<<ITEM_FLAG_DOWNLOAD_FAILED),
  };

class GMSource;
class GMPodcastDownloader;

class GMPodcastSource : public GMSource {
friend class GMPodcastDownloader;
FXDECLARE(GMPodcastSource)
protected:
  GMTrackDatabase     * db         = nullptr;
  GMCoverCache        * covercache = nullptr;
  GMPodcastDownloader * downloader = nullptr;
  FXint                 navailable = 0;
protected:
  GMPodcastSource(){}
private:
  GMPodcastSource(const GMPodcastSource&);
  GMPodcastSource& operator=(const GMPodcastSource&);
protected:
  void scheduleUpdate();
  void updateAvailable();
  void setItemFlags(FXuint add,FXuint remove,FXuint condition);
public:
  enum {
    ID_ADD_FEED = GMSource::ID_LAST,
    ID_REFRESH_FEED,
    ID_DOWNLOAD_FEED,
    ID_REMOVE_FEED,
    ID_MARK_PLAYED,
    ID_MARK_NEW,
    ID_FEED_UPDATER,
    ID_LOAD_COVERS,
    ID_DELETE_LOCAL,
    ID_AUTO_DOWNLOAD,
    ID_LAST
    };
public:
  long onCmdAutoDownload(FXObject*,FXSelector,void*);
  long onCmdAddFeed(FXObject*,FXSelector,void*);
  long onCmdRefreshFeed(FXObject*,FXSelector,void*);
  long onCmdDownloadFeed(FXObject*,FXSelector,void*);
  long onCmdRemoveFeed(FXObject*,FXSelector,void*);
  long onCmdMarkPlayed(FXObject*,FXSelector,void*);
  long onCmdMarkNew(FXObject*,FXSelector,void*);
  long onCmdFeedUpdated(FXObject*,FXSelector,void*);
  long onCmdTrackPlayed(FXObject*,FXSelector,void*);
  long onCmdLoadCovers(FXObject*,FXSelector,void*);
  long onCmdDeleteLocal(FXObject*,FXSelector,void*);
  long onCmdCopyTrack(FXObject*,FXSelector,void*);
  long onCmdRequestTrack(FXObject*,FXSelector,void*);
protected:
  void removeFeeds(const FXIntList&);
public:
  GMPodcastSource(GMTrackDatabase * db);

  void getLocalFiles(const FXIntList & ids,FXStringList&);

  void refreshFeeds();

  void loadCovers() override;

  void updateCovers() override;

  void setLastUpdate();

  void setUpdateInterval(FXlong);

  FXlong getUpdateInterval() const;

  void configure(GMColumnList&) override;

  FXbool hasCurrentTrack(GMSource * ) const override;

  FXbool getTrack(GMTrack & info) const override;

  FXbool setTrack(GMTrack & info) const override;

  FXString getName() const override;

  const FXchar * getAlbumName() const override { return fxtr("Feeds"); }

  FXIcon* getAlbumIcon() const override;

  GMCoverCache * getCoverCache() const override { return covercache; }

  FXint getType() const override { return SOURCE_PODCAST; }

  FXString settingKey() const override { return "podcast"; }

  FXint getSortColumn(FXbool) const override { return HEADER_TRACK; }

  FXbool canBrowse() const override { return true; }

  FXbool hasArtistList() const override { return false; }

  FXbool defaultBrowse() const override { return true; }

  FXbool defaultTags() const override { return true; }

  FXbool autoPlay() const override { return false; }

  FXbool source_menu(FXMenuPane * pane) override;

  FXbool source_context_menu(FXMenuPane * pane) override;

  FXbool album_context_menu(FXMenuPane * pane) override;

  FXbool track_context_menu(FXMenuPane * pane) override;

  FXbool listTags(GMList *,FXIcon *) override;

  FXbool listArtists(GMList *,FXIcon *,const FXIntList &) override { return true; }

  FXbool listAlbums(GMAlbumList *,const FXIntList &,const FXIntList &) override;

  FXbool listTracks(GMTrackList * tracklist,const FXIntList & albumlist,const FXIntList & genrelist) override;

  FXuint dnd_provides(FXDragType types[]) override;

  virtual ~GMPodcastSource();
  };

#endif
