/********************************************************************************
*                                                                               *
*                         S c r o l l b a r   O b j e c t s                     *
*                                                                               *
*********************************************************************************
* Copyright (C) 1997,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "FXColors.h"
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
#include "FXScrollBar.h"

/*
  Notes:
  - Should increase/decrease, and slider get messages instead?
  - If non-scrollable, but drawn anyway, don't draw thumb!
  - In case of a coarse range, we have rounding also.
  - Note that now range may be less than page; if this is the case then
    the position will be stuck at zero; position should ALWAYS be >= 0.
  - When range or page changes, call setPosition() since even though position
    may not have changed, the size of the scroll-thumb may have changed!
  - Raised WHEELJUMPTIME from 5ms to 16ms, which is around one frame-time
    on a typical 60Hz display.
  - The WHEELJUMPINCR was dropped from 16 to 8, which means a single scroll
    wheel click would complete in 128ms, or 8 frame times.
  - Minor trickiness when animating scroll: ensure we come out exactly so
    a up/down scroll followed by an equal down/up scroll lands us at the
    exact location we came from.
*/


#define SCROLLBAR_MASK  (SCROLLBAR_HORIZONTAL|SCROLLBAR_WHEELJUMP)
#define WHEELJUMPTIME   16666666
#define WHEELJUMPINCR   8

using namespace FX;

/*******************************************************************************/

