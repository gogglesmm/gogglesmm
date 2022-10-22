/********************************************************************************
*                                                                               *
*                 S h u t t e r   C o n t a i n e r   W i d g e t               *
*                                                                               *
*********************************************************************************
* Copyright (C) 1998,2022 by Charles W. Warren.   All Rights Reserved.          *
*********************************************************************************
* This library is free software; you can redistribute it and/or modify          *
* it under the terms of the GNU Lesser General Public License as published by   *
* the Free Software Foundation; either version 3 of the License, or             *
* (at your option) any later version.                                           *
*                                                                               *
* This library is distributed in the hope that it will be useful,               *
* but WITHOUT ANY WARRANTY; without even the implied warranty of                *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                 *
* GNU Lesser General Public License for more details.                           *
*                                                                               *
* You should have received a copy of the GNU Lesser General Public License      *
* along with this program.  If not, see <http://www.gnu.org/licenses/>          *
********************************************************************************/
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "fxmath.h"
#include "FXArray.h"
#include "FXHash.h"
#include "FXMutex.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXSize.h"
#include "FXPoint.h"
#include "FXRectangle.h"
#include "FXStringDictionary.h"
#include "FXSettings.h"
#include "FXRegistry.h"
#include "FXAccelTable.h"
#include "FXEvent.h"
#include "FXWindow.h"
#include "FXApp.h"
#include "FXIcon.h"
#include "FXFrame.h"
#include "FXPacker.h"
#include "FXVerticalFrame.h"
#include "FXLabel.h"
#include "FXButton.h"
#include "FXScrollBar.h"
#include "FXScrollWindow.h"
#include "FXShutter.h"

/*
  Notes:
  - Horizontal scrollbar is turned off; this means default width is computed
    from contents.
  - Scroll window in collapsed items is not hidden, but made zero-height.  This
    means content width is still computed properly and thus the FXShutter will
    get a default width which depends on ALL items, not just the current one.
  - On a collapsing item, the scrollbar stays the way it was before the collapse
    started, while on an expanding item, the scrollbar stays off until the item is
    fully expanded.
*/

using namespace FX;

/*******************************************************************************/

