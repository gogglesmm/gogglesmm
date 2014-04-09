/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2006-2014 by Sander Jansen. All Rights Reserved      *
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
#include <limits.h>
#include "gmdefs.h"
#include <FXPNGIcon.h>
#include "GMTrack.h"
#include "GMTrackList.h"
#include "GMTrackItem.h"
#include "GMSource.h"
#include "GMTrackView.h"
#include "GMPlayerManager.h"
#include "GMTrackDatabase.h"
#include "GMDatabaseSource.h"
#include "GMPlayListSource.h"
#include "GMPodcastSource.h"
#include "GMPlayQueue.h"
#include "GMTrackDatabase.h"
#include "GMIconTheme.h"

//#include "icons.h"

#define SIDE_SPACING             4    // Left or right spacing between items
#define DETAIL_TEXT_SPACING      2    // Spacing between text and icon in detail icon mode
#define MINI_TEXT_SPACING        2    // Spacing between text and icon in mini icon mode
#define BIG_LINE_SPACING         6    // Line spacing in big icon mode
#define BIG_TEXT_SPACING         2    // Spacing between text and icon in big icon mode
#define ITEM_SPACE             128    // Default space for item


#define ICON_WIDTH 10
#define ICON_HEIGHT 15

FXint GMDBTrackItem::max_time=0;
FXint GMDBTrackItem::max_trackno=0;
FXint GMDBTrackItem::max_queue=0;

FXint GMDBTrackItem::max_digits(FXint num){
  if (num>9) {
    FXint n=0;
    while(num>0) { ++n; num/=10; }
    return n;
    }
  return 1;
  }

GMDBTrackItem::GMDBTrackItem(FXint track_id,FXint track_path,const FXchar * track_mrl,const FXchar * track_title,FXint track_artist,FXint track_album_artist,FXint track_composer,FXint track_conductor,const FXchar * track_album,FXint track_time,FXuint track_no,FXint track_queue,FXushort track_year,FXushort track_album_year,FXushort track_playcount,FXint track_bitrate,FXlong track_playdate,FXuchar track_rating) :
  GMTrackItem(track_id), mrl(track_mrl),
                 title(track_title),
                 album(track_album),
                 playdate(track_playdate),
                 artist(track_artist),
                 albumartist(track_album_artist),
                 composer(track_composer),conductor(track_conductor),
                 time(track_time),
                 no(track_no),
                 queue(track_queue),
                 path(track_path),
                 bitrate(track_bitrate),
                 year(track_year),
                 album_year(track_album_year),
                 playcount(track_playcount),
                 rating(track_rating) {

  state|=GMTrackItem::DRAGGABLE;
  }

GMDBTrackItem::~GMDBTrackItem(){
  }


FXIcon * GMDBTrackItem::getIcon() const {
#ifdef HAVE_PLAYQUEUE
  if (GMPlayerManager::instance()->getPlayQueue() && GMPlayerManager::instance()->getPlayQueue()->hasTrack(id))
    return GMIconTheme::instance()->icon_playqueue;
  else
#endif
    return NULL;
  }

const FXString * GMDBTrackItem::getColumnData(FXint type,FXString &text,FXuint & justify,FXint & max) const{
  const FXString * textptr;
  justify=COLUMN_JUSTIFY_NORMAL;
  switch(type){
    case HEADER_QUEUE         : text.format("%d",queue);
                                textptr=&text;
                                justify=COLUMN_JUSTIFY_LEFT_RIGHT_ALIGNED;
                                max=GMDBTrackItem::max_queue;
                                break;

    case HEADER_TRACK         : if (GMTRACKNO((FXuint)(FXuval)no)>0) {
                                  text.format("%d",GMTRACKNO((FXuint)(FXuval)no));
                                  textptr=&text;
                                  justify=COLUMN_JUSTIFY_LEFT_RIGHT_ALIGNED;
                                  max=GMDBTrackItem::max_trackno;
                                  }
                                else {
                                  textptr=NULL;
                                  }
                                break;

    case HEADER_DISC          : if (GMDISCNO((FXuint)(FXuval)no)>0) {
                                  text.format("%d",GMDISCNO((FXuint)(FXuval)no));
                                  textptr=&text;
                                  }
                                else {
                                  textptr=NULL;
                                  }
                                break;

    case HEADER_TITLE         : textptr = &title;  			break;
    case HEADER_FILENAME      : text.format("%s/%s",GMPlayerManager::instance()->getTrackDatabase()->getTrackPath(path),mrl.text());
                                textptr=&text;
                                break;
    case HEADER_FILETYPE      : text=FXPath::extension(mrl).lower();
                                textptr=&text; break;
    case HEADER_ALBUM         : textptr = &album;  			break;
    case HEADER_ARTIST        : textptr = GMPlayerManager::instance()->getTrackDatabase()->getArtist(artist);       break;
    case HEADER_ALBUM_ARTIST  : textptr = GMPlayerManager::instance()->getTrackDatabase()->getArtist(albumartist);  break;
    case HEADER_COMPOSER      : textptr = GMPlayerManager::instance()->getTrackDatabase()->getArtist(composer);     break;
    case HEADER_CONDUCTOR     : textptr = GMPlayerManager::instance()->getTrackDatabase()->getArtist(conductor);    break;
//    case HEADER_GENRE         : textptr = &genre;    	  break;
    case HEADER_YEAR          : justify=COLUMN_JUSTIFY_RIGHT; //justify=COLUMN_JUSTIFY_CENTER_RIGHT_ALIGNED;
                                //max=9999;
                                if (year>0) {text.format("%d",year); textptr=&text; } else textptr=NULL; break;

    case HEADER_PLAYCOUNT     : justify=COLUMN_JUSTIFY_CENTER_RIGHT_ALIGNED;
                                //max=9999;
                                if (playcount>0) {text.format("%d",playcount); textptr=&text; } else textptr=NULL; break;

    case HEADER_PLAYDATE      : if (playdate>0) {
                                text=FXSystem::localTime(playdate);
                                textptr=&text;
                                }
                                else {
                                textptr=NULL;
                                }
                                break;

    case HEADER_TIME          : /*textptr = &timestring;*/
                                text.format("%d:%.2d",time/60,time%60);
                                textptr=&text;
                                justify=COLUMN_JUSTIFY_CENTER_RIGHT_ALIGNED;
                                max=GMDBTrackItem::max_time;
                                break;
    case HEADER_BITRATE       : text.format("%d kbps",bitrate/*(bitrate / 1024.0)*/);
                                textptr=&text;
                                justify=COLUMN_JUSTIFY_RIGHT;
                                break;
    case HEADER_RATING        : //textptr = ratingStrings + ((rating <= sizeof(GMDBTrackItem::ratingStrings)/sizeof(FXString))?rating:0);
                                textptr=NULL;
                                max=rating;
                                break;
    default							      : textptr=NULL;			 			break;
    }
  return textptr;
  }

