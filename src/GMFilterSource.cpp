/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2015-2021 by Sander Jansen. All Rights Reserved      *
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
#include "GMFilter.h"
#include "GMDatabase.h"
#include "GMTrackDatabase.h"
#include "GMSource.h"
#include "GMDatabaseSource.h"
#include "GMFilterSource.h"
#include "GMFilterEditor.h"
#include "GMApp.h"
#include "GMIconTheme.h"
#include "GMTrackView.h"
#include "GMSourceView.h"
#include "GMPlayerManager.h"
#include "GMWindow.h"

/*

#include "GMList.h"
#include "GMCover.h"
#include "GMCoverCache.h"
#include "GMTrackList.h"
#include "GMTrackItem.h"
#include "GMTaskManager.h"
#include "GMClipboard.h"
*/

#define FILTER_DB_V1 2015

FXObjectListOf<GMFilterSource> GMFilterSource::sources;


void GMFilterSource::init(GMTrackDatabase * database,GMSourceList & list){
  FXFileStream store;

  // Load from disk
  if (store.open(GMApp::getDataDirectory()+PATHSEPSTRING+"filters.db",FXStreamLoad)){
    FXuint version;
    FXint  nitems;
    store >> version;
    if (version==FILTER_DB_V1) {
      store >> nitems;
      for (FXint i=0;i<nitems;i++){
        GMFilterSource * src = new GMFilterSource(database);
        src->match.load(store);
        sources.append(src);
        }

#if FOXVERSION >= FXVERSION(1,7,57)
      for (FXint i=0;i<sources.no();i++) {
        list.append(sources[i]);
        }
#else
      list.append(sources);
#endif
      return;
      }
    }

  // Initialize Default Ones
  sources.append(new GMFilterSource(database,GMFilter("Recently Played",Rule::ColumnPlayDate,Rule::OperatorGreater,60*60*24*7)));
  sources.append(new GMFilterSource(database,GMFilter("Recently Added",Rule::ColumnImportDate,Rule::OperatorGreater,60*60*24*7)));

#if FOXVERSION >= FXVERSION(1,7,57)
  for (FXint i=0;i<sources.no();i++) {
    list.append(sources[i]);
    }
#else
  list.append(sources);
#endif

  // Save to disk
  GMFilterSource::save();
  }


void GMFilterSource::save() {
  FXFileStream store;
  if (store.open(GMApp::getDataDirectory()+PATHSEPSTRING+"filters.db",FXStreamSave)){
    GMFilter::nextid=0;
    FXuint version = FILTER_DB_V1;
    FXint  nitems  = sources.no();
    store << version;
    store << nitems;
    for (FXint i=0;i<sources.no();i++){
      sources[i]->match.save(store);
      }
    }
  }


void GMFilterSource::create(GMTrackDatabase * database) {
  GMFilterSource * source = new GMFilterSource(database);
  GMFilterEditor editor(GMPlayerManager::instance()->getMainWindow(),source->match);
  if (editor.execute(PLACEMENT_SCREEN)) {
    editor.getFilter(source->match);
    sources.append(source);
    GMFilterSource::save();
    GMPlayerManager::instance()->insertSource(source);
    GMPlayerManager::instance()->getSourceView()->refresh();
    GMPlayerManager::instance()->getSourceView()->setSource(source);
    return;
    }
  delete source;
  }



FXDEFMAP(GMFilterSource) GMFilterSourceMap[]={
  FXMAPFUNC(SEL_COMMAND,GMFilterSource::ID_EDIT,GMFilterSource::onCmdEdit),
  FXMAPFUNC(SEL_COMMAND,GMFilterSource::ID_REMOVE,GMFilterSource::onCmdRemove),
  };

FXIMPLEMENT(GMFilterSource,GMDatabaseSource,GMFilterSourceMap,ARRAYNUMBER(GMFilterSourceMap));


GMFilterSource::GMFilterSource(GMTrackDatabase * db,const GMFilter & m) : GMDatabaseSource(db),match(m) {
  }


GMFilterSource::GMFilterSource(GMTrackDatabase * db) : GMDatabaseSource(db) {
  }


GMFilterSource::~GMFilterSource(){
  }


FXString GMFilterSource::settingKey() const {
  return "database_filter_" + FXString::value(match.id);
  }


FXString GMFilterSource::getName() const {
  return match.name;
  }


void GMFilterSource::updateView() {
  FXString query = match.getMatch();
  if (query.length()) {
    db->execute("DROP VIEW IF EXISTS query_view;");
    db->execute("CREATE TEMP VIEW query_view AS "
                "SELECT tracks.id as track, tracks.album as album FROM tracks JOIN albums ON tracks.album == albums.id "
                                                                             "JOIN artists AS album_artist ON (albums.artist == album_artist.id) "
                                                                             "JOIN artists AS track_artist ON (tracks.artist == track_artist.id) "
                                                                             "JOIN pathlist ON (tracks.path == pathlist.id) "
                                                                             "LEFT JOIN artists AS composers ON (tracks.composer == composers.id) "
                                                                             "LEFT JOIN artists AS conductors ON (tracks.conductor == conductors.id) " + query);
    hasview=true;
    }
  else {
    hasview=false;
    }
  }


void GMFilterSource::configure(GMColumnList& columns) {
  GMDatabaseSource::configure(columns);
  updateView();
  }


long GMFilterSource::onCmdEdit(FXObject*,FXSelector,void*){
  GMFilterEditor editor(GMPlayerManager::instance()->getMainWindow(),match);
  if (editor.execute(PLACEMENT_SCREEN)) {
    editor.getFilter(match);
    updateView();
    GMFilterSource::save();
    GMPlayerManager::instance()->getTrackView()->refresh();
    GMPlayerManager::instance()->getSourceView()->refresh(this);
    }
  return 1;
  }


long GMFilterSource::onCmdRemove(FXObject*,FXSelector,void *){
  if (GMPlayerManager::instance()->getMainWindow()->question(fxtr("Remove Filter"),fxtr("Are you sure you want to remove the filter?"),fxtr("&Yes"),fxtr("&No"))){
    sources.remove(this);
    GMFilterSource::save();
    GMPlayerManager::instance()->removeSource(this);
    }
  return 1;
  }


FXbool GMFilterSource::source_context_menu(FXMenuPane * pane){
  new GMMenuCommand(pane,fxtr("Edit"),GMIconTheme::instance()->icon_edit,this,GMFilterSource::ID_EDIT);
  new GMMenuCommand(pane,fxtr("Remove Filter"),GMIconTheme::instance()->icon_delete,this,GMFilterSource::ID_REMOVE);
  return true;
  }
