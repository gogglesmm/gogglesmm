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
#ifndef GMSOURCE_H
#define GMSOURCE_H

#ifndef GMTRACKLIST_H
#include "GMTrackList.h"
#endif

class GMList;
class GMAlbumList;
/*
enum {
  FLAG_CAN_AUTOPLAY	 =0x1,
  FLAG_CAN_FILTER		 =0x2,
  FLAG_CAN_BROWSE		 =0x4,
  FLAG_DEFAULT_BROWSE=0x8
  };
*/

enum {
  FILTER_TRACK    = 0x1,
  FILTER_ARTIST   = 0x2,
  FILTER_ALBUM    = 0x4,
  FILTER_TAG      = 0x8,
  FILTER_ALL      = (FILTER_TRACK|FILTER_ARTIST|FILTER_ALBUM|FILTER_TAG),
  FILTER_DEFAULT  = (FILTER_TRACK|FILTER_ARTIST|FILTER_ALBUM)
  };

enum {
  SOURCE_UNKNOWN  = -1,
  SOURCE_INVALID  = -1,
  SOURCE_PLAYQUEUE,
  SOURCE_DATABASE,
  SOURCE_DATABASE_PLAYLIST,
  SOURCE_FILESYSTEM,
  SOURCE_INTERNET_RADIO,
  SOURCE_PODCAST,
  SOURCE_ALBUM,
  SOURCE_ARTIST,
  SOURCE_AUDIOCD
  };

enum {
  HEADER_DEFAULT=0,
  HEADER_SHUFFLE=1,
  HEADER_BROWSE,
  HEADER_QUEUE,
  HEADER_TRACK,
  HEADER_TITLE,
  HEADER_ALBUM,
  HEADER_ARTIST,
  HEADER_TIME,
  HEADER_TAG, /// WAS HEADER_GENRE
  HEADER_BITRATE,
  HEADER_RATING,
  HEADER_YEAR,
  HEADER_DISC,
  HEADER_ALBUM_ARTIST,
  HEADER_PLAYCOUNT,
  HEADER_PLAYDATE,
  HEADER_FILENAME,
  HEADER_FILETYPE,
  HEADER_COMPOSER,
  HEADER_CONDUCTOR,
  HEADER_DATE,
  HEADER_STATUS,
  HEADER_NONE
  };


/// Sort Function
class GMTrackItem;
typedef FXint (*GMTrackListSortFunc)(const GMTrackItem*,const GMTrackItem*);





class GMSource : public FXObject {
FXDECLARE(GMSource)
/*
protected:
  FXString settingkey;
  FXString name;
  FXuint   sourcetype;
*/
protected:
//	FXIntList genre_selection;
//	FXIntList artist_selection;
//	FXIntList album_selection;
  FXint current_track;
  GMTrackListSortFunc sort_browse;
private:
  GMSource(const GMSource&);
  GMSource& operator=(const GMSource&);
public:
  enum {
    ID_TRACK_PLAYED = 1,
    ID_EDIT_GENRE,
    ID_EDIT_ARTIST,
    ID_EDIT_ALBUM,
    ID_EDIT_TRACK,
    ID_COPY_ARTIST,
    ID_COPY_ALBUM,
    ID_COPY_TRACK,
    ID_DELETE_TAG,
    ID_DELETE_ARTIST,
    ID_DELETE_ALBUM,
    ID_DELETE_TRACK,
    ID_DELETE_TAG_ADV,
    ID_DELETE_ARTIST_ADV,
    ID_DELETE_ALBUM_ADV,
    ID_DELETE_TRACK_ADV,
    ID_PASTE,
    ID_DROP,
    ID_EXPORT,
    ID_TIP_TEXT,
    ID_LAST
    };
public:
  GMSource();

  virtual void configure(GMColumnList&) {}

  virtual void shuffle(GMTrackList*,FXuint) const{}


  GMTrackListSortFunc getSortBrowse() const { return sort_browse; }

  virtual FXint getSortColumn(FXbool browse) const { if (browse) return HEADER_BROWSE; else return HEADER_ARTIST; }

  void setCurrentTrack(FXint t) { current_track=t; }

  FXint getCurrentTrack() const { return current_track; }

  virtual FXbool hasCurrentTrack(GMSource * ) const { return false; }

  virtual FXbool hasTrack(const FXString &,FXint &) { return false; }

  virtual void resetCurrent() { current_track=-1; }

  virtual void markCurrent(const GMTrackItem*);

  virtual FXbool findCurrent(GMTrackList * tracklist,GMSource * src);

  virtual FXbool findCurrentArtist(GMList * tracklist,GMSource * src);

  virtual FXbool findCurrentAlbum(GMAlbumList *,GMSource * src);

  virtual FXint getNumTracks() const;

  virtual FXString getTrackFilename(FXint id) const;

  virtual FXbool getTrack(GMTrack & info) const;

  virtual FXbool setTrack(GMTrack &) const { return false; }

  virtual FXint getType() const { return SOURCE_INVALID; }

  virtual FXbool getQueueColumn(FXbool) const { return false; }

  virtual FXbool canBrowse() const { return true; }

  virtual FXbool canFilter() const { return false; }

  virtual FXbool defaultBrowse() const { return true; }

  virtual FXbool autoPlay() const { return true; }

  virtual FXbool hasArtistList() const { return true; }

  virtual FXString getName() const { return FXString::null; }

  virtual const FXchar * getAlbumName() const { return fxtr("Albums"); }

  /// Items have been dragged around.
  virtual void dragged(GMTrackList*);

  /// Sorting is about to be changed.
  virtual void sorted(GMTrackList*,FXint) {}


  virtual FXString settingKey() const { return "nokey"; }

  virtual void load(FXSettings&) {}

  virtual void save(FXSettings&) const {}

  virtual void save(GMTrackList*) {}




  virtual FXbool setFilter(const FXString&,FXuint) {return false;}

  virtual FXbool listGenres(GMList *,FXIcon *) { return false; }

  virtual FXbool listTags(GMList *,FXIcon *) { return false; }

  virtual FXbool listArtists(GMList *,FXIcon *,const FXIntList &) { return false; }

  virtual FXbool listComposers(GMList *,FXIcon *,const FXIntList &) { return false; }

  virtual FXbool listAlbums(GMAlbumList *,const FXIntList &,const FXIntList &) { return false; }

  virtual FXbool listTracks(GMTrackList*,const FXIntList &,const FXIntList &) { return false; }

  virtual FXbool updateSelectedTracks(GMTrackList*) { return false; }

  virtual FXbool genre_context_menu(FXMenuPane*) { return false; }

  virtual FXbool artist_context_menu(FXMenuPane*) { return false; }

  virtual FXbool album_context_menu(FXMenuPane*) { return false; }

  virtual FXbool track_context_menu(FXMenuPane*) { return false; }

  virtual FXbool source_context_menu(FXMenuPane*) { return false; }


  virtual FXbool dnd_accepts(FXDragType*,FXuint) { return false; }

  virtual FXuint dnd_provides(FXDragType []) {return 0;}


  virtual FXbool track_double_click() { return false; }



  virtual ~GMSource();
  };

typedef FXObjectListOf<GMSource> GMSourceList;



#endif