#define GET_ARTIST_STRING(x)  GMPlayerManager::instance()->getTrackDatabase()->getArtist(x)


static inline FXbool begins_with_keyword(const FXString & t){
  for (FXint i=0;i<GMPlayerManager::instance()->getPreferences().gui_sort_keywords.no();i++){
    if (comparecase(t,GMPlayerManager::instance()->getPreferences().gui_sort_keywords[i],GMPlayerManager::instance()->getPreferences().gui_sort_keywords[i].length())==0) return TRUE;
    }
  return FALSE;
  }

static inline FXbool begins_with_keyword_ptr(const FXString * t){
  for (FXint i=0;i<GMPlayerManager::instance()->getPreferences().gui_sort_keywords.no();i++){
    if (comparecase(*t,GMPlayerManager::instance()->getPreferences().gui_sort_keywords[i],GMPlayerManager::instance()->getPreferences().gui_sort_keywords[i].length())==0) return TRUE;
    }
  return FALSE;
  }

static inline FXint keywordcompare(const FXString *a,const FXString *b) {
  register FXint pa,pb;

  if (a==b) return 0;

//  fxmessage("cmp: %s - %s\n",a->text(),b->text());

  if (begins_with_keyword_ptr(a))
    pa=FXMIN(a->length()-1,a->find(' ')+1);
  else
    pa=0;

  if (begins_with_keyword_ptr(b))
    pb=FXMIN(b->length()-1,b->find(' ')+1);
  else
    pb=0;
  return comparecase(&((*a)[pa]),&((*b)[pb]));
  }

static inline FXint keywordcompare(const FXString & a,const FXString & b) {
  register FXint pa,pb;
  if (begins_with_keyword(a))
    pa=FXMIN(a.length()-1,a.find(' ')+1);
  else
    pa=0;

  if (begins_with_keyword(b))
    pb=FXMIN(b.length()-1,b.find(' ')+1);
  else
    pb=0;
  return comparecase(&((a)[pa]),&((b)[pb]));
  }




FXint GMDBTrackItem::browseSort(const GMTrackItem * pa,const GMTrackItem * pb){
  const GMDBTrackItem * const ta = (GMDBTrackItem*)pa;
  const GMDBTrackItem * const tb = (GMDBTrackItem*)pb;
  register FXint x;

  if (GMTrackView::album_by_year) {
    if (ta->album_year > tb->album_year)
      return (GMTrackView::reverse_album) ? -1 : 1;
    else if (ta->album_year < tb->album_year)
      return (GMTrackView::reverse_album) ? 1 : -1;
    }

  x = keywordcompare(ta->album,tb->album);
  if (x!=0) return (GMTrackView::reverse_album) ? -x : x;

  x = keywordcompare(GET_ARTIST_STRING(ta->albumartist),GET_ARTIST_STRING(tb->albumartist));
  if (x!=0) return (GMTrackView::reverse_artist) ? -x : x;

  /// Track & Disc
  if (ta->no>tb->no) return 1;
  else if (ta->no<tb->no) return -1;
  return 0;
  }



FXint GMDBTrackItem::ascendingFilename(const GMTrackItem* pa,const GMTrackItem* pb){
  const GMDBTrackItem * const ta = (GMDBTrackItem*)pa;
  const GMDBTrackItem * const tb = (GMDBTrackItem*)pb;
  if (ta->path!=tb->path) {
    const GMTrackDatabase* const db = GMPlayerManager::instance()->getTrackDatabase();
    FXint x = comparecase(db->getTrackPath(ta->path),db->getTrackPath(tb->path));
    if (x!=0) return x;
    }
  return comparecase(ta->mrl,tb->mrl);
  }

