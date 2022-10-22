/********************************************************************************
*                                                                               *
*                     P o p u p   W i n d o w   O b j e c t                     *
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
#include "FXEvent.h"
#include "FXWindow.h"
#include "FXDCWindow.h"
#include "FXApp.h"
#include "FXPopup.h"
#include "fxpriv.h"

/*
  Notes:
  - FXPopup now supports stretchable children; this is useful when popups
    are displayed & forced to other size than default, e.g. for a ComboBox!
  - LAYOUT_FIX_xxx takes precedence over PACK_UNIFORM_xxx!!
  - Perhaps the grab owner could be equal to the owner?
  - Perhaps popup should resize when recalc() has been called!
  - Need to implement Mikael Aronssen's suggestion to make it easier to
    run FXPopup in a modal loop.  This shouls also return a code, just
    like FXDialogBox does.  Thus it becomes very straighforward to run
    a popup menu just to get a single choice value back.
  - Do not assume root window is at (0,0); multi-monitor machines may
    have secondary monitor anywhere relative to primary display.
  - If any child is resized due to GUI update, and FXPopup is in
    POPUP_SHRINKWRAP mode, ID_LAYOUT causes resize() of the popup it
    self as well as layout() of its interior.  For normal shell windows
    only layout() is called.
*/


// Frame styles
#define FRAME_MASK        (FRAME_SUNKEN|FRAME_RAISED|FRAME_THICK)

using namespace FX;

/*******************************************************************************/

