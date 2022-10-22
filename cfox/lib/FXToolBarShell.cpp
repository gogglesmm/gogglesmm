/********************************************************************************
*                                                                               *
*                    T o o l   B a r   S h e l l   W i d g e t                  *
*                                                                               *
*********************************************************************************
* Copyright (C) 2000,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "FXEvent.h"
#include "FXWindow.h"
#include "FXDCWindow.h"
#include "FXApp.h"
#include "FXCursor.h"
#include "FXToolBarShell.h"

/*
  Notes:
  - Managed by Window Manager because it needs to stay on top of window.
  - Window manager may hide it when application does not have focus.
  - If it has a child and the child is shown, it will show, otherwise it'll hide.
  - When there was a recalc() because something changed in the interior of the
    widget, the onLayout() routine tries to shrink-wrap the FXToolBarShell around
    its contents.  This takes care of changes in the child.
  - You can resize if the widget inside the FXToolBarShell has LAYOUT_FIX_WIDTH or
    LAYOUT_FIX_HEIGHT.
*/


#define HANDLE_SIZE  6
#define FRAME_MASK   (FRAME_SUNKEN|FRAME_RAISED|FRAME_THICK)

using namespace FX;

/*******************************************************************************/

namespace FX {

// Map
FXDEFMAP(FXToolBarShell) FXToolBarShellMap[]={
  FXMAPFUNC(SEL_PAINT,0,FXToolBarShell::onPaint),
  FXMAPFUNC(SEL_MOTION,0,FXToolBarShell::onMotion),
  FXMAPFUNC(SEL_ENTER,0,FXToolBarShell::onEnter),
  FXMAPFUNC(SEL_LEAVE,0,FXToolBarShell::onLeave),
  FXMAPFUNC(SEL_LEFTBUTTONPRESS,0,FXToolBarShell::onLeftBtnPress),
  FXMAPFUNC(SEL_LEFTBUTTONRELEASE,0,FXToolBarShell::onLeftBtnRelease),
  FXMAPFUNC(SEL_CHORE,FXToolBarShell::ID_LAYOUT,FXToolBarShell::onLayout),
  };



// Object implementation
FXIMPLEMENT(FXToolBarShell,FXTopWindow,FXToolBarShellMap,ARRAYNUMBER(FXToolBarShellMap))



// Cursor shape based on mode
const FXDefaultCursor FXToolBarShell::cursorType[16]={
  DEF_ARROW_CURSOR,
  DEF_DRAGH_CURSOR,     // DRAG_TOP
  DEF_DRAGH_CURSOR,     // DRAG_BOTTOM
  DEF_ARROW_CURSOR,

  DEF_DRAGV_CURSOR,     // DRAG_LEFT
  DEF_DRAGTL_CURSOR,    // DRAG_LEFT DRAG_TOP
  DEF_DRAGTR_CURSOR,    // DRAG_LEFT DRAG_BOTTOM
  DEF_ARROW_CURSOR,

  DEF_DRAGV_CURSOR,     // DRAG_RIGHT
  DEF_DRAGTR_CURSOR,    // DRAG_RIGHT DRAG_TOP
  DEF_DRAGTL_CURSOR,    // DRAG_RIGHT DRAG_BOTTOM
  DEF_ARROW_CURSOR,

  DEF_ARROW_CURSOR,
  DEF_ARROW_CURSOR,
  DEF_ARROW_CURSOR,
  DEF_MOVE_CURSOR,      // DRAG_WHOLE
  };


// Constructor
FXToolBarShell::FXToolBarShell(){
  flags|=FLAG_ENABLED;
  baseColor=0;
  hiliteColor=0;
  shadowColor=0;
  borderColor=0;
  border=0;
  gripx=0;
  gripy=0;
  xopp=0;
  yopp=0;
  mode=DRAG_NONE;
  }


// Make toolbar shell
FXToolBarShell::FXToolBarShell(FXWindow* own,FXuint opts,FXint x,FXint y,FXint w,FXint h,FXint hs,FXint vs):FXTopWindow(own,FXString::null,nullptr,nullptr,(opts|DECOR_SHRINKABLE|DECOR_STRETCHABLE)&~(DECOR_TITLE|DECOR_MINIMIZE|DECOR_MAXIMIZE|DECOR_CLOSE|DECOR_BORDER|DECOR_MENU),x,y,w,h,0,0,0,0,hs,vs){
  flags|=FLAG_ENABLED;
  baseColor=getApp()->getBaseColor();
  hiliteColor=getApp()->getHiliteColor();
  shadowColor=getApp()->getShadowColor();
  borderColor=getApp()->getBorderColor();
  border=(options&FRAME_THICK)?2:(options&(FRAME_SUNKEN|FRAME_RAISED))?1:0;
  gripx=0;
  gripy=0;
  xopp=0;
  yopp=0;
  mode=DRAG_NONE;
  }


// Create window
void FXToolBarShell::create(){
  FXTopWindow::create();
  if(getFirst() && getFirst()->shown()) show();
  }



void FXToolBarShell::drawBorderRectangle(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h){
  dc.setForeground(borderColor);
  dc.drawRectangle(x,y,w-1,h-1);
  }


void FXToolBarShell::drawRaisedRectangle(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h){
  dc.setForeground(shadowColor);
  dc.fillRectangle(x,y+h-1,w,1);
  dc.fillRectangle(x+w-1,y,1,h);
  dc.setForeground(hiliteColor);
  dc.fillRectangle(x,y,w,1);
  dc.fillRectangle(x,y,1,h);
  }


void FXToolBarShell::drawSunkenRectangle(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h){
  dc.setForeground(shadowColor);
  dc.fillRectangle(x,y,w,1);
  dc.fillRectangle(x,y,1,h);
  dc.setForeground(hiliteColor);
  dc.fillRectangle(x,y+h-1,w,1);
  dc.fillRectangle(x+w-1,y,1,h);
  }


void FXToolBarShell::drawRidgeRectangle(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h){
  dc.setForeground(hiliteColor);
  dc.fillRectangle(x,y,w,1);
  dc.fillRectangle(x,y,1,h);
  dc.fillRectangle(x+1,y+h-2,w-2,1);
  dc.fillRectangle(x+w-2,y+1,1,h-2);
  dc.setForeground(shadowColor);
  dc.fillRectangle(x+1,y+1,w-3,1);
  dc.fillRectangle(x+1,y+1,1,h-3);
  dc.fillRectangle(x,y+h-1,w,1);
  dc.fillRectangle(x+w-1,y,1,h);
  }


void FXToolBarShell::drawGrooveRectangle(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h){
  dc.setForeground(shadowColor);
  dc.fillRectangle(x,y,w,1);
  dc.fillRectangle(x,y,1,h);
  dc.fillRectangle(x+1,y+h-2,w-2,1);
  dc.fillRectangle(x+w-2,y+1,1,h-2);
  dc.setForeground(hiliteColor);
  dc.fillRectangle(x+1,y+1,w-3,1);
  dc.fillRectangle(x+1,y+1,1,h-3);
  dc.fillRectangle(x,y+h-1,w,1);
  dc.fillRectangle(x+w-1,y,1,h);
  }


void FXToolBarShell::drawDoubleRaisedRectangle(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h){
  dc.setForeground(baseColor);
  dc.fillRectangle(x,y,w-1,1);
  dc.fillRectangle(x,y,1,h-1);
  dc.setForeground(hiliteColor);
  dc.fillRectangle(x+1,y+1,w-2,1);
  dc.fillRectangle(x+1,y+1,1,h-2);
  dc.setForeground(shadowColor);
  dc.fillRectangle(x+1,y+h-2,w-2,1);
  dc.fillRectangle(x+w-2,y+1,1,h-1);
  dc.setForeground(borderColor);
  dc.fillRectangle(x,y+h-1,w,1);
  dc.fillRectangle(x+w-1,y,1,h);
  }


void FXToolBarShell::drawDoubleSunkenRectangle(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h){
  dc.setForeground(shadowColor);
  dc.fillRectangle(x,y,w-1,1);
  dc.fillRectangle(x,y,1,h-1);
  dc.setForeground(borderColor);
  dc.fillRectangle(x+1,y+1,w-3,1);
  dc.fillRectangle(x+1,y+1,1,h-3);
  dc.setForeground(hiliteColor);
  dc.fillRectangle(x,y+h-1,w,1);
  dc.fillRectangle(x+w-1,y,1,h);
  dc.setForeground(baseColor);
  dc.fillRectangle(x+1,y+h-2,w-2,1);
  dc.fillRectangle(x+w-2,y+1,1,h-2);
  }


// Draw border
void FXToolBarShell::drawFrame(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h){
  switch(options&FRAME_MASK){
    case FRAME_LINE: drawBorderRectangle(dc,x,y,w,h); break;
    case FRAME_SUNKEN: drawSunkenRectangle(dc,x,y,w,h); break;
    case FRAME_RAISED: drawRaisedRectangle(dc,x,y,w,h); break;
    case FRAME_GROOVE: drawGrooveRectangle(dc,x,y,w,h); break;
    case FRAME_RIDGE: drawRidgeRectangle(dc,x,y,w,h); break;
    case FRAME_SUNKEN|FRAME_THICK: drawDoubleSunkenRectangle(dc,x,y,w,h); break;
    case FRAME_RAISED|FRAME_THICK: drawDoubleRaisedRectangle(dc,x,y,w,h); break;
    }
  }


// Handle repaint
long FXToolBarShell::onPaint(FXObject*,FXSelector,void* ptr){
  FXEvent *ev=(FXEvent*)ptr;
  FXDCWindow dc(this,ev);
  drawFrame(dc,0,0,width,height);
  return 1;
  }


// Perform layout; return 0 because no GUI update is needed
long FXToolBarShell::onLayout(FXObject*,FXSelector,void*){
  FXuint hints;
  FXint w,h;
  if(getFirst() && getFirst()->shown()){
    hints=getFirst()->getLayoutHints();
    if(hints&LAYOUT_FIX_HEIGHT) h=getFirst()->getHeight();
    else h=getFirst()->getDefaultHeight();
    if(hints&LAYOUT_FIX_WIDTH) w=getFirst()->getWidth();
    else w=getFirst()->getDefaultWidth();
    resize(w+(border<<1),h+(border<<1));
    show();
    }
  else{
    hide();
    }
  return 0;
  }


// Find out where window was grabbed
FXuchar FXToolBarShell::where(FXint x,FXint y) const {
  FXuchar code=DRAG_NONE;
  if(getFirst() && ((0<=x && x<border+padleft) || (0<=y && y<border+padtop) || (width-padright-border<=x && x<width) || (height-padbottom-border<=y && y<height))){
    FXuint hints=getFirst()->getLayoutHints();
    if(hints&LAYOUT_FIX_WIDTH){
      if(x<HANDLE_SIZE) code|=DRAG_LEFT;
      if(width-HANDLE_SIZE<=x) code|=DRAG_RIGHT;
      }
    if(hints&LAYOUT_FIX_HEIGHT){
      if(y<HANDLE_SIZE) code|=DRAG_TOP;
      if(height-HANDLE_SIZE<=y) code|=DRAG_BOTTOM;
      }
    }
  return code;
  }


// Change cursor if we entered the window normally
long FXToolBarShell::onEnter(FXObject* sender,FXSelector sel,void* ptr){
  FXEvent *event=(FXEvent*)ptr;
  FXTopWindow::onEnter(sender,sel,ptr);
  if(event->code!=CROSSINGGRAB){
    setDefaultCursor(getApp()->getDefaultCursor(cursorType[where(event->win_x,event->win_y)]));
    }
  return 1;
  }


// Restore cursor if we left the window normally
long FXToolBarShell::onLeave(FXObject* sender,FXSelector sel,void* ptr){
  FXEvent *event=(FXEvent*)ptr;
  FXTopWindow::onLeave(sender,sel,ptr);
  if(event->code!=CROSSINGUNGRAB){
    setDefaultCursor(getApp()->getDefaultCursor(DEF_ARROW_CURSOR));
    }
  return 1;
  }


// Pressed LEFT button
long FXToolBarShell::onLeftBtnPress(FXObject*,FXSelector,void* ptr){
  FXEvent *event=(FXEvent*)ptr;
  flags&=~FLAG_TIP;
  handle(this,FXSEL(SEL_FOCUS_SELF,0),ptr);
  if(isEnabled()){
    grab();
    if(target && target->tryHandle(this,FXSEL(SEL_LEFTBUTTONPRESS,message),ptr)) return 1;
    mode=where(event->win_x,event->win_y);
    if(mode!=DRAG_NONE){
      if(mode&DRAG_TOP){
        gripy=event->win_y;
        yopp=ypos+height;
        }
      else if(mode&DRAG_BOTTOM){
        gripy=event->win_y-height;
        yopp=ypos;
        }
      if(mode&DRAG_LEFT){
        gripx=event->win_x;
        xopp=xpos+width;
        }
      else if(mode&DRAG_RIGHT){
        gripx=event->win_x-width;
        xopp=xpos;
        }
      setDragCursor(getApp()->getDefaultCursor(cursorType[mode]));
      }
    return 1;
    }
  return 0;
  }


// Released LEFT button
long FXToolBarShell::onLeftBtnRelease(FXObject*,FXSelector,void* ptr){
  if(isEnabled()){
    ungrab();
    mode=DRAG_NONE;
    setDragCursor(getApp()->getDefaultCursor(DEF_ARROW_CURSOR));
    if(target && target->tryHandle(this,FXSEL(SEL_LEFTBUTTONRELEASE,message),ptr)) return 1;
    return 1;
    }
  return 0;
  }


// Moved the mouse
long FXToolBarShell::onMotion(FXObject*,FXSelector,void* ptr){
  FXEvent *event=(FXEvent*)ptr;

  // We were dragging
  if(mode!=DRAG_NONE){

    // Old pos
    FXint x=xpos;
    FXint y=ypos;
    FXint w=width;
    FXint h=height;

    // Minimum size
    FXint mw=padright+padleft+border+border+1;
    FXint mh=padtop+padbottom+border+border+1;

    // Vertical
    if(mode&DRAG_TOP){
      y=event->root_y-gripy;
      if(y>yopp-mh) y=yopp-mh;
      h=yopp-y;
      }
    else if(mode&DRAG_BOTTOM){
      y=yopp;
      h=event->root_y-gripy-yopp;
      if(h<mh) h=mh;
      }

    // Horizontal
    if(mode&DRAG_LEFT){
      x=event->root_x-gripx;
      if(x>xopp-mw) x=xopp-mw;
      w=xopp-x;
      }
    else if(mode&DRAG_RIGHT){
      x=xopp;
      w=event->root_x-gripx-xopp;
      if(w<mw) w=mw;
      }

    // New placement
    position(x,y,w,h);
    return 1;
    }

  // Just hovering around
  setDefaultCursor(getApp()->getDefaultCursor(cursorType[where(event->win_x,event->win_y)]));
  return 0;
  }


// Get width
FXint FXToolBarShell::getDefaultWidth(){
  FXWindow* child=getFirst();
  FXuint hints;
  FXint w=0;
  if(child && child->shown()){
    hints=child->getLayoutHints();
    if(hints&LAYOUT_FIX_WIDTH){       // Fixed width
      w=child->getWidth();
      }
    else if(hints&LAYOUT_SIDE_LEFT){  // Vertical
      w=child->getWidthForHeight((hints&LAYOUT_FIX_HEIGHT) ? child->getHeight() : child->getDefaultHeight());
      }
    else{                             // Horizontal
      w=child->getDefaultWidth();
      }
    }
  return w+(border<<1);
  }


// Get height
FXint FXToolBarShell::getDefaultHeight(){
  FXWindow* child=getFirst();
  FXuint hints;
  FXint h=0;
  if(child && child->shown()){
    hints=child->getLayoutHints();
    if(hints&LAYOUT_FIX_HEIGHT){      // Fixed height
      h=child->getHeight();
      }
    else if(hints&LAYOUT_SIDE_LEFT){  // Vertical
      h=child->getDefaultHeight();
      }
    else{                             // Horizontal
      h=child->getHeightForWidth((hints&LAYOUT_FIX_WIDTH) ? child->getWidth() : child->getDefaultWidth());
      }
    }
  return h+(border<<1);
  }



// Recalculate layout
void FXToolBarShell::layout(){
  if(getFirst() && getFirst()->shown()){
    getFirst()->position(border,border,width-(border<<1),height-(border<<1));
    }
  flags&=~FLAG_DIRTY;
  }


// Change frame border style
void FXToolBarShell::setFrameStyle(FXuint style){
  FXuint opts=(options&~FRAME_MASK) | (style&FRAME_MASK);
  if(options!=opts){
    FXint b=(opts&FRAME_THICK) ? 2 : (opts&(FRAME_SUNKEN|FRAME_RAISED)) ? 1 : 0;
    options=opts;
    if(border!=b){
      border=b;
      recalc();
      }
    update();
    }
  }


// Get frame style
FXuint FXToolBarShell::getFrameStyle() const {
  return (options&FRAME_MASK);
  }


// Set base color
void FXToolBarShell::setBaseColor(FXColor clr){
  if(clr!=baseColor){
    baseColor=clr;
    update();
    }
  }


// Set highlight color
void FXToolBarShell::setHiliteColor(FXColor clr){
  if(clr!=hiliteColor){
    hiliteColor=clr;
    update();
    }
  }


// Set shadow color
void FXToolBarShell::setShadowColor(FXColor clr){
  if(clr!=shadowColor){
    shadowColor=clr;
    update();
    }
  }


// Set border color
void FXToolBarShell::setBorderColor(FXColor clr){
  if(clr!=borderColor){
    borderColor=clr;
    update();
    }
  }


// Save data
void FXToolBarShell::save(FXStream& store) const {
  FXTopWindow::save(store);
  store << baseColor;
  store << hiliteColor;
  store << shadowColor;
  store << borderColor;
  store << border;
  }


// Load data
void FXToolBarShell::load(FXStream& store){
  FXTopWindow::load(store);
  store >> baseColor;
  store >> hiliteColor;
  store >> shadowColor;
  store >> borderColor;
  store >> border;
  }

}
