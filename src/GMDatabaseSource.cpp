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
#include "gmdefs.h"
#include "gmutils.h"
#include "GMTrack.h"
#include "GMApp.h"
#include "GMList.h"
#include "GMTag.h"
#include "GMDatabase.h"
#include "GMTrackDatabase.h"
#include "GMCover.h"
#include "GMCoverCache.h"
#include "GMAlbumList.h"
#include "GMTrackList.h"
#include "GMTrackItem.h"
#include "GMTrackView.h"
#include "GMSource.h"
#include "GMSourceView.h"
#include "GMClipboard.h"
#include "GMDatabaseSource.h"
#include "GMPlayListSource.h"
#include "GMPlayQueue.h"

#include "GMTaskManager.h"
#include "GMPlayerManager.h"
#include "GMWindow.h"
#include "GMIconTheme.h"
#include "GMFilename.h"
#include "GMImportDialog.h"
#include "GMAudioScrobbler.h"
#include "GMAudioPlayer.h"
#include "GMTrackEditor.h"
#include "GMScanner.h"
#include "GMCoverLoader.h"

#include "GMFilter.h"
#include "GMFilterSource.h"

#include <sqlite3.h>


FXbool GMDatabaseClipboardData::request(FXDragType target,GMClipboard * clipboard){
  if (target==GMClipboard::urilistType){
    FXString uri;
    FXStringList filenames;
    db->getTrackFilenames(tracks,filenames);
    gm_convert_filenames_to_uri(filenames,uri);
    clipboard->setDNDData(FROM_CLIPBOARD,target,uri);
    return true;
    }
  else if (target==GMClipboard::kdeclipboard){
    clipboard->setDNDData(FROM_CLIPBOARD,target,"0");
    return true;
    }
  else if (target==GMClipboard::gnomeclipboard){
    FXString clipdata;
    FXStringList filenames;
    db->getTrackFilenames(tracks,filenames);
    gm_convert_filenames_to_gnomeclipboard(filenames,clipdata);
    clipboard->setDNDData(FROM_CLIPBOARD,target,clipdata);
    return true;
    }
  return false;
  }



FXDEFMAP(GMDatabaseSource) GMDatabaseSourceMap[]={

  FXMAPFUNC(SEL_QUERY_TIP,0,GMDatabaseSource::onQueryTip),

//  FXMAPFUNC(SEL_COMMAND,GMDatabaseSource::ID_EDIT_GENRE,GMDatabaseSource::onCmdEditGenre),
//  FXMAPFUNC(SEL_COMMAND,GMDatabaseSource::ID_EDIT_ARTIST,GMDatabaseSource::onCmdEditArtist),
//  FXMAPFUNC(SEL_COMMAND,GMDatabaseSource::ID_EDIT_ALBUM,GMDatabaseSource::onCmdEditAlbum),
  FXMAPFUNCS(SEL_COMMAND,GMDatabaseSource::ID_SEARCH_COVER,GMDatabaseSource::ID_SEARCH_COVER_ALBUM,GMDatabaseSource::onCmdSearchCover),

  FXMAPFUNC(SEL_COMMAND,GMDatabaseSource::ID_EDIT_TRACK,GMDatabaseSource::onCmdEditTrack),
  FXMAPFUNC(SEL_COMMAND,GMDatabaseSource::ID_EDIT_RATING,GMDatabaseSource::onCmdEditRating),
  //FXMAPFUNC(SEL_COMMAND,GMDatabaseSource::ID_DELETE_TAG,GMDatabaseSource::onCmdDelete),
  FXMAPFUNC(SEL_COMMAND,GMDatabaseSource::ID_DELETE_ARTIST,GMDatabaseSource::onCmdDelete),
  FXMAPFUNC(SEL_COMMAND,GMDatabaseSource::ID_DELETE_ALBUM,GMDatabaseSource::onCmdDelete),
  FXMAPFUNC(SEL_COMMAND,GMDatabaseSource::ID_DELETE_TRACK,GMDatabaseSource::onCmdDelete),

  FXMAPFUNC(SEL_COMMAND,GMDatabaseSource::ID_EXPORT_GENRE,GMDatabaseSource::onCmdExportTracks),
  FXMAPFUNC(SEL_COMMAND,GMDatabaseSource::ID_EXPORT_ARTIST,GMDatabaseSource::onCmdExportTracks),
  FXMAPFUNC(SEL_COMMAND,GMDatabaseSource::ID_EXPORT_ALBUM,GMDatabaseSource::onCmdExportTracks),
  FXMAPFUNC(SEL_COMMAND,GMDatabaseSource::ID_EXPORT_TRACK,GMDatabaseSource::onCmdExportTracks),

  FXMAPFUNC(SEL_COMMAND,GMDatabaseSource::ID_COPY_ARTIST,GMDatabaseSource::onCmdCopyArtistAlbum),
  FXMAPFUNC(SEL_COMMAND,GMDatabaseSource::ID_COPY_ALBUM,GMDatabaseSource::onCmdCopyArtistAlbum),
  FXMAPFUNC(SEL_COMMAND,GMDatabaseSource::ID_COPY_TRACK,GMDatabaseSource::onCmdCopyTrack),
  FXMAPFUNC(SEL_DND_REQUEST,GMDatabaseSource::ID_COPY_ARTIST,GMDatabaseSource::onCmdRequestArtistAlbum),
  FXMAPFUNC(SEL_DND_REQUEST,GMDatabaseSource::ID_COPY_ALBUM,GMDatabaseSource::onCmdRequestArtistAlbum),
  FXMAPFUNC(SEL_DND_REQUEST,GMDatabaseSource::ID_COPY_TRACK,GMDatabaseSource::onCmdRequestTrack),
  FXMAPFUNC(SEL_COMMAND,GMDatabaseSource::ID_EXPORT,GMDatabaseSource::onCmdExport),
  FXMAPFUNC(SEL_UPDATE,GMDatabaseSource::ID_EXPORT,GMDatabaseSource::onUpdExport),

  FXMAPFUNC(SEL_COMMAND,GMDatabaseSource::ID_NEW_PLAYLIST,GMDatabaseSource::onCmdNewPlayList),
  FXMAPFUNC(SEL_COMMAND,GMDatabaseSource::ID_IMPORT_PLAYLIST,GMDatabaseSource::onCmdImportPlayList),

  FXMAPFUNC(SEL_COMMAND,GMDatabaseSource::ID_NEW_FILTER,GMDatabaseSource::onCmdNewFilter),

  FXMAPFUNC(SEL_COMMAND,GMDatabaseSource::ID_CLEAR,GMDatabaseSource::onCmdClear),
  FXMAPFUNC(SEL_DND_DROP,GMDatabaseSource::ID_DROP,GMDatabaseSource::onCmdDrop),

  FXMAPFUNC(SEL_COMMAND,GMDatabaseSource::ID_PASTE,GMDatabaseSource::onCmdPaste),
  FXMAPFUNC(SEL_UPDATE,GMDatabaseSource::ID_PASTE,GMDatabaseSource::onUpdPaste),
  FXMAPFUNC(SEL_TIMEOUT,GMDatabaseSource::ID_TRACK_PLAYED,GMDatabaseSource::onCmdTrackPlayed),
  FXMAPFUNC(SEL_COMMAND,GMDatabaseSource::ID_OPEN_FOLDER,GMDatabaseSource::onCmdOpenFolder),
  FXMAPFUNC(SEL_COMMAND,GMDatabaseSource::ID_ADD_COVER,GMDatabaseSource::onCmdAddCover),
  FXMAPFUNC(SEL_TASK_COMPLETED,GMDatabaseSource::ID_LOAD_COVERS,GMDatabaseSource::onCmdLoadCovers),
  FXMAPFUNC(SEL_TASK_CANCELLED,GMDatabaseSource::ID_LOAD_COVERS,GMDatabaseSource::onCmdLoadCovers),
  FXMAPFUNC(SEL_CHORE,GMDatabaseSource::ID_IMPORT_FILES,GMDatabaseSource::onDndImportFiles)
  };

FXIMPLEMENT(GMDatabaseSource,GMSource,GMDatabaseSourceMap,ARRAYNUMBER(GMDatabaseSourceMap));

GMDatabaseSource* GMDatabaseSource::filterowner=NULL;
GMCoverCache* GMDatabaseSource::covercache=NULL;

GMDatabaseSource::GMDatabaseSource(GMTrackDatabase * database) :
  db(database),
  playlist(0),
  filtermask(FILTER_DEFAULT),
  hasfilter(false),
  hasview(false) {
  FXASSERT(db);
  sort_browse=GMDBTrackItem::browseSort;
  }

GMDatabaseSource::~GMDatabaseSource() {
  }

void GMDatabaseSource::shutdown() {
  delete covercache;
  covercache=NULL;
  }

void GMDatabaseSource::loadCovers() {
  if (covercache==NULL) {
    covercache = new GMCoverCache("albumcovers",GMPlayerManager::instance()->getPreferences().gui_coverdisplay_size);
    if (!covercache->load()) {
      updateCovers();
      }
    }
  else if (covercache->getSize()!=GMPlayerManager::instance()->getPreferences().gui_coverdisplay_size){
    updateCovers();
    }
  }

void GMDatabaseSource::updateCovers() {
  if (covercache) {
    GMCoverPathList list;
    if (db->listAlbumPaths(list)) {
      GMCoverLoader * loader = new GMCoverLoader(covercache->getTempFilename(),list,GMPlayerManager::instance()->getPreferences().gui_coverdisplay_size,GMPlayerManager::instance()->getDatabaseSource(),ID_LOAD_COVERS);
      GMPlayerManager::instance()->runTask(loader);
      }
    }
  }


long GMDatabaseSource::onCmdLoadCovers(FXObject*,FXSelector sel,void*ptr) {
  GMCoverLoader * loader = *static_cast<GMCoverLoader**>(ptr);
  if (FXSELTYPE(sel)==SEL_TASK_COMPLETED) {
    covercache->load(loader->getCacheWriter());
    GMPlayerManager::instance()->getTrackView()->redrawAlbumList();
    }
  delete loader;
  return 0;
  }








#include <stdlib.h> // need this for rand()

//generates a psuedo-random integer between min and max
int randint(int min, int max,unsigned int * random_seed)
{
  return min+int( ((double)(max-min+1))*rand_r(random_seed)/(RAND_MAX+1.0));
}


void GMDatabaseSource::shuffle(GMTrackList*list,FXuint sort_seed) const {
  list->setSortFunc(GMDBTrackItem::ascendingAlbum);
  list->sortItems();
  list->setSortFunc(NULL);

  /// Initial Value comes from sort_seed (read from registry and such...)
  FXuint random_seed = sort_seed;
  rand_r(&random_seed);

  FXint n;
  FXint nitems=list->getNumItems()-1;
  for (FXint i=0;i<nitems;i++){
    n=randint(i,nitems,&random_seed);
    FXASSERT(n<list->getNumItems());
    list->moveItem(i,n);
    }
  }