FXint GMDBTrackItem::descendingFilename(const GMTrackItem* pa,const GMTrackItem* pb){
  const GMDBTrackItem * const ta = (GMDBTrackItem*)pa;
  const GMDBTrackItem * const tb = (GMDBTrackItem*)pb;
  if (ta->path!=tb->path) {
    const GMTrackDatabase* const db = GMPlayerManager::instance()->getTrackDatabase();
    FXint x = comparecase(db->getTrackPath(ta->path),db->getTrackPath(tb->path));
    if (x!=0) return -x;
    }
  return -comparecase(ta->mrl,tb->mrl);
  }


FXint GMDBTrackItem::ascendingFiletype(const GMTrackItem* pa,const GMTrackItem* pb){
  const GMDBTrackItem * const ta = (GMDBTrackItem*)pa;
  const GMDBTrackItem * const tb = (GMDBTrackItem*)pb;
  return comparecase(FXPath::extension(ta->mrl),FXPath::extension(tb->mrl));
  }

FXint GMDBTrackItem::descendingFiletype(const GMTrackItem* pa,const GMTrackItem* pb){
  const GMDBTrackItem * const ta = (GMDBTrackItem*)pa;
  const GMDBTrackItem * const tb = (GMDBTrackItem*)pb;
  return -comparecase(FXPath::extension(ta->mrl),FXPath::extension(tb->mrl));
  }



FXint GMDBTrackItem::ascendingTitle(const GMTrackItem* pa,const GMTrackItem* pb){
  const GMDBTrackItem * const ta = (GMDBTrackItem*)pa;
  const GMDBTrackItem * const tb = (GMDBTrackItem*)pb;
  return keywordcompare(ta->title,tb->title);
  }

FXint GMDBTrackItem::descendingTitle(const GMTrackItem* pa,const GMTrackItem* pb){
  const GMDBTrackItem * const ta = (GMDBTrackItem*)pa;
  const GMDBTrackItem * const tb = (GMDBTrackItem*)pb;
  return keywordcompare(tb->title,ta->title);
  }

FXint GMDBTrackItem::ascendingTrack(const GMTrackItem* pa,const GMTrackItem* pb){
  register const FXuint a=GMTRACKNO((FXuint)(FXuval)((GMDBTrackItem*)pa)->no);
  register const FXuint b=GMTRACKNO((FXuint)(FXuval)((GMDBTrackItem*)pb)->no);
  if (a>b) return 1;
  else if (a<b) return -1;
  return 0;
  }

FXint GMDBTrackItem::descendingTrack(const GMTrackItem* pa,const GMTrackItem* pb){
  register const FXint a=GMTRACKNO((FXuint)(FXuval)((GMDBTrackItem*)pa)->no);
  register const FXint b=GMTRACKNO((FXuint)(FXuval)((GMDBTrackItem*)pb)->no);
  if (a>b) return -1;
  else if (a<b) return 1;
  return 0;
  }

FXint GMDBTrackItem::ascendingDisc(const GMTrackItem* pa,const GMTrackItem* pb){
  register const FXuint a=GMDISCNO((FXuint)(FXuval)((GMDBTrackItem*)pa)->no);
  register const FXuint b=GMDISCNO((FXuint)(FXuval)((GMDBTrackItem*)pb)->no);
  if (a>b) return 1;
  else if (a<b) return -1;
  return 0;
  }

FXint GMDBTrackItem::descendingDisc(const GMTrackItem* pa,const GMTrackItem* pb){
  register const FXint a=GMDISCNO((FXuint)(FXuval)((GMDBTrackItem*)pa)->no);
  register const FXint b=GMDISCNO((FXuint)(FXuval)((GMDBTrackItem*)pb)->no);
  if (a>b) return -1;
  else if (a<b) return 1;
  return 0;
  }


FXint GMDBTrackItem::ascendingQueue(const GMTrackItem* pa,const GMTrackItem* pb){
  register const FXint a=(FXint)(FXival)((GMDBTrackItem*)pa)->queue;
  register const FXint b=(FXint)(FXival)((GMDBTrackItem*)pb)->queue;
  if (a>b) return 1;
  else if (a<b) return -1;
  return 0;
  }

FXint GMDBTrackItem::descendingQueue(const GMTrackItem* pa,const GMTrackItem* pb){
  register const FXint a=(FXint)(FXival)((GMDBTrackItem*)pa)->queue;
  register const FXint b=(FXint)(FXival)((GMDBTrackItem*)pb)->queue;
  if (a>b) return -1;
  else if (a<b) return 1;
  return 0;
  }

FXint GMDBTrackItem::ascendingPlaycount(const GMTrackItem* pa,const GMTrackItem* pb){
  register const FXushort a=(FXushort)(FXival)((GMDBTrackItem*)pa)->playcount;
  register const FXushort b=(FXushort)(FXival)((GMDBTrackItem*)pb)->playcount;
  if (a>b) return 1;
  else if (a<b) return -1;
  return 0;
  }

FXint GMDBTrackItem::descendingPlaycount(const GMTrackItem* pa,const GMTrackItem* pb){
  register const FXushort a=(FXushort)(FXival)((GMDBTrackItem*)pa)->playcount;
  register const FXushort b=(FXushort)(FXival)((GMDBTrackItem*)pb)->playcount;
  if (a>b) return -1;
  else if (a<b) return 1;
  return 0;
  }

