/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*                        Icon List Widget (under LGPL3)                        *
*      Copyright (C) 1999,2010 by Jeroen van der Zijp. All Rights Reserved.    *
*                               ---                                            *
*                           Modifications                                      *
*           Copyright (C) 2006-2017 by Sander Jansen. All Rights Reserved      *
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
#include <limits.h>
#include "gmdefs.h"
#include "gmutils.h"
#include <fxkeys.h>
#include <FXPNGIcon.h>
#include "GMTrack.h"
#include "GMTrackList.h"
#include "GMTrackItem.h"
#include "GMSource.h"
#include "GMTrackView.h"
#include "GMPlayerManager.h"
#include "GMIconTheme.h"
#include "GMTrackDatabase.h"
#include "GMDatabaseSource.h"
#include "GMPlayListSource.h"
#include "GMPlayQueue.h"

//#include "icons.h"

#define SIDE_SPACING             4    // Left or right spacing between items
#define DETAIL_TEXT_SPACING      2    // Spacing between text and icon in detail icon mode
#define MINI_TEXT_SPACING        2    // Spacing between text and icon in mini icon mode
#define BIG_LINE_SPACING         6    // Line spacing in big icon mode
#define BIG_TEXT_SPACING         2    // Spacing between text and icon in big icon mode
#define ITEM_SPACE             128    // Default space for item

#define SELECT_MASK   (TRACKLIST_EXTENDEDSELECT|TRACKLIST_SINGLESELECT|TRACKLIST_BROWSESELECT|TRACKLIST_MULTIPLESELECT)
#define TRACKLIST_MASK (SELECT_MASK)

#define ICON_WIDTH 10
#define ICON_HEIGHT 15

#define UTF8_ELLIPSIS "…"

#define UTF32_BLACK_STAR 0x2605
#define UTF32_WHITE_STAR 0x2606
#define UTF8_BLACK_STAR "\xe2\x98\x85"
#define UTF8_WHITE_STAR "\xe2\x98\x86"

// Set or kill focus
void GMTrackItem::setFocus(FXbool focus){
  if(focus) state|=FOCUS; else state&=~FOCUS;
  }

// Select or deselect item
void GMTrackItem::setSelected(FXbool selected){
  if(selected) state|=SELECTED; else state&=~SELECTED;
  }

// Icon is draggable
void GMTrackItem::setDraggable(FXbool draggable){
  if(draggable) state|=DRAGGABLE; else state&=~DRAGGABLE;
  }


// Map
FXDEFMAP(GMTrackList) GMTrackListMap[]={
  FXMAPFUNC(SEL_PAINT,0,GMTrackList::onPaint),
  FXMAPFUNC(SEL_MOTION,0,GMTrackList::onMotion),
  FXMAPFUNC(SEL_LEFTBUTTONPRESS,0,GMTrackList::onLeftBtnPress),
  FXMAPFUNC(SEL_LEFTBUTTONRELEASE,0,GMTrackList::onLeftBtnRelease),
  FXMAPFUNC(SEL_RIGHTBUTTONPRESS,0,GMTrackList::onRightBtnPress),
  FXMAPFUNC(SEL_RIGHTBUTTONRELEASE,0,GMTrackList::onRightBtnRelease),
  FXMAPFUNC(SEL_TIMEOUT,GMTrackList::ID_AUTOSCROLL,GMTrackList::onAutoScroll),
  FXMAPFUNC(SEL_TIMEOUT,GMTrackList::ID_TIPTIMER,GMTrackList::onTipTimer),
  FXMAPFUNC(SEL_UNGRABBED,0,GMTrackList::onUngrabbed),
  FXMAPFUNC(SEL_KEYPRESS,0,GMTrackList::onKeyPress),
  FXMAPFUNC(SEL_KEYRELEASE,0,GMTrackList::onKeyRelease),
  FXMAPFUNC(SEL_ENTER,0,GMTrackList::onEnter),
  FXMAPFUNC(SEL_LEAVE,0,GMTrackList::onLeave),
  FXMAPFUNC(SEL_FOCUSIN,0,GMTrackList::onFocusIn),
  FXMAPFUNC(SEL_FOCUSOUT,0,GMTrackList::onFocusOut),
  FXMAPFUNC(SEL_CLICKED,0,GMTrackList::onClicked),
  FXMAPFUNC(SEL_DOUBLECLICKED,0,GMTrackList::onDoubleClicked),
  FXMAPFUNC(SEL_TRIPLECLICKED,0,GMTrackList::onTripleClicked),
  FXMAPFUNC(SEL_COMMAND,0,GMTrackList::onCommand),
  FXMAPFUNC(SEL_QUERY_TIP,0,GMTrackList::onQueryTip),
  FXMAPFUNC(SEL_QUERY_HELP,0,GMTrackList::onQueryHelp),
  FXMAPFUNC(SEL_COMMAND,GMTrackList::ID_HEADER,GMTrackList::onCmdHeader),
  FXMAPFUNC(SEL_UPDATE,GMTrackList::ID_HEADER,GMTrackList::onUpdHeader),
  FXMAPFUNC(SEL_CHANGED,GMTrackList::ID_HEADER,GMTrackList::onChgHeader),
  FXMAPFUNC(SEL_CLICKED,GMTrackList::ID_HEADER,GMTrackList::onClkHeader),
  FXMAPFUNC(SEL_ENTER,GMTrackList::ID_HEADER,GMTrackList::onMouseLeave),
  FXMAPFUNC(SEL_ENTER,GMTrackList::ID_HSCROLLED,GMTrackList::onMouseLeave),
  FXMAPFUNC(SEL_ENTER,GMTrackList::ID_VSCROLLED,GMTrackList::onMouseLeave),
  FXMAPFUNC(SEL_COMMAND,GMTrackList::ID_HSCROLLED,GMTrackList::onMouseLeave),
  FXMAPFUNC(SEL_COMMAND,GMTrackList::ID_VSCROLLED,GMTrackList::onMouseLeave),
  FXMAPFUNC(SEL_CHANGED,GMTrackList::ID_HSCROLLED,GMTrackList::onMouseLeave),
  FXMAPFUNC(SEL_CHANGED,GMTrackList::ID_VSCROLLED,GMTrackList::onMouseLeave),
  FXMAPFUNC(SEL_TIMEOUT,GMTrackList::ID_WHEEL_TIMEOUT,GMTrackList::onWheelTimeout),

  FXMAPFUNC(SEL_RIGHTBUTTONRELEASE,GMTrackList::ID_HEADER,GMTrackList::onHeaderRightBtnRelease),
  FXMAPFUNC(SEL_COMMAND,GMTrackList::ID_SELECT_ALL,GMTrackList::onCmdSelectAll),
  FXMAPFUNC(SEL_COMMAND,GMTrackList::ID_DESELECT_ALL,GMTrackList::onCmdDeselectAll),
  FXMAPFUNC(SEL_COMMAND,GMTrackList::ID_SELECT_INVERSE,GMTrackList::onCmdSelectInverse),
  FXMAPFUNC(SEL_COMMAND,FXWindow::ID_SETVALUE,GMTrackList::onCmdSetValue),
  FXMAPFUNC(SEL_COMMAND,FXWindow::ID_SETINTVALUE,GMTrackList::onCmdSetIntValue),
  FXMAPFUNC(SEL_COMMAND,FXWindow::ID_GETINTVALUE,GMTrackList::onCmdGetIntValue),
  };


// Object implementation
FXIMPLEMENT(GMTrackList,FXScrollArea,GMTrackListMap,ARRAYNUMBER(GMTrackListMap))


/*******************************************************************************/


// Serialization
GMTrackList::GMTrackList(){
  flags|=FLAG_ENABLED;
  header=(FXHeader*)-1L;
  anchor=-1;
  current=-1;
  extent=-1;
  cursor=-1;
  viewable=-1;
  active=-1;
  font=(FXFont*)-1L;
  activeFont=(FXFont*)-1L;
  sortfunc=nullptr;
  textColor=0;
  selbackColor=0;
  seltextColor=0;
  rowColor=0;
  activeColor=0;
  activeTextColor=0;
  shadowColor=0;
  lineHeight=1;
  anchorx=0;
  anchory=0;
  currentx=0;
  currenty=0;
  grabx=0;
  graby=0;
  ratingx=-1;
  ratingy=-1;
  ratingl=-1;
  state=false;
  sortMethod=HEADER_DEFAULT;
  }