void GMDatabaseSource::configure(GMColumnList& list) {
  list.no(17);
  FXint i=0;
  list[i++]=GMColumn(notr("No"),HEADER_TRACK,GMDBTrackItem::ascendingTrack,GMDBTrackItem::descendingTrack,43,(!playlist) ,true,0);
  list[i++]=GMColumn(notr("Queue"),HEADER_QUEUE,GMDBTrackItem::ascendingQueue,GMDBTrackItem::descendingQueue,60,(playlist),false,1);
  list[i++]=GMColumn(notr("Artist"),HEADER_ARTIST,GMDBTrackItem::ascendingArtist,GMDBTrackItem::descendingArtist,400,true,true,3);
  list[i++]=GMColumn(notr("Title"),HEADER_TITLE,GMDBTrackItem::ascendingTitle,GMDBTrackItem::descendingTitle,360,true,true,2);
  list[i++]=GMColumn(notr("Album Artist"),HEADER_ALBUM_ARTIST,GMDBTrackItem::ascendingAlbumArtist,GMDBTrackItem::descendingAlbumArtist,200,true,false,4);
  list[i++]=GMColumn(notr("Album"),HEADER_ALBUM,GMDBTrackItem::ascendingAlbum,GMDBTrackItem::descendingAlbum,200,true,false,5);
  list[i++]=GMColumn(notr("Disc"),HEADER_DISC,GMDBTrackItem::ascendingDisc,GMDBTrackItem::descendingDisc,43,false,false,6);
//  list[i++]=GMColumn(notr("Tags"),HEADER_TAG,GMDBTrackItem::ascendingTrack,GMDBTrackItem::descendingTrack,200,true,false,7);
  list[i++]=GMColumn(notr("Year"),HEADER_YEAR,GMDBTrackItem::ascendingYear,GMDBTrackItem::descendingYear,60,true,false,8);
  list[i++]=GMColumn(notr("Time"),HEADER_TIME,GMDBTrackItem::ascendingTime,GMDBTrackItem::descendingTime,60,true,true,9);
  list[i++]=GMColumn(notr("Play Count"),HEADER_PLAYCOUNT,GMDBTrackItem::ascendingPlaycount,GMDBTrackItem::descendingPlaycount,60,false,false,10);
  list[i++]=GMColumn(notr("Play Date"),HEADER_PLAYDATE,GMDBTrackItem::ascendingPlaydate,GMDBTrackItem::descendingPlaydate,60,false,false,11);
  list[i++]=GMColumn(notr("Path"),HEADER_FILENAME,GMDBTrackItem::ascendingFilename,GMDBTrackItem::descendingFilename,400,false,false,12);
  list[i++]=GMColumn(notr("File Type"),HEADER_FILETYPE,GMDBTrackItem::ascendingFiletype,GMDBTrackItem::descendingFiletype,30,false,false,13);
  list[i++]=GMColumn(notr("Format"),HEADER_AUDIOFORMAT,GMDBTrackItem::ascendingFormat,GMDBTrackItem::descendingFormat,400,false,false,14);
  list[i++]=GMColumn(notr("Composer"),HEADER_COMPOSER,GMDBTrackItem::ascendingComposer,GMDBTrackItem::descendingComposer,30,false,false,15);
  list[i++]=GMColumn(notr("Conductor"),HEADER_CONDUCTOR,GMDBTrackItem::ascendingConductor,GMDBTrackItem::descendingConductor,400,false,false,16);
  list[i++]=GMColumn(notr("Rating"),HEADER_RATING,GMDBTrackItem::ascendingRating,GMDBTrackItem::descendingRating,30,false,false,17,this,ID_EDIT_RATING);
  }


FXbool GMDatabaseSource::hasCurrentTrack(GMSource * src) const {
  if (src==this || (src->getType()>=SOURCE_PLAYQUEUE && src->getType()<=SOURCE_DATABASE_PLAYLIST)) return true;
  return false;
  }

FXbool GMDatabaseSource::findCurrent(GMTrackList * list,GMSource * src) {
  if (src && src->getType()>=SOURCE_PLAYQUEUE && src->getType()<=SOURCE_DATABASE_PLAYLIST && src->getCurrentTrack()!=-1)
    return GMSource::findCurrent(list,src);
  return false;
  }


FXbool GMDatabaseSource::findCurrentArtist(GMList * list,GMSource * src) {
  GMDatabaseSource * dbs = dynamic_cast<GMDatabaseSource*>(src);
  if (dbs && dbs->getCurrentTrack()!=-1 ){
    FXint artist,album;
    db->getTrackAssociation(dbs->getCurrentTrack(),artist,album);
    if (artist==-1) return false;
    for (FXint i=0;i<list->getNumItems();i++){
      if (artist==(FXint)(FXival)list->getItemData(i)){
        list->selectItem(i);
        list->makeItemVisible(i);
        return true;
        }
      }
    }
  return false;
  }


FXbool GMDatabaseSource::findCurrentAlbum(GMAlbumList * list,GMSource * src) {
  GMDatabaseSource * dbs = dynamic_cast<GMDatabaseSource*>(src);
  if (dbs && dbs->getCurrentTrack()!=-1 ){
    FXint artist=-1,album=-1,i;
    GMAlbumListItem * item=NULL;
    db->getTrackAssociation(dbs->getCurrentTrack(),artist,album);
    if (album==-1) return false;
    for (i=0;i<list->getNumItems();i++){
      item = list->getItem(i);
      if (!item) continue;
      if (album==item->getId()){
        list->selectItem(i);
        list->makeItemVisible(i);
        return true;
        }
      }
    }
  return false;
  }



FXbool GMDatabaseSource::hasTrack(const FXString & mrl,FXint & id) {
  FXint pid = db->hasPath(FXPath::directory(mrl));
  id = 0;
  if (pid) {
    id = db->hasTrack(mrl,pid);
    }
  return id;
  }

FXint GMDatabaseSource::getNumTracks() const{
  return db->getNumTracks();
  }

//FXString GMDatabaseSource::getTrackFilename(FXint id) const{
//  return db->getTrackFilename(id);
//  }

FXbool GMDatabaseSource::getTrack(GMTrack & info) const{
  return db->getTrack(current_track,info);
  }

FXbool GMDatabaseSource::genre_context_menu(FXMenuPane * /*pane*/) {
  //new GMMenuCommand(pane,fxtr("Edit…\tF2\tEdit Genre."),GMIconTheme::instance()->icon_edit,this,GMDatabaseSource::ID_EDIT_GENRE);
//  new GMMenuCommand(pane,"Export" … "\t\tCopy associated tracks to destination.",GMIconTheme::instance()->icon_export,this,ID_EXPORT_GENRE);
//  new FXMenuSeparator(pane);
  //new GMMenuCommand(pane,fxtr("Remove…\tDel\tRemove Tag from Library."),GMIconTheme::instance()->icon_delete,this,GMSource::ID_DELETE_TAG);
  return false;
  }

FXbool GMDatabaseSource::artist_context_menu(FXMenuPane * pane){
  new GMMenuCommand(pane,fxtr("Copy\tCtrl-C\tCopy associated tracks to the clipboard."),GMIconTheme::instance()->icon_copy,this,ID_COPY_ARTIST);
  new FXMenuSeparator(pane);
  new GMMenuCommand(pane,fxtr("Remove…\tDel\tRemove associated tracks from library."),GMIconTheme::instance()->icon_delete,this,GMSource::ID_DELETE_ARTIST);
  return true;
  }

FXbool GMDatabaseSource::album_context_menu(FXMenuPane * pane){
  new GMMenuCommand(pane,fxtr("Copy\tCtrl-C\tCopy associated tracks to the clipboard."),GMIconTheme::instance()->icon_copy,this,ID_COPY_ALBUM);
  new GMMenuCommand(pane,fxtr("Find Cover…\t\tFind Cover with Google Image Search"),NULL,this,ID_SEARCH_COVER_ALBUM);
  new FXMenuSeparator(pane);
  new GMMenuCommand(pane,fxtr("Remove…\tDel\tRemove associated tracks from library."),GMIconTheme::instance()->icon_delete,this,GMSource::ID_DELETE_ALBUM);
  return true;
  }

FXbool GMDatabaseSource::track_context_menu(FXMenuPane * pane){
  new GMMenuCommand(pane,fxtr("Edit…\tF2\tEdit Track Information."),GMIconTheme::instance()->icon_edit,this,GMDatabaseSource::ID_EDIT_TRACK);
  new GMMenuCommand(pane,fxtr("Set Cover…\t\t"),NULL,this,GMDatabaseSource::ID_ADD_COVER);

  new GMMenuCommand(pane,fxtr("Copy\tCtrl-C\tCopy track(s) to clipboard."),GMIconTheme::instance()->icon_copy,this,ID_COPY_TRACK);
#ifdef DEBUG
  new GMMenuCommand(pane,"Export\t\tCopy tracks to destination.",GMIconTheme::instance()->icon_export,this,ID_EXPORT_TRACK);
#endif
  new FXMenuSeparator(pane);

  if (GMPlayerManager::instance()->getTrackView()->numTrackSelected()==1){
    new GMMenuCommand(pane,"Open Folder Location\t\tOpen Folder Location.",NULL,this,ID_OPEN_FOLDER);
    new GMMenuCommand(pane,fxtr("Find Cover…\t\tFind Cover with Google Image Search"),NULL,this,ID_SEARCH_COVER);
    new FXMenuSeparator(pane);
    }

  new GMMenuCommand(pane,fxtr("Remove…\tDel\tRemove track(s) from library."),GMIconTheme::instance()->icon_delete,this,GMSource::ID_DELETE_TRACK);

/*
    if (getCurrentSourceType()==SOURCE_PLAYLIST && !browserframe->shown()) {
      new GMMenuCommand(&pane,"Remove from Playlist\t\tRemove track(s) from playlist.",icon_delete,this,ID_PLAYLIST_DEL_TRACK);
      new FXMenuSeparator(&pane);
      new GMMenuCommand(&pane,tr("Reorder Playlist"),NULL,this,ID_REORDER_PLAYLIST);
      }
*/
  return true;
  }

FXbool GMDatabaseSource::source_context_menu(FXMenuPane * pane){
//  new GMMenuCommand(pane,fxtr("Import Folder…\tCtrl-O\tImport Music from folder into Library"),GMIconTheme::instance()->icon_import,GMPlayerManager::instance()->getMainWindow(),GMWindow::ID_IMPORT_DIRS);
//  new GMMenuCommand(pane,fxtr("New Play List…\t\tCreate a new play list."),GMIconTheme::instance()->icon_playlist,this,GMDatabaseSource::ID_NEW_PLAYLIST);
 // new FXMenuSeparator(pane);
  new GMMenuCommand(pane,fxtr("Export As…"),GMIconTheme::instance()->icon_export,this,GMDatabaseSource::ID_EXPORT);
 // new FXMenuSeparator(pane);
  new GMMenuCommand(pane,fxtr("Remove All Tracks\t\tRemove all tracks from the library"),GMIconTheme::instance()->icon_delete,this,GMDatabaseSource::ID_CLEAR);
  return true;
  }


