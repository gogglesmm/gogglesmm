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
#include <fxkeys.h>
#include "GMTrack.h"
#include "GMApp.h"
#include "GMList.h"
#include "GMTrackList.h"
#include "GMSourceView.h"
#include "GMTrackView.h"
#include "GMSource.h"
#include "GMDatabaseSource.h"
#include "GMStreamSource.h"
#include "GMPlayListSource.h"
#include "GMPlayerManager.h"
#include "GMWindow.h"
#include "GMIconTheme.h"



FXDEFMAP(GMSourceView) GMSourceViewMap[]={
  FXMAPFUNC(SEL_COMMAND,GMSourceView::ID_SOURCE_LIST_HEADER,GMSourceView::onCmdSortSourceList),
  FXMAPFUNC(SEL_RIGHTBUTTONRELEASE,GMSourceView::ID_SOURCE_LIST,GMSourceView::onSourceContextMenu),
  FXMAPFUNC(SEL_COMMAND,GMSourceView::ID_SOURCE_LIST,GMSourceView::onCmdSourceSelected),
  FXMAPFUNC(SEL_DND_MOTION,GMSourceView::ID_SOURCE_LIST,GMSourceView::onDndSourceMotion),
  FXMAPFUNC(SEL_DND_DROP,GMSourceView::ID_SOURCE_LIST,GMSourceView::onDndSourceDrop),
  FXMAPFUNC(SEL_COMMAND,GMSourceView::ID_NEW_STATION,GMSourceView::onCmdNewStation),
  FXMAPFUNC(SEL_COMMAND,GMSourceView::ID_EXPORT,GMSourceView::onCmdExport),
  FXMAPFUNC(SEL_UPDATE,GMSourceView::ID_EXPORT,GMSourceView::onUpdExport),
  FXMAPFUNC(SEL_QUERY_TIP,GMSourceView::ID_SOURCE_LIST,GMSourceView::onSourceTipText),
  };

FXIMPLEMENT(GMSourceView,GMScrollFrame,GMSourceViewMap,ARRAYNUMBER(GMSourceViewMap))

GMSourceView::GMSourceView() : source(NULL) {
  }

GMSourceView::GMSourceView(FXComposite* p) : GMScrollFrame(p) , source(NULL) {
  sourcelistheader = new GMHeaderButton(this,tr("Sources\tPress to change sorting order\tPress to change sorting order"),NULL,this,ID_SOURCE_LIST_HEADER,LAYOUT_FILL_X|FRAME_RAISED|JUSTIFY_LEFT);
  sourcelist       = new GMTreeList(this,this,ID_SOURCE_LIST,LAYOUT_FILL_X|LAYOUT_FILL_Y|TREELIST_BROWSESELECT);

  sourcelist->dropEnable();
  sourcelist->setSortFunc(source_list_sort);

  sourcelistheader->setArrowState(ARROW_DOWN);

  updateColors();
  }

GMSourceView::~GMSourceView(){
  }

FXbool GMSourceView::focusNext() {
  sourcelist->setFocus();
  return true;
  }

FXbool GMSourceView::focusPrevious() {
  sourcelist->setFocus();
  return true;
  }


void GMSourceView::updateColors() {
  sourcelist->setRowColor(GMPlayerManager::instance()->getPreferences().gui_row_color);
  }

void GMSourceView::updateSource(GMSource * src){
  FXTreeItem * item = sourcelist->getFirstItem();
  while(item) {
    if (item->getData()==src) {
      item->setText(tr(src->getName().text()));
      break;
      }
    item=item->getNext();
    }
  resort();
  }


void GMSourceView::setSource(GMSource * src,FXbool makecurrent/*=true*/){
  if (src!=source) {
    source=src;
    if (makecurrent) {
      FXTreeItem * item = sourcelist->getFirstItem();
      while(item) {
        if (item->getData()==src) {
          sourcelist->setCurrentItem(item,false);
          break;
          }
        item=item->getNext();
        }
      }
    GMPlayerManager::instance()->getTrackView()->setSource(source);
    }
  }


void GMSourceView::clear() {
  sourcelist->clearItems();
  }


void GMSourceView::refresh() {
  clear();
  listSources();
  }