// Icon List
GMTrackList::GMTrackList(FXComposite *p,FXObject* tgt,FXSelector sel,FXuint opts,FXint x,FXint y,FXint w,FXint h):FXScrollArea(p,opts,x,y,w,h){
  flags|=FLAG_ENABLED;
  header=new GMHeader(this,this,GMTrackList::ID_HEADER,HEADER_TRACKING|HEADER_BUTTON|HEADER_RESIZE|FRAME_LINE);
  target=tgt;
  message=sel;
  anchor=-1;
  current=-1;
  extent=-1;
  cursor=-1;
  viewable=-1;
  active=-1;
  font=getApp()->getNormalFont();
  activeFont=font;
  sortfunc=nullptr;
  textColor=getApp()->getForeColor();
  selbackColor=getApp()->getSelbackColor();
  seltextColor=getApp()->getSelforeColor();
  shadowColor=getApp()->getShadowColor();
  rowColor=backColor;
  activeColor=backColor;
  activeTextColor=textColor;
  lineHeight=1;
  anchorx=0;
  anchory=0;
  currentx=0;
  currenty=0;
  grabx=0;
  graby=0;
  ratingx=-1;
  ratingy=-1;
  ratingl=-1;
  state=false;
  sortMethod=HEADER_DEFAULT;

  GMScrollArea::replaceScrollbars(this);
  }

// Create window
void GMTrackList::create(){
  FXScrollArea::create();
  font->create();
  }


void GMTrackList::markUnsorted() {
  sortMethod=HEADER_DEFAULT;
  sortfunc=nullptr;
  }

// Detach window
void GMTrackList::detach(){
  FXScrollArea::detach();
  font->detach();
  }


// If window can have focus
FXbool GMTrackList::canFocus() const { return true; }

// Into focus chain
void GMTrackList::setFocus(){
  FXScrollArea::setFocus();
  setDefault(true);
  }


// Out of focus chain
void GMTrackList::killFocus(){
  FXScrollArea::killFocus();
  setDefault(maybe);
  }


// Propagate size change
void GMTrackList::recalc(){
  FXScrollArea::recalc();
  flags|=FLAG_RECALC;
  cursor=-1;
  }


// Return visible area y position
FXint GMTrackList::getVisibleY() const {
  return header->getHeight();
  }

// Return visible area height
FXint GMTrackList::getVisibleHeight() const {
  return height-header->getHeight()-horizontal->getHeight();
  }


// Move content
void GMTrackList::moveContents(FXint x,FXint y){
  FXScrollArea::moveContents(x,y);
  header->setPosition(x);
  }


// Recompute interior
void GMTrackList::recompute(){
  lineHeight=FXMAX(GMIconTheme::instance()->getSmallSize(),(4+font->getFontHeight()));
  flags&=~FLAG_RECALC;
  }


// Determine content width of icon list
FXint GMTrackList::getContentWidth(){
  if(flags&FLAG_RECALC) recompute();
  return header->getTotalSize();
  }


// Determine content height of icon list
FXint GMTrackList::getContentHeight(){
  if(flags&FLAG_RECALC) recompute();
  return items.no()*lineHeight;
  }


// Recalculate layout
void GMTrackList::layout(){
  FXint hh=header->getDefaultHeight();

  // Place scroll bars
  placeScrollBars(width,height-hh);

  // Place header control
  header->position(0,0,width,hh);

  // Set line size
  vertical->setLine(lineHeight);
  horizontal->setLine(header->getTotalSize());

  // We were supposed to make this item viewable
  if(0<=viewable){
    makeItemVisible(viewable);
    }

  // Force repaint
  update();

  // Clean
  flags&=~FLAG_DIRTY;
  }


// Header changed but content size didn't
long GMTrackList::onChgHeader(FXObject*,FXSelector,void*){
  return 1;
  }


// Header subdivision resize has been requested;
// we want to set the width of the header column
// to that of the widest item.
long GMTrackList::onClkHeader(FXObject*,FXSelector,void* ptr){
  FXint hi=(FXint)(FXival)ptr;
  FXint i,tw,w,nw=0,type;
  FXuint justify;
  FXint max;
  FXString text;

  type=getHeaderData(hi)->type;
  if (type==HEADER_RATING) {
    nw=(5*font->getTextWidth(starset)) + SIDE_SPACING + 2;
    }
  else {
    for(i=0;i<items.no();i++){
      w=0;
      const FXString * textptr=items[i]->getColumnData(type,text,justify,max);
      if (textptr && !textptr->empty()){
        tw=font->getTextWidth(*textptr);
        w=tw+SIDE_SPACING+2;
        }
      if(w>nw) nw=w;
      }
    }

  if (hi==0) {
    nw+=ICON_WIDTH+DETAIL_TEXT_SPACING+SIDE_SPACING/2;
    }

  // Set new header width
  if(nw>0 && nw!=header->getItemSize(hi)){
    header->setItemSize(hi,nw);
    flags&=~FLAG_RECALC;
    }
  return 1;
  }

long GMTrackList::onHeaderRightBtnRelease(FXObject*,FXSelector,void*ptr){
  if (target) target->handle(this,FXSEL(SEL_RIGHTBUTTONRELEASE,message+1),ptr);
  return 1;
  };

long GMTrackList::onCmdHeader(FXObject*,FXSelector,void*ptr){
  GMColumn * column = getHeaderData((FXuint)(FXival)ptr);
  if (column) {
    if (column->type==sortMethod)
      GMPlayerManager::instance()->getTrackView()->setSortMethod(column->type,(sortfunc==column->ascending));
    else
      GMPlayerManager::instance()->getTrackView()->setSortMethod(column->type);

    if (sortfunc)
      GMPlayerManager::instance()->getTrackView()->sortTracks();
    }
  return 1;
  }

long GMTrackList::onUpdHeader(FXObject*,FXSelector,void*){
  for (FXint i=0;i<header->getNumItems();i++){
    GMColumn * column = getHeaderData(i);
    if (column && sortMethod==column->type) {
      if (sortfunc==column->ascending)
        header->setArrowDir(i,FXHeaderItem::ARROW_DOWN);
      else
        header->setArrowDir(i,FXHeaderItem::ARROW_UP);
      }
    else {
      header->setArrowDir(i,FXHeaderItem::ARROW_NONE);
      }
    }
  return 1;
  }

long GMTrackList::onMouseLeave(FXObject*sender,FXSelector sel,void*ptr) {
  if (ratingl!=-1) {
    FXint item = header->getItemAt(ratingx);
    FXint rx   = header->getItemOffset(item);
    FXint rw   = header->getItemSize(item);
    FXint ry   = pos_y+ratingl*getLineHeight()+header->getDefaultHeight();
    FXint rh   = getLineHeight();
    ratingl=-1;
    update(rx,ry,rw,rh);
    }

  if (FXSELTYPE(sel)!=SEL_ENTER) {
    getApp()->addTimeout(this,ID_WHEEL_TIMEOUT,5_ms);
    if (FXSELID(sel)==ID_HSCROLLED) {
      if (FXSELTYPE(sel)==SEL_COMMAND)
        return FXScrollArea::onHScrollerChanged(sender,sel,ptr);
      else if (FXSELTYPE(sel)==SEL_CHANGED)
        return FXScrollArea::onHScrollerDragged(sender,sel,ptr);
      }
    else if (FXSELID(sel)==ID_VSCROLLED){
      if (FXSELTYPE(sel)==SEL_COMMAND)
        return FXScrollArea::onVScrollerChanged(sender,sel,ptr);
      else if (FXSELTYPE(sel)==SEL_CHANGED)
        return FXScrollArea::onVScrollerDragged(sender,sel,ptr);
      }
    }
  setDefaultCursor(getApp()->getDefaultCursor(DEF_ARROW_CURSOR));
  return 0;
  }