FXbool GMDatabaseSource::source_menu(FXMenuPane * pane){
  new GMMenuCommand(pane,fxtr("Import Folder…\tCtrl-O\tImport Music from folder into Library"),GMIconTheme::instance()->icon_import,GMPlayerManager::instance()->getMainWindow(),GMWindow::ID_IMPORT_DIRS);
  new GMMenuCommand(pane,fxtr("Sync Folder…\t\tSynchronize Folder with Music in Library"),GMIconTheme::instance()->icon_sync,GMPlayerManager::instance()->getMainWindow(),GMWindow::ID_SYNC_DIRS);
  new FXMenuSeparator(pane);
  new GMMenuCommand(pane,fxtr("Play File or Stream…\t\tPlay File or Stream"),NULL,GMPlayerManager::instance()->getMainWindow(),GMWindow::ID_OPEN);
  new FXMenuSeparator(pane);
  new GMMenuCommand(pane,fxtr("New Filter…\t\tCreate a new filter"),NULL,this,GMDatabaseSource::ID_NEW_FILTER);
  new GMMenuCommand(pane,fxtr("New Playlist…\t\tCreate a new playlist"),NULL,this,GMDatabaseSource::ID_NEW_PLAYLIST);
  new GMMenuCommand(pane,fxtr("Import Playlist…\t\tImport existing playlist"),GMIconTheme::instance()->icon_import,this,GMDatabaseSource::ID_IMPORT_PLAYLIST);
  return true;
  }

FXuint GMDatabaseSource::dnd_provides(FXDragType types[]){
  types[0]=GMClipboard::kdeclipboard;
  types[1]=GMClipboard::urilistType;
  types[2]=GMClipboard::selectedtracks;
  return 3;
  }

FXbool GMDatabaseSource::dnd_accepts(FXDragType*types,FXuint ntypes){
  FXbool check=false;
  for (FXuint i=0;i<ntypes;i++){
    if (types[i]==GMClipboard::trackdatabase) return false;
    else if (types[i]==GMClipboard::alltracks) return false;
    else if (types[i]==GMClipboard::selectedtracks) return false;
    else if (types[i]==GMClipboard::kdeclipboard) check=true;
    else if (types[i]==FXWindow::urilistType) check=true;
    }
  return check;
  }




FXbool GMDatabaseSource::setFilter(const FXString & text,FXuint mask){

  // don't do anything if nothing changed
  if (filtermask==mask && filter==text && filterowner==this)
    return false;

  filter=text;
  filtermask=mask;


  FXString query,keyword_query,match_query;
  FXPtrListOf<FXchar> keywords;
  if (filtermask) {

    // get search words from string
    FXString word;
    FXbool quotes=false;
    FXint i;
    for (i=0;i<text.length();i++){
      if (text[i]=='\\' && (i+1)<text.length() && text[i+1]=='\"'){
        word+=text[i+1];
        i++;
        }
      else if (text[i]=='\"') {
        quotes=!quotes;
        }
      else if (Ascii::isSpace(text[i]) && !quotes) {
        if (!word.empty()) {
          FXchar * sf = sqlite3_mprintf("LIKE '%%%q%%'",word.text());
          keywords.append(sf);
          word.clear();
          }
        }
      else {
        word+=text[i];
        }
      }

    if (!word.empty()) {
      FXchar * sf = sqlite3_mprintf("LIKE '%%%q%%'",word.text());
      keywords.append(sf);
      word=FXString::null;
      }

    }

  db->execute("DROP VIEW IF EXISTS filtered;");

  if (keywords.no() && filtermask) {

    query = "CREATE TEMP VIEW filtered AS SELECT tracks.id as track, tracks.album as album FROM tracks JOIN albums ON tracks.album == albums.id JOIN artists AS album_artist ON (albums.artist == album_artist.id) JOIN artists AS track_artist ON (tracks.artist == track_artist.id) LEFT JOIN artists AS composers ON (tracks.composer == composers.id) LEFT JOIN artists AS conductors ON (tracks.conductor == conductors.id) WHERE ";
    if (playlist) {
      query+="tracks.id IN (SELECT track FROM playlist_tracks WHERE playlist == " + FXString::value(playlist) + ") AND ";
      }
    for (int i=0;i<keywords.no();i++){
      if (filtermask&FILTER_ARTIST) {
        if (!match_query.empty()) match_query+=" OR ";
        match_query+=FXString::value("(composers.name %s OR conductors.name %s OR track_artist.name %s OR album_artist.name %s)",keywords[i],keywords[i],keywords[i],keywords[i]);
        }
      if (filtermask&FILTER_ALBUM) {
        if (!match_query.empty()) match_query+=" OR ";
        match_query+=FXString::value("(albums.name %s)",keywords[i]);
        }
      if (filtermask&FILTER_TRACK) {
        if (!match_query.empty()) match_query+=" OR ";
        match_query+=FXString::value("(title %s)",keywords[i]);
        }
      if (filtermask&FILTER_TAG) {
        if (!match_query.empty()) match_query+=" OR ";
        match_query+=FXString::value("(track IN (SELECT track FROM track_tags JOIN tags ON track_tags.tag == tags.id WHERE tags.name %s))",keywords[i]);
        }
      if (!keyword_query.empty()) keyword_query+=" AND ";
      keyword_query+="("+match_query+")";
      match_query.clear();
      }
    query+=keyword_query;

    //fxmessage("q: %s\n",query.text());
    GM_TICKS_START();
    db->execute(query);
    GM_TICKS_END();
    hasfilter=true;
    }
  else {
    hasfilter=false;
    }

  for (FXint i=0;i<keywords.no();i++){
    sqlite3_free(keywords[i]);
    }

  filterowner=this;
  return true;
  }


FXbool GMDatabaseSource::listTags(GMList * list,FXIcon * icon) {
  FXint id;
  GMQuery q;
  FXString query;

  GM_TICKS_START();
  try {
    if (hasFilter() || hasview) {
      if (hasview && hasFilter())
        query = "SELECT DISTINCT(id),name FROM tags WHERE id IN (SELECT tag FROM track_tags WHERE track IN (SELECT track FROM query_view) AND track in (SELECT track FROM filtered));";    
      else if (hasFilter())
        query = "SELECT DISTINCT(id),name FROM tags WHERE id IN (SELECT tag FROM track_tags WHERE track IN (SELECT track FROM filtered));";
      else
        query = "SELECT DISTINCT(id),name FROM tags WHERE id IN (SELECT tag FROM track_tags WHERE track IN (SELECT track FROM query_view));";





      }
    else {
      if (playlist)
        query = "SELECT DISTINCT(tags.id),tags.name from tags WHERE id IN (SELECT tag FROM playlist_tracks JOIN track_tags ON (playlist_tracks.playlist == "  + FXString::value(playlist) +" AND track_tags.track == playlist_tracks.track));";
      else
        query = "SELECT DISTINCT(id),name FROM tags WHERE id IN (SELECT tag FROM track_tags);";
      }
    q = db->compile(query);
    while(q.row()){
      q.get(0,id);
      list->appendItem(q.get(1),icon,(void*)(FXival)id);
      }
    }
  catch(GMDatabaseException & e){
    list->clearItems();
    return false;
    }
  GM_TICKS_END();
  return true;
  }


#if 0

FXbool GMDatabaseSource::listComposers(GMList * list,FXIcon * icon,const FXIntList & genrelist){
   GMListItem * item;
  const FXchar * name;
  FXint id;
  GMQuery q;
  FXString genreselection;
  FXString query;
  FXString filterquery;

  gm_query_make_selection(genrelist,genreselection);


  GM_TICKS_START();

  try {
    if (!hasFilter()){
      if (genrelist.no()==0) {
        if (!playlist) {
          query = "SELECT id,name FROM artists WHERE id IN (SELECT DISTINCT(composer) FROM tracks);";
          }
        else{
          query = "SELECT DISTINCT(artists.id), artists.name "
                  "FROM artists "
                  "WHERE id IN ( "
                    "SELECT DISTINCT(composer) FROM tracks WHERE id IN ( "
                      "SELECT DISTINCT(track) FROM playlist_tracks WHERE playlist == " + FXString::value(playlist) + "));";
          }
        }
      else {
        if (!playlist)
          query = "SELECT id,name FROM artists WHERE id IN (SELECT DISTINCT(composer) FROM tracks WHERE genre " + genreselection + ");";
        else
          query = "SELECT id,name FROM artists WHERE id IN (SELECT DISTINCT(composer) FROM tracks WHERE genre " + genreselection + " AND id IN ( SELECT DISTINCT(track) FROM playlist_tracks WHERE playlist == " + FXString::value(playlist) + "));";
        }
      }
    else {
      query = "SELECT artists.id, artists.name FROM artists WHERE id IN (SELECT artist FROM filtered ";
      if (genrelist.no()) {
        query+=" WHERE genre " + genreselection;
        }
      query+=");";
      }

    q = db->compile(query);
    while(q.row()){
      q.get(0,id);
      name=q.get(1);
      item = new GMListItem(name,icon,(void*)(FXival)id);
      item->setDraggable(true);
      list->appendItem(item);
      }
    }
  catch(GMDatabaseException & e){
    list->clearItems();
    return false;
    }
  GM_TICKS_END();
  return true;
  }

#endif





FXbool GMDatabaseSource::listArtists(GMList * list,FXIcon * icon,const FXIntList & taglist){
  GMListItem * item;
  const FXchar * name;
  FXint id;
  GMQuery q;
  FXString tagselection;
  FXString query;
  FXString filterquery;

  GMQuery::makeSelection(taglist,tagselection);


  GM_TICKS_START();
  try {
    if (hasFilter() || hasview) {
      query = "SELECT id,name FROM artists WHERE id IN (SELECT DISTINCT(artist) FROM albums WHERE";

      if (hasview) {
        query += " id IN (SELECT DISTINCT(album) FROM query_view";
        if (taglist.no()) {
          query += " WHERE track IN ( SELECT track FROM track_tags WHERE tag " + tagselection + ")";
          }
        query+=")";
        }

      if (hasFilter()) {
        if (hasview) query+=" AND";
        query += " id IN (SELECT DISTINCT(album) FROM filtered";
        if (taglist.no()) {
          query += " WHERE track IN ( SELECT track FROM track_tags WHERE tag " + tagselection + ")";
          }
        query+=")";
        }
      query+=");";
      }
    else {
      if (taglist.no()==0) {
        if (!playlist)
          query = "SELECT id,name FROM artists WHERE id IN (SELECT DISTINCT(artist) FROM albums);";
        else
          query = "SELECT DISTINCT(artists.id), artists.name FROM albums JOIN artists ON artists.id == albums.artist AND albums.id IN (SELECT DISTINCT(album) FROM playlist_tracks JOIN tracks ON playlist_tracks.track == tracks.id AND playlist_tracks.playlist == " + FXString::value(playlist) + " ORDER BY album)";
        }
      else {
        if (!playlist) {
          query = "SELECT DISTINCT(id), name FROM artists "
                    "WHERE id IN ( "
                      "SELECT artist "
                      "FROM albums WHERE id IN ( "
                        "SELECT DISTINCT(album) FROM tracks, track_tags WHERE tracks.id == track_tags.track AND tag";
          query+=tagselection + "));";
          }
        else {
          query = "SELECT DISTINCT(artists.id),artists.name FROM artists WHERE id IN (SELECT DISTINCT(artist) FROM albums WHERE id IN (SELECT DISTINCT(album) from tracks WHERE id IN (SELECT track FROM playlist_tracks WHERE playlist == "+ FXString::value(playlist) +" INTERSECT SELECT track from track_tags WHERE tag "+ tagselection +" )));";
          }
        }
      }
    q = db->compile(query);
    while(q.row()){
      q.get(0,id);
      name=q.get(1);
      item = new GMListItem(name,icon,(void*)(FXival)id);
      item->setDraggable(true);
      list->appendItem(item);
      }
    }
  catch(GMDatabaseException & e){
    list->clearItems();
    return false;
    }
  GM_TICKS_END();
  return true;
  }