FXint GMDBTrackItem::ascendingPlaydate(const GMTrackItem* pa,const GMTrackItem* pb){
  register const FXlong a=(FXlong)((GMDBTrackItem*)pa)->playcount;
  register const FXlong b=(FXlong)((GMDBTrackItem*)pb)->playcount;
  if (a>b) return 1;
  else if (a<b) return -1;
  return 0;
  }

FXint GMDBTrackItem::descendingPlaydate(const GMTrackItem* pa,const GMTrackItem* pb){
  register const FXlong a=(FXlong)((GMDBTrackItem*)pa)->playdate;
  register const FXlong b=(FXlong)((GMDBTrackItem*)pb)->playdate;
  if (a>b) return -1;
  else if (a<b) return 1;
  return 0;
  }


FXint GMDBTrackItem::ascendingYear(const GMTrackItem* pa,const GMTrackItem* pb){
  register const FXushort a=(FXushort)(FXival)((GMDBTrackItem*)pa)->year;
  register const FXushort b=(FXushort)(FXival)((GMDBTrackItem*)pb)->year;
  if (a>b) return 1;
  if (a<b) return -1;
  return 0;
  }

FXint GMDBTrackItem::descendingYear(const GMTrackItem* pa,const GMTrackItem* pb){
  register const FXushort a=(FXushort)(FXival)((GMDBTrackItem*)pa)->year;
  register const FXushort b=(FXushort)(FXival)((GMDBTrackItem*)pb)->year;
  if (a>b) return -1;
  if (a<b) return 1;
  return 0;
  }

FXint GMDBTrackItem::ascendingTime(const GMTrackItem* pa,const GMTrackItem* pb){
  register const FXint a=((GMDBTrackItem*)pa)->time;
  register const FXint b=((GMDBTrackItem*)pb)->time;
  if (a>b) return 1;
  if (a<b) return -1;
  return 0;
  }

FXint GMDBTrackItem::descendingTime(const GMTrackItem* pa,const GMTrackItem* pb){
  register const FXint a=((GMDBTrackItem*)pa)->time;
  register const FXint b=((GMDBTrackItem*)pb)->time;
  if (a>b) return -1;
  if (a<b) return 1;
  return 0;
  }

FXint GMDBTrackItem::ascendingAlbum(const GMTrackItem* pa,const GMTrackItem* pb){
  const GMDBTrackItem * const ta = (GMDBTrackItem*)pa;
  const GMDBTrackItem * const tb = (GMDBTrackItem*)pb;

  register FXint x = keywordcompare(ta->album,tb->album);
  if (x!=0) return x;

  /// Track & Disc
  if (ta->no>tb->no) return 1;
  else if (ta->no<tb->no) return -1;
  return 0;
  }

FXint GMDBTrackItem::descendingAlbum(const GMTrackItem* pa,const GMTrackItem* pb){
  const GMDBTrackItem * const ta = (GMDBTrackItem*)pa;
  const GMDBTrackItem * const tb = (GMDBTrackItem*)pb;

  register FXint x = keywordcompare(tb->album,ta->album);
  if (x!=0) return x;

  /// Track & Disc
  if (ta->no>tb->no) return 1;
  else if (ta->no<tb->no) return -1;
  return 0;
  }


FXint GMDBTrackItem::ascendingArtist(const GMTrackItem* pa,const GMTrackItem* pb){
  const GMDBTrackItem * const ta = (GMDBTrackItem*)pa;
  const GMDBTrackItem * const tb = (GMDBTrackItem*)pb;
  register FXint x;

  x = keywordcompare(GET_ARTIST_STRING(ta->artist),GET_ARTIST_STRING(tb->artist));
  if (x!=0) return x;

  x = keywordcompare(ta->album,tb->album);
  if (x!=0) return x;

  /// Track & Disc
  if (ta->no>tb->no) return 1;
  else if (ta->no<tb->no) return -1;
  return 0;
  }


FXint GMDBTrackItem::descendingArtist(const GMTrackItem* pa,const GMTrackItem* pb){
  const GMDBTrackItem * const ta = (GMDBTrackItem*)pa;
  const GMDBTrackItem * const tb = (GMDBTrackItem*)pb;
  register FXint x;

  x = keywordcompare(GET_ARTIST_STRING(tb->artist),GET_ARTIST_STRING(ta->artist));
  if (x!=0) return x;

  x = keywordcompare(ta->album,tb->album);
  if (x!=0) return x;

  /// Track & Disc
  if (ta->no>tb->no) return 1;
  else if (ta->no<tb->no) return -1;
  return 0;
  }

FXint GMDBTrackItem::ascendingAlbumArtist(const GMTrackItem* pa,const GMTrackItem* pb){
  const GMDBTrackItem * const ta = (GMDBTrackItem*)pa;
  const GMDBTrackItem * const tb = (GMDBTrackItem*)pb;

  register FXint x;

  x = keywordcompare(GET_ARTIST_STRING(ta->albumartist),GET_ARTIST_STRING(tb->albumartist));
  if (x!=0) return x;

  x = keywordcompare(ta->album,tb->album);
  if (x!=0) return x;

  /// Track & Disc
  if (ta->no>tb->no) return 1;
  else if (ta->no<tb->no) return -1;
  return 0;
  }


