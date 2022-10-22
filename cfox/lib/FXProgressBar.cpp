/********************************************************************************
*                                                                               *
*                      P r o g r e s s B a r   W i d g e t                      *
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
#include "FXStream.h"
#include "FXString.h"
#include "FXSize.h"
#include "FXPoint.h"
#include "FXRectangle.h"
#include "FXStringDictionary.h"
#include "FXSettings.h"
#include "FXRegistry.h"
#include "FXFont.h"
#include "FXEvent.h"
#include "FXWindow.h"
#include "FXBitmap.h"
#include "FXDCWindow.h"
#include "FXColors.h"
#include "FXApp.h"
#include "FXProgressBar.h"


/*
  Notes:
  - Reduced flicker by not drawing background at all.
  - Reduced flicker by setting clip rectangle to only redraw interior.
  - Progress bar has a target, as it can send update messages.
*/

#define PROGRESSBAR_MASK (PROGRESSBAR_PERCENTAGE|PROGRESSBAR_VERTICAL|PROGRESSBAR_DIAL)

using namespace FX;

/*******************************************************************************/

namespace FX {


// Furnish our own version
extern FXAPI FXint __snprintf(FXchar* string,FXint length,const FXchar* format,...);


// Map
FXDEFMAP(FXProgressBar) FXProgressBarMap[]={
  FXMAPFUNC(SEL_PAINT,0,FXProgressBar::onPaint),
  FXMAPFUNC(SEL_COMMAND,FXProgressBar::ID_SETVALUE,FXProgressBar::onCmdSetValue),
  FXMAPFUNC(SEL_COMMAND,FXProgressBar::ID_SETINTVALUE,FXProgressBar::onCmdSetIntValue),
  FXMAPFUNC(SEL_COMMAND,FXProgressBar::ID_GETINTVALUE,FXProgressBar::onCmdGetIntValue),
  FXMAPFUNC(SEL_COMMAND,FXProgressBar::ID_SETLONGVALUE,FXProgressBar::onCmdSetLongValue),
  FXMAPFUNC(SEL_COMMAND,FXProgressBar::ID_GETLONGVALUE,FXProgressBar::onCmdGetLongValue),
  FXMAPFUNC(SEL_COMMAND,FXProgressBar::ID_SETINTRANGE,FXProgressBar::onCmdSetIntRange),
  FXMAPFUNC(SEL_COMMAND,FXProgressBar::ID_GETINTRANGE,FXProgressBar::onCmdGetIntRange),
  };


// Object implementation
FXIMPLEMENT(FXProgressBar,FXFrame,FXProgressBarMap,ARRAYNUMBER(FXProgressBarMap))


// Make progress bar
FXProgressBar::FXProgressBar(FXComposite* p,FXObject* tgt,FXSelector sel,FXuint opts,FXint x,FXint y,FXint w,FXint h,FXint pl,FXint pr,FXint pt,FXint pb):FXFrame(p,opts,x,y,w,h,pl,pr,pt,pb){
  target=tgt;
  message=sel;
  progress=0;
  total=100;
  barsize=(opts&PROGRESSBAR_DIAL)?60:6;
  font=getApp()->getNormalFont();
  backColor=getApp()->getBackColor();
  barBGColor=getApp()->getBackColor();
  barColor=FXColors::RoyalBlue;
  textNumColor=FXColors::DarkBlue;
  textAltColor=FXColors::White;
  }


// Get minimum width
FXint FXProgressBar::getDefaultWidth(){
  FXint w=1,t;
  if((options&PROGRESSBAR_VERTICAL) || (options&PROGRESSBAR_DIAL)){
    w=barsize;
    if(options&PROGRESSBAR_PERCENTAGE){
      t=font->getTextWidth("100%",4);
      if(w<t) w=t;
      }
    }
  return w+padleft+padright+(border<<1);
  }


// Get minimum height
FXint FXProgressBar::getDefaultHeight(){
  FXint h=1,t;
  if(!(options&PROGRESSBAR_VERTICAL) || (options&PROGRESSBAR_DIAL)){
    h=barsize;
    if(options&PROGRESSBAR_PERCENTAGE){
      t=font->getFontHeight();
      if(h<t) h=t;
      }
    }
  return h+padtop+padbottom+(border<<1);
  }


// Create window
void FXProgressBar::create(){
  FXFrame::create();
  font->create();
  }


// Detach window
void FXProgressBar::detach(){
  FXFrame::detach();
  font->detach();
  }


// Update progress value from a message
long FXProgressBar::onCmdSetValue(FXObject*,FXSelector,void* ptr){
  setProgress((FXuint)(FXival)ptr);
  return 1;
  }


// Set value
long FXProgressBar::onCmdSetIntValue(FXObject*,FXSelector,void* ptr){
  setProgress(*((FXint*)ptr));
  return 1;
  }


// Get value
long FXProgressBar::onCmdGetIntValue(FXObject*,FXSelector,void* ptr){
  *((FXint*)ptr)=getProgress();
  return 1;
  }


// Update value from a message
long FXProgressBar::onCmdSetLongValue(FXObject*,FXSelector,void* ptr){
  setProgress((FXint)*((FXlong*)ptr));
  return 1;
  }


// Obtain value with a message
long FXProgressBar::onCmdGetLongValue(FXObject*,FXSelector,void* ptr){
  *((FXlong*)ptr)=(FXlong)getProgress();
  return 1;
  }


// Update range from a message
long FXProgressBar::onCmdSetIntRange(FXObject*,FXSelector,void* ptr){
  setTotal(((FXint*)ptr)[1]-((FXint*)ptr)[0]);
  return 1;
  }


// Get range with a message
long FXProgressBar::onCmdGetIntRange(FXObject*,FXSelector,void* ptr){
  ((FXint*)ptr)[0]=0;
  ((FXint*)ptr)[1]=getTotal();
  return 1;
  }


// Draw only the interior, i.e. the part that changes
void FXProgressBar::drawInterior(FXDCWindow& dc){
  FXdouble fraction=(total>0)?((FXdouble)progress/(FXdouble)total):1.0;
  FXint percent,barlength,barwidth,barfilled,tx,ty,tw,th,dx,dy,ds,n;
  FXchar numtext[10];

  // Calculate percentage complete
  percent=(FXuint)(100.0*fraction);

  // Round dial
  if(options&PROGRESSBAR_DIAL){

    // If total is 0, it's 100%
    barfilled=(FXuint)(23040.0*fraction);

    // Dial size and location
    tw=width-(border<<1)-padleft-padright;
    th=height-(border<<1)-padtop-padbottom;
    ds=FXMIN(tw,th)-1;
    dx=border+padleft+(tw-ds)/2;
    dy=border+padtop+(th-ds)/2;

    // Draw unfilled piece
    if(barfilled!=23040){
      dc.setForeground(barBGColor);
      dc.fillArc(dx,dy,ds,ds,5760,23040-barfilled);
      }

    // Draw filled piece
    if(barfilled!=0){

      // New gradient method
      for(int i=ds; i>0; i-=8){
        dc.setForeground(makeBlendColor(barBGColor,barColor,(100*i+ds/2)/ds));
        dc.fillArc(dx+(ds-i)/2,dy+(ds-i)/2,i,i,5760,-barfilled);
        }
      }

    // Draw outside circle
    dc.setForeground(borderColor);
    dc.drawArc(dx+1,dy,ds,ds,90*64,45*64);
    dc.drawArc(dx,dy+1,ds,ds,135*64,45*64);
    dc.setForeground(baseColor);
    dc.drawArc(dx-1,dy,ds,ds,270*64,45*64);
    dc.drawArc(dx,dy-1,ds,ds,315*64,45*64);

    dc.setForeground(shadowColor);
    dc.drawArc(dx,dy,ds,ds,45*64,180*64);
    dc.setForeground(hiliteColor);
    dc.drawArc(dx,dy,ds,ds,225*64,180*64);

    // Draw text
    if(options&PROGRESSBAR_PERCENTAGE){
      tw=font->getTextWidth("100%",4);
      if(tw>(10*ds)/16) return;                  // Text too wide
      th=font->getFontHeight();
      if(th>ds/2) return;                        // Text too tall
      n=__snprintf(numtext,sizeof(numtext),"%d%%",percent);
      tw=font->getTextWidth(numtext,n);
      tx=dx+ds/2-tw/2;
      ty=dy+ds/2+font->getFontAscent()+5;
      dc.setFont(font);
      dc.setForeground(textNumColor);
      dc.drawText(tx,ty,numtext,n);
      }
    }

  // Vertical bar
  else if(options&PROGRESSBAR_VERTICAL){

    // Calculate length and filled part of bar
    barlength=height-(border<<1)-padtop-padbottom;
    barwidth=width-(border<<1)-padleft-padbottom;
    barfilled=(FXuint)(barlength*fraction);

    // Draw completed bar
    if(0<barfilled){

      // Light/dark bubble
      dc.fillHorizontalGradient(border+padleft,height-border-padbottom-barfilled,barwidth/2,barfilled,makeHiliteColor(barColor,40),barColor);
      dc.fillHorizontalGradient(border+padleft+barwidth/2,height-border-padbottom-barfilled,barwidth-barwidth/2,barfilled,barColor,makeShadowColor(barColor,20));
      }

    // Draw uncompleted bar
    if(barfilled<barlength){
      dc.setForeground(barBGColor);
      dc.fillRectangle(border+padleft,border+padtop,barwidth,barlength-barfilled);
      }

    // Draw text
    if(options&PROGRESSBAR_PERCENTAGE){
      n=__snprintf(numtext,sizeof(numtext),"%d%%",percent);
      tw=font->getTextWidth(numtext,n);
      th=font->getFontHeight();
      ty=border+padtop+(barlength-th)/2+font->getFontAscent();
      tx=border+padleft+(barwidth-tw)/2;
      dc.setFont(font);
      if(height-border-barfilled>ty){           // In upper side
        dc.setForeground(textNumColor);
        dc.setClipRectangle(border+padleft,border+padtop,barwidth,barlength);
        dc.drawText(tx,ty,numtext,n);
        }
      else if(ty-th>height-border-barfilled){   // In lower side
        dc.setForeground(textAltColor);
        dc.setClipRectangle(border+padleft,border+padtop,barwidth,barlength);
        dc.drawText(tx,ty,numtext,n);
        }
      else{                                     // In between!
        dc.setForeground(textAltColor);
        dc.setClipRectangle(border+padleft,height-border-padbottom-barfilled,barwidth,barfilled);
        dc.drawText(tx,ty,numtext,n);
        dc.setForeground(textNumColor);
        dc.setClipRectangle(border+padleft,border+padtop,barwidth,barlength-barfilled);
        dc.drawText(tx,ty,numtext,n);
        dc.clearClipRectangle();
        }
      }
    }

  // Horizontal bar
  else{

    // Calculate length and filled part of bar
    barlength=width-(border<<1)-padleft-padright;
    barwidth=height-(border<<1)-padtop-padbottom;
    barfilled=(FXuint)(barlength*fraction);

    // Draw completed bar
    if(0<barfilled){

      // Light/dark bubble
      dc.fillVerticalGradient(border+padleft,border+padtop,barfilled,barwidth/2,makeHiliteColor(barColor,40),barColor);
      dc.fillVerticalGradient(border+padleft,border+padtop+barwidth/2,barfilled,barwidth-barwidth/2,barColor,makeShadowColor(barColor,20));
      }

    // Draw uncompleted bar
    if(barfilled<barlength){
      dc.setForeground(barBGColor);
      dc.fillRectangle(border+padleft+barfilled,border+padtop,barlength-barfilled,barwidth);
      }

    // Draw text
    if(options&PROGRESSBAR_PERCENTAGE){
      n=__snprintf(numtext,sizeof(numtext),"%d%%",percent);
      tw=font->getTextWidth(numtext,n);
      th=font->getFontHeight();
      ty=border+padtop+(barwidth-th)/2+font->getFontAscent();
      tx=border+padleft+(barlength-tw)/2;
      dc.setFont(font);
      if(border+padleft+barfilled<=tx){           // In right side
        dc.setForeground(textNumColor);
        dc.setClipRectangle(border+padleft,border+padtop,barlength,barwidth);
        dc.drawText(tx,ty,numtext,n);
        }
      else if(tx+tw<=border+padleft+barfilled){   // In left side
        dc.setForeground(textAltColor);
        dc.setClipRectangle(border+padleft,border+padtop,barlength,barwidth);
        dc.drawText(tx,ty,numtext,n);
        }
      else{                               // In between!
        dc.setForeground(textAltColor);
        dc.setClipRectangle(border+padleft,border+padtop,barfilled,barwidth);
        dc.drawText(tx,ty,numtext,n);
        dc.setForeground(textNumColor);
        dc.setClipRectangle(border+padleft+barfilled,border+padtop,barlength-barfilled,barwidth);
        dc.drawText(tx,ty,numtext,n);
        dc.clearClipRectangle();
        }
      }
    }
  }


// Draw the progress bar
long FXProgressBar::onPaint(FXObject*,FXSelector,void *ptr){
  FXDCWindow dc(this,(FXEvent*)ptr);

  // Erase to base color
  dc.setForeground(baseColor);
  dc.fillRectangle(0,0,width,height);

  // Draw borders
  drawFrame(dc,padleft,padtop,width-padleft-padright,height-padtop-padbottom);

  // Draw interior
  drawInterior(dc);
  return 1;
  }


// Set amount of progress made
void FXProgressBar::setProgress(FXuint value){
  if(value>total) value=total;
  if(value!=progress){
    progress=value;
    if(xid){
      FXDCWindow dc(this);
      drawInterior(dc);
      }
    getApp()->flush();
    }
  }


// Increment amount of progress
void FXProgressBar::increment(FXuint value){
  setProgress(progress+value);
  }


// Set total amount to completion
void FXProgressBar::setTotal(FXuint value){
  if(value!=total){
    total=value;
    if(xid){
      FXDCWindow dc(this);
      drawInterior(dc);
      }
    getApp()->flush();
    }
  }


// Change bar color
void FXProgressBar::setBarColor(FXColor clr){
  if(barColor!=clr){
    barColor=clr;
    update(border,border,width-(border<<1),height-(border<<1));
    }
  }


// Change bar background color
void FXProgressBar::setBarBGColor(FXColor clr){
  if(barBGColor!=clr){
    barBGColor=clr;
    update(border,border,width-(border<<1),height-(border<<1));
    }
  }


// Change text foreground color
void FXProgressBar::setTextColor(FXColor clr){
  if(textNumColor!=clr){
    textNumColor=clr;
    update();
    }
  }


// Change alternate text color
void FXProgressBar::setTextAltColor(FXColor clr){
  if(textAltColor!=clr){
    textAltColor=clr;
    update();
    }
  }


// Hide percentage display
void FXProgressBar::hideNumber(){
  if(options&PROGRESSBAR_PERCENTAGE){
    options&=~PROGRESSBAR_PERCENTAGE;
    recalc();
    update();
    }
  }


// Show percentage display
void FXProgressBar::showNumber(){
  if(!(options&PROGRESSBAR_PERCENTAGE)){
    options|=PROGRESSBAR_PERCENTAGE;
    recalc();
    update();
    }
  }


void FXProgressBar::setBarSize(FXint size){
  if(size<1){ fxerror("%s::setBarSize: zero or negative barsize specified.\n",getClassName()); }
  if(barsize!=size){
    barsize=size;
    recalc();
    update();
    }
  }


// Change the font
void FXProgressBar::setFont(FXFont *fnt){
  if(!fnt){ fxerror("%s::setFont: NULL font specified.\n",getClassName()); }
  if(font!=fnt){
    font=fnt;
    recalc();
    update();
    }
  }


// Change style of the bar widget
void FXProgressBar::setBarStyle(FXuint style){
  FXuint opts=(options&~PROGRESSBAR_MASK) | (style&PROGRESSBAR_MASK);
  if(options!=opts){
    options=opts;
    recalc();
    }
  }


// Get style of the bar widget
FXuint FXProgressBar::getBarStyle() const {
  return (options&PROGRESSBAR_MASK);
  }


// Save object to stream
void FXProgressBar::save(FXStream& store) const {
  FXFrame::save(store);
  store << progress;
  store << total;
  store << barsize;
  store << font;
  store << barBGColor;
  store << barColor;
  store << textNumColor;
  store << textAltColor;
  }


// Load object from stream
void FXProgressBar::load(FXStream& store){
  FXFrame::load(store);
  store >> progress;
  store >> total;
  store >> barsize;
  store >> font;
  store >> barBGColor;
  store >> barColor;
  store >> textNumColor;
  store >> textAltColor;
  }



// Destroy
FXProgressBar::~FXProgressBar(){
  font=(FXFont*)-1L;
  }

}