FXbool GMDatabaseSource::listAlbums(GMAlbumList * list,const FXIntList & artistlist,const FXIntList & taglist){
  const FXchar * c_name=NULL;
  FXint id;
  FXint year;
  FXint audio_channels;
  FXint audio_rate;
  FXint audio_format;
  FXint artist;
  FXString query;

  FXString tagselection;
  FXString artistselection;

  GMQuery::makeSelection(taglist,tagselection);
  GMQuery::makeSelection(artistlist,artistselection);

  GM_TICKS_START();

  GMAlbumListItem * item=NULL;
  GMQuery q;
  try {
    if (hasFilter() || hasview){
      query = "SELECT albums.id,albums.name,albums.year,artists.id,albums.audio_channels,albums.audio_rate,albums.audio_format FROM albums,artists WHERE artists.id == albums.artist";

      if (hasview) {
        query += " AND albums.id IN (SELECT album FROM query_view";
        if (taglist.no()) {
          query+=" JOIN track_tags ON track_tags.track == query_view.track WHERE tag " + tagselection;
          }
        query+=" )";
        }

      if (hasFilter()) {
        query += " AND albums.id IN (SELECT album FROM filtered";
        if (taglist.no()) {
          query+=" JOIN track_tags ON track_tags.track == filtered.track WHERE tag " + tagselection;
          }
        query+=" )";
        }
      if (artistlist.no()) {
        query+=" AND artist " + artistselection;
        }
      query+=" ORDER BY albums.name";
      }
    else {
      if (playlist) {
        if (taglist.no()) {
          query = "SELECT DISTINCT(albums.id),albums.name,albums.year,artists.id,albums.audio_channels,albums.audio_rate,albums.audio_format FROM albums,artists WHERE artists.id == albums.artist AND albums.id IN (SELECT album FROM tracks WHERE id IN (SELECT track FROM playlist_tracks WHERE playlist == "+ FXString::value(playlist) +" INTERSECT SELECT track FROM track_tags WHERE tag " + tagselection + ")) ";
          }
        else {
          query = "SELECT DISTINCT(albums.id),albums.name,albums.year,artists.id,albums.audio_channels,albums.audio_rate,albums.audio_format FROM albums,artists WHERE artists.id == albums.artist AND albums.id IN (SELECT album FROM playlist_tracks JOIN tracks ON playlist_tracks.track == tracks.id AND playlist_tracks.playlist == "+ FXString::value(playlist) +")";
          }
        if (artistlist.no())
          query+=" AND artist " + artistselection;
        }
      else {
        if (taglist.no()) {
          query = "SELECT albums.id,albums.name,albums.year,artists.id,albums.audio_channels,albums.audio_rate,albums.audio_format FROM albums,artists WHERE artists.id == albums.artist AND albums.id IN (SELECT DISTINCT(album) FROM tracks WHERE id IN (SELECT DISTINCT(track) FROM track_tags WHERE tag " + tagselection + ")) ";
          if (artistlist.no())
            query+=" AND albums.artist " + artistselection;
          }
        else {
          query = "SELECT albums.id,albums.name,albums.year,artists.id,albums.audio_channels,albums.audio_rate,albums.audio_format FROM albums,artists";
          if (artistlist.no()) {
            query+=" WHERE artist " + artistselection;
            query+=" AND artists.id == albums.artist";
            }
          else
            query+=" WHERE artists.id == albums.artist ";
          }
        }
      query+=" ORDER BY albums.name;";
      }

    q = db->compile(query);

    while(q.row()){
      q.get(0,id);
      c_name = q.get(1);
      q.get(2,year);
      q.get(3,artist);
      q.get(4,audio_channels);
      q.get(5,audio_rate);
      q.get(6,audio_format);

      FXString property;

      if (audio_channels>2)
        property+=FXString::value("%dch ",audio_channels);
      if (audio_format>16 && audio_rate>44100)
        property+=FXString::value("%d/%d",audio_format,audio_rate/1000);
      else if (audio_rate>44100)
        property+=FXString::value("%dkHz",audio_rate/1000);

      if (artistlist.no()!=1 && c_name!=NULL && item && item->getTitle()==c_name) {
        item->setShowArtist(true);
        item = new GMAlbumListItem(artist,c_name,property,year,id);
        list->appendItem(item);
        item->setShowArtist(true);
        }
      else {
        item = new GMAlbumListItem(artist,c_name,property,year,id);
        list->appendItem(item);
        }
      }
    }
  catch(GMDatabaseException & e){
    list->clearItems();
    return false;
    }


  list->sortItems();
  if (list->getNumItems()>1){
    FXString all = FXString::value(fxtrformat("All %d Albums"),list->getNumItems());
    if (artistlist.no()==1)
      list->prependItem(new GMAlbumListItem(artist,all,0,-1));
    else
      list->prependItem(new GMAlbumListItem(-1,all,0,-1));
    }
  GM_TICKS_END();
  return true;
  }



FXbool GMDatabaseSource::listTracks(GMTrackList * tracklist,const FXIntList & albumlist,const FXIntList & taglist){
  const FXbool browse_mode = GMPlayerManager::instance()->getTrackView()->hasBrowser();
  GMQuery q;
  FXString query;
  const FXchar * c_albumname;
  const FXchar * c_title;
  const FXchar * c_mrl;
  FXint path;
  FXint time;
  FXuint no;
  FXint id;
  FXint queue=1;
  FXint album_year;
  FXint track_year;
  FXint playcount;
  FXlong playdate;
  FXint bitrate,samplerate,channels,filetype;
  FXint rating;

  FXint album,artist,composer,conductor,albumartist;

  FXString tagselection;
  FXString albumselection;

  GMQuery::makeSelection(taglist,tagselection);
  GMQuery::makeSelection(albumlist,albumselection);

  GMDBTrackItem * item;

  GMDBTrackItem::max_queue=0;
  GMDBTrackItem::max_trackno=0;
  GMDBTrackItem::max_time=0;

  GM_TICKS_START();

  try {

    query = "SELECT tracks.id,"
                   "tracks.path, "
                   "tracks.mrl, "
                   "tracks.title, "
                   "tracks.time,"
                   "tracks.no,"
                   "tracks.year,"
                   "tracks.artist, "
                   "tracks.composer, "
                   "tracks.conductor, "
                   "albums.artist,albums.name,albums.year,albums.id, "
                   "tracks.playcount,"
                   "tracks.bitrate,"
                   "tracks.samplerate,"
                   "tracks.channels,"
                   "tracks.filetype,"
                   "tracks.playdate, "
                   "tracks.rating ";

    if (playlist && browse_mode==false)
      query += "FROM playlist_tracks JOIN tracks ON playlist_tracks.playlist == " + FXString::value(playlist) + " AND playlist_tracks.track == tracks.id ";
    else
      query += "FROM tracks ";

    query += "JOIN albums ON tracks.album == albums.id ";

    if (taglist.no()) {
      query+="LEFT OUTER JOIN track_tags ON track_tags.track == tracks.id ";
      query+=" WHERE track_tags.tag " + tagselection;

      if (albumlist.no())
        query+=" AND tracks.album " + albumselection;

      if (hasview)
        query+=" AND tracks.id IN (SELECT track FROM query_view) ";

      if (hasFilter())
        query+=" AND tracks.id IN (SELECT track FROM filtered) ";

      if (playlist && browse_mode)
        query+=" AND tracks.id IN (SELECT track FROM playlist_tracks WHERE playlist ==  " + FXString::value(playlist) + ")";
      }
    else if (albumlist.no()) {
      query+=" WHERE tracks.album " + albumselection;
      if (hasFilter())
        query+=" AND tracks.id IN (SELECT track FROM filtered) ";
      if (hasview)
        query+=" AND tracks.id IN (SELECT track FROM query_view) ";

      if (playlist && browse_mode)
        query+=" AND tracks.id IN (SELECT track FROM playlist_tracks WHERE playlist ==  " + FXString::value(playlist) + ")";
      }
    else if (hasFilter()) {
      if (hasview)
        query+=" WHERE tracks.id IN (SELECT track FROM query_view) AND tracks.id IN (SELECT track FROM filtered)";
      else
        query+=" WHERE tracks.id IN (SELECT track FROM filtered)";

      if (playlist && browse_mode)
        query+=" AND tracks.id IN (SELECT track FROM playlist_tracks WHERE playlist ==  " + FXString::value(playlist) + ")";
      }
    else if (hasview) {
      query+=" AND tracks.id IN (SELECT track FROM query_view) ";
      if (playlist && browse_mode)
        query+=" AND tracks.id IN (SELECT track FROM playlist_tracks WHERE playlist ==  " + FXString::value(playlist) + ")";
      }

    if (playlist && browse_mode==false)
      query+=" ORDER BY playlist_tracks.queue;";
    else
      query+=";";

    q = db->compile(query);

    while(q.row()){
      q.get(0,id);
      q.get(1,path);

      c_mrl   = q.get(2);
      c_title = q.get(3);

      q.get(4,time);
      q.get(5,no);
      q.get(6,track_year);

      q.get(7,artist);
      q.get(8,composer);
      q.get(9,conductor);
      q.get(10,albumartist);

      c_albumname = q.get(11);

      q.get(12,album_year);
      q.get(13,album);
      q.get(14,playcount);
      q.get(15,bitrate);
      q.get(16,samplerate);
      q.get(17,channels);
      q.get(18,filetype);
      q.get(19,playdate);
      q.get(20,rating);

      GMDBTrackItem::max_trackno=FXMAX(GMDBTrackItem::max_digits(GMTRACKNO(no)),GMDBTrackItem::max_trackno);
      GMDBTrackItem::max_queue=FXMAX(GMDBTrackItem::max_digits(queue),GMDBTrackItem::max_queue);

      /// To 0 - 5 stars
      rating/=51;

      if (bitrate<0)
        bitrate *= -(samplerate*channels);

      item = new GMDBTrackItem(id,
                               path,
                               c_mrl,
                               c_title,
                               artist,
                               albumartist,
                               composer,
                               conductor,
                               album,
                               c_albumname,
                               time,
                               no,
                               queue++,
                               (FXushort)track_year,
                               (FXushort)album_year,
                               (FXushort)playcount,
                               filetype,
                               bitrate,
                               samplerate,
                               channels,
                               playdate,
                               (FXushort)rating);

      tracklist->appendItem(item);
      }
    GMDBTrackItem::max_trackno = tracklist->getFont()->getTextWidth(FXString('8',GMDBTrackItem::max_trackno));
    GMDBTrackItem::max_queue   = tracklist->getFont()->getTextWidth(FXString('8',GMDBTrackItem::max_digits(queue)));
    GMDBTrackItem::max_time    = tracklist->getFont()->getTextWidth("88:88",5);
    }
  catch(GMDatabaseException & e){
    tracklist->clearItems();
    return false;
    }
  GM_TICKS_END();
  return true;
  }


