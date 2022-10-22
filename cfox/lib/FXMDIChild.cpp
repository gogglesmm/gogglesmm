/********************************************************************************
*                                                                               *
*          M u l t i p l e   D o c u m e n t   C h i l d   W i n d o w          *
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
#include "FXArray.h"
#include "FXHash.h"
#include "FXMutex.h"
#include "FXRunnable.h"
#include "FXAutoThreadStorageKey.h"
#include "FXThread.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXSize.h"
#include "FXPoint.h"
#include "FXRectangle.h"
#include "FXStringDictionary.h"
#include "FXSettings.h"
#include "FXRegistry.h"
#include "FXAccelTable.h"
#include "FXFont.h"
#include "FXEvent.h"
#include "FXWindow.h"
#include "FXDCWindow.h"
#include "FXApp.h"
#include "FXGIFIcon.h"
#include "FXLabel.h"
#include "FXButton.h"
#include "FXMenuButton.h"
#include "FXComposite.h"
#include "FXShell.h"
#include "FXPopup.h"
#include "FXMenuPane.h"
#include "FXScrollBar.h"
#include "FXScrollArea.h"
#include "FXMDIButton.h"
#include "FXMDIChild.h"
#include "FXMDIClient.h"


/*
  Notes:
  - Stacking order changes should be performed by MDIClient!
  - Need options for MDI child decorations (close btn, window menu, min btn, max btn,
    title etc).
  - Iconified version should be fixed size, showing as much of title as feasible
    (tail with ...'s)
  - Initial icon placement on the bottom of the MDIClient somehow...
  - Close v.s. delete messages are not consistent.
*/

#define BORDERWIDTH      4                          // MDI Child border width
#define HANDLESIZE       20                         // Resize handle length
#define MINWIDTH         80                         // Minimum width
#define MINHEIGHT        30                         // Minimum height
#define TITLESPACE       120                        // Width of title when minimized


using namespace FX;

/*******************************************************************************/

