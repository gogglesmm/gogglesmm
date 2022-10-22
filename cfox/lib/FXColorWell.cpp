/********************************************************************************
*                                                                               *
*                         C o l o r W e l l   C l a s s                         *
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
#include "FXElement.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXColors.h"
#include "FXSize.h"
#include "FXPoint.h"
#include "FXRectangle.h"
#include "FXObject.h"
#include "FXStringDictionary.h"
#include "FXSettings.h"
#include "FXRegistry.h"
#include "FXEvent.h"
#include "FXWindow.h"
#include "FXDCWindow.h"
#include "FXApp.h"
#include "FXLabel.h"
#include "FXColors.h"
#include "FXColorWell.h"
#include "FXColorSelector.h"
#include "FXColorDialog.h"

/*
  Notes:
  - FXColorWell now observes border styles, but padding is outside of the well.
  - Focus rectangle now drawn inside the color swatch part of the well.
  - Possibly have no border or padding; so you can now tightly pack wells together,
    for example, inside FXMatrix.
  - Probably need to use this feature in FXColorSelector (large collection of
    custom colors in block under panel).
  - Also, FXColorWell has default size that can be changed; minimum is 3 pixels,
    1 pixel for color, 2 for focus rectangle.
  - Single-click should send SEL_COMMAND to target.
  - Perhaps change of color should send SEL_CHANGED.
  - Do not start drag operation unless moving a little bit.
  - Drive from keyboard.
  - Yes, you can now drag a color into a text widget, or drag the name of
    a color into the well, as well as pasting a color into a text widget or
    vice versa!
*/

using namespace FX;

/*******************************************************************************/

