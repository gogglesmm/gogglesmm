/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2007-2010 by Sander Jansen. All Rights Reserved      *
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
#include "GMTrack.h"
#include "GMList.h"
#include "GMDatabase.h"
#include "GMTrackDatabase.h"
#include "GMTrackList.h"
#include "GMTrackItem.h"
#include "GMTrackView.h"
#include "GMSource.h"
#include "GMSourceView.h"
#include "GMClipboard.h"
#include "GMStreamSource.h"
#include "GMPlayerManager.h"
#include "GMWindow.h"
#include "GMIconTheme.h"
#include "GMFilename.h"


FXDEFMAP(GMStreamSource) GMStreamSourceMap[]={
  FXMAPFUNC(SEL_COMMAND,GMStreamSource::ID_NEW_STATION,GMStreamSource::onCmdNewStation),
  FXMAPFUNC(SEL_COMMAND,GMStreamSource::ID_EDIT_STATION,GMStreamSource::onCmdEditStation),
  FXMAPFUNC(SEL_COMMAND,GMStreamSource::ID_DELETE_STATION,GMStreamSource::onCmdDeleteStation),
  FXMAPFUNC(SEL_UPDATE,GMStreamSource::ID_EXPORT,GMStreamSource::onUpdExport)

  };

FXIMPLEMENT(GMStreamSource,GMSource,GMStreamSourceMap,ARRAYNUMBER(GMStreamSourceMap));


GMStreamSource::GMStreamSource() : db(NULL) {
  }

GMStreamSource::GMStreamSource(GMTrackDatabase * database) : db(database)  {
  FXASSERT(db);
  }

GMStreamSource::~GMStreamSource(){
  }


void GMStreamSource::configure(GMColumnList& list){
  list.no(4);
  list[0]=GMColumn(notr("No"),HEADER_TRACK,GMStreamTrackItem::ascendingTrack,GMStreamTrackItem::descendingTrack,60,true,true,0);
  list[1]=GMColumn(notr("Station"),HEADER_TITLE,GMStreamTrackItem::ascendingTitle,GMStreamTrackItem::descendingTitle,200,true,true,1);
  list[2]=GMColumn(notr("Bitrate"),HEADER_BITRATE,GMStreamTrackItem::ascendingTime,GMStreamTrackItem::descendingTime,80,true,true,2);
  list[3]=GMColumn(notr("Genre"),HEADER_TAG,GMStreamTrackItem::ascendingTrack,GMStreamTrackItem::descendingTrack,150,true,true,3);
  }


FXbool GMStreamSource::hasCurrentTrack(GMSource * src) const {
  if (src==this) return true;
  return false;
  }

FXbool GMStreamSource::setTrack(GMTrack & track) const {
  if (current_track>=0 && track.bitrate>0) {
    db->setStreamBitrate(current_track,track.bitrate);
    if (GMPlayerManager::instance()->getTrackView()->getSource()==this)
      GMPlayerManager::instance()->getTrackView()->refresh();
    }
  return true;
  }

FXbool GMStreamSource::getTrack(GMTrack & info) const {
  info.clear();
  info.mrl=getTrackFilename(current_track);
  return false;
  }

FXString GMStreamSource::getTrackFilename(FXint id) const{
  const char * url;
  FXString query;
  GMQuery q;
  try {
    query="SELECT url FROM streams WHERE id == " + FXString::value(id) + ";";
    q = db->compile(query);
    q.row();
    url = q.get(0);
    return url;
    }
  catch(GMDatabaseException & e){
    return FXString::null;
    }
  return url;
  }


FXbool GMStreamSource::source_context_menu(FXMenuPane * pane){
  new GMMenuCommand(pane,fxtr("New Station…\t\t"),NULL,this,ID_NEW_STATION);
  return true;
  }

