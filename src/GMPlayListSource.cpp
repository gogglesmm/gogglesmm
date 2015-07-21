/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2006-2015 by Sander Jansen. All Rights Reserved      *
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
#include "GMList.h"
#include "GMDatabase.h"
#include "GMTrackDatabase.h"
#include "GMTrackList.h"
#include "GMTrackItem.h"
#include "GMTrackView.h"
#include "GMSourceView.h"
#include "GMSource.h"
#include "GMDatabaseSource.h"
#include "GMPlayListSource.h"
#include "GMPlayerManager.h"
#include "GMTaskManager.h"
#include "GMWindow.h"
#include "GMScanner.h"
#include "GMImportDialog.h"
#include "GMIconTheme.h"
#include "GMClipboard.h"
#include "GMAudioPlayer.h"


void getSelectedTrackQueues(FXIntList & list) {
  FXint nitems = GMPlayerManager::instance()->getTrackView()->getNumTracks();
  for (FXint i=0;i<nitems;i++){
    if (GMPlayerManager::instance()->getTrackView()->isTrackItemSelected(i))
      list.append(dynamic_cast<GMDBTrackItem*>(GMPlayerManager::instance()->getTrackView()->getTrackItem(i))->getTrackQueue());
    }
  }

void getTrackQueues(FXIntList & list) {
  FXint nitems = GMPlayerManager::instance()->getTrackView()->getNumTracks();
  list.no(nitems);
  for (FXint i=0;i<nitems;i++){
    list[i]=dynamic_cast<GMDBTrackItem*>(GMPlayerManager::instance()->getTrackView()->getTrackItem(i))->getTrackQueue();
    }
  }



FXDEFMAP(GMPlayListSource) GMPlayListSourceMap[]={
  FXMAPFUNC(SEL_COMMAND,GMPlayListSource::ID_EDIT_NAME,GMPlayListSource::onCmdEditName),
  FXMAPFUNC(SEL_COMMAND,GMPlayListSource::ID_REMOVE,GMPlayListSource::onCmdRemove),
  FXMAPFUNC(SEL_COMMAND,GMPlayListSource::ID_DELETE_TAG,GMPlayListSource::onCmdRemoveInPlaylist),
  FXMAPFUNC(SEL_COMMAND,GMPlayListSource::ID_DELETE_ARTIST,GMPlayListSource::onCmdRemoveInPlaylist),
  FXMAPFUNC(SEL_COMMAND,GMPlayListSource::ID_DELETE_ALBUM,GMPlayListSource::onCmdRemoveInPlaylist),
  FXMAPFUNC(SEL_COMMAND,GMPlayListSource::ID_DELETE_TRACK,GMPlayListSource::onCmdRemoveInPlaylist),
  FXMAPFUNC(SEL_COMMAND,GMPlayListSource::ID_DELETE_TAG_ADV,GMPlayListSource::onCmdRemoveInPlaylist),
  FXMAPFUNC(SEL_COMMAND,GMPlayListSource::ID_DELETE_ARTIST_ADV,GMPlayListSource::onCmdRemoveInPlaylist),
  FXMAPFUNC(SEL_COMMAND,GMPlayListSource::ID_DELETE_ALBUM_ADV,GMPlayListSource::onCmdRemoveInPlaylist),
  FXMAPFUNC(SEL_COMMAND,GMPlayListSource::ID_DELETE_TRACK_ADV,GMPlayListSource::onCmdRemoveInPlaylist),
  FXMAPFUNC(SEL_COMMAND,GMPlayListSource::ID_IMPORT,GMPlayListSource::onCmdImport),
  };

FXIMPLEMENT(GMPlayListSource,GMDatabaseSource,GMPlayListSourceMap,ARRAYNUMBER(GMPlayListSourceMap));


GMPlayListSource::GMPlayListSource(GMTrackDatabase * database,FXint pl) : GMDatabaseSource(database),current_queue(-1) {
  FXASSERT(pl);
  playlist=pl;
  db->getPlaylistName(playlist,name);
  orderchanged=false;
  }

GMPlayListSource::~GMPlayListSource() {
  }