namespace FX {

// Map
FXDEFMAP(FXColorWell) FXColorWellMap[]={
  FXMAPFUNC(SEL_PAINT,0,FXColorWell::onPaint),
  FXMAPFUNC(SEL_MOTION,0,FXColorWell::onMotion),
  FXMAPFUNC(SEL_DRAGGED,0,FXColorWell::onDragged),
  FXMAPFUNC(SEL_DND_MOTION,0,FXColorWell::onDNDMotion),
  FXMAPFUNC(SEL_DND_ENTER,0,FXColorWell::onDNDEnter),
  FXMAPFUNC(SEL_DND_LEAVE,0,FXColorWell::onDNDLeave),
  FXMAPFUNC(SEL_DND_DROP,0,FXColorWell::onDNDDrop),
  FXMAPFUNC(SEL_DND_REQUEST,0,FXColorWell::onDNDRequest),
  FXMAPFUNC(SEL_FOCUSIN,0,FXColorWell::onFocusIn),
  FXMAPFUNC(SEL_FOCUSOUT,0,FXColorWell::onFocusOut),
  FXMAPFUNC(SEL_LEFTBUTTONPRESS,0,FXColorWell::onLeftBtnPress),
  FXMAPFUNC(SEL_LEFTBUTTONRELEASE,0,FXColorWell::onLeftBtnRelease),
  FXMAPFUNC(SEL_CLICKED,0,FXColorWell::onClicked),
  FXMAPFUNC(SEL_DOUBLECLICKED,0,FXColorWell::onDoubleClicked),
  FXMAPFUNC(SEL_KEYPRESS,0,FXColorWell::onKeyPress),
  FXMAPFUNC(SEL_KEYRELEASE,0,FXColorWell::onKeyRelease),
  FXMAPFUNC(SEL_UNGRABBED,0,FXColorWell::onUngrabbed),
  FXMAPFUNC(SEL_BEGINDRAG,0,FXColorWell::onBeginDrag),
  FXMAPFUNC(SEL_ENDDRAG,0,FXColorWell::onEndDrag),
  FXMAPFUNC(SEL_QUERY_TIP,0,FXColorWell::onQueryTip),
  FXMAPFUNC(SEL_QUERY_HELP,0,FXColorWell::onQueryHelp),
  FXMAPFUNC(SEL_COMMAND,FXColorWell::ID_SETVALUE,FXColorWell::onCmdSetValue),
  FXMAPFUNC(SEL_COMMAND,FXColorWell::ID_SETINTVALUE,FXColorWell::onCmdSetIntValue),
  FXMAPFUNC(SEL_COMMAND,FXColorWell::ID_GETINTVALUE,FXColorWell::onCmdGetIntValue),
  FXMAPFUNC(SEL_COMMAND,FXColorWell::ID_SETHELPSTRING,FXColorWell::onCmdSetHelp),
  FXMAPFUNC(SEL_COMMAND,FXColorWell::ID_GETHELPSTRING,FXColorWell::onCmdGetHelp),
  FXMAPFUNC(SEL_COMMAND,FXColorWell::ID_SETTIPSTRING,FXColorWell::onCmdSetTip),
  FXMAPFUNC(SEL_COMMAND,FXColorWell::ID_GETTIPSTRING,FXColorWell::onCmdGetTip),
  FXMAPFUNC(SEL_UPDATE,FXColorWell::ID_COLOR,FXColorWell::onUpdColor),
  FXMAPFUNC(SEL_CHANGED,FXColorWell::ID_COLOR,FXColorWell::onChgColor),
  FXMAPFUNC(SEL_COMMAND,FXColorWell::ID_COLOR,FXColorWell::onCmdColor),
  };


// Object implementation
FXIMPLEMENT(FXColorWell,FXFrame,FXColorWellMap,ARRAYNUMBER(FXColorWellMap))



/*******************************************************************************/


// Init
FXColorWell::FXColorWell(){
  wellColor[0]=FXColors::White;
  wellColor[1]=FXColors::White;
  wellSize=12;
  rgba=FXColors::White;
  flags|=FLAG_ENABLED|FLAG_DROPTARGET;
  }


// Make a color well
FXColorWell::FXColorWell(FXComposite* p,FXColor clr,FXObject* tgt,FXSelector sel,FXuint opts,FXint x,FXint y,FXint w,FXint h,FXint pl,FXint pr,FXint pt,FXint pb):FXFrame(p,opts,x,y,w,h,pl,pr,pt,pb){
  flags|=FLAG_ENABLED|FLAG_DROPTARGET;
  wellColor[0]=blendOverWhite(clr);
  wellColor[1]=blendOverBlack(clr);
  wellSize=12;
  rgba=clr;
  target=tgt;
  message=sel;
  }


// If window can have focus
FXbool FXColorWell::canFocus() const { return true; }


// Into focus chain
void FXColorWell::setFocus(){
  FXFrame::setFocus();
  setDefault(true);
  }


// Out of focus chain
void FXColorWell::killFocus(){
  FXFrame::killFocus();
  setDefault(maybe);
  }


// Get default size
FXint FXColorWell::getDefaultWidth(){
  return wellSize+padleft+padright+(border<<1);
  }


FXint FXColorWell::getDefaultHeight(){
  return wellSize+padtop+padbottom+(border<<1);
  }


// Handle repaint
long FXColorWell::onPaint(FXObject*,FXSelector,void* ptr){
  FXDCWindow dc(this,(FXEvent*)ptr);
  FXPoint points[3];
  dc.setForeground(backColor);
  dc.fillRectangle(0,0,width,padtop);
  dc.fillRectangle(0,padtop,padleft,height-padtop-padbottom);
  dc.fillRectangle(width-padright,padtop,padright,height-padtop-padbottom);
  dc.fillRectangle(0,height-padbottom,width,padbottom);
  drawFrame(dc,padleft,padtop,width-padright-padleft,height-padtop-padbottom);
  dc.setForeground(wellColor[0]);
  points[0].x=points[1].x=padleft+border;
  points[2].x=width-padright-border;
  points[0].y=points[2].y=padtop+border;
  points[1].y=height-padbottom-border;
  dc.fillPolygon(points,3);
  dc.setForeground(wellColor[1]);
  points[0].x=padleft+border;
  points[1].x=points[2].x=width-padright-border;
  points[0].y=points[1].y=height-padbottom-border;
  points[2].y=padtop+border;
  dc.fillPolygon(points,3);
  if(hasFocus()){
    dc.drawFocusRectangle(padleft+border+1,padtop+border+1,width-padright-padleft-(border<<1)-2,height-padbottom-padtop-(border<<1)-2);
    }
  return 1;
  }


// Gained focus
long FXColorWell::onFocusIn(FXObject* sender,FXSelector sel,void* ptr){
  FXFrame::onFocusIn(sender,sel,ptr);
  update();
  return 1;
  }


// Lost focus
long FXColorWell::onFocusOut(FXObject* sender,FXSelector sel,void* ptr){
  FXFrame::onFocusOut(sender,sel,ptr);
  update();
  return 1;
  }


// Dragging something over well; save old color and block GUI updates
long FXColorWell::onDNDEnter(FXObject* sender,FXSelector sel,void* ptr){
  if(FXFrame::onDNDEnter(sender,sel,ptr)) return 1;
  flags&=~FLAG_UPDATE;
  return 1;
  }


// Dragged out of well, so restore old color and reenable GUI updates
long FXColorWell::onDNDLeave(FXObject* sender,FXSelector sel,void* ptr){
  if(FXFrame::onDNDLeave(sender,sel,ptr)) return 1;
  flags|=FLAG_UPDATE;
  return 1;
  }


// Handle drag-and-drop motion
long FXColorWell::onDNDMotion(FXObject* sender,FXSelector sel,void* ptr){

  // Handle base class first
  if(FXFrame::onDNDMotion(sender,sel,ptr)) return 1;

  // No more messages while inside
  setDragRectangle(0,0,width,height,false);

  // Is it a color being dropped?
  if(offeredDNDType(FROM_DRAGNDROP,colorType)){
    acceptDrop(DRAG_ACCEPT);
    return 1;
    }

  // Is it a name of a color being dropped?
  if(offeredDNDType(FROM_DRAGNDROP,textType)){
    acceptDrop(DRAG_ACCEPT);
    return 1;
    }
  return 0;
  }


// Handle drag-and-drop drop
long FXColorWell::onDNDDrop(FXObject* sender,FXSelector sel,void* ptr){
  FXuchar *pointer;
  FXuint   length;
  FXColor  color;

  // Enable updating
  flags|=FLAG_UPDATE;

  // Try handling it in base class first
  if(FXFrame::onDNDDrop(sender,sel,ptr)) return 1;

  // Try and obtain the color first
  if(getDNDData(FROM_DRAGNDROP,colorType,pointer,length)){
    color=FXRGBA((((FXushort*)pointer)[0]+128)/257,(((FXushort*)pointer)[1]+128)/257,(((FXushort*)pointer)[2]+128)/257,(((FXushort*)pointer)[3]+128)/257);
    freeElms(pointer);
    setRGBA(color,true);
    return 1;
    }

  // Maybe its the name of a color
  if(getDNDData(FROM_DRAGNDROP,textType,pointer,length)){
    resizeElms(pointer,length+1); pointer[length]='\0';
    color=colorFromName((const FXchar*)pointer);
    freeElms(pointer);

    // Accept the drop only if it was a valid color name
    if(color!=FXRGBA(0,0,0,0)){
      setRGBA(color,true);
      return 1;
      }
    }

  return 0;
  }


// Service requested DND data
long FXColorWell::onDNDRequest(FXObject* sender,FXSelector sel,void* ptr){
  FXEvent *event=(FXEvent*)ptr;

  // Try handling it in base class first
  if(FXFrame::onDNDRequest(sender,sel,ptr)) return 1;

  // Requested as a color
  if(event->target==colorType){
    FXushort *color;
    allocElms(color,4);
    color[0]=257*FXREDVAL(rgba);
    color[1]=257*FXGREENVAL(rgba);
    color[2]=257*FXBLUEVAL(rgba);
    color[3]=257*FXALPHAVAL(rgba);
    setDNDData(FROM_DRAGNDROP,colorType,(FXuchar*)color,sizeof(FXushort)*4);
    return 1;
    }

  // Requested as a color name
  if(event->target==textType){
    FXchar *string;
    callocElms(string,50);
    nameFromColor(string,rgba);
    setDNDData(FROM_DRAGNDROP,textType,(FXuchar*)string,strlen(string));
    return 1;
    }
  return 0;
  }


// Start a drag operation
long FXColorWell::onBeginDrag(FXObject* sender,FXSelector sel,void* ptr){
  FXDragType types[2]={colorType,textType};
  if(!FXFrame::onBeginDrag(sender,sel,ptr)){
    beginDrag(types,ARRAYNUMBER(types));
    setDragCursor(getApp()->getDefaultCursor(DEF_SWATCH_CURSOR));
    }
  return 1;
  }


// End drag operation
long FXColorWell::onEndDrag(FXObject* sender,FXSelector sel,void* ptr){
  if(!FXFrame::onEndDrag(sender,sel,ptr)){
    endDrag(didAccept()==DRAG_COPY);
    setDragCursor(getApp()->getDefaultCursor(DEF_ARROW_CURSOR));
    }
  return 1;
  }


// Dragged stuff around
long FXColorWell::onDragged(FXObject* sender,FXSelector sel,void* ptr){
  if(!FXFrame::onDragged(sender,sel,ptr)){
    handleDrag(((FXEvent*)ptr)->root_x,((FXEvent*)ptr)->root_y,DRAG_COPY);
    if(didAccept()==DRAG_COPY){
      setDragCursor(getApp()->getDefaultCursor(DEF_SWATCH_CURSOR));
      }
    else{
      setDragCursor(getApp()->getDefaultCursor(DEF_DNDSTOP_CURSOR));
      }
    }
  return 1;
  }


// Moving
long FXColorWell::onMotion(FXObject*,FXSelector,void* ptr){
  if(flags&FLAG_DODRAG){
    handle(this,FXSEL(SEL_DRAGGED,0),ptr);
    return 1;
    }
  if((flags&FLAG_TRYDRAG) && ((FXEvent*)ptr)->moved){
    if(handle(this,FXSEL(SEL_BEGINDRAG,0),ptr)) flags|=FLAG_DODRAG;
    flags&=~FLAG_TRYDRAG;
    return 1;
    }
  return 0;
  }


// Drag start
long FXColorWell::onLeftBtnPress(FXObject*,FXSelector,void* ptr){
  FXEvent* event=(FXEvent*)ptr;
  flags&=~FLAG_TIP;
  handle(this,FXSEL(SEL_FOCUS_SELF,0),ptr);
  if(isEnabled()){
    grab();
    if(target && target->tryHandle(this,FXSEL(SEL_LEFTBUTTONPRESS,message),ptr)) return 1;
    if(event->click_count==1){
      flags&=~FLAG_UPDATE;
      flags|=FLAG_TRYDRAG;
      }
    }
  return 1;
  }


// Drop
long FXColorWell::onLeftBtnRelease(FXObject*,FXSelector,void* ptr){
  FXEvent* event=(FXEvent*)ptr;
  FXuint flgs=flags;
  if(isEnabled()){
    ungrab();
    flags|=FLAG_UPDATE;
    flags&=~(FLAG_TRYDRAG|FLAG_DODRAG);
    if(target && target->tryHandle(this,FXSEL(SEL_LEFTBUTTONRELEASE,message),ptr)) return 1;
    if(flgs&FLAG_DODRAG){handle(this,FXSEL(SEL_ENDDRAG,0),ptr);}
    if(event->click_count==1){
      handle(this,FXSEL(SEL_CLICKED,0),(void*)(FXuval)rgba);
      if(!event->moved && target) target->tryHandle(this,FXSEL(SEL_COMMAND,message),(void*)(FXuval)rgba);
      }
    else if(event->click_count==2){
      handle(this,FXSEL(SEL_DOUBLECLICKED,0),(void*)(FXuval)rgba);
      }
    return 1;
    }
  return 1;
  }


// Key Press
long FXColorWell::onKeyPress(FXObject*,FXSelector,void* ptr){
  FXEvent* event=(FXEvent*)ptr;
  flags&=~FLAG_TIP;
  if(isEnabled()){
    if(target && target->tryHandle(this,FXSEL(SEL_KEYPRESS,message),ptr)) return 1;
    switch(event->code){
      case KEY_space:
      case KEY_KP_Enter:
      case KEY_Return:
        flags&=~FLAG_UPDATE;
        return 1;
      }
    }
  return 0;
  }


// Key Release
long FXColorWell::onKeyRelease(FXObject*,FXSelector,void* ptr){
  FXEvent* event=(FXEvent*)ptr;
  if(isEnabled()){
    flags|=FLAG_UPDATE;
    if(target && target->tryHandle(this,FXSEL(SEL_KEYRELEASE,message),ptr)) return 1;
    switch(event->code){
      case KEY_space:
        handle(this,FXSEL(SEL_CLICKED,0),(void*)(FXuval)rgba);
        if(target) target->tryHandle(this,FXSEL(SEL_COMMAND,message),(void*)(FXuval)rgba);
        return 1;
      case KEY_KP_Enter:
      case KEY_Return:
        handle(this,FXSEL(SEL_DOUBLECLICKED,0),(void*)(FXuval)rgba);
        return 1;
      }
    }
  return 0;
  }


// Clicked in the well
long FXColorWell::onClicked(FXObject*,FXSelector,void*){
  return target && target->tryHandle(this,FXSEL(SEL_CLICKED,message),(void*)(FXuval)rgba);
  }


// Double clicked in well; normally pops the color dialog
// except when COLORWELL_SOURCEONLY is passed in which case
// editing by the dialog is not allowed (used for wells inside the
// color dialog itself for example).
// The well follows the editing via the dialog; when the dialog
// is closed by cancelling it it will revert to the old color
long FXColorWell::onDoubleClicked(FXObject*,FXSelector,void*){
  if(target && target->tryHandle(this,FXSEL(SEL_DOUBLECLICKED,message),(void*)(FXuval)rgba)) return 1;
  if(isSourceOnly()) return 1;
  FXColorDialog colordialog(this,tr("Color Dialog"));
  FXColor oldcolor=getRGBA();
  colordialog.setTarget(this);
  colordialog.setSelector(ID_COLOR);
  colordialog.setOpaqueOnly(isOpaqueOnly());
  if(!colordialog.execute()){
    setRGBA(oldcolor,true);
    }
  return 1;
  }


// The widget lost the grab for some reason
long FXColorWell::onUngrabbed(FXObject* sender,FXSelector sel,void* ptr){
  FXFrame::onUngrabbed(sender,sel,ptr);
  flags&=~(FLAG_TRYDRAG|FLAG_DODRAG);
  flags|=FLAG_UPDATE;
  endDrag(false);
  return 1;
  }


// Update another Color Well
long FXColorWell::onUpdColor(FXObject* sender,FXSelector,void*){
  sender->handle(this,FXSEL(SEL_COMMAND,ID_SETINTVALUE),(void*)&rgba);
  return 1;
  }


// Change from another Color Well
long FXColorWell::onChgColor(FXObject*,FXSelector,void* ptr){
  flags&=~FLAG_UPDATE;
  setRGBA((FXColor)(FXuval)ptr);
  if(target) target->tryHandle(this,FXSEL(SEL_CHANGED,message),(void*)(FXuval)rgba);
  return 1;
  }


// Command from another Color Well
long FXColorWell::onCmdColor(FXObject*,FXSelector,void* ptr){
  flags|=FLAG_UPDATE;
  setRGBA((FXColor)(FXuval)ptr);
  if(target) target->tryHandle(this,FXSEL(SEL_COMMAND,message),(void*)(FXuval)rgba);
  return 1;
  }


// Set help using a message
long FXColorWell::onCmdSetHelp(FXObject*,FXSelector,void* ptr){
  setHelpText(*((FXString*)ptr));
  return 1;
  }


// Get help using a message
long FXColorWell::onCmdGetHelp(FXObject*,FXSelector,void* ptr){
  *((FXString*)ptr)=getHelpText();
  return 1;
  }


// Set tip using a message
long FXColorWell::onCmdSetTip(FXObject*,FXSelector,void* ptr){
  setTipText(*((FXString*)ptr));
  return 1;
  }


// Get tip using a message
long FXColorWell::onCmdGetTip(FXObject*,FXSelector,void* ptr){
  *((FXString*)ptr)=getTipText();
  return 1;
  }


// We were asked about tip text
long FXColorWell::onQueryTip(FXObject* sender,FXSelector sel,void* ptr){
  if(FXFrame::onQueryTip(sender,sel,ptr)) return 1;
  if((flags&FLAG_TIP) && !tip.empty()){
    sender->handle(this,FXSEL(SEL_COMMAND,ID_SETSTRINGVALUE),(void*)&tip);
    return 1;
    }
  return 0;
  }


// We were asked about status text
long FXColorWell::onQueryHelp(FXObject* sender,FXSelector sel,void* ptr){
  if(FXFrame::onQueryHelp(sender,sel,ptr)) return 1;
  if((flags&FLAG_HELP) && !help.empty()){
    sender->handle(this,FXSEL(SEL_COMMAND,ID_SETSTRINGVALUE),(void*)&help);
    return 1;
    }
  return 0;
  }


// Change RGBA color
void FXColorWell::setRGBA(FXColor clr,FXbool notify){
  if(isOpaqueOnly()) clr|=FXRGBA(0,0,0,255);
  if(clr!=rgba){
    rgba=clr;
    wellColor[0]=blendOverWhite(rgba);
    wellColor[1]=blendOverBlack(rgba);
    update();
    if(notify && target) target->tryHandle(this,FXSEL(SEL_COMMAND,message),(void*)(FXuval)rgba);
    }
  }


// Set color
long FXColorWell::onCmdSetValue(FXObject*,FXSelector,void* ptr){
  setRGBA((FXColor)(FXuval)ptr);
  return 1;
  }


// Update well from a message
long FXColorWell::onCmdSetIntValue(FXObject*,FXSelector,void* ptr){
  setRGBA(*((FXColor*)ptr));
  return 1;
  }


// Obtain value from well
long FXColorWell::onCmdGetIntValue(FXObject*,FXSelector,void* ptr){
  *((FXColor*)ptr)=getRGBA();
  return 1;
  }


// Change minimum well size
void FXColorWell::setWellSise(FXint ws){
  if(ws<3) ws=3;
  if(wellSize!=ws){
    wellSize=ws;
    recalc();
    }
  }


// Return true if only opaque colors allowed
FXbool FXColorWell::isOpaqueOnly() const {
  return (options&COLORWELL_OPAQUEONLY)!=0;
  }


// Change opaque only mode
void FXColorWell::setOpaqueOnly(FXbool opaque){
  if(opaque){
    options|=COLORWELL_OPAQUEONLY;
    setRGBA(rgba);
    }
  else{
    options&=~COLORWELL_OPAQUEONLY;
    }
  }


// Return true if only a source
FXbool FXColorWell::isSourceOnly() const {
  return (options&COLORWELL_SOURCEONLY)!=0;
  }


// Change source only mode
void FXColorWell::setSourceOnly(FXbool srconly){
  options^=((0-srconly)^options)&COLORWELL_SOURCEONLY;
  }


// Save data
void FXColorWell::save(FXStream& store) const {
  FXFrame::save(store);
  store << wellColor[0] << wellColor[1];
  store << rgba;
  store << tip;
  store << help;
  }


// Load data
void FXColorWell::load(FXStream& store){
  FXFrame::load(store);
  store >> wellColor[0] >> wellColor[1];
  store >> rgba;
  store >> tip;
  store >> help;
  }


// Destroy
FXColorWell::~FXColorWell(){
  }

}
