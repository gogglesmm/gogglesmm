/********************************************************************************
*                                                                               *
*                         D o c k S i t e   W i d g e t                         *
*                                                                               *
*********************************************************************************
* Copyright (C) 2004,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "FXGIFIcon.h"
#include "FXFrame.h"
#include "FXComposite.h"
#include "FXPacker.h"
#include "FXPopup.h"
#include "FXMenuPane.h"
#include "FXMenuCaption.h"
#include "FXMenuCommand.h"
#include "FXMenuCascade.h"
#include "FXMenuSeparator.h"
#include "FXMenuRadio.h"
#include "FXMenuCheck.h"
#include "FXShell.h"
#include "FXSeparator.h"
#include "FXTopWindow.h"
#include "FXDockBar.h"
#include "FXDockSite.h"
#include "FXToolBarGrip.h"
#include "FXToolBarShell.h"
#include "icons.h"


/*
  Notes:


                       Jeroen's Theory of Docking

  Because if I don't write this down, no one will *ever* figure this out!

  We assume here we're docking a horizontally oriented bar inside a horizontally
  oriented dock.  Vertical orientation works, of course, completely analoguous.

  Docking a floating bar.  When dragging a floating bar, we call findDockNear()
  to determine which dock site this bar could dock into.  This dock site is a
  sibling of the current widget remembered in the drydock variable.  The routine
  findDockNear() invokes insideDock() to determine if a docking could take place
  or not.

  Not all dock sites are available for docking.  Some dock bars have a preferred
  orientation and can not be docked everywhere.  To this end, the dock bar
  maintains a member variable allowed which contains the set of sides that the
  bar is allowed to dock at.

  When findDockNear() returns a non-NULL value, a potential dock site has been
  found; a timer is set to dock at this site if the cursor hovers near its
  current position:- we don't dock immediately, since the user may be just moving
  the bar across a potential dock site on his way to another place on the screen.

  When the dragging ends, or when the timer expires, the bar is docked at the
  dock site.  Subsequent dragging will simply move the bar inside the dock site,
  and just rearrange the dock site.

  Floating a docked bar.  When moving a docked bar, dragging it simply rearranges
  the order of the bars inside the dock, until such time that the bar is substantially
  moved away from the dock site, as determined by insideDock().  Then the dock bar
  is reparented under the toolbar shell and subsequent movement will simply move
  the floaring bar.

  The function insideDock() determines if a proposed position is to be considered
  docked or non-docked.  This is determined differently, based on whether the bar
  is currently docked or not:

    1 If the bar is docked, we consider the proposed bar position undocked when
      the upper edge is FUDGE pixels above the dock site, or when the lower edge
      is FUDGE pixels below the dock site.

    2 Alternatively, if the bar is floating, we consider the bar position docked when
      the upper or lower edge of the bar is within PROXIMITY inside the dock site.

    3 In addition to 1 and 2, we also require that the horizontal alignment of the
      dock bar and dock site are within a certain TOLERANCE. This is determined
      differently depending on whether the bar is wider than the site or not:

        a If the bar is wider, we want the dock site within -TOLERANCE to
          bar width + TOLERANCE relative to the bar.

        b If the dock site is wider, we want the bar to be within -TOLERANCE to
          docksite width + TOLERANCE relative to the site.

        c A minor wrinkle is that if the bar is stretched (LAYOUT_FILL_X), we
          don't use the current width of the bar in the above calculations,
          but the default width instead; this is because a subsequent undocking,
          which would shrink-wrap around the dock bar, leaves a smaller widget
          and this may then immediately redock again.

    4 If the above tests indicate that the bar is docked, insideDock() returns
      true.

   When undocking a bar, the arrangement inside the dock site needs to stay the same
   for the remaining bars [if any].  Thus, we call undockToolBar() to inform the dock
   site that one of its bars will be removed.  Depending on its own internal layout
   algorithm, the dock site will then be able to adjust the options of the remaining
   bars so that they stay in place.

   When docking a bar, we need to figure out where the new bar goes.  To that end,
   dockToolBar() informs the dock site to determine where the new bar goes and adjust
   the layout options of the other dock bars accordingly.

   If the dock bar is moved, the dock site is informed by moveToolBar() which lets
   the dock site then rearrange the layouts according to the new bar position.

*/