void GMPlayListSource::save(GMTrackList* tracklist) {
  if (orderchanged) {
    GMPlayListItemList items;
    if (tracklist->getNumItems()) {
      items.no(tracklist->getNumItems());
      if (tracklist->getSortMethod()==HEADER_QUEUE){
        for (FXint i=0;i<tracklist->getNumItems();i++){
          const GMDBTrackItem * trk = dynamic_cast<GMDBTrackItem*>(tracklist->getItem(i));
          items[i].queue = trk->getTrackQueue();
          items[i].track = trk->getId();
          }
        }
      else {
        for (FXint i=0;i<tracklist->getNumItems();i++){
          const GMDBTrackItem * trk = dynamic_cast<GMDBTrackItem*>(tracklist->getItem(i));
          items[i].queue = i+1;
          items[i].track = trk->getId();
          }
        tracklist->setSortMethod(HEADER_QUEUE);
        }
      db->updatePlaylist(playlist,items);

      /// write back to database.
      orderchanged=false;
      }
    }
  }


void GMPlayListSource::sorted(GMTrackList*tracklist,FXint method) {
  if (orderchanged) {
    if (tracklist->getSortMethod()==HEADER_QUEUE && method!=HEADER_QUEUE) {
      save(tracklist);
      }
    }
  }


void GMPlayListSource::dragged(GMTrackList*tracklist){
  FXint nitems = tracklist->getNumItems();
  orderchanged=true;
  if (tracklist->getSortMethod()==HEADER_QUEUE){
    if (tracklist->getSortFunc()==GMDBTrackItem::descendingQueue) {
      for (FXint i=0;i<nitems;i++){
        dynamic_cast<GMDBTrackItem*>(tracklist->getItem(i))->setTrackQueue(nitems-i);
        }
      }
    else {
      for (FXint i=0;i<nitems;i++){
        dynamic_cast<GMDBTrackItem*>(tracklist->getItem(i))->setTrackQueue(i+1);
        }
      }
    }
  else {
    tracklist->markUnsorted();
    }
  }


FXbool GMPlayListSource::findCurrent(GMTrackList * list,GMSource * src) {
  if (src->getCurrentTrack()==-1) return false;
  if (src==this) {
    for (FXint i=0;i<list->getNumItems();i++){
      if (list->getItemId(i)==current_track && dynamic_cast<GMDBTrackItem*>(list->getItem(i))->getTrackQueue()==current_queue) {
        list->setActiveItem(i);
        list->setCurrentItem(i);
        return true;
        }
      }
    }
  else {
    GMDatabaseSource::findCurrent(list,src);
    }
  return false;
  }

FXbool GMPlayListSource::hasCurrentTrack(GMSource * src) const {
  if (src==this) return true;
  else if (db->trackInPlaylist(src->getCurrentTrack(),playlist)) return true;
  return false;
  }


void GMPlayListSource::markCurrent(const GMTrackItem*item) {
  if (item) {
    current_track = item->getId();
    current_queue = dynamic_cast<const GMDBTrackItem*>(item)->getTrackQueue();
    }
  else {
    current_track = -1;
    current_queue = -1;
    }
  }


FXbool GMPlayListSource::source_context_menu(FXMenuPane * pane){
  new GMMenuCommand(pane,fxtr("Rename…"),GMIconTheme::instance()->icon_edit,this,GMPlayListSource::ID_EDIT_NAME);
  new GMMenuCommand(pane,fxtr("Import Playlist…"),GMIconTheme::instance()->icon_import,this,GMPlayListSource::ID_IMPORT);
  new GMMenuCommand(pane,fxtr("Export As…"),GMIconTheme::instance()->icon_export,this,GMPlayListSource::ID_EXPORT);
  new GMMenuCommand(pane,fxtr("Remove Playlist"),GMIconTheme::instance()->icon_delete,this,GMPlayListSource::ID_REMOVE);
  return true;
  }


FXbool GMPlayListSource::dnd_accepts(FXDragType*types,FXuint ntypes){
  FXWindow * src = FXApp::instance()->getDragWindow();
  for (FXuint i=0;i<ntypes;i++){
    if (types[i]==GMClipboard::kdeclipboard && !src) return true;
    else if (types[i]==FXWindow::urilistType && !src) return true;
    else if (types[i]==GMClipboard::alltracks) return true;
    else if (types[i]==GMClipboard::selectedtracks) return true;
    }
  return false;
  }


class FromDiskTarget : public FXObject {
FXDECLARE(FromDiskTarget)
protected:
  FXCheckButton * from_library;
protected:
  FromDiskTarget(){}
private:
  FromDiskTarget(const FromDiskTarget&);
  FromDiskTarget& operator=(const FromDiskTarget&);
public:
  enum {
    ID_FROM_DISK = 1
    };
public:

  FromDiskTarget(FXCheckButton*disk,FXCheckButton*library) : from_library(library) {
    disk->setTarget(this);
    disk->setSelector(FromDiskTarget::ID_FROM_DISK);
    }