long GMTrackList::onWheelTimeout(FXObject*,FXSelector,void*) {
  FXint item = header->getItemAt(ratingx);
  if (getHeaderType(item)==HEADER_RATING) {
    ratingl    = (ratingy-pos_y-header->getDefaultHeight())/getLineHeight();
    FXint rx   = header->getItemOffset(item);
    FXint rw   = header->getItemSize(item);
    FXint ry   = pos_y+ratingl*getLineHeight()+header->getDefaultHeight();
    FXint rh   = getLineHeight();
    update(rx,ry,rw,rh);
    }
  return 1;
  }

void GMTrackList::appendHeader(const FXString & label,FXint size,GMColumn * column){
  GMColumn * c;
  for (FXint i=0;i<header->getNumItems();i++){
    c = getHeaderData(i);
    if (column->index < c->index){
      header->insertItem(i,label,nullptr,size,column);
      return;
      }
    }
  header->appendItem(label,nullptr,size,column);
  }


// Remove header caption
void GMTrackList::removeHeader(FXint index){
  if(index<0 || header->getNumItems()<=index){ fxerror("%s::removeHeader: index out of range.\n",getClassName()); }
  header->removeItem(index);
  }

// Return number of headers
FXint GMTrackList::getNumHeaders() const {
  return header->getNumItems();
  }

/// Return index of given header type if displayed, otherwise -1
FXint GMTrackList::getHeaderByType(FXuint type) const {
  for (FXint i=0;i<header->getNumItems();i++){
    if (getHeaderType(i) == type) {
      return i;
      }
    }
  return -1;
  }

/// Remove all headers
void GMTrackList::clearHeaders() {
  header->clearItems();
  }

/// Save header configuration
void GMTrackList::saveHeaders() {
  GMColumn * column = nullptr;
  for (FXint i=0;i<header->getNumItems();i++){
    column = getHeaderData(i);
    FXASSERT(column);
    column->size  = header->getItemSize(i);
    //column->index = i;
    }
  }


// True if item is current
FXbool GMTrackList::isItemPlayable(FXint index) const {
  if(index<0 || items.no()<=index){ fxerror("%s::isItemPlayable: index out of range.\n",getClassName()); }
  return items[index]->canPlay();
  }


// True if item is selected
FXbool GMTrackList::isItemSelected(FXint index) const {
  if(index<0 || items.no()<=index){ fxerror("%s::isItemSelected: index out of range.\n",getClassName()); }
  return items[index]->isSelected();
  }


// True if item is current
FXbool GMTrackList::isItemCurrent(FXint index) const {
  if(index<0 || items.no()<=index){ fxerror("%s::isItemCurrent: index out of range.\n",getClassName()); }
  return index==current;
  }

// True if item (partially) visible
FXbool GMTrackList::isItemVisible(FXint index) const {
  FXbool vis=false;
  FXint y,hh;
  if(index<0 || items.no()<=index){ fxerror("%s::isItemVisible: index out of range.\n",getClassName()); }
  hh=header->getDefaultHeight();
  y=pos_y+hh+index*lineHeight;
  if(hh<y+lineHeight && y<getVisibleHeight()) vis=true;
  return vis;
  }

// Make item fully visible
void GMTrackList::makeItemVisible(FXint index){
  if(0<=index && index<items.no()){

    // Remember for later
    viewable=index;

    // Was realized
    if(xid){
      FXint y,hh,px,py,vh;

      // Force layout if dirty
      if(flags&FLAG_RECALC) layout();

      px=pos_x;
      py=pos_y;

      //vw=getVisibleWidth();
      vh=getVisibleHeight();
      hh=header->getDefaultHeight();
      y=hh+index*lineHeight;
      if(py+y+lineHeight >= vh+hh) py=hh+vh-y-lineHeight;
      if(py+y <= hh) py=hh-y;

      // Scroll into view
      setPosition(px,py);

      // Done it
      viewable=-1;
      }
    }
  }

/// Find Item by Id
FXint GMTrackList::findItemById(FXint id) const{
  for (FXint i=0;i<items.no();i++){
    if (items[i]->id==id) return i;
    }
  return -1;
  }


// Get item at position x,y
FXint GMTrackList::getItemAt(FXint /*x*/,FXint y) const {
  FXint index;
  y-=pos_y;
  y-=header->getDefaultHeight();
  index=y/lineHeight;
  if(index<0 || index>=items.no()) return -1;
  return index;
  }


// Did we hit the item, and which part of it did we hit
FXint GMTrackList::hitItem(FXint index,FXint /*x*/,FXint /*y*/,FXint /*ww*/,FXint /*hh*/) const {
  FXint /*ix,iy,r,c,*/hit=0;
  if(0<=index && index<items.no()){
/*
    x-=pos_x;
    y-=pos_y;
    y-=header->getDefaultHeight();
    r=index;
    c=0;
    ix=header->getTotalSize()*c;
    iy=lineHeight*r;
    hit=2; //FIXME items[index]->hitItem(this,x-ix,y-iy,ww,hh);
*/
    hit=2;
    }
  return hit;
  }


// Repaint
void GMTrackList::updateItem(FXint index) const {
  if(xid && 0<=index && index<items.no()){
    update(0,pos_y+header->getDefaultHeight()+index*lineHeight,width,lineHeight);
    }
  }



// Select one item
FXbool GMTrackList::selectItem(FXint index,FXbool notify){
  if(index<0 || items.no()<=index){ fxerror("%s::selectItem: index out of range.\n",getClassName()); }
  if(!items[index]->isSelected()){
    switch(options&SELECT_MASK){
      case TRACKLIST_SINGLESELECT:
      case TRACKLIST_BROWSESELECT:
        killSelection(notify);
      case TRACKLIST_EXTENDEDSELECT:
      case TRACKLIST_MULTIPLESELECT:
        items[index]->setSelected(true);
        updateItem(index);
        if(notify && target){target->tryHandle(this,FXSEL(SEL_SELECTED,message),(void*)(FXival)index);}
        break;
      }
    return true;
    }
  return false;
  }


// Deselect one item
FXbool GMTrackList::deselectItem(FXint index,FXbool notify){
  if(index<0 || items.no()<=index){ fxerror("%s::deselectItem: index out of range.\n",getClassName()); }
  if(items[index]->isSelected()){
    switch(options&SELECT_MASK){
      case TRACKLIST_EXTENDEDSELECT:
      case TRACKLIST_MULTIPLESELECT:
      case TRACKLIST_SINGLESELECT:
        items[index]->setSelected(false);
        updateItem(index);
        if(notify && target){target->tryHandle(this,FXSEL(SEL_DESELECTED,message),(void*)(FXival)index);}
        break;
      }
    return true;
    }
  return false;
  }


// Toggle one item
FXbool GMTrackList::toggleItem(FXint index,FXbool notify){
  if(index<0 || items.no()<=index){ fxerror("%s::toggleItem: index out of range.\n",getClassName()); }
  switch(options&SELECT_MASK){
    case TRACKLIST_BROWSESELECT:
      if(!items[index]->isSelected()){
        killSelection(notify);
        items[index]->setSelected(true);
        updateItem(index);
        if(notify && target){target->tryHandle(this,FXSEL(SEL_SELECTED,message),(void*)(FXival)index);}
        }
      break;
    case TRACKLIST_SINGLESELECT:
      if(!items[index]->isSelected()){
        killSelection(notify);
        items[index]->setSelected(true);
        updateItem(index);
        if(notify && target){target->tryHandle(this,FXSEL(SEL_SELECTED,message),(void*)(FXival)index);}
        }
      else{
        items[index]->setSelected(false);
        updateItem(index);
        if(notify && target){target->tryHandle(this,FXSEL(SEL_DESELECTED,message),(void*)(FXival)index);}
        }
      break;
    case TRACKLIST_EXTENDEDSELECT:
    case TRACKLIST_MULTIPLESELECT:
      if(!items[index]->isSelected()){
        items[index]->setSelected(true);
        updateItem(index);
        if(notify && target){target->tryHandle(this,FXSEL(SEL_SELECTED,message),(void*)(FXival)index);}
        }
      else{
        items[index]->setSelected(false);
        updateItem(index);
        if(notify && target){target->tryHandle(this,FXSEL(SEL_DESELECTED,message),(void*)(FXival)index);}
        }
      break;
    }
  return true;
  }