#define HANDLESIZE       6              // Resize handle length
#define FUDGE            30             // Vertical distance beyond which bar pulls out
#define PROXIMITY        10             // Vertical proximity below which bar is sucked int dock
#define TOLERANCE        30             // Horizontal alignment tolerance beyond which bar pulls out
#define DOCKINGSNAPDELAY 300000000      // Delay before dock bar snaps to dock site

// Docking side
#define LAYOUT_SIDE_MASK (LAYOUT_SIDE_LEFT|LAYOUT_SIDE_RIGHT|LAYOUT_SIDE_TOP|LAYOUT_SIDE_BOTTOM)

using namespace FX;

/*******************************************************************************/

namespace FX {

// Map
FXDEFMAP(FXDockBar) FXDockBarMap[]={
//  FXMAPFUNC(SEL_MOTION,0,FXDockBar::onMotion),
//  FXMAPFUNC(SEL_ENTER,0,FXDockBar::onEnter),
//  FXMAPFUNC(SEL_LEAVE,0,FXDockBar::onLeave),
  FXMAPFUNC(SEL_FOCUS_PREV,0,FXDockBar::onFocusLeft),
  FXMAPFUNC(SEL_FOCUS_NEXT,0,FXDockBar::onFocusRight),
//  FXMAPFUNC(SEL_LEFTBUTTONPRESS,0,FXDockBar::onLeftBtnPress),
//  FXMAPFUNC(SEL_LEFTBUTTONRELEASE,0,FXDockBar::onLeftBtnRelease),
  FXMAPFUNC(SEL_UPDATE,FXDockBar::ID_DOCK_FLOAT,FXDockBar::onUpdUndock),
  FXMAPFUNC(SEL_UPDATE,FXDockBar::ID_DOCK_TOP,FXDockBar::onUpdDockTop),
  FXMAPFUNC(SEL_UPDATE,FXDockBar::ID_DOCK_BOTTOM,FXDockBar::onUpdDockBottom),
  FXMAPFUNC(SEL_UPDATE,FXDockBar::ID_DOCK_LEFT,FXDockBar::onUpdDockLeft),
  FXMAPFUNC(SEL_UPDATE,FXDockBar::ID_DOCK_RIGHT,FXDockBar::onUpdDockRight),
  FXMAPFUNC(SEL_UPDATE,FXDockBar::ID_DOCK_FLIP,FXDockBar::onUpdDockFlip),
  FXMAPFUNC(SEL_COMMAND,FXDockBar::ID_DOCK_FLOAT,FXDockBar::onCmdUndock),
  FXMAPFUNC(SEL_COMMAND,FXDockBar::ID_DOCK_TOP,FXDockBar::onCmdDockTop),
  FXMAPFUNC(SEL_COMMAND,FXDockBar::ID_DOCK_BOTTOM,FXDockBar::onCmdDockBottom),
  FXMAPFUNC(SEL_COMMAND,FXDockBar::ID_DOCK_LEFT,FXDockBar::onCmdDockLeft),
  FXMAPFUNC(SEL_COMMAND,FXDockBar::ID_DOCK_RIGHT,FXDockBar::onCmdDockRight),
  FXMAPFUNC(SEL_BEGINDRAG,FXDockBar::ID_TOOLBARGRIP,FXDockBar::onBeginDragGrip),
  FXMAPFUNC(SEL_ENDDRAG,FXDockBar::ID_TOOLBARGRIP,FXDockBar::onEndDragGrip),
  FXMAPFUNC(SEL_DRAGGED,FXDockBar::ID_TOOLBARGRIP,FXDockBar::onDraggedGrip),
  FXMAPFUNC(SEL_TIMEOUT,FXDockBar::ID_TIMER,FXDockBar::onDockTimer),
  FXMAPFUNC(SEL_RIGHTBUTTONRELEASE,FXDockBar::ID_TOOLBARGRIP,FXDockBar::onPopupMenu),
  };


// Object implementation
FXIMPLEMENT(FXDockBar,FXPacker,FXDockBarMap,ARRAYNUMBER(FXDockBarMap))


/*
// Cursor shape based on mode
const FXDefaultCursor FXDockBar::cursorType[16]={
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
  DEF_ARROW_CURSOR,
  };
*/


// Deserialization
FXDockBar::FXDockBar():drydock(nullptr),wetdock(nullptr){
  flags|=FLAG_ENABLED;
  gripx=0;
  gripy=0;
  allowed=ALLOW_EVERYWHERE;
//  mode=DRAG_NONE;
  }


// Make a dockable and, possibly, floatable toolbar
FXDockBar::FXDockBar(FXComposite* p,FXComposite* q,FXuint opts,FXint x,FXint y,FXint w,FXint h,FXint pl,FXint pr,FXint pt,FXint pb,FXint hs,FXint vs):FXPacker(p,opts,x,y,w,h,pl,pr,pt,pb,hs,vs),drydock(p),wetdock(q){
  flags|=FLAG_ENABLED;
  gripx=0;
  gripy=0;
  allowed=ALLOW_EVERYWHERE;
//  mode=DRAG_NONE;
  }


// Make a non-floatable toolbar
FXDockBar::FXDockBar(FXComposite* p,FXuint opts,FXint x,FXint y,FXint w,FXint h,FXint pl,FXint pr,FXint pt,FXint pb,FXint hs,FXint vs):FXPacker(p,opts,x,y,w,h,pl,pr,pt,pb,hs,vs),drydock(nullptr),wetdock(nullptr){
  flags|=FLAG_ENABLED;
  gripx=0;
  gripy=0;
  allowed=ALLOW_EVERYWHERE;
//  mode=DRAG_NONE;
  }


// Return true if toolbar is docked
FXbool FXDockBar::isDocked() const {
  return (getParent()!=wetdock);
  }


// Set parent when docked, if it was docked it will remain docked
void FXDockBar::setDryDock(FXComposite* dry){
  if(dry && dry->id() && getParent()==drydock){
    reparent(dry,nullptr);
    }
  drydock=dry;
  }


// Set parent when floating
void FXDockBar::setWetDock(FXComposite* wet){
  if(wet && wet->id() && getParent()==wetdock){
    reparent(wet,nullptr);
    }
  wetdock=wet;
  }


// Dock the bar before other window
void FXDockBar::dock(FXDockSite* docksite,FXWindow* other,FXbool notify){
  if(docksite && getParent()!=docksite){
    setDryDock(docksite);
    reparent(docksite,other);
    if(wetdock) wetdock->hide();
    docksite->dockToolBar(this,other);
    if(notify && target){target->tryHandle(this,FXSEL(SEL_DOCKED,message),docksite);}
    }
  }


// Dock the bar near position in dock site
void FXDockBar::dock(FXDockSite* docksite,FXint localx,FXint localy,FXbool notify){
  if(docksite && getParent()!=docksite){
    setDryDock(docksite);
    reparent(docksite,nullptr);
    if(wetdock) wetdock->hide();
    docksite->dockToolBar(this,localx,localy);
    if(notify && target){target->tryHandle(this,FXSEL(SEL_DOCKED,message),docksite);}
    }
  }


// Undock the bar
void FXDockBar::undock(FXint rootx,FXint rooty,FXbool notify){
  FXDockSite* docksite=dynamic_cast<FXDockSite*>(getParent());
  if(wetdock && isDocked()){
    if(docksite) docksite->undockToolBar(this);
    reparent(wetdock);
    wetdock->position(rootx,rooty,wetdock->getDefaultWidth(),wetdock->getDefaultHeight());
    wetdock->show();
    if(notify && target){target->tryHandle(this,FXSEL(SEL_FLOATED,message),docksite);}
    }
  }


// Return true if layout side is allowable
FXbool FXDockBar::isAllowable(FXuint hints) const {
  if(hints&LAYOUT_SIDE_LEFT){
    if(hints&LAYOUT_SIDE_BOTTOM){       // Right
      if(allowed&ALLOW_RIGHT) return true;
      }
    else{                               // Left
      if(allowed&ALLOW_LEFT) return true;
      }
    }
  else{
    if(hints&LAYOUT_SIDE_BOTTOM){       // Bottom
      if(allowed&ALLOW_BOTTOM) return true;
      }
    else{                               // Top
      if(allowed&ALLOW_TOP) return true;
      }
    }
  return false;
  }


// Search siblings of drydock for first dock opportunity
FXDockSite* FXDockBar::findDockAtSide(FXuint side){
  FXDockSite* docksite;
  FXWindow *child;
  if(drydock){
    child=drydock->getParent()->getFirst();
    while(child){
      docksite=dynamic_cast<FXDockSite*>(child);
      if(docksite && docksite->shown() && side==(docksite->getLayoutHints()&LAYOUT_SIDE_MASK)){
        if(isAllowable(docksite->getLayoutHints())) return docksite;
        }
      child=child->getNext();
      }
    }
  return nullptr;
  }


// Test if bar is inside docksite
FXbool FXDockBar::insideDock(FXDockSite* docksite,FXint barx,FXint bary){
  if(docksite){

    // Bar size
    FXint barw=getWidth();
    FXint barh=getHeight();

    // Vertically oriented dock
    if(docksite->getLayoutHints()&LAYOUT_SIDE_LEFT){

      // If docked, undock when left or right edge pulls out beyond FUDGE pixels from dock; when floating, dock when left or right edge moves within PROXIMITY of dock
      if(((getParent()==docksite) && (docksite->getX()-FUDGE<=barx && barx+barw<docksite->getX()+docksite->getWidth()+FUDGE)) || ((getParent()!=docksite) && ((docksite->getX()-PROXIMITY<=barx && barx<docksite->getX()+docksite->getWidth()+PROXIMITY) || (docksite->getX()-PROXIMITY<=barx+barw && barx+barw<=docksite->getX()+docksite->getWidth()+PROXIMITY)))){

        // If filled, fudge the height
        if(getLayoutHints()&LAYOUT_FILL_Y) barh=getDefaultHeight();

        // Test if either bar or dock "sticks out" too much to dock
        if(barh>docksite->getHeight()){
          if(bary-TOLERANCE<=docksite->getY() && docksite->getY()+docksite->getHeight()<=bary+barh+TOLERANCE) return true;
          }
        else{
          if(docksite->getY()-TOLERANCE<=bary && bary+barh<=docksite->getY()+docksite->getHeight()+TOLERANCE) return true;
          }
        }
      }

    // Horizontally oriented dock
    else{

      // If docked, undock when upper or lower edge pulls out beyond FUDGE pixels from dock; when floating, dock when upper or lower edge moves within PROXIMITY of dock
      if(((getParent()==docksite) && (docksite->getY()-FUDGE<=bary && bary+barh<=docksite->getY()+docksite->getHeight()+FUDGE)) || ((getParent()!=docksite) && ((docksite->getY()-PROXIMITY<=bary && bary<=docksite->getY()+docksite->getHeight()+PROXIMITY) || (docksite->getY()-PROXIMITY<=bary+barh && bary+barh<=docksite->getY()+docksite->getHeight()+PROXIMITY)))){

        // If filled, fudge the width
        if(getLayoutHints()&LAYOUT_FILL_X) barw=getDefaultWidth();

        // Test if either bar or dock "sticks out" too much to dock
        if(barw>docksite->getWidth()){
          if(barx-TOLERANCE<=docksite->getX() && docksite->getX()+docksite->getWidth()<=barx+barw+TOLERANCE) return true;
          }
        else{
          if(docksite->getX()-TOLERANCE<=barx && barx+barw<=docksite->getX()+docksite->getWidth()+TOLERANCE) return true;
          }
        }
      }
    }
  return false;
  }


// Search siblings of drydock for dock opportunity near given coordinates
FXDockSite* FXDockBar::findDockNear(FXint rootx,FXint rooty){
  FXDockSite *docksite;
  FXWindow *child;
  FXint barx,bary;
  if(drydock){

    // Translate without pain; assumes position of the top window is correct
    for(child=drydock->getParent(),barx=rootx,bary=rooty; child!=getRoot(); child=child->getParent()){
      barx-=child->getX();
      bary-=child->getY();
      }

    // Localize dock site
    child=drydock->getParent()->getFirst();
    while(child){
      docksite=dynamic_cast<FXDockSite*>(child);
      if(docksite && docksite->shown() && insideDock(docksite,barx,bary)){
        if(isAllowable(docksite->getLayoutHints())) return docksite;
        }
      child=child->getNext();
      }
    }
  return nullptr;
  }


// Undock
long FXDockBar::onCmdUndock(FXObject*,FXSelector,void*){
  FXint rootx,rooty;
  translateCoordinatesTo(rootx,rooty,getRoot(),8,8);
  undock(rootx,rooty,true);
  return 1;
  }


// Check if undocked
long FXDockBar::onUpdUndock(FXObject* sender,FXSelector,void*){
  sender->handle(this,(wetdock && wetdock!=getParent())?FXSEL(SEL_COMMAND,ID_ENABLE):FXSEL(SEL_COMMAND,ID_DISABLE),nullptr);
  return 1;
  }


// Redock on top
long FXDockBar::onCmdDockTop(FXObject*,FXSelector,void*){
  dock(findDockAtSide(LAYOUT_SIDE_TOP),nullptr,true);
  return 1;
  }


// Check if docked at top
long FXDockBar::onUpdDockTop(FXObject* sender,FXSelector,void*){
  FXDockSite* docksite=findDockAtSide(LAYOUT_SIDE_TOP);
  sender->handle(this,(docksite && docksite!=getParent())?FXSEL(SEL_COMMAND,ID_ENABLE):FXSEL(SEL_COMMAND,ID_DISABLE),nullptr);
  return 1;
  }


// Redock on bottom
long FXDockBar::onCmdDockBottom(FXObject*,FXSelector,void*){
  dock(findDockAtSide(LAYOUT_SIDE_BOTTOM),nullptr,true);
  return 1;
  }


// Check if docked at bottom
long FXDockBar::onUpdDockBottom(FXObject* sender,FXSelector,void*){
  FXDockSite* docksite=findDockAtSide(LAYOUT_SIDE_BOTTOM);
  sender->handle(this,(docksite && docksite!=getParent())?FXSEL(SEL_COMMAND,ID_ENABLE):FXSEL(SEL_COMMAND,ID_DISABLE),nullptr);
  return 1;
  }


// Redock on left
long FXDockBar::onCmdDockLeft(FXObject*,FXSelector,void*){
  dock(findDockAtSide(LAYOUT_SIDE_LEFT),nullptr,true);
  return 1;
  }


// Check if docked at left
long FXDockBar::onUpdDockLeft(FXObject* sender,FXSelector,void*){
  FXDockSite* docksite=findDockAtSide(LAYOUT_SIDE_LEFT);
  sender->handle(this,(docksite && docksite!=getParent())?FXSEL(SEL_COMMAND,ID_ENABLE):FXSEL(SEL_COMMAND,ID_DISABLE),nullptr);
  return 1;
  }


// Redock on right
long FXDockBar::onCmdDockRight(FXObject*,FXSelector,void*){
  dock(findDockAtSide(LAYOUT_SIDE_RIGHT),nullptr,true);
  return 1;
  }


// Check if docked at right
long FXDockBar::onUpdDockRight(FXObject* sender,FXSelector,void*){
  FXDockSite* docksite=findDockAtSide(LAYOUT_SIDE_RIGHT);
  sender->handle(this,(docksite && docksite!=getParent())?FXSEL(SEL_COMMAND,ID_ENABLE):FXSEL(SEL_COMMAND,ID_DISABLE),nullptr);
  return 1;
  }


// Check for flip
long FXDockBar::onUpdDockFlip(FXObject* sender,FXSelector,void*){
  sender->handle(this,FXSEL(SEL_COMMAND,ID_DISABLE),nullptr);
  return 1;
  }


// Right clicked on bar
long FXDockBar::onPopupMenu(FXObject*,FXSelector,void* ptr){
  FXEvent* event=static_cast<FXEvent*>(ptr);
  if(event->moved) return 1;
  FXMenuPane dockmenu(this);
  FXGIFIcon dockflipicon(getApp(),dockflip);
  FXGIFIcon docktopicon(getApp(),docktop,FXRGB(255,255,255),IMAGE_ALPHACOLOR);
  FXGIFIcon dockbottomicon(getApp(),dockbottom,FXRGB(255,255,255),IMAGE_ALPHACOLOR);
  FXGIFIcon docklefticon(getApp(),dockleft,FXRGB(255,255,255),IMAGE_ALPHACOLOR);
  FXGIFIcon dockrighticon(getApp(),dockright,FXRGB(255,255,255),IMAGE_ALPHACOLOR);
  FXGIFIcon dockfreeicon(getApp(),dockfree,FXRGB(255,255,255),IMAGE_ALPHACOLOR);
  new FXMenuCaption(&dockmenu,tr("Docking"));
  new FXMenuSeparator(&dockmenu);
  new FXMenuCommand(&dockmenu,tr("Top"),&docktopicon,this,ID_DOCK_TOP);
  new FXMenuCommand(&dockmenu,tr("Bottom"),&dockbottomicon,this,ID_DOCK_BOTTOM);
  new FXMenuCommand(&dockmenu,tr("Left"),&docklefticon,this,ID_DOCK_LEFT);
  new FXMenuCommand(&dockmenu,tr("Right"),&dockrighticon,this,ID_DOCK_RIGHT);
  new FXMenuCommand(&dockmenu,tr("Float"),&dockfreeicon,this,ID_DOCK_FLOAT);
  new FXMenuCommand(&dockmenu,tr("Flip"),&dockflipicon,this,ID_DOCK_FLIP);
  dockmenu.create();
  dockmenu.popup(nullptr,event->root_x,event->root_y);
  getApp()->runModalWhileShown(&dockmenu);
  return 1;
  }


// Tool bar grip drag started; the grip widget can be at any level under this dock bar
long FXDockBar::onBeginDragGrip(FXObject* sender,FXSelector,void* ptr){
  FXWindow *grip=static_cast<FXWindow*>(sender);
  FXEvent* event=static_cast<FXEvent*>(ptr);
  if(dynamic_cast<FXDockSite*>(drydock)){
    for(gripx=event->click_x,gripy=event->click_y; grip && grip!=this; grip=grip->getParent()){
      gripx+=grip->getX();
      gripy+=grip->getY();
      }
    raise();
    return 1;
    }
  return 0;
  }


// Tool bar grip drag ended
long FXDockBar::onEndDragGrip(FXObject*,FXSelector,void* ptr){
  FXToolBarShell *toolbarshell=dynamic_cast<FXToolBarShell*>(getParent());
  FXEvent* event=static_cast<FXEvent*>(ptr);
  FXDockSite *toolbardock;
  FXint rootx,rooty,localx,localy;
  getApp()->removeTimeout(this,ID_TIMER);
  if(toolbarshell){
    if(!(event->state&CONTROLMASK)){
      rootx=event->root_x-gripx;
      rooty=event->root_y-gripy;
      toolbardock=findDockNear(rootx,rooty);
      if(toolbardock){
        translateCoordinatesTo(localx,localy,toolbardock,0,0);
        dock(toolbardock,localx,localy,true);
        }
      }
    }
  return 1;
  }


// Hovered near dock site:- dock it!
long FXDockBar::onDockTimer(FXObject*,FXSelector,void* ptr){
  FXDockSite *toolbardock=static_cast<FXDockSite*>(ptr);
  FXint localx,localy;
  translateCoordinatesTo(localx,localy,toolbardock,0,0);
  dock(toolbardock,localx,localy,true);
  return 1;
  }


// Tool bar grip dragged
long FXDockBar::onDraggedGrip(FXObject*,FXSelector,void* ptr){
  FXToolBarShell *toolbarshell=dynamic_cast<FXToolBarShell*>(getParent());
  FXDockSite *toolbardock=dynamic_cast<FXDockSite*>(getParent());
  FXEvent* event=static_cast<FXEvent*>(ptr);
  FXint rootx,rooty,dockx,docky;

  // Root position
  rootx=event->root_x-gripx;
  rooty=event->root_y-gripy;

  // Stop dock timer
  getApp()->removeTimeout(this,ID_TIMER);

  // We are docked
  if(toolbardock){

    // Get mouse position relative to dock site
    toolbardock->translateCoordinatesFrom(dockx,docky,getRoot(),rootx,rooty);

    // Move the bar around in dock site
    toolbardock->moveToolBar(this,dockx,docky);

    // Test if we pulled too far to stay inside
    if(!insideDock(toolbardock,dockx+toolbardock->getX(),docky+toolbardock->getY())){
      undock(rootx,rooty,true);
      }
    }

  // We are floating
  else if(toolbarshell){

    // We're near a dock, if we hover around we'll dock there
    if(!(event->state&CONTROLMASK)){
      toolbardock=findDockNear(rootx,rooty);
      if(toolbardock) getApp()->addTimeout(this,ID_TIMER,DOCKINGSNAPDELAY,toolbardock);
      }

    // Move around freely
    wetdock->move(rootx,rooty);
    }

  return 1;
  }


/*
// Find out where window was grabbed
FXuchar FXDockBar::where(FXint x,FXint y) const {
  FXuchar code=DRAG_NONE;
  if((0<=x && x<border+padleft) || (0<=y && y<border+padtop) || (width-padright-border<=x && x<width) || (height-padbottom-border<=y && y<height)){
    if(x<HANDLESIZE) code|=DRAG_LEFT;
    if(width-HANDLESIZE<=x) code|=DRAG_RIGHT;
    if(y<HANDLESIZE) code|=DRAG_TOP;
    if(height-HANDLESIZE<=y) code|=DRAG_BOTTOM;
    }
  return code;
  }


// Change cursor if we entered the window normally
long FXDockBar::onEnter(FXObject* sender,FXSelector sel,void* ptr){
  FXPacker::onEnter(sender,sel,ptr);
  if(((FXEvent*)ptr)->code!=CROSSINGGRAB){
    setDefaultCursor(getApp()->getDefaultCursor(cursorType[where(((FXEvent*)ptr)->win_x,((FXEvent*)ptr)->win_y)]));
    }
  return 1;
  }


// Restore cursor if we left the window normally
long FXDockBar::onLeave(FXObject* sender,FXSelector sel,void* ptr){
  FXPacker::onLeave(sender,sel,ptr);
  if(((FXEvent*)ptr)->code!=CROSSINGUNGRAB){
    setDefaultCursor(getApp()->getDefaultCursor(DEF_ARROW_CURSOR));
    }
  return 1;
  }


// Pressed LEFT button
long FXDockBar::onLeftBtnPress(FXObject*,FXSelector,void* ptr){
  FXEvent *event=(FXEvent*)ptr;
  flags&=~FLAG_TIP;
  handle(this,FXSEL(SEL_FOCUS_SELF,0),ptr);
  if(isEnabled()){
    grab();
    if(target && target->tryHandle(this,FXSEL(SEL_LEFTBUTTONPRESS,message),ptr)) return 1;
    mode=where(event->win_x,event->win_y);
    if(mode!=DRAG_NONE){
      if(mode&DRAG_TOP) gripy=event->win_y;
      else if(mode&DRAG_BOTTOM) gripy=event->win_y-height;
      if(mode&DRAG_LEFT) gripx=event->win_x;
      else if(mode&DRAG_RIGHT) gripx=event->win_x-width;
      setDragCursor(getApp()->getDefaultCursor(cursorType[mode]));
      }
    return 1;
    }
  return 0;
  }


// Released LEFT button
long FXDockBar::onLeftBtnRelease(FXObject*,FXSelector,void* ptr){
  if(isEnabled()){
    ungrab();
    setDragCursor(getApp()->getDefaultCursor(DEF_ARROW_CURSOR));
    if(target && target->tryHandle(this,FXSEL(SEL_LEFTBUTTONRELEASE,message),ptr)) return 1;
    if(mode!=DRAG_NONE){
      mode=DRAG_NONE;
      recalc();
      }
    return 1;
    }
  return 0;
  }


// Moved the mouse
long FXDockBar::onMotion(FXObject*,FXSelector,void* ptr){
  FXEvent *event=(FXEvent*)ptr;
  if(mode!=DRAG_NONE){
    FXDockSite *toolbardock=dynamic_cast<FXDockSite*>(getParent());
    FXint mousex,mousey;
    FXint x,y,w,h;

    // Translate to dock site's frame
    toolbardock->translateCoordinatesFrom(mousex,mousey,getRoot(),event->root_x-gripx,event->root_y-gripy);

    x=xpos;
    y=ypos;
    w=width;
    h=height;

    // Vertical
    if(mode&DRAG_TOP){
      y=mousey;
      h=ypos+height-mousey;
      }
    else if(mode&DRAG_BOTTOM){
      h=mousey-ypos;
      }

    // Horizontal
    if(mode&DRAG_LEFT){
      x=mousex;
      w=xpos+width-mousex;
      }
    else if(mode&DRAG_RIGHT){
      w=mousex-xpos;
      }

    // Resize and move
    toolbardock->resizeToolBar(this,x,y,w,h);
    recalc();
    return 1;
    }

  // Change cursor based on location
  setDefaultCursor(getApp()->getDefaultCursor(cursorType[where(event->win_x,event->win_y)]));
  return 0;
  }
*/


// Save data
void FXDockBar::save(FXStream& store) const {
  FXPacker::save(store);
  store << drydock;
  store << wetdock;
  store << allowed;
  }


// Load data
void FXDockBar::load(FXStream& store){
  FXPacker::load(store);
  store >> drydock;
  store >> wetdock;
  store >> allowed;
  }


// Destroy
FXDockBar::~FXDockBar(){
  getApp()->removeTimeout(this,ID_TIMER);
  drydock=(FXComposite*)-1L;
  wetdock=(FXComposite*)-1L;
  }

}