FXbool GMStreamSource::track_context_menu(FXMenuPane * pane){
  new GMMenuCommand(pane,fxtr("Edit…\t\t"),GMIconTheme::instance()->icon_edit,this,ID_EDIT_STATION);
  new GMMenuCommand(pane,fxtr("New Station…\t\t"),NULL,this,ID_NEW_STATION);
  new GMMenuCommand(pane,fxtr("Remove\t\tRemove."),GMIconTheme::instance()->icon_delete,this,ID_DELETE_STATION);
  return true;
  }

FXbool GMStreamSource::listTracks(GMTrackList * tracklist,const FXIntList &/* albumlist*/,const FXIntList & /*genre*/){
  GMQuery q;
  FXString query;
//  const FXchar * c_artist;
  //const FXchar * c_albumname;
  const FXchar * c_title;
  const FXchar * c_genre;
//  FXint time;
//  FXint no;
  FXint id;
  FXint bitrate;
  FXint queue=1;
  try {

    query = "SELECT streams.id, streams.description, streams.bitrate, tags.name "
            "FROM streams, tags "
            "WHERE tags.id == streams.genre;";

    q = db->compile(query);

    while(q.row()){
      q.get(0,id);
      c_title = q.get(1);
      q.get(2,bitrate);
      c_genre = q.get(3);
      GMStreamTrackItem * item = new GMStreamTrackItem(id,c_title,c_genre,queue++,bitrate);
      tracklist->appendItem((GMTrackItem*)item);
      }
    GMStreamTrackItem::max_trackno = tracklist->getFont()->getTextWidth(FXString('8',GMDBTrackItem::max_digits(queue)));
    }
  catch(GMDatabaseException & e){
    tracklist->clearItems();
    return false;
    }
  return true;
  }

long GMStreamSource::onCmdNewStation(FXObject*,FXSelector,void*){
  FXDialogBox dialog(GMPlayerManager::instance()->getMainWindow(),fxtr("New Internet Radio Station"),DECOR_TITLE|DECOR_BORDER|DECOR_RESIZE,0,0,0,0,0,0,0,0,0,0);
  GMPlayerManager::instance()->getMainWindow()->create_dialog_header(&dialog,fxtr("New Internet Radio Station"),fxtr("Specify url and description of new station"),NULL);
  FXHorizontalFrame *closebox=new FXHorizontalFrame(&dialog,LAYOUT_SIDE_BOTTOM|LAYOUT_FILL_X|PACK_UNIFORM_WIDTH,0,0,0,0);
  new GMButton(closebox,fxtr("C&reate"),NULL,&dialog,FXDialogBox::ID_ACCEPT,BUTTON_INITIAL|BUTTON_DEFAULT|LAYOUT_RIGHT|FRAME_RAISED|FRAME_THICK,0,0,0,0, 15,15);
  new GMButton(closebox,fxtr("&Cancel"),NULL,&dialog,FXDialogBox::ID_CANCEL,BUTTON_DEFAULT|LAYOUT_RIGHT|FRAME_RAISED|FRAME_THICK,0,0,0,0, 15,15);
  new FXSeparator(&dialog,SEPARATOR_GROOVE|LAYOUT_FILL_X|LAYOUT_SIDE_BOTTOM);
  FXVerticalFrame * main = new FXVerticalFrame(&dialog,LAYOUT_FILL_X|LAYOUT_FILL_Y,0,0,0,0,10,5,10,10);
  FXMatrix * matrix = new FXMatrix(main,2,LAYOUT_FILL_X|MATRIX_BY_COLUMNS);
  new FXLabel(matrix,fxtr("Location"),NULL,LABEL_NORMAL|LAYOUT_RIGHT|LAYOUT_CENTER_Y);
  GMTextField * location_field = new GMTextField(matrix,40,NULL,0,LAYOUT_FILL_X|LAYOUT_FILL_COLUMN|FRAME_SUNKEN|FRAME_THICK);
  new FXLabel(matrix,fxtr("Description"),NULL,LABEL_NORMAL|LAYOUT_RIGHT|LAYOUT_CENTER_Y);
  GMTextField * description_field = new GMTextField(matrix,30,NULL,0,LAYOUT_FILL_X|LAYOUT_FILL_COLUMN|FRAME_SUNKEN|FRAME_THICK);
  new FXLabel(matrix,fxtr("Tag"),NULL,LABEL_NORMAL|LAYOUT_RIGHT|LAYOUT_CENTER_Y);
  GMComboBox * tagbox = new GMComboBox(matrix,20,NULL,0,LAYOUT_FILL_X|FRAME_LINE);
  db->listTags(tagbox);
  tagbox->setSortFunc(genre_list_sort);
  tagbox->setNumVisible(FXMIN(10,tagbox->getNumItems()));
  tagbox->sortItems();
  tagbox->setCurrentItem(-1);
  if (dialog.execute()) {
    FXString url=location_field->getText().trim();
    FXString name=description_field->getText().trim();
    FXString genre=tagbox->getText().trim();
    if (!url.empty()) {
      if (genre.empty()) genre=fxtr("Untitled");
      db->insertStream(url,name,genre);
      GMPlayerManager::instance()->getTrackView()->refresh();
      }
    }
  return 1;
  }


