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
#ifndef GMTRACKITEM_H
#define GMTRACKITEM_H

class GMTrackItem;

class GMDBTrackItem : public GMTrackItem {
friend class GMDatabaseSource;
public:
  static FXint max_time;
  static FXint max_queue;
  static FXint max_trackno;
  static FXint max_digits(FXint no);
  static const FXString ratingStrings[];
protected:
  FXString mrl;           /* 4 - 8 */
  FXString title;         /* 4 - 8 */
  FXString album;         /* 4 - 8 */
  FXlong   playdate;      /* 8 - 8 */
  FXint    artist;        /* 4 - 4 */ //FIXME pointer to FXString instead?
  FXint    albumartist;   /* 4 - 4 */ //FIXME pointer to FXString instead?
  FXint    composer;      /* 4 - 4 */ //FIXME pointer to FXString instead?
  FXint    conductor;     /* 4 - 4 */ //FIXME pointer to FXString instead?
  FXint    time;          /* 4 - 4 */
  FXuint   no;            /* 4 - 4 */
  FXint    queue;         /* 4 - 4 */
  FXint    path;          /* 4 - 4 */ //FIXME pointer to FXString instead?
  FXint    bitrate;       /* 4 - 4 */
  FXushort year;          /* 2 - 2 */
  FXushort album_year;    /* 2 - 2 */
  FXushort playcount;     /* 2 - 2 */
  FXuchar  rating;        /* 1 - 1 */
public:
  static FXint browseSort(const GMTrackItem*,const GMTrackItem*);
  static FXint ascendingTitle(const GMTrackItem*,const GMTrackItem*);
  static FXint descendingTitle(const GMTrackItem*,const GMTrackItem*);
  static FXint ascendingTrack(const GMTrackItem*,const GMTrackItem*);
  static FXint descendingTrack(const GMTrackItem*,const GMTrackItem*);
  static FXint ascendingDisc(const GMTrackItem*,const GMTrackItem*);
  static FXint descendingDisc(const GMTrackItem*,const GMTrackItem*);
  static FXint ascendingQueue(const GMTrackItem*,const GMTrackItem*);
  static FXint descendingQueue(const GMTrackItem*,const GMTrackItem*);
  static FXint ascendingTime(const GMTrackItem*,const GMTrackItem*);
  static FXint descendingTime(const GMTrackItem*,const GMTrackItem*);
  static FXint ascendingAlbum(const GMTrackItem*,const GMTrackItem*);
  static FXint descendingAlbum(const GMTrackItem*,const GMTrackItem*);
  static FXint ascendingArtist(const GMTrackItem*,const GMTrackItem*);
  static FXint descendingArtist(const GMTrackItem*,const GMTrackItem*);
  static FXint ascendingAlbumArtist(const GMTrackItem*,const GMTrackItem*);
  static FXint descendingAlbumArtist(const GMTrackItem*,const GMTrackItem*);
  static FXint ascendingYear(const GMTrackItem*,const GMTrackItem*);
  static FXint descendingYear(const GMTrackItem*,const GMTrackItem*);
  static FXint ascendingPlaycount(const GMTrackItem*,const GMTrackItem*);
  static FXint descendingPlaycount(const GMTrackItem*,const GMTrackItem*);
  static FXint ascendingPlaydate(const GMTrackItem*,const GMTrackItem*);
  static FXint descendingPlaydate(const GMTrackItem*,const GMTrackItem*);
  static FXint ascendingBitrate(const GMTrackItem*,const GMTrackItem*);
  static FXint descendingBitrate(const GMTrackItem*,const GMTrackItem*);
  static FXint ascendingFilename(const GMTrackItem*,const GMTrackItem*);
  static FXint descendingFilename(const GMTrackItem*,const GMTrackItem*);
  static FXint ascendingFiletype(const GMTrackItem*,const GMTrackItem*);
  static FXint descendingFiletype(const GMTrackItem*,const GMTrackItem*);
  static FXint ascendingComposer(const GMTrackItem*,const GMTrackItem*);
  static FXint descendingComposer(const GMTrackItem*,const GMTrackItem*);
  static FXint ascendingConductor(const GMTrackItem*,const GMTrackItem*);
  static FXint descendingConductor(const GMTrackItem*,const GMTrackItem*);
  static FXint ascendingRating(const GMTrackItem*,const GMTrackItem*);
  static FXint descendingRating(const GMTrackItem*,const GMTrackItem*);
protected:
  GMDBTrackItem(){}
protected:
  virtual const FXString * getColumnData(FXint i,FXString & t,FXuint &justify,FXint & max) const;
  virtual FXIcon* getIcon() const;
public:
  GMDBTrackItem(FXint id,
                FXint path,
                const FXchar * mrl,
                const FXchar * title,
                FXint artist,
                FXint albumartist,
                FXint composer,
                FXint conductor,
                const FXchar * album,
                FXint time,
                FXuint no,
                FXint queue,
                FXushort track_year,
                FXushort album_year,
                FXushort aplaycount,
                FXint bitrate,
                FXlong aplaydate,
                FXuchar track_rating);

