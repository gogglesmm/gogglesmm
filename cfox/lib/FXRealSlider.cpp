/********************************************************************************
*                                                                               *
*                       R e a l S l i d e r   W i d g e t                       *
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
#include "FXRealSlider.h"




/*
  Notes:
  - Maybe add bindings for arrow keys for value changes.
  - Yes, this *does* have a lot in common with FXSlider and its
    probably a good idea to give FXRealSlider and FXSlider a common
    base class at some point.
*/

#define TICKSIZE        4           // Length of ticks
#define OVERHANG        4           // Default amount of overhang
#define MINOVERHANG     3           // Minimal amount of overhang
#define HEADINSIDEBAR   20          // Default for inside bar head size
#define HEADOVERHANGING 9           // Default for overhanging head size

#define REALSLIDER_MASK (REALSLIDER_VERTICAL|REALSLIDER_ARROW_UP|REALSLIDER_ARROW_DOWN|REALSLIDER_INSIDE_BAR|REALSLIDER_TICKS_TOP|REALSLIDER_TICKS_BOTTOM)

using namespace FX;

/*******************************************************************************/

namespace FX {

// Map
FXDEFMAP(FXRealSlider) FXRealSliderMap[]={
  FXMAPFUNC(SEL_PAINT,0,FXRealSlider::onPaint),
  FXMAPFUNC(SEL_MOTION,0,FXRealSlider::onMotion),
  FXMAPFUNC(SEL_MOUSEWHEEL,0,FXRealSlider::onMouseWheel),
  FXMAPFUNC(SEL_LEFTBUTTONPRESS,0,FXRealSlider::onLeftBtnPress),
  FXMAPFUNC(SEL_LEFTBUTTONRELEASE,0,FXRealSlider::onLeftBtnRelease),
  FXMAPFUNC(SEL_MIDDLEBUTTONPRESS,0,FXRealSlider::onMiddleBtnPress),
  FXMAPFUNC(SEL_MIDDLEBUTTONRELEASE,0,FXRealSlider::onMiddleBtnRelease),
  FXMAPFUNC(SEL_KEYPRESS,0,FXRealSlider::onKeyPress),
  FXMAPFUNC(SEL_KEYRELEASE,0,FXRealSlider::onKeyRelease),
  FXMAPFUNC(SEL_UNGRABBED,0,FXRealSlider::onUngrabbed),
  FXMAPFUNC(SEL_QUERY_TIP,0,FXRealSlider::onQueryTip),
  FXMAPFUNC(SEL_QUERY_HELP,0,FXRealSlider::onQueryHelp),
  FXMAPFUNC(SEL_TIMEOUT,FXRealSlider::ID_AUTOSLIDE,FXRealSlider::onAutoSlide),
  FXMAPFUNC(SEL_COMMAND,FXRealSlider::ID_SETVALUE,FXRealSlider::onCmdSetValue),
  FXMAPFUNC(SEL_COMMAND,FXRealSlider::ID_SETINTVALUE,FXRealSlider::onCmdSetIntValue),
  FXMAPFUNC(SEL_COMMAND,FXRealSlider::ID_GETINTVALUE,FXRealSlider::onCmdGetIntValue),
  FXMAPFUNC(SEL_COMMAND,FXRealSlider::ID_SETLONGVALUE,FXRealSlider::onCmdSetLongValue),
  FXMAPFUNC(SEL_COMMAND,FXRealSlider::ID_GETLONGVALUE,FXRealSlider::onCmdGetLongValue),
  FXMAPFUNC(SEL_COMMAND,FXRealSlider::ID_SETREALVALUE,FXRealSlider::onCmdSetRealValue),
  FXMAPFUNC(SEL_COMMAND,FXRealSlider::ID_GETREALVALUE,FXRealSlider::onCmdGetRealValue),
  FXMAPFUNC(SEL_COMMAND,FXRealSlider::ID_SETINTRANGE,FXRealSlider::onCmdSetIntRange),
  FXMAPFUNC(SEL_COMMAND,FXRealSlider::ID_GETINTRANGE,FXRealSlider::onCmdGetIntRange),
  FXMAPFUNC(SEL_COMMAND,FXRealSlider::ID_SETREALRANGE,FXRealSlider::onCmdSetRealRange),
  FXMAPFUNC(SEL_COMMAND,FXRealSlider::ID_GETREALRANGE,FXRealSlider::onCmdGetRealRange),
  FXMAPFUNC(SEL_COMMAND,FXRealSlider::ID_SETHELPSTRING,FXRealSlider::onCmdSetHelp),
  FXMAPFUNC(SEL_COMMAND,FXRealSlider::ID_GETHELPSTRING,FXRealSlider::onCmdGetHelp),
  FXMAPFUNC(SEL_COMMAND,FXRealSlider::ID_SETTIPSTRING,FXRealSlider::onCmdSetTip),
  FXMAPFUNC(SEL_COMMAND,FXRealSlider::ID_GETTIPSTRING,FXRealSlider::onCmdGetTip),
  };


// Object implementation
FXIMPLEMENT(FXRealSlider,FXFrame,FXRealSliderMap,ARRAYNUMBER(FXRealSliderMap))


// Make a slider
FXRealSlider::FXRealSlider(){
  flags|=FLAG_ENABLED;
  headPos=0;
  headSize=0;
  slotSize=0;
  slotColor=0;
  dragPoint=0;
  range[0]=0.0;
  range[1]=0.0;
  delta=0.0;
  incr=0.01;
  gran=0.0;
  pos=0.0;
  }


// Make a slider
FXRealSlider::FXRealSlider(FXComposite* p,FXObject* tgt,FXSelector sel,FXuint opts,FXint x,FXint y,FXint w,FXint h,FXint pl,FXint pr,FXint pt,FXint pb):FXFrame(p,opts,x,y,w,h,pl,pr,pt,pb){
  flags|=FLAG_ENABLED;
  baseColor=getApp()->getBaseColor();
  hiliteColor=getApp()->getHiliteColor();
  shadowColor=getApp()->getShadowColor();
  borderColor=getApp()->getBorderColor();
  target=tgt;
  message=sel;
  headPos=0;
  headSize=(options&REALSLIDER_INSIDE_BAR)?HEADINSIDEBAR:HEADOVERHANGING;
  slotSize=5;
  slotColor=getApp()->getBackColor();
  dragPoint=0;
  range[0]=0.0;
  range[1]=1.0;
  delta=0.0;
  incr=0.01;
  gran=0.0;
  pos=0.5;
  }


// Enable the window
void FXRealSlider::enable(){
  if(!(flags&FLAG_ENABLED)){
    FXFrame::enable();
    update();
    }
  }


// Disable the window
void FXRealSlider::disable(){
  if(flags&FLAG_ENABLED){
    FXFrame::disable();
    update();
    }
  }


// Get default width
FXint FXRealSlider::getDefaultWidth(){
  FXint w;
  if(options&REALSLIDER_VERTICAL){
    if(options&REALSLIDER_INSIDE_BAR) w=4+headSize/2;
    else if(options&(REALSLIDER_ARROW_LEFT|REALSLIDER_ARROW_RIGHT)) w=slotSize+MINOVERHANG*2+headSize/2;
    else w=slotSize+MINOVERHANG*2;
    if(options&REALSLIDER_TICKS_LEFT) w+=TICKSIZE;
    if(options&REALSLIDER_TICKS_RIGHT) w+=TICKSIZE;
    }
  else{
    w=headSize+4;
    }
  return w+padleft+padright+(border<<1);
  }


// Get default height
FXint FXRealSlider::getDefaultHeight(){
  FXint h;
  if(options&REALSLIDER_VERTICAL){
    h=headSize+4;
    }
  else{
    if(options&REALSLIDER_INSIDE_BAR) h=4+headSize/2;
    else if(options&(REALSLIDER_ARROW_UP|REALSLIDER_ARROW_DOWN)) h=slotSize+2*MINOVERHANG+headSize/2;
    else h=slotSize+MINOVERHANG*2;
    if(options&REALSLIDER_TICKS_TOP) h+=TICKSIZE;
    if(options&REALSLIDER_TICKS_BOTTOM) h+=TICKSIZE;
    }
  return h+padtop+padbottom+(border<<1);
  }


// Returns true because a slider can receive focus
FXbool FXRealSlider::canFocus() const { return true; }


// Layout changed; even though the position is still
// the same, the head may have to be moved.
void FXRealSlider::layout(){
  setValue(pos);
  flags&=~FLAG_DIRTY;
  }


// Set help using a message
long FXRealSlider::onCmdSetHelp(FXObject*,FXSelector,void* ptr){
  setHelpText(*((FXString*)ptr));
  return 1;
  }


// Get help using a message
long FXRealSlider::onCmdGetHelp(FXObject*,FXSelector,void* ptr){
  *((FXString*)ptr)=getHelpText();
  return 1;
  }


// Set tip using a message
long FXRealSlider::onCmdSetTip(FXObject*,FXSelector,void* ptr){
  setTipText(*((FXString*)ptr));
  return 1;
  }


// Get tip using a message
long FXRealSlider::onCmdGetTip(FXObject*,FXSelector,void* ptr){
  *((FXString*)ptr)=getTipText();
  return 1;
  }


// We were asked about tip text
long FXRealSlider::onQueryTip(FXObject* sender,FXSelector sel,void* ptr){
  if(FXFrame::onQueryTip(sender,sel,ptr)) return 1;
  if((flags&FLAG_TIP) && !tip.empty()){
    sender->handle(this,FXSEL(SEL_COMMAND,ID_SETSTRINGVALUE),(void*)&tip);
    return 1;
    }
  return 0;
  }


// We were asked about status text
long FXRealSlider::onQueryHelp(FXObject* sender,FXSelector sel,void* ptr){
  if(FXFrame::onQueryHelp(sender,sel,ptr)) return 1;
  if((flags&FLAG_HELP) && !help.empty()){
    sender->handle(this,FXSEL(SEL_COMMAND,ID_SETSTRINGVALUE),(void*)&help);
    return 1;
    }
  return 0;
  }


// Update value from a message
long FXRealSlider::onCmdSetValue(FXObject*,FXSelector,void* ptr){
  setValue((FXdouble)(FXival)ptr);
  return 1;
  }


// Update value from a message
long FXRealSlider::onCmdSetIntValue(FXObject*,FXSelector,void* ptr){
  setValue((FXdouble)*((FXint*)ptr));
  return 1;
  }


// Obtain value from text field
long FXRealSlider::onCmdGetIntValue(FXObject*,FXSelector,void* ptr){
  *((FXint*)ptr)=(FXint)getValue();
  return 1;
  }


// Update value from a message
long FXRealSlider::onCmdSetLongValue(FXObject*,FXSelector,void* ptr){
  setValue((FXdouble)*((FXlong*)ptr));
  return 1;
  }


// Obtain value with a message
long FXRealSlider::onCmdGetLongValue(FXObject*,FXSelector,void* ptr){
  *((FXlong*)ptr)=(FXlong)getValue();
  return 1;
  }


// Update value from a message
long FXRealSlider::onCmdSetRealValue(FXObject*,FXSelector,void* ptr){
  setValue(*((FXdouble*)ptr));
  return 1;
  }


// Obtain value with a message
long FXRealSlider::onCmdGetRealValue(FXObject*,FXSelector,void* ptr){
  *((FXdouble*)ptr)=getValue();
  return 1;
  }


// Update range from a message
long FXRealSlider::onCmdSetIntRange(FXObject*,FXSelector,void* ptr){
  setRange((FXdouble)((FXint*)ptr)[0],(FXdouble)((FXint*)ptr)[1]);
  return 1;
  }


// Get range with a message
long FXRealSlider::onCmdGetIntRange(FXObject*,FXSelector,void* ptr){
  ((FXint*)ptr)[0]=(FXint)range[0];
  ((FXint*)ptr)[1]=(FXint)range[1];
  return 1;
  }


// Update range from a message
long FXRealSlider::onCmdSetRealRange(FXObject*,FXSelector,void* ptr){
  setRange(((FXdouble*)ptr)[0],((FXdouble*)ptr)[1]);
  return 1;
  }


// Get range with a message
long FXRealSlider::onCmdGetRealRange(FXObject*,FXSelector,void* ptr){
  ((FXdouble*)ptr)[0]=range[0];
  ((FXdouble*)ptr)[1]=range[1];
  return 1;
  }


// Pressed LEFT button
long FXRealSlider::onLeftBtnPress(FXObject*,FXSelector,void* ptr){
  FXEvent *event=(FXEvent*)ptr;
  FXdouble p=pos;
  flags&=~FLAG_TIP;
  handle(this,FXSEL(SEL_FOCUS_SELF,0),ptr);
  if(isEnabled()){
    grab();
    getApp()->removeTimeout(this,ID_AUTOSLIDE);
    if(target && target->tryHandle(this,FXSEL(SEL_LEFTBUTTONPRESS,message),ptr)) return 1;
    flags&=~FLAG_UPDATE;
    if(options&REALSLIDER_VERTICAL){
      if(event->win_y<headPos){
        getApp()->addTimeout(this,ID_AUTOSLIDE,getApp()->getScrollDelay(),(void*)(FXival)1);
        p+=incr;
        }
      else if(event->win_y>(headPos+headSize)){
        getApp()->addTimeout(this,ID_AUTOSLIDE,getApp()->getScrollDelay(),(void*)(FXival)-1);
        p-=incr;
        }
      else{
        dragPoint=event->win_y-headPos;
        flags|=FLAG_PRESSED;
        }
      }
    else{
      if(event->win_x<headPos){
        getApp()->addTimeout(this,ID_AUTOSLIDE,getApp()->getScrollDelay(),(void*)(FXival)-1);
        p-=incr;
        }
      else if(event->win_x>(headPos+headSize)){
        getApp()->addTimeout(this,ID_AUTOSLIDE,getApp()->getScrollDelay(),(void*)(FXival)1);
        p+=incr;
        }
      else{
        dragPoint=event->win_x-headPos;
        flags|=FLAG_PRESSED;
        }
      }
    if(0.0<gran) p=gran*Math::round(p/gran);
    if(p<range[0]) p=range[0];
    if(p>range[1]) p=range[1];
    if(p!=pos){
      setValue(p);
      flags|=FLAG_CHANGED;
      if(target) target->tryHandle(this,FXSEL(SEL_CHANGED,message),(void*)&pos);
      }
    return 1;
    }
  return 0;
  }


// Released Left button
long FXRealSlider::onLeftBtnRelease(FXObject*,FXSelector,void* ptr){
  FXuint flgs=flags;
  if(isEnabled()){
    ungrab();
    getApp()->removeTimeout(this,ID_AUTOSLIDE);
    setValue(pos);                                                 // Hop to exact position
    flags&=~FLAG_PRESSED;
    flags&=~FLAG_CHANGED;
    flags|=FLAG_UPDATE;
    if(target && target->tryHandle(this,FXSEL(SEL_LEFTBUTTONRELEASE,message),ptr)) return 1;
    if(flgs&FLAG_CHANGED){
      if(target) target->tryHandle(this,FXSEL(SEL_COMMAND,message),(void*)&pos);
      }
    return 1;
    }
  return 0;
  }


// Moving
long FXRealSlider::onMotion(FXObject*,FXSelector,void* ptr){
  FXEvent *event=(FXEvent*)ptr;
  FXint xx,yy,ww,hh,lo,hi,h,travel;
  FXdouble p;
  if(!isEnabled()) return 0;
  if(flags&FLAG_PRESSED){
    yy=border+padtop+2;
    xx=border+padleft+2;
    hh=height-(border<<1)-padtop-padbottom-4;
    ww=width-(border<<1)-padleft-padright-4;
    if(options&REALSLIDER_VERTICAL){
      h=event->win_y-dragPoint;
      travel=hh-headSize;
      if(h<yy) h=yy;
      if(h>yy+travel) h=yy+travel;
      if(h!=headPos){
        FXMINMAX(lo,hi,headPos,h);
        headPos=h;
        update(border,lo-1,width-(border<<1),hi+headSize+2-lo);
        }
      if(travel>0)
        p=range[0]+((range[1]-range[0])*(yy+travel-h))/travel;
      else
        p=range[0];
      }
    else{
      h=event->win_x-dragPoint;
      travel=ww-headSize;
      if(h<xx) h=xx;
      if(h>xx+travel) h=xx+travel;
      if(h!=headPos){
        FXMINMAX(lo,hi,headPos,h);
        headPos=h;
        update(lo-1,border,hi+headSize+2-lo,height-(border<<1));
        }
      if(travel>0)
        p=range[0]+((range[1]-range[0])*(h-xx))/travel;
      else
        p=range[0];
      }
    if(0.0<gran) p=gran*Math::round(p/gran);
    if(p<range[0]) p=range[0];
    if(p>range[1]) p=range[1];
    if(pos!=p){
      pos=p;
      flags|=FLAG_CHANGED;
      if(target) target->tryHandle(this,FXSEL(SEL_CHANGED,message),(void*)&pos);
      }
    return 1;
    }
  return 0;
  }


// Pressed middle or right
long FXRealSlider::onMiddleBtnPress(FXObject*,FXSelector,void* ptr){
  FXEvent *event=(FXEvent*)ptr;
  FXint xx,yy,ww,hh,lo,hi,h,travel;
  FXdouble p;
  flags&=~FLAG_TIP;
  handle(this,FXSEL(SEL_FOCUS_SELF,0),ptr);
  if(isEnabled()){
    grab();
    if(target && target->tryHandle(this,FXSEL(SEL_MIDDLEBUTTONPRESS,message),ptr)) return 1;
    dragPoint=headSize/2;
    yy=border+padtop+2;
    xx=border+padleft+2;
    hh=height-(border<<1)-padtop-padbottom-4;
    ww=width-(border<<1)-padleft-padright-4;
    flags&=~FLAG_UPDATE;
    flags|=FLAG_PRESSED;
    if(options&REALSLIDER_VERTICAL){
      h=event->win_y-dragPoint;
      travel=hh-headSize;
      if(h<yy) h=yy;
      if(h>yy+travel) h=yy+travel;
      if(h!=headPos){
        FXMINMAX(lo,hi,headPos,h);
        headPos=h;
        update(border,lo-1,width-(border<<1),hi+headSize+2-lo);
        }
      if(travel>0)
        p=range[0]+((range[1]-range[0])*(yy+travel-h))/travel;
      else
        p=range[0];
      }
    else{
      h=event->win_x-dragPoint;
      travel=ww-headSize;
      if(h<xx) h=xx;
      if(h>xx+travel) h=xx+travel;
      if(h!=headPos){
        FXMINMAX(lo,hi,headPos,h);
        headPos=h;
        update(lo-1,border,hi+headSize+2-lo,height-(border<<1));
        }
      if(travel>0)
        p=range[0]+((range[1]-range[0])*(h-xx))/travel;
      else
        p=range[0];
      }
    if(0.0<gran) p=gran*Math::round(p/gran);
    if(p<range[0]) p=range[0];
    if(p>range[1]) p=range[1];
    if(p!=pos){
      pos=p;
      flags|=FLAG_CHANGED;
      if(target) target->tryHandle(this,FXSEL(SEL_CHANGED,message),(void*)&pos);
      }
    return 1;
    }
  return 0;
  }


// Released middle button
long FXRealSlider::onMiddleBtnRelease(FXObject*,FXSelector,void* ptr){
  FXuint flgs=flags;
  if(isEnabled()){
    ungrab();
    getApp()->removeTimeout(this,ID_AUTOSLIDE);
    flags&=~FLAG_PRESSED;
    flags&=~FLAG_CHANGED;
    flags|=FLAG_UPDATE;
    setValue(pos);                         // Hop to exact position
    if(target && target->tryHandle(this,FXSEL(SEL_MIDDLEBUTTONRELEASE,message),ptr)) return 1;
    if(flgs&FLAG_CHANGED){
      if(target) target->tryHandle(this,FXSEL(SEL_COMMAND,message),(void*)&pos);
      }
    return 1;
    }
  return 0;
  }


// Mouse wheel
long FXRealSlider::onMouseWheel(FXObject*,FXSelector,void* ptr){
  FXEvent *event=(FXEvent*)ptr;
  FXdouble p=pos+incr*(event->code/120);
  if(p<range[0]) p=range[0];
  if(p>range[1]) p=range[1];
  if(pos!=p){
    setValue(p);
    if(target) target->tryHandle(this,FXSEL(SEL_COMMAND,message),(void*)&pos);
    }
  return 1;
  }


// Keyboard press
long FXRealSlider::onKeyPress(FXObject*,FXSelector,void* ptr){
  FXEvent* event=(FXEvent*)ptr;
  if(isEnabled()){
    if(target && target->tryHandle(this,FXSEL(SEL_KEYPRESS,message),ptr)) return 1;
    switch(event->code){
      case KEY_Left:
      case KEY_KP_Left:
        if(!(options&REALSLIDER_VERTICAL)) goto dec;
        break;
      case KEY_Right:
      case KEY_KP_Right:
        if(!(options&REALSLIDER_VERTICAL)) goto inc;
        break;
      case KEY_Up:
      case KEY_KP_Up:
        if(options&REALSLIDER_VERTICAL) goto inc;
        break;
      case KEY_Down:
      case KEY_KP_Down:
        if(options&REALSLIDER_VERTICAL) goto dec;
        break;
      case KEY_plus:
      case KEY_KP_Add:
inc:    setValue(pos+incr,true);
        return 1;
      case KEY_minus:
      case KEY_KP_Subtract:
dec:    setValue(pos-incr,true);
        return 1;
      }
    }
  return 0;
  }


// Keyboard release
long FXRealSlider::onKeyRelease(FXObject*,FXSelector,void* ptr){
  FXEvent* event=(FXEvent*)ptr;
  if(isEnabled()){
    if(target && target->tryHandle(this,FXSEL(SEL_KEYRELEASE,message),ptr)) return 1;
    switch(event->code){
      case KEY_Left:
      case KEY_KP_Left:
      case KEY_Right:
      case KEY_KP_Right:
        if(!(options&REALSLIDER_VERTICAL)) return 1;
        break;
      case KEY_Up:
      case KEY_KP_Up:
      case KEY_Down:
      case KEY_KP_Down:
        if(options&REALSLIDER_VERTICAL) return 1;
        break;
      case KEY_plus:
      case KEY_KP_Add:
      case KEY_KP_Subtract:
      case KEY_minus:
        return 1;
      }
    }
  return 0;
  }


// The widget lost the grab for some reason
long FXRealSlider::onUngrabbed(FXObject* sender,FXSelector sel,void* ptr){
  FXFrame::onUngrabbed(sender,sel,ptr);
  getApp()->removeTimeout(this,ID_AUTOSLIDE);
  flags&=~FLAG_PRESSED;
  flags&=~FLAG_CHANGED;
  flags|=FLAG_UPDATE;
  return 1;
  }


// Automatically move slider while holding down mouse
long FXRealSlider::onAutoSlide(FXObject*,FXSelector,void* ptr){
  FXint dir=(FXint)(FXival)ptr;
  FXdouble p=pos+incr*dir;
  if(p<=range[0]){
    p=range[0];
    }
  else if(p>=range[1]){
    p=range[1];
    }
  else{
    getApp()->addTimeout(this,ID_AUTOSLIDE,getApp()->getScrollSpeed(),(void*)(FXival)dir);
    }
  if(p!=pos){
    setValue(p);
    flags|=FLAG_CHANGED;
    if(target) target->tryHandle(this,FXSEL(SEL_CHANGED,message),(void*)&pos);
    return 1;
    }
  return 0;
  }


// Draw horizontal ticks
void FXRealSlider::drawHorzTicks(FXDCWindow& dc,FXint,FXint y,FXint,FXint){
  FXdouble interval=range[1]-range[0];
  FXint travel,offset,p;
  FXdouble v,d;
  if(0.0<interval){
    d=delta;
    if(d<=0.0) d=incr;
    dc.setForeground(FXRGB(0,0,0));
    travel=width-(border<<1)-padleft-padright-headSize-4;
    offset=border+padleft+2+headSize/2;
    for(v=range[0]; v<=range[1]; v+=d){
      p=offset+(FXint)(0.5+((travel*(v-range[0]))/interval));
      dc.fillRectangle(p,y,1,TICKSIZE);
      }
    }
  }


// Draw vertical ticks
void FXRealSlider::drawVertTicks(FXDCWindow& dc,FXint x,FXint,FXint,FXint){
  FXdouble interval=range[1]-range[0];
  FXint travel,offset,p;
  FXdouble v,d;
  if(0.0<interval){
    d=delta;
    if(d<=0.0) d=incr;
    dc.setForeground(FXRGB(0,0,0));
    travel=height-(border<<1)-padtop-padbottom-headSize-4;
    offset=height-border-padbottom-2-headSize/2;
    for(v=range[0]; v<=range[1]; v+=d){
      p=offset-(FXint)(0.5+((travel*(v-range[0]))/interval));
      dc.fillRectangle(x,p,TICKSIZE,1);
      }
    }
  }


// Draw slider head
void FXRealSlider::drawSliderHead(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h){
  FXint m;
  dc.setForeground(baseColor);
  dc.fillRectangle(x,y,w,h);
  if(options&REALSLIDER_VERTICAL){
    m=(h>>1);
    if(options&REALSLIDER_ARROW_LEFT){
      dc.setForeground(hiliteColor);
      dc.drawLine(x+m,y,x+w-1,y);
      dc.drawLine(x,y+m,x+m,y);
      dc.setForeground(shadowColor);
      dc.drawLine(x+1,y+h-m-1,x+m+1,y+h-1);
      dc.drawLine(x+m,y+h-2,x+w-1,y+h-2);
      dc.drawLine(x+w-2,y+1,x+w-2,y+h-1);
      dc.setForeground(borderColor);
      dc.drawLine(x,y+h-m-1,x+m,y+h-1);
      dc.drawLine(x+w-1,y+h-1,x+w-1,y);
      dc.fillRectangle(x+m,y+h-1,w-m,1);
      }
    else if(options&REALSLIDER_ARROW_RIGHT){
      dc.setForeground(hiliteColor);
      dc.drawLine(x,y,x+w-m-1,y);
      dc.drawLine(x,y+1,x,y+h-1);
      dc.drawLine(x+w-1,y+m,x+w-m-1,y);
#ifdef WIN32
      dc.setForeground(shadowColor);
      dc.drawLine(x+w-1,y+h-m-2,x+w-m-2,y+h-1);
      dc.drawLine(x+1,y+h-2,x+w-m-1,y+h-2);
      dc.setForeground(borderColor);
      dc.drawLine(x+w,y+h-m-2,x+w-m-1,y+h-1);
      dc.drawLine(x,y+h-1,x+w-m-1,y+h-1);
#else
      dc.setForeground(shadowColor);
      dc.drawLine(x+w-2,y+h-m-1,x+w-m-2,y+h-1);
      dc.drawLine(x+1,y+h-2,x+w-m-1,y+h-2);
      dc.setForeground(borderColor);
      dc.drawLine(x+w-1,y+h-m-1,x+w-m-1,y+h-1);
      dc.drawLine(x,y+h-1,x+w-m-1,y+h-1);
#endif
      }
    else if(options&REALSLIDER_INSIDE_BAR){
      drawDoubleRaisedRectangle(dc,x,y,w,h);
      dc.setForeground(shadowColor);
      dc.drawLine(x+1,y+m-1,x+w-2,y+m-1);
      dc.setForeground(hiliteColor);
      dc.drawLine(x+1,y+m,x+w-2,y+m);
      }
    else{
      drawDoubleRaisedRectangle(dc,x,y,w,h);
      }
    }
  else{
    m=(w>>1);
    if(options&REALSLIDER_ARROW_UP){
      dc.setForeground(hiliteColor);
      dc.drawLine(x,y+m,x+m,y);
      dc.drawLine(x,y+m,x,y+h-1);
      dc.setForeground(shadowColor);
      dc.drawLine(x+w-1,y+m+1,x+w-m-1,y+1);
      dc.drawLine(x+w-2,y+m+1,x+w-2,y+h-1);
      dc.drawLine(x+1,y+h-2,x+w-2,y+h-2);
      dc.setForeground(borderColor);
      dc.drawLine(x+w-1,y+m,x+w-m-1,y);
      dc.drawLine(x+w-1,y+m,x+w-1,y+h-1);
      dc.fillRectangle(x,y+h-1,w,1);
      }
    else if(options&REALSLIDER_ARROW_DOWN){
      dc.setForeground(hiliteColor);
      dc.drawLine(x,y,x+w-1,y);
      dc.drawLine(x,y+1,x,y+h-m-1);
      dc.drawLine(x,y+h-m-1,x+m,y+h-1);
      dc.setForeground(shadowColor);
      dc.drawLine(x+w-2,y+1,x+w-2,y+h-m-1);
      dc.drawLine(x+w-1,y+h-m-2,x+w-m-1,y+h-2);
      dc.setForeground(borderColor);
      dc.drawLine(x+w-1,y+h-m-1,x+w-m-1,y+h-1);
      dc.fillRectangle(x+w-1,y,1,h-m);
      }
    else if(options&REALSLIDER_INSIDE_BAR){
      drawDoubleRaisedRectangle(dc,x,y,w,h);
      dc.setForeground(shadowColor);
      dc.drawLine(x+m-1,y+1,x+m-1,y+h-2);
      dc.setForeground(hiliteColor);
      dc.drawLine(x+m,y+1,x+m,y+h-1);
      }
    else{
      drawDoubleRaisedRectangle(dc,x,y,w,h);
      }
    }
  }


// Handle repaint
long FXRealSlider::onPaint(FXObject*,FXSelector,void* ptr){
  FXDCWindow dc(this,(FXEvent*)ptr);
  FXint tx,ty,hhs=headSize/2;
  FXint xx,yy,ww,hh;

  // Repaint background
  dc.setForeground(backColor);
  dc.fillRectangle(0,0,width,height);

  // Repaint border
  drawFrame(dc,0,0,width,height);

  // Slot placement
  xx=border+padleft;
  yy=border+padtop;
  ww=width-(border<<1)-padleft-padright;
  hh=height-(border<<1)-padtop-padbottom;

  // Draw the slot
  if(options&REALSLIDER_VERTICAL){

    // Adjust slot placement for tickmarks
    if(options&REALSLIDER_TICKS_LEFT){ xx+=TICKSIZE; ww-=TICKSIZE; }
    if(options&REALSLIDER_TICKS_RIGHT){ ww-=TICKSIZE; }

    // Draw slider
    if(options&REALSLIDER_INSIDE_BAR){
      drawDoubleSunkenRectangle(dc,xx,yy,ww,hh);
      dc.setStipple(STIPPLE_GRAY);
      dc.setForeground(slotColor);
      dc.setBackground(baseColor);
      dc.setFillStyle(FILL_OPAQUESTIPPLED);
      dc.fillRectangle(xx+2,yy+2,ww-4,hh-4);
      dc.setFillStyle(FILL_SOLID);
      if(options&REALSLIDER_TICKS_LEFT) drawVertTicks(dc,border+padleft,yy,ww,hh);
      if(options&REALSLIDER_TICKS_RIGHT) drawVertTicks(dc,width-padright-border-TICKSIZE,yy,ww,hh);
      if(isEnabled()) drawSliderHead(dc,xx+2,headPos,ww-4,headSize);
      }
    else{
      if(options&REALSLIDER_ARROW_LEFT) tx=xx+hhs+(ww-slotSize-hhs)/2;
      else if(options&REALSLIDER_ARROW_RIGHT) tx=xx+(ww-slotSize-hhs)/2;
      else tx=xx+(ww-slotSize)/2;
      drawDoubleSunkenRectangle(dc,tx,yy,slotSize,hh);
      dc.setForeground(slotColor);
      dc.fillRectangle(tx+2,yy+2,slotSize-4,hh-4);
      if(options&REALSLIDER_TICKS_LEFT) drawVertTicks(dc,border+padleft,yy,ww,hh);
      if(options&REALSLIDER_TICKS_RIGHT) drawVertTicks(dc,width-padright-border-TICKSIZE,yy,ww,hh);
      if(isEnabled()) drawSliderHead(dc,xx,headPos,ww,headSize);
      }
    }
  else{

    // Adjust slot placement for tickmarks
    if(options&REALSLIDER_TICKS_TOP){ yy+=TICKSIZE; hh-=TICKSIZE; }
    if(options&REALSLIDER_TICKS_BOTTOM){ hh-=TICKSIZE; }

    // Draw slider
    if(options&REALSLIDER_INSIDE_BAR){
      drawDoubleSunkenRectangle(dc,xx,yy,ww,hh);
      dc.setForeground(slotColor);
      dc.setStipple(STIPPLE_GRAY);
      dc.setForeground(slotColor);
      dc.setBackground(baseColor);
      dc.setFillStyle(FILL_OPAQUESTIPPLED);
      dc.fillRectangle(xx+2,yy+2,ww-4,hh-4);
      dc.setFillStyle(FILL_SOLID);
      if(options&REALSLIDER_TICKS_TOP) drawHorzTicks(dc,xx,border+padtop,ww,hh);
      if(options&REALSLIDER_TICKS_BOTTOM) drawHorzTicks(dc,xx,height-border-padbottom-TICKSIZE,ww,hh);
      if(isEnabled()) drawSliderHead(dc,headPos,yy+2,headSize,hh-4);
      }
    else{
      if(options&REALSLIDER_ARROW_UP) ty=yy+hhs+(hh-slotSize-hhs)/2;
      else if(options&REALSLIDER_ARROW_DOWN) ty=yy+(hh-slotSize-hhs)/2;
      else ty=yy+(hh-slotSize)/2;
      drawDoubleSunkenRectangle(dc,xx,ty,ww,slotSize);
      dc.setForeground(slotColor);
      dc.fillRectangle(xx+2,ty+2,ww-4,slotSize-4);
      if(options&REALSLIDER_TICKS_TOP) drawHorzTicks(dc,xx,border+padtop,ww,hh);
      if(options&REALSLIDER_TICKS_BOTTOM) drawHorzTicks(dc,xx,height-border-padbottom-TICKSIZE,ww,hh);
      if(isEnabled()) drawSliderHead(dc,headPos,yy,headSize,hh);
      }
    }
  return 1;
  }


// Set slider range; this also revalidates the position,
// and possibly moves the head [even if the position was still OK,
// the head might still have to be moved to the exact position].
void FXRealSlider::setRange(FXdouble lo,FXdouble hi,FXbool notify){
  if(lo>hi){ fxerror("%s::setRange: trying to set negative range.\n",getClassName()); }
  if(range[0]!=lo || range[1]!=hi){
    if(options&(REALSLIDER_TICKS_TOP|REALSLIDER_TICKS_BOTTOM)) update();
    range[0]=lo;
    range[1]=hi;
    setValue(pos,notify);
    }
  }


// Set position; this should always cause the head to reflect
// the exact [discrete] value representing pos, even if several
// head positions may represent the same position!
// Also, the minimal amount is repainted, as one sometimes as very
// large/wide sliders.
void FXRealSlider::setValue(FXdouble p,FXbool notify){
  FXdouble interval=range[1]-range[0];
  FXint travel,lo,hi,h;
  if(p<range[0]) p=range[0];
  if(p>range[1]) p=range[1];
  if(options&REALSLIDER_VERTICAL){
    travel=height-(border<<1)-padtop-padbottom-headSize-4;
    h=height-border-padbottom-2-headSize;
    if(0<interval) h-=(FXint)(0.5+((travel*(p-range[0]))/interval));
    if(h!=headPos){
      FXMINMAX(lo,hi,headPos,h);
      headPos=h;
      update(border,lo-1,width-(border<<1),hi+headSize+2-lo);
      }
    }
  else{
    travel=width-(border<<1)-padleft-padright-headSize-4;
    h=border+padleft+2;
    if(0<interval) h+=(FXint)(0.5+((travel*(p-range[0]))/interval));
    if(h!=headPos){
      FXMINMAX(lo,hi,headPos,h);
      headPos=h;
      update(lo-1,border,hi+headSize+2-lo,height-(border<<1));
      }
    }
  if(pos!=p){
    pos=p;
    if(notify && target){target->tryHandle(this,FXSEL(SEL_COMMAND,message),(void*)&pos);}
    }
  }


// Get slider options
FXuint FXRealSlider::getSliderStyle() const {
  return (options&REALSLIDER_MASK);
  }


// Set slider options
void FXRealSlider::setSliderStyle(FXuint style){
  FXuint opts=(options&~REALSLIDER_MASK) | (style&REALSLIDER_MASK);
  if(options!=opts){
    headSize=(opts&REALSLIDER_INSIDE_BAR)?HEADINSIDEBAR:HEADOVERHANGING;
    options=opts;
    recalc();
    update();
    }
  }


// Set head size
void FXRealSlider::setHeadSize(FXint hs){
  if(headSize!=hs){
    headSize=hs;
    recalc();
    update();
    }
  }


// Set slot size
void FXRealSlider::setSlotSize(FXint bs){
  if(slotSize!=bs){
    slotSize=bs;
    recalc();
    update();
    }
  }


// Set slider increment
void FXRealSlider::setIncrement(FXdouble inc){
  if(inc<=0.0){ fxerror("%s::setIncrement: negative or zero increment specified.\n",getClassName()); }
  incr=inc;
  }


// Change slider granularity
void FXRealSlider::setGranularity(FXdouble gr){
  if(gr<0.0){ fxerror("%s::setGranularity: negative granularity specified.\n",getClassName()); }
  gran=gr;
  }


// Set slot color
void FXRealSlider::setSlotColor(FXColor clr){
  if(slotColor!=clr){
    slotColor=clr;
    update();
    }
  }


// Change the delta between ticks
void FXRealSlider::setTickDelta(FXdouble dist){
  if(dist<0.0) dist=0.0;
  if(delta!=dist){
    if(options&(REALSLIDER_TICKS_TOP|REALSLIDER_TICKS_BOTTOM)) update();
    delta=dist;
    }
  }


// Save object to stream
void FXRealSlider::save(FXStream& store) const {
  FXFrame::save(store);
  store << headSize;
  store << slotSize;
  store << slotColor;
  store << range[0];
  store << range[1];
  store << delta;
  store << incr;
  store << gran;
  store << pos;
  store << help;
  store << tip;
  }


// Load object from stream
void FXRealSlider::load(FXStream& store){
  FXFrame::load(store);
  store >> headSize;
  store >> slotSize;
  store >> slotColor;
  store >> range[0];
  store >> range[1];
  store >> delta;
  store >> incr;
  store >> gran;
  store >> pos;
  store >> help;
  store >> tip;
  }


// Delete
FXRealSlider::~FXRealSlider(){
  getApp()->removeTimeout(this,ID_AUTOSLIDE);
  }

}
