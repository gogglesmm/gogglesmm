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
#include "gmutils.h"
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

#define VALUE_SORT_ASC(a,b) (a>b) ? 1 : ((a<b) ? -1 : 0);
#define VALUE_SORT_DSC(a,b) (a>b) ? -1 : ((a<b) ? 1 : 0);

#define GET_ARTIST_STRING(x)  GMPlayerManager::instance()->getTrackDatabase()->getArtist(x)


// return true if string starts with configured keyword
static inline FXbool begins_with_keyword(const FXString & t){
  for (FXint i=0;i<GMPlayerManager::instance()->getPreferences().gui_sort_keywords.no();i++){
    if (comparecase(t,GMPlayerManager::instance()->getPreferences().gui_sort_keywords[i],GMPlayerManager::instance()->getPreferences().gui_sort_keywords[i].length())==0) return true;
    }
  return false;
  }

// return true if string starts with configured keyword
static inline FXbool begins_with_keyword_ptr(const FXString * t){
  for (FXint i=0;i<GMPlayerManager::instance()->getPreferences().gui_sort_keywords.no();i++){
    if (comparecase(*t,GMPlayerManager::instance()->getPreferences().gui_sort_keywords[i],GMPlayerManager::instance()->getPreferences().gui_sort_keywords[i].length())==0) return true;
    }
  return false;
  }