  long onUpdFromDisk(FXObject*sender,FXSelector,void*) {
    if (from_library->getCheck())
      sender->handle(this,FXSEL(SEL_COMMAND,FXWindow::ID_ENABLE),nullptr);
    else
      sender->handle(this,FXSEL(SEL_COMMAND,FXWindow::ID_DISABLE),nullptr);
    return 1;
    }
  };

FXDEFMAP(FromDiskTarget) FromDiskTargetMap[]={
  FXMAPFUNC(SEL_UPDATE,FromDiskTarget::ID_FROM_DISK,FromDiskTarget::onUpdFromDisk)
  };

FXIMPLEMENT(FromDiskTarget,FXObject,FromDiskTargetMap,ARRAYNUMBER(FromDiskTargetMap))


long GMPlayListSource::onCmdRemoveInPlaylist(FXObject*,FXSelector sel,void*){
  FXIntList queue;
  FXIntList tracks;
  FXStringList files;

  if (FXSELID(sel)==ID_DELETE_TRACK || FXSELID(sel)==ID_DELETE_TRACK_ADV) {
    getSelectedTrackQueues(queue);
    GMPlayerManager::instance()->getTrackView()->getSelectedTracks(tracks);
    }
  else {
    getTrackQueues(queue);
    GMPlayerManager::instance()->getTrackView()->getTracks(tracks);
    }

  if (tracks.no()==0)
    return 1;


  FXbool from_library=false;
  //FXbool from_disk=false;

  if (FXSELID(sel)>ID_DELETE_TRACK) {

    FXString title;
    FXString subtitle;

    switch(FXSELID(sel)){
      case ID_DELETE_TAG_ADV    : title=fxtr("Remove Tag?");
                                  subtitle=fxtr("Remove tracks with tag from play list?");
                                  break;
      case ID_DELETE_ARTIST_ADV : title=fxtr("Remove Artist?");
                                  subtitle=fxtr("Remove tracks from artist from play list?");
                                  break;
      case ID_DELETE_ALBUM_ADV  : title=fxtr("Remove Album?");
                                  subtitle=fxtr("Remove tracks from album from play list?");
                                  break;
      case ID_DELETE_TRACK_ADV  : title=fxtr("Remove Track(s)?");
                                  subtitle=fxtr("Remove track(s) from play list?");
                                  break;
      default: FXASSERT(0); break;
      }

    FXDialogBox dialog(GMPlayerManager::instance()->getMainWindow(),title,DECOR_TITLE|DECOR_BORDER|DECOR_RESIZE|DECOR_CLOSE,0,0,0,0,0,0,0,0,0,0);
    GMPlayerManager::instance()->getMainWindow()->create_dialog_header(&dialog,title,subtitle,nullptr);
    FXHorizontalFrame *closebox=new FXHorizontalFrame(&dialog,LAYOUT_SIDE_BOTTOM|LAYOUT_FILL_X|PACK_UNIFORM_WIDTH,0,0,0,0);
    new GMButton(closebox,fxtr("&Remove"),nullptr,&dialog,FXDialogBox::ID_ACCEPT,BUTTON_INITIAL|BUTTON_DEFAULT|LAYOUT_RIGHT|FRAME_RAISED|FRAME_THICK,0,0,0,0, 15,15);
    new GMButton(closebox,fxtr("&Cancel"),nullptr,&dialog,FXDialogBox::ID_CANCEL,BUTTON_DEFAULT|LAYOUT_RIGHT|FRAME_RAISED|FRAME_THICK,0,0,0,0, 15,15);
    new FXSeparator(&dialog,SEPARATOR_GROOVE|LAYOUT_FILL_X|LAYOUT_SIDE_BOTTOM);
    FXVerticalFrame * main = new FXVerticalFrame(&dialog,LAYOUT_FILL_X|LAYOUT_FILL_Y,0,0,0,0,30,20,10,10);
    FXCheckButton * library_check = new FXCheckButton(main,fxtr("Remove tracks from music library"));
    FXCheckButton * disk_check = new FXCheckButton(main,fxtr("Remove tracks from disk"));
    FromDiskTarget disktgt(disk_check,library_check);

    if (!dialog.execute())
      return 1;

    from_library=library_check->getCheck();
    //from_disk=disk_check->getCheck();
    }

  // Check current queue...
  if (current_queue >= 0)  {
    for (FXint i=0;i<queue.no();i++){
      if (current_queue==queue[i]) {
        current_queue=-1;
        break;
        }
      }
    }

  try {
    db->begin();
    if (from_library)
      db->removeTracks(tracks);
    else if (GMPlayerManager::instance()->getTrackView()->hasBrowser())
      db->removePlaylistTracks(playlist,tracks);
    else
      db->removePlaylistQueue(playlist,queue);
    db->commit();
    }
  catch(GMDatabaseException&) {
    db->rollback();
    }
  GMPlayerManager::instance()->getTrackView()->refresh();
  return 1;
  }