// Select items in rectangle
FXbool GMTrackList::selectInRectangle(FXint x,FXint y,FXint w,FXint h,FXbool notify){
  FXint index;
  FXbool changed=false;
  for(index=0; index<items.no(); index++){
    if(hitItem(index,x,y,w,h)){
      changed|=selectItem(index,notify);
      }
    }
  return changed;
  }


// Extend selection
FXbool GMTrackList::extendSelection(FXint index,FXbool notify){
  FXbool changes=false;
  FXint i1,i2,i3,i;
  if(0<=index && 0<=anchor && 0<=extent){

    // Find segments
    i1=index;
    if(anchor<i1){i2=i1;i1=anchor;}
    else{i2=anchor;}
    if(extent<i1){i3=i2;i2=i1;i1=extent;}
    else if(extent<i2){i3=i2;i2=extent;}
    else{i3=extent;}

    // First segment
    for(i=i1; i<i2; i++){

      // item===extent---anchor
      // item===anchor---extent
      if(i1==index){
        if(!items[i]->isSelected()){
          items[i]->setSelected(true);
          updateItem(i);
          changes=true;
          if(notify && target){target->tryHandle(this,FXSEL(SEL_SELECTED,message),(void*)(FXival)i);}
          }
        }

      // extent===anchor---item
      // extent===item-----anchor
      else if(i1==extent){
        if(items[i]->isSelected()){
          items[i]->setSelected(false);
          updateItem(i);
          changes=true;
          if(notify && target){target->tryHandle(this,FXSEL(SEL_DESELECTED,message),(void*)(FXival)i);}
          }
        }
      }

    // Second segment
    for(i=i2+1; i<=i3; i++){

      // extent---anchor===item
      // anchor---extent===item
      if(i3==index){
        if(!items[i]->isSelected()){
          items[i]->setSelected(true);
          updateItem(i);
          changes=true;
          if(notify && target){target->tryHandle(this,FXSEL(SEL_SELECTED,message),(void*)(FXival)i);}
          }
        }

      // item-----anchor===extent
      // anchor---item=====extent
      else if(i3==extent){
        if(items[i]->isSelected()){
          items[i]->setSelected(false);
          updateItem(i);
          changes=true;
          if(notify && target){target->tryHandle(this,FXSEL(SEL_DESELECTED,message),(void*)(FXival)i);}
          }
        }
      }
    extent=index;
    }
  return changes;
  }


// Kill selection
FXbool GMTrackList::killSelection(FXbool notify){
  FXbool changes=false;
  FXint i;
  for(i=0; i<items.no(); i++){
    if(items[i]->isSelected()){
      items[i]->setSelected(false);
      updateItem(i);
      changes=true;
      if(notify && target){target->tryHandle(this,FXSEL(SEL_DESELECTED,message),(void*)(FXival)i);}
      }
    }
  return changes;
  }

// Update value from a message
long GMTrackList::onCmdSetValue(FXObject*,FXSelector,void* ptr){
  setCurrentItem((FXint)(FXival)ptr);
  return 1;
  }


// Obtain value from list
long GMTrackList::onCmdGetIntValue(FXObject*,FXSelector,void* ptr){
  *((FXint*)ptr)=getCurrentItem();
  return 1;
  }


// Update value from a message
long GMTrackList::onCmdSetIntValue(FXObject*,FXSelector,void* ptr){
  setCurrentItem(*((FXint*)ptr));
  return 1;
  }


// Start motion timer while in this window
long GMTrackList::onEnter(FXObject* sender,FXSelector sel,void* ptr){
  FXScrollArea::onEnter(sender,sel,ptr);
  getApp()->addTimeout(this,ID_TIPTIMER,getApp()->getMenuPause());
  cursor=-1;
  return 1;
  }

void GMTrackList::clearRating() {
  }

// Stop motion timer when leaving window
long GMTrackList::onLeave(FXObject* sender,FXSelector sel,void* ptr){
  FXScrollArea::onLeave(sender,sel,ptr);
  getApp()->removeTimeout(this,ID_TIPTIMER);

  if (ratingl!=-1) {
    FXint item = header->getItemAt(ratingx);
    FXint rx   = header->getItemOffset(item);
    FXint rw   = header->getItemSize(item);
    FXint ry   = pos_y+ratingl*getLineHeight()+header->getDefaultHeight();
    FXint rh   = getLineHeight();
    ratingl=-1;
    update(rx,ry,rw,rh);
    setDefaultCursor(getApp()->getDefaultCursor(DEF_ARROW_CURSOR));
    }
  cursor=-1;
  return 1;
  }


// We timed out, i.e. the user didn't move for a while
long GMTrackList::onTipTimer(FXObject*,FXSelector,void*){
  FXTRACE((250,"%s::onTipTimer %p\n",getClassName(),this));
  flags|=FLAG_TIP;
  return 1;
  }


// We were asked about tip text
long GMTrackList::onQueryTip(FXObject* sender,FXSelector sel,void* ptr){
  if(FXScrollArea::onQueryTip(sender,sel,ptr)) return 1;
/*
FIXME
  if((flags&FLAG_TIP) && (0<=cursor)){
    FXString string=items[cursor]->getText().section('\t',0);
    sender->handle(this,FXSEL(SEL_COMMAND,ID_SETSTRINGVALUE),(void*)&string);
    return 1;
    }
*/
  return 0;
  }


// We were asked about status text
long GMTrackList::onQueryHelp(FXObject* sender,FXSelector sel,void* ptr){
  if(FXScrollArea::onQueryHelp(sender,sel,ptr)) return 1;
  if((flags&FLAG_HELP) && !help.empty()){
    sender->handle(this,FXSEL(SEL_COMMAND,ID_SETSTRINGVALUE),(void*)&help);
    return 1;
    }
  return 0;
  }


// Gained focus
long GMTrackList::onFocusIn(FXObject* sender,FXSelector sel,void* ptr){
  FXScrollArea::onFocusIn(sender,sel,ptr);
  if(0<=current){
    FXASSERT(current<items.no());
    items[current]->setFocus(true);
    updateItem(current);
    }
  return 1;
  }


// Lost focus
long GMTrackList::onFocusOut(FXObject* sender,FXSelector sel,void* ptr){
  FXScrollArea::onFocusOut(sender,sel,ptr);
  if(0<=current){
    FXASSERT(current<items.no());
    items[current]->setFocus(false);
    updateItem(current);
    }
  return 1;
  }


// Draw item list
long GMTrackList::onPaint(FXObject*,FXSelector,void* ptr){
  FXint rlo,rhi,dw,index,vw,y;
  FXEvent* event=(FXEvent*)ptr;
  FXDCWindow dc(this,event);

  // Draw nothing
  if (header->getNumItems()==0) {
    dc.setForeground(backColor);
    dc.fillRectangle(0,0,width,height);
    return 1;
    }

  // Set font
  dc.setFont(font);

  // Calculate stipple width
  dw=font->getTextWidth(UTF8_ELLIPSIS,3);

  /// Determine Rating Character to draw
  if (font->hasChar(UTF32_BLACK_STAR)){
    starset = UTF8_BLACK_STAR;
    if (font->hasChar(UTF32_WHITE_STAR))
      starunset = UTF8_WHITE_STAR;
    else
      starunset.clear();
    }
  else {
    starset = "* ";
    starunset.clear();
    }

  // Exposed rows
  rlo=(event->rect.y-pos_y-header->getDefaultHeight())/lineHeight;
  rhi=(event->rect.y+event->rect.h-pos_y-header->getDefaultHeight())/lineHeight;
  if(rlo<0) rlo=0;
  if(rhi>=items.no()) rhi=items.no()-1;

  vw = getVisibleWidth();

  // Repaint the items
  y=pos_y+rlo*lineHeight+header->getDefaultHeight();
  for(index=rlo; index<=rhi; index++,y+=lineHeight){
    if (active==index) {
      dc.setForeground(activeColor);
      dc.setFont(activeFont);
      }
    else if (index%2)
      dc.setForeground(rowColor);
    else
      dc.setForeground(backColor);

    draw(dc,event,index,pos_x,y,vw,lineHeight,dw);

    if (active==index)
      dc.setFont(font);
    }

  // Background below
  y=pos_y+(rhi+1)*lineHeight+header->getDefaultHeight();
  if(y<event->rect.y+event->rect.h){
    dc.setForeground(backColor);
    dc.fillRectangle(event->rect.x,y,event->rect.w,event->rect.y+event->rect.h-y);
    }
  return 1;
  }


