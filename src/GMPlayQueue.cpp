/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2009-2021 by Sander Jansen. All Rights Reserved      *
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
#include "gmdefs.h"
#include "gmutils.h"
#include "GMList.h"
#include "GMTrack.h"
#include "GMDatabase.h"
#include "GMTrackDatabase.h"
#include "GMTrackList.h"
#include "GMTrackItem.h"
#include "GMTrackView.h"
#include "GMSource.h"
#include "GMSourceView.h"
#include "GMClipboard.h"
#include "GMDatabaseSource.h"
#include "GMPlayListSource.h"
#include "GMPlayQueue.h"
#include "GMPlayerManager.h"
#include "GMWindow.h"
#include "GMIconTheme.h"
#include "GMFilename.h"

extern void getSelectedTrackQueues(FXIntList & list);


FXDEFMAP(GMPlayQueue) GMPlayQueueMap[]={
  FXMAPFUNC(SEL_COMMAND,GMPlayQueue::ID_DELETE_TRACK,GMPlayQueue::onCmdRemoveInPlaylist),
  FXMAPFUNC(SEL_COMMAND,GMPlayQueue::ID_CLEAR,GMPlayQueue::onCmdClear)
  };

FXIMPLEMENT(GMPlayQueue,GMPlayListSource,GMPlayQueueMap,ARRAYNUMBER(GMPlayQueueMap));


GMPlayQueue::GMPlayQueue(GMTrackDatabase * database) : GMPlayListSource(database,database->getPlayQueue())  {
  updateTrackHash();
  ntracks=0;
  poptrack=false;
  GMQuery q(db,"SELECT count(track) FROM playlist_tracks WHERE playlist==?");
  q.execute(playlist,ntracks);
  }

GMPlayQueue::~GMPlayQueue() {
  }




void GMPlayQueue::configure(GMColumnList& list) {
  list.no(17);
  FXint i=0;
  list[i++]=GMColumn(notr("No"),HEADER_TRACK,GMDBTrackItem::ascendingTrack,GMDBTrackItem::descendingTrack,43,false ,false,0);
  list[i++]=GMColumn(notr("Queue"),HEADER_QUEUE,GMDBTrackItem::ascendingQueue,GMDBTrackItem::descendingQueue,70,true,true,1);
  list[i++]=GMColumn(notr("Artist"),HEADER_ARTIST,GMDBTrackItem::ascendingArtist,GMDBTrackItem::descendingArtist,200,true,true,2);
  list[i++]=GMColumn(notr("Title"),HEADER_TITLE,GMDBTrackItem::ascendingTitle,GMDBTrackItem::descendingTitle,300,true,true,3);
  list[i++]=GMColumn(notr("Album"),HEADER_ALBUM,GMDBTrackItem::ascendingAlbum,GMDBTrackItem::descendingAlbum,200,true,true,4);
  list[i++]=GMColumn(notr("Year"),HEADER_YEAR,GMDBTrackItem::ascendingYear,GMDBTrackItem::descendingYear,60,false,false,5);
  list[i++]=GMColumn(notr("Album Artist"),HEADER_ALBUM_ARTIST,GMDBTrackItem::ascendingAlbumArtist,GMDBTrackItem::descendingAlbumArtist,200,false,false,6);
  list[i++]=GMColumn(notr("Disc"),HEADER_DISC,GMDBTrackItem::ascendingDisc,GMDBTrackItem::descendingDisc,43,false,false,7);
  list[i++]=GMColumn(notr("Time"),HEADER_TIME,GMDBTrackItem::ascendingTime,GMDBTrackItem::descendingTime,60,true,true,8);
  list[i++]=GMColumn(notr("Play Count"),HEADER_PLAYCOUNT,GMDBTrackItem::ascendingPlaycount,GMDBTrackItem::descendingPlaycount,60,false,false,9);
  list[i++]=GMColumn(notr("Play Date"),HEADER_PLAYDATE,GMDBTrackItem::ascendingPlaydate,GMDBTrackItem::descendingPlaydate,60,false,false,10);
  list[i++]=GMColumn(notr("Path"),HEADER_FILENAME,GMDBTrackItem::ascendingFilename,GMDBTrackItem::descendingFilename,400,false,false,11);
  list[i++]=GMColumn(notr("File Type"),HEADER_FILETYPE,GMDBTrackItem::ascendingFiletype,GMDBTrackItem::descendingFiletype,30,false,false,12);
  list[i++]=GMColumn(notr("Format"),HEADER_AUDIOFORMAT,GMDBTrackItem::ascendingFormat,GMDBTrackItem::descendingFormat,400,false,false,14);
  list[i++]=GMColumn(notr("Composer"),HEADER_COMPOSER,GMDBTrackItem::ascendingComposer,GMDBTrackItem::descendingComposer,30,false,false,14);
  list[i++]=GMColumn(notr("Conductor"),HEADER_CONDUCTOR,GMDBTrackItem::ascendingConductor,GMDBTrackItem::descendingConductor,400,false,false,15);
  list[i++]=GMColumn(notr("Rating"),HEADER_RATING,GMDBTrackItem::ascendingRating,GMDBTrackItem::descendingRating,30,false,false,17,this,ID_EDIT_RATING);
  }