FXint GMDBTrackItem::descendingAlbumArtist(const GMTrackItem* pa,const GMTrackItem* pb){
  const GMDBTrackItem * const ta = (GMDBTrackItem*)pa;
  const GMDBTrackItem * const tb = (GMDBTrackItem*)pb;

  register FXint x;

  x = keywordcompare(GET_ARTIST_STRING(tb->albumartist),GET_ARTIST_STRING(ta->albumartist));
  if (x!=0) return x;

  x = keywordcompare(ta->album,tb->album);
  if (x!=0) return x;

  /// Track & Disc
  if (ta->no>tb->no) return 1;
  else if (ta->no<tb->no) return -1;
  return 0;
  }

#if 0
FXint GMDBTrackItem::ascendingGenre(const GMTrackItem* pa,const GMTrackItem* pb){
  const GMDBTrackItem * const ta = (GMDBTrackItem*)pa;
  const GMDBTrackItem * const tb = (GMDBTrackItem*)pb;
/*
  register FXint a=0,b=0,x;

  x = comparecase(ta->genre,tb->genre);
  if (x!=0) return x;

  if (begins_with_keyword(ta->artist)) a=FXMIN(ta->artist.length()-1,ta->artist.find(' ')+1);
  if (begins_with_keyword(tb->artist)) b=FXMIN(tb->artist.length()-1,tb->artist.find(' ')+1);
  x = comparecase(&ta->artist[a],&tb->artist[b]);
  if (x!=0) return x;

  a=b=0;
  if (begins_with_keyword(ta->album)) a=FXMIN(ta->album.length()-1,ta->album.find(' ')+1);
  if (begins_with_keyword(tb->album)) b=FXMIN(tb->album.length()-1,tb->album.find(' ')+1);
  x = comparecase(&ta->album[a],&tb->album[b]);

  if (x!=0) return x;
*/
  /// Track & Disc
  if (ta->no>tb->no) return 1;
  else if (ta->no<tb->no) return -1;
  return 0;
  }

FXint GMDBTrackItem::descendingGenre(const GMTrackItem* pa,const GMTrackItem* pb){
  const GMDBTrackItem * const ta = (GMDBTrackItem*)pa;
  const GMDBTrackItem * const tb = (GMDBTrackItem*)pb;
/*
  register FXint a=0,b=0,x;

  x = comparecase(tb->genre,ta->genre);
  if (x!=0) return x;

  if (begins_with_keyword(ta->artist)) a=FXMIN(ta->artist.length()-1,ta->artist.find(' ')+1);
  if (begins_with_keyword(tb->artist)) b=FXMIN(tb->artist.length()-1,tb->artist.find(' ')+1);
  x = comparecase(&ta->artist[a],&tb->artist[b]);
  if (x!=0) return x;

  a=b=0;
  if (begins_with_keyword(ta->album)) a=FXMIN(ta->album.length()-1,ta->album.find(' ')+1);
  if (begins_with_keyword(tb->album)) b=FXMIN(tb->album.length()-1,tb->album.find(' ')+1);
  x = comparecase(&ta->album[a],&tb->album[b]);

  if (x!=0) return x;
*/

  /// Track & Disc
  if (ta->no>tb->no) return 1;
  else if (ta->no<tb->no) return -1;
  return 0;
  }

#endif

FXint GMDBTrackItem::ascendingBitrate(const GMTrackItem* pa,const GMTrackItem* pb){
  register const FXint a=((GMDBTrackItem*)pa)->bitrate;
  register const FXint b=((GMDBTrackItem*)pb)->bitrate;
  if (a>b) return 1;
  else if (a<b) return -1;
  return 0;
  }

FXint GMDBTrackItem::descendingBitrate(const GMTrackItem* pa,const GMTrackItem* pb){
  register const FXint a=((GMDBTrackItem*)pa)->bitrate;
  register const FXint b=((GMDBTrackItem*)pb)->bitrate;
  if (a>b) return -1;
  else if (a<b) return 1;
  return 0;
  }





FXint GMStreamTrackItem::max_trackno=0;

FXint GMStreamTrackItem::max_digits(FXint num){
  if (num>9) {
    FXint n=0;
    while(num>0) { ++n; num/=10; }
    return n;
    }
  return 1;
  }


GMStreamTrackItem::GMStreamTrackItem(FXint track_id,const FXchar * track_title,const FXchar * track_genre,FXint track_no,FXint track_bitrate) :
  GMTrackItem(), title(track_title),genre(track_genre),bitrate(track_bitrate),no(track_no) {
  id=track_id;
  }


const FXString * GMStreamTrackItem::getColumnData(FXint type,FXString &text,FXuint & justify,FXint & max) const{
  const FXString * textptr;
  justify=COLUMN_JUSTIFY_NORMAL;
  switch(type){
    case HEADER_TRACK   : text.format("%d",GMTRACKNO((FXuint)(FXuval)no));
                          textptr=&text;
                          justify=COLUMN_JUSTIFY_LEFT_RIGHT_ALIGNED;
                          max=GMStreamTrackItem::max_trackno;
                          break;
    case HEADER_TITLE   : textptr = &title;  			break;
    case HEADER_TAG     : textptr = &genre;    	  break;
    case HEADER_BITRATE : text.format("%d kbps",bitrate / 1024);
                          textptr=&text;
                          break;
    default							: textptr=NULL;			 			break;
    }
  return textptr;
  }