void GMTrackList::draw(FXDC& dc,FXEvent *,FXint index,FXint x,FXint y,FXint w,FXint h,FXint dw) const {
  FXint iw,ih,tw=0,th=0,yt,hi,drw,space,used,xx,type,offset;
  FXString text;
  const FXString * textptr;
  FXint max=50;
  FXuint justify;
  FXIcon * icon=nullptr;

  iw=ih=GMIconTheme::instance()->getSmallSize();


  icon=items[index]->getIcon();


  /// Draw background
  if(items[index]->isSelected()){
    if (active==index) icon = GMIconTheme::instance()->icon_play;
    dc.setForeground(getSelBackColor());
    dc.fillRectangle(0,y,w,h);
    }
  else{
    if (active==index)  icon = GMIconTheme::instance()->icon_play;
    dc.fillRectangle(0,y,w,h);
    }

  /// Draw Focus
  if(items[index]->hasFocus()){
    dc.drawFocusRectangle(x+1,y+1,getHeader()->getTotalSize()-2,h-2);
    }

  /// Draw Icon
  xx=x+SIDE_SPACING/2;
  if(icon){
    dc.setClipRectangle(x,y,header->getItemSize(0),h);
    dc.drawIcon(icon,xx,y+(h-ih)/2);
    dc.clearClipRectangle();
    }
  xx+=iw+DETAIL_TEXT_SPACING;

  /// Draw Text
  th=dc.getFont()->getFontHeight();
  yt=y+(h-th-4)/2;
  if(items[index]->isSelected())
    dc.setForeground(getSelTextColor());
  else if (active==index)
    dc.setForeground(getActiveTextColor());
  else if (items[index]->isShaded())
    dc.setForeground(getShadowColor());
  else
    dc.setForeground(getTextColor());

  used=iw+DETAIL_TEXT_SPACING+SIDE_SPACING/2;
  for(hi=0;hi<header->getNumItems()&& xx<=w+getVisibleX(); hi++){
    space=header->getItemSize(hi)-used;
    if (xx+space>=getVisibleX()){
      type=getHeaderType(hi);
      textptr=items[index]->getColumnData(type,text,justify,max);

      if (type==HEADER_RATING) {
        tw=dc.getFont()->getTextWidth(starset);
        offset=xx+2+(FXint)((double)tw/(double)2.0);
        dc.setClipRectangle(xx,y,space,h);
        if (ratingl == index){
          FXint stars=0;

          for (stars=0;stars<5 && (offset+(stars*tw))<ratingx;stars++) {
            dc.drawText(xx+2+(stars*tw),yt+dc.getFont()->getFontAscent()+2,starset.text(),starset.length());
            }
          if (starunset.empty()) {
            dc.setForeground(getApp()->getShadowColor());
            for (;stars<5;stars++) {
              dc.drawText(xx+2+(stars*tw),yt+dc.getFont()->getFontAscent()+2,starset.text(),starset.length());
              }
            }
          else {
            for (;stars<5;stars++) {
              dc.drawText(xx+2+(stars*tw),yt+dc.getFont()->getFontAscent()+2,starunset.text(),starunset.length());
              }
            }
          }
        else {
          for (FXint i=0;i<max;i++) {
            dc.drawText(xx+2+(i*tw),yt+dc.getFont()->getFontAscent()+2,starset.text(),starset.length());
            }
          }
        dc.clearClipRectangle();
        }
      else if (textptr) {
        drw=textptr->length();
        tw=dc.getFont()->getTextWidth(*textptr);
        if(tw>space-4){
          while((tw=dc.getFont()->getTextWidth(textptr->text(),drw))+dw>space-4 && drw>1) drw=textptr->dec(drw);
          dc.setClipRectangle(xx,y,space,h);
          dc.drawText(xx+2,yt+dc.getFont()->getFontAscent()+2,textptr->text(),drw);
          dc.drawText(xx+tw+2,yt+dc.getFont()->getFontAscent()+2,UTF8_ELLIPSIS,3);
          dc.clearClipRectangle();
          }
        else if (justify==0 || (justify && max>space)) {
          dc.drawText(xx+2,yt+dc.getFont()->getFontAscent()+2,textptr->text(),drw);
          }
        else if (justify==COLUMN_JUSTIFY_LEFT_RIGHT_ALIGNED) {
          dc.drawText(xx+2+max-tw,yt+dc.getFont()->getFontAscent()+2,text.text(),drw);
          }
        else if (justify==COLUMN_JUSTIFY_CENTER_RIGHT_ALIGNED){
          dc.drawText(xx+((space/2)-(max/2))+(max-tw),yt+dc.getFont()->getFontAscent()+2,textptr->text(),drw);
          }
        else if (justify==COLUMN_JUSTIFY_RIGHT){
          dc.drawText(xx+(space-tw)-2,yt+dc.getFont()->getFontAscent()+2,textptr->text(),drw);
          }
        else {
          dc.drawText(xx+2,yt+dc.getFont()->getFontAscent()+2,textptr->text(),drw);
          }
        }
      }
    xx+=space;
    used=0;
    }
  }



// Select all items
long GMTrackList::onCmdSelectAll(FXObject*,FXSelector,void*){
  for(int i=0; i<items.no(); i++) selectItem(i,true);
  return 1;
  }


// Deselect all items
long GMTrackList::onCmdDeselectAll(FXObject*,FXSelector,void*){
  for(int i=0; i<items.no(); i++) deselectItem(i,true);
  return 1;
  }


// Select inverse of current selection
long GMTrackList::onCmdSelectInverse(FXObject*,FXSelector,void*){
  for(int i=0; i<items.no(); i++) toggleItem(i,true);
  return 1;
  }



// Sort the items based on the sort function
void GMTrackList::sortItems(){
  GMTrackItem *v,*c=0;
  FXbool exch=false;
  FXint i,j,h;
  if(sortfunc){
    if(0<=current){
      c=items[current];
      }
    for(h=1; h<=items.no()/9; h=3*h+1){}
    for(; h>0; h/=3){
      for(i=h+1;i<=items.no();i++){
        v=items[i-1];
        j=i;
        while(j>h && sortfunc(items[j-h-1],v)>0){
          items[j-1]=items[j-h-1];
          exch=true;
          j-=h;
          }
        items[j-1]=v;
        }
      }
    if(0<=current){
      for(i=0; i<items.no(); i++){
        if(items[i]==c){ current=i; break; }
        }
      }
    if(exch) recalc();
    }
  }


// Set current item
void GMTrackList::setCurrentItem(FXint index,FXbool notify){
  if(index<-1 || items.no()<=index){ fxerror("%s::setCurrentItem: index out of range.\n",getClassName()); }
  if(index!=current){

    // Deactivate old item
    if(0<=current){

      // No visible change if it doen't have the focus
      if(hasFocus()){
        items[current]->setFocus(false);
        updateItem(current);
        }
      }

    current=index;

    // Activate new item
    if(0<=current){

      // No visible change if it doen't have the focus
      if(hasFocus()){
        items[current]->setFocus(true);
        updateItem(current);
        }
      }

    // Notify item change
    if(notify && target){target->tryHandle(this,FXSEL(SEL_CHANGED,message),(void*)(FXival)current);}
    }

  // In browse selection mode, select item
  if((options&SELECT_MASK)==TRACKLIST_BROWSESELECT && 0<=current ){
    selectItem(current,notify);
    }
  }


// Set anchor item
void GMTrackList::setAnchorItem(FXint index){
  if(index<-1 || items.no()<=index){ fxerror("%s::setAnchorItem: index out of range.\n",getClassName()); }
  anchor=index;
  extent=index;
  }


// Set active item
void GMTrackList::setActiveItem(FXint i) {
  active=i;
  if (items.no()) {
    if (i>=0)
      makeItemVisible(i);
    update();
    }
  }