/// Perhaps member of icon theme?
static FXIcon * icon_for_sourcetype(FXint type) {
  switch(type){
    case SOURCE_DATABASE          : return GMIconTheme::instance()->icon_source_library; break;
    case SOURCE_INTERNET_RADIO    : return GMIconTheme::instance()->icon_source_internetradio; break;
    case SOURCE_DATABASE_PLAYLIST : return GMIconTheme::instance()->icon_source_playlist; break;
    case SOURCE_PLAYQUEUE         : return GMIconTheme::instance()->icon_source_playqueue; break;
    case SOURCE_FILESYSTEM        : return GMIconTheme::instance()->icon_source_local; break;
    case SOURCE_PODCAST           : return GMIconTheme::instance()->icon_source_podcast; break;
    default                       : break;
    }
  return NULL;
  }


void GMSourceView::refresh(GMSource * src) {
  GMTreeItem * item = (GMTreeItem*)sourcelist->findItemByData(src);
  if (item) {
    FXIcon * icon=icon_for_sourcetype(src->getType());
    sourcelist->setItemText(item,src->getName());
    sourcelist->setItemOpenIcon(item,icon);
    sourcelist->setItemClosedIcon(item,icon);
    }
  }


void GMSourceView::init() {
  loadSettings("window");
  clear();
  listsources();


  FXString key = getApp()->reg().readStringEntry("window","source-list-current","");
  if (!key.empty()){
    FXTreeItem * item = sourcelist->getFirstItem();
    while(item) {
      GMSource * src = (GMSource*)item->getData();
      if (src->settingKey()==key) {
        sourcelist->setCurrentItem(item);
        break;
        }
      item=item->getNext();
      }
    }

  if (sourcelist->getCurrentItem()==NULL && sourcelist->getFirstItem())
    sourcelist->setCurrentItem(sourcelist->getFirstItem());

  source=(GMSource*)sourcelist->getItemData(sourcelist->getCurrentItem());
  GMPlayerManager::instance()->getTrackView()->init(source);
  }


void GMSourceView::resort() {
  sortSources();
  }


FXbool GMSourceView::listsources() {
  GMTreeItem * item=NULL;
  for (FXint i=0;i<GMPlayerManager::instance()->getNumSources();i++){
    GMSource * src = GMPlayerManager::instance()->getSource(i);
    FXIcon * icon=icon_for_sourcetype(src->getType());
    item = new GMTreeItem(src->getName(),icon,icon,src);
    sourcelist->appendItem(NULL,item);
    }
  sourcelist->sortItems();
  return true;
  }


FXbool GMSourceView::listSources() {
  listsources();
  setSource((GMSource*)sourcelist->getItemData(sourcelist->getCurrentItem()),false);
  return true;
  }


void GMSourceView::sortSources() const{
  sourcelist->sortItems();
  }


void GMSourceView::loadSettings(const FXString & key) {
  FXbool sort_reverse,view;

  sort_reverse = getApp()->reg().readBoolEntry(key.text(),"source-list-sort-reverse",false);
  if (sort_reverse)
    sourcelist->setSortFunc(source_list_sort_reverse);
  else
    sourcelist->setSortFunc(source_list_sort);

  view = getApp()->reg().readBoolEntry(key.text(),"source-list",true);
  if (view)
    getParent()->show();
  else
    getParent()->hide();
  }


void GMSourceView::saveSettings(const FXString & key) const {
  getApp()->reg().writeBoolEntry(key.text(),"source-list-sort-reverse",sourcelist->getSortFunc()==source_list_sort_reverse);
  getApp()->reg().writeBoolEntry(key.text(),"source-list",getParent()->shown());
  }



void GMSourceView::saveView() const {
  saveSettings("window");
  if (source) {
    getApp()->reg().writeStringEntry("window","source-list-current",source->settingKey().text());
    }
  }


long GMSourceView::onCmdSourceSelected(FXObject*,FXSelector,void*){
  FXTreeItem * item = sourcelist->getCurrentItem();
  if (item) {
    setSource((GMSource*)item->getData(),false);
    }
  return 1;
  }


long GMSourceView::onCmdSortSourceList(FXObject*,FXSelector,void*){
  if (sourcelist->getSortFunc()==source_list_sort) {
    sourcelist->setSortFunc(source_list_sort_reverse);
    sourcelistheader->setArrowState(ARROW_UP);
    }
  else {
    sourcelist->setSortFunc(source_list_sort);
    sourcelistheader->setArrowState(ARROW_DOWN);
    }
  sortSources();
  return 1;
  }

