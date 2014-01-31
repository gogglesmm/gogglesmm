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
#include "GMClipboard.h"


FXDEFMAP(GMClipboard) GMClipboardMap[]={
  FXMAPFUNC(SEL_CLIPBOARD_LOST,0,GMClipboard::onClipboardLost),
  FXMAPFUNC(SEL_CLIPBOARD_GAINED,0,GMClipboard::onClipboardGained),
  FXMAPFUNC(SEL_CLIPBOARD_REQUEST,0,GMClipboard::onClipboardRequest),
  };

FXIMPLEMENT(GMClipboard,FXShell,GMClipboardMap,ARRAYNUMBER(GMClipboardMap))

GMClipboard * GMClipboard::me=NULL;

FXDragType GMClipboard::kdeclipboard=0;
FXDragType GMClipboard::gnomeclipboard=0;
FXDragType GMClipboard::gnomedragndrop=0;
FXDragType GMClipboard::trackdatabase=0;
FXDragType GMClipboard::selectedtracks=0;
FXDragType GMClipboard::alltracks=0;

GMClipboard * GMClipboard::instance(){
  return me;
  }

GMClipboard::GMClipboard() {
  }

GMClipboard::GMClipboard(FXApp * app) : FXShell(app,0,0,0,0,0), clipdata(NULL), clipowner(NULL) {
  me=this;
  }

void GMClipboard::create(){
  FXShell::create();

  kdeclipboard   = getApp()->registerDragType("application/x-kde-cutselection");
  gnomeclipboard = getApp()->registerDragType("x-special/gnome-copied-files");
  gnomedragndrop = getApp()->registerDragType("x-special/gnome-icon-list");
  trackdatabase  = getApp()->registerDragType("application/goggles-music-manager-database");
  selectedtracks = getApp()->registerDragType("application/goggles-dnd-selected-tracks");
  alltracks      = getApp()->registerDragType("application/goggles-dnd-all-tracks");

  if (FXWindow::urilistType==0){
    FXWindow::urilistType=getApp()->registerDragType(FXWindow::urilistTypeName);
    }
  }

GMClipboard::~GMClipboard(){
  if (clipdata) delete clipdata;
  clipdata=NULL;
  clipowner=NULL;
  }

bool GMClipboard::doesOverrideRedirect() const {
  return true;
  }

FXbool GMClipboard::acquire(FXObject * owner,const FXDragType * types,FXuint num_types,GMClipboardData * data){
  if (acquireClipboard(types,num_types)){
//    fxmessage("acquired clipboard %d\n",hasClipboard());
    clipowner=owner;
    clipdata=data;
    return true;
    }
  return false;
  }

FXbool GMClipboard::owned(FXObject * obj){
  if (hasClipboard() && obj==clipowner) return true;
  return false;
  }

FXbool GMClipboard::release(){
  return false;
  }


long GMClipboard::onClipboardLost(FXObject*,FXSelector,void*){
  //fxmessage("lost clipboard\n");
  if (clipdata) delete clipdata;
  clipdata=NULL;
  clipowner=NULL;
  return 1;
  }

long GMClipboard::onClipboardGained(FXObject*,FXSelector,void*){
 // fxmessage("gained clipboard\n");
  return 1;
  }


long GMClipboard::onClipboardRequest(FXObject*,FXSelector,void*ptr){
  FXEvent *event=(FXEvent*)ptr;
  //fxmessage("got request: %s\n",getApp()->getDragTypeName(event->target).text());
  if (clipdata && clipdata->request(event->target,this)){
    return 1;
    }
  return 0;
  }








