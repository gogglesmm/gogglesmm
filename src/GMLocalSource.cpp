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
#include "gmdefs.h"
#include "gmutils.h"
#include "GMTrack.h"
#include "GMDatabase.h"
#include "GMTrackDatabase.h"
#include "GMTrackList.h"
#include "GMTrackItem.h"
#include "GMTrackView.h"
#include "GMSource.h"
#include "GMSourceView.h"
#include "GMClipboard.h"
#include "GMLocalSource.h"
#include "GMPlayerManager.h"
#include "GMWindow.h"
#include "GMIconTheme.h"
#include "GMFilename.h"


FXDEFMAP(GMLocalSource) GMLocalSourceMap[]={
  FXMAPFUNC(SEL_COMMAND,GMLocalSource::ID_COPY_TRACK,GMLocalSource::onCmdCopyTrack),
  FXMAPFUNC(SEL_DND_REQUEST,GMLocalSource::ID_COPY_TRACK,GMLocalSource::onCmdRequestTrack),
  };


FXIMPLEMENT(GMLocalSource,GMSource,GMLocalSourceMap,ARRAYNUMBER(GMLocalSourceMap));


GMLocalSource::GMLocalSource()  {
  path=FXSystem::getHomeDirectory();
  }

GMLocalSource::~GMLocalSource(){
  }

void GMLocalSource::configure(GMColumnList& list){
  list.no(1);
  list[0]=GMColumn(notr("Filename"),HEADER_FILENAME,GMLocalTrackItem::ascendingFilename,GMLocalTrackItem::descendingFilename,600,true,true,0);
  }


void GMLocalSource::markCurrent(const GMTrackItem* item) {
  GMSource::markCurrent(item);
  if (current_track!=-1) {  
    current_path = path + PATHSEPSTRING + (dynamic_cast<const GMLocalTrackItem*>(item))->getFilename();
    }
  }

FXbool GMLocalSource::findCurrent(GMTrackList * list,GMSource * src) {
  if (src==this) {
    if (FXPath::directory(current_path)==path) {
      const FXString name = FXPath::name(current_path);
      for (FXint i=0;i<list->getNumItems();i++){
        GMLocalTrackItem * item = dynamic_cast<GMLocalTrackItem*>(list->getItem(i));
        if (item->getFilename()==name) {
          list->setActiveItem(i);
          list->setCurrentItem(i);
          return true;
          }
        }
      }
    }
  return false;
  }


FXbool GMLocalSource::hasCurrentTrack(GMSource * src) const {
  if (src==this) return true;
  return false;
  }


FXbool GMLocalSource::getTrack(GMTrack & track) const {
  track.url=path+PATHSEPSTRING+files[current_track-1];
  track.loadTag(track.url);
  return true;
  }

FXbool GMLocalSource::track_double_click() {
  GMLocalTrackItem * item = (GMLocalTrackItem*)GMPlayerManager::instance()->getTrackView()->getCurrentTrackItem();
  if (item->getFilename()=="..") {
    path=FXPath::upLevel(path);
    GMPlayerManager::instance()->getTrackView()->refresh();
    return true;
    }
  else if (FXStat::isDirectory(path+PATHSEPSTRING+item->getFilename())){
    path+=PATHSEPSTRING+item->getFilename();
    GMPlayerManager::instance()->getTrackView()->refresh();
    return true;
    }
  return false;
  }


void GMLocalSource::save(FXSettings& settings) const {
  settings.writeStringEntry(settingKey().text(),"path",path.text());
  }

void GMLocalSource::load(FXSettings& settings) {
  path = settings.readStringEntry(settingKey().text(),"path",FXSystem::getHomeDirectory().text());
  }

FXuint GMLocalSource::dnd_provides(FXDragType types[]){
  types[0]=GMClipboard::kdeclipboard;
  types[1]=GMClipboard::urilistType;
  return 2;
  }