long GMStreamSource::onCmdEditStation(FXObject*,FXSelector,void*){
  GMTextField * location_field=NULL;
  FXIntList tracks;
  GMPlayerManager::instance()->getTrackView()->getSelectedTracks(tracks);
  FXDialogBox dialog(GMPlayerManager::instance()->getMainWindow(),fxtr("Edit Internet Radio Station"),DECOR_TITLE|DECOR_BORDER|DECOR_RESIZE|DECOR_CLOSE,0,0,0,0,0,0,0,0,0,0);
  GMPlayerManager::instance()->getMainWindow()->create_dialog_header(&dialog,fxtr("Edit Internet Radio Station"),fxtr("Update url and description of station"),NULL);
  FXHorizontalFrame *closebox=new FXHorizontalFrame(&dialog,LAYOUT_SIDE_BOTTOM|LAYOUT_FILL_X|PACK_UNIFORM_WIDTH,0,0,0,0);
  new GMButton(closebox,fxtr("&Save"),NULL,&dialog,FXDialogBox::ID_ACCEPT,BUTTON_INITIAL|BUTTON_DEFAULT|LAYOUT_RIGHT|FRAME_RAISED|FRAME_THICK,0,0,0,0, 15,15);
  new GMButton(closebox,fxtr("&Cancel"),NULL,&dialog,FXDialogBox::ID_CANCEL,BUTTON_DEFAULT|LAYOUT_RIGHT|FRAME_RAISED|FRAME_THICK,0,0,0,0, 15,15);
  new FXSeparator(&dialog,SEPARATOR_GROOVE|LAYOUT_FILL_X|LAYOUT_SIDE_BOTTOM);
  FXVerticalFrame * main = new FXVerticalFrame(&dialog,LAYOUT_FILL_X|LAYOUT_FILL_Y,0,0,0,0,10,5,10,10);
  FXMatrix * matrix = new FXMatrix(main,2,LAYOUT_FILL_X|MATRIX_BY_COLUMNS);
  if (tracks.no()==1) {
    new FXLabel(matrix,fxtr("Location"),NULL,LABEL_NORMAL|LAYOUT_RIGHT|LAYOUT_CENTER_Y);
    location_field = new GMTextField(matrix,40,NULL,0,LAYOUT_FILL_X|LAYOUT_FILL_COLUMN|FRAME_SUNKEN|FRAME_THICK);
    }
  new FXLabel(matrix,fxtr("Description"),NULL,LABEL_NORMAL|LAYOUT_RIGHT|LAYOUT_CENTER_Y);
  GMTextField * description_field = new GMTextField(matrix,30,NULL,0,LAYOUT_FILL_X|LAYOUT_FILL_COLUMN|FRAME_SUNKEN|FRAME_THICK);
  new FXLabel(matrix,fxtr("Genre"),NULL,LABEL_NORMAL|LAYOUT_RIGHT|LAYOUT_CENTER_Y);
  GMComboBox * tagbox = new GMComboBox(matrix,20,NULL,0,LAYOUT_FILL_X|FRAME_LINE);
  db->listTags(tagbox);
  tagbox->setSortFunc(genre_list_sort);
  tagbox->setCurrentItem(-1);
  tagbox->setNumVisible(FXMIN(10,tagbox->getNumItems()));
  tagbox->sortItems();

  GMStream info;

  if (tracks.no()==1) {
    db->getStream(tracks[0],info);
    location_field->setText(info.url);
    description_field->setText(info.description);
    tagbox->setCurrentItem(tagbox->findItem(info.tag));
    }

  if (dialog.execute()) {
    FXbool changed=false;


    if (tracks.no()==1 && location_field->getText()!=info.url && !location_field->getText().empty()) {
      db->setStreamFilename(tracks[0],location_field->getText());
      changed=true;
      }

    if (description_field->getText()!=info.description && !description_field->getText().empty()){
      for (FXint i=0;i<tracks.no();i++)
        db->setStreamDescription(tracks[i],description_field->getText());
      changed=true;
      }

    if (tagbox->getText()!=info.tag && !tagbox->getText().empty()){
      for (FXint i=0;i<tracks.no();i++)
        db->setStreamGenre(tracks[i],tagbox->getText());
      changed=true;
      }

    if (changed) GMPlayerManager::instance()->getTrackView()->refresh();

    }
  return 1;
  }