FXint GMStreamTrackItem::ascendingTime(const GMTrackItem* pa,const GMTrackItem* pb){
  register const FXint a=(FXint)(FXival)((GMStreamTrackItem*)pa)->bitrate;
  register const FXint b=(FXint)(FXival)((GMStreamTrackItem*)pb)->bitrate;
  if (a>b) return 1;
  if (a<b) return -1;
  return 0;
  }

FXint GMStreamTrackItem::descendingTime(const GMTrackItem* pa,const GMTrackItem* pb){
  register const FXint a=(FXint)(FXival)((GMStreamTrackItem*)pa)->bitrate;
  register const FXint b=(FXint)(FXival)((GMStreamTrackItem*)pb)->bitrate;
  if (a>b) return -1;
  if (a<b) return 1;
  return 0;
  }


FXint GMStreamTrackItem::ascendingGenre(const GMTrackItem* pa,const GMTrackItem* pb){
  const GMStreamTrackItem * const ta = (GMStreamTrackItem*)pa;
  const GMStreamTrackItem * const tb = (GMStreamTrackItem*)pb;
  register FXint x;
  x = comparecase(ta->genre,tb->genre);
  if (x!=0) return x;
  return ascendingTrack(pa,pb);
  }

FXint GMStreamTrackItem::descendingGenre(const GMTrackItem* pa,const GMTrackItem* pb){
  const GMStreamTrackItem * const ta = (GMStreamTrackItem*)pa;
  const GMStreamTrackItem * const tb = (GMStreamTrackItem*)pb;
  register FXint x;
  x = comparecase(tb->genre,ta->genre);
  if (x!=0) return x;
  return ascendingTrack(pa,pb);
  }


FXint GMStreamTrackItem::ascendingTrack(const GMTrackItem* pa,const GMTrackItem* pb){
  if (((GMStreamTrackItem*)pa)->no>((GMStreamTrackItem*)pb)->no) return 1;
  else if (((GMStreamTrackItem*)pa)->no<((GMStreamTrackItem*)pb)->no) return -1;
  return 0;
  }

FXint GMStreamTrackItem::descendingTrack(const GMTrackItem* pa,const GMTrackItem* pb){
  if (((GMStreamTrackItem*)pa)->no>((GMStreamTrackItem*)pb)->no) return -1;
  else if (((GMStreamTrackItem*)pa)->no<((GMStreamTrackItem*)pb)->no) return 1;
  return 0;
  }

FXint GMStreamTrackItem::ascendingTitle(const GMTrackItem* pa,const GMTrackItem* pb){
  const GMStreamTrackItem * const ta = (GMStreamTrackItem*)pa;
  const GMStreamTrackItem * const tb = (GMStreamTrackItem*)pb;
  register FXint a=0,b=0;
  if (begins_with_keyword(ta->title)) a=FXMIN(ta->title.length()-1,ta->title.find(' ')+1);
  if (begins_with_keyword(tb->title)) b=FXMIN(tb->title.length()-1,tb->title.find(' ')+1);
  return comparecase(&ta->title[a],&tb->title[b]);
  }

FXint GMStreamTrackItem::descendingTitle(const GMTrackItem* pa,const GMTrackItem* pb){
  const GMStreamTrackItem * const ta = (GMStreamTrackItem*)pa;
  const GMStreamTrackItem * const tb = (GMStreamTrackItem*)pb;
  register FXint a=0,b=0;
  if (begins_with_keyword(ta->title)) a=FXMIN(ta->title.length()-1,ta->title.find(' ')+1);
  if (begins_with_keyword(tb->title)) b=FXMIN(tb->title.length()-1,tb->title.find(' ')+1);
  return -comparecase(&ta->title[a],&tb->title[b]);
  }


FXint GMDBTrackItem::ascendingComposer(const GMTrackItem* pa,const GMTrackItem* pb){
  const GMDBTrackItem * const ta = (GMDBTrackItem*)pa;
  const GMDBTrackItem * const tb = (GMDBTrackItem*)pb;

  register FXint a=0,b=0,x;

  x = keywordcompare(GET_ARTIST_STRING(ta->composer),GET_ARTIST_STRING(tb->composer));
  if (x!=0) return x;

  a=b=0;
  if (begins_with_keyword(ta->album)) a=FXMIN(ta->album.length()-1,ta->album.find(' ')+1);
  if (begins_with_keyword(tb->album)) b=FXMIN(tb->album.length()-1,tb->album.find(' ')+1);
  x = comparecase(&ta->album[a],&tb->album[b]);

  if (x!=0) return x;

  /// Track & Disc
  if (ta->no>tb->no) return 1;
  else if (ta->no<tb->no) return -1;
  return 0;
  }


FXint GMDBTrackItem::descendingComposer(const GMTrackItem* pa,const GMTrackItem* pb){
  const GMDBTrackItem * const ta = (GMDBTrackItem*)pa;
  const GMDBTrackItem * const tb = (GMDBTrackItem*)pb;

  register FXint a=0,b=0,x;

  x = keywordcompare(GET_ARTIST_STRING(tb->composer),GET_ARTIST_STRING(ta->composer));
  if (x!=0) return x;

  a=b=0;
  if (begins_with_keyword(ta->album)) a=FXMIN(ta->album.length()-1,ta->album.find(' ')+1);
  if (begins_with_keyword(tb->album)) b=FXMIN(tb->album.length()-1,tb->album.find(' ')+1);
  x = comparecase(&ta->album[a],&tb->album[b]);

  if (x!=0) return x;

  /// Track & Disc
  if (ta->no>tb->no) return 1;
  else if (ta->no<tb->no) return -1;
  return 0;
  }



