/********************************************************************************
*                                                                               *
*                          P i c k e r   B u t t o n                            *
*                                                                               *
*********************************************************************************
* Copyright (C) 2001,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "FXIcon.h"
#include "FXPicker.h"


/*
  Notes:
  - The SEL_COMMAND is generated when the mouse is released; this
    is done so we won't report SEL_LEFTBUTTONRELEASE to the underlying
    widget without a preceeding SEL_LEFTBUTTONPRESS.
*/

#define TOPIC_KEYBOARD  1009

using namespace FX;

/*******************************************************************************/

namespace FX {


// Map
FXDEFMAP(FXPicker) FXPickerMap[]={
  FXMAPFUNC(SEL_MOTION,0,FXPicker::onMotion),
  FXMAPFUNC(SEL_LEFTBUTTONPRESS,0,FXPicker::onLeftBtnPress),
  FXMAPFUNC(SEL_LEFTBUTTONRELEASE,0,FXPicker::onLeftBtnRelease),
  FXMAPFUNC(SEL_KEYPRESS,0,FXPicker::onKeyPress),
  FXMAPFUNC(SEL_KEYRELEASE,0,FXPicker::onKeyRelease),
  FXMAPFUNC(SEL_KEYPRESS,FXPicker::ID_HOTKEY,FXPicker::onHotKeyPress),
  FXMAPFUNC(SEL_KEYRELEASE,FXPicker::ID_HOTKEY,FXPicker::onHotKeyRelease),
  };


// Object implementation
FXIMPLEMENT(FXPicker,FXButton,FXPickerMap,ARRAYNUMBER(FXPickerMap))


// Deserialization
FXPicker::FXPicker(){
  location.x=0;
  location.y=0;
  picked=false;
  }


// Construct and init
FXPicker::FXPicker(FXComposite* p,const FXString& text,FXIcon* ic,FXObject* tgt,FXSelector sel,FXuint opts,FXint x,FXint y,FXint w,FXint h,FXint pl,FXint pr,FXint pt,FXint pb):FXButton(p,text,ic,tgt,sel,opts,x,y,w,h,pl,pr,pt,pb){
  dragCursor=getApp()->getDefaultCursor(DEF_CROSSHAIR_CURSOR);
  location.x=0;
  location.y=0;
  picked=false;
  }


// Mouse moved
long FXPicker::onMotion(FXObject*,FXSelector,void* ptr){
  if(state==STATE_DOWN && !picked){
    location.x=((FXEvent*)ptr)->root_x;
    location.y=((FXEvent*)ptr)->root_y;
    if(target){ target->tryHandle(this,FXSEL(SEL_CHANGED,message),(void*)&location); }
    return 1;
    }
  return 0;
  }


// Pressed mouse button
long FXPicker::onLeftBtnPress(FXObject*,FXSelector,void* ptr){
  handle(this,FXSEL(SEL_FOCUS_SELF,0),ptr);
  flags&=~FLAG_TIP;
  if(isEnabled() && !(flags&FLAG_PRESSED)){
    flags|=FLAG_PRESSED;
    if(state==STATE_UP){
      grab();
      setState(STATE_DOWN);
      flags&=~FLAG_UPDATE;
      picked=false;
      }
    else{
      picked=true;
      }
    return 1;
    }
  return 0;
  }


// Released mouse button
long FXPicker::onLeftBtnRelease(FXObject*,FXSelector,void*){
  if(isEnabled() && (flags&FLAG_PRESSED)){
    flags&=~FLAG_PRESSED;
    if(state==STATE_DOWN && picked){
      ungrab();
      flags|=FLAG_UPDATE;
      setState(STATE_UP);
      picked=false;
      if(target){ target->tryHandle(this,FXSEL(SEL_COMMAND,message),(void*)&location); }
      }
    return 1;
    }
  return 0;
  }


// Key Press
long FXPicker::onKeyPress(FXObject*,FXSelector,void* ptr){
  flags&=~FLAG_TIP;
  if(isEnabled() && !(flags&FLAG_PRESSED)){
    FXEvent* event=(FXEvent*)ptr;
    FXTRACE((TOPIC_KEYBOARD,"%s::onKeyPress keysym=0x%04x state=%04x\n",getClassName(),event->code,event->state));
    if((event->code==KEY_space || event->code==KEY_KP_Space) || (isDefault() && (event->code==KEY_Return || event->code==KEY_KP_Enter))){
      flags|=FLAG_PRESSED;
      if(state==STATE_UP){
        grab();
        setState(STATE_DOWN);
        flags&=~FLAG_UPDATE;
        picked=false;
        }
      else{
        picked=true;
        }
      return 1;
      }
    }
  return 0;
  }


// Key Release
long FXPicker::onKeyRelease(FXObject*,FXSelector,void* ptr){
  if(isEnabled() && (flags&FLAG_PRESSED)){
    FXEvent* event=(FXEvent*)ptr;
    FXTRACE((TOPIC_KEYBOARD,"%s::onKeyRelease keysym=0x%04x state=%04x\n",getClassName(),event->code,event->state));
    if((event->code==KEY_space || event->code==KEY_KP_Space) || (isDefault() && (event->code==KEY_Return || event->code==KEY_KP_Enter))){
      flags&=~FLAG_PRESSED;
      if(state==STATE_DOWN && picked){
        ungrab();
        flags|=FLAG_UPDATE;
        setState(STATE_UP);
        picked=false;
        if(target){ target->tryHandle(this,FXSEL(SEL_COMMAND,message),(void*)&location); }
        }
      return 1;
      }
    }
  return 0;
  }


// Hot key combination pressed
long FXPicker::onHotKeyPress(FXObject*,FXSelector,void* ptr){
  flags&=~FLAG_TIP;
  handle(this,FXSEL(SEL_FOCUS_SELF,0),ptr);
  if(isEnabled() && !(flags&FLAG_PRESSED)){
    flags|=FLAG_PRESSED;
    if(state==STATE_UP){
      grab();
      setState(STATE_DOWN);
      flags&=~FLAG_UPDATE;
      picked=false;
      }
    else{
      picked=true;
      }
    }
  return 1;
  }


// Hot key combination released
long FXPicker::onHotKeyRelease(FXObject*,FXSelector,void*){
  if(isEnabled() && (flags&FLAG_PRESSED)){
    flags&=~FLAG_PRESSED;
    if(state==STATE_DOWN && picked){
      ungrab();
      flags|=FLAG_UPDATE;
      setState(STATE_UP);
      picked=false;
      if(target){ target->tryHandle(this,FXSEL(SEL_COMMAND,message),(void*)&location); }
      }
    }
  return 1;
  }

}