FXbool GMPlayQueue::findCurrent(GMTrackList * list,GMSource * src) {
  if (src->getCurrentTrack()==-1) return false;
  if (src==this) {
    for (FXint i=0;i<list->getNumItems();i++){
      if (list->getItemId(i)==current_track && dynamic_cast<GMDBTrackItem*>(list->getItem(i))->getTrackQueue()==1) {
        list->setActiveItem(i);
        list->setCurrentItem(i);
        return true;
        }
      }
    }
  return false;
  }



FXint GMPlayQueue::getNumTracks() const {
  return ntracks;
  }


FXString GMPlayQueue::getName() const {
  return FXString::value("Play Queue (%d)",ntracks);
  }

FXbool GMPlayQueue::source_context_menu(FXMenuPane * pane) {
  new GMMenuCommand(pane,fxtr("Clear"),GMIconTheme::instance()->icon_delete,this,ID_CLEAR);
  return true;
  }


void GMPlayQueue::updateTrackHash() {
  GM_TICKS_START();
  FXint track;
  FXint count;
  tracks.clear();
  try {
    GMQuery q(db,"SELECT track,count(track) FROM playlist_tracks WHERE playlist==? GROUP BY track;");
    q.set(0,playlist);
    while(q.row()) {
      q.get(0,track);
      q.get(1,count);
      tracks.insert(track,count);
      }
    }
  catch(GMDatabaseException & e){
    }
  GM_TICKS_END();
  }




FXbool GMPlayQueue::track_context_menu(FXMenuPane * pane){
  new GMMenuCommand(pane,fxtr("Edit…\tF2\tEdit Track Information."),GMIconTheme::instance()->icon_edit,this,GMDatabaseSource::ID_EDIT_TRACK);
  new GMMenuCommand(pane,fxtr("Copy\tCtrl-C\tCopy track(s) to clipboard."),GMIconTheme::instance()->icon_copy,this,ID_COPY_TRACK);
  new FXMenuSeparator(pane);
  new GMMenuCommand(pane,fxtr("Remove…\tDel\tRemove track(s) from play queue."),GMIconTheme::instance()->icon_delete,this,ID_DELETE_TRACK);
  return true;
  }


FXbool GMPlayQueue::canPlaySource(GMSource * src) const {
  return (src && (src->getType()==SOURCE_DATABASE || src->getType()==SOURCE_DATABASE_FILTER || src->getType()==SOURCE_DATABASE_PLAYLIST || src->getType()==SOURCE_PLAYQUEUE));
  }

void GMPlayQueue::addTracks(GMSource * src,const FXIntList & tracks) {
  if (src!=this && canPlaySource(src) && tracks.no() && db->insertPlaylistTracks(playlist,tracks)) {
    ntracks+=tracks.no();
    updateTrackHash();
    GMPlayerManager::instance()->getSourceView()->refresh(this);
    }
  }

long GMPlayQueue::onCmdClear(FXObject*,FXSelector,void*){
  db->executeFormat("DELETE FROM playlist_tracks WHERE playlist == %d",playlist);
  updateTrackHash();
  ntracks=0;
  poptrack=false;
  GMPlayerManager::instance()->getSourceView()->refresh(this);
  GMPlayerManager::instance()->getTrackView()->refresh();
  return 1;
  }


long GMPlayQueue::onCmdRemoveInPlaylist(FXObject*,FXSelector,void*){
  FXIntList queue;
  FXIntList tracks;
  getSelectedTrackQueues(queue);
  if (queue.no()) {
    try {
      GMLockTransaction transaction(db);
      db->removePlaylistQueue(playlist,queue);
      transaction.commit();
      ntracks-=queue.no();
      updateTrackHash();
      }
    catch(GMDatabaseException&){
      return 1;
      }
    GMPlayerManager::instance()->getTrackView()->refresh();
    GMPlayerManager::instance()->getSourceView()->refresh(this);
    }
  return 1;
  }



FXbool GMPlayQueue::hasTrack(FXint id) const{
  return tracks.at(id)>0;
  }



FXint GMPlayQueue::getNext() {
  GMQuery q(db,"SELECT track FROM playlist_tracks WHERE playlist == ? ORDER BY queue ASC LIMIT 1");

  current_track=-1;
  q.execute(playlist,current_track);
  if (current_track>0 && poptrack) {

    FXint cnt = tracks.at(current_track) - 1;
    if (cnt>0)
      tracks.insert(current_track,cnt);
    else
      tracks.remove(current_track);

    if (ntracks) ntracks--;

    db->executeFormat("DELETE FROM playlist_tracks WHERE playlist == %d AND queue == (SELECT MIN(queue) FROM playlist_tracks WHERE playlist == %d);",playlist,playlist);

    current_track=-1;
    q.execute(playlist,current_track);
    }
  poptrack=true;
  return current_track;
  }


FXint GMPlayQueue::getCurrent() {
  current_track=-1;
  try {
    GMQuery q(db,"SELECT track FROM playlist_tracks WHERE playlist == ? ORDER BY queue ASC LIMIT 1");
    q.execute(playlist,current_track);
    poptrack=true;
    }
  catch(GMDatabaseException & e){
    return -1;
    }
  return current_track;
  }