FXbool GMDatabaseSource::updateSelectedTracks(GMTrackList*tracklist) {
  FXString query;
  GMQuery  q;
  const FXchar * c_albumname;
  const FXchar * c_title;
  const FXchar * c_mrl;
  FXint path;
  FXint time;
  FXuint no;
  //FXint id;
  //FXint queue=1;
  FXint album_year;
  FXint track_year;
  FXint playcount;
  FXlong playdate;
  FXint bitrate;
  FXint rating;

  FXint artist,composer,conductor,albumartist;


  GM_TICKS_START();

  try {
    query = "SELECT "
                 "tracks.path, "
                 "tracks.mrl, "
                 "tracks.title, "
                 "tracks.time,"
                 "tracks.no,"
                 "tracks.year,"
                 "tracks.artist, "
                 "tracks.composer, "
                 "tracks.conductor, "
                 "albums.artist,albums.name,albums.year,"
                 "tracks.playcount,"
                 "tracks.bitrate,"
                 "tracks.playdate, "
                 "tracks.rating "
           "FROM tracks JOIN albums ON tracks.album == albums.id WHERE tracks.id == ?;";

    q = db->compile(query);

    for (FXint i=0;i<tracklist->getNumItems();i++) {
      if (tracklist->isItemSelected(i)) {
        GMDBTrackItem * item = dynamic_cast<GMDBTrackItem*>(tracklist->getItem(i));
        FXASSERT(item);
        q.set(0,item->id);
        q.row();
        q.get(0,path);
        c_mrl   = q.get(1);
        c_title = q.get(2);
        q.get(3,time);
        q.get(4,no);
        q.get(5,track_year);
        q.get(6,artist);
        q.get(7,composer);
        q.get(8,conductor);
        q.get(9,albumartist);
        c_albumname = q.get(10);
        q.get(11,album_year);
        q.get(12,playcount);
        q.get(13,bitrate);
        q.get(14,playdate);
        q.get(15,rating);

        item->mrl         = c_mrl;
        item->title       = c_title;
        item->album       = c_albumname;
        item->playdate    = playdate;
        item->artist      = artist;
        item->albumartist = albumartist;
        item->composer    = composer;
        item->conductor   = conductor;
        item->time        = time;
        item->no          = no;
        item->path        = path;
        item->year        = track_year;
        item->album_year  = album_year;
        item->playcount   = playcount;
        item->rating      = rating;

        q.reset();
        tracklist->updateItem(i);
        }
      }
    }
  catch(GMDatabaseException & e){
    return false;
    }
  GM_TICKS_END();
  return true;
  }



long GMDatabaseSource::onCmdEditTrack(FXObject*,FXSelector,void*){
  GMEditTrackDialog dialog(GMPlayerManager::instance()->getMainWindow(),db);
  dialog.execute();
  return 1;
  }

long GMDatabaseSource::onCmdEditRating(FXObject*,FXSelector,void* ptr){
  FXuchar rating = (FXuchar)(FXuval)ptr;
  GMDBTrackItem * item = dynamic_cast<GMDBTrackItem*>(GMPlayerManager::instance()->getTrackView()->getCurrentTrackItem());
  FXASSERT(item);
  item->setRating(rating);
  db->setTrackRating(item->getId(),rating*51);
  return 1;
  }



void GMDatabaseSource::removeFiles(const FXStringList & files) {
  FXDialogBox dialog(GMPlayerManager::instance()->getMainWindow(),fxtr("Remove Audio Files?"),DECOR_TITLE|DECOR_BORDER|DECOR_RESIZE|DECOR_CLOSE,0,0,600,400,0,0,0,0,0,0);
  GMPlayerManager::instance()->getMainWindow()->create_dialog_header(&dialog,fxtr("Remove Audio Files..."),fxtr("The following audio files are going to be removed"));
  FXHorizontalFrame *closebox=new FXHorizontalFrame(&dialog,LAYOUT_SIDE_BOTTOM|LAYOUT_FILL_X|PACK_UNIFORM_WIDTH,0,0,0,0);
  new GMButton(closebox,fxtr("&Remove"),NULL,&dialog,FXDialogBox::ID_ACCEPT,BUTTON_INITIAL|BUTTON_DEFAULT|LAYOUT_RIGHT|FRAME_RAISED|FRAME_THICK,0,0,0,0, 15,15);
  new GMButton(closebox,fxtr("&Cancel"),NULL,&dialog,FXDialogBox::ID_CANCEL,BUTTON_DEFAULT|LAYOUT_RIGHT|FRAME_RAISED|FRAME_THICK,0,0,0,0, 15,15);
  new FXSeparator(&dialog,SEPARATOR_GROOVE|LAYOUT_FILL_X|LAYOUT_SIDE_BOTTOM);

  FXVerticalFrame * main = new FXVerticalFrame(&dialog,LAYOUT_FILL_X|LAYOUT_FILL_Y);
  FXVerticalFrame * sunken = new FXVerticalFrame(main,LAYOUT_FILL_X|LAYOUT_FILL_Y|FRAME_SUNKEN|FRAME_THICK,0,0,0,0,0,0,0,0);
  FXList * list = new FXList(sunken,NULL,0,LAYOUT_FILL_X|LAYOUT_FILL_Y);

  for (int i=0;i<files.no();i++) {
    list->appendItem(files[i]);
    }
  if (dialog.execute()) {
    for (int i=0;i<files.no();i++){
      FXFile::remove(files[i]);
      }
    }
  }




long GMDatabaseSource::onCmdExport(FXObject*,FXSelector,void*){
  const FXchar patterns[]="XML Shareable Playlist Format (*.xspf)\nPLS (*.pls)\nExtended M3U (*.m3u)\nM3U (*.m3u)\nText Comma-Separated (*.csv)";
  FXString searchdir = FXApp::instance()->reg().readStringEntry("Settings","last-export-directory",FXSystem::getHomeDirectory().text());
  FXString title;
  FXuint opts=0;

  if (!playlist)
    title=fxtr("Export Main Library");
  else
    title=fxtr("Export Playlist");
  GMExportDialog dialog(GMPlayerManager::instance()->getMainWindow(),title);
  dialog.setDirectory(searchdir);
  dialog.setSelectMode(SELECTFILE_ANY);
  dialog.setPatternList(patterns);
  dialog.setCurrentPattern(0);
  dialog.setMatchMode(FXPath::CaseFold);
  dialog.setRelativePath(FXApp::instance()->reg().readBoolEntry("Settings","export-relative-paths",false));

  if (dialog.execute()){
    if (FXStat::exists(dialog.getFilename())){
      if (FXMessageBox::question(GMPlayerManager::instance()->getMainWindow(),MBOX_YES_NO,fxtr("Overwrite File?"),fxtr("File already exists. Would you like to overwrite it?"))!=MBOX_CLICKED_YES)
        return 1;
      }

    if (dialog.getRelativePath())
      opts|=PLAYLIST_OPTIONS_RELATIVE;

    FXApp::instance()->reg().writeStringEntry("Settings","last-export-directory",dialog.getDirectory().text());
    FXApp::instance()->reg().writeBoolEntry("Settings","export-relative-paths",dialog.getRelativePath());

    FXApp::instance()->beginWaitCursor();
    switch(dialog.getCurrentPattern()){
      case 0: db->exportList(dialog.getFilename(),playlist,PLAYLIST_XSPF,opts); break;
      case 1: db->exportList(dialog.getFilename(),playlist,PLAYLIST_PLS,opts); break;
      case 2: db->exportList(dialog.getFilename(),playlist,PLAYLIST_M3U_EXTENDED,opts); break;
      case 3: db->exportList(dialog.getFilename(),playlist,PLAYLIST_M3U,opts); break;
      case 4: db->exportList(dialog.getFilename(),playlist,PLAYLIST_CSV,opts); break;
      }
    FXApp::instance()->endWaitCursor();
    }
  return 1;
  }

long GMDatabaseSource::onUpdExport(FXObject*sender,FXSelector,void*){
  sender->handle(this,FXSEL(SEL_COMMAND,FXWindow::ID_ENABLE),NULL);
  return 1;
  }



