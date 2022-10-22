/********************************************************************************
*                                                                               *
*                         C o n s o l e   W i d g e t                           *
*                                                                               *
*********************************************************************************
* Copyright (C) 2006,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include <new>
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "fxmath.h"
#include "FXElement.h"
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
#include "FXFont.h"
#include "FXEvent.h"
#include "FXWindow.h"
#include "FXDCWindow.h"
#include "FXApp.h"
#include "FXScrollBar.h"
#include "FXScrollArea.h"
#include "FXConsole.h"


/*
  To do:
*/



using namespace FX;

/*******************************************************************************/

namespace FX {


FXDEFMAP(FXConsole) FXConsoleMap[]={
  FXMAPFUNC(SEL_PAINT,0,FXConsole::onPaint),
  FXMAPFUNC(SEL_COMMAND,FXConsole::ID_XXX,FXConsole::onXXX),
  };


// Object implementation
FXIMPLEMENT(FXConsole,FXScrollArea,FXConsoleMap,ARRAYNUMBER(FXConsoleMap))


// Deserialization
FXConsole::FXConsole(){
  flags|=FLAG_ENABLED|FLAG_DROPTARGET;
  font=nullptr;
  margintop=0;
  marginbottom=0;
  marginleft=0;
  marginright=0;
  historylines=0;
  visiblelines=0;
  topline=0;
  vrows=0;
  vcols=0;
  textColor=0;
  selbackColor=0;
  seltextColor=0;
  cursorColor=0;
  }


// Construct and init
FXConsole::FXConsole(FXComposite *p,FXObject* tgt,FXSelector sel,FXuint opts,FXint x,FXint y,FXint w,FXint h,FXint pl,FXint pr,FXint pt,FXint pb):FXScrollArea(p,opts,x,y,w,h){
  flags|=FLAG_ENABLED|FLAG_DROPTARGET;
  defaultCursor=getApp()->getDefaultCursor(DEF_TEXT_CURSOR);
  dragCursor=getApp()->getDefaultCursor(DEF_TEXT_CURSOR);
  font=getApp()->getNormalFont();
  textColor=getApp()->getForeColor();
  selbackColor=getApp()->getSelbackColor();
  seltextColor=getApp()->getSelforeColor();
  cursorColor=getApp()->getForeColor();
  target=tgt;
  message=sel;
  margintop=pt;
  marginbottom=pb;
  marginleft=pl;
  marginright=pr;
  historylines=100;
  visiblelines=1;
  topline=0;
  vrows=0;
  vcols=0;
  contents.no(historylines);
  style.no(historylines);
for(int i=0; i<contents.no(); i++){ contents[i].format("blablablablaaaaaaaaaaaaaaa%d\n",i); }
  }


// Create window
void FXConsole::create(){
  FXScrollArea::create();
  font->create();
  recalc();
  }


// Detach window
void FXConsole::detach(){
  FXScrollArea::detach();
  font->detach();
  }


// If window can have focus
FXbool FXConsole::canFocus() const {
  return true;
  }


// Get default width
FXint FXConsole::getDefaultWidth(){
  return 0<vcols ? marginleft+marginright+vcols*font->getTextWidth("8",1) : FXScrollArea::getDefaultWidth();
  }


// Get default height
FXint FXConsole::getDefaultHeight(){
  return 0<vrows ? margintop+marginbottom+vrows*font->getFontHeight() : FXScrollArea::getDefaultHeight();
  }


// Move content
void FXConsole::moveContents(FXint x,FXint y){
  FXint dx=x-pos_x;
  FXint dy=y-pos_y;
  pos_x=x;
  pos_y=y;
  scroll(marginleft,margintop,getVisibleWidth()-marginleft-marginright,getVisibleHeight()-margintop-marginbottom,dx,dy);
  }


// Determine minimum content width of scroll area
FXint FXConsole::getContentWidth(){
  return marginleft+marginright+1;             // FIXME
  }


// Determine minimum content height of scroll area
FXint FXConsole::getContentHeight(){
  return margintop+marginbottom+font->getFontHeight()*contents.no();
  }


// Recalculate layout
void FXConsole::layout(){
  FXint hh=font->getFontHeight();
  FXint totallines;

  // Number of visible lines has changed
  visiblelines=(height-margintop-marginbottom+hh-1)/hh;
  if(visiblelines<1) visiblelines=1;

  // Total lines to buffer
  totallines=FXMAX(visiblelines,historylines);
  if(contents.no()!=totallines){
    contents.no(totallines);                    // FIXME keep text insofar as possible!
    if(style.no()){ style.no(totallines); }
    }

  FXTRACE((100,"visiblelines=%d historylines=%d totallines=%d hh=%d space=%d\n",visiblelines,historylines,totallines,hh,height-margintop-marginbottom));

  // Scrollbars adjusted
  placeScrollBars(width,height);

  // No more dirty
  flags&=~FLAG_DIRTY;
  }


// Character width
FXint FXConsole::charWidth(FXwchar ch,FXint col) const {
  if(ch<' '){
    if(ch!='\t'){
      return font->getCharWidth('#');
      }
    return font->getCharWidth(' ')*(8-col%8);
    }
  return font->getCharWidth(ch);
  }


// Determine style
FXuint FXConsole::styleOf(FXint line,FXint index,FXint p,FXint c) const {
  FXuint s=0;
  FXchar ch;

  // Selected part of text
//  if(selstartpos<=p && p<selendpos) s|=STYLE_SELECTED;

  // Highlighted part of text
 // if(hilitestartpos<=p && p<hiliteendpos) s|=STYLE_HILITE;

  // Current active line
//  if((row==cursorrow)&&(options&TEXT_SHOWACTIVE)) s|=STYLE_ACTIVE;

  // Blank part of line
  if(p>=contents[index].length()) return s;

  // Special style for control characters
  ch=contents[index][p];

  // Get value from style buffer
  if(style.no()) s|=style[index][p];

  // Tabs are just fill
  if(ch == '\t') return s;

  // Spaces are just fill
  if(ch == ' ') return s;

  // Newlines are just fill
  if(ch == '\n') return s;

  // Get special style for control codes
  if((FXuchar)ch < ' ') return s|STYLE_CONTROL|STYLE_TEXT;

  return s|STYLE_TEXT;
  }


// Draw fragment of text in given style
void FXConsole::drawTextFragment(FXDCWindow& dc,FXint x,FXint y,FXint,FXint,const FXchar *text,FXint n,FXuint sty) const {
  FXColor color=FXRGB(255,255,255);
  dc.setForeground(color);
  y+=font->getFontAscent();
  dc.drawText(x,y,text,n);
  }


// Draw text line with correct style
void FXConsole::drawTextLine(FXDCWindow& dc,FXint line,FXint left,FXint right) const {
  FXint index=(topline+line)%contents.no();
  FXint edge=pos_x+marginleft;
  FXint h=font->getFontHeight();
  FXint y=pos_y+margintop+line*h;
  FXint x=0;
  FXint w=0;
  FXuint curstyle;
  FXuint newstyle;
  FXint cw,sp,ep,sc,ec;

  // Scan ahead till until we hit the end or the left edge
  for(sp=sc=0; sp<contents[index].length(); sp=contents[index].inc(sp),sc++){
    cw=charWidth(contents[index].wc(sp),sc);
    if(x+edge+cw>=left) break;
    x+=cw;
    }

  // First style to display
  curstyle=styleOf(line,index,sp,sc);

  // Draw until we hit the end or the right edge
  for(ep=sp,ec=sc; ep<contents[index].length(); ep=contents[index].inc(ep),ec++){
    newstyle=styleOf(line,index,ep,ec);
    if(newstyle!=curstyle){
//      fillBufferRect(dc,edge+x,y,w,h,curstyle);
      if(curstyle&STYLE_TEXT) drawTextFragment(dc,edge+x,y,w,h,&contents[index][sp],ep-sp,curstyle);
      curstyle=newstyle;
      sp=ep;
      x+=w;
      w=0;
      }
    cw=charWidth(contents[index].wc(ep),ec);
    if(x+edge+w>=right) break;
    w+=cw;
    }

  // Draw unfinished fragment
//  fillBufferRect(dc,edge+x,y,w,h,curstyle);
  if(curstyle&STYLE_TEXT) drawTextFragment(dc,edge+x,y,w,h,&contents[index][sp],ep-sp,curstyle);
  x+=w;

  // Fill any left-overs outside of text
  if(x+edge<right){
    curstyle=styleOf(line,index,ep,ec);
//    fillBufferRect(dc,edge+x,y,right-edge-x,h,curstyle);
    }
  }


// Repaint lines of text
void FXConsole::drawContents(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h) const {
  FXint hh=font->getFontHeight();
  FXint yy=pos_y+margintop;
  FXint tl=(y-yy)/hh;
  FXint bl=(y+h-yy)/hh;
  FXint ln;
  if(tl<0) tl=0;
  if(bl>=contents.no()) bl=contents.no()-1;
  FXTRACE((100,"tl=%d bl=%d\n",tl,bl));
  for(ln=tl; ln<=bl; ln++){
    drawTextLine(dc,ln,x,x+w);
    }
  }


// Draw the text
long FXConsole::onPaint(FXObject*,FXSelector,void* ptr){
  FXEvent* event=(FXEvent*)ptr;
  FXDCWindow dc(this,event);
  dc.setFont(font);

dc.setForeground(FXRGB(255,0,0));
dc.fillRectangle(event->rect.x,event->rect.y,event->rect.w,event->rect.h);

  // Viewport
  FXint vw=getVisibleWidth();
  FXint vh=getVisibleHeight();

  // Paint top margin
  if(event->rect.y<=margintop){
    dc.setForeground(backColor);
    dc.fillRectangle(0,0,vw,margintop);
    }

  // Paint bottom margin
  if(event->rect.y+event->rect.h>=vh-marginbottom){
    dc.setForeground(backColor);
    dc.fillRectangle(0,vh-marginbottom,vw,marginbottom);
    }

  // Paint left margin
  if(event->rect.x<marginleft){
    dc.setForeground(backColor);
    dc.fillRectangle(0,margintop,marginleft,vh-margintop-marginbottom);
    }

  // Paint right margin
  if(event->rect.x+event->rect.w>=vw-marginright){
    dc.setForeground(backColor);
    dc.fillRectangle(vw-marginright,margintop,marginright,vh-margintop-marginbottom);
    }

  // Paint text
  dc.setClipRectangle(marginleft,margintop,vw-marginright-marginleft,vh-margintop-marginbottom);
  drawContents(dc,event->rect.x,event->rect.y,event->rect.w,event->rect.h);

  return 1;
  }


// Draw the text
long FXConsole::onXXX(FXObject*,FXSelector,void*){
  return 1;
  }


// Change top margin
void FXConsole::setMarginTop(FXint mt){
  if(margintop!=mt){
    margintop=mt;
    recalc();
    update();
    }
  }


// Change bottom margin
void FXConsole::setMarginBottom(FXint mb){
  if(marginbottom!=mb){
    marginbottom=mb;
    recalc();
    update();
    }
  }


// Change left margin
void FXConsole::setMarginLeft(FXint ml){
  if(marginleft!=ml){
    marginleft=ml;
    recalc();
    update();
    }
  }


// Change right margin
void FXConsole::setMarginRight(FXint mr){
  if(marginright!=mr){
    marginright=mr;
    recalc();
    update();
    }
  }


// Change history lines
void FXConsole::setHistoryLines(FXint hl){
  if(hl<1) hl=1;
  if(historylines!=hl){
    historylines=hl;
    recalc();
    update();
    }
  }


// Change number of visible rows
void FXConsole::setVisibleRows(FXint rows){
  if(rows<0) rows=0;
  if(vrows!=rows){
    vrows=rows;
    recalc();
    }
  }


// Change number of visible columns
void FXConsole::setVisibleColumns(FXint cols){
  if(cols<0) cols=0;
  if(vcols!=cols){
    vcols=cols;
    recalc();
    }
  }


// Change the font
void FXConsole::setFont(FXFont* fnt){
  if(!fnt){ fxerror("%s::setFont: NULL font specified.\n",getClassName()); }
  if(font!=fnt){
    font=fnt;
    recalc();
    update();
    }
  }


// Save object to stream
void FXConsole::save(FXStream& store) const {
  FXScrollArea::save(store);
  store << margintop;
  store << marginbottom;
  store << marginleft;
  store << marginright;
  store << font;
  store << textColor;
  store << selbackColor;
  store << seltextColor;
  store << cursorColor;
  store << help;
  store << tip;
  }


// Load object from stream
void FXConsole::load(FXStream& store){
  FXScrollArea::load(store);
  store >> margintop;
  store >> marginbottom;
  store >> marginleft;
  store >> marginright;
  store >> font;
  store >> textColor;
  store >> selbackColor;
  store >> seltextColor;
  store >> cursorColor;
  store >> help;
  store >> tip;
  }


// Clean up
FXConsole::~FXConsole(){
  font=(FXFont*)-1L;
  }

}
