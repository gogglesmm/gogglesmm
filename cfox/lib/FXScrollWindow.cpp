/********************************************************************************
*                                                                               *
*                     S c r o l l W i n d o w   W i d g e t                     *
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
#include "fxkeys.h"
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
#include "FXScrollWindow.h"


/*
  Notes:
  - Intercepts pagedn/pageup to scroll.
  - We're assuming you're not using LAYOUT_FIX_WIDTH and LAYOUT_FILL_X
    at the same time...
  - Note that content window's position is not necessarily the same as
    the scroll position pos_x and pos_y.
*/

using namespace FX;


/*******************************************************************************/

namespace FX {

// Map
FXDEFMAP(FXScrollWindow) FXScrollWindowMap[]={
  FXMAPFUNC(SEL_KEYPRESS,0,FXScrollWindow::onKeyPress),
  FXMAPFUNC(SEL_KEYRELEASE,0,FXScrollWindow::onKeyRelease),
  FXMAPFUNC(SEL_FOCUS_SELF,0,FXScrollWindow::onFocusSelf),
  };


// Object implementation
FXIMPLEMENT(FXScrollWindow,FXScrollArea,FXScrollWindowMap,ARRAYNUMBER(FXScrollWindowMap))



// Construct and init
FXScrollWindow::FXScrollWindow(FXComposite* p,FXuint opts,FXint x,FXint y,FXint w,FXint h):FXScrollArea(p,opts,x,y,w,h){
  }


// Get content window; may be NULL
FXWindow* FXScrollWindow::contentWindow() const {
  return corner->getNext();
  }


// Determine content width of scroll area
FXint FXScrollWindow::getContentWidth(){
  FXuint hints;
  FXint w=1;
  if(contentWindow()){
    hints=contentWindow()->getLayoutHints();
    if(hints&LAYOUT_FIX_WIDTH) w=contentWindow()->getWidth();
    else w=contentWindow()->getDefaultWidth();
    }
  return w;
  }


// Determine content height of scroll area
FXint FXScrollWindow::getContentHeight(){
  FXuint hints;
  FXint h=1;
  if(contentWindow()){
    hints=contentWindow()->getLayoutHints();
    if(hints&LAYOUT_FIX_HEIGHT) h=contentWindow()->getHeight();
    else h=contentWindow()->getDefaultHeight();
    }
  return h;
  }


// Move contents; moves child window
void FXScrollWindow::moveContents(FXint x,FXint y){
  FXWindow* contents=contentWindow();
  FXint xx,yy,ww,hh,vw,vh;
  FXuint hints;
  if(contents){

    // Get hints
    hints=contents->getLayoutHints();

    // Get content size
    ww=getContentWidth();
    hh=getContentHeight();

    // Get visible size
    vw=getVisibleWidth();
    vh=getVisibleHeight();

    // Determine x-position
    xx=x;
    if(ww<vw){
      if(hints&LAYOUT_FILL_X) ww=vw;
      if(hints&LAYOUT_CENTER_X) xx=(vw-ww)/2;
      else if(hints&LAYOUT_RIGHT) xx=vw-ww;
      else xx=0;
      }

    // Determine y-position
    yy=y;
    if(hh<vh){
      if(hints&LAYOUT_FILL_Y) hh=vh;
      if(hints&LAYOUT_CENTER_Y) yy=(vh-hh)/2;
      else if(hints&LAYOUT_BOTTOM) yy=vh-hh;
      else yy=0;
      }
    contents->move(xx,yy);
    }
  pos_x=x;
  pos_y=y;
  }


// Recalculate layout
void FXScrollWindow::layout(){
  FXWindow* contents=contentWindow();
  FXint xx,yy,ww,hh,vw,vh;
  FXuint hints;

  // Layout scroll bars and viewport
  FXScrollArea::layout();

  // Set line size something reasonable
  horizontal->setLine(10);
  vertical->setLine(10);

  // Resize contents
  if(contents){

    // Get hints
    hints=contents->getLayoutHints();

    // Get content size
    ww=getContentWidth();
    hh=getContentHeight();

    // Get visible size
    vw=getVisibleWidth();
    vh=getVisibleHeight();

    // Determine x-position
    xx=pos_x;
    if(ww<vw){
      if(hints&LAYOUT_FILL_X) ww=vw;
      if(hints&LAYOUT_CENTER_X) xx=(vw-ww)/2;
      else if(hints&LAYOUT_RIGHT) xx=vw-ww;
      else xx=0;
      }

    // Determine y-position
    yy=pos_y;
    if(hh<vh){
      if(hints&LAYOUT_FILL_Y) hh=vh;
      if(hints&LAYOUT_CENTER_Y) yy=(vh-hh)/2;
      else if(hints&LAYOUT_BOTTOM) yy=vh-hh;
      else yy=0;
      }

    // Reposition content window
    contents->position(xx,yy,ww,hh);

    // Make sure its under the scroll bars
    contents->lower();
    }
  flags&=~FLAG_DIRTY;
  }


// Focus on widget itself and try put focus on the content window as well
long FXScrollWindow::onFocusSelf(FXObject* sender,FXSelector,void* ptr){        // See FXMDIChild
  setFocus();
  if(contentWindow()) contentWindow()->handle(sender,FXSEL(SEL_FOCUS_SELF,0),ptr);
  return 1;
  }


// Keyboard press
long FXScrollWindow::onKeyPress(FXObject* sender,FXSelector sel,void* ptr){
  if(FXScrollArea::onKeyPress(sender,sel,ptr)) return 1;
  switch(((FXEvent*)ptr)->code){
    case KEY_Up:
      setPosition(pos_x,pos_y+verticalScrollBar()->getLine());
      return 1;
    case KEY_Down:
      setPosition(pos_x,pos_y-verticalScrollBar()->getLine());
      return 1;
    case KEY_Page_Up:
    case KEY_KP_Page_Up:
      setPosition(pos_x,pos_y+verticalScrollBar()->getPage());
      return 1;
    case KEY_Page_Down:
    case KEY_KP_Page_Down:
      setPosition(pos_x,pos_y-verticalScrollBar()->getPage());
      return 1;
    }
  return 0;
  }


// Keyboard release
long FXScrollWindow::onKeyRelease(FXObject* sender,FXSelector sel,void* ptr){
  if(FXScrollArea::onKeyRelease(sender,sel,ptr)) return 1;
  switch(((FXEvent*)ptr)->code){
    case KEY_Up:
    case KEY_Down:
    case KEY_Page_Up:
    case KEY_KP_Page_Up:
    case KEY_Page_Down:
    case KEY_KP_Page_Down:
      return 1;
    }
  return 0;
  }

}