/*
FXbool GMLocalSource::source_context_menu(FXMenuPane * pane){
  return false;
  }

FXbool GMLocalSource::track_context_menu(FXMenuPane * pane){
  return false;
  }
*/
FXbool GMLocalSource::listTracks(GMTrackList * tracklist,const FXIntList &/* albumlist*/,const FXIntList & /*genre*/){

  while(!FXStat::isDirectory(path) && !path.empty()) {
    path=FXPath::upLevel(path);
    }



  FXDir     dir(path);
  FXStat    stat;
  FXString  name;
  FXString  pathname;
  FXString  ext;
  FXint     id=1;
  FXuchar   flags;


  files.clear();
    // Are we at the top directory?
  FXbool istop=FXPath::isTopDirectory(path);
  FXbool islink;

  tracklist->clearItems();
  if (dir.isOpen()) {

    // Loop over directory entries
    while(dir.next(name)){

      // Hidden files of the form ".xxx" are normally not shown, but ".." is so we can
      // navigate up as well as down.  However, at the root level we can't go up any
      // further so we show "." but not ".."; this allows us to explicitly select "/."
      // as a directory when we're in directory selection mode.
      if(name[0]=='.'){
        if(name[1]==0){
          continue;
          }
        else if(name[1]=='.' && name[2]==0){
          if(istop) continue;
          }
        else{
          continue;
          }
        }

      flags=0;

      // Build full pathname
      pathname=path+PATHSEPSTRING+name;

      // Get file/link info
      if(!FXStat::statLink(pathname,stat)) continue;

      // If its a link, get the info on file itself
      islink=stat.isLink();
      if(islink && !FXStat::statFile(pathname,stat)) continue;

      if (stat.isFile()) {
        ext = FXPath::extension(name);
        if (ext!="ogg" && ext!="opus" && ext!="flac" && ext!="mp3" && ext!="oga" && ext!="mpc" && ext!="wav" && ext!="m4a")
          continue;
        }

      if (stat.isDirectory())
        flags|=GMLocalTrackItem::FOLDER;

      GMLocalTrackItem * item = new GMLocalTrackItem(id++,name,flags);

      files.append(name);

      tracklist->appendItem(item);
      }
    }
  return true;
  }


class GMFileListClipboardData : public GMClipboardData {
public:
  FXStringList files;
public:
  FXbool request(FXDragType target,GMClipboard * clipboard) {
    if (target==GMClipboard::urilistType){
      FXString uri;
      gm_convert_filenames_to_uri(files,uri);
      clipboard->setDNDData(FROM_CLIPBOARD,target,uri);
      return true;
      }
    else if (target==GMClipboard::kdeclipboard){
      clipboard->setDNDData(FROM_CLIPBOARD,target,"0");
      return true;
      }
    else if (target==GMClipboard::gnomeclipboard){
      FXString clipdata;
      gm_convert_filenames_to_gnomeclipboard(files,clipdata);
      clipboard->setDNDData(FROM_CLIPBOARD,target,clipdata);
      return true;
      }
    return false;
    }
  ~GMFileListClipboardData() {
    }
  };




long GMLocalSource::onCmdCopyTrack(FXObject*,FXSelector,void*){
  FXDragType types[3]={GMClipboard::kdeclipboard,GMClipboard::gnomeclipboard,FXWindow::urilistType};
  GMFileListClipboardData * data = new GMFileListClipboardData;
  if (GMClipboard::instance()->acquire(this,types,3,data)){
    FXApp::instance()->beginWaitCursor();
    GMTrackView * view = GMPlayerManager::instance()->getTrackView();
    for (FXint i=0;i<view->getNumTracks();i++){
      if (view->isTrackItemSelected(i)) {
        data->files.append(path+PATHSEPSTRING+((GMLocalTrackItem*)view->getTrackItem(i))->getFilename());
        fxmessage("%s\n",data->files[data->files.no()-1].text());
        }
      }
    FXApp::instance()->endWaitCursor();
    }
  else {
    delete data;
    FXApp::instance()->beep();
    }
  return 1;
  }



long GMLocalSource::onCmdRequestTrack(FXObject*sender,FXSelector,void*ptr){
  FXEvent* event=(FXEvent*)ptr;
  FXWindow*window=(FXWindow*)sender;
  if(event->target==GMClipboard::urilistType){
    FXStringList filenames;
    FXIntList tracks;
    FXString uri;
 //   GMPlayerManager::instance()->getTrackView()->getSelectedTracks(tracks);
    GMTrackView * view = GMPlayerManager::instance()->getTrackView();

    for (FXint i=0;i<view->getNumTracks();i++){
      if (view->isTrackItemSelected(i)) {
        filenames.append(path+PATHSEPSTRING+((GMLocalTrackItem*)view->getTrackItem(i))->getFilename());
        fxmessage("%s\n",filenames[filenames.no()-1].text());
        }
      }
    gm_convert_filenames_to_uri(filenames,uri);
    window->setDNDData(FROM_DRAGNDROP,event->target,uri);
    return 1;
    }
  else if (event->target==GMClipboard::kdeclipboard){
    window->setDNDData(FROM_DRAGNDROP,event->target,"0"); // copy
    return 1;
    }
  return 0;
  }