namespace FX {


// Map
FXDEFMAP(FXPopup) FXPopupMap[]={
  FXMAPFUNC(SEL_PAINT,0,FXPopup::onPaint),
  FXMAPFUNC(SEL_ENTER,0,FXPopup::onEnter),
  FXMAPFUNC(SEL_LEAVE,0,FXPopup::onLeave),
  FXMAPFUNC(SEL_MOTION,0,FXPopup::onMotion),
  FXMAPFUNC(SEL_MAP,0,FXPopup::onMap),
  FXMAPFUNC(SEL_FOCUS_NEXT,0,FXPopup::onDefault),
  FXMAPFUNC(SEL_FOCUS_PREV,0,FXPopup::onDefault),
  FXMAPFUNC(SEL_FOCUS_UP,0,FXPopup::onFocusUp),
  FXMAPFUNC(SEL_FOCUS_DOWN,0,FXPopup::onFocusDown),
  FXMAPFUNC(SEL_FOCUS_LEFT,0,FXPopup::onFocusLeft),
  FXMAPFUNC(SEL_FOCUS_RIGHT,0,FXPopup::onFocusRight),
  FXMAPFUNC(SEL_LEFTBUTTONPRESS,0,FXPopup::onButtonPress),
  FXMAPFUNC(SEL_LEFTBUTTONRELEASE,0,FXPopup::onButtonRelease),
  FXMAPFUNC(SEL_MIDDLEBUTTONPRESS,0,FXPopup::onButtonPress),
  FXMAPFUNC(SEL_MIDDLEBUTTONRELEASE,0,FXPopup::onButtonRelease),
  FXMAPFUNC(SEL_RIGHTBUTTONPRESS,0,FXPopup::onButtonPress),
  FXMAPFUNC(SEL_RIGHTBUTTONRELEASE,0,FXPopup::onButtonRelease),
  FXMAPFUNC(SEL_KEYPRESS,0,FXPopup::onKeyPress),
  FXMAPFUNC(SEL_KEYRELEASE,0,FXPopup::onKeyRelease),
  FXMAPFUNC(SEL_UNGRABBED,0,FXPopup::onUngrabbed),
  FXMAPFUNC(SEL_CHORE,FXPopup::ID_LAYOUT,FXPopup::onLayout),
  FXMAPFUNC(SEL_COMMAND,FXWindow::ID_UNPOST,FXPopup::onCmdUnpost),
  FXMAPFUNCS(SEL_COMMAND,FXPopup::ID_CHOICE,FXPopup::ID_CHOICE+999,FXPopup::onCmdChoice),
  };


// Object implementation
FXIMPLEMENT(FXPopup,FXShell,FXPopupMap,ARRAYNUMBER(FXPopupMap))


// Deserialization
FXPopup::FXPopup():prevActive(nullptr),nextActive(nullptr){
  }


// Transient window used for popups
FXPopup::FXPopup(FXWindow* own,FXuint opts,FXint x,FXint y,FXint w,FXint h):FXShell(own,opts,x,y,w,h),prevActive(nullptr),nextActive(nullptr){
  defaultCursor=getApp()->getDefaultCursor(DEF_RARROW_CURSOR);
  dragCursor=getApp()->getDefaultCursor(DEF_RARROW_CURSOR);
  flags|=FLAG_ENABLED;
  grabowner=nullptr;
  baseColor=getApp()->getBaseColor();
  hiliteColor=getApp()->getHiliteColor();
  shadowColor=getApp()->getShadowColor();
  borderColor=getApp()->getBorderColor();
  border=(options&FRAME_THICK)?2:(options&(FRAME_SUNKEN|FRAME_RAISED))?1:0;
  }


// Popups do override-redirect
FXbool FXPopup::doesOverrideRedirect() const {
  return true;
  }


// Popups do save-unders
FXbool FXPopup::doesSaveUnder() const {
  return true;
  }


#ifdef WIN32

const void* FXPopup::GetClass() const { return TEXT("FXPopup"); }

#endif


// Setting focus causes keyboard grab, because all keyboard
// events need to be reported to the active popup.
void FXPopup::setFocus(){
  FXShell::setFocus();
  //grabKeyboard();
  }


// Killing the focus should revert the keyboard grab to the
// previously active popup, or release the keyboard grab
void FXPopup::killFocus(){
  FXShell::killFocus();
  //if(prevActive){
  //  prevActive->setFocus();
  //  }
  //else{
  //  ungrabKeyboard();
  //  }
  }


// Get owner; if it has none, it's owned by itself
FXWindow* FXPopup::getGrabOwner(){
  return grabowner ? grabowner : this;
  }


// Get width
FXint FXPopup::getDefaultWidth(){
  FXWindow* child;
  FXint w,wmax,wcum;
  FXuint hints;
  wmax=wcum=0;
  for(child=getFirst(); child; child=child->getNext()){
    if(child->shown()){
      hints=child->getLayoutHints();
      if(hints&LAYOUT_FIX_WIDTH) w=child->getWidth();
      else w=child->getDefaultWidth();
      if(wmax<w) wmax=w;
      }
    }
  for(child=getFirst(); child; child=child->getNext()){
    if(child->shown()){
      hints=child->getLayoutHints();
      if(hints&LAYOUT_FIX_WIDTH) w=child->getWidth();
      else if(options&PACK_UNIFORM_WIDTH) w=wmax;
      else w=child->getDefaultWidth();
      wcum+=w;
      }
    }
  if(!(options&POPUP_HORIZONTAL)) wcum=wmax;
  return wcum+(border<<1);
  }


// Get height
FXint FXPopup::getDefaultHeight(){
  FXWindow* child;
  FXint h,hmax,hcum;
  FXuint hints;
  hmax=hcum=0;
  for(child=getFirst(); child; child=child->getNext()){
    if(child->shown()){
      hints=child->getLayoutHints();
      if(hints&LAYOUT_FIX_HEIGHT) h=child->getHeight();
      else h=child->getDefaultHeight();
      if(hmax<h) hmax=h;
      }
    }
  for(child=getFirst(); child; child=child->getNext()){
    if(child->shown()){
      hints=child->getLayoutHints();
      if(hints&LAYOUT_FIX_HEIGHT) h=child->getHeight();
      else if(options&PACK_UNIFORM_HEIGHT) h=hmax;
      else h=child->getDefaultHeight();
      hcum+=h;
      }
    }
  if(options&POPUP_HORIZONTAL) hcum=hmax;
  return hcum+(border<<1);
  }


// Recalculate layout
void FXPopup::layout(){
  FXWindow *child;
  FXint w,h,x,y,remain,t;
  FXuint hints;
  FXint sumexpand=0;
  FXint numexpand=0;
  FXint mw=0;
  FXint mh=0;
  FXint e=0;

  // Horizontal
  if(options&POPUP_HORIZONTAL){

    // Get maximum size if uniform packed
    if(options&PACK_UNIFORM_WIDTH) mw=maxChildWidth();

    // Space available
    remain=width-(border<<1);

    // Find number of paddable children and total space remaining
    for(child=getFirst(); child; child=child->getNext()){
      if(child->shown()){
        hints=child->getLayoutHints();
        if(hints&LAYOUT_FIX_WIDTH) w=child->getWidth();
        else if(options&PACK_UNIFORM_WIDTH) w=mw;
        else w=child->getDefaultWidth();
        FXASSERT(w>=0);
        if((hints&LAYOUT_FILL_X) && !(hints&LAYOUT_FIX_WIDTH)){
          sumexpand+=w;
          numexpand++;
          }
        else{
          remain-=w;
          }
        }
      }

    // Do the layout
    for(child=getFirst(),x=border; child; child=child->getNext()){
      if(child->shown()){
        hints=child->getLayoutHints();
        if(hints&LAYOUT_FIX_WIDTH) w=child->getWidth();
        else if(options&PACK_UNIFORM_WIDTH) w=mw;
        else w=child->getDefaultWidth();
        if((hints&LAYOUT_FILL_X) && !(hints&LAYOUT_FIX_WIDTH)){
          if(sumexpand>0){
            t=w*remain;
            FXASSERT(sumexpand>0);
            w=t/sumexpand;
            e+=t%sumexpand;
            if(e>=sumexpand){w++;e-=sumexpand;}
            }
          else{
            FXASSERT(numexpand>0);
            w=remain/numexpand;
            e+=remain%numexpand;
            if(e>=numexpand){w++;e-=numexpand;}
            }
          }
        child->position(x,border,w,height-(border<<1));
        x+=w;
        }
      }
    }

  // Vertical
  else{

    // Get maximum size if uniform packed
    if(options&PACK_UNIFORM_HEIGHT) mh=maxChildHeight();

    // Space available
    remain=height-(border<<1);

    // Find number of paddable children and total space remaining
    for(child=getFirst(); child; child=child->getNext()){
      if(child->shown()){
        hints=child->getLayoutHints();
        if(hints&LAYOUT_FIX_HEIGHT) h=child->getHeight();
        else if(options&PACK_UNIFORM_HEIGHT) h=mh;
        else h=child->getDefaultHeight();
        FXASSERT(h>=0);
        if((hints&LAYOUT_FILL_Y) && !(hints&LAYOUT_FIX_HEIGHT)){
          sumexpand+=h;
          numexpand++;
          }
        else{
          remain-=h;
          }
        }
      }

    // Do the layout
    for(child=getFirst(),y=border; child; child=child->getNext()){
      if(child->shown()){
        hints=child->getLayoutHints();
        if(hints&LAYOUT_FIX_HEIGHT) h=child->getHeight();
        else if(options&PACK_UNIFORM_HEIGHT) h=mh;
        else h=child->getDefaultHeight();
        if((hints&LAYOUT_FILL_Y) && !(hints&LAYOUT_FIX_HEIGHT)){
          if(sumexpand>0){
            t=h*remain;
            FXASSERT(sumexpand>0);
            h=t/sumexpand;
            e+=t%sumexpand;
            if(e>=sumexpand){h++;e-=sumexpand;}
            }
          else{
            FXASSERT(numexpand>0);
            h=remain/numexpand;
            e+=remain%numexpand;
            if(e>=numexpand){h++;e-=numexpand;}
            }
          }
        child->position(border,y,width-(border<<1),h);
        y+=h;
        }
      }
    }
  flags&=~FLAG_DIRTY;
  }


void FXPopup::drawBorderRectangle(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h){
  dc.setForeground(borderColor);
  dc.drawRectangle(x,y,w-1,h-1);
  }


void FXPopup::drawRaisedRectangle(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h){
  dc.setForeground(shadowColor);
  dc.fillRectangle(x,y+h-1,w,1);
  dc.fillRectangle(x+w-1,y,1,h);
  dc.setForeground(hiliteColor);
  dc.fillRectangle(x,y,w,1);
  dc.fillRectangle(x,y,1,h);
  }


void FXPopup::drawSunkenRectangle(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h){
  dc.setForeground(shadowColor);
  dc.fillRectangle(x,y,w,1);
  dc.fillRectangle(x,y,1,h);
  dc.setForeground(hiliteColor);
  dc.fillRectangle(x,y+h-1,w,1);
  dc.fillRectangle(x+w-1,y,1,h);
  }


void FXPopup::drawRidgeRectangle(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h){
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


void FXPopup::drawGrooveRectangle(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h){
  dc.setForeground(shadowColor);
  dc.fillRectangle(x,y,w,1);
  dc.fillRectangle(x,y,1,h);
  dc.fillRectangle(x+1,y+h-2,w-2,1);
  dc.fillRectangle(x+w-2,y+1,1,h-2);
  dc.setForeground(hiliteColor);
  dc.fillRectangle(x+1,y+1,w-2,1);
  dc.fillRectangle(x+1,y+1,1,h-2);
  dc.fillRectangle(x+1,y+h-1,w,1);
  dc.fillRectangle(x+w-1,y+1,1,h);
  }


void FXPopup::drawDoubleRaisedRectangle(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h){
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


void FXPopup::drawDoubleSunkenRectangle(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h){
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
void FXPopup::drawFrame(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h){
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
long FXPopup::onPaint(FXObject*,FXSelector,void* ptr){
  FXEvent *ev=(FXEvent*)ptr;
  FXDCWindow dc(this,ev);
  dc.setForeground(backColor);
  dc.fillRectangle(border,border,width-(border<<1),height-(border<<1));
  drawFrame(dc,0,0,width,height);
  return 1;
  }


// Moving focus up
long FXPopup::onFocusUp(FXObject* sender,FXSelector sel,void* ptr){
  if(!(options&POPUP_HORIZONTAL)) return FXPopup::onFocusPrev(sender,sel,ptr);
  return 0;
  }

// Moving focus down
long FXPopup::onFocusDown(FXObject* sender,FXSelector sel,void* ptr){
  if(!(options&POPUP_HORIZONTAL)) return FXPopup::onFocusNext(sender,sel,ptr);
  return 0;
  }

// Moving focus left
long FXPopup::onFocusLeft(FXObject* sender,FXSelector sel,void* ptr){
  if(options&POPUP_HORIZONTAL) return FXPopup::onFocusPrev(sender,sel,ptr);
  return 0;
  }

// Moving focus right
long FXPopup::onFocusRight(FXObject* sender,FXSelector sel,void* ptr){
  if(options&POPUP_HORIZONTAL) return FXPopup::onFocusNext(sender,sel,ptr);
  return 0;
  }

// Focus moved down; wrap back to begin if at end
long FXPopup::onFocusNext(FXObject*,FXSelector,void* ptr){
  FXWindow *child;
  if(getFocus()){
    child=getFocus()->getNext();
    while(child){
      if(child->shown()){
        if(child->handle(this,FXSEL(SEL_FOCUS_SELF,0),ptr)) return 1;
        }
      child=child->getNext();
      }
    }
  child=getFirst();
  while(child){
    if(child->shown()){
      if(child->handle(this,FXSEL(SEL_FOCUS_SELF,0),ptr)) return 1;
      }
    child=child->getNext();
    }
  return 0;
  }


// Focus moved up; wrap back to end if at begin
long FXPopup::onFocusPrev(FXObject*,FXSelector,void* ptr){
  FXWindow *child;
  if(getFocus()){
    child=getFocus()->getPrev();
    while(child){
      if(child->shown()){
        if(child->handle(this,FXSEL(SEL_FOCUS_SELF,0),ptr)) return 1;
        }
      child=child->getPrev();
      }
    }
  child=getLast();
  while(child){
    if(child->shown()){
      if(child->handle(this,FXSEL(SEL_FOCUS_SELF,0),ptr)) return 1;
      }
    child=child->getPrev();
    }
  return 0;
  }


// Moved into the popup:- tell the target
long FXPopup::onEnter(FXObject* sender,FXSelector sel,void* ptr){
  FXint px,py;
  FXShell::onEnter(sender,sel,ptr);
  translateCoordinatesTo(px,py,getParent(),((FXEvent*)ptr)->win_x,((FXEvent*)ptr)->win_y);
  if(contains(px,py) && getGrabOwner()->grabbed()) getGrabOwner()->ungrab();
  return 1;
  }


// Moved outside the popup:- tell the target
long FXPopup::onLeave(FXObject* sender,FXSelector sel,void* ptr){
  FXint px,py;
  FXShell::onLeave(sender,sel,ptr);
  translateCoordinatesTo(px,py,getParent(),((FXEvent*)ptr)->win_x,((FXEvent*)ptr)->win_y);
  if(!contains(px,py) && shown() && !getGrabOwner()->grabbed() && getGrabOwner()->shown()) getGrabOwner()->grab();
  return 1;
  }


// Moved (while outside the popup):- tell the target
long FXPopup::onMotion(FXObject*,FXSelector,void* ptr){
  FXEvent* ev=(FXEvent*)ptr;
  FXint xx,yy;
  if(contains(ev->root_x,ev->root_y)){
    if(getGrabOwner()->grabbed()) getGrabOwner()->ungrab();
    }
  else{
    getGrabOwner()->getParent()->translateCoordinatesFrom(xx,yy,getRoot(),ev->root_x,ev->root_y);
    if(!getGrabOwner()->contains(xx,yy)){
      if(!getGrabOwner()->grabbed() && getGrabOwner()->shown()) getGrabOwner()->grab();
      }
    }
  return 1;
  }


// Window may have appeared under the cursor, so ungrab if it was grabbed
long FXPopup::onMap(FXObject* sender,FXSelector sel,void* ptr){
  FXint x,y; FXuint buttons;
  FXShell::onMap(sender,sel,ptr);
  getCursorPosition(x,y,buttons);
  if(0<=x && 0<=y && x<width && y<height){
    FXTRACE((200,"under cursor\n"));
    if(getGrabOwner()->grabbed()) getGrabOwner()->ungrab();
    }
  return 1;
  }


// Perform layout; return 0 because no GUI update is needed
long FXPopup::onLayout(FXObject*,FXSelector,void*){
  if(getShrinkWrap()){
    resize(getDefaultWidth(),getDefaultHeight());
    }
  else{
    layout();
    }
  return 0;
  }


// Pressed button outside popup
long FXPopup::onButtonPress(FXObject*,FXSelector,void*){
  FXTRACE((200,"%s::onButtonPress %p\n",getClassName(),this));
  handle(this,FXSEL(SEL_COMMAND,ID_UNPOST),nullptr);
  //popdown(0);
  return 1;
  }


// Released button outside popup
long FXPopup::onButtonRelease(FXObject*,FXSelector,void* ptr){
  FXEvent* event=(FXEvent*)ptr;
  FXTRACE((200,"%s::onButtonRelease %p\n",getClassName(),this));
  if(event->moved){handle(this,FXSEL(SEL_COMMAND,ID_UNPOST),nullptr);}
  //popdown(0);
  return 1;
  }


// The widget lost the grab for some reason; unpost the menu
long FXPopup::onUngrabbed(FXObject* sender,FXSelector sel,void* ptr){
  FXShell::onUngrabbed(sender,sel,ptr);
  handle(this,FXSEL(SEL_COMMAND,ID_UNPOST),nullptr);
  return 1;
  }


// Key press; escape cancels popup
long FXPopup::onKeyPress(FXObject* sender,FXSelector sel,void* ptr){
  FXEvent* event=(FXEvent*)ptr;
  if(event->code==KEY_Escape || event->code==KEY_Cancel){
    handle(this,FXSEL(SEL_COMMAND,ID_UNPOST),nullptr);
    return 1;
    }
  return FXShell::onKeyPress(sender,sel,ptr);
  }


// Key release; escape cancels popup
long FXPopup::onKeyRelease(FXObject* sender,FXSelector sel,void* ptr){
  FXEvent* event=(FXEvent*)ptr;
  if(event->code==KEY_Escape || event->code==KEY_Cancel){
    handle(this,FXSEL(SEL_COMMAND,ID_UNPOST),nullptr);
    return 1;
    }
  return FXShell::onKeyRelease(sender,sel,ptr);
  }


// Unpost menu in case it was its own owner; otherwise
// tell the owner to do so.
long FXPopup::onCmdUnpost(FXObject*,FXSelector,void* ptr){
  FXTRACE((150,"%s::onCmdUnpost %p\n",getClassName(),this));
  if(grabowner){
    grabowner->handle(this,FXSEL(SEL_COMMAND,ID_UNPOST),ptr);
    }
  else{
    popdown();
    if(grabbed()) ungrab();
    }
  return 1;
  }


// Show popup and add to popup stack
void FXPopup::show(){
  if(!(flags&FLAG_SHOWN)){
    FXShell::show();
    prevActive=getApp()->popupWindow;
    if(prevActive) prevActive->nextActive=this;
    getApp()->popupWindow=this;
    setFocus();
    }
  }


// Hide popup and remove from popup stack
void FXPopup::hide(){
  if(flags&FLAG_SHOWN){
    FXShell::hide();
    if(getApp()->popupWindow==this) getApp()->popupWindow=prevActive;
    if(prevActive) prevActive->nextActive=nextActive;
    if(nextActive) nextActive->prevActive=prevActive;
    nextActive=nullptr;
    prevActive=nullptr;
    killFocus();
    }
  // Focus back to popup under this one, iff this was top one
  }


// Popup the menu at some location
void FXPopup::popup(FXWindow* grabto,FXint x,FXint y,FXint w,FXint h){
  FXint rx,ry,rw,rh;
#ifdef WIN32
  RECT rect;
  MONITORINFO minfo;
  HMONITOR monitor;

  rect.left=x;
  rect.right=x+w;
  rect.top=y;
  rect.bottom=y+h;

  // Get monitor info if we have this API
  monitor=MonitorFromRect(&rect,MONITOR_DEFAULTTOPRIMARY);
  if(monitor){
    memset(&minfo,0,sizeof(minfo));
    minfo.cbSize=sizeof(minfo);
    GetMonitorInfo(monitor,&minfo);
    rx=minfo.rcWork.left;
    ry=minfo.rcWork.top;
    rw=minfo.rcWork.right-minfo.rcWork.left;
    rh=minfo.rcWork.bottom-minfo.rcWork.top;
    }

  // Otherwise use the work-area
  else{
    SystemParametersInfo(SPI_GETWORKAREA,sizeof(RECT),&rect,0);
    rx=rect.left;
    ry=rect.top;
    rw=rect.right-rect.left;
    rh=rect.bottom-rect.top;
    }
#else
  rx=getRoot()->getX();
  ry=getRoot()->getY();
  rw=getRoot()->getWidth();
  rh=getRoot()->getHeight();
#endif
  FXTRACE((150,"%s::popup %p\n",getClassName(),this));
  grabowner=grabto;
  if((options&POPUP_SHRINKWRAP) || w<=1) w=getDefaultWidth();
  if((options&POPUP_SHRINKWRAP) || h<=1) h=getDefaultHeight();
  if(x+w>rx+rw) x=rx+rw-w;
  if(y+h>ry+rh) y=ry+rh-h;
  if(x<rx) x=rx;
  if(y<ry) y=ry;
  position(x,y,w,h);
  show();
  raise();
  setFocus();
  if(!grabowner) grab();
  }


// Pops down menu and its submenus
void FXPopup::popdown(){
  FXTRACE((150,"%s::popdown %p\n",getClassName(),this));
  if(!grabowner) ungrab();
  grabowner=nullptr;
  killFocus();
  hide();
  }


/*
// Popup the menu and grab to the given owner
FXint FXPopup::popup(FXint x,FXint y,FXint w,FXint h){
  FXint rw,rh;
  create();
  if((options&POPUP_SHRINKWRAP) || w<=1) w=getDefaultWidth();
  if((options&POPUP_SHRINKWRAP) || h<=1) h=getDefaultHeight();
  rw=getRoot()->getWidth();
  rh=getRoot()->getHeight();
  if(x+w>rw) x=rw-w;
  if(y+h>rh) y=rh-h;
  if(x<0) x=0;
  if(y<0) y=0;
  position(x,y,w,h);
  show();
  raise();
  return getApp()->runPopup(this);
  }


// Pop down the menu
void FXPopup::popdown(FXint value){
  getApp()->stopModal(this,value);
  hide();
  }
*/

// Close popup
long FXPopup::onCmdChoice(FXObject*,FXSelector,void*){
  //popdown(SELID(sel)-ID_CHOICE);
  return 1;
  }


// Set base color
void FXPopup::setBaseColor(FXColor clr){
  if(baseColor!=clr){
    baseColor=clr;
    update();
    }
  }


// Set highlight color
void FXPopup::setHiliteColor(FXColor clr){
  if(hiliteColor!=clr){
    hiliteColor=clr;
    update();
    }
  }


// Set shadow color
void FXPopup::setShadowColor(FXColor clr){
  if(shadowColor!=clr){
    shadowColor=clr;
    update();
    }
  }


// Set border color
void FXPopup::setBorderColor(FXColor clr){
  if(borderColor!=clr){
    borderColor=clr;
    update();
    }
  }


// Get popup orientation
FXuint FXPopup::getOrientation() const {
  return (options&POPUP_HORIZONTAL);
  }


// Set popup orientation
void FXPopup::setOrientation(FXuint orient){
  FXuint opts=((orient^options)&POPUP_HORIZONTAL)^options;
  if(options!=opts){
    options=opts;
    recalc();
    }
  }


// Return shrinkwrap mode
FXbool FXPopup::getShrinkWrap() const {
  return (options&POPUP_SHRINKWRAP)!=0;
  }


// Change shrinkwrap mode
void FXPopup::setShrinkWrap(FXbool flag){
  options^=((0-flag)^options)&POPUP_SHRINKWRAP;
  }


// Change frame border style
void FXPopup::setFrameStyle(FXuint style){
  FXuint opts=((style^options)&FRAME_MASK)^options;
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
FXuint FXPopup::getFrameStyle() const {
  return (options&FRAME_MASK);
  }


// Unlink popup from active popup stack
FXPopup::~FXPopup(){
  if(getApp()->popupWindow==this) getApp()->popupWindow=prevActive;
  if(prevActive) prevActive->nextActive=nextActive;
  if(nextActive) nextActive->prevActive=prevActive;
  prevActive=(FXPopup*)-1L;
  nextActive=(FXPopup*)-1L;
  grabowner=(FXWindow*)-1L;
  }

}