long GMDatabaseSource::onCmdExportTracks(FXObject*,FXSelector sel,void*){
  const FXuint labelstyle=LAYOUT_CENTER_Y|LABEL_NORMAL|LAYOUT_RIGHT;

  FXIntList tracks;
  if (FXSELID(sel)==ID_EXPORT_TRACK) {
    GMPlayerManager::instance()->getTrackView()->getSelectedTracks(tracks);
    }
  else {
    GMPlayerManager::instance()->getTrackView()->getTracks(tracks);
    }
  if (tracks.no()==0) return 1;

  FXString title;
  FXString subtitle;

  switch(FXSELID(sel)){
    case ID_EXPORT_GENRE: title=fxtr("Export Genre");
                          subtitle=fxtr("Export tracks with genre to destination directory.");
                          break;
    case ID_EXPORT_ARTIST:title=fxtr("Export Artists");
                          subtitle=fxtr("Export tracks from artist to destination directory.");
                          break;
    case ID_EXPORT_ALBUM: title=fxtr("Export Albums");
                          subtitle=fxtr("Export tracks from album to destination directory.");
                          break;
    case ID_EXPORT_TRACK: title=fxtr("Export Tracks");
                          subtitle=fxtr("Export tracks to destination directory.");
                          break;
    default: FXASSERT(0); break;
    }

//  GMTextField * textfield;
  FXDialogBox dialog(GMPlayerManager::instance()->getMainWindow(),title,DECOR_TITLE|DECOR_BORDER|DECOR_RESIZE|DECOR_CLOSE,0,0,0,0,0,0,0,0,0,0);
  GMPlayerManager::instance()->getMainWindow()->create_dialog_header(&dialog,title,subtitle,NULL);
  FXHorizontalFrame *closebox=new FXHorizontalFrame(&dialog,LAYOUT_SIDE_BOTTOM|LAYOUT_FILL_X|PACK_UNIFORM_WIDTH,0,0,0,0);
  new GMButton(closebox,fxtr("&Export"),NULL,&dialog,FXDialogBox::ID_ACCEPT,BUTTON_INITIAL|BUTTON_DEFAULT|LAYOUT_RIGHT|FRAME_RAISED|FRAME_THICK,0,0,0,0, 15,15);
  new GMButton(closebox,fxtr("&Cancel"),NULL,&dialog,FXDialogBox::ID_CANCEL,BUTTON_DEFAULT|LAYOUT_RIGHT|FRAME_RAISED|FRAME_THICK,0,0,0,0, 15,15);
  new FXSeparator(&dialog,SEPARATOR_GROOVE|LAYOUT_FILL_X|LAYOUT_SIDE_BOTTOM);
  FXVerticalFrame * main = new FXVerticalFrame(&dialog,LAYOUT_FILL_X|LAYOUT_FILL_Y,0,0,0,0,30,20,10,10);
  FXMatrix * matrix = new FXMatrix(main,2,MATRIX_BY_COLUMNS|LAYOUT_FILL_X,0,0,0,0,0,0,4,0);
/*
  new FXLabel(matrix,"Directory:",NULL,labelstyle);
  FXHorizontalFrame * hframe = new FXHorizontalFrame(matrix,LAYOUT_FILL_X|LAYOUT_FILL_COLUMN,0,0,0,0,0,0,0,0);
  GMTextField * textfield = new GMTextField(hframe,20,NULL,0,LAYOUT_FILL_X|TEXTFIELD_ENTER_ONLY|FRAME_SUNKEN|FRAME_THICK);
  new GMButton(hframe,… "\tSelect Directory",NULL,NULL,0);

*/
  new FXLabel(matrix,fxtr("Template:"),NULL,labelstyle);
  new GMTextField(matrix,20,NULL,0,LAYOUT_FILL_X|TEXTFIELD_ENTER_ONLY|FRAME_SUNKEN|FRAME_THICK|LAYOUT_FILL_COLUMN);
 // textfield->setFont(font_fixed);

  new FXLabel(matrix,fxtr("Encoding:"),NULL,labelstyle);
  GMListBox * list_codecs = new GMListBox(matrix,NULL,0,FRAME_SUNKEN|FRAME_THICK|LAYOUT_FILL_COLUMN);
  for (int i=0;gmcodecnames[i]!=NULL;i++)
    list_codecs->appendItem(gmcodecnames[i]);
  list_codecs->setNumVisible(9);

  new FXLabel(matrix,fxtr("Options:"),NULL,labelstyle);
  new GMCheckButton(matrix,fxtr("Replace spaces with underscores"),NULL,0,LAYOUT_FILL_COLUMN|CHECKBUTTON_NORMAL);
  new FXFrame(matrix,FRAME_NONE);
  new GMCheckButton(matrix,fxtr("Lower case"),NULL,0,LAYOUT_FILL_COLUMN|CHECKBUTTON_NORMAL);
  new FXFrame(matrix,FRAME_NONE);
  new GMCheckButton(matrix,fxtr("Lower case extension"),NULL,0,LAYOUT_FILL_COLUMN|CHECKBUTTON_NORMAL);

  if (dialog.execute()){



/*
    FXStringList files;

    GMTrack info;
    db->getTrackFilenames(tracks,files);

    FXString dest="/home/sxj/cartrip";

    gm_make_path(dest);

    for (FXint i=0;i<files.no();i++){
      db->getTrack(tracks[i],info);
      GMFilename::create(dest,info,"/home/sxj/cartrip/-%p-%T","\'\\#~!\"$&();<>|`^*?[]/.:",GMFilename::NOSPACES|GMFilename::LOWERCASE|GMFilename::LOWERCASE_EXTENSION);
      dest = FXPath::directory(dest) + PATHSEPSTRING + FXString::value("%.3d",i+1) + FXPath::name(dest);
      fxmessage("%s\n",dest.text());
      FXFile::copy(files[i],dest);
      }

*/


//    NOSPACES  						= 0x00000001,
//    LOWERCASE 						= 0x00000002,
//    LOWERCASE_EXTENSION	  = 0x00000004









    }
  return 1;
  }

#include "GMCover.h"




class GMCoverTask : public GMTask {
public:
  enum {
    ModeAppend       = 0,
    ModeReplace      = 1,
    ModeReplaceAll   = 2,
    };
protected:
  GMTrackDatabase * database;
  FXIntList         tracks;
  FXStringList      files;
  GMCover*          cover;
  FXint             mode;
protected:
  FXint run() {
    try {
      database->beginTask();
      for (FXival i=0;i<files.no() && processing;i++) {
        if (database->interrupt)
          database->waitTask();

        taskmanager->setStatus(FXString::value("Writing Cover %ld/%ld..",i+1,tracks.no()));

        GMFileTag tag;
        if (tag.open(files[i],FILETAG_TAGS)) {
          switch (mode) {
            case ModeAppend     : tag.appendCover(cover);                     break;
            case ModeReplace    : tag.replaceCover(cover,COVER_REPLACE_TYPE); break;
            case ModeReplaceAll : tag.replaceCover(cover,COVER_REPLACE_ALL);  break;
            default             : break;
            }
          tag.save();
          database->setTrackImported(tracks[i],FXThread::time());
          }
        }
      database->commitTask();
      }
    catch(GMDatabaseException&) {
      database->rollbackTask();
      return 1;
      }
    return 0;
    }
public:
  GMCoverTask(GMTrackDatabase*db,const FXIntList & t,const FXStringList & f,GMCover * c,FXint m) : database(db), tracks(t), files(f), cover(c),mode(m) {
    }
  ~GMCoverTask() {
    delete cover;
    }
  };





static const FXchar * const covertypes[]={
  "Other",
  "File Icon",
  "Other File Icon",
  "Front",
  "Back",
  "Leaflet",
  "Media",
  "Lead Artist",
  "Artist",
  "Conductor",
  "Band",
  "Composer",
  "Lyricist",
  "Recording Location",
  "During Recording",
  "During Perfomance",
  "Screen Capture",
  "Fish",
  "Illustration",
  "Band Logo",
  "Publisher Logo"
  };


long GMDatabaseSource::onCmdAddCover(FXObject*,FXSelector,void*){
  const FXuint labelstyle=LAYOUT_CENTER_Y|LABEL_NORMAL|LAYOUT_RIGHT;

  FXIntList tracks;
  GMPlayerManager::instance()->getTrackView()->getSelectedTracks(tracks);

  const FXchar image_patterns[]="All Images (*.png,*.jpeg,*.jpg,*.bmp,*.gif)";

  GMFileDialog dialog(GMPlayerManager::instance()->getMainWindow(),"Select Cover");
  dialog.setDirectory(GMApp::instance()->reg().readStringEntry("directories","last-add-cover-dir",FXSystem::getHomeDirectory().text()));
  dialog.setPatternList(image_patterns);
  if (dialog.execute()) {
    GMApp::instance()->reg().writeStringEntry("directories","last-add-cover-dir",dialog.getDirectory().text());
    GMCover * cover = GMCover::fromFile(dialog.getFilename());
    GMImageInfo info;
    if (cover && cover->getImageInfo(info)) {
      FXDialogBox confirmdialog(GMPlayerManager::instance()->getMainWindow(),"Add Album Cover",DECOR_TITLE|DECOR_BORDER|DECOR_RESIZE|DECOR_CLOSE,0,0,0,0,0,0,0,0,0,0);
      FXHorizontalFrame *closebox=new FXHorizontalFrame(&confirmdialog,LAYOUT_SIDE_BOTTOM|LAYOUT_FILL_X|PACK_UNIFORM_WIDTH,0,0,0,0);
      new GMButton(closebox,fxtr("&OK"),NULL,&confirmdialog,FXDialogBox::ID_ACCEPT,BUTTON_INITIAL|BUTTON_DEFAULT|LAYOUT_RIGHT|FRAME_RAISED|FRAME_THICK,0,0,0,0, 15,15);
      new GMButton(closebox,fxtr("&Cancel"),NULL,&confirmdialog,FXDialogBox::ID_CANCEL,BUTTON_DEFAULT|LAYOUT_RIGHT|FRAME_RAISED|FRAME_THICK,0,0,0,0, 15,15);
      new FXSeparator(&dialog,SEPARATOR_GROOVE|LAYOUT_FILL_X|LAYOUT_SIDE_BOTTOM);
   //   FXHorizontalFrame * main = new FXHorizontalFrame(&confirmdialog,LAYOUT_FILL_X|LAYOUT_FILL_Y,0,0,0,0,10,10,10,10);
      FXVerticalFrame * main = new FXVerticalFrame(&confirmdialog,LAYOUT_FILL_X|LAYOUT_FILL_Y,0,0,0,0,10,10,10,10);

      FXImage * image = GMCover::copyToImage(cover,256);
      FXImageFrame * view = new FXImageFrame(main,image,LAYOUT_FILL|FRAME_LINE);
      view->setBackColor(FXApp::instance()->getBackColor());


      FXMatrix * matrix = new FXMatrix(main,2,MATRIX_BY_COLUMNS|LAYOUT_FILL,0,0,0,0,0,0,4,0);

      new FXLabel(matrix,fxtr("Dimensions:"),NULL,labelstyle);
      new FXLabel(matrix,FXString::value("%d x %d",info.width,info.height),NULL,LAYOUT_CENTER_Y|LABEL_NORMAL|LAYOUT_FILL_COLUMN);

      new FXLabel(matrix,fxtr("Size:"),NULL,labelstyle);
      new FXLabel(matrix,FXString::value("%d",cover->size),NULL,LAYOUT_CENTER_Y|LABEL_NORMAL|LAYOUT_FILL_COLUMN);

      new FXLabel(matrix,fxtr("Cover Type:"),NULL,labelstyle);
      GMListBox * list_types = new GMListBox(matrix,NULL,0,FRAME_SUNKEN|FRAME_THICK|LAYOUT_FILL_COLUMN);
      for (FXuint i=0;i<ARRAYNUMBER(covertypes);i++)
        list_types->appendItem(covertypes[i],NULL,(void*)(FXival)i);

      list_types->setNumVisible(9);
      list_types->setCurrentItem(list_types->findItem("Front"));

      new FXLabel(matrix,fxtr("Description:"),NULL,labelstyle|LAYOUT_FILL_ROW);
      GMTextField * label = new GMTextField(matrix,20,NULL,0,FRAME_SUNKEN|FRAME_THICK|LAYOUT_FILL_COLUMN|LAYOUT_FILL);

      new FXLabel(matrix,fxtr("Tag Mode:"),NULL,labelstyle);
      GMListBox * list_tag = new GMListBox(matrix,NULL,0,FRAME_SUNKEN|FRAME_THICK|LAYOUT_FILL_COLUMN);
      list_tag->appendItem("Append",NULL,(void*)(FXival)GMCoverTask::ModeAppend);
      list_tag->appendItem("Replace",NULL,(void*)(FXival)GMCoverTask::ModeReplace);
      list_tag->appendItem("Replace All",NULL,(void*)(FXival)GMCoverTask::ModeReplaceAll);
      list_tag->setNumVisible(3);

      if (confirmdialog.execute()) {
        FXStringList files;
        db->getTrackFilenames(tracks,files);

        FXint mode  = (FXint)(FXival)list_tag->getItemData(list_tag->getCurrentItem());
        cover->type = (FXint)(FXival)list_types->getItemData(list_types->getCurrentItem());
        cover->description = label->getText();

        GMCoverTask * task = new GMCoverTask(db,tracks,files,cover,mode);
        task->setTarget(GMPlayerManager::instance());
        task->setSelector(GMPlayerManager::ID_IMPORT_TASK);
        GMPlayerManager::instance()->runTask(task);
        return 1;
        }
      }
    delete cover;
    }
  return 1;
  }