// Key Press
long GMTrackList::onKeyPress(FXObject*,FXSelector,void* ptr){
  FXEvent* event=(FXEvent*)ptr;
  FXint index=current;
  flags&=~FLAG_TIP;
  if(!isEnabled()) return 0;
  if(target && target->tryHandle(this,FXSEL(SEL_KEYPRESS,message),ptr)) return 1;
  switch(event->code){
    case KEY_Control_L:
    case KEY_Control_R:
    case KEY_Shift_L:
    case KEY_Shift_R:
    case KEY_Alt_L:
    case KEY_Alt_R:
      if(flags&FLAG_DODRAG){handle(this,FXSEL(SEL_DRAGGED,0),ptr);}
      return 1;
    case KEY_Page_Up:
    case KEY_KP_Page_Up:
      setPosition(pos_x,pos_y+verticalScrollBar()->getPage());
      return 1;
    case KEY_Page_Down:
    case KEY_KP_Page_Down:
      setPosition(pos_x,pos_y-verticalScrollBar()->getPage());
      return 1;
    case KEY_Right:
    case KEY_KP_Right:
      setPosition(pos_x-10,pos_y);
      return 1;
      goto hop;
    case KEY_Left:
    case KEY_KP_Left:
      setPosition(pos_x+10,pos_y);
      return 1;
      goto hop;
    case KEY_Up:
    case KEY_KP_Up:
      index-=1;
      goto hop;
    case KEY_Down:
    case KEY_KP_Down:
      index+=1;
      goto hop;
    case KEY_Home:
    case KEY_KP_Home:
      index=0;
      goto hop;
    case KEY_End:
    case KEY_KP_End:
      index=items.no()-1;
hop:  if(0<=index && index<items.no()){
        setCurrentItem(index,true);
        makeItemVisible(index);
        if((options&SELECT_MASK)==TRACKLIST_EXTENDEDSELECT){
          if(event->state&SHIFTMASK){
            if(0<=anchor){
              selectItem(anchor,true);
              extendSelection(index,true);
              }
            else{
              selectItem(index,true);
              }
            }
          else if(!(event->state&CONTROLMASK)){
            killSelection(true);
            selectItem(index,true);
            setAnchorItem(index);
            }
          }
        }
      handle(this,FXSEL(SEL_CLICKED,0),(void*)(FXival)current);
      if(0<=current){
        handle(this,FXSEL(SEL_COMMAND,0),(void*)(FXival)current);
        }
      return 1;
    case KEY_space:
    case KEY_KP_Space:
      if(0<=current){
        switch(options&SELECT_MASK){
          case TRACKLIST_EXTENDEDSELECT:
            if(event->state&SHIFTMASK){
              if(0<=anchor){
                selectItem(anchor,true);
                extendSelection(current,true);
                }
              else{
                selectItem(current,true);
                }
              }
            else if(event->state&CONTROLMASK){
              toggleItem(current,true);
              }
            else{
              killSelection(true);
              selectItem(current,true);
              }
            break;
          case TRACKLIST_MULTIPLESELECT:
          case TRACKLIST_SINGLESELECT:
            toggleItem(current,true);
            break;
          }
        setAnchorItem(current);
        }
      handle(this,FXSEL(SEL_CLICKED,0),(void*)(FXival)current);
      if(0<=current){
        handle(this,FXSEL(SEL_COMMAND,0),(void*)(FXival)current);
        }
      return 1;
    case KEY_Return:
    case KEY_KP_Enter:
      handle(this,FXSEL(SEL_DOUBLECLICKED,0),(void*)(FXival)current);
      if(0<=current){
        handle(this,FXSEL(SEL_COMMAND,0),(void*)(FXival)current);
        }
      return 1;
    default: break;
    }
  return 0;
  }


// Key Release
long GMTrackList::onKeyRelease(FXObject*,FXSelector,void* ptr){
  FXEvent* event=(FXEvent*)ptr;
  if(!isEnabled()) return 0;
  if(target && target->tryHandle(this,FXSEL(SEL_KEYRELEASE,message),ptr)) return 1;
  switch(event->code){
    case KEY_Shift_L:
    case KEY_Shift_R:
    case KEY_Control_L:
    case KEY_Control_R:
    case KEY_Alt_L:
    case KEY_Alt_R:
      if(flags&FLAG_DODRAG){handle(this,FXSEL(SEL_DRAGGED,0),ptr);}
      return 1;
    }
  return 0;
  }


// Autoscrolling timer
long GMTrackList::onAutoScroll(FXObject* sender,FXSelector sel,void* ptr){
  // Scroll the content
  FXScrollArea::onAutoScroll(sender,sel,ptr);

  // Content scrolled, so perhaps something else under cursor
  if(flags&FLAG_DODRAG){
    handle(this,FXSEL(SEL_DRAGGED,0),ptr);
    return 1;
    }

  return 0;
  }


// Mouse moved
long GMTrackList::onMotion(FXObject*,FXSelector,void* ptr){
  FXEvent* event=(FXEvent*)ptr;
  FXint oldcursor=cursor;
  FXuint flg=flags;

  // Kill the tip
  flags&=~FLAG_TIP;

  // Kill the tip timer
  getApp()->removeTimeout(this,ID_TIPTIMER);

  // Right mouse scrolling
  if(flags&FLAG_SCROLLING){
    setPosition(event->win_x-grabx,event->win_y-graby);
    return 1;
    }

  // Drag and drop mode
  if(flags&FLAG_DODRAG){
    if(startAutoScroll(event,true)) return 1;
    handle(this,FXSEL(SEL_DRAGGED,0),ptr);
    return 1;
    }

  // Tentative drag and drop
  if(flags&FLAG_TRYDRAG){
    if(event->moved){
      flags&=~FLAG_TRYDRAG;
      if(handle(this,FXSEL(SEL_BEGINDRAG,0),ptr)){
        flags|=FLAG_DODRAG;
        }
      }
    return 1;
    }


  // Check for hovering
  if (!grabbed()) {
    FXint rx,ry,rw,rh;
    FXint cl = header->getItemAt(event->last_x);
    FXint cc = header->getItemAt(event->win_x);
    FXint rl = (event->last_y-pos_y-header->getDefaultHeight())/getLineHeight();
    FXint rc = (event->win_y-pos_y-header->getDefaultHeight())/getLineHeight();

    ry = pos_y+FXMIN(rl,rc)*getLineHeight()+header->getDefaultHeight();
    if (rl==rc)
      rh = getLineHeight();
    else
      rh = getLineHeight()*(FXABS(rl-rc)+1);

    if ( (rl<getNumItems() || rc<getNumItems()) && (getHeaderType(cc)==HEADER_RATING || getHeaderType(cl)==HEADER_RATING) ) {
      if (getHeaderType(cc)==HEADER_RATING) {
        rx      = header->getItemOffset(cc);
        rw      = header->getItemSize(cc);
        ratingx = event->win_x;
        ratingy = event->win_y;
        ratingl = rc;
        setDefaultCursor(GMIconTheme::instance()->cursor_hand);
        }
      else {
        rx      = header->getItemOffset(cl);
        rw      = header->getItemSize(cl);
        ratingl = ratingx = ratingy =-1;
        setDefaultCursor(getApp()->getDefaultCursor(DEF_ARROW_CURSOR));
        }
      update(rx,ry,rw,rh);
      }
    else {
      ratingl=ratingx=ratingy=-1;
      setDefaultCursor(getApp()->getDefaultCursor(DEF_ARROW_CURSOR));
      }

    }

  // Reset tip timer if nothing's going on
  getApp()->addTimeout(this,ID_TIPTIMER,getApp()->getMenuPause());

  // Get item we're over
  cursor=getItemAt(event->win_x,event->win_y);

  // Force GUI update only when needed
  return (cursor!=oldcursor)||(flg&FLAG_TIP);
  }