long GMSourceView::onSourceTipText(FXObject*sender,FXSelector,void*ptr){
  FXint x,y; FXuint buttons;
  sourcelist->getCursorPosition(x,y,buttons);
  FXTreeItem * item = sourcelist->getItemAt(x,y);
  if (item && item->getData()) {
    GMSource * src = (GMSource*)item->getData();
    return src->handle(sender,FXSEL(SEL_QUERY_TIP,0),ptr);
    }
  return 0;
  }

long GMSourceView::onSourceContextMenu(FXObject*,FXSelector,void*ptr){
  FXEvent * event =static_cast<FXEvent*>(ptr);
  if (event->moved) return 0;
  GMTreeItem * item = dynamic_cast<GMTreeItem*>(sourcelist->getItemAt(event->win_x,event->win_y));
  GMMenuPane pane(this);
  GMSource * src = item ? static_cast<GMSource*>(item->getData()) : NULL;
  FXbool src_items = false;

  if (src)
    src_items = src->source_context_menu(&pane);

  if (src && src->canBrowse()) {
    if (src_items) new FXMenuSeparator(&pane);
    new GMMenuCheck(&pane,tr("Show Browser\tCtrl-B\tShow Browser"),GMPlayerManager::instance()->getTrackView(),GMTrackView::ID_TOGGLE_BROWSER);
    new GMMenuCheck(&pane,tr("Show Tags\tCtrl-T\tShow Tags"),GMPlayerManager::instance()->getTrackView(),GMTrackView::ID_TOGGLE_TAGS);
    }

  // Install Source Items (Group by source)
  if (src==NULL || src_items==false) {
    FXint nadded=(&pane)->numChildren();
    FXint nlast=(&pane)->numChildren();
    for (FXint i=0;i<GMPlayerManager::instance()->getNumSources();i++) {
      if (nadded>1) {
        new FXMenuSeparator(&pane);
        nadded=0;
        nlast+=1;
        }
      if (GMPlayerManager::instance()->getSource(i)->source_menu(&pane)){
        FXint n = (&pane)->numChildren();
        nadded = n - nlast;
        nlast  = n;
        }
      }
    }

  if (item) {
    sourcelist->setCurrentItem(item);
    onCmdSourceSelected(NULL,0,NULL); // Simulate SEL_COMMAND
    }

  if (pane.getFirst()){
    pane.create();
    ewmh_change_window_type(&pane,WINDOWTYPE_POPUP_MENU);
    pane.popup(NULL,event->root_x,event->root_y);
    getApp()->runPopup(&pane);
    }
  return 1;
  }


long GMSourceView::onDndSourceMotion(FXObject*,FXSelector,void*ptr){
  FXEvent * event = static_cast<FXEvent*>(ptr);
  GMTreeItem * item = dynamic_cast<GMTreeItem*>(sourcelist->getItemAt(event->win_x,event->win_y));
  if (item) {
    GMSource * src = static_cast<GMSource *>(item->getData());
    FXDragType*types;
    FXuint     ntypes;
    if (sourcelist->inquireDNDTypes(FROM_DRAGNDROP,types,ntypes)){
      if (src->dnd_accepts(types,ntypes)){
        sourcedrop=source;
        sourcelist->acceptDrop(DRAG_ACCEPT);
        freeElms(types);
        return 1;
        }
      freeElms(types);
      }
    }
  sourcedrop=NULL;
  return 0;
  }

long GMSourceView::onDndSourceDrop(FXObject*,FXSelector,void*ptr){
  if (sourcedrop) {
    long code =  sourcedrop->handle(this,FXSEL(SEL_DND_DROP,GMSource::ID_DROP),ptr);
    sourcedrop=NULL;
    return code;
    }
  return 0;
  }

long GMSourceView::onCmdNewStation(FXObject*sender,FXSelector,void*ptr){
  for (FXint i=0;i<GMPlayerManager::instance()->getNumSources();i++){
    GMSource * src = GMPlayerManager::instance()->getSource(i);
    if (src->getType()==SOURCE_INTERNET_RADIO)
      return src->handle(sender,FXSEL(SEL_COMMAND,GMStreamSource::ID_NEW_STATION),ptr);
    }
  return 0;
  }

long GMSourceView::onCmdExport(FXObject*sender,FXSelector,void*ptr){
  if (source)
    return source->handle(sender,FXSEL(SEL_COMMAND,GMSource::ID_EXPORT),ptr);
  return 0;
  }


long GMSourceView::onUpdExport(FXObject*sender,FXSelector,void*ptr){
  if (source)
    return source->handle(sender,FXSEL(SEL_UPDATE,GMSource::ID_EXPORT),ptr);
  return 0;
  }

