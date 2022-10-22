/********************************************************************************
*                                                                               *
*                      S c r o l l A r e a   W i d g e t                        *
*                                                                               *
*********************************************************************************
* Copyright (C) 1998,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "FXDCWindow.h"
#include "FXApp.h"
#include "FXScrollBar.h"
#include "FXScrollArea.h"


/*
  To do:
  - When tabbing, we will never put focus on scrollbar.
  - Perhaps scroll windows should observe FRAME_SUNKEN etc.
  - Here's a new idea:- perhaps the scrollbars should be GUI-updated from the
    FXScrollArea.  Then layout() will do nothing but place the bars.
  - What if we want to keep two scrolled windows in sync, i.e. scroll them
    both the same amount.
  - The original content size is computed by getContentWidth() and getContentHeight().
    In the typical subclass, content size is computed only when there's reason to believe
    it has changed; a cached value is returned otherwise.
  - Note that the content size does NOT include the sizes of fixed elements like headers or
    rulers; it should return ONLY the size of the scrolled document.
  - The placement of the viewport is returned by getVisibleX(), getVisibleY(), getVisibleWidth(),
    and getVisibleHeight().  Subclass these when header controls need to be subtracted from
    the visible area.
    If the document is not scrolled, (getVisibleX(), getVisibleY()) corresponds to
    the document coordinate (0,0).  Drawing should be relative to getVisibleX() and
    getVisibleY() so that subclasses may place header controls around the visible area.
  - The getVisibleX(), getVisibleY(), getVisibleWidth() and getVisibleHeight()
    identify the scrollable part in a FXScrollArea subclass; only pixels inside
    this area are normally scrolled.  Also, this area determines the auto-scroll
    processing; when cursor position is near the edge of the visible part, auto-
    scrolling commences.  Thus, autoscrolling can start while the cursor is still
    fully inside the window but near the visible scroll-area.
  - When computing default width, the content width is assumed to be 0, unless the
    HSCROLLING_OFF mode is in effect.  Then the width of the vertical scrollbar is
    added unless the vertical scrollbar is suppressed (using VSCROLLER_NEVER).
    A subclass may subsequently add any additional space needed for headers or rulers.
  - Likewise, when computing default height, the content height is assumed to be 0, unless
    the VSCROLLING_OFF mode is in effect.  Then the height of the horizontal scrollbar is
    added unless the horizontal scrollbar is suppressed (using HSCROLLER_NEVER).
    A subclass may subsequently add any additional space needed for headers or rulers.
  - Thus, the minimum size will be such that at scrollbars (and possible other fixed-
    elements such as headers or rules) will be fully visible.
  - Note that the horizontal scrollbar width NO LONGER influences the default width,
    and the vertical scrollbar NO LONGER influences default height.  This was done
    to get accurate minimum sizes for subclasses which add headers or rulers.
  - Should honor LAYOUT_XXX options passed to FXScrollBars.  This will be handy
    to allow alternate scrollbar placement strategies (for this to work the subclasses
    will need to be fixed to use getVisibleX() and getVisibleY() everywhere).
*/


#define AUTOSCROLL_FUDGE  11       // Proximity to wall at which we start autoscrolling
#define SCROLLER_MASK     (HSCROLLER_ALWAYS|HSCROLLER_NEVER|VSCROLLER_ALWAYS|VSCROLLER_NEVER|SCROLLERS_DONT_TRACK)

using namespace FX;

/*******************************************************************************/