// Pressed a button
long GMTrackList::onLeftBtnPress(FXObject*,FXSelector,void* ptr){
  FXEvent* event=(FXEvent*)ptr;
  FXint index;
  flags&=~FLAG_TIP;
  handle(this,FXSEL(SEL_FOCUS_SELF,0),ptr);
  if(isEnabled()){
    grab();
    flags&=~FLAG_UPDATE;

    // First change callback
    if(target && target->tryHandle(this,FXSEL(SEL_LEFTBUTTONPRESS,message),ptr)) return 1;

    // Locate item
    index=getItemAt(event->win_x,event->win_y);

    // No item
    if(index<0){
      return 1;
      }

    // Previous selection state
    state=items[index]->isSelected();

    // Change current item
    setCurrentItem(index,true);

    FXint item = header->getItemAt(event->win_x);
    if (getHeaderType(item)==HEADER_RATING && getHeaderData(item)->target) {
      FXint tw = getFont()->getTextWidth(starset);
      FXint xx = header->getItemOffset(item) + 2 + (FXint)((double)tw/(double)2);
      FXuchar rating=0;
      for (FXint i=0;i<5;i++) {
        if (xx+(i*tw)<event->win_x) {
          rating++;
          continue;
          }
        break;
        }
      ungrab();
      getHeaderData(item)->target->handle(this,FXSEL(SEL_COMMAND,getHeaderData(item)->message),(void*)(FXival)rating);
      return 0;
      }




    // Change item selection
    switch(options&SELECT_MASK){
      case TRACKLIST_EXTENDEDSELECT:
        if(event->state&SHIFTMASK){
          if(0<=anchor){
            selectItem(anchor,true);
            extendSelection(index,true);
            }
          else{
            selectItem(index,true);
            setAnchorItem(index);
            }
          }
        else if(event->state&CONTROLMASK){
          if(!state) selectItem(index,true);
          setAnchorItem(index);
          }
        else{
          if(!state){ killSelection(true); selectItem(index,true); }
          setAnchorItem(index);
          }
        break;
      case TRACKLIST_MULTIPLESELECT:
      case TRACKLIST_SINGLESELECT:
        if(!state) selectItem(index,true);
        break;
      }

    // Are we dragging?
    if(/*state && */items[index]->isSelected() && items[index]->isDraggable()){
      flags|=FLAG_TRYDRAG;
      }

    flags|=FLAG_PRESSED;
    return 1;
    }
  return 0;
  }


// Released button
long GMTrackList::onLeftBtnRelease(FXObject*,FXSelector,void* ptr){
  FXEvent* event=(FXEvent*)ptr;
  FXuint flg=flags;
  if(isEnabled()){
    ungrab();
    stopAutoScroll();
    flags|=FLAG_UPDATE;
    flags&=~(FLAG_PRESSED|FLAG_TRYDRAG|FLAG_LASSO|FLAG_DODRAG);

    // First chance callback
    if(target && target->tryHandle(this,FXSEL(SEL_LEFTBUTTONRELEASE,message),ptr)) return 1;

    // Was dragging
    if(flg&FLAG_DODRAG){
      handle(this,FXSEL(SEL_ENDDRAG,0),ptr);
      return 1;
      }

    // Must have pressed
    if(flg&FLAG_PRESSED){

      // Selection change
      switch(options&SELECT_MASK){
        case TRACKLIST_EXTENDEDSELECT:
          if(0<=current){
            if(event->state&CONTROLMASK){
              if(state) deselectItem(current,true);
              }
            else if(!(event->state&SHIFTMASK)){
              if(state){ killSelection(true); selectItem(current,true); }
              }
            }
          break;
        case TRACKLIST_MULTIPLESELECT:
        case TRACKLIST_SINGLESELECT:
          if(0<=current){
            if(state) deselectItem(current,true);
            }
          break;
        }

      // Scroll to make item visibke
      makeItemVisible(current);

      // Update anchor
      setAnchorItem(current);

      // Generate clicked callbacks
      if(event->click_count==1){
        handle(this,FXSEL(SEL_CLICKED,0),(void*)(FXival)current);
        }
      else if(event->click_count==2){
        handle(this,FXSEL(SEL_DOUBLECLICKED,0),(void*)(FXival)current);
        }
      else if(event->click_count==3){
        handle(this,FXSEL(SEL_TRIPLECLICKED,0),(void*)(FXival)current);
        }

      // Command callback only when clicked on item
      if(0<=current){
        handle(this,FXSEL(SEL_COMMAND,0),(void*)(FXival)current);
        }
      }
    return 1;
    }
  return 0;
  }


// Pressed right button
long GMTrackList::onRightBtnPress(FXObject*,FXSelector,void* ptr){
  FXEvent* event=(FXEvent*)ptr;
  flags&=~FLAG_TIP;
  handle(this,FXSEL(SEL_FOCUS_SELF,0),ptr);
  if(isEnabled()){
    grab();
    flags&=~FLAG_UPDATE;
    if(target && target->tryHandle(this,FXSEL(SEL_RIGHTBUTTONPRESS,message),ptr)) return 1;
    flags|=FLAG_SCROLLING;
    grabx=event->win_x-pos_x;
    graby=event->win_y-pos_y;
    return 1;
    }
  return 0;
  }


// Released right button
long GMTrackList::onRightBtnRelease(FXObject*,FXSelector,void* ptr){
  if(isEnabled()){
    ungrab();
    flags&=~FLAG_SCROLLING;
    flags|=FLAG_UPDATE;
    if(target) target->tryHandle(this,FXSEL(SEL_RIGHTBUTTONRELEASE,message),ptr);
    return 1;
    }
  return 0;
  }


// The widget lost the grab for some reason
long GMTrackList::onUngrabbed(FXObject* sender,FXSelector sel,void* ptr){
  FXScrollArea::onUngrabbed(sender,sel,ptr);
  flags&=~(FLAG_DODRAG|FLAG_LASSO|FLAG_TRYDRAG|FLAG_PRESSED|FLAG_CHANGED|FLAG_SCROLLING);
  flags|=FLAG_UPDATE;
  stopAutoScroll();
  return 1;
  }


// Command message
long GMTrackList::onCommand(FXObject*,FXSelector,void* ptr){
  return target && target->tryHandle(this,FXSEL(SEL_COMMAND,message),ptr);
  }


// Clicked in list
long GMTrackList::onClicked(FXObject*,FXSelector,void* ptr){
  return target && target->tryHandle(this,FXSEL(SEL_CLICKED,message),ptr);
  }


// Double Clicked in list; ptr may or may not point to an item
long GMTrackList::onDoubleClicked(FXObject*,FXSelector,void* ptr){
  return target && target->tryHandle(this,FXSEL(SEL_DOUBLECLICKED,message),ptr);
  }


// Triple Clicked in list; ptr may or may not point to an item
long GMTrackList::onTripleClicked(FXObject*,FXSelector,void* ptr){
  return target && target->tryHandle(this,FXSEL(SEL_TRIPLECLICKED,message),ptr);
  }

// Retrieve item
GMTrackItem *GMTrackList::getItem(FXint index) const {
  if(index<0 || items.no()<=index){ fxerror("%s::getItem: index out of range.\n",getClassName()); }
  return items[index];
  }


// Replace item with another
FXint GMTrackList::setItem(FXint index,GMTrackItem* item,FXbool notify){

  // Must have item
  if(!item){ fxerror("%s::setItem: item is nullptr.\n",getClassName()); }

  // Must be in range
  if(index<0 || items.no()<=index){ fxerror("%s::setItem: index out of range.\n",getClassName()); }

  // Notify item will be replaced
  if(notify && target){target->tryHandle(this,FXSEL(SEL_REPLACED,message),(void*)(FXival)index);}

  // Copy the state over
  item->state=items[index]->state;

  // Delete old
  delete items[index];

  // Add new
  items[index]=item;

  // Redo layout
  recalc();
  return index;
  }

// Insert item
FXint GMTrackList::insertItem(FXint index,GMTrackItem* item,FXbool notify){
  FXint old=current;

  // Must have item
  if(!item){ fxerror("%s::insertItem: item is nullptr.\n",getClassName()); }

  // Must be in range
  if(index<0 || items.no()<index){ fxerror("%s::insertItem: index out of range.\n",getClassName()); }

  // Add item to list
  items.insert(index,item);

  // Adjust indices
  if(anchor>=index)  anchor++;
  if(extent>=index)  extent++;
  if(current>=index) current++;
  if(viewable>=index) viewable++;
  if(current<0 && items.no()==1) current=0;

  // Notify item has been inserted
  if(notify && target){target->tryHandle(this,FXSEL(SEL_INSERTED,message),(void*)(FXival)index);}

  // Current item may have changed
  if(old!=current){
    if(notify && target){target->tryHandle(this,FXSEL(SEL_CHANGED,message),(void*)(FXival)current);}
    }

  // Was new item
  if(0<=current && current==index){
    if(hasFocus()){
      items[current]->setFocus(true);
      }
    if((options&SELECT_MASK)==TRACKLIST_BROWSESELECT){
      selectItem(current,notify);
      }
    }

  // Redo layout
  recalc();
  return index;
  }