long GMDatabaseSource::onCmdDelete(FXObject*,FXSelector sel,void*){
  FXIntList tracks;
  FXIntList selected;
  FXStringList files;
  if (FXSELID(sel)==ID_DELETE_TRACK) {
    GMPlayerManager::instance()->getTrackView()->getSelectedTracks(tracks);
    }
  else {
    GMPlayerManager::instance()->getTrackView()->getTracks(tracks);
    }
  if (tracks.no()==0) return 1;

  FXString title;
  FXString subtitle;

  switch(FXSELID(sel)){
/*
    case ID_DELETE_TAG   : title=fxtr("Remove Genre?");
                          subtitle=fxtr("Remove tracks with genre from library?");
                          GMPlayerManager::instance()->getTrackView()->getSelectedTags(selected);
                          if (selected.no()==0) return 1;
                          break;
*/
    case ID_DELETE_ARTIST:title=fxtr("Remove Artist?");
                          subtitle=fxtr("Remove tracks from artist from library?");
                          GMPlayerManager::instance()->getTrackView()->getSelectedArtists(selected);
                          if (selected.no()==0) return 1;
                          break;
    case ID_DELETE_ALBUM: title=fxtr("Remove Album?");
                          subtitle=fxtr("Remove tracks from album from library?");
                          GMPlayerManager::instance()->getTrackView()->getSelectedAlbums(selected);
                          if (selected.no()==0) return 1;
                          break;
    case ID_DELETE_TRACK: title=fxtr("Remove Track(s)?");
                          subtitle=fxtr("Remove track(s) from library?");
                          break;
    default: FXASSERT(0); break;
    }




  FXDialogBox dialog(GMPlayerManager::instance()->getMainWindow(),title,DECOR_TITLE|DECOR_BORDER|DECOR_RESIZE|DECOR_CLOSE,0,0,0,0,0,0,0,0,0,0);
  GMPlayerManager::instance()->getMainWindow()->create_dialog_header(&dialog,title,subtitle,NULL);
  FXHorizontalFrame *closebox=new FXHorizontalFrame(&dialog,LAYOUT_SIDE_BOTTOM|LAYOUT_FILL_X|PACK_UNIFORM_WIDTH,0,0,0,0);
  new GMButton(closebox,fxtr("&Remove"),NULL,&dialog,FXDialogBox::ID_ACCEPT,BUTTON_INITIAL|BUTTON_DEFAULT|LAYOUT_RIGHT|FRAME_RAISED|FRAME_THICK,0,0,0,0, 15,15);
  new GMButton(closebox,fxtr("&Cancel"),NULL,&dialog,FXDialogBox::ID_CANCEL,BUTTON_DEFAULT|LAYOUT_RIGHT|FRAME_RAISED|FRAME_THICK,0,0,0,0, 15,15);
  new FXSeparator(&dialog,SEPARATOR_GROOVE|LAYOUT_FILL_X|LAYOUT_SIDE_BOTTOM);
  FXVerticalFrame * main = new FXVerticalFrame(&dialog,LAYOUT_FILL_X|LAYOUT_FILL_Y,0,0,0,0,30,20,10,10);
  GMCheckButton * from_disk = new GMCheckButton(main,fxtr("Remove tracks from disk"));
  from_disk->setCheck(FXApp::instance()->reg().readBoolEntry("delete dialog","from-disk",false));

  if (dialog.execute()){

//    db->beginDelete();

    if (from_disk->getCheck())
      db->getTrackFilenames(tracks,files);


    switch(FXSELID(sel)){
/*
      case ID_DELETE_TAG:
        if (!db->removeGenre(selected[0]))
          FXMessageBox::error(GMPlayerManager::instance()->getMainWindow(),MBOX_OK,fxtr("Library Error"),fxtr("Unable to remove genre from the library"));
        break;
*/
      case ID_DELETE_ARTIST:
        if (!db->removeArtist(selected[0]))
          FXMessageBox::error(GMPlayerManager::instance()->getMainWindow(),MBOX_OK,fxtr("Library Error"),fxtr("Unable to remove artist from the library"));
        break;
      case ID_DELETE_ALBUM:
        if (!db->removeAlbum(selected[0]))
          FXMessageBox::error(GMPlayerManager::instance()->getMainWindow(),MBOX_OK,fxtr("Library Error"),fxtr("Unable to remove album from the library"));
        break;
      case ID_DELETE_TRACK:
          try {
            db->begin();
            db->removeTracks(tracks);
            db->commit();
            }
          catch(GMDatabaseException&) {
            db->rollback();
            FXMessageBox::error(GMPlayerManager::instance()->getMainWindow(),MBOX_OK,fxtr("Library Error"),fxtr("Unable to remove track from the library."));
            }
        break;
      default: FXASSERT(0); break;
      }

    if (from_disk->getCheck())
      removeFiles(files);

    FXApp::instance()->reg().writeBoolEntry("delete dialog","from-disk",from_disk->getCheck());

    GMPlayerManager::instance()->getSourceView()->refresh();
    GMPlayerManager::instance()->getTrackView()->refresh();
    }
  return 1;
  }


long GMDatabaseSource::onCmdCopyArtistAlbum(FXObject*,FXSelector,void*){
  FXDragType types[4]={GMClipboard::trackdatabase,GMClipboard::kdeclipboard,GMClipboard::gnomeclipboard,FXWindow::urilistType};
  GMDatabaseClipboardData * data = new GMDatabaseClipboardData;
  if (GMClipboard::instance()->acquire(this,types,4,data)){
    FXApp::instance()->beginWaitCursor();
    data->db=db;
    GMPlayerManager::instance()->getTrackView()->getTracks(data->tracks);
    FXApp::instance()->endWaitCursor();
    }
  else {
    delete data;
    FXApp::instance()->beep();
    }
  return 1;
  }

long GMDatabaseSource::onCmdCopyTrack(FXObject*,FXSelector,void*){
  FXDragType types[4]={GMClipboard::trackdatabase,GMClipboard::kdeclipboard,GMClipboard::gnomeclipboard,FXWindow::urilistType};
  GMDatabaseClipboardData * data = new GMDatabaseClipboardData;
  if (GMClipboard::instance()->acquire(this,types,4,data)){
    FXApp::instance()->beginWaitCursor();
    data->db=db;
    GMPlayerManager::instance()->getTrackView()->getSelectedTracks(data->tracks);
    FXApp::instance()->endWaitCursor();
    }
  else {
    delete data;
    FXApp::instance()->beep();
    }
  return 1;
  }