namespace FX {

// Map
FXDEFMAP(FXShutterItem) FXShutterItemMap[]={
  FXMAPFUNC(SEL_FOCUS_UP,0,FXShutterItem::onFocusUp),
  FXMAPFUNC(SEL_FOCUS_DOWN,0,FXShutterItem::onFocusDown),
  FXMAPFUNC(SEL_COMMAND,FXShutterItem::ID_SHUTTERITEM_BUTTON,FXShutterItem::onCmdButton),
  };


// Object implementation
FXIMPLEMENT(FXShutterItem,FXVerticalFrame,FXShutterItemMap,ARRAYNUMBER(FXShutterItemMap))


// Serialization
FXShutterItem::FXShutterItem(){
  button=(FXButton*)-1L;
  scrollWindow=(FXScrollWindow*)-1L;
  content=(FXVerticalFrame*)-1L;
  }


// Construct shutter item
FXShutterItem::FXShutterItem(FXShutter* p,const FXString& text,FXIcon* icon,FXuint opts,FXint x,FXint y,FXint w,FXint h,FXint pl,FXint pr,FXint pt,FXint pb,FXint hs,FXint vs):FXVerticalFrame(p,(opts&~(PACK_UNIFORM_HEIGHT|PACK_UNIFORM_WIDTH)),x,y,w,h,0,0,0,0,0,0){
  button=new FXButton(this,text,icon,this,FXShutterItem::ID_SHUTTERITEM_BUTTON,FRAME_RAISED|FRAME_THICK|LAYOUT_FILL_X,0,0,0,0,0,0,0,0);
  scrollWindow=new FXScrollWindow(this,HSCROLLING_OFF|LAYOUT_FILL_X|LAYOUT_FIX_HEIGHT,0,0,0,0);
  content=new FXVerticalFrame(scrollWindow,LAYOUT_FILL_X|LAYOUT_FILL_Y|(opts&(PACK_UNIFORM_HEIGHT|PACK_UNIFORM_WIDTH)),0,0,0,0,pl,pr,pt,pb,hs,vs);
  content->setBackColor(getApp()->getShadowColor());
  }


// Button Pressed
long FXShutterItem::onCmdButton(FXObject*,FXSelector,void* ptr){
  getParent()->handle(this,FXSEL(SEL_COMMAND,FXShutter::ID_OPEN_SHUTTERITEM),ptr);
  return 1;
  }


// Focus moved up
long FXShutterItem::onFocusUp(FXObject* sender,FXSelector sel,void* ptr){
  return FXVerticalFrame::onFocusPrev(sender,sel,ptr);
  }


// Focus moved down
long FXShutterItem::onFocusDown(FXObject* sender,FXSelector sel,void* ptr){
  return FXVerticalFrame::onFocusNext(sender,sel,ptr);
  }


// Change help text
void FXShutterItem::setHelpText(const FXString& text){
  button->setHelpText(text);
  }


// Get help text
FXString FXShutterItem::getHelpText() const {
  return button->getHelpText();
  }


// Change tip text
void FXShutterItem::setTipText(const FXString& text){
  button->setTipText(text);
  }


// Get tip text
FXString FXShutterItem::getTipText() const {
  return button->getTipText();
  }


// Save object to stream
void FXShutterItem::save(FXStream& store) const {
  FXVerticalFrame::save(store);
  store << button;
  store << scrollWindow;
  store << content;
  }


// Load object from stream
void FXShutterItem::load(FXStream& store){
  FXVerticalFrame::load(store);
  store >> button;
  store >> scrollWindow;
  store >> content;
  }


// Thrash it
FXShutterItem::~FXShutterItem(){
  button=(FXButton*)-1L;
  scrollWindow=(FXScrollWindow*)-1L;
  content=(FXVerticalFrame*)-1L;
  }


/*******************************************************************************/

// Map
FXDEFMAP(FXShutter) FXShutterMap[]={
  FXMAPFUNC(SEL_FOCUS_UP,0,FXShutter::onFocusUp),
  FXMAPFUNC(SEL_FOCUS_DOWN,0,FXShutter::onFocusDown),
  FXMAPFUNCS(SEL_UPDATE,FXShutter::ID_OPEN_FIRST,FXShutter::ID_OPEN_LAST,FXShutter::onUpdOpen),
  FXMAPFUNC(SEL_TIMEOUT,FXShutter::ID_SHUTTER_TIMEOUT,FXShutter::onTimeout),
  FXMAPFUNC(SEL_COMMAND,FXShutter::ID_OPEN_SHUTTERITEM,FXShutter::onOpenItem),
  FXMAPFUNC(SEL_COMMAND,FXShutter::ID_SETVALUE,FXShutter::onCmdSetValue),
  FXMAPFUNC(SEL_COMMAND,FXShutter::ID_SETINTVALUE,FXShutter::onCmdSetIntValue),
  FXMAPFUNC(SEL_COMMAND,FXShutter::ID_GETINTVALUE,FXShutter::onCmdGetIntValue),
  FXMAPFUNCS(SEL_COMMAND,FXShutter::ID_OPEN_FIRST,FXShutter::ID_OPEN_LAST,FXShutter::onCmdOpen),
  };


// Object implementation
FXIMPLEMENT(FXShutter,FXVerticalFrame,FXShutterMap,ARRAYNUMBER(FXShutterMap))


// Serialization
FXShutter::FXShutter(){
  current=-1;
  closing=-1;
  closingHeight=0;
  heightIncrement=1;
  }


// Make shutter
FXShutter::FXShutter(FXComposite* p,FXObject* tgt,FXSelector sel,FXuint opts,FXint x,FXint y,FXint w,FXint h,FXint pl,FXint pr,FXint pt,FXint pb,FXint hs,FXint vs):FXVerticalFrame(p,opts,x,y,w,h,pl,pr,pt,pb,hs,vs){
  target=tgt;
  message=sel;
  current=-1;
  closing=-1;
  closingHeight=0;
  heightIncrement=1;
  }


// Focus moved up
long FXShutter::onFocusUp(FXObject* sender,FXSelector sel,void* ptr){
  return FXVerticalFrame::onFocusPrev(sender,sel,ptr);
  }


// Focus moved down
long FXShutter::onFocusDown(FXObject* sender,FXSelector sel,void* ptr){
  return FXVerticalFrame::onFocusNext(sender,sel,ptr);
  }


// Update value from a message
long FXShutter::onCmdSetValue(FXObject*,FXSelector,void* ptr){
  setCurrent((FXint)(FXival)ptr);
  return 1;
  }


// Update value from a message
long FXShutter::onCmdSetIntValue(FXObject*,FXSelector,void* ptr){
  setCurrent(*((FXint*)ptr));
  return 1;
  }


// Obtain value from text field
long FXShutter::onCmdGetIntValue(FXObject*,FXSelector,void* ptr){
  *((FXint*)ptr)=current;
  return 1;
  }


// Open item
long FXShutter::onCmdOpen(FXObject*,FXSelector sel,void*){
  setCurrent(FXSELID(sel)-ID_OPEN_FIRST,true);
  return 1;
  }


// Update the nth button
long FXShutter::onUpdOpen(FXObject* sender,FXSelector sel,void* ptr){
  sender->handle(this,((FXSELID(sel)-ID_OPEN_FIRST)==current) ? FXSEL(SEL_COMMAND,ID_CHECK) : FXSEL(SEL_COMMAND,ID_UNCHECK),ptr);
  return 1;
  }


// The sender of the message is the item to open up
long FXShutter::onOpenItem(FXObject* sender,FXSelector,void*){
  FXint which=indexOfChild((FXWindow*)sender);
  if(current==which) which--;
  if(0<=which){
    FXTime speed=getApp()->getAnimSpeed();
    if(0<speed){
      FXShutterItem *closingItem=(FXShutterItem*)childAtIndex(current);
      if(closingItem){
        closingHeight=closingItem->getHeight();
        closing=current;
        heightIncrement=0;
        getApp()->addTimeout(this,ID_SHUTTER_TIMEOUT,speed);
        }
      }
    }
  setCurrent(which,true);
  return 1;
  }


// Shutter Item Animation
long FXShutter::onTimeout(FXObject*,FXSelector,void*){
  if(0<=closing){
    heightIncrement+=5;
    closingHeight-=heightIncrement;
    if(closingHeight<0) closingHeight=0;
    recalc();
    if(0<closingHeight){
      getApp()->addTimeout(this,ID_SHUTTER_TIMEOUT,getApp()->getAnimSpeed());
      return 1;
      }
    closing=-1;
    }
  return 1;
  }


// Layout
void FXShutter::layout(){
  FXShutterItem* child;
  FXint i;

  // One of the children may have disappeared
  if(current<0) current=0;
  if(current>=numChildren()) current=numChildren()-1;

  // Force only one of the children to be open
  for(child=(FXShutterItem*)getFirst(),i=0; child; child=(FXShutterItem*)child->getNext(),i++){
    if(i==current){           // Expanded or expanded
      child->setLayoutHints(LAYOUT_FILL_X|LAYOUT_FILL_Y);
      child->getScrollWindow()->setLayoutHints(LAYOUT_FILL_X|LAYOUT_FILL_Y);
      }
    else if(i==closing){      // Collapsing
      child->setLayoutHints(LAYOUT_FILL_X);
      child->getScrollWindow()->setLayoutHints(LAYOUT_FILL_X|LAYOUT_FIX_HEIGHT);
      child->getScrollWindow()->setHeight(closingHeight);
      }
    else{                     // Collapsed
      child->setLayoutHints(LAYOUT_FILL_X);
      child->getScrollWindow()->setLayoutHints(LAYOUT_FILL_X|LAYOUT_FIX_HEIGHT);
      child->getScrollWindow()->setHeight(0);
      }
    }

  // Then layout normally
  FXVerticalFrame::layout();
  }


// Set current subwindow
void FXShutter::setCurrent(FXint panel,FXbool notify){
  if(panel!=current && 0<=panel && panel<numChildren()){
    current=panel;
    recalc();
    if(notify && target){ target->tryHandle(this,FXSEL(SEL_COMMAND,message),(void*)(FXival)current); }
    }
  }


// Save object to stream
void FXShutter::save(FXStream& store) const {
  FXVerticalFrame::save(store);
  store << current;
  }


// Load object from stream
void FXShutter::load(FXStream& store){
  FXVerticalFrame::load(store);
  store >> current;
  }


// Clean up
FXShutter::~FXShutter() {
  getApp()->removeTimeout(this,ID_SHUTTER_TIMEOUT);
  }

}