// Append item
FXint GMTrackList::appendItem(GMTrackItem* item,FXbool notify){
  return insertItem(items.no(),item,notify);
  }


// Prepend item
FXint GMTrackList::prependItem(GMTrackItem* item,FXbool notify){
  return insertItem(0,item,notify);
  }


// Move item from oldindex to newindex
FXint GMTrackList::moveItem(FXint newindex,FXint oldindex,FXbool notify){
  FXint old=current;
  GMTrackItem *item;

  // Must be in range
  if(newindex<0 || oldindex<0 || items.no()<=newindex || items.no()<=oldindex){ fxerror("%s::moveItem: index out of range.\n",getClassName()); }

  // Did it change?
  if(oldindex!=newindex){

    // Move item
    item=items[oldindex];
    items.erase(oldindex);
    items.insert(newindex,item);

    // Move item down
    if(newindex<oldindex){
      if(newindex<=anchor && anchor<oldindex) anchor++;
      if(newindex<=extent && extent<oldindex) extent++;
      if(newindex<=current && current<oldindex) current++;
      if(newindex<=viewable && viewable<oldindex) viewable++;
      }

    // Move item up
    else{
      if(oldindex<anchor && anchor<=newindex) anchor--;
      if(oldindex<extent && extent<=newindex) extent--;
      if(oldindex<current && current<=newindex) current--;
      if(oldindex<viewable && viewable<=newindex) viewable--;
      }

    // Adjust if it was equal
    if(anchor==oldindex) anchor=newindex;
    if(extent==oldindex) extent=newindex;
    if(current==oldindex) current=newindex;
    if(viewable==oldindex) viewable=newindex;

    // Current item may have changed
    if(old!=current){
      if(notify && target){target->tryHandle(this,FXSEL(SEL_CHANGED,message),(void*)(FXival)current);}
      }

    // Redo layout
    recalc();
    }
  return newindex;
  }


// Extract node from list
GMTrackItem* GMTrackList::extractItem(FXint index,FXbool notify){
  GMTrackItem *result;
  FXint old=current;

  // Must be in range
  if(index<0 || items.no()<=index){ fxerror("%s::extractItem: index out of range.\n",getClassName()); }

  // Notify item will be deleted
  if(notify && target){target->tryHandle(this,FXSEL(SEL_DELETED,message),(void*)(FXival)index);}

  // Extract item
  result=items[index];

  // Remove from list
  items.erase(index);

  // Adjust indices
  if(anchor>index || anchor>=items.no())  anchor--;
  if(extent>index || extent>=items.no())  extent--;
  if(current>index || current>=items.no()) current--;
  if(viewable>index || viewable>=items.no())  viewable--;

  // Current item has changed
  if(index<=old){
    if(notify && target){target->tryHandle(this,FXSEL(SEL_CHANGED,message),(void*)(FXival)current);}
    }

  // Deleted current item
  if(0<=current && index==old){
    if(hasFocus()){
      items[current]->setFocus(true);
      }
    if((options&SELECT_MASK)==TRACKLIST_BROWSESELECT){
      selectItem(current,notify);
      }
    }

  // Redo layout
  recalc();

  // Return item
  return result;
  }


// Remove node from list
void GMTrackList::removeItem(FXint index,FXbool notify){
  FXint old=current;

  // Must be in range
  if(index<0 || items.no()<=index){ fxerror("%s::removeItem: index out of range.\n",getClassName()); }

  // Notify item will be deleted
  if(notify && target){target->tryHandle(this,FXSEL(SEL_DELETED,message),(void*)(FXival)index);}

  // Delete item
  delete items[index];

  // Remove from list
  items.erase(index);

  // Adjust indices
  if(anchor>index || anchor>=items.no())  anchor--;
  if(extent>index || extent>=items.no())  extent--;
  if(current>index || current>=items.no()) current--;
  if(viewable>index || viewable>=items.no())  viewable--;

  // Current item has changed
  if(index<=old){
    if(notify && target){target->tryHandle(this,FXSEL(SEL_CHANGED,message),(void*)(FXival)current);}
    }

  // Deleted current item
  if(0<=current && index==old){
    if(hasFocus()){
      items[current]->setFocus(true);
      }
    if((options&SELECT_MASK)==TRACKLIST_BROWSESELECT){
      selectItem(current,notify);
      }
    }

  // Redo layout
  recalc();
  }


// Remove all items
void GMTrackList::clearItems(FXbool notify){
  FXint old=current;

  // Delete items
  for(FXint index=items.no()-1; 0<=index; index--){
    if(notify && target){target->tryHandle(this,FXSEL(SEL_DELETED,message),(void*)(FXival)index);}
    delete items[index];
    }

  // Free array
  items.clear();

  // Adjust indices
  current=-1;
  anchor=-1;
  extent=-1;
  viewable=-1;

  // Current item has changed
  if(old!=-1){
    if(notify && target){target->tryHandle(this,FXSEL(SEL_CHANGED,message),(void*)(FXival)-1);}
    }

  // Redo layout
  recalc();
  }


// Change the font
void GMTrackList::setFont(FXFont* fnt){
  if(!fnt){ fxerror("%s::setFont: nullptr font specified.\n",getClassName()); }
  if(font!=fnt){
    font=fnt;
    recalc();
    update();
    }
  }

// Change the font
void GMTrackList::setActiveFont(FXFont* fnt){
  if(!fnt){ fxerror("%s::setFont: nullptr font specified.\n",getClassName()); }
  if(activeFont!=fnt){
    activeFont=fnt;
    recalc();
    update();
    }
  }



// Set text color
void GMTrackList::setTextColor(FXColor clr){
  if(clr!=textColor){
    textColor=clr;
    update();
    }
  }


// Set select background color
void GMTrackList::setSelBackColor(FXColor clr){
  if(clr!=selbackColor){
    selbackColor=clr;
    update();
    }
  }


// Set selected text color
void GMTrackList::setSelTextColor(FXColor clr){
  if(clr!=seltextColor){
    seltextColor=clr;
    update();
    }
  }

// Change the row color
void GMTrackList::setRowColor(FXColor clr){
  if(clr!=rowColor){
    rowColor=clr;
    update();
    }
  }

// Change the active color
void GMTrackList::setActiveColor(FXColor clr){
  if(clr!=activeColor){
    activeColor=clr;
    update();
    }
  }

// Change the active color
void GMTrackList::setShadowColor(FXColor clr){
  if(clr!=shadowColor){
    shadowColor=clr;
    update();
    }
  }

// Change the active text color
void GMTrackList::setActiveTextColor(FXColor clr){
  if(clr!=activeTextColor){
    activeTextColor=clr;
    update();
    }
  }


// Change list style
void GMTrackList::setListStyle(FXuint style){
  FXuint opts=(options&~TRACKLIST_MASK) | (style&TRACKLIST_MASK);
  if(options!=opts){
    options=opts;
    recalc();
    }
  }


// Get list style
FXuint GMTrackList::getListStyle() const {
  return (options&TRACKLIST_MASK);
  }


// Change help text
void GMTrackList::setHelpText(const FXString& text){
  help=text;
  }


// Save data
void GMTrackList::save(FXStream& store) const {
  FXScrollArea::save(store);
  store << header;
  store << anchor;
  store << current;
  store << extent;
  store << font;
  store << textColor;
  store << selbackColor;
  store << seltextColor;
  store << lineHeight;
  store << help;
  }


// Load data
void GMTrackList::load(FXStream& store){
  FXScrollArea::load(store);
  store >> header;
  store >> anchor;
  store >> current;
  store >> extent;
  store >> font;
  store >> textColor;
  store >> selbackColor;
  store >> seltextColor;
  store >> lineHeight;
  store >> help;
  }


// Cleanup
GMTrackList::~GMTrackList(){
  getApp()->removeTimeout(this,ID_TIPTIMER);
  getApp()->removeTimeout(this,ID_WHEEL_TIMEOUT);
  clearItems(false);
  header=(FXHeader*)-1L;
  font=(FXFont*)-1L;
  activeFont=(FXFont*)-1L;
  }
