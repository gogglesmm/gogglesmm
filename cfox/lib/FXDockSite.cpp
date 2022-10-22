/********************************************************************************
*                                                                               *
*                         D o c k   S i t e   W i d g e t                       *
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
#include "FXEvent.h"
#include "FXWindow.h"
#include "FXApp.h"
#include "FXPacker.h"
#include "FXDockBar.h"
#include "FXDockSite.h"


/*
  Notes:
  - Use "Box-Car" algorithm when sliding horizontally (vertically).
  - Vertical arrangement is very tricky; we don't insert in between
    galleys when dragging since its not so easy to use; but this remains
    a possibility for future expansion.
  - We can STILL do wrapping of individual toolbars inside the toolbar dock;
    normally we compute the width the standard way, but if this does not
    fit the available space and its the first widget on the galley, we
    can call getHeightForWidth() and thereby wrap the item in on the
    galley.  Thus we have both wrapping of the toobars as well as
    wrapping inside the toolbar.
  - FIXME we should look at LAYOUT_DOCK_NEXT before shown() because
    if you hide a bar, we want to keep stuff in the same place.
  - Another nice addition would be to constrain docking from adding
    extra galleys, except when unavoidable.
*/

#define FUDGE 20        // Amount to move down/up before jumping into next galley

using namespace FX;

/*******************************************************************************/