  void setTrackQueue(FXint q) { queue=q; }

  FXint getTrackQueue() const { return queue; }

  /// Return Track Number
  FXint getTrackNumber() const { return no; }

  /// Return Track Title
  FXString getTrackTitle() const { return title; }

  /// Sets rating
  void setRating(FXuchar r) { rating = r; }

  virtual ~GMDBTrackItem();
  };









class GMStreamTrackItem : public GMTrackItem {
public:
  static FXint max_trackno;
  static FXint max_digits(FXint no);
protected:
  FXString title;
  FXString artist;
  FXString genre;
  FXint    bitrate;
  FXint 	 no;
public:
  static FXint ascendingTitle(const GMTrackItem*,const GMTrackItem*);
  static FXint descendingTitle(const GMTrackItem*,const GMTrackItem*);
  static FXint ascendingTrack(const GMTrackItem*,const GMTrackItem*);
  static FXint descendingTrack(const GMTrackItem*,const GMTrackItem*);
  static FXint ascendingGenre(const GMTrackItem*,const GMTrackItem*);
  static FXint descendingGenre(const GMTrackItem*,const GMTrackItem*);
  static FXint ascendingTime(const GMTrackItem*,const GMTrackItem*);
  static FXint descendingTime(const GMTrackItem*,const GMTrackItem*);
protected:
  GMStreamTrackItem(){}
protected:
  virtual const FXString * getColumnData(FXint i,FXString & t,FXuint &justify,FXint & max) const;
public:
  GMStreamTrackItem(FXint id,const FXchar * title,const FXchar * genre,FXint no,FXint bitrate);

  /// Return Track Number
  FXint getTrackNumber() const { return no; }

  /// Return Track Title
  FXString getTrackTitle() const { return title; }

  virtual ~GMStreamTrackItem() {}
  };

class GMLocalTrackItem : public GMTrackItem {
protected:
  FXString filename;
public:
  enum {
    FOLDER = 0x8 // Keep this in sync with GMTrackItem::state
    };
public:
  static FXint ascendingFilename(const GMTrackItem*,const GMTrackItem*);
  static FXint descendingFilename(const GMTrackItem*,const GMTrackItem*);
protected:
  GMLocalTrackItem(){}
protected:
  virtual const FXString * getColumnData(FXint i,FXString & t,FXuint &justify,FXint & max) const;
  virtual FXIcon* getIcon() const;
public:
  GMLocalTrackItem(FXint id,const FXString & filename,FXuchar fl);

  FXString getFilename() const { return filename; }

  virtual ~GMLocalTrackItem() {}
  };


class GMFeedItem : public GMTrackItem {
protected:
  FXString title;
  FXTime   date;
  FXuint   time;
  FXuint   flags;
public:
  enum {
    FOLDER = 0x8 // Keep this in sync with GMTrackItem::state
    };
public:
  static FXint ascendingDate(const GMTrackItem*,const GMTrackItem*);
  static FXint descendingDate(const GMTrackItem*,const GMTrackItem*);
  static FXint ascendingTime(const GMTrackItem*,const GMTrackItem*);
  static FXint descendingTime(const GMTrackItem*,const GMTrackItem*);
protected:
  GMFeedItem(){}
protected:
  virtual const FXString * getColumnData(FXint i,FXString & t,FXuint &justify,FXint & max) const;
//  virtual FXIcon* getIcon() const;
public:
  GMFeedItem(FXint id,const FXString & title,FXTime date,FXuint time,FXuint flags);

  virtual ~GMFeedItem() {}
  };







#endif