namespace FX {

// Map
FXDEFMAP(FXScrollBar) FXScrollBarMap[]={
  FXMAPFUNC(SEL_PAINT,0,FXScrollBar::onPaint),
  FXMAPFUNC(SEL_MOTION,0,FXScrollBar::onMotion),
  FXMAPFUNC(SEL_MOUSEWHEEL,0,FXScrollBar::onMouseWheel),
  FXMAPFUNC(SEL_LEFTBUTTONPRESS,0,FXScrollBar::onLeftBtnPress),
  FXMAPFUNC(SEL_LEFTBUTTONRELEASE,0,FXScrollBar::onLeftBtnRelease),
  FXMAPFUNC(SEL_MIDDLEBUTTONPRESS,0,FXScrollBar::onMiddleBtnPress),
  FXMAPFUNC(SEL_MIDDLEBUTTONRELEASE,0,FXScrollBar::onMiddleBtnRelease),
  FXMAPFUNC(SEL_RIGHTBUTTONPRESS,0,FXScrollBar::onRightBtnPress),
  FXMAPFUNC(SEL_RIGHTBUTTONRELEASE,0,FXScrollBar::onRightBtnRelease),
  FXMAPFUNC(SEL_UNGRABBED,0,FXScrollBar::onUngrabbed),
  FXMAPFUNC(SEL_TIMEOUT,FXScrollBar::ID_TIMEWHEEL,FXScrollBar::onTimeWheel),
  FXMAPFUNC(SEL_TIMEOUT,FXScrollBar::ID_AUTOSCROLL,FXScrollBar::onAutoScroll),
  FXMAPFUNC(SEL_COMMAND,FXScrollBar::ID_SETVALUE,FXScrollBar::onCmdSetValue),
  FXMAPFUNC(SEL_COMMAND,FXScrollBar::ID_SETINTVALUE,FXScrollBar::onCmdSetIntValue),
  FXMAPFUNC(SEL_COMMAND,FXScrollBar::ID_GETINTVALUE,FXScrollBar::onCmdGetIntValue),
  FXMAPFUNC(SEL_COMMAND,FXScrollBar::ID_SETLONGVALUE,FXScrollBar::onCmdSetLongValue),
  FXMAPFUNC(SEL_COMMAND,FXScrollBar::ID_GETLONGVALUE,FXScrollBar::onCmdGetLongValue),
  FXMAPFUNC(SEL_COMMAND,FXScrollBar::ID_SETINTRANGE,FXScrollBar::onCmdSetIntRange),
  FXMAPFUNC(SEL_COMMAND,FXScrollBar::ID_GETINTRANGE,FXScrollBar::onCmdGetIntRange),
  };


// Object implementation
FXIMPLEMENT(FXScrollBar,FXWindow,FXScrollBarMap,ARRAYNUMBER(FXScrollBarMap))


// For deserialization
FXScrollBar::FXScrollBar(){
  flags|=FLAG_ENABLED|FLAG_SHOWN;
  range=0;
  page=0;
  line=0;
  pos=0;
  barsize=15;
  thumbsize=8;
  thumbpos=15;
  wheelLines=1;
  hiliteColor=0;
  shadowColor=0;
  borderColor=0;
  arrowColor=0;
  dragpoint=0;
  mode=MODE_NONE;
  }


// Make a scrollbar
FXScrollBar::FXScrollBar(FXComposite* p,FXObject* tgt,FXSelector sel,FXuint opts,FXint x,FXint y,FXint w,FXint h):FXWindow(p,opts,x,y,w,h){
  flags|=FLAG_ENABLED|FLAG_SHOWN;
  wheelLines=getApp()->getWheelLines();
  backColor=getApp()->getBaseColor();
  hiliteColor=getApp()->getHiliteColor();
  shadowColor=getApp()->getShadowColor();
  borderColor=getApp()->getBorderColor();
  arrowColor=getApp()->getForeColor();
  barsize=getApp()->getScrollBarSize();
  thumbpos=barsize;
  thumbsize=barsize>>1;
  target=tgt;
  message=sel;
  range=100;
  page=1;
  line=1;
  pos=0;
  dragpoint=0;
  mode=MODE_NONE;
  }


// Get default size
FXint FXScrollBar::getDefaultWidth(){
  return (options&SCROLLBAR_HORIZONTAL) ? barsize+barsize+(barsize>>1) : barsize;
  }


FXint FXScrollBar::getDefaultHeight(){
  return (options&SCROLLBAR_HORIZONTAL) ? barsize : barsize+barsize+(barsize>>1);
  }


// Layout changed
void FXScrollBar::layout(){
  setPosition(pos);
  flags&=~FLAG_DIRTY;
  }


// Update value from a message
long FXScrollBar::onCmdSetValue(FXObject*,FXSelector,void* ptr){
  setPosition((FXint)(FXival)ptr);
  return 1;
  }


// Update value from a message
long FXScrollBar::onCmdSetIntValue(FXObject*,FXSelector,void* ptr){
  setPosition(*((FXint*)ptr));
  return 1;
  }



// Obtain value with a message
long FXScrollBar::onCmdGetIntValue(FXObject*,FXSelector,void* ptr){
  *((FXint*)ptr)=getPosition();
  return 1;
  }


// Update value from a message
long FXScrollBar::onCmdSetLongValue(FXObject*,FXSelector,void* ptr){
  setPosition((FXint)*((FXlong*)ptr));
  return 1;
  }


// Obtain value with a message
long FXScrollBar::onCmdGetLongValue(FXObject*,FXSelector,void* ptr){
  *((FXlong*)ptr)=(FXlong)getPosition();
  return 1;
  }


// Update range from a message
long FXScrollBar::onCmdSetIntRange(FXObject*,FXSelector,void* ptr){
  setRange(((FXint*)ptr)[1]);
  return 1;
  }


// Get range with a message
long FXScrollBar::onCmdGetIntRange(FXObject*,FXSelector,void* ptr){
  ((FXint*)ptr)[0]=0;
  ((FXint*)ptr)[1]=getRange();
  return 1;
  }


// Pressed LEFT button in slider
// Note we don't move the focus to the scrollbar widget!
long FXScrollBar::onLeftBtnPress(FXObject*,FXSelector,void* ptr){
  FXEvent *event=(FXEvent*)ptr;
  FXint r=range-page;
  FXint p=pos;
  if(isEnabled()){
    grab();
    getApp()->removeTimeout(this,ID_TIMEWHEEL);
    getApp()->removeTimeout(this,ID_AUTOSCROLL);
    if(target && target->tryHandle(this,FXSEL(SEL_LEFTBUTTONPRESS,message),ptr)) return 1;
    flags&=~FLAG_UPDATE;
    if(options&SCROLLBAR_HORIZONTAL){     // Horizontal scrollbar
      if(event->win_x<height){                   // Left arrow
        getApp()->addTimeout(this,ID_AUTOSCROLL,getApp()->getScrollDelay(),(void*)(FXival)-line);
        p=pos-line;
        update();
        mode=MODE_DEC;
        }
      else if(width-height<=event->win_x){       // Right arrow
        getApp()->addTimeout(this,ID_AUTOSCROLL,getApp()->getScrollDelay(),(void*)(FXival)line);
        p=pos+line;
        update();
        mode=MODE_INC;
        }
      else if(event->win_x<thumbpos){             // Page left
        getApp()->addTimeout(this,ID_AUTOSCROLL,getApp()->getScrollDelay(),(void*)(FXival)-page);
        p=pos-page;
        update();
        mode=MODE_PAGE_DEC;
        }
      else if(thumbpos+thumbsize<=event->win_x){  // Page right
        getApp()->addTimeout(this,ID_AUTOSCROLL,getApp()->getScrollDelay(),(void*)(FXival)page);
        p=pos+page;
        update();
        mode=MODE_PAGE_INC;
        }
      else{                                       // Grabbed the puck
        if(event->state&(CONTROLMASK|SHIFTMASK|ALTMASK)) mode=MODE_FINE_DRAG;
        dragpoint=event->win_x-thumbpos;
        mode=MODE_DRAG;
        }
      }
    else{                                 // Vertical scrollbar
      if(event->win_y<width){                   // Up arrow
        getApp()->addTimeout(this,ID_AUTOSCROLL,getApp()->getScrollDelay(),(void*)(FXival)-line);
        p=pos-line;
        update();
        mode=MODE_DEC;
        }
      else if(height-width<=event->win_y){      // Down arrow
        getApp()->addTimeout(this,ID_AUTOSCROLL,getApp()->getScrollDelay(),(void*)(FXival)line);
        p=pos+line;
        update();
        mode=MODE_INC;
        }
      else if(event->win_y<thumbpos){             // Page up
        getApp()->addTimeout(this,ID_AUTOSCROLL,getApp()->getScrollDelay(),(void*)(FXival)-page);
        p=pos-page;
        update();
        mode=MODE_PAGE_DEC;
        }
      else if(thumbpos+thumbsize<=event->win_y){  // Page down
        getApp()->addTimeout(this,ID_AUTOSCROLL,getApp()->getScrollDelay(),(void*)(FXival)page);
        p=pos+page;
        update();
        mode=MODE_PAGE_INC;
        }
      else{                                       // Grabbed the puck
        if(event->state&(CONTROLMASK|SHIFTMASK|ALTMASK)) mode=MODE_FINE_DRAG;
        dragpoint=event->win_y-thumbpos;
        mode=MODE_DRAG;
        }
      }
    if(p>r) p=r;
    if(p<0) p=0;
    FXASSERT(0<=p);
    if(p!=pos){
      setPosition(p);
      flags|=FLAG_CHANGED;
      if(target) target->tryHandle(this,FXSEL(SEL_CHANGED,message),(void*)(FXival)pos);
      }
    return 1;
    }
  return 0;
  }


// Released LEFT button
long FXScrollBar::onLeftBtnRelease(FXObject*,FXSelector,void* ptr){
  FXuint flgs=flags;
  if(isEnabled()){
    ungrab();
    flags&=~FLAG_CHANGED;
    flags|=FLAG_UPDATE;
    dragpoint=0;
    mode=MODE_NONE;
    setPosition(pos);
    update();
    getApp()->removeTimeout(this,ID_TIMEWHEEL);
    getApp()->removeTimeout(this,ID_AUTOSCROLL);
    if(target && target->tryHandle(this,FXSEL(SEL_LEFTBUTTONRELEASE,message),ptr)) return 1;
    if(flgs&FLAG_CHANGED){
      if(target) target->tryHandle(this,FXSEL(SEL_COMMAND,message),(void*)(FXival)pos);
      }
    return 1;
    }
  return 0;
  }


// Pressed MIDDLE button in slider
long FXScrollBar::onMiddleBtnPress(FXObject*,FXSelector,void* ptr){
  FXEvent *event=(FXEvent*)ptr;
  FXint r=range-page;
  FXint p=pos;
  FXint travel,lo,hi,t;
  if(isEnabled()){
    grab();
    getApp()->removeTimeout(this,ID_TIMEWHEEL);
    getApp()->removeTimeout(this,ID_AUTOSCROLL);
    if(target && target->tryHandle(this,FXSEL(SEL_MIDDLEBUTTONPRESS,message),ptr)) return 1;
    mode=MODE_DRAG;
    flags&=~FLAG_UPDATE;
    dragpoint=thumbsize/2;
    if(options&SCROLLBAR_HORIZONTAL){
      travel=width-height-height-thumbsize;
      t=event->win_x-dragpoint;
      if(t<height) t=height;
      if(t>(width-height-thumbsize)) t=width-height-thumbsize;
      if(t!=thumbpos){
        FXMINMAX(lo,hi,t,thumbpos);
        update(lo,0,hi+thumbsize-lo,height);
        thumbpos=t;
        }
      if(travel>0){ p=(FXint)((((FXdouble)(thumbpos-height))*r)/travel); } else { p=0; }
      }
    else{
      travel=height-width-width-thumbsize;
      t=event->win_y-dragpoint;
      if(t<width) t=width;
      if(t>(height-width-thumbsize)) t=height-width-thumbsize;
      if(t!=thumbpos){
        FXMINMAX(lo,hi,t,thumbpos);
        update(0,lo,width,hi+thumbsize-lo);
        thumbpos=t;
        }
      if(travel>0){ p=(FXint)((((FXdouble)(thumbpos-width))*r)/travel); } else { p=0; }
      }
    if(p>r) p=r;
    if(p<0) p=0;
    FXASSERT(0<=p);
    if(pos!=p){
      pos=p;
      flags|=FLAG_CHANGED;
      if(target) target->tryHandle(this,FXSEL(SEL_CHANGED,message),(void*)(FXival)pos);
      }
    return 1;
    }
  return 0;
  }


// Released MIDDLE button
long FXScrollBar::onMiddleBtnRelease(FXObject*,FXSelector,void* ptr){
  FXuint flgs=flags;
  if(isEnabled()){
    ungrab();
    flags&=~FLAG_CHANGED;
    flags|=FLAG_UPDATE;
    dragpoint=0;
    mode=MODE_NONE;
    setPosition(pos);
    update();
    getApp()->removeTimeout(this,ID_TIMEWHEEL);
    getApp()->removeTimeout(this,ID_AUTOSCROLL);
    if(target && target->tryHandle(this,FXSEL(SEL_MIDDLEBUTTONRELEASE,message),ptr)) return 1;
    if(flgs&FLAG_CHANGED){
      if(target) target->tryHandle(this,FXSEL(SEL_COMMAND,message),(void*)(FXival)pos);
      }
    return 1;
    }
  return 0;
  }


// Pressed RIGHT button in slider
long FXScrollBar::onRightBtnPress(FXObject*,FXSelector,void* ptr){
  FXEvent *event=(FXEvent*)ptr;
  FXint r=range-page;
  FXint p=pos;
  if(isEnabled()){
    grab();
    getApp()->removeTimeout(this,ID_TIMEWHEEL);
    getApp()->removeTimeout(this,ID_AUTOSCROLL);
    if(target && target->tryHandle(this,FXSEL(SEL_RIGHTBUTTONPRESS,message),ptr)) return 1;
    flags&=~FLAG_UPDATE;
    if(options&SCROLLBAR_HORIZONTAL){     // Horizontal scrollbar
      if(event->win_x<height){                   // Left arrow
        getApp()->addTimeout(this,ID_AUTOSCROLL,getApp()->getScrollDelay(),(void*)(FXival)-1);
        p=pos-1;
        update();
        mode=MODE_DEC;
        }
      else if(width-height<=event->win_x){       // Right arrow
        getApp()->addTimeout(this,ID_AUTOSCROLL,getApp()->getScrollDelay(),(void*)(FXival)1);
        p=pos+1;
        update();
        mode=MODE_INC;
        }
      else if(event->win_x<thumbpos){             // Page left
        getApp()->addTimeout(this,ID_AUTOSCROLL,getApp()->getScrollDelay(),(void*)(FXival)-line);
        p=pos-line;
        update();
        mode=MODE_PAGE_DEC;
        }
      else if(thumbpos+thumbsize<=event->win_x){  // Page right
        getApp()->addTimeout(this,ID_AUTOSCROLL,getApp()->getScrollDelay(),(void*)(FXival)line);
        p=pos+line;
        update();
        mode=MODE_PAGE_INC;
        }
      else{                                       // Grabbed the puck
        dragpoint=event->win_x-thumbpos;
        mode=MODE_FINE_DRAG;
        }
      }
    else{                                 // Vertical scrollbar
      if(event->win_y<width){                   // Up arrow
        getApp()->addTimeout(this,ID_AUTOSCROLL,getApp()->getScrollDelay(),(void*)(FXival)-1);
        p=pos-1;
        update();
        mode=MODE_DEC;
        }
      else if(height-width<=event->win_y){      // Down arrow
        getApp()->addTimeout(this,ID_AUTOSCROLL,getApp()->getScrollDelay(),(void*)(FXival)1);
        p=pos+1;
        update();
        mode=MODE_INC;
        }
      else if(event->win_y<thumbpos){             // Page up
        getApp()->addTimeout(this,ID_AUTOSCROLL,getApp()->getScrollDelay(),(void*)(FXival)-line);
        p=pos-line;
        update();
        mode=MODE_PAGE_DEC;
        }
      else if(thumbpos+thumbsize<=event->win_y){  // Page down
        getApp()->addTimeout(this,ID_AUTOSCROLL,getApp()->getScrollDelay(),(void*)(FXival)line);
        p=pos+line;
        update();
        mode=MODE_PAGE_INC;
        }
      else{                                       // Grabbed the puck
        dragpoint=event->win_y-thumbpos;
        mode=MODE_FINE_DRAG;
        }
      }
    if(p>r) p=r;
    if(p<0) p=0;
    FXASSERT(0<=p);
    if(p!=pos){
      setPosition(p);
      flags|=FLAG_CHANGED;
      if(target) target->tryHandle(this,FXSEL(SEL_CHANGED,message),(void*)(FXival)pos);
      }
    return 1;
    }
  return 0;
  }


// Released RIGHT button
long FXScrollBar::onRightBtnRelease(FXObject*,FXSelector,void* ptr){
  FXuint flgs=flags;
  if(isEnabled()){
    ungrab();
    flags&=~FLAG_CHANGED;
    flags|=FLAG_UPDATE;
    dragpoint=0;
    mode=MODE_NONE;
    setPosition(pos);
    update();
    getApp()->removeTimeout(this,ID_TIMEWHEEL);
    getApp()->removeTimeout(this,ID_AUTOSCROLL);
    if(target && target->tryHandle(this,FXSEL(SEL_RIGHTBUTTONRELEASE,message),ptr)) return 1;
    if(flgs&FLAG_CHANGED){
      if(target) target->tryHandle(this,FXSEL(SEL_COMMAND,message),(void*)(FXival)pos);
      }
    return 1;
    }
  return 0;
  }


// The widget lost the grab for some reason
long FXScrollBar::onUngrabbed(FXObject* sender,FXSelector sel,void* ptr){
  FXWindow::onUngrabbed(sender,sel,ptr);
  getApp()->removeTimeout(this,ID_TIMEWHEEL);
  getApp()->removeTimeout(this,ID_AUTOSCROLL);
  flags&=~FLAG_CHANGED;
  flags|=FLAG_UPDATE;
  dragpoint=0;
  mode=MODE_NONE;
  return 1;
  }


// Moving
long FXScrollBar::onMotion(FXObject*,FXSelector,void* ptr){
  FXEvent *event=(FXEvent*)ptr;
  FXint r=range-page;
  FXint p=pos;
  FXint travel,hi,lo,t;
  if(!isEnabled()) return 0;
  if(mode>=MODE_DRAG){

    // If modifiers down, fine scrolling method goes in effect, if
    // not, switch back to coarse mode (thanks, Tony <verant@mail.ru>)!
    if(event->state&(CONTROLMASK|SHIFTMASK|ALTMASK|RIGHTBUTTONMASK))
      mode=MODE_FINE_DRAG;
    else
      mode=MODE_DRAG;

    // Coarse movements
    if(mode==MODE_DRAG){
      if(options&SCROLLBAR_HORIZONTAL){
        travel=width-height-height-thumbsize;
        t=event->win_x-dragpoint;
        if(t<height) t=height;
        if(t>(width-height-thumbsize)) t=width-height-thumbsize;
        if(t!=thumbpos){
          FXMINMAX(lo,hi,t,thumbpos);
          update(lo,0,hi+thumbsize-lo,height);
          thumbpos=t;
          }
        if(travel>0){ p=(FXint)((((FXdouble)(thumbpos-height))*r+travel/2)/travel); }
        }
      else{
        travel=height-width-width-thumbsize;
        t=event->win_y-dragpoint;
        if(t<width) t=width;
        if(t>(height-width-thumbsize)) t=height-width-thumbsize;
        if(t!=thumbpos){
          FXMINMAX(lo,hi,t,thumbpos);
          update(0,lo,width,hi+thumbsize-lo);
          thumbpos=t;
          }
        if(travel>0){ p=(FXint)((((FXdouble)(thumbpos-width))*r+travel/2)/travel); }
        }
      }

    // Fine movements
    else if(mode==MODE_FINE_DRAG){
      if(options&SCROLLBAR_HORIZONTAL){
        travel=width-height-height-thumbsize;
        p=pos+event->win_x-event->last_x;
        if(p>r) p=r;
        if(p<0) p=0;
        if(r>0){ t=height+(FXint)((((FXdouble)pos)*travel)/r); } else { t=height; }
        if(t!=thumbpos){
          FXMINMAX(lo,hi,t,thumbpos);
          update(lo,0,hi+thumbsize-lo,height);
          thumbpos=t;
          }
        }
      else{
        travel=height-width-width-thumbsize;
        p=pos+event->win_y-event->last_y;
        if(p>r) p=r;
        if(p<0) p=0;
        if(r>0){ t=width+(FXint)((((FXdouble)pos)*travel)/r); } else { t=width; }
        if(t!=thumbpos){
          FXMINMAX(lo,hi,t,thumbpos);
          update(0,lo,width,hi+thumbsize-lo);
          thumbpos=t;
          }
        }
      }

    // Clamp range and issue callbacks
    if(p>r) p=r;
    if(p<0) p=0;
    FXASSERT(0<=p);
    if(pos!=p){
      pos=p;
      flags|=FLAG_CHANGED;
      if(target) target->tryHandle(this,FXSEL(SEL_CHANGED,message),(void*)(FXival)pos);
      return 1;
      }
    }
  return 0;
  }


// Mouse wheel
long FXScrollBar::onMouseWheel(FXObject*,FXSelector,void* ptr){
  if(isEnabled()){
    FXEvent* ev=(FXEvent*)ptr;
    FXint r=range-page;
    FXint jump,dragjump;
    getApp()->removeTimeout(this,ID_AUTOSCROLL);
    if(!(ev->state&(LEFTBUTTONMASK|MIDDLEBUTTONMASK|RIGHTBUTTONMASK))){

      // Scroll by a line-at-a-time, page-at-a-time, or by given wheel-lines
      if(ev->state&ALTMASK) jump=line;
      else if(ev->state&CONTROLMASK) jump=page;
      else jump=FXMIN(page,wheelLines*line);

      // Move scroll position
      dragpoint-=ev->code*jump/120;

      // Keep it within bounds of the document
      if(pos+dragpoint<0) dragpoint=-pos;
      if(pos+dragpoint>r) dragpoint=r-pos;

      // Any amount to scroll at all?
      if(dragpoint){

        // Jump scroll is less cpu time
        if(options&SCROLLBAR_WHEELJUMP){
          setPosition(pos+dragpoint);
          dragpoint=0;
          if(target) target->tryHandle(this,FXSEL(SEL_COMMAND,message),(void*)(FXival)pos);
          }

        // Animated scroll is better looking
        else{
          dragjump=dragpoint;
          if(FXABS(dragjump)>WHEELJUMPINCR) dragjump/=WHEELJUMPINCR;

          // Schedule timer to animate scroll
          getApp()->addTimeout(this,ID_TIMEWHEEL,WHEELJUMPTIME,(void*)(FXival)dragjump);
          }
        }
      return 1;
      }
    }
  return 0;
  }


// Smoothly scroll to desired value as determined by wheel
long FXScrollBar::onTimeWheel(FXObject*,FXSelector,void* ptr){
  if(dragpoint){

    // Scroll step, which is a fraction of the amount to be scrolled
    FXint delta=(FXint)(FXival)ptr;

    // Target scroll amount larger than animation step:- schedule another step
    // Adjust the amount left to go to the target scroll amount
    if(FXABS(dragpoint)>FXABS(delta)){

      // Scroll by delta step
      setPosition(pos+delta);

      // Adjust remaining amount to scroll
      dragpoint-=delta;

      // Schedule next timer
      getApp()->addTimeout(this,ID_TIMEWHEEL,WHEELJUMPTIME,ptr);

      // Tell target intermediate position
      if(target) target->tryHandle(this,FXSEL(SEL_CHANGED,message),(void*)(FXival)pos);
      return 1;
      }

    // Almost there; make one more step to get to exact target
    setPosition(pos+dragpoint);

    // Since this will reach the target, remaining scroll amount now zero
    dragpoint=0;

    // Tell target where scrollbar is now at
    if(target) target->tryHandle(this,FXSEL(SEL_COMMAND,message),(void*)(FXival)pos);
    }
  return 1;
  }


// Automatic scroll based on timer
long FXScrollBar::onAutoScroll(FXObject*,FXSelector,void* ptr){
  FXint p=pos+(FXint)(FXival)ptr;
  FXint r=range-page;
  if(p>=r) p=r;
  if(p<=0) p=0;
  FXASSERT(0<=p);
  if(p!=pos){
    getApp()->addTimeout(this,ID_AUTOSCROLL,getApp()->getScrollSpeed(),ptr);
    setPosition(p);
    flags|=FLAG_CHANGED;
    if(target) target->tryHandle(this,FXSEL(SEL_CHANGED,message),(void*)(FXival)pos);
    return 1;
    }
  return 0;
  }


// Draw button in scrollbar; this is slightly different from a raised rectangle
void FXScrollBar::drawButton(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h,FXbool down){
  if(down){
    dc.setForeground(borderColor);
    dc.fillRectangle(x,y,w-2,1);
    dc.fillRectangle(x,y,1,h-2);
    dc.setForeground(shadowColor);
    dc.fillRectangle(x+1,y+1,w-3,1);
    dc.fillRectangle(x+1,y+1,1,h-3);
    dc.setForeground(hiliteColor);
    dc.fillRectangle(x,y+h-1,w-1,1);
    dc.fillRectangle(x+w-1,y+1,1,h-1);
    dc.setForeground(backColor);
    dc.fillRectangle(x+1,y+h-2,w-1,1);
    dc.fillRectangle(x+w-2,y+2,1,h-2);
    }
  else{
    dc.setForeground(backColor);
    dc.fillRectangle(x,y,w-1,1);
    dc.fillRectangle(x,y,1,h-1);
    dc.setForeground(hiliteColor);
    dc.fillRectangle(x+1,y+1,w-2,1);
    dc.fillRectangle(x+1,y+1,1,h-2);
    dc.setForeground(shadowColor);
    dc.fillRectangle(x+1,y+h-2,w-2,1);
    dc.fillRectangle(x+w-2,y+1,1,h-2);
    dc.setForeground(borderColor);
    dc.fillRectangle(x,y+h-1,w,1);
    dc.fillRectangle(x+w-1,y,1,h);
    }
  dc.setForeground(backColor);
  dc.fillRectangle(x+2,y+2,w-4,h-4);
  }


// Draw thumb in the scrollbar
void FXScrollBar::drawThumb(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h){
  dc.setForeground(backColor);
  dc.fillRectangle(x,y,w-1,1);
  dc.fillRectangle(x,y,1,h-1);
  dc.setForeground(hiliteColor);
  dc.fillRectangle(x+1,y+1,w-2,1);
  dc.fillRectangle(x+1,y+1,1,h-2);
  dc.setForeground(shadowColor);
  dc.fillRectangle(x+1,y+h-2,w-2,1);
  dc.fillRectangle(x+w-2,y+1,1,h-2);
  dc.setForeground(borderColor);
  dc.fillRectangle(x,y+h-1,w,1);
  dc.fillRectangle(x+w-1,y,1,h);
  dc.setForeground(backColor);
  dc.fillRectangle(x+2,y+2,w-4,h-4);
  }


// Draw left arrow
void FXScrollBar::drawLeftArrow(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h,FXbool down){
  FXPoint points[3];
  FXint ah,ab;
  ab=(h-7)|1;
  ah=ab>>1;
  x=x+((w-ah)>>1);
  y=y+((h-ab)>>1);
  if(down){ ++x; ++y; }
  points[0].x=x+ah;
  points[0].y=y;
  points[1].x=x+ah;
  points[1].y=y+ab-1;
  points[2].x=x;
  points[2].y=y+ah;
  dc.setForeground(arrowColor);
  dc.fillPolygon(points,3);
  }


// Draw right arrow
void FXScrollBar::drawRightArrow(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h,FXbool down){
  FXPoint points[3];
  FXint ah,ab;
  ab=(h-7)|1;
  ah=ab>>1;
  x=x+((w-ah)>>1);
  y=y+((h-ab)>>1);
  if(down){ ++x; ++y; }
  points[0].x=x;
  points[0].y=y;
  points[1].x=x;
  points[1].y=y+ab-1;
  points[2].x=x+ah;
  points[2].y=y+ah;
  dc.setForeground(arrowColor);
  dc.fillPolygon(points,3);
  }


// Draw up arrow
void FXScrollBar::drawUpArrow(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h,FXbool down){
  FXPoint points[3];
  FXint ah,ab;
  ab=(w-7)|1;
  ah=ab>>1;
  x=x+((w-ab)>>1);
  y=y+((h-ah)>>1);
  if(down){ ++x; ++y; }
  points[0].x=x+ah;
  points[0].y=y-1;
  points[1].x=x;
  points[1].y=y+ah;
  points[2].x=x+ab;
  points[2].y=y+ah;
  dc.setForeground(arrowColor);
  dc.fillPolygon(points,3);
  }


// Draw down arrow
void FXScrollBar::drawDownArrow(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h,FXbool down){
  FXPoint points[3];
  FXint ah,ab;
  ab=(w-7)|1;
  ah=ab>>1;
  x=x+((w-ab)>>1);
  y=y+((h-ah)>>1);
  if(down){ ++x; ++y; }
  points[0].x=x+1;
  points[0].y=y;
  points[1].x=x+ab-1;
  points[1].y=y;
  points[2].x=x+ah;
  points[2].y=y+ah;
  dc.setForeground(arrowColor);
  dc.fillPolygon(points,3);
  }


// Handle repaint
long FXScrollBar::onPaint(FXObject*,FXSelector,void* ptr){
  FXEvent *ev=(FXEvent*)ptr;
  int total;
  FXDCWindow dc(this,ev);
  if(options&SCROLLBAR_HORIZONTAL){
    total=width-height-height;
    if(thumbsize<total){                                    // Scrollable
      dc.setStipple(STIPPLE_GRAY);
      dc.setFillStyle(FILL_OPAQUESTIPPLED);
      if(mode==MODE_PAGE_DEC){
        dc.setForeground(backColor);
        dc.setBackground(shadowColor);
        }
      else{
        dc.setForeground(hiliteColor);
        dc.setBackground(backColor);
        }
      dc.fillRectangle(height,0,thumbpos-height,height);
      if(mode==MODE_PAGE_INC){
        dc.setForeground(backColor);
        dc.setBackground(shadowColor);
        }
      else{
        dc.setForeground(hiliteColor);
        dc.setBackground(backColor);
        }
      dc.fillRectangle(thumbpos+thumbsize,0,width-height-thumbpos-thumbsize,height);
      }
    else{                                                   // Non-scrollable
      dc.setForeground(hiliteColor);
      dc.setBackground(backColor);
      dc.fillRectangle(height,0,total,height);
      }
    dc.setFillStyle(FILL_SOLID);
    drawThumb(dc,thumbpos,0,thumbsize,height);
    drawButton(dc,width-height,0,height,height,(mode==MODE_INC));
    drawRightArrow(dc,width-height,0,height,height,(mode==MODE_INC));
    drawButton(dc,0,0,height,height,(mode==MODE_DEC));
    drawLeftArrow(dc,0,0,height,height,(mode==MODE_DEC));
    }
  else{
    total=height-width-width;
    if(thumbsize<total){                                    // Scrollable
      dc.setStipple(STIPPLE_GRAY);
      dc.setFillStyle(FILL_OPAQUESTIPPLED);
      if(mode==MODE_PAGE_DEC){
        dc.setForeground(backColor);
        dc.setBackground(shadowColor);
        }
      else{
        dc.setForeground(hiliteColor);
        dc.setBackground(backColor);
        }
      dc.fillRectangle(0,width,width,thumbpos-width);
      if(mode==MODE_PAGE_INC){
        dc.setForeground(backColor);
        dc.setBackground(shadowColor);
        }
      else{
        dc.setForeground(hiliteColor);
        dc.setBackground(backColor);
        }
      dc.fillRectangle(0,thumbpos+thumbsize,width,height-width-thumbpos-thumbsize);
      }
    else{                                                   // Non-scrollable
      dc.setForeground(hiliteColor);
      dc.setBackground(backColor);
      dc.fillRectangle(0,width,width,total);
      }
    dc.setFillStyle(FILL_SOLID);
    drawThumb(dc,0,thumbpos,width,thumbsize);
    drawButton(dc,0,height-width,width,width,(mode==MODE_INC));
    drawDownArrow(dc,0,height-width,width,width,(mode==MODE_INC));
    drawButton(dc,0,0,width,width,(mode==MODE_DEC));
    drawUpArrow(dc,0,0,width,width,(mode==MODE_DEC));
    }
  return 1;
  }


// Set range
void FXScrollBar::setRange(FXint r,FXbool notify){
  if(r<1) r=1;
  if(range!=r){
    range=r;
    setPosition(pos,notify);
    }
  }


// Set page size
void FXScrollBar::setPage(FXint p,FXbool notify){
  if(p<1) p=1;
  if(page!=p){
    page=p;
    setPosition(pos,notify);
    }
  }


// Set line size
void FXScrollBar::setLine(FXint l){
  if(l<1) l=1;
  line=l;
  }


// Set position; tricky because the thumb size may have changed
// as well; we do the minimal possible update to repaint properly.
void FXScrollBar::setPosition(FXint p,FXbool notify){
  FXint hi=thumbpos+thumbsize;
  FXint lo=thumbpos;
  FXint r=range-page;
  FXint total,travel,l,h;
  if(p>r) p=r;
  if(p<0) p=0;
  FXASSERT(0<=p);
  if(options&SCROLLBAR_HORIZONTAL){
    total=width-height-height;
    thumbsize=(total*page)/range;
    if(thumbsize<(barsize>>1)) thumbsize=(barsize>>1);
    travel=total-thumbsize;
    if(r>0){ thumbpos=height+(FXint)((((FXdouble)p)*travel)/r); } else { thumbpos=height; }
    l=thumbpos;
    h=thumbpos+thumbsize;
    if(l!=lo || h!=hi){
      update(FXMIN(l,lo),0,FXMAX(h,hi)-FXMIN(l,lo),height);
      }
    }
  else{
    total=height-width-width;
    thumbsize=(total*page)/range;
    if(thumbsize<(barsize>>1)) thumbsize=(barsize>>1);
    travel=total-thumbsize;
    if(r>0){ thumbpos=width+(FXint)((((FXdouble)p)*travel)/r); } else { thumbpos=width; }
    l=thumbpos;
    h=thumbpos+thumbsize;
    if(l!=lo || h!=hi){
      update(0,FXMIN(l,lo),width,FXMAX(h,hi)-FXMIN(l,lo));
      }
    }
  if(pos!=p){
    pos=p;
    if(notify && target){target->tryHandle(this,FXSEL(SEL_COMMAND,message),(void*)(FXival)pos);}
    }
  }


// Set highlight color
void FXScrollBar::setHiliteColor(FXColor clr){
  if(hiliteColor!=clr){
    hiliteColor=clr;
    update();
    }
  }


// Set shadow color
void FXScrollBar::setShadowColor(FXColor clr){
  if(shadowColor!=clr){
    shadowColor=clr;
    update();
    }
  }


// Set border color
void FXScrollBar::setBorderColor(FXColor clr){
  if(borderColor!=clr){
    borderColor=clr;
    update();
    }
  }


// Set arrow color
void FXScrollBar::setArrowColor(FXColor clr){
  if(arrowColor!=clr){
    arrowColor=clr;
    update();
    }
  }


// Change the scrollbar style
FXuint FXScrollBar::getScrollBarStyle() const {
  return (options&SCROLLBAR_MASK);
  }


// Get the current scrollbar style
void FXScrollBar::setScrollBarStyle(FXuint style){
  FXuint opts=(options&~SCROLLBAR_MASK) | (style&SCROLLBAR_MASK);
  if(options!=opts){
    options=opts;
    recalc();
    update();
    }
  }


// Change the bar size
void FXScrollBar::setBarSize(FXint size){
  if(barsize!=size){
    barsize=size;
    recalc();
    }
  }


// Save object to stream
void FXScrollBar::save(FXStream& store) const {
  FXWindow::save(store);
  store << barsize;
  store << hiliteColor;
  store << shadowColor;
  store << borderColor;
  store << arrowColor;
  store << range;
  store << page;
  store << line;
  store << pos;
  }


// Load object from stream
void FXScrollBar::load(FXStream& store){
  FXWindow::load(store);
  store >> barsize;
  store >> hiliteColor;
  store >> shadowColor;
  store >> borderColor;
  store >> arrowColor;
  store >> range;
  store >> page;
  store >> line;
  store >> pos;
  }


// Delete
FXScrollBar::~FXScrollBar(){
  getApp()->removeTimeout(this,ID_TIMEWHEEL);
  getApp()->removeTimeout(this,ID_AUTOSCROLL);
  }


/*******************************************************************************/

// Map
FXDEFMAP(FXScrollCorner) FXScrollCornerMap[]={
  FXMAPFUNC(SEL_PAINT,0,FXScrollCorner::onPaint),
  };


// Object implementation
FXIMPLEMENT(FXScrollCorner,FXWindow,FXScrollCornerMap,ARRAYNUMBER(FXScrollCornerMap))


// Deserialization
FXScrollCorner::FXScrollCorner(){
  flags|=FLAG_ENABLED|FLAG_SHOWN;
  }


// Construct and init
FXScrollCorner::FXScrollCorner(FXComposite* p):FXWindow(p){
  backColor=getApp()->getBaseColor();
  flags|=FLAG_ENABLED|FLAG_SHOWN;
  }


// Slightly different from Frame border
long FXScrollCorner::onPaint(FXObject*,FXSelector,void* ptr){
  FXDCWindow dc(this,(FXEvent*)ptr);
  dc.setForeground(backColor);
  dc.fillRectangle(0,0,width,height);
  return 1;
  }


// Enable the window
void FXScrollCorner::enable(){
  }


// Disable the window
void FXScrollCorner::disable(){
  }

}