namespace FX {

// Map
FXDEFMAP(FXMDIChild) FXMDIChildMap[]={
  FXMAPFUNC(SEL_PAINT,0,FXMDIChild::onPaint),
  FXMAPFUNC(SEL_MOTION,0,FXMDIChild::onMotion),
  FXMAPFUNC(SEL_ENTER,0,FXMDIChild::onEnter),
  FXMAPFUNC(SEL_LEAVE,0,FXMDIChild::onLeave),
  FXMAPFUNC(SEL_FOCUSIN,0,FXMDIChild::onFocusIn),
  FXMAPFUNC(SEL_FOCUSOUT,0,FXMDIChild::onFocusOut),
  FXMAPFUNC(SEL_LEFTBUTTONPRESS,0,FXMDIChild::onLeftBtnPress),
  FXMAPFUNC(SEL_LEFTBUTTONRELEASE,0,FXMDIChild::onLeftBtnRelease),
  FXMAPFUNC(SEL_MIDDLEBUTTONPRESS,0,FXMDIChild::onMiddleBtnPress),
  FXMAPFUNC(SEL_MIDDLEBUTTONRELEASE,0,FXMDIChild::onMiddleBtnRelease),
  FXMAPFUNC(SEL_RIGHTBUTTONPRESS,0,FXMDIChild::onRightBtnPress),
  FXMAPFUNC(SEL_RIGHTBUTTONRELEASE,0,FXMDIChild::onRightBtnRelease),
  FXMAPFUNC(SEL_CLOSE,0,FXMDIChild::onCmdClose),
  FXMAPFUNC(SEL_SELECTED,0,FXMDIChild::onSelected),
  FXMAPFUNC(SEL_DESELECTED,0,FXMDIChild::onDeselected),
  FXMAPFUNC(SEL_FOCUS_SELF,0,FXMDIChild::onFocusSelf),
  FXMAPFUNC(SEL_UPDATE,FXMDIChild::ID_MDI_CLOSE,FXMDIChild::onUpdClose),
  FXMAPFUNC(SEL_UPDATE,FXMDIChild::ID_MDI_MAXIMIZE,FXMDIChild::onUpdMaximize),
  FXMAPFUNC(SEL_UPDATE,FXMDIChild::ID_MDI_MINIMIZE,FXMDIChild::onUpdMinimize),
  FXMAPFUNC(SEL_UPDATE,FXMDIChild::ID_MDI_RESTORE,FXMDIChild::onUpdRestore),
  FXMAPFUNC(SEL_UPDATE,FXMDIChild::ID_MDI_WINDOW,FXMDIChild::onUpdWindow),
  FXMAPFUNC(SEL_UPDATE,FXMDIChild::ID_MDI_MENUCLOSE,FXMDIChild::onUpdMenuClose),
  FXMAPFUNC(SEL_UPDATE,FXMDIChild::ID_MDI_MENUMINIMIZE,FXMDIChild::onUpdMenuMinimize),
  FXMAPFUNC(SEL_UPDATE,FXMDIChild::ID_MDI_MENURESTORE,FXMDIChild::onUpdMenuRestore),
  FXMAPFUNC(SEL_UPDATE,FXMDIChild::ID_MDI_MENUWINDOW,FXMDIChild::onUpdMenuWindow),
  FXMAPFUNC(SEL_COMMAND,FXMDIChild::ID_MDI_CLOSE,FXMDIChild::onCmdClose),
  FXMAPFUNC(SEL_COMMAND,FXMDIChild::ID_MDI_MAXIMIZE,FXMDIChild::onCmdMaximize),
  FXMAPFUNC(SEL_COMMAND,FXMDIChild::ID_MDI_MINIMIZE,FXMDIChild::onCmdMinimize),
  FXMAPFUNC(SEL_COMMAND,FXMDIChild::ID_MDI_RESTORE,FXMDIChild::onCmdRestore),
  FXMAPFUNC(SEL_COMMAND,FXMDIChild::ID_MDI_MENUCLOSE,FXMDIChild::onCmdClose),
  FXMAPFUNC(SEL_COMMAND,FXMDIChild::ID_MDI_MENUMINIMIZE,FXMDIChild::onCmdMinimize),
  FXMAPFUNC(SEL_COMMAND,FXMDIChild::ID_MDI_MENURESTORE,FXMDIChild::onCmdRestore),
  FXMAPFUNC(SEL_COMMAND,FXMDIChild::ID_SETSTRINGVALUE,FXMDIChild::onCmdSetStringValue),
  FXMAPFUNC(SEL_COMMAND,FXMDIChild::ID_GETSTRINGVALUE,FXMDIChild::onCmdGetStringValue),
  FXMAPFUNC(SEL_COMMAND,FXMDIChild::ID_SETICONVALUE,FXMDIChild::onCmdSetIconValue),
  FXMAPFUNC(SEL_COMMAND,FXMDIChild::ID_GETICONVALUE,FXMDIChild::onCmdGetIconValue),
  };


// Object implementation
FXIMPLEMENT(FXMDIChild,FXComposite,FXMDIChildMap,ARRAYNUMBER(FXMDIChildMap))


// Serialization
FXMDIChild::FXMDIChild(){
  flags|=FLAG_ENABLED|FLAG_SHOWN;
  windowbtn=(FXMenuButton*)-1L;
  minimizebtn=(FXButton*)-1L;
  restorebtn=(FXButton*)-1L;
  maximizebtn=(FXButton*)-1L;
  deletebtn=(FXButton*)-1L;
  font=(FXFont*)-1L;
  baseColor=0;
  hiliteColor=0;
  shadowColor=0;
  borderColor=0;
  titleColor=0;
  titleBackColor=0;
  iconPosX=0;
  iconPosY=0;
  iconWidth=0;
  iconHeight=0;
  normalPosX=0;
  normalPosY=0;
  normalWidth=0;
  normalHeight=0;
  spotx=0;
  spoty=0;
  xoff=0;
  yoff=0;
  newx=0;
  newy=0;
  neww=0;
  newh=0;
  mode=DRAG_NONE;
  }


// Create MDI Child Window
FXMDIChild::FXMDIChild(FXMDIClient* p,const FXString& name,FXIcon* ic,FXPopup* pup,FXuint opts,FXint x,FXint y,FXint w,FXint h):FXComposite(p,opts,x,y,w,h),title(name){
  flags|=FLAG_ENABLED|FLAG_SHOWN;
  windowbtn=new FXMDIWindowButton(this,pup,this,FXWindow::ID_MDI_WINDOW);
  minimizebtn=new FXMDIMinimizeButton(this,this,FXWindow::ID_MDI_MINIMIZE,FRAME_RAISED);
  restorebtn=new FXMDIRestoreButton(this,this,FXWindow::ID_MDI_RESTORE,FRAME_RAISED);
  maximizebtn=new FXMDIMaximizeButton(this,this,FXWindow::ID_MDI_MAXIMIZE,FRAME_RAISED);
  deletebtn=new FXMDIDeleteButton(this,this,FXWindow::ID_MDI_CLOSE,FRAME_RAISED);
  windowbtn->setIcon(ic);
  baseColor=getApp()->getBaseColor();
  hiliteColor=getApp()->getHiliteColor();
  shadowColor=getApp()->getShadowColor();
  borderColor=getApp()->getBorderColor();
  titleColor=getApp()->getSelforeColor();
  titleBackColor=getApp()->getSelbackColor();
  font=getApp()->getNormalFont();
  iconPosX=xpos;
  iconPosY=ypos;
  iconWidth=width;
  iconHeight=height;
  normalPosX=xpos;
  normalPosY=ypos;
  normalWidth=width;
  normalHeight=height;
  if(options&(MDI_MAXIMIZED|MDI_MINIMIZED)){
    normalWidth=p->getWidth()*2/3;
    normalHeight=p->getHeight()*2/3;
    if(normalWidth<8) normalWidth=200;
    if(normalHeight<8) normalHeight=160;
    }
  spotx=0;
  spoty=0;
  xoff=0;
  yoff=0;
  newx=0;
  newy=0;
  neww=0;
  newh=0;
  mode=DRAG_NONE;
  }


// Create window
void FXMDIChild::create(){
  FXComposite::create();
  font->create();
  recalc();
  }


// Detach window
void FXMDIChild::detach(){
  FXComposite::detach();
  font->detach();
  }


// Get content window (if any!)
FXWindow *FXMDIChild::contentWindow() const {
  return deletebtn->getNext();
  }


// Get width
FXint FXMDIChild::getDefaultWidth(){
  FXint mw,bw;
  mw=windowbtn->getDefaultWidth();
  bw=deletebtn->getDefaultWidth();
  return TITLESPACE+mw+3*bw+(BORDERWIDTH<<1)+2+4+4+6+2;
  }


// Get height
FXint FXMDIChild::getDefaultHeight(){
  FXint fh,mh,bh;
  fh=font->getFontHeight();
  mh=windowbtn->getDefaultHeight();
  bh=deletebtn->getDefaultHeight();
  return FXMAX3(fh,mh,bh)+(BORDERWIDTH<<1)+2;
  }


// Just tell server where the windows are!
void FXMDIChild::layout(){
  FXWindow *contents=contentWindow();
  FXint th,fh,mw,mh,bw,bh,by,bx;
  fh=font->getFontHeight();
  mw=windowbtn->getDefaultWidth();
  mh=windowbtn->getDefaultHeight();
  bw=deletebtn->getDefaultWidth();
  bh=deletebtn->getDefaultHeight();
  th=FXMAX3(fh,mh,bh)+2;
  bx=width-BORDERWIDTH-bw-2;
  by=BORDERWIDTH+(th-bh)/2;
  if(isMaximized()){
    windowbtn->hide();
    deletebtn->hide();
    maximizebtn->hide();
    minimizebtn->hide();
    restorebtn->hide();
    if(contents){
      contents->position(0,0,width,height);
      contents->raise();
      contents->show();
      }
    }
  else if(isMinimized()){
    windowbtn->position(BORDERWIDTH+2,BORDERWIDTH+(th-mh)/2,mw,mh);
    deletebtn->position(bx,by,bw,bh); bx-=bw+3;
    maximizebtn->position(bx,by,bw,bh); bx-=bw+3;
    restorebtn->position(bx,by,bw,bh);
    windowbtn->show();
    deletebtn->show();
    maximizebtn->show();
    minimizebtn->hide();
    restorebtn->show();
    if(contents){
      contents->hide();
      }
    }
  else{
    windowbtn->position(BORDERWIDTH+2,BORDERWIDTH+(th-mh)/2,mw,mh);
    deletebtn->position(bx,by,bw,bh); bx-=bw+3;
    maximizebtn->position(bx,by,bw,bh); bx-=bw+3;
    minimizebtn->position(bx,by,bw,bh);
    windowbtn->show();
    deletebtn->show();
    maximizebtn->show();
    minimizebtn->show();
    restorebtn->hide();
    if(contents){
      contents->position(BORDERWIDTH+2,BORDERWIDTH+2+th,width-(BORDERWIDTH<<1)-4,height-th-(BORDERWIDTH<<1)-4);
      contents->show();
      }
    }
  flags&=~FLAG_DIRTY;
  }


// Restore window
FXbool FXMDIChild::restore(FXbool notify){
  if(isMaximized() || isMinimized()){
    if(isMinimized()){
      iconPosX=xpos;
      iconPosY=ypos;
      iconWidth=width;
      iconHeight=height;
      }
    xpos=normalPosX;
    ypos=normalPosY;
    width=normalWidth;
    height=normalHeight;
    options&=~(MDI_MINIMIZED|MDI_MAXIMIZED);
    recalc();
    if(notify && target){target->tryHandle(this,FXSEL(SEL_RESTORE,message),nullptr);}
    }
  return true;
  }


// Maximize window
FXbool FXMDIChild::maximize(FXbool notify){
  if(!isMaximized()){
    if(isMinimized()){
      iconPosX=xpos;
      iconPosY=ypos;
      iconWidth=width;
      iconHeight=height;
      }
    else{
      normalPosX=xpos;
      normalPosY=ypos;
      normalWidth=width;
      normalHeight=height;
      }
    xpos=0;
    ypos=0;
    width=getParent()->getWidth();
    height=getParent()->getHeight();
    options|=MDI_MAXIMIZED;
    options&=~MDI_MINIMIZED;
    recalc();
    if(notify && target){target->tryHandle(this,FXSEL(SEL_MAXIMIZE,message),nullptr);}
    }
  return true;
  }


// Minimize window
FXbool FXMDIChild::minimize(FXbool notify){
  if(!isMinimized()){
    if(!isMaximized()){
      normalPosX=xpos;
      normalPosY=ypos;
      normalWidth=width;
      normalHeight=height;
      }
    xpos=iconPosX;
    ypos=iconPosY;
    width=getDefaultWidth();
    height=getDefaultHeight();
    options|=MDI_MINIMIZED;
    options&=~MDI_MAXIMIZED;
    recalc();
    if(notify && target){target->tryHandle(this,FXSEL(SEL_MINIMIZE,message),nullptr);}
    }
  return true;
  }


// Close MDI window, return true if actually closed
FXbool FXMDIChild::close(FXbool notify){
  FXMDIChild *alternative=(FXMDIChild*)(getNext()?getNext():getPrev());
  FXMDIClient *client=(FXMDIClient*)getParent();

  // Deactivate this window
  client->setActiveChild(alternative,notify);

  // See if OK to close
  if(!notify || !target || !target->tryHandle(this,FXSEL(SEL_CLOSE,message),nullptr)){

    // Target will receive no further messages from us
    setTarget(nullptr);
    setSelector(0);

    // Self destruct
    delete this;

    // Was closed
    return true;
    }

  // Reactivate window, we're not closing
  client->setActiveChild(this,notify);

  // Didn't close
  return false;
  }


// Is it maximized?
FXbool FXMDIChild::isMaximized() const {
  return (options&MDI_MAXIMIZED)!=0;
  }


// Is it minimized
FXbool FXMDIChild::isMinimized() const {
  return (options&MDI_MINIMIZED)!=0;
  }


// Move this window to the specified position in the parent's coordinates
void FXMDIChild::move(FXint x,FXint y){
  FXComposite::move(x,y);
  if(isMinimized()){
    iconPosX=x;
    iconPosY=y;
    }
  else if(!isMaximized()){
    normalPosX=x;
    normalPosY=y;
    }
  }


// Resize this window to the specified width and height
void FXMDIChild::resize(FXint w,FXint h){
  FXComposite::resize(w,h);
  if(isMinimized()){
    iconWidth=w;
    iconHeight=h;
    }
  else if(!isMaximized()){
    normalWidth=w;
    normalHeight=h;
    }
  }


// Move and resize this window in the parent's coordinates
void FXMDIChild::position(FXint x,FXint y,FXint w,FXint h){
  FXComposite::position(x,y,w,h);
  if(isMinimized()){
    iconPosX=x;
    iconPosY=y;
    iconWidth=w;
    iconHeight=h;
    }
  else if(!isMaximized()){
    normalPosX=x;
    normalPosY=y;
    normalWidth=w;
    normalHeight=h;
    }
  }


// Into focus chain
void FXMDIChild::setFocus(){
  FXMDIClient *client=(FXMDIClient*)getParent();
  client->setActiveChild(this,true);
  FXComposite::setFocus();
  }


// If window can have focus
FXbool FXMDIChild::canFocus() const { return true; }


// Change cursor based on location over window
void FXMDIChild::changeCursor(FXuchar which){
  switch(which){
    case DRAG_TOP:
    case DRAG_BOTTOM:
      setDefaultCursor(getApp()->getDefaultCursor(DEF_DRAGH_CURSOR));
      setDragCursor(getApp()->getDefaultCursor(DEF_DRAGH_CURSOR));
      break;
    case DRAG_LEFT:
    case DRAG_RIGHT:
      setDefaultCursor(getApp()->getDefaultCursor(DEF_DRAGV_CURSOR));
      setDragCursor(getApp()->getDefaultCursor(DEF_DRAGV_CURSOR));
      break;
    case DRAG_TOPLEFT:
    case DRAG_BOTTOMRIGHT:
      setDefaultCursor(getApp()->getDefaultCursor(DEF_DRAGTL_CURSOR));
      setDragCursor(getApp()->getDefaultCursor(DEF_DRAGTL_CURSOR));
      break;
    case DRAG_TOPRIGHT:
    case DRAG_BOTTOMLEFT:
      setDefaultCursor(getApp()->getDefaultCursor(DEF_DRAGTR_CURSOR));
      setDragCursor(getApp()->getDefaultCursor(DEF_DRAGTR_CURSOR));
      break;
    default:
      setDefaultCursor(getApp()->getDefaultCursor(DEF_ARROW_CURSOR));
      setDragCursor(getApp()->getDefaultCursor(DEF_ARROW_CURSOR));
      break;
    }
  }


// Draw rubberband box
void FXMDIChild::drawRubberBox(FXint x,FXint y,FXint w,FXint h){
  if(BORDERWIDTH*2<w && BORDERWIDTH*2<h){
    FXDCWindow dc(getParent());
    dc.clipChildren(false);
    dc.setFunction(BLT_SRC_XOR_DST);
    dc.setForeground(getParent()->getBackColor());
    //dc.drawHashBox(xx,yy,w,h,BORDERWIDTH);
    dc.setLineWidth(BORDERWIDTH);
    dc.drawRectangle(x+BORDERWIDTH/2,y+BORDERWIDTH/2,w-BORDERWIDTH,h-BORDERWIDTH);
    }
  }


// Draw animation morphing from old to new rectangle
void FXMDIChild::animateRectangles(FXint ox,FXint oy,FXint ow,FXint oh,FXint nx,FXint ny,FXint nw,FXint nh){
  if(xid && getApp()->getAnimSpeed()){
    FXDCWindow dc(getParent());
    FXint bx,by,bw,bh,s,t;
    dc.clipChildren(false);
    dc.setFunction(BLT_SRC_XOR_DST);
    dc.setForeground(getParent()->getBackColor());
    FXuint step=500;
    for(s=0,t=10000; s<=10000; s+=step,t-=step){
      bx=(nx*s+ox*t)/10000;
      by=(ny*s+oy*t)/10000;
      bw=(nw*s+ow*t)/10000;
      bh=(nh*s+oh*t)/10000;
      if(BORDERWIDTH*2<bw && BORDERWIDTH*2<bh){
        dc.drawHashBox(bx,by,bw,bh,BORDERWIDTH);
        getApp()->flush(true);
        FXThread::sleep(getApp()->getAnimSpeed());
        dc.drawHashBox(bx,by,bw,bh,BORDERWIDTH);
        getApp()->flush(true);
        }
      }
    }
  }


// Handle repaint
long FXMDIChild::onPaint(FXObject*,FXSelector,void* ptr){
  FXEvent *ev=(FXEvent*)ptr;
  FXint xx,yy,th,titlespace,letters,dots,dotspace;
  FXint fh,mh,bh,bw,mw;

  // If box is shown, hide it temporarily
  if(mode&DRAG_INVERTED) drawRubberBox(newx,newy,neww,newh);

  {
  FXDCWindow dc(this,ev);

  // Draw MDIChild background
  dc.setForeground(baseColor);
  dc.fillRectangle(ev->rect.x,ev->rect.y,ev->rect.w,ev->rect.h);

  // Only draw stuff when not maximized
  if(!isMaximized()){

    // Compute sizes
    fh=font->getFontHeight();
    mw=windowbtn->getDefaultWidth();
    mh=windowbtn->getDefaultHeight();
    bw=deletebtn->getDefaultWidth();
    bh=deletebtn->getDefaultHeight();
    th=FXMAX3(fh,mh,bh)+2;

    // Draw outer border
    dc.setForeground(baseColor);
    dc.fillRectangle(0,0,width-1,1);
    dc.fillRectangle(0,0,1,height-2);
    dc.setForeground(hiliteColor);
    dc.fillRectangle(1,1,width-2,1);
    dc.fillRectangle(1,1,1,height-2);
    dc.setForeground(shadowColor);
    dc.fillRectangle(1,height-2,width-1,1);
    dc.fillRectangle(width-2,1,1,height-2);
    dc.setForeground(borderColor);
    dc.fillRectangle(0,height-1,width,1);
    dc.fillRectangle(width-1,0,1,height);

    // Draw title background
    if(isActive()){
      dc.fillHorizontalGradient(BORDERWIDTH,BORDERWIDTH,width-BORDERWIDTH*2,th,(hasFocus()?titleBackColor:shadowColor),backColor);
      }
    else{
      dc.setForeground(backColor);
      dc.fillRectangle(BORDERWIDTH,BORDERWIDTH,width-BORDERWIDTH*2,th);
      }

    // Draw title
    if(!title.empty()){
      xx=BORDERWIDTH+mw+2+4;
      yy=BORDERWIDTH+font->getFontAscent()+(th-fh)/2;

      // Compute space for title
      titlespace=width-mw-3*bw-(BORDERWIDTH<<1)-2-4-4-6-2;

      dots=0;
      letters=title.length();

      // Title too large for space
      if(font->getTextWidth(title.text(),letters)>titlespace){
        dotspace=titlespace-font->getTextWidth("...",3);
        while(letters>0 && font->getTextWidth(title.text(),letters)>dotspace) letters--;
        dots=3;
        if(letters==0){
          letters=1;
          dots=0;
          }
        }

      // Draw as much of the title as possible
      dc.setForeground(isActive() ? titleColor : borderColor);
      dc.setFont(font);
      dc.drawText(xx,yy,title.text(),letters);
      dc.drawText(xx+font->getTextWidth(title.text(),letters),yy,"...",dots);
      }

    // Draw inner border
    if(!isMinimized()){
      dc.setForeground(shadowColor);
      dc.fillRectangle(BORDERWIDTH,BORDERWIDTH+th,width-BORDERWIDTH*2-1,1);
      dc.fillRectangle(BORDERWIDTH,BORDERWIDTH+th,1,height-th-BORDERWIDTH*2-1);
      dc.setForeground(borderColor);
      dc.fillRectangle(BORDERWIDTH+1,BORDERWIDTH+th+1,width-BORDERWIDTH*2-3,1);
      dc.fillRectangle(BORDERWIDTH+1,BORDERWIDTH+th+1,1,height-th-BORDERWIDTH*2-3);
      dc.setForeground(hiliteColor);
      dc.fillRectangle(BORDERWIDTH,height-BORDERWIDTH-1,width-BORDERWIDTH*2,1);
      dc.fillRectangle(width-BORDERWIDTH-1,BORDERWIDTH+th,1,height-th-BORDERWIDTH*2);
      dc.setForeground(baseColor);
      dc.fillRectangle(BORDERWIDTH+1,height-BORDERWIDTH-2,width-BORDERWIDTH*2-2,1);
      dc.fillRectangle(width-BORDERWIDTH-2,BORDERWIDTH+th+1,1,height-th-BORDERWIDTH*2-2);
      }
    }
  }

  // Redraw the box over freshly painted window
  if(mode&DRAG_INVERTED) drawRubberBox(newx,newy,neww,newh);

  return 1;
  }


// Find out where window was grabbed
FXuchar FXMDIChild::where(FXint x,FXint y) const {
  FXuchar code=DRAG_NONE;
  FXint fh,mh,bh,th;
  fh=font->getFontHeight();
  mh=windowbtn->getDefaultHeight();
  bh=deletebtn->getDefaultHeight();
  th=FXMAX3(fh,mh,bh)+2;
  if(!isMinimized() && x<HANDLESIZE) code|=DRAG_LEFT;
  if(!isMinimized() && width-HANDLESIZE<=x) code|=DRAG_RIGHT;
  if(!isMinimized() && y<HANDLESIZE) code|=DRAG_TOP;
  if(!isMinimized() && height-HANDLESIZE<=y) code|=DRAG_BOTTOM;
  if(BORDERWIDTH<=x && x<=width-BORDERWIDTH && BORDERWIDTH<=y && y<BORDERWIDTH+th) code=DRAG_TITLE;
  return code;
  }


// Entering window
long FXMDIChild::onEnter(FXObject* sender,FXSelector sel,void* ptr){
  FXComposite::onEnter(sender,sel,ptr);
  if(mode==DRAG_NONE){ changeCursor(where(((FXEvent*)ptr)->win_x,((FXEvent*)ptr)->win_y)); }
  return 1;
  }


// Leaving window
long FXMDIChild::onLeave(FXObject* sender,FXSelector sel,void* ptr){
  FXComposite::onLeave(sender,sel,ptr);
  if(mode==DRAG_NONE){ changeCursor(DRAG_NONE); }
  return 1;
  }


// Focus on widget itself and try put focus on the content window as well
long FXMDIChild::onFocusSelf(FXObject* sender,FXSelector,void* ptr){            // See FXScrollWindow
  setFocus();
  if(contentWindow()) contentWindow()->handle(sender,FXSEL(SEL_FOCUS_SELF,0),ptr);
  return 1;
  }


// Gained focus
long FXMDIChild::onFocusIn(FXObject* sender,FXSelector sel,void* ptr){
  FXint fh,mh,bh,th;
  FXComposite::onFocusIn(sender,sel,ptr);
  fh=font->getFontHeight();
  mh=windowbtn->getDefaultHeight();
  bh=deletebtn->getDefaultHeight();
  th=FXMAX3(fh,mh,bh)+2;
  windowbtn->setBackColor(isActive() ? titleBackColor : backColor);
  update(BORDERWIDTH,BORDERWIDTH,width-(BORDERWIDTH<<1),th);
  return 1;
  }


// Lost focus
long FXMDIChild::onFocusOut(FXObject* sender,FXSelector sel,void* ptr){
  FXint fh,mh,bh,th;
  FXComposite::onFocusOut(sender,sel,ptr);
  fh=font->getFontHeight();
  mh=windowbtn->getDefaultHeight();
  bh=deletebtn->getDefaultHeight();
  th=FXMAX3(fh,mh,bh)+2;
  windowbtn->setBackColor(isActive() ? shadowColor : backColor);
  update(BORDERWIDTH,BORDERWIDTH,width-(BORDERWIDTH<<1),th);
  return 1;
  }


// Pressed LEFT button
long FXMDIChild::onLeftBtnPress(FXObject*,FXSelector,void* ptr){
  FXEvent *event=(FXEvent*)ptr;
  flags&=~FLAG_TIP;
  handle(this,FXSEL(SEL_FOCUS_SELF,0),ptr);
  if(isEnabled()){
    grab();
    if(target && target->tryHandle(this,FXSEL(SEL_LEFTBUTTONPRESS,message),ptr)) return 1;
    if(event->click_count==1){
      mode=where(event->win_x,event->win_y);
      if(mode!=DRAG_NONE){
        if(mode&(DRAG_TOP|DRAG_TITLE)) spoty=event->win_y;
        else if(mode&DRAG_BOTTOM) spoty=event->win_y-height;
        if(mode&(DRAG_LEFT|DRAG_TITLE)) spotx=event->win_x;
        else if(mode&DRAG_RIGHT) spotx=event->win_x-width;
        xoff=event->win_x+xpos-event->root_x;
        yoff=event->win_y+ypos-event->root_y;
        newx=xpos;
        newy=ypos;
        neww=width;
        newh=height;
        if(!(options&MDI_TRACKING)){
          if(!(mode&DRAG_TITLE)){
            drawRubberBox(newx,newy,neww,newh);
            mode|=DRAG_INVERTED;
            }
          }
        }
      }
    return 1;
    }
  return 0;
  }


// Released LEFT button
long FXMDIChild::onLeftBtnRelease(FXObject*,FXSelector,void* ptr){
  FXEvent *event=(FXEvent*)ptr;
  if(isEnabled()){
    ungrab();
    if(target && target->tryHandle(this,FXSEL(SEL_LEFTBUTTONRELEASE,message),ptr)) return 1;
    if(event->click_count==1){
      if(mode!=DRAG_NONE){
        if(!(options&MDI_TRACKING)){
          if(mode&DRAG_INVERTED) drawRubberBox(newx,newy,neww,newh);
          position(newx,newy,neww,newh);
          }
        mode=DRAG_NONE;
        recalc();
        }
      }
    else if(event->click_count==2){
      if(isMinimized()){
        animateRectangles(xpos,ypos,width,height,normalPosX,normalPosY,normalWidth,normalHeight);
        restore(true);
        }
      else if(isMaximized()){
        animateRectangles(xpos,ypos,width,height,normalPosX,normalPosY,normalWidth,normalHeight);
        restore(true);
        }
      else{
        animateRectangles(xpos,ypos,width,height,0,0,getParent()->getWidth(),getParent()->getHeight());
        maximize(true);
        }
      }
    return 1;
    }
  return 0;
  }


// Moved
long FXMDIChild::onMotion(FXObject*,FXSelector,void* ptr){
  FXEvent *event=(FXEvent*)ptr;
  FXint tmp,mousex,mousey;
  FXint oldx,oldy,oldw,oldh;
  if(mode!=DRAG_NONE){

    // Mouse in FXMDIClient's coordinates
    mousex=event->root_x+xoff;
    mousey=event->root_y+yoff;

    // Keep inside FXMDIClient
    if(mousex<0) mousex=0;
    if(mousey<0) mousey=0;
    if(mousex>=getParent()->getWidth()) mousex=getParent()->getWidth()-1;
    if(mousey>=getParent()->getHeight()) mousey=getParent()->getHeight()-1;

    // Remember old box
    oldx=newx;
    oldy=newy;
    oldw=neww;
    oldh=newh;

    // Dragging title
    if(mode&DRAG_TITLE){
      if(!event->moved) return 1;
      newy=mousey-spoty;
      newx=mousex-spotx;
      setDragCursor(getApp()->getDefaultCursor(DEF_MOVE_CURSOR));
      }

    // Dragging sides
    else{

      // Vertical
      if(mode&DRAG_TOP){
        tmp=newh+newy-mousey+spoty;
        if(tmp>=MINHEIGHT){ newh=tmp; newy=mousey-spoty; }
        }
      else if(mode&DRAG_BOTTOM){
        tmp=mousey-spoty-newy;
        if(tmp>=MINHEIGHT){ newh=tmp; }
        }

      // Horizontal
      if(mode&DRAG_LEFT){
        tmp=neww+newx-mousex+spotx;
        if(tmp>=MINWIDTH){ neww=tmp; newx=mousex-spotx; }
        }
      else if(mode&DRAG_RIGHT){
        tmp=mousex-spotx-newx;
        if(tmp>=MINWIDTH){ neww=tmp; }
        }
      }

    // Move box
    if(!(options&MDI_TRACKING)){
      if(mode&DRAG_INVERTED) drawRubberBox(oldx,oldy,oldw,oldh);
      drawRubberBox(newx,newy,neww,newh);
      mode|=DRAG_INVERTED;
      }
    else{
      position(newx,newy,neww,newh);
      }
    return 1;
    }

  // Othersize just change cursor based on location
  changeCursor(where(event->win_x,event->win_y));
  return 0;
  }


// Pressed MIDDLE button
long FXMDIChild::onMiddleBtnPress(FXObject*,FXSelector,void* ptr){
  flags&=~FLAG_TIP;
  handle(this,FXSEL(SEL_FOCUS_SELF,0),ptr);
  if(isEnabled()){
    grab();
    if(target && target->tryHandle(this,FXSEL(SEL_MIDDLEBUTTONPRESS,message),ptr)) return 1;
    return 1;
    }
  return 0;
  }


// Released MIDDLE button
long FXMDIChild::onMiddleBtnRelease(FXObject*,FXSelector,void* ptr){
  if(isEnabled()){
    ungrab();
    if(target && target->tryHandle(this,FXSEL(SEL_MIDDLEBUTTONRELEASE,message),ptr)) return 1;
    return 1;
    }
  return 0;
  }


// Pressed RIGHT button
long FXMDIChild::onRightBtnPress(FXObject*,FXSelector,void* ptr){
  flags&=~FLAG_TIP;
  if(isEnabled()){
    grab();
    if(target && target->tryHandle(this,FXSEL(SEL_RIGHTBUTTONPRESS,message),ptr)) return 1;
    lower();
    return 1;
    }
  return 0;
  }


// Released RIGHT button
long FXMDIChild::onRightBtnRelease(FXObject*,FXSelector,void* ptr){
  if(isEnabled()){
    ungrab();
    if(target && target->tryHandle(this,FXSEL(SEL_RIGHTBUTTONRELEASE,message),ptr)) return 1;
    return 1;
    }
  return 0;
  }


// Update value from a message
long FXMDIChild::onCmdSetStringValue(FXObject*,FXSelector,void* ptr){
  setTitle(*((FXString*)ptr));
  return 1;
  }


// Obtain value from text field
long FXMDIChild::onCmdGetStringValue(FXObject*,FXSelector,void* ptr){
  *((FXString*)ptr)=getTitle();
  return 1;
  }


// Update icon from a message
long FXMDIChild::onCmdSetIconValue(FXObject*,FXSelector,void* ptr){
  setIcon(*((FXIcon**)ptr));
  return 1;
  }


// Obtain icon from text field
long FXMDIChild::onCmdGetIconValue(FXObject*,FXSelector,void* ptr){
  *((FXIcon**)ptr)=getIcon();
  return 1;
  }


// Window was selected; the pointer is the old active child
long FXMDIChild::onSelected(FXObject*,FXSelector,void* ptr){    // FIXME
  if(!(flags&FLAG_ACTIVE)){
    if(target) target->tryHandle(this,FXSEL(SEL_SELECTED,message),ptr);
    windowbtn->setBackColor(hasFocus() ? titleBackColor : shadowColor);
    flags|=FLAG_ACTIVE;
    recalc();
    update();
    }
  return 1;
  }


// Window was deselected; the pointer is the new active child
long FXMDIChild::onDeselected(FXObject*,FXSelector,void* ptr){    // FIXME
  if(flags&FLAG_ACTIVE){
    if(target) target->tryHandle(this,FXSEL(SEL_DESELECTED,message),ptr);
    windowbtn->setBackColor(backColor);
    flags&=~FLAG_ACTIVE;
    recalc();
    update();
    }
  return 1;
  }


/*******************************************************************************/


// Restore window command
long FXMDIChild::onCmdRestore(FXObject*,FXSelector,void*){
  animateRectangles(xpos,ypos,width,height,normalPosX,normalPosY,normalWidth,normalHeight);
  restore(true);
  return 1;
  }


// Update restore command
long FXMDIChild::onUpdRestore(FXObject* sender,FXSelector,void*){
  sender->handle(this,isMinimized()||isMaximized()?FXSEL(SEL_COMMAND,ID_ENABLE):FXSEL(SEL_COMMAND,ID_DISABLE),nullptr);
  return 1;
  }


// Update MDI restore button on menu bar
long FXMDIChild::onUpdMenuRestore(FXObject* sender,FXSelector,void*){
  if(isMaximized()){
    sender->handle(this,FXSEL(SEL_COMMAND,ID_SHOW),nullptr);
    sender->handle(this,FXSEL(SEL_COMMAND,ID_ENABLE),nullptr);
    }
  else{
    sender->handle(this,FXSEL(SEL_COMMAND,ID_HIDE),nullptr);
    }
  return 1;
  }


// Maximize window command
long FXMDIChild::onCmdMaximize(FXObject*,FXSelector,void*){
  animateRectangles(xpos,ypos,width,height,0,0,getParent()->getWidth(),getParent()->getHeight());
  maximize(true);
  return 1;
  }


// Update maximized command
long FXMDIChild::onUpdMaximize(FXObject* sender,FXSelector,void*){
  sender->handle(this,isMaximized()?FXSEL(SEL_COMMAND,ID_DISABLE):FXSEL(SEL_COMMAND,ID_ENABLE),nullptr);
  return 1;
  }


// Minimize window command
long FXMDIChild::onCmdMinimize(FXObject*,FXSelector,void*){
  animateRectangles(xpos,ypos,width,height,iconPosX,iconPosY,getDefaultWidth(),getDefaultHeight());
  minimize(true);
  return 1;
  }


// Update minimized command
long FXMDIChild::onUpdMinimize(FXObject* sender,FXSelector,void*){
  sender->handle(this,isMinimized()?FXSEL(SEL_COMMAND,ID_DISABLE):FXSEL(SEL_COMMAND,ID_ENABLE),nullptr);
  return 1;
  }


// Update MDI minimized button on menu bar
long FXMDIChild::onUpdMenuMinimize(FXObject* sender,FXSelector,void*){
  if(isMaximized()){
    sender->handle(this,FXSEL(SEL_COMMAND,ID_SHOW),nullptr);
    sender->handle(this,FXSEL(SEL_COMMAND,ID_ENABLE),nullptr);
    }
  else{
    sender->handle(this,FXSEL(SEL_COMMAND,ID_HIDE),nullptr);
    }
  return 1;
  }


// Close window after asking FXMDIChild's target; returns 1 if closed
long FXMDIChild::onCmdClose(FXObject*,FXSelector,void*){
  return close(true);
  }


// Update close command
long FXMDIChild::onUpdClose(FXObject* sender,FXSelector,void*){
  sender->handle(this,FXSEL(SEL_COMMAND,ID_ENABLE),nullptr);
  return 1;
  }


// Update MDI close button on menu bar
long FXMDIChild::onUpdMenuClose(FXObject* sender,FXSelector,void*){
  if(isMaximized()){
    sender->handle(this,FXSEL(SEL_COMMAND,ID_ENABLE),nullptr);
    sender->handle(this,FXSEL(SEL_COMMAND,ID_SHOW),nullptr);
    }
  else{
    sender->handle(this,FXSEL(SEL_COMMAND,ID_HIDE),nullptr);
    }
  return 1;
  }


// Update window menu button
long FXMDIChild::onUpdWindow(FXObject* sender,FXSelector,void*){
  sender->handle(this,FXSEL(SEL_COMMAND,ID_ENABLE),nullptr);
  return 1;
  }


// Update MDI window menu button on menu bar
long FXMDIChild::onUpdMenuWindow(FXObject* sender,FXSelector,void*){
  FXIcon *icon=getIcon();
  if(isMaximized()){
    sender->handle(this,FXSEL(SEL_COMMAND,ID_SHOW),nullptr);
    sender->handle(this,FXSEL(SEL_COMMAND,ID_ENABLE),nullptr);
    sender->handle(this,FXSEL(SEL_COMMAND,ID_SETICONVALUE),(void*)&icon);
    }
  else{
    sender->handle(this,FXSEL(SEL_COMMAND,ID_HIDE),nullptr);
    }
  return 1;
  }


// Set base color
void FXMDIChild::setBaseColor(FXColor clr){
  if(baseColor!=clr){
    baseColor=clr;
    update();
    }
  }


// Set highlight color
void FXMDIChild::setHiliteColor(FXColor clr){
  if(hiliteColor!=clr){
    hiliteColor=clr;
    update();
    }
  }


// Set shadow color
void FXMDIChild::setShadowColor(FXColor clr){
  if(shadowColor!=clr){
    shadowColor=clr;
    update();
    }
  }


// Set border color
void FXMDIChild::setBorderColor(FXColor clr){
  if(borderColor!=clr){
    borderColor=clr;
    update();
    }
  }


// Set title color
void FXMDIChild::setTitleColor(FXColor clr){
  if(titleColor!=clr){
    titleColor=clr;
    update();
    }
  }


// Set title color
void FXMDIChild::setTitleBackColor(FXColor clr){
  if(titleBackColor!=clr){
    titleBackColor=clr;
    update();
    }
  }


// Set new window title
void FXMDIChild::setTitle(const FXString& name){
  if(title!=name){
    title=name;
    update();
    }
  }


// Delegate all unhandled messages to content window or MDI child's target,
// except for those messages with ID's which belong to the MDI child itself.
long FXMDIChild::onDefault(FXObject* sender,FXSelector sel,void* ptr){
  if(FXMDIChild::ID_LAST<=FXSELID(sel)){
    if(contentWindow() && contentWindow()->tryHandle(sender,sel,ptr)) return 1;
    return target && target->tryHandle(sender,sel,ptr);
    }
  return 0;
  }


// Get icon used for the menu button
FXIcon *FXMDIChild::getIcon() const {
  return windowbtn->getIcon();
  }


// Change icon used for window menu button
void FXMDIChild::setIcon(FXIcon* ic){
  windowbtn->setIcon(ic);
  }


// Obtain window menu
FXPopup* FXMDIChild::getMenu() const {
  return windowbtn->getMenu();
  }


// Change window menu
void FXMDIChild::setMenu(FXPopup* menu){
  windowbtn->setMenu(menu);
  }


// Change the font
void FXMDIChild::setFont(FXFont *fnt){
  if(!fnt){ fxerror("%s::setFont: NULL font specified.\n",getClassName()); }
  if(font!=fnt){
    font=fnt;
    recalc();
    update();
    }
  }


// Set tracking instead of just outline
void FXMDIChild::setTracking(FXbool tracking){
  if(tracking) options|=MDI_TRACKING; else options&=~MDI_TRACKING;
  }


// Return true if tracking
FXbool FXMDIChild::getTracking() const {
  return (options&MDI_TRACKING)!=0;
  }


// Save object to stream
void FXMDIChild::save(FXStream& store) const {
  FXComposite::save(store);
  store << title;
  store << windowbtn;
  store << minimizebtn;
  store << restorebtn;
  store << maximizebtn;
  store << deletebtn;
  store << font;
  store << baseColor;
  store << hiliteColor;
  store << shadowColor;
  store << borderColor;
  store << titleColor;
  store << titleBackColor;
  store << iconPosX;
  store << iconPosY;
  store << iconWidth;
  store << iconHeight;
  store << normalPosX;
  store << normalPosY;
  store << normalWidth;
  store << normalHeight;
  }


// Load object from stream
void FXMDIChild::load(FXStream& store){
  FXComposite::load(store);
  store >> title;
  store >> windowbtn;
  store >> minimizebtn;
  store >> restorebtn;
  store >> maximizebtn;
  store >> deletebtn;
  store >> font;
  store >> baseColor;
  store >> hiliteColor;
  store >> shadowColor;
  store >> borderColor;
  store >> titleColor;
  store >> titleBackColor;
  store >> iconPosX;
  store >> iconPosY;
  store >> iconWidth;
  store >> iconHeight;
  store >> normalPosX;
  store >> normalPosY;
  store >> normalWidth;
  store >> normalHeight;
  }


// Destruct thrashes the pointers
FXMDIChild::~FXMDIChild(){
  if(((FXMDIClient*)getParent())->active==this) ((FXMDIClient*)getParent())->active=nullptr;
  windowbtn=(FXMenuButton*)-1L;
  minimizebtn=(FXButton*)-1L;
  restorebtn=(FXButton*)-1L;
  maximizebtn=(FXButton*)-1L;
  deletebtn=(FXButton*)-1L;
  font=(FXFont*)-1L;
  }

}