long GMDatabaseSource::onCmdRequestArtistAlbum(FXObject*sender,FXSelector,void*ptr){
  FXEvent* event=(FXEvent*)ptr;
  FXWindow*window=(FXWindow*)sender;
  if(event->target==GMClipboard::urilistType){
    FXStringList filenames;
    FXIntList tracks;
    FXString uri;
    GMPlayerManager::instance()->getTrackView()->getTracks(tracks);
    db->getTrackFilenames(tracks,filenames);
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

long GMDatabaseSource::onCmdRequestTrack(FXObject*sender,FXSelector,void*ptr){
  FXEvent* event=(FXEvent*)ptr;
  FXWindow*window=(FXWindow*)sender;
  if(event->target==GMClipboard::urilistType){
    FXStringList filenames;
    FXIntList tracks;
    FXString uri;
    GMPlayerManager::instance()->getTrackView()->getSelectedTracks(tracks);
    db->getTrackFilenames(tracks,filenames);
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


void gm_import_files(const FXStringList & filelist,FXint playlist,FXuint whence){
  GMImportDialog dialog(GMPlayerManager::instance()->getMainWindow(),whence);
  if (dialog.execute()) {
    GMImportTask * task = new GMImportTask(GMPlayerManager::instance(),GMPlayerManager::ID_IMPORT_TASK);
    task->setOptions(GMPlayerManager::instance()->getPreferences().import);
    task->setInput(filelist);
    task->setPlaylist(playlist);
    GMPlayerManager::instance()->runTask(task);
    }
  }


enum {
  DND_KDE              =0x1,
  DND_URI              =0x2,
  DND_TRACKS_SELECTED  =0x4,
  DND_TRACKS_ALL       =0x8,
  DND_TRACKS_ID        =0x10,
  DND_GNOME            =0x20
  };

FXuint gm_parse_dragtypes(FXDragType*types,FXuint ntypes){
  FXuint dnd=0;
  for (FXuint i=0;i<ntypes;i++){
    if (types[i]==GMClipboard::alltracks)            dnd|=DND_TRACKS_ALL;
    else if (types[i]==GMClipboard::selectedtracks)  dnd|=DND_TRACKS_SELECTED;
    else if (types[i]==GMClipboard::trackdatabase)   dnd|=DND_TRACKS_ID;
    else if (types[i]==GMClipboard::kdeclipboard)    dnd|=DND_KDE;
    else if (types[i]==FXWindow::urilistType)        dnd|=DND_URI;
    else if (types[i]==GMClipboard::gnomeclipboard)  dnd|=DND_GNOME;
    }
  freeElms(types);
  return dnd;
  }



void GMDatabaseSource::addTracks(GMSource * src,const FXIntList & tracks) {
  if (src->getType()==SOURCE_DATABASE || src->getType()==SOURCE_DATABASE_PLAYLIST || src->getType()==SOURCE_DATABASE_FILTER || src->getType()==SOURCE_PLAYQUEUE)
    db->insertPlaylistTracks(playlist,tracks);
  }

long GMDatabaseSource::onCmdDrop(FXObject*sender,FXSelector,void*){
  FXWindow*    window=dynamic_cast<FXWindow*>(sender);
  FXuint       from,ntypes;
  FXDragType * types=NULL;
  if (window->inquireDNDTypes(FROM_DRAGNDROP,types,ntypes)){
    from = gm_parse_dragtypes(types,ntypes);

    if (!playlist && from&(DND_TRACKS_SELECTED|DND_TRACKS_ALL|DND_TRACKS_ID) )
      return 0;

    if (from&(DND_TRACKS_SELECTED|DND_TRACKS_ALL)) {
      FXIntList tracks;
      if (from&DND_TRACKS_SELECTED)
        GMPlayerManager::instance()->getTrackView()->getSelectedTracks(tracks);
      else
        GMPlayerManager::instance()->getTrackView()->getTracks(tracks);

      if (tracks.no()) addTracks(GMPlayerManager::instance()->getTrackView()->getSource(),tracks);
      }
    else if ((from&DND_URI) && window->getDNDData(FROM_DRAGNDROP,FXWindow::urilistType,dndfiles)) {
      FXApp::instance()->addChore(this,ID_IMPORT_FILES,&dndfiles);
      }
    else {
      return 0;
      }
    return 1;
    }
  return 0;
  }


long GMDatabaseSource::onCmdPaste(FXObject*,FXSelector,void*){
  FXString     files;
  FXStringList filelist;
  FXDragType * types;
  FXuint       from,ntypes;
  GMClipboard * clipboard = GMClipboard::instance();

  if (clipboard->inquireDNDTypes(FROM_CLIPBOARD,types,ntypes)) {
    from = gm_parse_dragtypes(types,ntypes);
    if (!playlist && from&(DND_TRACKS_SELECTED|DND_TRACKS_ALL|DND_TRACKS_ID) )
      return 0;

    if (from&DND_TRACKS_ID) {
      GMDatabaseClipboardData * clipdata = dynamic_cast<GMDatabaseClipboardData*>(clipboard->getClipData());
      if (clipdata && clipdata->tracks.no() && db->insertPlaylistTracks(playlist,clipdata->tracks))
        GMPlayerManager::instance()->getTrackView()->refresh();
      else
        FXApp::instance()->beep();
      return 0; // done here
      }
    else if (from&DND_GNOME && clipboard->getDNDData(FROM_CLIPBOARD,GMClipboard::gnomeclipboard,files)) {
      gm_convert_gnomeclipboard_to_filenames(files,filelist);
      }
    else if (from&DND_URI && clipboard->getDNDData(FROM_CLIPBOARD,FXWindow::urilistType,files)) {
      gm_convert_uri_to_filenames(files,filelist);
      }

    if (filelist.no()) {
      gm_import_files(filelist,playlist,IMPORT_FROMPASTE);
      }
    }
  return 0;
  }



long GMDatabaseSource::onUpdPaste(FXObject*,FXSelector,void*){
  return 1;
  }


long GMDatabaseSource::onCmdNewPlayList(FXObject*,FXSelector,void*){
  FXDialogBox dialog(GMPlayerManager::instance()->getMainWindow(),fxtr("Create Playlist"),DECOR_TITLE|DECOR_BORDER|DECOR_RESIZE,0,0,0,0,0,0,0,0,0,0);
  GMPlayerManager::instance()->getMainWindow()->create_dialog_header(&dialog,fxtr("Create Playlist"),fxtr("Specify name of the new playlist"),NULL);
  FXHorizontalFrame *closebox=new FXHorizontalFrame(&dialog,LAYOUT_SIDE_BOTTOM|LAYOUT_FILL_X|PACK_UNIFORM_WIDTH,0,0,0,0);
  new GMButton(closebox,fxtr("&Create"),NULL,&dialog,FXDialogBox::ID_ACCEPT,BUTTON_INITIAL|BUTTON_DEFAULT|LAYOUT_RIGHT|FRAME_RAISED|FRAME_THICK,0,0,0,0, 15,15);
  new GMButton(closebox,fxtr("&Cancel"),NULL,&dialog,FXDialogBox::ID_CANCEL,BUTTON_DEFAULT|LAYOUT_RIGHT|FRAME_RAISED|FRAME_THICK,0,0,0,0, 15,15);
  new FXSeparator(&dialog,SEPARATOR_GROOVE|LAYOUT_FILL_X|LAYOUT_SIDE_BOTTOM);
  FXVerticalFrame * main = new FXVerticalFrame(&dialog,LAYOUT_FILL_X|LAYOUT_FILL_Y,0,0,0,0,30,20,10,10);
  FXMatrix * matrix = new FXMatrix(main,2,LAYOUT_FILL_X|MATRIX_BY_COLUMNS);
  new FXLabel(matrix,fxtr("Name"),NULL,LABEL_NORMAL|LAYOUT_RIGHT|LAYOUT_CENTER_Y);
  GMTextField * name_field = new GMTextField(matrix,20,&dialog,FXDialogBox::ID_ACCEPT,LAYOUT_FILL_X|LAYOUT_FILL_COLUMN|FRAME_SUNKEN|FRAME_THICK|TEXTFIELD_ENTER_ONLY);
  name_field->setText(fxtr("New Playlist"));
  dialog.create();
  gm_focus_and_select(name_field);
  if (dialog.execute()) {
    FXString label= name_field->getText().trim();
    if (!label.empty()) {
      GMPlayerManager::instance()->createPlaylist(label);
      }
    }
  return 1;
  }



long GMDatabaseSource::onCmdImportPlayList(FXObject*,FXSelector,void*){
  GMImportDialog dialog(GMPlayerManager::instance()->getMainWindow(),IMPORT_FROMFILE|IMPORT_PLAYLIST);
  if (dialog.execute()){

    FXString buffer;
    FXStringList urls;

    if (gm_buffer_file(dialog.getFilename(),buffer)) {

      FXString title;
      FXString extension = FXPath::extension(dialog.getFilename());

      if (comparecase(extension,"m3u")==0)
        ap_parse_m3u(buffer,urls);
      else if (comparecase(extension,"pls")==0)
        ap_parse_pls(buffer,urls);
      else
        ap_parse_xspf(buffer,urls,title);

      if (urls.no()) {

        gm_make_absolute_path(FXPath::directory(dialog.getFilename()),urls);

        title.trim();
        if (title.empty()) title = FXPath::title(dialog.getFilename());

        FXint pl = GMPlayerManager::instance()->createPlaylist(title);

        GMImportTask * task = new GMImportTask(GMPlayerManager::instance(),GMPlayerManager::ID_IMPORT_TASK);
        task->setOptions(GMPlayerManager::instance()->getPreferences().import);
        task->setInput(urls);
        task->setPlaylist(pl);
        GMPlayerManager::instance()->runTask(task);



//        GMImportDatabase searchdialog(GMPlayerManager::instance()->getMainWindow(),urls,GMPlayerManager::instance()->getPreferences().import,pl);
//        searchdialog.execute();
//        GMPlayerManager::instance()->getTrackView()->refresh();
        }
      }
    }
  return 1;
  }



long GMDatabaseSource::onCmdClear(FXObject*,FXSelector,void*){
  FXDialogBox dialog(GMPlayerManager::instance()->getMainWindow(),fxtr("Clear Music Library?"),DECOR_TITLE|DECOR_BORDER|DECOR_RESIZE|DECOR_CLOSE,0,0,0,0,0,0,0,0,0,0);
  GMPlayerManager::instance()->getMainWindow()->create_dialog_header(&dialog,fxtr("Clear Music Library?"),fxtr("Remove all tracks from the music library?"),NULL);
  FXHorizontalFrame *closebox=new FXHorizontalFrame(&dialog,LAYOUT_SIDE_BOTTOM|LAYOUT_FILL_X|PACK_UNIFORM_WIDTH,0,0,0,0);
  new GMButton(closebox,fxtr("&Remove All"),NULL,&dialog,FXDialogBox::ID_ACCEPT,BUTTON_INITIAL|BUTTON_DEFAULT|LAYOUT_RIGHT|FRAME_RAISED|FRAME_THICK,0,0,0,0, 15,15);
  new GMButton(closebox,fxtr("&Cancel"),NULL,&dialog,FXDialogBox::ID_CANCEL,BUTTON_DEFAULT|LAYOUT_RIGHT|FRAME_RAISED|FRAME_THICK,0,0,0,0, 15,15);
  new FXSeparator(&dialog,SEPARATOR_GROOVE|LAYOUT_FILL_X|LAYOUT_SIDE_BOTTOM);
  FXVerticalFrame * main = new FXVerticalFrame(&dialog,LAYOUT_FILL_X|LAYOUT_FILL_Y,0,0,0,0,30,20,10,10);
  GMCheckButton * playlist_check = new GMCheckButton(main,fxtr("Keep play lists"));
  playlist_check->setCheck(FXApp::instance()->reg().readBoolEntry("clear dialog","keep-play-lists",true));
  if (dialog.execute()){
    //GMPlayerManager::instance()->stop();
    db->clearTracks(!playlist_check->getCheck());
    GMPlayerManager::instance()->removePlayListSources();
    GMPlayerManager::instance()->getSourceView()->refresh();
    GMPlayerManager::instance()->getTrackView()->refresh();
    FXApp::instance()->reg().writeBoolEntry("clear dialog","keep-play-lists",playlist_check->getCheck());
    }
  return 1;
  }

long GMDatabaseSource::onQueryTip(FXObject*sender,FXSelector,void*){
  FXint ntracks=0,nartists=0,nalbums=0,ntime=0;
  db->getTrackStats(ntracks,nartists,nalbums,ntime,playlist);
  FXString text,time_text;
  text.format("%s\n%d Artists, %d Albums, %d Tracks\nTotal Time: ",getName().text(),nartists,nalbums,ntracks);
  gm_print_time(ntime,time_text);
  text+=time_text;
  sender->handle(this,FXSEL(SEL_COMMAND,FXWindow::ID_SETSTRINGVALUE),(void*)&text);
  return 1;
  }


long GMDatabaseSource::onCmdTrackPlayed(FXObject*,FXSelector,void*) {
  FXTRACE((60,"%s::onCmdTrackPlayed\n",getClassName()));
  FXASSERT(current_track>=0);
  FXlong timestamp = (FXlong)FXThread::time();
  db->setTrackPlayed(current_track,timestamp);
  GMTrack info;
  if (getTrack(info) && GMPlayerManager::instance()->getAudioScrobbler())
    GMPlayerManager::instance()->getAudioScrobbler()->submit(timestamp,info);
  return 1;
  }

long GMDatabaseSource::onCmdOpenFolder(FXObject*,FXSelector,void*){
  FXIntList tracks;
  GMPlayerManager::instance()->getTrackView()->getSelectedTracks(tracks);
  if (tracks.no()==1) {
    gm_open_folder(FXPath::directory(db->getTrackFilename(tracks[0])));
    }
  return 1;
  }

long GMDatabaseSource::onCmdSearchCover(FXObject*,FXSelector sel,void*){
  FXIntList tracks;
  GMTrack   info;

  if (FXSELID(sel)==ID_SEARCH_COVER)
    GMPlayerManager::instance()->getTrackView()->getSelectedTracks(tracks);
  else
    GMPlayerManager::instance()->getTrackView()->getTracks(tracks);

  if (tracks.no()) {
    db->getTrack(tracks[0],info);
    gm_image_search(info.album_artist + "+" + info.album);
    }
  return 1;
  }


long GMDatabaseSource::onDndImportFiles(FXObject*,FXSelector,void*){
  FXStringList filelist;
  gm_convert_uri_to_filenames(dndfiles,filelist);
  if (filelist.no()){
    gm_import_files(filelist,playlist,IMPORT_FROMPASTE);
    return 1;
    }
  return 0;
  }


long GMDatabaseSource::onCmdNewFilter(FXObject*,FXSelector,void*){
  GMFilterSource::create(db);
  return 1;
  }
