/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2006-2018 by Sander Jansen. All Rights Reserved      *
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

#include "xincs.h"
#include "GMClipboard.h"

#include <FX88591Codec.h>
#include <FXUTF16Codec.h>


class GMTextData : public GMClipboardData {
public:
  FXString data;

  GMTextData(){}

  virtual FXbool request(FXDragType target, GMClipboard * clipboard) {

    if (target == FXWindow::utf8Type){
      clipboard->setDNDData(FROM_CLIPBOARD, target, data);
      return true;
      }

    if(target == FXWindow::stringType || target == FXWindow::textType){
      FX88591Codec ascii;
      clipboard->setDNDData(FROM_CLIPBOARD, target, ascii.utf2mb(data));
      return true;
      }

    if(target == FXWindow::utf16Type){
      FXUTF16LECodec unicode;
      clipboard->setDNDData(FROM_CLIPBOARD, target, unicode.utf2mb(data));
      return 1;
      }

    return false;
    }
  };


FXDEFMAP(GMClipboard) GMClipboardMap[]={
  FXMAPFUNC(SEL_CLIPBOARD_LOST,0,GMClipboard::onClipboardLost),
  FXMAPFUNC(SEL_CLIPBOARD_GAINED,0,GMClipboard::onClipboardGained),
  FXMAPFUNC(SEL_CLIPBOARD_REQUEST,0,GMClipboard::onClipboardRequest),
  };

FXIMPLEMENT(GMClipboard,FXShell,GMClipboardMap,ARRAYNUMBER(GMClipboardMap))

GMClipboard * GMClipboard::me=nullptr;

FXDragType GMClipboard::kdeclipboard=0;
FXDragType GMClipboard::gnomeclipboard=0;
FXDragType GMClipboard::gnomedragndrop=0;
FXDragType GMClipboard::trackdatabase=0;
FXDragType GMClipboard::selectedtracks=0;
FXDragType GMClipboard::alltracks=0;
FXDragType GMClipboard::theclipboard=0;

GMClipboard * GMClipboard::instance(){
  return me;
  }

GMClipboard::GMClipboard() {
  }

GMClipboard::GMClipboard(FXApp * a) : FXShell(a,0,0,0,0,0) {
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
  theclipboard   = getApp()->registerDragType("CLIPBOARD");

  if (FXWindow::urilistType==0){
    FXWindow::urilistType=getApp()->registerDragType(FXWindow::urilistTypeName);
    }
  }

GMClipboard::~GMClipboard(){
  if (clipdata) delete clipdata;
  clipdata=nullptr;
  clipowner=nullptr;
  }

bool GMClipboard::doesOverrideRedirect() const {
  return true;
  }

FXbool GMClipboard::acquire(FXObject * o,const FXDragType * types,FXuint num_types,GMClipboardData * d){
  if (acquireClipboard(types,num_types)){
    clipowner=o;
    clipdata=d;
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


void GMClipboard::saveClipboard() {
#ifndef WIN32
  if (hasClipboard()) {
    GM_DEBUG_PRINT("saveClipboard: we already own the clipboard\n");
    return;
    }

  FXID owner = XGetSelectionOwner(static_cast<Display*>(getApp()->getDisplay()), theclipboard);

  if (owner == 0) {
    GM_DEBUG_PRINT("saveClipboard: nobody owns the clipboard\n");
    return;
    }

  if (owner == xid) {
    GM_DEBUG_PRINT("saveClipboard: owner matches our xid\n");
    return;
    }

  FXWindow * window = getApp()->findWindowWithId(owner);
  if (window == nullptr) {
    GM_DEBUG_PRINT("saveClipboard: window not ours\n");
    return;
    }

  GM_DEBUG_PRINT("saveClipboard: window is ours, need to save\n");

  FXASSERT(clipdata == nullptr);
  FXASSERT(clipowner == nullptr);

  FXuint ntypes;
  FXDragType * types=nullptr;
  if (inquireDNDTypes(FROM_CLIPBOARD,types,ntypes)){
    for (FXuint t = 0; t < ntypes; t++) {
      if (types[t] == FXWindow::utf8Type) {

        FXDragType text_types[] = {
          FXWindow::stringType,
          FXWindow::utf8Type,
          FXWindow::utf16Type,
          FXWindow::textType
          };

        GMTextData * textdata = new GMTextData();
        getDNDData(FROM_CLIPBOARD, FXWindow::utf8Type, textdata->data);
        acquire(this, text_types, 4, textdata);
        break;
        }
      }
    }
  freeElms(types);
#endif
  }


long GMClipboard::onClipboardLost(FXObject*,FXSelector,void*){
  if (clipdata) delete clipdata;
  clipdata=nullptr;
  clipowner=nullptr;
  return 1;
  }

long GMClipboard::onClipboardGained(FXObject*,FXSelector,void*){
  return 1;
  }


long GMClipboard::onClipboardRequest(FXObject*,FXSelector,void*ptr){
  FXEvent *event=(FXEvent*)ptr;
  if (clipdata && clipdata->request(event->target,this)){
    return 1;
    }
  return 0;
  }