// compare two string taking into account the configured keywords it needs to ignore
static inline FXint keywordcompare(const FXString *a,const FXString *b) {
  FXint pa,pb;

  if (a==b) return 0;

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

// compare two string taking into account the configured keywords it needs to ignore
static inline FXint keywordcompare(const FXString & a,const FXString & b) {
  FXint pa,pb;
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


GMDBTrackItem::GMDBTrackItem(FXint track_id,FXint track_path,const FXchar * track_mrl,const FXchar * track_title,FXint track_artist,FXint track_album_artist,FXint track_composer,FXint track_conductor,FXint album_id,const FXchar * track_album,FXint track_time,FXuint track_no,FXint track_queue,FXushort track_year,FXushort track_album_year,FXushort track_playcount,FXuchar track_filetype,FXint track_bitrate,FXint track_samplerate,FXuchar track_channels,FXlong track_playdate,FXuchar track_rating) :
  GMTrackItem(track_id), mrl(track_mrl),
                 title(track_title),
                 album(track_album),
                 playdate(track_playdate),
                 artist(track_artist),
                 albumartist(track_album_artist),
                 composer(track_composer),conductor(track_conductor),
                 albumid(album_id),
                 time(track_time),
                 no(track_no),
                 queue(track_queue),
                 path(track_path),
                 bitrate(track_bitrate),
                 samplerate(track_samplerate),
                 channels(track_channels),
                 filetype(track_filetype),
                 year(track_year),
                 album_year(track_album_year),
                 playcount(track_playcount),
                 rating(track_rating) {

  state|=GMTrackItem::DRAGGABLE;
  }

GMDBTrackItem::~GMDBTrackItem(){
  }


FXIcon * GMDBTrackItem::getIcon() const {
  if (GMPlayerManager::instance()->getPlayQueue() && GMPlayerManager::instance()->getTrackView()->getSource()!=GMPlayerManager::instance()->getPlayQueue() && GMPlayerManager::instance()->getPlayQueue()->hasTrack(id))
    return GMIconTheme::instance()->icon_playqueue;
  else
    return NULL;
  }


const FXchar * const filetypes[] = {
  "",
  "vorbis",
  "opus",
  "speex",
  "ogg flac",
  "flac",
  "mp3",
  "aac",
  "alac"
  };


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
    case HEADER_FILETYPE      : text=filetypes[filetype];
                                textptr = &text;
                                break;
    case HEADER_TITLE         : textptr = &title;  			break;
    case HEADER_FILENAME      : text.format("%s/%s",GMPlayerManager::instance()->getTrackDatabase()->getTrackPath(path),mrl.text());
                                textptr=&text;
                                break;
    case HEADER_ALBUM         : textptr = &album;  			break;
    case HEADER_ARTIST        : textptr = GMPlayerManager::instance()->getTrackDatabase()->getArtist(artist);       break;
    case HEADER_ALBUM_ARTIST  : textptr = GMPlayerManager::instance()->getTrackDatabase()->getArtist(albumartist);  break;
    case HEADER_COMPOSER      : textptr = GMPlayerManager::instance()->getTrackDatabase()->getArtist(composer);     break;
    case HEADER_CONDUCTOR     : textptr = GMPlayerManager::instance()->getTrackDatabase()->getArtist(conductor);    break;
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
    case HEADER_BITRATE       :
                                if (bitrate < 0)
                                  text.format("%d bit",-bitrate);
                                else
                                  text.format("%d kbps",bitrate);
                                textptr=&text;
                                justify=COLUMN_JUSTIFY_RIGHT;
                                break;
    case HEADER_AUDIOFORMAT   :
                                if (channels>2) {
                                  if (bitrate>samplerate)
                                    text.format("%s %dch %d bit %g kHz",filetypes[filetype],channels,bitrate/(samplerate*channels),(float)samplerate/1000.0f);
                                  else if (bitrate>0)
                                    text.format("%s %dch %d kbps %g kHz",filetypes[filetype],channels,bitrate,(float)samplerate/1000.0f);
                                  else
                                    text.format("%s %dch %g kHz",filetypes[filetype],channels,(float)samplerate/1000.0f);
                                  }
                                else {
                                  if (bitrate>samplerate)
                                    text.format("%s %d bit %g kHz",filetypes[filetype],bitrate/(samplerate*channels),(float)samplerate/1000.0f);
                                  else if (bitrate>0)
                                    text.format("%s %d kbps %g kHz",filetypes[filetype],bitrate,(float)samplerate/1000.0f);
                                  else
                                    text.format("%s %g kHz",filetypes[filetype],(float)samplerate/1000.0f);
                                  }
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





FXint GMDBTrackItem::browseSort(const GMTrackItem * pa,const GMTrackItem * pb){
  const GMDBTrackItem * const ta = (GMDBTrackItem*)pa;
  const GMDBTrackItem * const tb = (GMDBTrackItem*)pb;
  FXint x;

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

  if (ta->albumid>tb->albumid) return 1;
  else if (ta->albumid<tb->albumid) return -1;

  /// Track & Disc
  if (ta->no>tb->no) return 1;
  else if (ta->no<tb->no) return -1;

  return 0;
  }


FXint GMDBTrackItem::ascendingFilename(const GMTrackItem* pa,const GMTrackItem* pb){
  const GMDBTrackItem * const ta = dynamic_cast<const GMDBTrackItem*>(pa);
  const GMDBTrackItem * const tb = dynamic_cast<const GMDBTrackItem*>(pb);
  if (ta->path!=tb->path) {
    const GMTrackDatabase* const db = GMPlayerManager::instance()->getTrackDatabase();
    FXint x = comparecase(db->getTrackPath(ta->path),db->getTrackPath(tb->path));
    if (x!=0) return x;
    }
  return comparecase(ta->mrl,tb->mrl);
  }


FXint GMDBTrackItem::descendingFilename(const GMTrackItem* pa,const GMTrackItem* pb){
  const GMDBTrackItem * const ta = dynamic_cast<const GMDBTrackItem*>(pa);
  const GMDBTrackItem * const tb = dynamic_cast<const GMDBTrackItem*>(pb);
  if (ta->path!=tb->path) {
    const GMTrackDatabase* const db = GMPlayerManager::instance()->getTrackDatabase();
    FXint x = comparecase(db->getTrackPath(ta->path),db->getTrackPath(tb->path));
    if (x!=0) return -x;
    }
  return -comparecase(ta->mrl,tb->mrl);
  }


FXint GMDBTrackItem::ascendingFiletype(const GMTrackItem* pa,const GMTrackItem* pb){
  const GMDBTrackItem * const ta = dynamic_cast<const GMDBTrackItem*>(pa);
  const GMDBTrackItem * const tb = dynamic_cast<const GMDBTrackItem*>(pb);
  return comparecase(FXPath::extension(ta->mrl),FXPath::extension(tb->mrl));
  }


FXint GMDBTrackItem::descendingFiletype(const GMTrackItem* pa,const GMTrackItem* pb){
  const GMDBTrackItem * const ta = dynamic_cast<const GMDBTrackItem*>(pa);
  const GMDBTrackItem * const tb = dynamic_cast<const GMDBTrackItem*>(pb);
  return -comparecase(FXPath::extension(ta->mrl),FXPath::extension(tb->mrl));
  }


FXint GMDBTrackItem::ascendingTitle(const GMTrackItem* pa,const GMTrackItem* pb){
  const GMDBTrackItem * const ta = dynamic_cast<const GMDBTrackItem*>(pa);
  const GMDBTrackItem * const tb = dynamic_cast<const GMDBTrackItem*>(pb);
  return keywordcompare(ta->title,tb->title);
  }


FXint GMDBTrackItem::descendingTitle(const GMTrackItem* pa,const GMTrackItem* pb){
  const GMDBTrackItem * const ta = dynamic_cast<const GMDBTrackItem*>(pa);
  const GMDBTrackItem * const tb = dynamic_cast<const GMDBTrackItem*>(pb);
  return keywordcompare(tb->title,ta->title);
  }


FXint GMDBTrackItem::ascendingTrack(const GMTrackItem* pa,const GMTrackItem* pb){
  const FXuint a=GMTRACKNO(dynamic_cast<const GMDBTrackItem*>(pa)->no);
  const FXuint b=GMTRACKNO(dynamic_cast<const GMDBTrackItem*>(pb)->no);
  return VALUE_SORT_ASC(a,b);
  }


FXint GMDBTrackItem::descendingTrack(const GMTrackItem* pa,const GMTrackItem* pb){
  const FXuint a=GMTRACKNO(dynamic_cast<const GMDBTrackItem*>(pa)->no);
  const FXuint b=GMTRACKNO(dynamic_cast<const GMDBTrackItem*>(pb)->no);
  return VALUE_SORT_DSC(a,b);
  }


FXint GMDBTrackItem::ascendingDisc(const GMTrackItem* pa,const GMTrackItem* pb){
  const FXuint a=GMDISCNO(dynamic_cast<const GMDBTrackItem*>(pa)->no);
  const FXuint b=GMDISCNO(dynamic_cast<const GMDBTrackItem*>(pb)->no);
  return VALUE_SORT_ASC(a,b);
  }


FXint GMDBTrackItem::descendingDisc(const GMTrackItem* pa,const GMTrackItem* pb){
  const FXuint a=GMDISCNO(dynamic_cast<const GMDBTrackItem*>(pa)->no);
  const FXuint b=GMDISCNO(dynamic_cast<const GMDBTrackItem*>(pb)->no);
  return VALUE_SORT_DSC(a,b);
  }


FXint GMDBTrackItem::ascendingQueue(const GMTrackItem* pa,const GMTrackItem* pb){
  const FXint a=dynamic_cast<const GMDBTrackItem*>(pa)->queue;
  const FXint b=dynamic_cast<const GMDBTrackItem*>(pb)->queue;
  return VALUE_SORT_ASC(a,b);
  }


FXint GMDBTrackItem::descendingQueue(const GMTrackItem* pa,const GMTrackItem* pb){
  const FXint a=dynamic_cast<const GMDBTrackItem*>(pa)->queue;
  const FXint b=dynamic_cast<const GMDBTrackItem*>(pb)->queue;
  return VALUE_SORT_DSC(a,b);
  }


FXint GMDBTrackItem::ascendingPlaycount(const GMTrackItem* pa,const GMTrackItem* pb){
  const FXushort a=dynamic_cast<const GMDBTrackItem*>(pa)->playcount;
  const FXushort b=dynamic_cast<const GMDBTrackItem*>(pb)->playcount;
  return VALUE_SORT_ASC(a,b);
  }


FXint GMDBTrackItem::descendingPlaycount(const GMTrackItem* pa,const GMTrackItem* pb){
  const FXushort a=dynamic_cast<const GMDBTrackItem*>(pa)->playcount;
  const FXushort b=dynamic_cast<const GMDBTrackItem*>(pb)->playcount;
  return VALUE_SORT_DSC(a,b);
  }

FXint GMDBTrackItem::ascendingPlaydate(const GMTrackItem* pa,const GMTrackItem* pb){
  const FXlong a=dynamic_cast<const GMDBTrackItem*>(pa)->playdate;
  const FXlong b=dynamic_cast<const GMDBTrackItem*>(pb)->playdate;
  return VALUE_SORT_ASC(a,b);
  }


FXint GMDBTrackItem::descendingPlaydate(const GMTrackItem* pa,const GMTrackItem* pb){
  const FXlong a=dynamic_cast<const GMDBTrackItem*>(pa)->playdate;
  const FXlong b=dynamic_cast<const GMDBTrackItem*>(pb)->playdate;
  return VALUE_SORT_DSC(a,b);
  }


FXint GMDBTrackItem::ascendingYear(const GMTrackItem* pa,const GMTrackItem* pb){
  const FXushort a=dynamic_cast<const GMDBTrackItem*>(pa)->year;
  const FXushort b=dynamic_cast<const GMDBTrackItem*>(pb)->year;
  return VALUE_SORT_ASC(a,b);
  }


FXint GMDBTrackItem::descendingYear(const GMTrackItem* pa,const GMTrackItem* pb){
  const FXushort a=dynamic_cast<const GMDBTrackItem*>(pa)->year;
  const FXushort b=dynamic_cast<const GMDBTrackItem*>(pb)->year;
  return VALUE_SORT_DSC(a,b);
  }


FXint GMDBTrackItem::ascendingTime(const GMTrackItem* pa,const GMTrackItem* pb){
  const FXint a=dynamic_cast<const GMDBTrackItem*>(pa)->time;
  const FXint b=dynamic_cast<const GMDBTrackItem*>(pb)->time;
  return VALUE_SORT_ASC(a,b);
  }


FXint GMDBTrackItem::descendingTime(const GMTrackItem* pa,const GMTrackItem* pb){
  const FXint a=dynamic_cast<const GMDBTrackItem*>(pa)->time;
  const FXint b=dynamic_cast<const GMDBTrackItem*>(pb)->time;
  return VALUE_SORT_DSC(a,b);
  }


FXint GMDBTrackItem::ascendingBitrate(const GMTrackItem* pa,const GMTrackItem* pb){
  const FXint a=dynamic_cast<const GMDBTrackItem*>(pa)->bitrate;
  const FXint b=dynamic_cast<const GMDBTrackItem*>(pb)->bitrate;
  return VALUE_SORT_ASC(a,b);
  }


FXint GMDBTrackItem::descendingBitrate(const GMTrackItem* pa,const GMTrackItem* pb){
  const FXint a=dynamic_cast<const GMDBTrackItem*>(pa)->bitrate;
  const FXint b=dynamic_cast<const GMDBTrackItem*>(pb)->bitrate;
  return VALUE_SORT_DSC(a,b);
  }

FXint GMDBTrackItem::ascendingFormat(const GMTrackItem* pa,const GMTrackItem* pb){
  const FXint a=dynamic_cast<const GMDBTrackItem*>(pa)->bitrate;
  const FXint b=dynamic_cast<const GMDBTrackItem*>(pb)->bitrate;
  return VALUE_SORT_ASC(a,b);
  }


FXint GMDBTrackItem::descendingFormat(const GMTrackItem* pa,const GMTrackItem* pb){
  const FXint a=dynamic_cast<const GMDBTrackItem*>(pa)->bitrate;
  const FXint b=dynamic_cast<const GMDBTrackItem*>(pb)->bitrate;
  return VALUE_SORT_DSC(a,b);
  }



FXint GMDBTrackItem::ascendingAlbum(const GMTrackItem* pa,const GMTrackItem* pb){
  const GMDBTrackItem * const ta = dynamic_cast<const GMDBTrackItem*>(pa);
  const GMDBTrackItem * const tb = dynamic_cast<const GMDBTrackItem*>(pb);

  FXint x = keywordcompare(ta->album,tb->album);
  if (x!=0) return x;

  if (ta->albumid>tb->albumid) return 1;
  else if (ta->albumid<tb->albumid) return -1;

  /// Track & Disc
  return VALUE_SORT_ASC(ta->no,tb->no);
  }


FXint GMDBTrackItem::descendingAlbum(const GMTrackItem* pa,const GMTrackItem* pb){
  const GMDBTrackItem * const ta = dynamic_cast<const GMDBTrackItem*>(pa);
  const GMDBTrackItem * const tb = dynamic_cast<const GMDBTrackItem*>(pb);

  FXint x = keywordcompare(tb->album,ta->album);
  if (x!=0) return x;

  if (ta->albumid>tb->albumid) return -1;
  else if (ta->albumid<tb->albumid) return 1;

  /// Track & Disc (keep track order ascending)
  return VALUE_SORT_ASC(ta->no,tb->no);
  }


FXint GMDBTrackItem::ascendingArtist(const GMTrackItem* pa,const GMTrackItem* pb){
  const GMDBTrackItem * const ta = dynamic_cast<const GMDBTrackItem*>(pa);
  const GMDBTrackItem * const tb = dynamic_cast<const GMDBTrackItem*>(pb);
  FXint x;

  x = keywordcompare(GET_ARTIST_STRING(ta->artist),GET_ARTIST_STRING(tb->artist));
  if (x!=0) return x;

  x = keywordcompare(ta->album,tb->album);
  if (x!=0) return x;

  if (ta->albumid>tb->albumid) return 1;
  else if (ta->albumid<tb->albumid) return -1;

  /// Track & Disc
  return VALUE_SORT_ASC(ta->no,tb->no);
  }


FXint GMDBTrackItem::descendingArtist(const GMTrackItem* pa,const GMTrackItem* pb){
  const GMDBTrackItem * const ta = dynamic_cast<const GMDBTrackItem*>(pa);
  const GMDBTrackItem * const tb = dynamic_cast<const GMDBTrackItem*>(pb);
  FXint x;

  x = keywordcompare(GET_ARTIST_STRING(tb->artist),GET_ARTIST_STRING(ta->artist));
  if (x!=0) return x;

  x = keywordcompare(ta->album,tb->album);
  if (x!=0) return x;

  if (ta->albumid>tb->albumid) return -1;
  else if (ta->albumid<tb->albumid) return 1;

  /// Track & Disc (keep track order ascending)
  return VALUE_SORT_ASC(ta->no,tb->no);
  }


FXint GMDBTrackItem::ascendingAlbumArtist(const GMTrackItem* pa,const GMTrackItem* pb){
  const GMDBTrackItem * const ta = dynamic_cast<const GMDBTrackItem*>(pa);
  const GMDBTrackItem * const tb = dynamic_cast<const GMDBTrackItem*>(pb);

  FXint x;

  x = keywordcompare(GET_ARTIST_STRING(ta->albumartist),GET_ARTIST_STRING(tb->albumartist));
  if (x!=0) return x;

  x = keywordcompare(ta->album,tb->album);
  if (x!=0) return x;

  if (ta->albumid>tb->albumid) return 1;
  else if (ta->albumid<tb->albumid) return -1;


  /// Track & Disc
  return VALUE_SORT_ASC(ta->no,tb->no);
  }


FXint GMDBTrackItem::descendingAlbumArtist(const GMTrackItem* pa,const GMTrackItem* pb){
  const GMDBTrackItem * const ta = dynamic_cast<const GMDBTrackItem*>(pa);
  const GMDBTrackItem * const tb = dynamic_cast<const GMDBTrackItem*>(pb);

  FXint x;

  x = keywordcompare(GET_ARTIST_STRING(tb->albumartist),GET_ARTIST_STRING(ta->albumartist));
  if (x!=0) return x;

  x = keywordcompare(ta->album,tb->album);
  if (x!=0) return x;

  if (ta->albumid>tb->albumid) return -1;
  else if (ta->albumid<tb->albumid) return 1;


  /// Track & Disc (keep track order ascending)
  return VALUE_SORT_ASC(ta->no,tb->no);
  }


FXint GMDBTrackItem::ascendingComposer(const GMTrackItem* pa,const GMTrackItem* pb){
  const GMDBTrackItem * const ta = dynamic_cast<const GMDBTrackItem*>(pa);
  const GMDBTrackItem * const tb = dynamic_cast<const GMDBTrackItem*>(pb);

  FXint x;

  x = keywordcompare(GET_ARTIST_STRING(ta->composer),GET_ARTIST_STRING(tb->composer));
  if (x!=0) return x;

  x = keywordcompare(ta->album,tb->album);
  if (x!=0) return x;

  if (ta->albumid>tb->albumid) return 1;
  else if (ta->albumid<tb->albumid) return -1;

  /// Track & Disc
  return VALUE_SORT_ASC(ta->no,tb->no);
  }


FXint GMDBTrackItem::descendingComposer(const GMTrackItem* pa,const GMTrackItem* pb){
  const GMDBTrackItem * const ta = dynamic_cast<const GMDBTrackItem*>(pa);
  const GMDBTrackItem * const tb = dynamic_cast<const GMDBTrackItem*>(pb);

  FXint x;

  x = keywordcompare(GET_ARTIST_STRING(tb->composer),GET_ARTIST_STRING(ta->composer));
  if (x!=0) return x;

  x = keywordcompare(tb->album,ta->album);
  if (x!=0) return x;

  if (ta->albumid>tb->albumid) return -1;
  else if (ta->albumid<tb->albumid) return 1;

  /// Track & Disc (keep track order ascending)
  return VALUE_SORT_ASC(ta->no,tb->no);
  }


FXint GMDBTrackItem::ascendingConductor(const GMTrackItem* pa,const GMTrackItem* pb){
  const GMDBTrackItem * const ta = dynamic_cast<const GMDBTrackItem*>(pa);
  const GMDBTrackItem * const tb = dynamic_cast<const GMDBTrackItem*>(pb);

  FXint x;

  x = keywordcompare(GET_ARTIST_STRING(ta->composer),GET_ARTIST_STRING(tb->composer));
  if (x!=0) return x;

  x = keywordcompare(ta->album,tb->album);
  if (x!=0) return x;

  if (ta->albumid>tb->albumid) return 1;
  else if (ta->albumid<tb->albumid) return -1;

  /// Track & Disc
  return VALUE_SORT_ASC(ta->no,tb->no);
  }


FXint GMDBTrackItem::descendingConductor(const GMTrackItem* pa,const GMTrackItem* pb){
  const GMDBTrackItem * const ta = dynamic_cast<const GMDBTrackItem*>(pa);
  const GMDBTrackItem * const tb = dynamic_cast<const GMDBTrackItem*>(pb);

  FXint x;

  x = keywordcompare(GET_ARTIST_STRING(tb->composer),GET_ARTIST_STRING(ta->composer));
  if (x!=0) return x;

  x = keywordcompare(tb->album,ta->album);
  if (x!=0) return x;

  if (ta->albumid>tb->albumid) return -1;
  else if (ta->albumid<tb->albumid) return 1;


  /// Track & Disc (keep track order ascending)
  return VALUE_SORT_ASC(ta->no,tb->no);
  }


FXint GMDBTrackItem::ascendingRating(const GMTrackItem* pa,const GMTrackItem* pb){
  const FXuchar a=dynamic_cast<const GMDBTrackItem*>(pa)->rating;
  const FXuchar b=dynamic_cast<const GMDBTrackItem*>(pb)->rating;
  return VALUE_SORT_ASC(a,b);
  }


FXint GMDBTrackItem::descendingRating(const GMTrackItem* pa,const GMTrackItem* pb){
  const FXuchar a=dynamic_cast<const GMDBTrackItem*>(pa)->rating;
  const FXuchar b=dynamic_cast<const GMDBTrackItem*>(pb)->rating;
  return VALUE_SORT_DSC(a,b);
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
  const FXint a=dynamic_cast<const GMStreamTrackItem*>(pa)->bitrate;
  const FXint b=dynamic_cast<const GMStreamTrackItem*>(pb)->bitrate;
  return VALUE_SORT_ASC(a,b);
  }


FXint GMStreamTrackItem::descendingTime(const GMTrackItem* pa,const GMTrackItem* pb){
  const FXint a=dynamic_cast<const GMStreamTrackItem*>(pa)->bitrate;
  const FXint b=dynamic_cast<const GMStreamTrackItem*>(pb)->bitrate;
  return VALUE_SORT_DSC(a,b);
  }


FXint GMStreamTrackItem::ascendingGenre(const GMTrackItem* pa,const GMTrackItem* pb){
  const GMStreamTrackItem * const ta = dynamic_cast<const GMStreamTrackItem*>(pa);
  const GMStreamTrackItem * const tb = dynamic_cast<const GMStreamTrackItem*>(pb);
  FXint x;
  x = comparecase(ta->genre,tb->genre);
  if (x!=0) return x;
  return ascendingTrack(pa,pb);
  }


FXint GMStreamTrackItem::descendingGenre(const GMTrackItem* pa,const GMTrackItem* pb){
  const GMStreamTrackItem * const ta = dynamic_cast<const GMStreamTrackItem*>(pa);
  const GMStreamTrackItem * const tb = dynamic_cast<const GMStreamTrackItem*>(pb);
  FXint x;
  x = comparecase(tb->genre,ta->genre);
  if (x!=0) return x;
  return ascendingTrack(pa,pb);
  }


FXint GMStreamTrackItem::ascendingTrack(const GMTrackItem* pa,const GMTrackItem* pb){
  const FXint a=dynamic_cast<const GMStreamTrackItem*>(pa)->no;
  const FXint b=dynamic_cast<const GMStreamTrackItem*>(pb)->no;
  return VALUE_SORT_ASC(a,b);
  }

FXint GMStreamTrackItem::descendingTrack(const GMTrackItem* pa,const GMTrackItem* pb){
  const FXint a=dynamic_cast<const GMStreamTrackItem*>(pa)->no;
  const FXint b=dynamic_cast<const GMStreamTrackItem*>(pb)->no;
  return VALUE_SORT_DSC(a,b);
  }


FXint GMStreamTrackItem::ascendingTitle(const GMTrackItem* pa,const GMTrackItem* pb){
  const GMStreamTrackItem * const ta = dynamic_cast<const GMStreamTrackItem*>(pa);
  const GMStreamTrackItem * const tb = dynamic_cast<const GMStreamTrackItem*>(pb);
  FXint a=0,b=0;
  if (begins_with_keyword(ta->title)) a=FXMIN(ta->title.length()-1,ta->title.find(' ')+1);
  if (begins_with_keyword(tb->title)) b=FXMIN(tb->title.length()-1,tb->title.find(' ')+1);
  return comparecase(&ta->title[a],&tb->title[b]);
  }


FXint GMStreamTrackItem::descendingTitle(const GMTrackItem* pa,const GMTrackItem* pb){
  const GMStreamTrackItem * const ta = dynamic_cast<const GMStreamTrackItem*>(pa);
  const GMStreamTrackItem * const tb = dynamic_cast<const GMStreamTrackItem*>(pb);
  FXint a=0,b=0;
  if (begins_with_keyword(ta->title)) a=FXMIN(ta->title.length()-1,ta->title.find(' ')+1);
  if (begins_with_keyword(tb->title)) b=FXMIN(tb->title.length()-1,tb->title.find(' ')+1);
  return -comparecase(&ta->title[a],&tb->title[b]);
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
  const GMLocalTrackItem * const ta = dynamic_cast<const GMLocalTrackItem*>(pa);
  const GMLocalTrackItem * const tb = dynamic_cast<const GMLocalTrackItem*>(pb);
  FXint diff = (tb->state&FOLDER)-(ta->state&FOLDER);
  if (diff==0) return comparecase(ta->filename,tb->filename);;
  return diff;
  }


FXint GMLocalTrackItem::descendingFilename(const GMTrackItem* pa,const GMTrackItem* pb){
  const GMLocalTrackItem * const ta = dynamic_cast<const GMLocalTrackItem*>(pa);
  const GMLocalTrackItem * const tb = dynamic_cast<const GMLocalTrackItem*>(pb);
  FXint diff = (ta->state&FOLDER)-(tb->state&FOLDER);
  if (diff==0) return -comparecase(ta->filename,tb->filename);
  return diff;
  }





GMFeedItem::GMFeedItem(FXint i,const FXchar * tf,const FXchar * t, FXTime d,FXuint tm,FXuint f) : GMTrackItem(i),feed(tf),title(t),date(d),time(tm),flags(f) {
  if (flags&(1<<ITEM_FLAG_PLAYED))
    state|=SHADED;
  state|=GMTrackItem::DRAGGABLE;
 }


FXIcon* GMFeedItem::getIcon() const {
  if (flags&(1<<ITEM_FLAG_LOCAL)) {
    return GMIconTheme::instance()->icon_localcopy;
    }
  else if (flags&(1<<ITEM_FLAG_QUEUE)){
    return GMIconTheme::instance()->icon_download;
    }
  else if (flags&ITEM_FAILED){
    return GMIconTheme::instance()->icon_error;
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
  const GMFeedItem * const ta = dynamic_cast<const GMFeedItem*>(pa);
  const GMFeedItem * const tb = dynamic_cast<const GMFeedItem*>(pb);
  return VALUE_SORT_ASC(ta->date,tb->date);
  }


FXint GMFeedItem::descendingDate(const GMTrackItem* pa,const GMTrackItem* pb){
  const GMFeedItem * const ta = dynamic_cast<const GMFeedItem*>(pa);
  const GMFeedItem * const tb = dynamic_cast<const GMFeedItem*>(pb);
  return VALUE_SORT_DSC(ta->date,tb->date);
  }


FXint GMFeedItem::ascendingTime(const GMTrackItem* pa,const GMTrackItem* pb){
  const FXint a=dynamic_cast<const GMFeedItem*>(pa)->time;
  const FXint b=dynamic_cast<const GMFeedItem*>(pb)->time;
  return VALUE_SORT_ASC(a,b);
  }


FXint GMFeedItem::descendingTime(const GMTrackItem* pa,const GMTrackItem* pb){
  const FXint a=dynamic_cast<const GMFeedItem*>(pa)->time;
  const FXint b=dynamic_cast<const GMFeedItem*>(pb)->time;
  return VALUE_SORT_DSC(a,b);
  }


FXint GMFeedItem::ascendingFeed(const GMTrackItem* pa,const GMTrackItem* pb){
  const GMFeedItem * const ta = dynamic_cast<const GMFeedItem*>(pa);
  const GMFeedItem * const tb = dynamic_cast<const GMFeedItem*>(pb);

  FXint x = keywordcompare(ta->feed,tb->feed);
  if (x!=0) return x;

  return VALUE_SORT_ASC(ta->date,tb->date);
  }


FXint GMFeedItem::descendingFeed(const GMTrackItem* pa,const GMTrackItem* pb){
  const GMFeedItem * const ta = dynamic_cast<const GMFeedItem*>(pa);
  const GMFeedItem * const tb = dynamic_cast<const GMFeedItem*>(pb);

  FXint x = keywordcompare(tb->feed,ta->feed);
  if (x!=0) return x;

  // keep date ascending
  return VALUE_SORT_ASC(ta->date,tb->date);
  }


FXint GMFeedItem::ascendingTitle(const GMTrackItem* pa,const GMTrackItem* pb){
  const GMFeedItem * const ta = dynamic_cast<const GMFeedItem*>(pa);
  const GMFeedItem * const tb = dynamic_cast<const GMFeedItem*>(pb);
  FXint a=0,b=0;
  if (begins_with_keyword(ta->title)) a=FXMIN(ta->title.length()-1,ta->title.find(' ')+1);
  if (begins_with_keyword(tb->title)) b=FXMIN(tb->title.length()-1,tb->title.find(' ')+1);
  return comparecase(&ta->title[a],&tb->title[b]);
  }


FXint GMFeedItem::descendingTitle(const GMTrackItem* pa,const GMTrackItem* pb){
  const GMFeedItem * const ta = dynamic_cast<const GMFeedItem*>(pa);
  const GMFeedItem * const tb = dynamic_cast<const GMFeedItem*>(pb);
  FXint a=0,b=0;
  if (begins_with_keyword(ta->title)) a=FXMIN(ta->title.length()-1,ta->title.find(' ')+1);
  if (begins_with_keyword(tb->title)) b=FXMIN(tb->title.length()-1,tb->title.find(' ')+1);
  return -comparecase(&ta->title[a],&tb->title[b]);
  }