namespace FX {

FXDEFMAP(FXScrollArea) FXScrollAreaMap[]={
  FXMAPFUNC(SEL_MOUSEWHEEL,0,FXScrollArea::onVMouseWheel),
  FXMAPFUNC(SEL_TIMEOUT,FXScrollArea::ID_AUTOSCROLL,FXScrollArea::onAutoScroll),
  FXMAPFUNC(SEL_COMMAND,FXScrollArea::ID_HSCROLLED,FXScrollArea::onHScrollerChanged),
  FXMAPFUNC(SEL_COMMAND,FXScrollArea::ID_VSCROLLED,FXScrollArea::onVScrollerChanged),
  FXMAPFUNC(SEL_CHANGED,FXScrollArea::ID_HSCROLLED,FXScrollArea::onHScrollerDragged),
  FXMAPFUNC(SEL_CHANGED,FXScrollArea::ID_VSCROLLED,FXScrollArea::onVScrollerDragged),
  };


// Object implementation
FXIMPLEMENT(FXScrollArea,FXComposite,FXScrollAreaMap,ARRAYNUMBER(FXScrollAreaMap))


// Scroll acceleration near edge
static const FXint acceleration[AUTOSCROLL_FUDGE+1]={1,1,1,2,3,4,6,7,8,16,32,64};


// Deserialization
FXScrollArea::FXScrollArea(){
  flags|=FLAG_SHOWN;
  horizontal=nullptr;
  vertical=nullptr;
  corner=nullptr;
  pos_x=0;
  pos_y=0;
  }


// Construct and init
FXScrollArea::FXScrollArea(FXComposite* p,FXuint opts,FXint x,FXint y,FXint w,FXint h):FXComposite(p,opts,x,y,w,h){
  flags|=FLAG_SHOWN;
  horizontal=new FXScrollBar(this,this,ID_HSCROLLED,(opts&SCROLLERS_DONT_TRACK)?(SCROLLBAR_HORIZONTAL|SCROLLBAR_WHEELJUMP|LAYOUT_SIDE_BOTTOM):(SCROLLBAR_HORIZONTAL|LAYOUT_SIDE_BOTTOM));
  vertical=new FXScrollBar(this,this,ID_VSCROLLED,(opts&SCROLLERS_DONT_TRACK)?(SCROLLBAR_VERTICAL|SCROLLBAR_WHEELJUMP|LAYOUT_SIDE_RIGHT):(SCROLLBAR_VERTICAL|LAYOUT_SIDE_RIGHT));
  corner=new FXScrollCorner(this);
  backColor=getApp()->getBackColor();
  pos_x=0;
  pos_y=0;
  }


// Return content area width
FXint FXScrollArea::getContentWidth(){
  return 1;
  }


// Return content area height
FXint FXScrollArea::getContentHeight(){
  return 1;
  }


// Return visible scroll-area x position
FXint FXScrollArea::getVisibleX() const {
  return 0;
  }


// Return visible scroll-area y position
FXint FXScrollArea::getVisibleY() const {
  return 0;
  }


// Return visible scroll-area width
FXint FXScrollArea::getVisibleWidth() const {
  return width-vertical->getWidth();
  }


// Return visible scroll-area height
FXint FXScrollArea::getVisibleHeight() const {
  return height-horizontal->getHeight();
  }


// Get default width
FXint FXScrollArea::getDefaultWidth(){
  FXint w=0;
  if((options&HSCROLLER_NEVER)&&(options&HSCROLLER_ALWAYS)) w=getContentWidth();
  if(!(options&VSCROLLER_NEVER)) w+=vertical->getDefaultWidth();
  return FXMAX(w,1);
  }


// Get default height
FXint FXScrollArea::getDefaultHeight(){
  FXint h=0;
  if((options&VSCROLLER_NEVER)&&(options&VSCROLLER_ALWAYS)) h=getContentHeight();
  if(!(options&HSCROLLER_NEVER)) h+=horizontal->getDefaultHeight();
  return FXMAX(h,1);
  }


// Move content
void FXScrollArea::moveContents(FXint x,FXint y){
  scroll(getVisibleX(),getVisibleY(),getVisibleWidth(),getVisibleHeight(),x-pos_x,y-pos_y);
  pos_x=x;
  pos_y=y;
  }


// Start automatic scrolling
FXbool FXScrollArea::startAutoScroll(FXEvent *event,FXbool onlywheninside){
  FXint vx=getVisibleX();
  FXint vy=getVisibleY();
  FXint vw=getVisibleWidth();
  FXint vh=getVisibleHeight();
  FXbool autoscrolling=false;
  flags&=~FLAG_SCROLLINSIDE;
  if(onlywheninside) flags|=FLAG_SCROLLINSIDE;
  if(horizontal->getPage()<horizontal->getRange()){
    if((event->win_x<vx+AUTOSCROLL_FUDGE) && (0<horizontal->getPosition())) autoscrolling=true;
    else if((vx+vw-AUTOSCROLL_FUDGE<=event->win_x) && (horizontal->getPosition()<horizontal->getRange()-horizontal->getPage())) autoscrolling=true;
    }
  if(vertical->getPage()<vertical->getRange()){
    if((event->win_y<vy+AUTOSCROLL_FUDGE) && (0<vertical->getPosition())) autoscrolling=true;
    else if((vy+vh-AUTOSCROLL_FUDGE<=event->win_y) && (vertical->getPosition()<vertical->getRange()-vertical->getPage())) autoscrolling=true;
    }
  if(onlywheninside && (event->win_x<vx || event->win_y<vy || vx+vw<=event->win_x || vy+vh<=event->win_y)) autoscrolling=false;
  if(autoscrolling){
    if(!getApp()->hasTimeout(this,ID_AUTOSCROLL)){
      getApp()->addTimeout(this,ID_AUTOSCROLL,getApp()->getScrollSpeed(),event);
      }
    }
  else{
    getApp()->removeTimeout(this,ID_AUTOSCROLL);
    }
  return autoscrolling;
  }


// Stop automatic scrolling
void FXScrollArea::stopAutoScroll(){
  getApp()->removeTimeout(this,ID_AUTOSCROLL);
  flags&=~FLAG_SCROLLINSIDE;
  }


// Place scrollbars
void FXScrollArea::placeScrollBars(FXint vw,FXint vh){
  FXint cw,ch,new_x,new_y;
  FXint sh_h=0;
  FXint sv_w=0;

  // Inviolate
  FXASSERT(pos_x<=0 && pos_y<=0);

  // Get content size
  cw=getContentWidth();
  ch=getContentHeight();

  // Get dimensions of the scroll bars
  if(!(options&HSCROLLER_NEVER)) sh_h=horizontal->getDefaultHeight();
  if(!(options&VSCROLLER_NEVER)) sv_w=vertical->getDefaultWidth();

  // Should we disable the scroll bars?  A bit tricky as the scrollbars
  // may influence each other's presence.  Also, we don't allow more than
  // 50% of the viewport to be taken up by scrollbars; when the scrollbars
  // take up more than 50% of the available space we simply turn them off.
  if(!(options&(HSCROLLER_ALWAYS|VSCROLLER_ALWAYS)) && (cw<=vw) && (ch<=vh)){sh_h=sv_w=0;}
  if(!(options&HSCROLLER_ALWAYS) && ((cw<=vw-sv_w) || (0>=vh-sh_h-sh_h))) sh_h=0;
  if(!(options&VSCROLLER_ALWAYS) && ((ch<=vh-sh_h) || (0>=vw-sv_w-sv_w))) sv_w=0;
  if(!(options&HSCROLLER_ALWAYS) && ((cw<=vw-sv_w) || (0>=vh-sh_h-sh_h))) sh_h=0;

  // Viewport size with scroll bars taken into account
  vw-=sv_w;
  vh-=sh_h;

  // Adjust content size, now that we know about those scroll bars
  if((options&HSCROLLER_NEVER)&&(options&HSCROLLER_ALWAYS)) cw=vw;
  if((options&VSCROLLER_NEVER)&&(options&VSCROLLER_ALWAYS)) ch=vh;

  // Furthermore, content size won't be smaller than the viewport
  if(cw<vw) cw=vw;
  if(ch<vh) ch=vh;

  // Content size
  horizontal->setRange(cw);
  vertical->setRange(ch);

  // Page size may have changed
  horizontal->setPage(vw);
  vertical->setPage(vh);

  // Position may have changed
  horizontal->setPosition(-pos_x);
  vertical->setPosition(-pos_y);

  // Get back the adjusted position
  new_x=-horizontal->getPosition();
  new_y=-vertical->getPosition();

  // Scroll to force position back into range
  if(new_x!=pos_x || new_y!=pos_y){
    moveContents(new_x,new_y);
    }

  // Read back validated position
  pos_x=-horizontal->getPosition();
  pos_y=-vertical->getPosition();

  // Place horizontal scroll bar
  horizontal->position(0,height-sh_h,width-sv_w,sh_h);
  horizontal->raise();

  // Place vertical scroll bar
  vertical->position(width-sv_w,0,sv_w,height-sh_h);
  vertical->raise();

  // Place scroll corner
  corner->position(width-sv_w,height-sh_h,sv_w,sh_h);
  corner->raise();
  }


// Recalculate layout
void FXScrollArea::layout(){

  // Place scroll bars
  placeScrollBars(width,height);

  // Clean
  flags&=~FLAG_DIRTY;
  }


// Set position
void FXScrollArea::setPosition(FXint x,FXint y){
  FXint new_x,new_y;

  // Set scroll bars
  horizontal->setPosition(-x);
  vertical->setPosition(-y);

  // Then read back valid position from scroll bars
  new_x=-horizontal->getPosition();
  new_y=-vertical->getPosition();

  // Move content if there's a change
  if(new_x!=pos_x || new_y!=pos_y){
    moveContents(new_x,new_y);
    }
  }


// Changed
long FXScrollArea::onHScrollerChanged(FXObject*,FXSelector,void* ptr){
  FXint new_x=-(FXint)(FXival)ptr;
  if(new_x!=pos_x){
    moveContents(new_x,pos_y);
    }
  flags&=~FLAG_TIP;
  return 1;
  }


// Changed
long FXScrollArea::onVScrollerChanged(FXObject*,FXSelector,void* ptr){
  FXint new_y=-(FXint)(FXival)ptr;
  if(new_y!=pos_y){
    moveContents(pos_x,new_y);
    }
  flags&=~FLAG_TIP;
  return 1;
  }


// Dragged
long FXScrollArea::onHScrollerDragged(FXObject*,FXSelector,void* ptr){
  if(!(options&SCROLLERS_DONT_TRACK)){
    FXint new_x=-(FXint)(FXival)ptr;
    if(new_x!=pos_x){
      moveContents(new_x,pos_y);
      }
    }
  flags&=~FLAG_TIP;
  return 1;
  }


// Dragged
long FXScrollArea::onVScrollerDragged(FXObject*,FXSelector,void* ptr){
  if(!(options&SCROLLERS_DONT_TRACK)){
    FXint new_y=-(FXint)(FXival)ptr;
    if(new_y!=pos_y){
      moveContents(pos_x,new_y);
      }
    }
  flags&=~FLAG_TIP;
  return 1;
  }


// Mouse wheel used for vertical scrolling
long FXScrollArea::onVMouseWheel(FXObject* sender,FXSelector sel,void* ptr){
  vertical->handle(sender,sel,ptr);
  return 1;
  }


// Mouse wheel used for horizontal scrolling
long FXScrollArea::onHMouseWheel(FXObject* sender,FXSelector sel,void* ptr){
  horizontal->handle(sender,sel,ptr);
  return 1;
  }


// Timeout
long FXScrollArea::onAutoScroll(FXObject*,FXSelector sel,void* ptr){
  FXEvent* event=(FXEvent*)ptr;
  FXint vx=getVisibleX();
  FXint vy=getVisibleY();
  FXint vw=getVisibleWidth();
  FXint vh=getVisibleHeight();
  FXint dx=0;
  FXint dy=0;

  // If scrolling only while inside, and not inside, we stop scrolling
  if((flags&FLAG_SCROLLINSIDE) && !(vx<=event->win_x && vy<=event->win_y && event->win_x<vx+vw && event->win_y<vy+vh)) return 0;

  // Figure scroll amount x
  if(event->win_x<vx+AUTOSCROLL_FUDGE) dx=vx+AUTOSCROLL_FUDGE-event->win_x;
  else if(vx+vw-AUTOSCROLL_FUDGE<=event->win_x) dx=vx+vw-AUTOSCROLL_FUDGE-event->win_x;

  // Figure scroll amount y
  if(event->win_y<vy+AUTOSCROLL_FUDGE) dy=vy+AUTOSCROLL_FUDGE-event->win_y;
  else if(vy+vh-AUTOSCROLL_FUDGE<=event->win_y) dy=vy+vh-AUTOSCROLL_FUDGE-event->win_y;

  // Keep autoscrolling
  if(dx || dy){
    FXint oldposx=pos_x;
    FXint oldposy=pos_y;
    if(flags&FLAG_SCROLLINSIDE){
      FXASSERT(FXABS(dx)<=AUTOSCROLL_FUDGE);
      FXASSERT(FXABS(dy)<=AUTOSCROLL_FUDGE);
      dx*=acceleration[FXABS(dx)];
      dy*=acceleration[FXABS(dy)];
      }

    // Scroll a bit
    setPosition(pos_x+dx,pos_y+dy);

    // Setup next timer if we can still scroll some more
    if((pos_x!=oldposx) || (pos_y!=oldposy)){
      getApp()->addTimeout(this,FXSELID(sel),getApp()->getScrollSpeed(),event);
      }
    }

  // Kill tip
  flags&=~FLAG_TIP;
  return 0;
  }


// True if horizontally scrollable enabled
FXbool FXScrollArea::isHorizontalScrollable() const {
  return !((options&HSCROLLER_NEVER) && (options&HSCROLLER_ALWAYS));
  }


// True if vertically scrollable enabled
FXbool FXScrollArea::isVerticalScrollable() const {
  return !((options&VSCROLLER_NEVER) && (options&VSCROLLER_ALWAYS));
  }


// Set scroll style
void FXScrollArea::setScrollStyle(FXuint style){
  FXuint opts=(options&~SCROLLER_MASK) | (style&SCROLLER_MASK);
  if(options!=opts){
    if(opts&SCROLLERS_DONT_TRACK){
      horizontal->setScrollBarStyle(horizontal->getScrollBarStyle()|SCROLLBAR_WHEELJUMP);
      vertical->setScrollBarStyle(vertical->getScrollBarStyle()|SCROLLBAR_WHEELJUMP);
      }
    else{
      horizontal->setScrollBarStyle(horizontal->getScrollBarStyle()&~SCROLLBAR_WHEELJUMP);
      vertical->setScrollBarStyle(vertical->getScrollBarStyle()&~SCROLLBAR_WHEELJUMP);
      }
    options=opts;
    recalc();
    }
  }


// Get scroll style
FXuint FXScrollArea::getScrollStyle() const {
  return (options&SCROLLER_MASK);
  }


// Save object to stream
void FXScrollArea::save(FXStream& store) const {
  FXComposite::save(store);
  store << horizontal;
  store << vertical;
  store << corner;
  store << pos_x;
  store << pos_y;
  }


// Load object from stream
void FXScrollArea::load(FXStream& store){
  FXComposite::load(store);
  store >> horizontal;
  store >> vertical;
  store >> corner;
  store >> pos_x;
  store >> pos_y;
  }


// Clean up
FXScrollArea::~FXScrollArea(){
  getApp()->removeTimeout(this,ID_AUTOSCROLL);
  horizontal=(FXScrollBar*)-1L;
  vertical=(FXScrollBar*)-1L;
  corner=(FXScrollCorner*)-1L;
  }

}