long GMPlayListSource::onCmdImport(FXObject*,FXSelector,void*){
  GMImportDialog dialog(GMPlayerManager::instance()->getMainWindow(),IMPORT_FROMFILE|IMPORT_PLAYLIST);
  if (dialog.execute()){

    FXString buffer;
    FXStringList urls;
    FXString title;

    if (gm_buffer_file(dialog.getFilename(),buffer)) {

      FXString extension = FXPath::extension(dialog.getFilename());

      if (comparecase(extension,"m3u")==0)
        ap_parse_m3u(buffer,urls);
      else if (comparecase(extension,"pls")==0)
        ap_parse_pls(buffer,urls);
      else
        ap_parse_xspf(buffer,urls,title);

      if (urls.no()) {
        gm_make_absolute_path(FXPath::directory(dialog.getFilename()),urls);

        GMImportTask * task = new GMImportTask(GMPlayerManager::instance(),GMPlayerManager::ID_IMPORT_TASK);
        task->setOptions(GMPlayerManager::instance()->getPreferences().import);
        task->setInput(urls);
        task->setPlaylist(playlist);
        GMPlayerManager::instance()->runTask(task);
        }
      }
    }
  return 1;
  }


long GMPlayListSource::onCmdEditName(FXObject*,FXSelector,void *){
  FXDialogBox dialog(GMPlayerManager::instance()->getMainWindow(),fxtr("Edit Playlist"),DECOR_TITLE|DECOR_BORDER|DECOR_RESIZE,0,0,0,0,0,0,0,0,0,0);
  GMPlayerManager::instance()->getMainWindow()->create_dialog_header(&dialog,fxtr("Edit Playlist"),fxtr("Change playlist name"));
  FXHorizontalFrame *closebox=new FXHorizontalFrame(&dialog,LAYOUT_SIDE_BOTTOM|LAYOUT_FILL_X|PACK_UNIFORM_WIDTH,0,0,0,0);
  new GMButton(closebox,fxtr("&Save"),nullptr,&dialog,FXDialogBox::ID_ACCEPT,BUTTON_INITIAL|BUTTON_DEFAULT|LAYOUT_RIGHT|FRAME_RAISED|FRAME_THICK,0,0,0,0, 15,15);
  new GMButton(closebox,fxtr("&Cancel"),nullptr,&dialog,FXDialogBox::ID_CANCEL,BUTTON_DEFAULT|LAYOUT_RIGHT|FRAME_RAISED|FRAME_THICK,0,0,0,0, 15,15);
  new FXSeparator(&dialog,SEPARATOR_GROOVE|LAYOUT_FILL_X|LAYOUT_SIDE_BOTTOM);
  FXVerticalFrame * main = new FXVerticalFrame(&dialog,LAYOUT_FILL_X|LAYOUT_FILL_Y,0,0,0,0,30,20,10,10);
  FXMatrix * matrix = new FXMatrix(main,2,LAYOUT_FILL_X|MATRIX_BY_COLUMNS);
  new FXLabel(matrix,fxtr("Name"),nullptr,LABEL_NORMAL|LAYOUT_RIGHT|LAYOUT_CENTER_Y);
  GMTextField * name_field = new GMTextField(matrix,20,&dialog,FXDialogBox::ID_ACCEPT,LAYOUT_FILL_X|LAYOUT_FILL_COLUMN|FRAME_SUNKEN|FRAME_THICK|TEXTFIELD_ENTER_ONLY);
  name_field->setText(getName());
  dialog.create();
  gm_focus_and_select(name_field);
  if (dialog.execute()) {
    FXString label= name_field->getText().trim();
    if (!label.empty()) {
      db->setPlaylistName(playlist,label);
      name=label;
      GMPlayerManager::instance()->getSourceView()->updateSource(this);
      }
    }
  return 1;
  }


long GMPlayListSource::onCmdRemove(FXObject*,FXSelector,void *){
  if (GMPlayerManager::instance()->getMainWindow()->question(fxtr("Delete Play List?"),fxtr("Are you sure you want to delete the playlist?"),fxtr("Yes"),fxtr("No"))){
    if (db->removePlaylist(playlist)) {
      FXApp::instance()->reg().deleteSection(settingKey().text());
      GMPlayerManager::instance()->removeSource(this);
      }
    else {
      // FIXME show error?
      }
    }
  return 1;
  }