long GMStreamSource::onCmdDeleteStation(FXObject*,FXSelector,void*){
  const FXString title=fxtr("Remove Internet Radio Station(s)?");
  const FXString subtitle=fxtr("Remove Internet Radio Station(s) from library?");
  FXDialogBox dialog(GMPlayerManager::instance()->getMainWindow(),title,DECOR_TITLE|DECOR_BORDER|DECOR_RESIZE|DECOR_CLOSE,0,0,0,0,0,0,0,0,0,0);
  GMPlayerManager::instance()->getMainWindow()->create_dialog_header(&dialog,title,subtitle,NULL);
  FXHorizontalFrame *closebox=new FXHorizontalFrame(&dialog,LAYOUT_SIDE_BOTTOM|LAYOUT_FILL_X|PACK_UNIFORM_WIDTH,0,0,0,0);
  new GMButton(closebox,fxtr("&Remove"),NULL,&dialog,FXDialogBox::ID_ACCEPT,BUTTON_INITIAL|BUTTON_DEFAULT|LAYOUT_RIGHT|FRAME_RAISED|FRAME_THICK,0,0,0,0, 15,15);
  new GMButton(closebox,fxtr("&Cancel"),NULL,&dialog,FXDialogBox::ID_CANCEL,BUTTON_DEFAULT|LAYOUT_RIGHT|FRAME_RAISED|FRAME_THICK,0,0,0,0, 15,15);
//  new FXSeparator(&dialog,SEPARATOR_GROOVE|LAYOUT_FILL_X|LAYOUT_SIDE_BOTTOM);
  if (dialog.execute()){
    FXIntList tracks;
    GMPlayerManager::instance()->getTrackView()->getSelectedTracks(tracks);
//    db->beginDelete();

    for (int i=0;i<tracks.no();i++){
      if (!db->removeStream(tracks[i])){
        FXMessageBox::error(GMPlayerManager::instance()->getMainWindow(),MBOX_OK,fxtr("Library Error"),fxtrformat("Unable to remove station from the library."));
        }
      }

//    db->endDelete();
    GMPlayerManager::instance()->getTrackView()->refresh();
    }
  return 1;
  }


long GMStreamSource::onUpdExport(FXObject*sender,FXSelector,void*){
  sender->handle(this,FXSEL(SEL_COMMAND,FXWindow::ID_DISABLE),NULL);
  return 1;
  }


