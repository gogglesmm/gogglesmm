/********************************************************************************
*                                                                               *
*                      S w i t c h   P a n e l   C l a s s                      *
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
#include "FXSwitcher.h"


/*
  Notes:
  - We do not look at LAYOUT_FIX_WIDTH and LAYOUT_FIX_HEIGHT because
    FXSwitcher resizes its children anyway and doing this would cause
    the size to grow instead of shrinkwrap to the biggest child.
  - Thanks to Charles Warren for the suggestion for the collapse
    modes.
  - The current panel number is adjusted if the number of panels changes.
*/


// Switcher styles
#define SWITCHER_MASK (SWITCHER_HCOLLAPSE|SWITCHER_VCOLLAPSE)

using namespace FX;

/*******************************************************************************/

namespace FX {

// Map
FXDEFMAP(FXSwitcher) FXSwitcherMap[]={
  FXMAPFUNC(SEL_PAINT,0,FXSwitcher::onPaint),
  FXMAPFUNCS(SEL_UPDATE,FXSwitcher::ID_OPEN_FIRST,FXSwitcher::ID_OPEN_LAST,FXSwitcher::onUpdOpen),
  FXMAPFUNC(SEL_COMMAND,FXSwitcher::ID_SETVALUE,FXSwitcher::onCmdSetValue),
  FXMAPFUNC(SEL_COMMAND,FXSwitcher::ID_SETINTVALUE,FXSwitcher::onCmdSetIntValue),
  FXMAPFUNC(SEL_COMMAND,FXSwitcher::ID_GETINTVALUE,FXSwitcher::onCmdGetIntValue),
  FXMAPFUNCS(SEL_COMMAND,FXSwitcher::ID_OPEN_FIRST,FXSwitcher::ID_OPEN_LAST,FXSwitcher::onCmdOpen),
  };


// Object implementation
FXIMPLEMENT(FXSwitcher,FXPacker,FXSwitcherMap,ARRAYNUMBER(FXSwitcherMap))


// Serialization
FXSwitcher::FXSwitcher():current(-1){
  }


// Make a switcher window
FXSwitcher::FXSwitcher(FXComposite *p,FXuint opts,FXint x,FXint y,FXint w,FXint h,FXint pl,FXint pr,FXint pt,FXint pb):FXPacker(p,opts,x,y,w,h,pl,pr,pt,pb,0,0),current(-1){
  }


// Handle repaint
long FXSwitcher::onPaint(FXObject*,FXSelector,void* ptr){
  FXEvent *ev=(FXEvent*)ptr;
  FXDCWindow dc(this,ev);
  dc.setForeground(backColor);
  dc.fillRectangle(ev->rect.x,ev->rect.y,ev->rect.w,ev->rect.h);
  drawFrame(dc,0,0,width,height);
  return 1;
  }


// Update value from a message
long FXSwitcher::onCmdSetValue(FXObject*,FXSelector,void* ptr){
  setCurrent((FXint)(FXuval)ptr);
  return 1;
  }


// Update value from a message
long FXSwitcher::onCmdSetIntValue(FXObject*,FXSelector,void* ptr){
  setCurrent(*((FXint*)ptr));
  return 1;
  }


// Obtain value from text field
long FXSwitcher::onCmdGetIntValue(FXObject*,FXSelector,void* ptr){
  *((FXint*)ptr)=getCurrent();
  return 1;
  }


// Bring nth to the top
long FXSwitcher::onCmdOpen(FXObject*,FXSelector sel,void*){
  setCurrent(FXSELID(sel)-ID_OPEN_FIRST,true);
  return 1;
  }


// Update the nth button
long FXSwitcher::onUpdOpen(FXObject* sender,FXSelector sel,void* ptr){
  sender->handle(this,((FXSELID(sel)-ID_OPEN_FIRST)==current) ? FXSEL(SEL_COMMAND,ID_CHECK) : FXSEL(SEL_COMMAND,ID_UNCHECK),ptr);
  return 1;
  }


// Get maximum child width
FXint FXSwitcher::getDefaultWidth(){
  FXWindow* child;
  FXint i,w,wmax=0,wcur=0;
  for(i=0,child=getFirst(); child; child=child->getNext(),i++){
    if(i==current) wcur=child->getDefaultWidth();
    if(wmax<(w=child->getDefaultWidth())) wmax=w;
    }
  if(options&SWITCHER_HCOLLAPSE) wmax=wcur;
  return padleft+padright+(border<<1)+wmax;
  }


// Get maximum child height
FXint FXSwitcher::getDefaultHeight(){
  FXWindow* child;
  FXint i,h,hmax=0,hcur=0;
  for(i=0,child=getFirst(); child; child=child->getNext(),i++){
    if(i==current) hcur=child->getDefaultHeight();
    if(hmax<(h=child->getDefaultHeight())) hmax=h;
    }
  if(options&SWITCHER_VCOLLAPSE) hmax=hcur;
  return padtop+padbottom+(border<<1)+hmax;
  }


// Recalculate layout
void FXSwitcher::layout(){
  FXWindow *child;
  FXint i,x,y,w,h;
  x=border+padleft;
  y=border+padtop;
  w=width-padright-padleft-(border<<1);
  h=height-padtop-padbottom-(border<<1);
  if(current<0) current=0;
  if(current>=numChildren()) current=numChildren()-1;
  for(child=getFirst(),i=0; child; child=child->getNext(),i++){
    child->position(x,y,w,h);
    if(i==current){
      child->show();
      }
    else{
      child->hide();
      }
    }
  flags&=~FLAG_DIRTY;
  }


// Set current subwindow
void FXSwitcher::setCurrent(FXint panel,FXbool notify){
  if(current!=panel && 0<=panel && panel<numChildren()){
    current=panel;
    recalc();
    if(notify && target){ target->tryHandle(this,FXSEL(SEL_COMMAND,message),(void*)(FXival)current); }
    }
  }


// Set switcher style
void FXSwitcher::setSwitcherStyle(FXuint style){
  FXuint opts=(options&~SWITCHER_MASK) | (style&SWITCHER_MASK);
  if(options!=opts){
    options=opts;
    recalc();
    }
  }


// Get switcher style
FXuint FXSwitcher::getSwitcherStyle() const {
  return (options&SWITCHER_MASK);
  }


// Save object to stream
void FXSwitcher::save(FXStream& store) const {
  FXPacker::save(store);
  store << current;
  }


// Load object from stream
void FXSwitcher::load(FXStream& store){
  FXPacker::load(store);
  store >> current;
  }

}
