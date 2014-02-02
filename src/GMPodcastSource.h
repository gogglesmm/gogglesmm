/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2007-2014 by Sander Jansen. All Rights Reserved      *
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
  };

class GMSource;
class GMPodcastDownloader;

class GMPodcastSource : public GMSource {
friend class GMPodcastDownloader;
FXDECLARE(GMPodcastSource)
protected:
  GMTrackDatabase     * db;
  GMPodcastDownloader * downloader;
protected:
  GMPodcastSource();
private:
  GMPodcastSource(const GMPodcastSource&);
  GMPodcastSource& operator=(const GMPodcastSource&);
public:
  enum {
    ID_ADD_FEED = GMSource::ID_LAST,
    ID_REFRESH_FEED,
    ID_DOWNLOAD_FEED,
    ID_REMOVE_FEED,
    ID_LAST
    };
public:
  long onCmdAddFeed(FXObject*,FXSelector,void*);
  long onCmdRefreshFeed(FXObject*,FXSelector,void*);
  long onCmdDownloadFeed(FXObject*,FXSelector,void*);
  long onCmdRemoveFeed(FXObject*,FXSelector,void*);
protected:
  void removeFeeds(const FXIntList&);
public:
  GMPodcastSource(GMTrackDatabase * db);

  virtual void configure(GMColumnList&);

  FXbool hasCurrentTrack(GMSource * ) const;

  virtual FXbool getTrack(GMTrack & info) const;

  virtual FXbool setTrack(GMTrack & info) const;

  FXString getName() const { return fxtr("Podcasts"); }

  virtual const FXchar * getAlbumName() const { return fxtr("Feeds"); }

  FXint getType() const { return SOURCE_PODCAST; }

  FXString settingKey() const { return "podcast"; }

  FXint getSortColumn(FXbool) const { return HEADER_TRACK; }

  FXbool canBrowse() const { return true; }

  FXbool hasArtistList() const { return false; }

  FXbool defaultBrowse() const { return true; }
  
  FXbool defaultTags() const { return true; }

  FXbool autoPlay() const { return false; }

  FXbool source_menu(FXMenuPane * pane);

  FXbool source_context_menu(FXMenuPane * pane);

  FXbool album_context_menu(FXMenuPane * pane);

  FXbool track_context_menu(FXMenuPane * pane);

  virtual FXbool listTags(GMList *,FXIcon *);

  virtual FXbool listArtists(GMList *,FXIcon *,const FXIntList &) { return true; }

  virtual FXbool listAlbums(GMAlbumList *,const FXIntList &,const FXIntList &);

  FXbool listTracks(GMTrackList * tracklist,const FXIntList & albumlist,const FXIntList & genrelist);

  virtual ~GMPodcastSource();
  };

#endif