FXint GMDBTrackItem::ascendingConductor(const GMTrackItem* pa,const GMTrackItem* pb){
  const GMDBTrackItem * const ta = (GMDBTrackItem*)pa;
  const GMDBTrackItem * const tb = (GMDBTrackItem*)pb;

  register FXint a=0,b=0,x;

  x = keywordcompare(GET_ARTIST_STRING(ta->composer),GET_ARTIST_STRING(tb->composer));
  if (x!=0) return x;

  a=b=0;
  if (begins_with_keyword(ta->album)) a=FXMIN(ta->album.length()-1,ta->album.find(' ')+1);
  if (begins_with_keyword(tb->album)) b=FXMIN(tb->album.length()-1,tb->album.find(' ')+1);
  x = comparecase(&ta->album[a],&tb->album[b]);

  if (x!=0) return x;

  /// Track & Disc
  if (ta->no>tb->no) return 1;
  else if (ta->no<tb->no) return -1;
  return 0;
  }


FXint GMDBTrackItem::descendingConductor(const GMTrackItem* pa,const GMTrackItem* pb){
  const GMDBTrackItem * const ta = (GMDBTrackItem*)pa;
  const GMDBTrackItem * const tb = (GMDBTrackItem*)pb;

  register FXint a=0,b=0,x;


  x = keywordcompare(GET_ARTIST_STRING(tb->composer),GET_ARTIST_STRING(ta->composer));
  if (x!=0) return x;

  a=b=0;
  if (begins_with_keyword(ta->album)) a=FXMIN(ta->album.length()-1,ta->album.find(' ')+1);
  if (begins_with_keyword(tb->album)) b=FXMIN(tb->album.length()-1,tb->album.find(' ')+1);
  x = comparecase(&ta->album[a],&tb->album[b]);

  if (x!=0) return x;

  /// Track & Disc
  if (ta->no>tb->no) return 1;
  else if (ta->no<tb->no) return -1;
  return 0;
  }

FXint GMDBTrackItem::ascendingRating(const GMTrackItem* pa,const GMTrackItem* pb){
  register const FXlong a=(FXlong)((GMDBTrackItem*)pa)->rating;
  register const FXlong b=(FXlong)((GMDBTrackItem*)pb)->rating;
  if (a>b) return 1;
  else if (a<b) return -1;
  return 0;
  }

FXint GMDBTrackItem::descendingRating(const GMTrackItem* pa,const GMTrackItem* pb){
  register const FXlong a=(FXlong)((GMDBTrackItem*)pa)->rating;
  register const FXlong b=(FXlong)((GMDBTrackItem*)pb)->rating;
  if (a>b) return -1;
  else if (a<b) return 1;
  return 0;
  }



GMLocalTrackItem::GMLocalTrackItem(FXint i,const FXString & f, FXuchar flags) : GMTrackItem(i),filename(f) {
  state|=(flags&FOLDER);

  if (state&FOLDER) {
    if (filename!="." && filename!="..")
      state|=DRAGGABLE;
    }
  else {
    state|=DRAGGABLE;
    }

  }

FXIcon* GMLocalTrackItem::getIcon() const {
  if (state&FOLDER){
    return GMIconTheme::instance()->icon_folder_small;
    }
  else {
    return GMIconTheme::instance()->icon_file_small;
    }
  }

const FXString * GMLocalTrackItem::getColumnData(FXint type,FXString&,FXuint & justify,FXint &) const{
  const FXString * textptr;
  justify=COLUMN_JUSTIFY_NORMAL;
  switch(type){
    case HEADER_FILENAME: textptr = &filename;  			break;
    default							: textptr = NULL;			 			  break;
    }
  return textptr;
  }


FXint GMLocalTrackItem::ascendingFilename(const GMTrackItem* pa,const GMTrackItem* pb){
  const GMLocalTrackItem * const ta = (GMLocalTrackItem*)pa;
  const GMLocalTrackItem * const tb = (GMLocalTrackItem*)pb;
  FXint diff = (tb->state&FOLDER)-(ta->state&FOLDER);
  if (diff==0) return comparecase(ta->filename,tb->filename);;
  return diff;
  }

FXint GMLocalTrackItem::descendingFilename(const GMTrackItem* pa,const GMTrackItem* pb){
  const GMLocalTrackItem * const ta = (GMLocalTrackItem*)pa;
  const GMLocalTrackItem * const tb = (GMLocalTrackItem*)pb;
  FXint diff = (ta->state&FOLDER)-(tb->state&FOLDER);
  if (diff==0) return -comparecase(ta->filename,tb->filename);
  return diff;
  }





GMFeedItem::GMFeedItem(FXint i,const FXchar * tf,const FXchar * t, FXTime d,FXuint tm,FXuint f) : GMTrackItem(i),feed(tf),title(t),date(d),time(tm),flags(f) {
  if (flags&(1<<ITEM_FLAG_PLAYED))
    state|=SHADED;
  }