namespace FX {

// Map
FXDEFMAP(FXDockSite) FXDockSiteMap[]={
//  FXMAPFUNC(SEL_MOTION,0,FXDockSite::onMotion),
//  FXMAPFUNC(SEL_ENTER,0,FXDockSite::onEnter),
//  FXMAPFUNC(SEL_LEAVE,0,FXDockSite::onLeave),
//  FXMAPFUNC(SEL_LEFTBUTTONPRESS,0,FXDockSite::onLeftBtnPress),
//  FXMAPFUNC(SEL_LEFTBUTTONRELEASE,0,FXDockSite::onLeftBtnRelease),
  FXMAPFUNC(SEL_FOCUS_PREV,0,FXDockSite::onFocusLeft),
  FXMAPFUNC(SEL_FOCUS_NEXT,0,FXDockSite::onFocusRight),
  };


// Object implementation
FXIMPLEMENT(FXDockSite,FXPacker,FXDockSiteMap,ARRAYNUMBER(FXDockSiteMap))


/*
// Cursor shape based on mode
const FXDefaultCursor FXDockSite::cursorType[16]={
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

// Make a dock site
FXDockSite::FXDockSite(FXComposite* p,FXuint opts,FXint x,FXint y,FXint w,FXint h,FXint pl,FXint pr,FXint pt,FXint pb,FXint hs,FXint vs):FXPacker(p,opts,x,y,w,h,pl,pr,pt,pb,hs,vs){
//  mode=DRAG_NONE;
  }


// Change wrap option
void FXDockSite::wrapGalleys(FXbool wrap){
  if(wrap && wrapGalleys()){
    options&=~DOCKSITE_NO_WRAP;
    recalc();
    }
  else if(!wrap && !wrapGalleys()){
    options|=DOCKSITE_NO_WRAP;
    recalc();
    }
  }


// Get wrap option
FXbool FXDockSite::wrapGalleys() const {
  return (options&DOCKSITE_NO_WRAP)==0;
  }


// Compute minimum width based on child layout hints
FXint FXDockSite::getDefaultWidth(){
  FXint total=0,galw=0,w;
  FXWindow *child;
  FXuint hints;

  // Vertically oriented
  if(options&LAYOUT_SIDE_LEFT){
    for(child=getFirst(); child; child=child->getNext()){
      if(child->shown()){
        hints=child->getLayoutHints();
        w=(hints&LAYOUT_FIX_WIDTH)?child->getWidth():child->getDefaultWidth();
        if(hints&LAYOUT_DOCK_NEXT){
          if(total) total+=hspacing;
          total+=galw;
          galw=w;
          }
        else{
          if(w>galw) galw=w;
          }
        }
      }
    total+=galw;
    }

  // Horizontally oriented
  else{
    for(child=getFirst(); child; child=child->getNext()){
      if(child->shown()){
        hints=child->getLayoutHints();
        w=(hints&LAYOUT_FIX_WIDTH)?child->getWidth():child->getDefaultWidth();
        if(hints&LAYOUT_DOCK_NEXT){
          if(galw>total) total=galw;
          galw=w;
          }
        else{
          if(galw) galw+=hspacing;
          galw+=w;
          }
        }
      }
    if(galw>total) total=galw;
    }
  return padleft+padright+total+(border<<1);
  }


// Compute minimum height based on child layout hints
FXint FXDockSite::getDefaultHeight(){
  FXint total=0,galh=0,h;
  FXWindow *child;
  FXuint hints;

  // Vertically oriented
  if(options&LAYOUT_SIDE_LEFT){
    for(child=getFirst(); child; child=child->getNext()){
      if(child->shown()){
        hints=child->getLayoutHints();
        h=(hints&LAYOUT_FIX_HEIGHT)?child->getHeight():child->getDefaultHeight();
        if(hints&LAYOUT_DOCK_NEXT){
          if(galh>total) total=galh;
          galh=h;
          }
        else{
          if(galh) galh+=vspacing;
          galh+=h;
          }
        }
      }
    if(galh>total) total=galh;
    }

  // Horizontally oriented
  else{
    for(child=getFirst(); child; child=child->getNext()){
      if(child->shown()){
        hints=child->getLayoutHints();
        h=(hints&LAYOUT_FIX_HEIGHT)?child->getHeight():child->getDefaultHeight();
        if(hints&LAYOUT_DOCK_NEXT){
          if(total) total+=vspacing;
          total+=galh;
          galh=h;
          }
        else{
          if(h>galh) galh=h;
          }
        }
      }
    total+=galh;
    }
  return padtop+padbottom+total+(border<<1);
  }


// Return width for given height (vertical orientation)
FXint FXDockSite::getWidthForHeight(FXint givenheight){
  FXint total=0,galh=0,galw=0,w,h;
  FXWindow *child;
  FXuint hints;
  givenheight-=padtop+padbottom+border+border;
  for(child=getFirst(); child; child=child->getNext()){
    if(child->shown()){
      hints=child->getLayoutHints();
      w=(hints&LAYOUT_FIX_WIDTH)?child->getWidth():child->getDefaultWidth();
      h=(hints&LAYOUT_FIX_HEIGHT)?child->getHeight():child->getDefaultHeight();
      if((hints&LAYOUT_DOCK_NEXT) || (!(options&DOCKSITE_NO_WRAP) && galh+h>givenheight)){
        if(total) total+=hspacing;
        total+=galw;
        galw=w;
        galh=h+vspacing;
        }
      else{
        galh+=h+vspacing;
        if(w>galw) galw=w;
        }
      }
    }
  total+=galw;
  return padleft+padright+total+(border<<1);
  }


// Return height for given width (horizontal orientation)
FXint FXDockSite::getHeightForWidth(FXint givenwidth){
  FXint total=0,galh=0,galw=0,w,h;
  FXWindow *child;
  FXuint hints;
  givenwidth-=padleft+padright+border+border;
  for(child=getFirst(); child; child=child->getNext()){
    if(child->shown()){
      hints=child->getLayoutHints();
      w=(hints&LAYOUT_FIX_WIDTH)?child->getWidth():child->getDefaultWidth();
      h=(hints&LAYOUT_FIX_HEIGHT)?child->getHeight():child->getDefaultHeight();
      if((hints&LAYOUT_DOCK_NEXT) || (!(options&DOCKSITE_NO_WRAP) && galw+w>givenwidth)){
        if(total) total+=vspacing;
        total+=galh;
        galw=w+hspacing;
        galh=h;
        }
      else{
        galw+=w+hspacing;
        if(h>galh) galh=h;
        }
      }
    }
  total+=galh;
  return padtop+padbottom+total+(border<<1);
  }


// Find begin and end of galley containing bar on horizontal dock site
void FXDockSite::galleyOfHorzBar(FXWindow *bar,FXWindow*& begin,FXWindow*& end) const {
  FXint space=width-padleft-padright-border-border,ss=0,w;
  FXWindow *child;
  for(child=bar; child->getPrev(); child=child->getPrev()){
    if(child->shown() && (child->getLayoutHints()&LAYOUT_DOCK_NEXT)) break;
    }
  for(begin=child; child!=bar; child=child->getNext()){
    if(!child->shown()) continue;
    w=(child->getLayoutHints()&LAYOUT_FIX_WIDTH) ? child->getWidth() : child->getDefaultWidth();
    if(!(options&DOCKSITE_NO_WRAP) && ss+w>space){
      begin=child;
      ss=w+hspacing;
      }
    else{
      ss+=w+hspacing;
      }
    }
  for(end=child; child; end=child,child=child->getNext()){
    if(!child->shown()) continue;
    w=(child->getLayoutHints()&LAYOUT_FIX_WIDTH) ? child->getWidth() : child->getDefaultWidth();
    if(ss && ((child->getLayoutHints()&LAYOUT_DOCK_NEXT) || (!(options&DOCKSITE_NO_WRAP) && ss+w>space))) break;
    ss+=w+hspacing;
    }
  }


// Find begin and end of galley containing bar on vertical dock site
void FXDockSite::galleyOfVertBar(FXWindow *bar,FXWindow*& begin,FXWindow*& end) const {
  FXint space=height-padtop-padbottom-border-border,ss=0,h;
  FXWindow *child;
  for(child=bar; child->getPrev(); child=child->getPrev()){
    if(child->shown() && (child->getLayoutHints()&LAYOUT_DOCK_NEXT)) break;
    }
  for(begin=child; child!=bar; child=child->getNext()){
    if(child->shown()){
      h=(child->getLayoutHints()&LAYOUT_FIX_HEIGHT) ? child->getHeight() : child->getDefaultHeight();
      if(!(options&DOCKSITE_NO_WRAP) && ss+h>space){
        begin=child;
        ss=h+vspacing;
        }
      else{
        ss+=h+vspacing;
        }
      }
    }
  for(end=child; child; end=child,child=child->getNext()){
    if(child->shown()){
      h=(child->getLayoutHints()&LAYOUT_FIX_HEIGHT) ? child->getHeight() : child->getDefaultHeight();
      if(ss && ((child->getLayoutHints()&LAYOUT_DOCK_NEXT) || (!(options&DOCKSITE_NO_WRAP) && ss+h>space))) break;
      ss+=h+vspacing;
      }
    }
  }



// Determine vertical galley size
FXint FXDockSite::galleyWidth(FXWindow *begin,FXWindow*& end,FXint space,FXint& require,FXint& expand) const {
  FXint galley,any,w,h;
  FXWindow *child;
  FXuint hints;
  require=expand=galley=any=0;
  for(child=end=begin; child; end=child,child=child->getNext()){
    if(child->shown()){

      // Get size
      hints=child->getLayoutHints();
      w=(hints&LAYOUT_FIX_WIDTH)?child->getWidth():child->getDefaultWidth();
      h=(hints&LAYOUT_FIX_HEIGHT)?child->getHeight():child->getDefaultHeight();

      // Break for new galley?
      if(any && ((hints&LAYOUT_DOCK_NEXT) || ((require+h>space) && wrapGalleys()))) break;

      // Expanding widgets
      if(hints&LAYOUT_FILL_Y) expand+=h;

      // Figure galley size
      require+=h+vspacing;
      if(w>galley) galley=w;
      any=1;
      }
    }
  require-=vspacing;
  return galley;
  }


// Determine horizontal galley size
FXint FXDockSite::galleyHeight(FXWindow *begin,FXWindow*& end,FXint space,FXint& require,FXint& expand) const {
  FXint galley,any,w,h;
  FXWindow *child;
  FXuint hints;
  require=expand=galley=any=0;
  for(child=end=begin; child; end=child,child=child->getNext()){
    if(child->shown()){

      // Get size
      hints=child->getLayoutHints();
      w=(hints&LAYOUT_FIX_WIDTH)?child->getWidth():child->getDefaultWidth();
      h=(hints&LAYOUT_FIX_HEIGHT)?child->getHeight():child->getDefaultHeight();

      // Break for new galley?
      if(any && ((hints&LAYOUT_DOCK_NEXT) || ((require+w>space) && wrapGalleys()))) break;

      // Expanding widgets
      if(hints&LAYOUT_FILL_X) expand+=w;

      // Figure galley size
      require+=w+hspacing;
      if(h>galley) galley=h;
      any=1;
      }
    }
  require-=hspacing;
  return galley;
  }


// Recalculate layout
void FXDockSite::layout(){
  FXint expand,remain,require,left,right,top,bottom,galx,galy,galw,galh,e,t,x,y,w,h;
  FXWindow *begin,*end,*child;
  FXuint hints;

  // Vertically oriented
  if(options&LAYOUT_SIDE_LEFT){

    // Galley height
    left=border+padleft;
    right=width-padright-border;

    // Loop over galleys
    for(begin=getFirst(); begin; begin=end->getNext()){

      // Space available
      top=border+padtop;
      bottom=height-padbottom-border;

      // Galley width
      galw=galleyWidth(begin,end,bottom-top,require,expand);

      // Remaining space
      remain=bottom-top-require;
      if(expand) require=bottom-top;

      // Start next galley
      galx=left;
      left=left+galw+hspacing;

      // Placement of widgets on galley
      for(child=begin,e=0; child; child=child->getNext()){
        if(child->shown()){

          // Get size
          hints=child->getLayoutHints();
          w=(hints&LAYOUT_FIX_WIDTH)?child->getWidth():child->getDefaultWidth();
          h=(hints&LAYOUT_FIX_HEIGHT)?child->getHeight():child->getDefaultHeight();

          // X-filled
          if(hints&LAYOUT_FILL_X) w=galw;

          // Y-filled
          if(hints&LAYOUT_FILL_Y){
            t=h*remain;
            e+=t%expand;
            h+=t/expand+e/expand;
            e%=expand;
            }

          require-=h;

          // Determine child x-position
          x=child->getX();
          if(x<galx) x=galx;
          if(x+w>galx+galw) x=galx+galw-w;

          // Determine child y-position
          y=child->getY();
          if(y+h>bottom-require) y=bottom-require-h;
          if(y<top) y=top;
          top=y+h+vspacing;

          require-=vspacing;

          // Placement on this galley
          child->position(x,y,w,h);
          }
        if(child==end) break;
        }
      }
    }

  // Horizontally oriented
  else{

    // Galley height
    top=border+padtop;
    bottom=height-padbottom-border;

    // Loop over galleys
    for(begin=getFirst(); begin; begin=end->getNext()){

      // Space available
      left=border+padleft;
      right=width-padright-border;

      // Galley height
      galh=galleyHeight(begin,end,right-left,require,expand);

      // Remaining space
      remain=right-left-require;
      if(expand) require=right-left;

      // Start next galley
      galy=top;
      top=top+galh+vspacing;

      // Placement of widgets on galley
      for(child=begin,e=0; child; child=child->getNext()){
        if(child->shown()){

          // Get size
          hints=child->getLayoutHints();
          w=(hints&LAYOUT_FIX_WIDTH)?child->getWidth():child->getDefaultWidth();
          h=(hints&LAYOUT_FIX_HEIGHT)?child->getHeight():child->getDefaultHeight();

          // Y-filled
          if(hints&LAYOUT_FILL_Y) h=galh;

          // X-filled
          if(hints&LAYOUT_FILL_X){
            t=w*remain;
            e+=t%expand;
            w+=t/expand+e/expand;
            e%=expand;
            }

          require-=w;

          // Determine child y-position
          y=child->getY();
          if(y<galy) y=galy;
          if(y+h>galy+galh) y=galy+galh-h;

          // Determine child x-position
          x=child->getX();
          if(x+w>right-require) x=right-require-w;
          if(x<left) x=left;
          left=x+w+hspacing;

          require-=hspacing;

          // Placement on this galley
          child->position(x,y,w,h);
          }
        if(child==end) break;
        }
      }
    }
  flags&=~FLAG_DIRTY;
  }


/*******************************************************************************/


// Move bar vertically in galley [begin..end]
void FXDockSite::moveVerBar(FXWindow*& begin,FXWindow*& end,FXWindow* bar,FXint barx,FXint bary,FXint barw,FXint barh,FXbool hop){
  FXWindow *child; FXint pos,by;

  by=bary;

  // Pushing up
  if(bary<bar->getY()){

    // Figure minimum position
    for(child=begin,pos=border+padtop; child && child!=bar; child=child->getNext()){
      if(child->shown()){ pos+=child->getHeight()+vspacing; }
      }

    // Can't move higher
    if(bary<pos){
      if(barh==bar->getHeight()){bary=pos;}else{barh=bary+barh-pos;bary=pos;}
      }

    // Bars in front of this one?
    if(bar!=begin){
      child=bar;
      pos=bary;

      // Move bars in box-car fashion
      do{
        child=child->getPrev();
        if(!child->shown()) continue;
        pos=pos-vspacing-child->getHeight();
        if(child->getY()<=pos) break;
        child->move(child->getX(),pos);
        }
      while(child!=begin);

      // Hop bar over
      child=bar->getPrev();
      if(hop && by<child->getY()){

        // Hopping over first on galley:- transfer flag over to new first
        if((child==begin) && (child->getLayoutHints()&LAYOUT_DOCK_NEXT)){
          child->setLayoutHints(child->getLayoutHints()&~LAYOUT_DOCK_NEXT);
          bar->setLayoutHints(bar->getLayoutHints()|LAYOUT_DOCK_NEXT);
          }

        // And rearrange order of children
        bary=child->getY();
        child->move(child->getY(),bary+barh+vspacing);
        bar->reparent(this,child);

        // Update galley
        if(child==begin) begin=bar;
        if(bar==end) end=child;
        }
      }
    }

  // Pushing down
  if(bary+barh>bar->getY()+bar->getHeight()){

    // Figure maximum position
    for(child=end,pos=height-padbottom-border; child && child!=bar; child=child->getPrev()){
      if(child->shown()){ pos-=child->getHeight()+vspacing; }
      }

    // Can't move lower
    if(bary+barh>pos){
      if(barh==bar->getHeight()){bary=pos-barh;}else{barh=pos-bary;}
      }

    // Bars after this one?
    if(bar!=end){
      child=bar;
      pos=bary+barh+vspacing;

      // Move bars in box-car fashion
      do{
        child=child->getNext();
        if(!child->shown()) continue;
        if(child->getY()>=pos) break;
        child->move(child->getX(),pos);
        pos=pos+vspacing+child->getHeight();
        }
      while(child!=end);

      // Hop bar over
      child=bar->getNext();
      if(hop && by+barh>child->getY()+child->getHeight()){

        // First on galley hopped over to the right:- transfer flag to new first
        if((bar==begin) && (bar->getLayoutHints()&LAYOUT_DOCK_NEXT)){
          bar->setLayoutHints(bar->getLayoutHints()&~LAYOUT_DOCK_NEXT);
          child->setLayoutHints(child->getLayoutHints()|LAYOUT_DOCK_NEXT);
          }

        // And rearrange order of children
        bary=child->getY()+child->getHeight()-barh;
        child->move(child->getX(),bary-child->getHeight()-vspacing);
        bar->reparent(this,child->getNext());

        // Update galley
        if(child==end) end=bar;
        if(bar==begin) begin=child;
        }
      }
    }

  // Move it
  bar->position(barx,bary,barw,barh);
  }


// Move bar horizontally in galley [begin..end]
void FXDockSite::moveHorBar(FXWindow*& begin,FXWindow*& end,FXWindow* bar,FXint barx,FXint bary,FXint barw,FXint barh,FXbool hop){
  FXWindow *child; FXint pos,bx;

  bx=barx;

  // Pushing left
  if(barx<bar->getX()){

    // Figure minimum position
    for(child=begin,pos=border+padleft; child && child!=bar; child=child->getNext()){
      if(child->shown()){ pos+=child->getWidth()+hspacing; }
      }

    // Can't move leftward
    if(barx<pos){
      if(barw==bar->getWidth()){barx=pos;}else{barw=barx+barw-pos;barx=pos;}
      }

    // Bars in front of this one?
    if(bar!=begin){
      child=bar;
      pos=barx;

      // Move bars in box-car fashion
      do{
        child=child->getPrev();
        if(!child->shown()) continue;
        pos=pos-hspacing-child->getWidth();
        if(child->getX()<=pos) break;
        child->move(pos,child->getY());
        }
      while(child!=begin);

      // Hop bar over
      child=bar->getPrev();
      if(hop && bx<child->getX()){

        // Hopping over first on galley:- transfer flag over to new first
        if((child==begin) && (child->getLayoutHints()&LAYOUT_DOCK_NEXT)){
          child->setLayoutHints(child->getLayoutHints()&~LAYOUT_DOCK_NEXT);
          bar->setLayoutHints(bar->getLayoutHints()|LAYOUT_DOCK_NEXT);
          }

        // And rearrange order of children
        barx=child->getX();
        child->move(barx+barw+hspacing,child->getY());
        bar->reparent(this,child);

        // Update galley
        if(child==begin) begin=bar;
        if(bar==end) end=child;
        }
      }
    }

  // Pushing right
  if(barx+barw>bar->getX()+bar->getWidth()){

    // Figure maximum position
    for(child=end,pos=width-padright-border; child && child!=bar; child=child->getPrev()){
      if(child->shown()){ pos-=child->getWidth()+hspacing; }
      }

    // Can't move rightward
    if(barx+barw>pos){
      if(barw==bar->getWidth()){barx=pos-barw;}else{barw=pos-barx;}
      }

    // Bars after this one?
    if(bar!=end){
      child=bar;
      pos=barx+barw+hspacing;

      // Move bars in box-car fashion
      do{
        child=child->getNext();
        if(!child->shown()) continue;
        if(child->getX()>=pos) break;
        child->move(pos,child->getY());
        pos=pos+hspacing+child->getWidth();
        }
      while(child!=end);

      // Hop bar over
      child=bar->getNext();
      if(hop && bx+barw>child->getX()+child->getWidth()){

        // First on galley hopped over to the right:- transfer flag to new first
        if((bar==begin) && (bar->getLayoutHints()&LAYOUT_DOCK_NEXT)){
          bar->setLayoutHints(bar->getLayoutHints()&~LAYOUT_DOCK_NEXT);
          child->setLayoutHints(child->getLayoutHints()|LAYOUT_DOCK_NEXT);
          }

        // And rearrange order of children
        barx=child->getX()+child->getWidth()-barw;
        child->move(barx-child->getWidth()-hspacing,child->getY());
        bar->reparent(this,child->getNext());

        // Update galley
        if(child==end) end=bar;
        if(bar==begin) begin=child;
        }
      }
    }

  // Move it
  bar->position(barx,bary,barw,barh);
  }


/*******************************************************************************/


// FIXME can not move left edge if docked on right and widget NOT galley-wide yet
// Position toolbar
void FXDockSite::resizeToolBar(FXDockBar* bar,FXint barx,FXint bary,FXint barw,FXint barh){
  if(bar && bar->getParent()==this){
    FXint top,bottom,left,right,expand,require,w,h,mbw,mbh,t;
    FXWindow *begin,*end,*c;

    //FXTRACE((100,"barx=%d bary=%d barw=%d barh=%d\n",barx,bary,barw,barh));

    // Interior
    top=border+padtop;
    bottom=height-padbottom-border;
    left=border+padleft;
    right=width-padright-border;

    // Vertically oriented
    if(options&LAYOUT_SIDE_LEFT){

      // Minimum bar width
      mbw=bar->getPadLeft()+bar->getPadRight()+bar->getBorderWidth()+bar->getBorderWidth();

      // Determine galley sizes
      for(begin=end=getFirst(); begin; begin=end->getNext()){
        w=galleyWidth(begin,end,bottom-top,require,expand);

        // Found galley of the bar
        if(before(begin,bar) && before(bar,end)){

          // Dragging the right
          if(barx+barw!=bar->getX()+bar->getWidth()){
            if(barw<mbw){ barw=mbw; }
            if(options&LAYOUT_SIDE_BOTTOM){     // Docked on right
              if(barx+barw>=left+w){                    // Can not drag rightward
                barw=left+w-barx;
                }
              else{                                     // Only drag leftward if another widget holds the galley up
                for(c=begin,t=-1; c; c=c->getNext()){
                  if(c!=bar && c->shown() && (t=(c->getLayoutHints()&LAYOUT_FIX_WIDTH) ? c->getWidth() : c->getDefaultWidth())==w) break;
                  if(c==end) break;
                  }
                if(t<w){
                  barw=left+w-barx;
                  }
                }
              }
            }

          // Dragging the left
          if(barx!=bar->getX()){
            if(barw<mbw){ barx=barx+barw-mbw; barw=mbw; }
            if(!(options&LAYOUT_SIDE_BOTTOM)){  // Docked on left
              if(barx<=left){                           // Can not drag leftward
                barw=barx+barw-left; barx=left;
                }
              else{                                     // Only drag rightward if another widget holds galley up
                for(c=begin,t=-1; c; c=c->getNext()){
                  if(c!=bar && c->shown() && (t=(c->getLayoutHints()&LAYOUT_FIX_WIDTH) ? c->getWidth() : c->getDefaultWidth())==w) break;
                  if(c==end) break;
                  }
                if(t<w){
                  barw=barx+barw-left; barx=left;
                  }
                }
              }
            }

          // Move bar vertically; this may change the galley start and end!
          moveVerBar(begin,end,bar,barx,bary,barw,barh,false);
          break;
          }
        left+=w+hspacing;
        }
      }

    // Horizontally oriented
    else{

      // Minimum bar height
      mbh=bar->getPadTop()+bar->getPadBottom()+bar->getBorderWidth()+bar->getBorderWidth();

      // Determine galley sizes
      for(begin=end=getFirst(); begin; begin=end->getNext()){
        h=galleyHeight(begin,end,right-left,require,expand);

        // Found galley of the bar
        if(before(begin,bar) && before(bar,end)){

          // Same bar, move horizontally
          if(bary<top) bary=top;
          if(bary+barh>top+h) bary=top+h-barh;

          // Move bar horizontally; this may change the galley start and end!
          moveHorBar(begin,end,bar,barx,bary,barw,barh,false);
          break;
          }
        top+=h+vspacing;
        }
      }
    }
  }


/*******************************************************************************/


// Move dock bar, changing its options to suit position
void FXDockSite::moveToolBar(FXDockBar* bar,FXint barx,FXint bary){
  FXint left,right,top,bottom,galx,galy,galw,galh,dockx,docky,barw,barh,expand,require,w,h;
  FXWindow *begin,*end,*cur,*curend,*nxt,*nxtend,*prv,*prvend;

  // We insist this bar hangs under this dock site
  if(bar && bar->getParent()==this){

    // Proposed location
    dockx=barx;
    docky=bary;

    // Bar size
    barw=bar->getWidth();
    barh=bar->getHeight();

    // Interior
    top=border+padtop;
    bottom=height-padbottom-border;
    left=border+padleft;
    right=width-padright-border;

    // Vertically oriented
    if(options&LAYOUT_SIDE_LEFT){

      // Determine galley sizes
      galx=left;
      galw=0;
      for(begin=getFirst(),cur=prv=nxt=curend=prvend=nxtend=nullptr; begin; begin=end->getNext()){
        w=galleyWidth(begin,end,bottom-top,require,expand);
        if(!after(end,bar)){ if(left<=barx && barx<left+w){ prv=begin; prvend=end; } }
        else if(!after(bar,begin)){ if(left<=barx+barw && barx+barw<left+w){ nxt=begin; nxtend=end; } }
        else{ cur=begin; curend=end; galx=left; galw=w; }
        left+=w+hspacing;
        }

      // Same bar, move vertically
      if(dockx<galx) dockx=galx;
      if(dockx+barw>galx+galw) dockx=galx+galw-barw;

      // Move bar vertically; this may change the galley start and end!
      moveVerBar(cur,curend,bar,dockx,docky,barw,barh,true);

      // Moving bar right, unless we're about to pull it out of the dock
      if(barx+barw>=galx+galw+FUDGE && (!bar->getWetDock() || barx+barw<width-padright-border)){
        if(nxt){                                  // Hang at end of next galley
          if(cur==bar && bar!=curend) cur->getNext()->setLayoutHints(cur->getNext()->getLayoutHints()|LAYOUT_DOCK_NEXT);
          nxt->setLayoutHints(nxt->getLayoutHints()|LAYOUT_DOCK_NEXT);
          bar->setLayoutHints(bar->getLayoutHints()&~LAYOUT_DOCK_NEXT);
          bar->reparent(this,nxtend->getNext());
          }
        else{                                     // Hang below last
          if(cur==bar && bar!=curend) cur->getNext()->setLayoutHints(cur->getNext()->getLayoutHints()|LAYOUT_DOCK_NEXT);
          else cur->setLayoutHints(cur->getLayoutHints()|LAYOUT_DOCK_NEXT);
          bar->setLayoutHints(bar->getLayoutHints()|LAYOUT_DOCK_NEXT);
          bar->reparent(this,nullptr);
          }
        }

      // Moving bar left, unless we're about to pull it out of the dock
      else if(barx<galx-FUDGE && (!bar->getWetDock() || barx>padleft+border)){
        if(prv){                                  // Hang at end of previous galley
          if(cur==bar && bar!=curend) cur->getNext()->setLayoutHints(cur->getNext()->getLayoutHints()|LAYOUT_DOCK_NEXT);
          prv->setLayoutHints(prv->getLayoutHints()|LAYOUT_DOCK_NEXT);
          bar->setLayoutHints(bar->getLayoutHints()&~LAYOUT_DOCK_NEXT);
          bar->reparent(this,prvend->getNext());
          }
        else{                                     // Hand above first
          if(cur==bar && bar!=curend) cur->getNext()->setLayoutHints(cur->getNext()->getLayoutHints()|LAYOUT_DOCK_NEXT);
          else cur->setLayoutHints(cur->getLayoutHints()|LAYOUT_DOCK_NEXT);
          bar->setLayoutHints(bar->getLayoutHints()|LAYOUT_DOCK_NEXT);
          bar->reparent(this,getFirst());
          }
        }
#if 0
      // Determine galley sizes
      galx=left;
      galw=0;
      for(begin=getFirst(),cur=prv=nxt=curend=prvend=nxtend=nullptr; begin; begin=end->getNext()){
        w=galleyWidth(begin,end,bottom-top,require,expand);
        if(!after(end,bar)){ if(left<=barx && barx<left+w){ prv=begin; prvend=end; } }
        else if(!after(bar,begin)){ if(left<=barx+barw && barx+barw<left+w){ nxt=begin; nxtend=end; }  }
        else{ cur=begin; curend=end; galx=left; galw=w; }
        left+=w+hspacing;
        }

      // Same bar, move vertically
      if(dockx<galx) dockx=galx;
      if(dockx+barw>galx+galw) dockx=galx+galw-barw;

      // Move bar vertically; this may change the galley start and end!
      moveVerBar(cur,curend,bar,dockx,docky,barw,barh,true);

      // Moving bar right, unless we're about to pull it out of the dock
      if(barx+barw>=galx+galw+FUDGE && (!bar->getWetDock() || barx+barw<width-padright-border)){
        if(nxt){                                  // Hang at end of next galley
          if(cur==bar && bar!=curend) cur->getNext()->setLayoutHints(cur->getNext()->getLayoutHints()|LAYOUT_DOCK_NEXT);
          nxt->setLayoutHints(nxt->getLayoutHints()|LAYOUT_DOCK_NEXT);
          bar->setLayoutHints(bar->getLayoutHints()&~LAYOUT_DOCK_NEXT);
          bar->reparent(this,nxtend->getNext());
          }
        else{                                     // Hang below last
          if(cur==bar && bar!=curend) cur->getNext()->setLayoutHints(cur->getNext()->getLayoutHints()|LAYOUT_DOCK_NEXT);
          else cur->setLayoutHints(cur->getLayoutHints()|LAYOUT_DOCK_NEXT);
          bar->setLayoutHints(bar->getLayoutHints()|LAYOUT_DOCK_NEXT);
          bar->reparent(this,nullptr);
          }
        }

      // Moving bar left, unless we're about to pull it out of the dock
      else if(barx<galx-FUDGE && (!bar->getWetDock() || barx>padleft+border)){
        if(prv){                                  // Hang at end of previous galley
          if(cur==bar && bar!=curend) cur->getNext()->setLayoutHints(cur->getNext()->getLayoutHints()|LAYOUT_DOCK_NEXT);
          prv->setLayoutHints(prv->getLayoutHints()|LAYOUT_DOCK_NEXT);
          bar->setLayoutHints(bar->getLayoutHints()&~LAYOUT_DOCK_NEXT);
          bar->reparent(this,prvend->getNext());
          }
        else{                                     // Hand above first
          if(cur==bar && bar!=curend) cur->getNext()->setLayoutHints(cur->getNext()->getLayoutHints()|LAYOUT_DOCK_NEXT);
          else cur->setLayoutHints(cur->getLayoutHints()|LAYOUT_DOCK_NEXT);
          bar->setLayoutHints(bar->getLayoutHints()|LAYOUT_DOCK_NEXT);
          bar->reparent(this,getFirst());
          }
        }
#endif
      }

    // Horizontally oriented
    else{

      // Determine galley sizes
      galy=top;
      galh=0;
      for(begin=getFirst(),cur=prv=nxt=curend=prvend=nxtend=nullptr; begin; begin=end->getNext()){
        h=galleyHeight(begin,end,right-left,require,expand);
        if(!after(end,bar)){ if(top<=bary && bary<top+h){ prv=begin; prvend=end; } }
        else if(!after(bar,begin)){ if(top<=bary+barh && bary+barh<top+h){ nxt=begin; nxtend=end; }  }
        else{ cur=begin; curend=end; galy=top; galh=h; }
        top+=h+vspacing;
        }

      // Same bar, move horizontally
      if(docky<galy) docky=galy;
      if(docky+barh>galy+galh) docky=galy+galh-barh;

      // Move bar horizontally; this may change the galley start and end!
      moveHorBar(cur,curend,bar,dockx,docky,barw,barh,true);

      // Moving bar down, unless we're about to pull it out of the dock
      if(bary+barh>=galy+galh+FUDGE && (!bar->getWetDock() || bary+barh<height-padbottom-border)){
        if(nxt){                                  // Hang at end of next galley
          if(cur==bar && bar!=curend) cur->getNext()->setLayoutHints(cur->getNext()->getLayoutHints()|LAYOUT_DOCK_NEXT);
          nxt->setLayoutHints(nxt->getLayoutHints()|LAYOUT_DOCK_NEXT);
          bar->setLayoutHints(bar->getLayoutHints()&~LAYOUT_DOCK_NEXT);
          bar->reparent(this,nxtend->getNext());
          }
        else{                                     // Hang below last
          if(cur==bar && bar!=curend) cur->getNext()->setLayoutHints(cur->getNext()->getLayoutHints()|LAYOUT_DOCK_NEXT);
          else cur->setLayoutHints(cur->getLayoutHints()|LAYOUT_DOCK_NEXT);
          bar->setLayoutHints(bar->getLayoutHints()|LAYOUT_DOCK_NEXT);
          bar->reparent(this,nullptr);
          }
        }

      // Moving bar up, unless we're about to pull it out of the dock
      else if(bary<galy-FUDGE && (!bar->getWetDock() || bary>border+padtop)){
        if(prv){                                  // Hang at end of previous galley
          if(cur==bar && bar!=curend) cur->getNext()->setLayoutHints(cur->getNext()->getLayoutHints()|LAYOUT_DOCK_NEXT);
          prv->setLayoutHints(prv->getLayoutHints()|LAYOUT_DOCK_NEXT);
          bar->setLayoutHints(bar->getLayoutHints()&~LAYOUT_DOCK_NEXT);
          bar->reparent(this,prvend->getNext());
          }
        else{                                     // Hand above first
          if(cur==bar && bar!=curend) cur->getNext()->setLayoutHints(cur->getNext()->getLayoutHints()|LAYOUT_DOCK_NEXT);
          else cur->setLayoutHints(cur->getLayoutHints()|LAYOUT_DOCK_NEXT);
          bar->setLayoutHints(bar->getLayoutHints()|LAYOUT_DOCK_NEXT);
          bar->reparent(this,getFirst());
          }
        }
      }
    }
  }


// Fix layouts for undocking given bar
void FXDockSite::undockToolBar(FXDockBar* bar){
  FXWindow *begin,*end;

  // We insist this bar hangs under this dock site
  if(bar && bar->getParent()==this){

    // Vertically oriented
    if(options&LAYOUT_SIDE_LEFT){

      // Get galley of bar
      galleyOfVertBar(bar,begin,end);

      // Adjust layout options
      if(begin==bar && bar!=end)
        begin->getNext()->setLayoutHints(begin->getNext()->getLayoutHints()|LAYOUT_DOCK_NEXT);
      else
        begin->setLayoutHints(begin->getLayoutHints()|LAYOUT_DOCK_NEXT);
      }

    // Horizontally oriented
    else{

      // Get galley of bar
      galleyOfHorzBar(bar,begin,end);

      // Adjust layout options
      if(begin==bar && bar!=end)
        begin->getNext()->setLayoutHints(begin->getNext()->getLayoutHints()|LAYOUT_DOCK_NEXT);
      else
        begin->setLayoutHints(begin->getLayoutHints()|LAYOUT_DOCK_NEXT);
      }

    // Fix bar's layout hints too
    bar->setLayoutHints(bar->getLayoutHints()&~LAYOUT_DOCK_NEXT);
    }
  }


// Fix layouts for docking given bar at given position
void FXDockSite::dockToolBar(FXDockBar* bar,FXWindow* other){

  // We insist this bar hangs under this dock site
  if(bar && bar->getParent()==this){

    // New galley for bar
    bar->setLayoutHints(bar->getLayoutHints()|LAYOUT_DOCK_NEXT);
    if(other) other->setLayoutHints(bar->getNext()->getLayoutHints()|LAYOUT_DOCK_NEXT);
    }
  }


// Fix layouts for docking given bar at given position
void FXDockSite::dockToolBar(FXDockBar* bar,FXint barx,FXint bary){
  FXint left,right,top,bottom,barw,barh,expand,require,cx,cy,w,h;
  FXWindow *begin,*end,*child;

  // We insist this bar hangs under this dock site
  if(bar && bar->getParent()==this){

    // Interior
    top=border+padtop;
    left=border+padleft;
    bottom=height-padbottom-border;
    right=width-padright-border;

    // Bar size
    barw=bar->getWidth();
    barh=bar->getHeight();

    // Vertically oriented
    if(options&LAYOUT_SIDE_LEFT){

      cx=barx+barw/2;

      // Tentatively
      bar->reparent(this,getFirst());
      bar->setLayoutHints(bar->getLayoutHints()|LAYOUT_DOCK_NEXT);
      if(bar->getNext()){

        // Start galley on next
        bar->getNext()->setLayoutHints(bar->getNext()->getLayoutHints()|LAYOUT_DOCK_NEXT);

        // Right of the left edge
        if(left<=cx){

          // Determine galley
          for(begin=bar->getNext(); begin; begin=end->getNext()){
            w=galleyWidth(begin,end,bottom-top,require,expand);
            if(left<=cx && cx<left+w){

              // Find spot on galley
              for(child=begin; child!=end->getNext() && (!child->shown() || bary>=child->getY()); child=child->getNext()){}

              // At the front
              if((child==begin) && (child->getLayoutHints()&LAYOUT_DOCK_NEXT)){
                child->setLayoutHints(child->getLayoutHints()&~LAYOUT_DOCK_NEXT);
                }
              else{
                bar->setLayoutHints(bar->getLayoutHints()&~LAYOUT_DOCK_NEXT);
                }

              // hang in front
              bar->reparent(this,child);
              goto ver;
              }
            left+=w+hspacing;
            }

          // Link at the bottom
          bar->reparent(this,nullptr);
          }
        }

      // Move horizontally
ver:  bar->move(FXCLAMP(left,barx,right),bary);
      }

    // Horizontally oriented
    else{

      cy=bary+barh/2;

      // Tentatively
      bar->reparent(this,getFirst());
      bar->setLayoutHints(bar->getLayoutHints()|LAYOUT_DOCK_NEXT);
      if(bar->getNext()){

        // Start galley on next
        bar->getNext()->setLayoutHints(bar->getNext()->getLayoutHints()|LAYOUT_DOCK_NEXT);

        // Below top edge
        if(top<=cy){

          // Determine galley
          for(begin=bar->getNext(); begin; begin=end->getNext()){
            h=galleyHeight(begin,end,right-left,require,expand);
            if(top<=cy && cy<top+h){

              // Find spot on galley
              for(child=begin; child!=end->getNext() && (!child->shown() || barx>=child->getX()); child=child->getNext()){}

              // At the front
              if((child==begin) && (child->getLayoutHints()&LAYOUT_DOCK_NEXT)){
                child->setLayoutHints(child->getLayoutHints()&~LAYOUT_DOCK_NEXT);
                }
              else{
                bar->setLayoutHints(bar->getLayoutHints()&~LAYOUT_DOCK_NEXT);
                }

              // hang in front
              bar->reparent(this,child);
              goto hor;
              }
            top+=h+vspacing;
            }

          // Link at the bottom
          bar->reparent(this,nullptr);
          }
        }

      // Move horizontally
hor:  bar->move(barx,FXCLAMP(top,bary,bottom));
      }
    }
  }

/*
// Find out where window was grabbed
FXuchar FXDockSite::where(FXint x,FXint y) const {
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
long FXDockSite::onEnter(FXObject* sender,FXSelector sel,void* ptr){
  FXPacker::onEnter(sender,sel,ptr);
  if(((FXEvent*)ptr)->code!=CROSSINGGRAB){
    setDefaultCursor(getApp()->getDefaultCursor(cursorType[where(((FXEvent*)ptr)->win_x,((FXEvent*)ptr)->win_y)]));
    }
  return 1;
  }


// Restore cursor if we left the window normally
long FXDockSite::onLeave(FXObject* sender,FXSelector sel,void* ptr){
  FXPacker::onLeave(sender,sel,ptr);
  if(((FXEvent*)ptr)->code!=CROSSINGUNGRAB){
    setDefaultCursor(getApp()->getDefaultCursor(DEF_ARROW_CURSOR));
    }
  return 1;
  }


// Pressed LEFT button
long FXDockSite::onLeftBtnPress(FXObject*,FXSelector,void* ptr){
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
long FXDockSite::onLeftBtnRelease(FXObject*,FXSelector,void* ptr){
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
long FXDockSite::onMotion(FXObject*,FXSelector,void* ptr){
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

}

