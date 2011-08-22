/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2009-2010 by Sander Jansen. All Rights Reserved      *
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
  FXMAPFUNC(SEL_COMMAND,GMPlayQueue::ID_DELETE_TRACK,GMPlayQueue::onCmdRemoveInPlaylist)
  };

FXIMPLEMENT(GMPlayQueue,GMPlayListSource,GMPlayQueueMap,ARRAYNUMBER(GMPlayQueueMap));


GMPlayQueue::GMPlayQueue(GMTrackDatabase * database) : GMPlayListSource(database,database->getPlayQueue())  {
  updateTrackHash();
  ntracks=0;
  GMQuery q(db,"SELECT count(track) FROM playlist_tracks WHERE playlist==?");
  q.execute(playlist,ntracks);
  }

GMPlayQueue::~GMPlayQueue() {
  }

FXint GMPlayQueue::getNumTracks() const {
  return ntracks;
  }


FXString GMPlayQueue::getName() const {
  return FXString::value("Play Queue (%d)",ntracks);
  }

FXbool GMPlayQueue::source_context_menu(FXMenuPane * pane) {
  return false;
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
      tracks.insert((void*)(FXival)track,(void*)(FXival)count);
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

void GMPlayQueue::addTracks(GMSource * src,const FXIntList & tracks) {
  if (src!=this && src->getType()!=SOURCE_INTERNET_RADIO && tracks.no() && db->insertPlaylistTracks(playlist,tracks)) {
    ntracks+=tracks.no();
    updateTrackHash();
    GMPlayerManager::instance()->getSourceView()->refresh(this);
    }
  }



long GMPlayQueue::onCmdRemoveInPlaylist(FXObject*,FXSelector sel,void*){
  FXIntList queue;
  FXIntList tracks;
  getSelectedTrackQueues(queue);
///  GMPlayerManager::instance()->getTrackView()->getSelectedTracks(tracks);
  if (queue.no()==0) return 1;

  try {
    db->begin();
    db->removePlaylistTracks(playlist,queue);
    db->commit();
    }
  catch(GMDatabaseException&){
    db->rollback();
    return 1;
    }


  GMPlayerManager::instance()->getTrackView()->refresh();
  ntracks-=queue.no();
  updateTrackHash();

  return 1;
  }



FXbool GMPlayQueue::hasTrack(FXint id) {
  GM_TICKS_START();
  if (tracks.find((void*)(FXival)id)) {
    GM_TICKS_END();
    return true;
    }
  else {
    GM_TICKS_END();
    return false;
    }

  /*
  FXint n=0;
  try {
    GMQuery q(db,"SELECT COUNT(track) FROM playlist_tracks WHERE playlist == ? AND track == ?");
    q.set(0,playlist);
    q.set(1,id);
    q.execute();
    q.get(0,n);
    q.reset();
    }
  catch(FXCompileException & e){
    return false;
    }
  catch(FXExecuteException & e){
    return false;
    }
*/
 // GM_TICKS_END();
 // return (n>0);
  }



FXint GMPlayQueue::getNext() {
  current_track=-1;
  try {
    GMQuery q(db,"SELECT track FROM playlist_tracks WHERE playlist == ? ORDER BY queue ASC LIMIT 1");
    q.execute(playlist,current_track);
    db->executeFormat("DELETE FROM playlist_tracks WHERE playlist == %d AND queue == (SELECT MIN(queue) FROM playlist_tracks WHERE playlist == %d);",playlist,playlist);
    }
  catch(GMDatabaseException & e){
    return -1;
    }
  ntracks--;
  return current_track;
  }


#if 0

  current_track=-1;
  play_queue+=1;
  try {
    GMQuery q(db,"SELECT track FROM playlist_tracks WHERE playlist == ? AND queue == ?;");
    q.set(0,playlist);
    q.set(1,play_queue);
    q.execute();
    q.get(0,current_track);
    //fxmessage("check %d %d %d\n",playlist,current_queue,current_track);
    q.reset();
    }
  catch(FXCompileException & e){
    return -1;
    }
  catch(FXExecuteException & e){
    return -1;
    }
  return current_track;
  }
#endif

FXint GMPlayQueue::getPrev() {
#if 0
  current_track=-1;
  play_queue-=1;
  try {
    GMQuery q(db,"SELECT track FROM playlist_tracks WHERE playlist == ? AND queue == ?;");
    q.set(0,playlist);
    q.set(1,play_queue);
    q.execute();
    q.get(0,current_track);
    //fxmessage("check %d %d %d\n",playlist,current_queue,current_track);
    q.reset();
    }
  catch(FXCompileException & e){
    return -1;
    }
  catch(FXExecuteException & e){
    return -1;
    }
 #endif
  return current_track;
  }


FXint GMPlayQueue::getCurrent() {
  current_track=-1;
  try {
    GMQuery q(db,"SELECT track FROM playlist_tracks WHERE playlist == ? ORDER BY queue ASC LIMIT 1");
    q.execute(playlist,current_track);
    }
  catch(GMDatabaseException & e){
    return -1;
    }
  return current_track;
  }
