FXIcon* GMFeedItem::getIcon() const {
  if (flags&(1<<ITEM_FLAG_LOCAL)) {   
    return GMIconTheme::instance()->icon_localcopy;
    }
  else if (flags&(1<<ITEM_FLAG_QUEUE)){
    return GMIconTheme::instance()->icon_download;
    }
  else {
    return NULL;    
    }
  }


const FXString * GMFeedItem::getColumnData(FXint type,FXString&text,FXuint & justify,FXint &) const{
  const FXString * textptr;
  justify=COLUMN_JUSTIFY_NORMAL;
  switch(type){
    case HEADER_ALBUM   : textptr = &feed;  			break;
    case HEADER_TITLE   : textptr = &title;  			break;
    case HEADER_DATE    : text=FXSystem::localTime("%b %d, %Y",date);
                          textptr=&text;
                          break;
    case HEADER_STATUS  : if (flags&(1<<ITEM_FLAG_LOCAL))
                            text = "On Disk";
                          else if (flags&(1<<ITEM_FLAG_QUEUE))
                            text = "In Queue";
                          else
                            text.clear();

                          textptr=&text;
                          break;
    case HEADER_TIME    : if (time) {
                            if (time>3600)
                              text.format("%d:%.2d:%.2d",(time/3600),(time%3600)/60,(time%60));
                            else
                              text.format("%d:%.2d",time/60,time%60);

                            textptr=&text;
                            justify=COLUMN_JUSTIFY_CENTER_RIGHT_ALIGNED;
                            }
                          else {
                            textptr = NULL;
                            }
                          //max=GMDBTrackItem::max_time;
                          break;

    default							: textptr = NULL;			 			  break;
    }
  return textptr;
  }


FXint GMFeedItem::ascendingDate(const GMTrackItem* pa,const GMTrackItem* pb){
  const GMFeedItem * const ta = (GMFeedItem*)pa;
  const GMFeedItem * const tb = (GMFeedItem*)pb;
  if (ta->date>tb->date) return 1;
  else if (ta->date<tb->date) return -1;
  return 0;
  }

FXint GMFeedItem::descendingDate(const GMTrackItem* pa,const GMTrackItem* pb){
  const GMFeedItem * const ta = (GMFeedItem*)pa;
  const GMFeedItem * const tb = (GMFeedItem*)pb;
  if (ta->date>tb->date) return -1;
  else if (ta->date<tb->date) return 1;
  return 0;
  }


FXint GMFeedItem::ascendingTime(const GMTrackItem* pa,const GMTrackItem* pb){
  register const FXint a=((GMFeedItem*)pa)->time;
  register const FXint b=((GMFeedItem*)pb)->time;
  if (a>b) return 1;
  if (a<b) return -1;
  return 0;
  }

FXint GMFeedItem::descendingTime(const GMTrackItem* pa,const GMTrackItem* pb){
  register const FXint a=((GMFeedItem*)pa)->time;
  register const FXint b=((GMFeedItem*)pb)->time;
  if (a>b) return -1;
  if (a<b) return 1;
  return 0;
  }

FXint GMFeedItem::ascendingFeed(const GMTrackItem* pa,const GMTrackItem* pb){
  const GMFeedItem * const ta = (GMFeedItem*)pa;
  const GMFeedItem * const tb = (GMFeedItem*)pb;

  register FXint x = keywordcompare(ta->feed,tb->feed);
  if (x!=0) return x;

  if (ta->date<tb->date) return 1;
  else if (ta->date>tb->date) return -1;
  return 0;
  }

FXint GMFeedItem::descendingFeed(const GMTrackItem* pa,const GMTrackItem* pb){
  const GMFeedItem * const ta = (GMFeedItem*)pa;
  const GMFeedItem * const tb = (GMFeedItem*)pb;

  register FXint x = keywordcompare(tb->feed,ta->feed);
  if (x!=0) return x;

  if (ta->date<tb->date) return 1;
  else if (ta->date>tb->date) return -1;
  return 0;
  }

FXint GMFeedItem::ascendingTitle(const GMTrackItem* pa,const GMTrackItem* pb){
  const GMFeedItem * const ta = (GMFeedItem*)pa;
  const GMFeedItem * const tb = (GMFeedItem*)pb;
  register FXint a=0,b=0;
  if (begins_with_keyword(ta->title)) a=FXMIN(ta->title.length()-1,ta->title.find(' ')+1);
  if (begins_with_keyword(tb->title)) b=FXMIN(tb->title.length()-1,tb->title.find(' ')+1);
  return comparecase(&ta->title[a],&tb->title[b]);
  }

FXint GMFeedItem::descendingTitle(const GMTrackItem* pa,const GMTrackItem* pb){
  const GMFeedItem * const ta = (GMFeedItem*)pa;
  const GMFeedItem * const tb = (GMFeedItem*)pb;
  register FXint a=0,b=0;
  if (begins_with_keyword(ta->title)) a=FXMIN(ta->title.length()-1,ta->title.find(' ')+1);
  if (begins_with_keyword(tb->title)) b=FXMIN(tb->title.length()-1,tb->title.find(' ')+1);
  return -comparecase(&ta->title[a],&tb->title[b]);
  }
