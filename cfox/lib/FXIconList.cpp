/********************************************************************************
*                                                                               *
*                          I c o n L i s t   O b j e c t                        *
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
#include "fxascii.h"
#include "fxunicode.h"
#include "FXArray.h"
#include "FXHash.h"
#include "FXMutex.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXColors.h"
#include "FXSize.h"
#include "FXPoint.h"
#include "FXObjectList.h"
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
#include "FXImage.h"
#include "FXIcon.h"
#include "FXButton.h"
#include "FXScrollBar.h"
#include "FXScrollArea.h"
#include "FXHeader.h"
#include "FXIconList.h"


/*
  To do:
  - In detail-mode, some items should be left, some right, and some centered in the field.
  - Return key simulates double click.
  - Need method to set header columns.
  - Sortfunc's will be hard to serialize, and hard to write w/o secretly #including
    the FXTreeItem header!
  - Rapid key actions are wrongly interpreted as double clicks
  - Upgrade later to accomodate heterogeneous item sizes (this means a more
    complex layout algorithm, and likely also explicit x,y recorded in each item).
  - In all list widgets, get rid of this complex marking business.
  - Should adding/removing items send SEL_INSERTED and SEL_DELETED callbacks.
  - Need to add support for arbitrary icon sizes same as FXTreeList already has;
    layout needs to be such that each column is as wide as widest item in that
    column only (and not as wide as the widest item in the list).
  - It may be convenient to have ways to move items around.
  - Need insertSorted() API to add item in the right place based on current
    sort function.
  - Changing icon should NOT cause recalc() when size does not change.
  - When XDND, autoscrolling happens a bit too fast because we get timers
    from motion as well as dnd-motion events; probably, it should not
    autoscroll when dragging icons.
  - Perhaps drawDetails() should ignore x coordinate and just look at getItemOffest()
    from the header instead.
  - Perhaps the ICONLIST_AUTOSIZE mode should be set with a separate API so that
    the visual stuff changed setListStyle().
  - Since '\0' is no longer special in FXString, perhaps we can replace the function
    of '\t' with '\0'.  This would be significantly more efficient.
*/



#define SIDE_SPACING             4    // Left or right spacing between items
#define DETAIL_TEXT_SPACING      2    // Spacing between text and icon in detail icon mode
#define MINI_TEXT_SPACING        2    // Spacing between text and icon in mini icon mode
#define BIG_LINE_SPACING         6    // Line spacing in big icon mode
#define BIG_TEXT_SPACING         2    // Spacing between text and icon in big icon mode
#define ITEM_SPACE             128    // Default space for item

#define SELECT_MASK   (ICONLIST_EXTENDEDSELECT|ICONLIST_SINGLESELECT|ICONLIST_BROWSESELECT|ICONLIST_MULTIPLESELECT)
#define ICONLIST_MASK (SELECT_MASK|ICONLIST_MINI_ICONS|ICONLIST_BIG_ICONS|ICONLIST_COLUMNS|ICONLIST_AUTOSIZE)

using namespace FX;

/*******************************************************************************/

namespace FX {


// Object implementation
FXIMPLEMENT(FXIconItem,FXObject,nullptr,0)


// Draw item
void FXIconItem::draw(const FXIconList* list,FXDC& dc,FXint x,FXint y,FXint w,FXint h) const {
  FXuint options=list->getListStyle();
  if(options&ICONLIST_BIG_ICONS) drawBigIcon(list,dc,x,y,w,h);
  else if(options&ICONLIST_MINI_ICONS) drawMiniIcon(list,dc,x,y,w,h);
  else drawDetails(list,dc,x,y,w,h);
  }


// Draw big icon
void FXIconItem::drawBigIcon(const FXIconList* list,FXDC& dc,FXint x,FXint y,FXint w,FXint h) const {
  FXint iw=0,ih=0,tw=0,th=0,ss=0,len,dw,s,space,xt,yt,xi,yi;
  FXFont *font=list->getFont();
  dc.fillRectangle(x,y,w,h);
  space=w-SIDE_SPACING;
  if(!label.empty()){
    for(len=0; len<label.length() && label[len]!='\t'; len++){}
    tw=4+font->getTextWidth(label.text(),len);
    th=4+font->getFontHeight();
    yt=y+h-th-BIG_LINE_SPACING/2;
    dw=0;
    if(tw>space){
      dw=font->getTextWidth("...",3);
      s=space-dw;
      while((tw=4+font->getTextWidth(label.text(),len))>s && len>1) len=label.dec(len);
      if(tw>s) dw=0;
      }
    if(tw<=space){         // FIXME as below in drawDetails
      xt=x+(w-tw-dw)/2;
      if(isSelected()){
        dc.setForeground(list->getSelBackColor());
        dc.fillRectangle(xt,yt,tw+dw,th);
        }
      if(!isEnabled())
        dc.setForeground(makeShadowColor(list->getBackColor()));
      else if(isSelected())
        dc.setForeground(list->getSelTextColor());
      else
        dc.setForeground(list->getTextColor());
      dc.drawText(xt+2,yt+font->getFontAscent()+2,label.text(),len);
      if(dw) dc.drawText(xt+tw-2,yt+font->getFontAscent()+2,"...",3);
      if(hasFocus()){
        dc.drawFocusRectangle(xt+1,yt+1,tw+dw-2,th-2);
        }
      }
    ss=BIG_TEXT_SPACING;    // Space between text and icon only added if we have both icon and text
    }
  if(bigIcon){
    iw=bigIcon->getWidth();
    ih=bigIcon->getHeight();
    xi=x+(w-iw)/2;
    yi=y+BIG_LINE_SPACING/2+(h-th-BIG_LINE_SPACING-ss-ih)/2;
    if(isSelected()){
      dc.drawIconShaded(bigIcon,xi,yi);
      }
    else{
      dc.drawIcon(bigIcon,xi,yi);
      }
    }
  }


// Draw mini icon
void FXIconItem::drawMiniIcon(const FXIconList* list,FXDC& dc,FXint x,FXint y,FXint w,FXint h) const {
  FXint iw=0,ih=0,tw=0,th=0,len,dw,s,space;
  FXFont *font=list->getFont();
  dc.fillRectangle(x,y,w,h);
  x+=SIDE_SPACING/2;
  space=w-SIDE_SPACING;
  if(miniIcon){
    iw=miniIcon->getWidth();
    ih=miniIcon->getHeight();
    if(isSelected()){
      dc.drawIconShaded(miniIcon,x,y+(h-ih)/2);
      }
    else{
      dc.drawIcon(miniIcon,x,y+(h-ih)/2);
      }
    x+=iw+MINI_TEXT_SPACING;
    space-=iw+MINI_TEXT_SPACING;
    }
  if(!label.empty()){
    for(len=0; len<label.length() && label[len]!='\t'; len++){}
    tw=4+font->getTextWidth(label.text(),len);
    th=4+font->getFontHeight();
    y+=(h-th)/2;
    dw=0;
    if(tw>space){                  // FIXME as below in drawDetails
      dw=font->getTextWidth("...",3);
      s=space-dw;
      while((tw=4+font->getTextWidth(label.text(),len))>s && len>1) len=label.dec(len);
      if(tw>s) dw=0;
      }
    if(tw<=space){
      if(isSelected()){
        dc.setForeground(list->getSelBackColor());
        dc.fillRectangle(x,y,tw+dw,th);
        }
      if(!isEnabled())
        dc.setForeground(makeShadowColor(list->getBackColor()));
      else if(isSelected())
        dc.setForeground(list->getSelTextColor());
      else
        dc.setForeground(list->getTextColor());
      dc.drawText(x+2,y+font->getFontAscent()+2,label.text(),len);
      if(dw) dc.drawText(x+tw-2,y+font->getFontAscent()+2,"...",3);
      if(hasFocus()){
        dc.drawFocusRectangle(x+1,y+1,tw+dw-2,th-2);
        }
      }
    }
  }


// Draw detail
void FXIconItem::drawDetails(const FXIconList* list,FXDC& dc,FXint x,FXint y,FXint w,FXint h) const {
  FXint iw=0,ih=0,tw=0,th=0,yt,beg,end,hi,drw,space,used,dw,xx;
  FXHeader *header=list->getHeader();
  FXFont *font=list->getFont();
  if(header->getNumItems()==0) return;
  if(isSelected()){
    dc.setForeground(list->getSelBackColor());
    dc.fillRectangle(x,y,w,h);
    }
  else{
    dc.fillRectangle(x,y,w,h);
    }
  if(hasFocus()){
    dc.drawFocusRectangle(x+1,y+1,w-2,h-2);
    }
  xx=x+SIDE_SPACING/2;
  if(miniIcon){
    iw=miniIcon->getWidth();
    ih=miniIcon->getHeight();
    dc.setClipRectangle(x,y,header->getItemSize(0),h);
    dc.drawIcon(miniIcon,xx,y+(h-ih)/2);
    dc.clearClipRectangle();
    xx+=iw+DETAIL_TEXT_SPACING;
    }
  if(!label.empty()){
    th=font->getFontHeight();
    dw=font->getTextWidth("...",3);
    yt=y+(h-th-4)/2;
    if(!isEnabled())
      dc.setForeground(makeShadowColor(list->getBackColor()));
    else if(isSelected())
      dc.setForeground(list->getSelTextColor());
    else
      dc.setForeground(list->getTextColor());
    used=iw+DETAIL_TEXT_SPACING+SIDE_SPACING/2;
    for(hi=beg=0; beg<label.length() && hi<header->getNumItems(); hi++,beg=end+1){
      space=header->getItemSize(hi)-used;
      for(end=beg; end<label.length() && label[end]!='\t'; end++){}
      if(end>beg){
        drw=end-beg;
        tw=font->getTextWidth(&label[beg],drw);
        if(tw>space-4){
          while((tw=font->getTextWidth(&label[beg],drw))+dw>space-4 && drw>1) drw=label.dec(drw);
          dc.setClipRectangle(xx,y,space,h);
          dc.drawText(xx+2,yt+font->getFontAscent()+2,&label[beg],drw);
          dc.drawText(xx+tw+2,yt+font->getFontAscent()+2,"...",3);
          dc.clearClipRectangle();
          }
        else{
          dc.drawText(xx+2,yt+font->getFontAscent()+2,&label[beg],drw);
          }
        }
      xx+=space;
      used=0;
      }
    }
  }


// See if item got hit and where: 0 is outside, 1 is icon, 2 is text
FXint FXIconItem::hitItem(const FXIconList* list,FXint rx,FXint ry,FXint rw,FXint rh) const {
  FXFont *font=list->getFont();
  FXuint options=list->getListStyle();
  FXint iw=0,tw=0,ih=0,th=0,ss=0,ix,iy,tx,ty,w,h,sp,tlen;
  for(tlen=0; tlen<label.length() && label[tlen]!='\t'; tlen++){}
  if(options&ICONLIST_BIG_ICONS){
    w=list->getItemWidth();
    h=list->getItemHeight();
    sp=w-SIDE_SPACING;
    if(!label.empty()){
      tw=4+font->getTextWidth(label.text(),tlen);
      th=4+font->getFontHeight();
      if(tw>sp) tw=sp;
      if(bigIcon) ss=BIG_TEXT_SPACING;
      }
    if(bigIcon){
      iw=bigIcon->getWidth();
      ih=bigIcon->getHeight();
      }
    ty=h-th-BIG_LINE_SPACING/2;
    iy=BIG_LINE_SPACING/2+(h-th-BIG_LINE_SPACING-ss-ih)/2;
    ix=(w-iw)/2;
    tx=(w-tw)/2;
    }
  else if(options&ICONLIST_MINI_ICONS){
    sp=list->getItemWidth()-SIDE_SPACING;
    ix=SIDE_SPACING/2;
    tx=SIDE_SPACING/2;
    if(miniIcon){
      iw=miniIcon->getWidth();
      ih=miniIcon->getHeight();
      tx+=iw+MINI_TEXT_SPACING;
      sp=sp-iw-MINI_TEXT_SPACING;
      }
    if(!label.empty()){
      tw=4+font->getTextWidth(label.text(),tlen);
      th=4+font->getFontHeight();
      if(tw>sp) tw=sp;
      }
    h=list->getItemHeight();
    iy=(h-ih)/2;
    ty=(h-th)/2;
    }
  else{
    ix=SIDE_SPACING/2;
    tx=SIDE_SPACING/2;
    if(miniIcon){
      iw=miniIcon->getWidth();
      ih=miniIcon->getHeight();
      tx+=iw+DETAIL_TEXT_SPACING;
      }
    if(!label.empty()){
      tw=10000000;
      th=4+font->getFontHeight();
      }
    h=list->getItemHeight();
    iy=(h-ih)/2;
    ty=(h-th)/2;
    }

  // In icon?
  if(ix<=rx+rw && iy<=ry+rh && rx<ix+iw && ry<iy+ih) return 1;

  // In text?
  if(tx<=rx+rw && ty<=ry+rh && rx<tx+tw && ry<ty+th) return 2;

  // Outside
  return 0;
  }


// Set or kill focus
void FXIconItem::setFocus(FXbool focus){
  state^=((0-focus)^state)&FOCUS;
  }

// Select or deselect item
void FXIconItem::setSelected(FXbool selected){
  state^=((0-selected)^state)&SELECTED;
  }

// Enable or disable the item
void FXIconItem::setEnabled(FXbool enabled){
  state^=((enabled-1)^state)&DISABLED;
  }

// Icon is draggable
void FXIconItem::setDraggable(FXbool draggable){
  state^=((0-draggable)^state)&DRAGGABLE;
  }


// Change item's text label
void FXIconItem::setText(const FXString& txt){
  label=txt;
  }


// Change item's big icon
void FXIconItem::setBigIcon(FXIcon* icn,FXbool owned){
  if(bigIcon && (state&BIGICONOWNED)){
    if(bigIcon!=icn) delete bigIcon;
    state&=~BIGICONOWNED;
    }
  bigIcon=icn;
  if(bigIcon && owned){
    state|=BIGICONOWNED;
    }
  }


// Change item's mini icon
void FXIconItem::setMiniIcon(FXIcon* icn,FXbool owned){
  if(miniIcon && (state&MINIICONOWNED)){
    if(miniIcon!=icn) delete miniIcon;
    state&=~MINIICONOWNED;
    }
  miniIcon=icn;
  if(miniIcon && owned){
    state|=MINIICONOWNED;
    }
  }


// Create icon
void FXIconItem::create(){
  if(bigIcon) bigIcon->create();
  if(miniIcon) miniIcon->create();
  }


// Destroy icon
void FXIconItem::destroy(){
  if((state&BIGICONOWNED) && bigIcon) bigIcon->destroy();
  if((state&MINIICONOWNED) && miniIcon) miniIcon->destroy();
  }


// Detach from icon resource
void FXIconItem::detach(){
  if(bigIcon) bigIcon->detach();
  if(miniIcon) miniIcon->detach();
  }


// Return tip text
FXString FXIconItem::getTipText() const {
  return label.section('\t',0);
  }


// Get item width
FXint FXIconItem::getWidth(const FXIconList* list) const {
  FXFont *font=list->getFont();
  FXuint options=list->getListStyle();
  FXint iw=0,tw=0,w=0,tlen;
  for(tlen=0; tlen<label.length() && label[tlen]!='\t'; tlen++){}
  if(options&ICONLIST_BIG_ICONS){
    if(bigIcon) iw=bigIcon->getWidth();
    if(!label.empty()) tw=4+font->getTextWidth(label.text(),tlen);
    w=SIDE_SPACING+FXMAX(tw,iw);
    }
  else if(options&ICONLIST_MINI_ICONS){
    if(miniIcon) iw=miniIcon->getWidth();
    if(!label.empty()) tw=4+font->getTextWidth(label.text(),tlen);
    if(iw && tw) iw+=MINI_TEXT_SPACING;
    w=SIDE_SPACING+iw+tw;
    }
  else{
    w=SIDE_SPACING;
    }
  return w;
  }


// Get item height
FXint FXIconItem::getHeight(const FXIconList* list) const {
  FXFont *font=list->getFont();
  FXuint options=list->getListStyle();
  FXint ih=0,th=0,h=0;
  if(options&ICONLIST_BIG_ICONS){
    if(bigIcon) ih=bigIcon->getHeight();
    if(!label.empty()) th=4+font->getFontHeight();
    if(ih && th) ih+=BIG_TEXT_SPACING;
    h=BIG_LINE_SPACING+ih+th;
    }
  else if(options&ICONLIST_MINI_ICONS){
    if(miniIcon) ih=miniIcon->getHeight();
    if(!label.empty()) th=4+font->getFontHeight();
    h=FXMAX(ih,th);
    }
  else{
    if(miniIcon) ih=miniIcon->getHeight();
    if(!label.empty()) th=4+font->getFontHeight();
    h=FXMAX(ih,th);
    }
  return h;
  }


// Save data
void FXIconItem::save(FXStream& store) const {
  FXObject::save(store);
  store << label;
  store << bigIcon;
  store << miniIcon;
  store << state;
  }


// Load data
void FXIconItem::load(FXStream& store){
  FXObject::load(store);
  store >> label;
  store >> bigIcon;
  store >> miniIcon;
  store >> state;
  }


// Delete icons if owned
FXIconItem::~FXIconItem(){
  if(state&BIGICONOWNED) delete bigIcon;
  if(state&MINIICONOWNED) delete miniIcon;
  bigIcon=(FXIcon*)-1L;
  miniIcon=(FXIcon*)-1L;
  }

/*******************************************************************************/

// Map
FXDEFMAP(FXIconList) FXIconListMap[]={
  FXMAPFUNC(SEL_PAINT,0,FXIconList::onPaint),
  FXMAPFUNC(SEL_MOTION,0,FXIconList::onMotion),
  FXMAPFUNC(SEL_LEFTBUTTONPRESS,0,FXIconList::onLeftBtnPress),
  FXMAPFUNC(SEL_LEFTBUTTONRELEASE,0,FXIconList::onLeftBtnRelease),
  FXMAPFUNC(SEL_RIGHTBUTTONPRESS,0,FXIconList::onRightBtnPress),
  FXMAPFUNC(SEL_RIGHTBUTTONRELEASE,0,FXIconList::onRightBtnRelease),
  FXMAPFUNC(SEL_TIMEOUT,FXIconList::ID_AUTOSCROLL,FXIconList::onAutoScroll),
  FXMAPFUNC(SEL_TIMEOUT,FXIconList::ID_TIPTIMER,FXIconList::onTipTimer),
  FXMAPFUNC(SEL_TIMEOUT,FXIconList::ID_LOOKUPTIMER,FXIconList::onLookupTimer),
  FXMAPFUNC(SEL_UNGRABBED,0,FXIconList::onUngrabbed),
  FXMAPFUNC(SEL_KEYPRESS,0,FXIconList::onKeyPress),
  FXMAPFUNC(SEL_KEYRELEASE,0,FXIconList::onKeyRelease),
  FXMAPFUNC(SEL_ENTER,0,FXIconList::onEnter),
  FXMAPFUNC(SEL_LEAVE,0,FXIconList::onLeave),
  FXMAPFUNC(SEL_FOCUSIN,0,FXIconList::onFocusIn),
  FXMAPFUNC(SEL_FOCUSOUT,0,FXIconList::onFocusOut),
  FXMAPFUNC(SEL_CLICKED,0,FXIconList::onClicked),
  FXMAPFUNC(SEL_DOUBLECLICKED,0,FXIconList::onDoubleClicked),
  FXMAPFUNC(SEL_TRIPLECLICKED,0,FXIconList::onTripleClicked),
  FXMAPFUNC(SEL_COMMAND,0,FXIconList::onCommand),
  FXMAPFUNC(SEL_QUERY_TIP,0,FXIconList::onQueryTip),
  FXMAPFUNC(SEL_QUERY_HELP,0,FXIconList::onQueryHelp),
  FXMAPFUNC(SEL_CHANGED,FXIconList::ID_HEADER,FXIconList::onChgHeader),
  FXMAPFUNC(SEL_CLICKED,FXIconList::ID_HEADER,FXIconList::onClkHeader),
  FXMAPFUNC(SEL_UPDATE,FXIconList::ID_SHOW_DETAILS,FXIconList::onUpdShowDetails),
  FXMAPFUNC(SEL_UPDATE,FXIconList::ID_SHOW_MINI_ICONS,FXIconList::onUpdShowMiniIcons),
  FXMAPFUNC(SEL_UPDATE,FXIconList::ID_SHOW_BIG_ICONS,FXIconList::onUpdShowBigIcons),
  FXMAPFUNC(SEL_UPDATE,FXIconList::ID_ARRANGE_BY_ROWS,FXIconList::onUpdArrangeByRows),
  FXMAPFUNC(SEL_UPDATE,FXIconList::ID_ARRANGE_BY_COLUMNS,FXIconList::onUpdArrangeByColumns),
  FXMAPFUNC(SEL_COMMAND,FXIconList::ID_SHOW_DETAILS,FXIconList::onCmdShowDetails),
  FXMAPFUNC(SEL_COMMAND,FXIconList::ID_SHOW_MINI_ICONS,FXIconList::onCmdShowMiniIcons),
  FXMAPFUNC(SEL_COMMAND,FXIconList::ID_SHOW_BIG_ICONS,FXIconList::onCmdShowBigIcons),
  FXMAPFUNC(SEL_COMMAND,FXIconList::ID_ARRANGE_BY_ROWS,FXIconList::onCmdArrangeByRows),
  FXMAPFUNC(SEL_COMMAND,FXIconList::ID_ARRANGE_BY_COLUMNS,FXIconList::onCmdArrangeByColumns),
  FXMAPFUNC(SEL_COMMAND,FXIconList::ID_SELECT_ALL,FXIconList::onCmdSelectAll),
  FXMAPFUNC(SEL_COMMAND,FXIconList::ID_DESELECT_ALL,FXIconList::onCmdDeselectAll),
  FXMAPFUNC(SEL_COMMAND,FXIconList::ID_SELECT_INVERSE,FXIconList::onCmdSelectInverse),
  FXMAPFUNC(SEL_COMMAND,FXWindow::ID_SETVALUE,FXIconList::onCmdSetValue),
  FXMAPFUNC(SEL_COMMAND,FXWindow::ID_SETINTVALUE,FXIconList::onCmdSetIntValue),
  FXMAPFUNC(SEL_COMMAND,FXWindow::ID_GETINTVALUE,FXIconList::onCmdGetIntValue),
  };


// Object implementation
FXIMPLEMENT(FXIconList,FXScrollArea,FXIconListMap,ARRAYNUMBER(FXIconListMap))


/*******************************************************************************/


// Serialization
FXIconList::FXIconList(){
  flags|=FLAG_ENABLED;
  header=(FXHeader*)-1L;
  nrows=1;
  ncols=1;
  anchor=-1;
  current=-1;
  extent=-1;
  viewable=-1;
  font=(FXFont*)-1L;
  sortfunc=nullptr;
  textColor=0;
  selbackColor=0;
  seltextColor=0;
  itemSpace=ITEM_SPACE;
  itemWidth=1;
  itemHeight=1;
  anchorx=0;
  anchory=0;
  currentx=0;
  currenty=0;
  grabx=0;
  graby=0;
  state=false;
  }


// Icon List
FXIconList::FXIconList(FXComposite *p,FXObject* tgt,FXSelector sel,FXuint opts,FXint x,FXint y,FXint w,FXint h):FXScrollArea(p,opts,x,y,w,h){
  flags|=FLAG_ENABLED;
  header=new FXHeader(this,this,FXIconList::ID_HEADER,HEADER_TRACKING|HEADER_BUTTON|HEADER_RESIZE|FRAME_RAISED|FRAME_THICK);
  target=tgt;
  message=sel;
  nrows=1;
  ncols=1;
  anchor=-1;
  current=-1;
  extent=-1;
  viewable=-1;
  font=getApp()->getNormalFont();
  sortfunc=nullptr;
  textColor=getApp()->getForeColor();
  selbackColor=getApp()->getSelbackColor();
  seltextColor=getApp()->getSelforeColor();
  itemSpace=ITEM_SPACE;
  itemWidth=1;
  itemHeight=1;
  anchorx=0;
  anchory=0;
  currentx=0;
  currenty=0;
  grabx=0;
  graby=0;
  state=false;
  }


// Create window
void FXIconList::create(){
  FXint i;
  FXScrollArea::create();
  for(i=0; i<items.no(); i++){items[i]->create();}
  font->create();
  }


// Detach window
void FXIconList::detach(){
  FXint i;
  FXScrollArea::detach();
  for(i=0; i<items.no(); i++){items[i]->detach();}
  font->detach();
  }


// If window can have focus
FXbool FXIconList::canFocus() const { return true; }


// Into focus chain
void FXIconList::setFocus(){
  FXScrollArea::setFocus();
  setDefault(true);
  }


// Out of focus chain
void FXIconList::killFocus(){
  FXScrollArea::killFocus();
  setDefault(maybe);
  }


// Propagate size change
void FXIconList::recalc(){
  FXScrollArea::recalc();
  flags|=FLAG_RECALC;
  }


// Return visible area y position
FXint FXIconList::getVisibleY() const {
  return (options&(ICONLIST_MINI_ICONS|ICONLIST_BIG_ICONS)) ? 0 : header->getHeight();
  }


// Return visible area height
FXint FXIconList::getVisibleHeight() const {
  return (options&(ICONLIST_MINI_ICONS|ICONLIST_BIG_ICONS)) ? height-horizontal->getHeight() : height-header->getHeight()-horizontal->getHeight();
  }


// Move content
void FXIconList::moveContents(FXint x,FXint y){
  FXScrollArea::moveContents(x,y);
  if(!(options&(ICONLIST_MINI_ICONS|ICONLIST_BIG_ICONS))){
    header->setPosition(x);
    }
  }


// Determine number of columns and number of rows
void FXIconList::getrowscols(FXint& nr,FXint& nc,FXint w,FXint h) const {
  if(options&(ICONLIST_BIG_ICONS|ICONLIST_MINI_ICONS)){
    if(options&ICONLIST_COLUMNS){
      nc=w/itemWidth;
      if(nc<1) nc=1;
      nr=(items.no()+nc-1)/nc;
      if(nr*itemHeight > h){
        nc=(w-vertical->getDefaultWidth())/itemWidth;
        if(nc<1) nc=1;
        nr=(items.no()+nc-1)/nc;
        }
      if(nr<1) nr=1;
      }
    else{
      nr=h/itemHeight;
      if(nr<1) nr=1;
      nc=(items.no()+nr-1)/nr;
      if(nc*itemWidth > w){
        nr=(h-horizontal->getDefaultHeight())/itemHeight;
        if(nr<1) nr=1;
        nc=(items.no()+nr-1)/nr;
        }
      if(nc<1) nc=1;
      }
    }
  else{
    nr=items.no();
    nc=1;
    }
  }


// Recompute interior
void FXIconList::recompute(){
  FXint w,h,i;

  itemWidth=1;
  itemHeight=1;

  // Measure the items
  for(i=0; i<items.no(); i++){
    w=items[i]->getWidth(this);
    h=items[i]->getHeight(this);
    if(w>itemWidth) itemWidth=w;
    if(h>itemHeight) itemHeight=h;
    }

  // Normally, apply fixed item size unless autosize is on
  if(options&(ICONLIST_MINI_ICONS|ICONLIST_BIG_ICONS)){
    if(!(options&ICONLIST_AUTOSIZE)) itemWidth=itemSpace;
    }

  // In detail mode, item width depends only on header
  else{
    itemWidth=header->getTotalSize();
    }

  // Get number of rows or columns
  getrowscols(nrows,ncols,width,height);

  // Done
  flags&=~FLAG_RECALC;
  }


// Determine content width of icon list
FXint FXIconList::getContentWidth(){
  if(flags&FLAG_RECALC) recompute();
  return ncols*itemWidth;
  }


// Determine content height of icon list
FXint FXIconList::getContentHeight(){
  if(flags&FLAG_RECALC) recompute();
  return nrows*itemHeight;
  }


// Recalculate layout
void FXIconList::layout(){
  FXint hh=(options&(ICONLIST_MINI_ICONS|ICONLIST_BIG_ICONS)) ? 0 : header->getDefaultHeight();

  // Place scroll bars
  placeScrollBars(width,height-hh);

  // Place header control
  header->position(0,0,width,hh);

  // Set line size
  vertical->setLine(itemHeight);
  horizontal->setLine(itemWidth);

  // We were supposed to make this item viewable
  if(0<=viewable){
    makeItemVisible(viewable);
    }

  // Force repaint
  update();

  // Clean
  flags&=~FLAG_DIRTY;
  }


// Changed size:- this is a bit tricky, because
// we don't want to re-measure the items, but the content
// size has changed because the number of rows/columns has...
void FXIconList::resize(FXint w,FXint h){
  FXint nr=nrows;
  FXint nc=ncols;
  if(w!=width || h!=height){
    getrowscols(nrows,ncols,w,h);
    if(nr!=nrows || nc!=ncols) update();
    }
  FXScrollArea::resize(w,h);
  }


// Changed size and/or pos:- this is a bit tricky, because
// we don't want to re-measure the items, but the content
// size has changed because the number of rows/columns has...
void FXIconList::position(FXint x,FXint y,FXint w,FXint h){
  FXint nr=nrows;
  FXint nc=ncols;
  if(w!=width || h!=height){
    getrowscols(nrows,ncols,w,h);
    if(nr!=nrows || nc!=ncols) update();
    }
  FXScrollArea::position(x,y,w,h);
  }


// Header changed but content size didn't
long FXIconList::onChgHeader(FXObject*,FXSelector,void*){
  return 1;
  }


// Header subdivision resize has been requested;
// we want to set the width of the header column
// to that of the widest item.
long FXIconList::onClkHeader(FXObject*,FXSelector,void* ptr){
  FXint hi=(FXint)(FXival)ptr;
  FXint i,iw,tw,w,nw=0;
  FXString text;

  // For detailed icon list
  if(!(options&(ICONLIST_MINI_ICONS|ICONLIST_BIG_ICONS))){
    for(i=0; i<items.no(); i++){
      w=0;

      // The first header item may have an icon
      if(hi==0){
        if(items[i]->miniIcon){
          iw=items[i]->miniIcon->getWidth();
          w+=iw+DETAIL_TEXT_SPACING+SIDE_SPACING/2;
          }
        }

      // Measure section of text
      text=items[i]->label.section('\t',hi);
      if(!text.empty()){
        tw=font->getTextWidth(text.text(),text.length());
        w+=tw+SIDE_SPACING+2;
        }

      // Keep the max
      if(w>nw) nw=w;
      }

    // Set new header width
    if(nw>0 && nw!=header->getItemSize(hi)){
      header->setItemSize(hi,nw);
      flags&=~FLAG_RECALC;
      }
    }
  return 1;
  }


// Set headers from array of strings
void FXIconList::setHeaders(const FXchar** strings,FXint size){
  header->clearItems();
  header->fillItems(strings,nullptr,size);
  }


// Set headers from newline separated strings
void FXIconList::setHeaders(const FXString& strings,FXint size){
  header->clearItems();
  header->fillItems(strings,nullptr,size);
  }


// Append header caption
void FXIconList::appendHeader(const FXString& text,FXIcon *icon,FXint size){
  header->appendItem(text,icon,size);
  }


// Remove header caption
void FXIconList::removeHeader(FXint index){
  if(index<0 || header->getNumItems()<=index){ fxerror("%s::removeHeader: index out of range.\n",getClassName()); }
  header->removeItem(index);
  }


// Change header caption
void FXIconList::setHeaderText(FXint index,const FXString& text){
  if(index<0 || header->getNumItems()<=index){ fxerror("%s::setHeaderText: index out of range.\n",getClassName()); }
  header->setItemText(index,text);
  }


// Get header caption
FXString FXIconList::getHeaderText(FXint index) const {
  if(index<0 || header->getNumItems()<=index){ fxerror("%s::getHeaderText: index out of range.\n",getClassName()); }
  return header->getItemText(index);
  }


// Change header icon
void FXIconList::setHeaderIcon(FXint index,FXIcon *icon){
  if(index<0 || header->getNumItems()<=index){ fxerror("%s::setHeaderIcon: index out of range.\n",getClassName()); }
  header->setItemIcon(index,icon);
  }


// Get header icon
FXIcon* FXIconList::getHeaderIcon(FXint index) const {
  if(index<0 || header->getNumItems()<=index){ fxerror("%s::getHeaderIcon: index out of range.\n",getClassName()); }
  return header->getItemIcon(index);
  }


// Change header size
void FXIconList::setHeaderSize(FXint index,FXint size){
  if(index<0 || header->getNumItems()<=index){ fxerror("%s::setHeaderSize: index out of range.\n",getClassName()); }
  header->setItemSize(index,size);
  }


// Get header size
FXint FXIconList::getHeaderSize(FXint index) const {
  if(index<0 || header->getNumItems()<=index){ fxerror("%s::getHeaderSize: index out of range.\n",getClassName()); }
  return header->getItemSize(index);
  }


// Return number of headers
FXint FXIconList::getNumHeaders() const {
  return header->getNumItems();
  }


// Change item text
void FXIconList::setItemText(FXint index,const FXString& text){
  if(index<0 || items.no()<=index){ fxerror("%s::setItemText: index out of range.\n",getClassName()); }
  if(items[index]->getText()!=text){
    items[index]->setText(text);
    recalc();
    }
  }


// Get item text
FXString FXIconList::getItemText(FXint index) const {
  if(index<0 || items.no()<=index){ fxerror("%s::getItemText: index out of range.\n",getClassName()); }
  return items[index]->getText();
  }


// Set item icon
void FXIconList::setItemBigIcon(FXint index,FXIcon* icon,FXbool owned){
  if(index<0 || items.no()<=index){ fxerror("%s::setItemBigIcon: index out of range.\n",getClassName()); }
  if(items[index]->getBigIcon()!=icon) recalc();
  items[index]->setBigIcon(icon,owned);
  }


// Get item icon
FXIcon* FXIconList::getItemBigIcon(FXint index) const {
  if(index<0 || items.no()<=index){ fxerror("%s::getItemBigIcon: index out of range.\n",getClassName()); }
  return items[index]->getBigIcon();
  }


// Set item icon
void FXIconList::setItemMiniIcon(FXint index,FXIcon* icon,FXbool owned){
  if(index<0 || items.no()<=index){ fxerror("%s::setItemMiniIcon: index out of range.\n",getClassName()); }
  if(items[index]->getMiniIcon()!=icon) recalc();
  items[index]->setMiniIcon(icon,owned);
  }


// Get item icon
FXIcon* FXIconList::getItemMiniIcon(FXint index) const {
  if(index<0 || items.no()<=index){ fxerror("%s::getItemMiniIcon: index out of range.\n",getClassName()); }
  return items[index]->getMiniIcon();
  }


// Set item data
void FXIconList::setItemData(FXint index,FXptr ptr){
  if(index<0 || items.no()<=index){ fxerror("%s::setItemData: index out of range.\n",getClassName()); }
  items[index]->setData(ptr);
  }


// Get item data
FXptr FXIconList::getItemData(FXint index) const {
  if(index<0 || items.no()<=index){ fxerror("%s::getItemData: index out of range.\n",getClassName()); }
  return items[index]->getData();
  }


// True if item is selected
FXbool FXIconList::isItemSelected(FXint index) const {
  if(index<0 || items.no()<=index){ fxerror("%s::isItemSelected: index out of range.\n",getClassName()); }
  return items[index]->isSelected();
  }


// True if item is current
FXbool FXIconList::isItemCurrent(FXint index) const {
  if(index<0 || items.no()<=index){ fxerror("%s::isItemCurrent: index out of range.\n",getClassName()); }
  return index==current;
  }


// True if item is enabled
FXbool FXIconList::isItemEnabled(FXint index) const {
  if(index<0 || items.no()<=index){ fxerror("%s::isItemEnabled: index out of range.\n",getClassName()); }
  return items[index]->isEnabled();
  }


// True if item (partially) visible
FXbool FXIconList::isItemVisible(FXint index) const {
  FXbool vis=false;
  FXint x,y,hh;
  if(index<0 || items.no()<=index){ fxerror("%s::isItemVisible: index out of range.\n",getClassName()); }
  if(options&(ICONLIST_BIG_ICONS|ICONLIST_MINI_ICONS)){
    if(options&ICONLIST_COLUMNS){
      FXASSERT(ncols>0);
      x=pos_x+itemWidth*(index%ncols);
      y=pos_y+itemHeight*(index/ncols);
      }
    else{
      FXASSERT(nrows>0);
      x=pos_x+itemWidth*(index/nrows);
      y=pos_y+itemHeight*(index%nrows);
      }
    if(0<x+itemWidth && x<getVisibleWidth() && 0<y+itemHeight && y<getVisibleHeight()) vis=true;
    }
  else{
    hh=header->getDefaultHeight();
    y=pos_y+hh+index*itemHeight;
    if(hh<y+itemHeight && y<getVisibleHeight()) vis=true;
    }
  return vis;
  }


// Make item fully visible
void FXIconList::makeItemVisible(FXint index){
  if(0<=index && index<items.no()){

    // Remember for later
    viewable=index;

    // Was realized
    if(xid){
      FXint x,y,hh,px,py,vw,vh;

      // Force layout if dirty
      if(flags&FLAG_RECALC) layout();

      px=pos_x;
      py=pos_y;

      vw=getVisibleWidth();
      vh=getVisibleHeight();

      // Showing icon view
      if(options&(ICONLIST_BIG_ICONS|ICONLIST_MINI_ICONS)){
        if(options&ICONLIST_COLUMNS){
          FXASSERT(ncols>0);
          x=itemWidth*(index%ncols);
          y=itemHeight*(index/ncols);
          }
        else{
          FXASSERT(nrows>0);
          x=itemWidth*(index/nrows);
          y=itemHeight*(index%nrows);
          }
        if(px+x+itemWidth >= vw) px=vw-x-itemWidth;
        if(px+x <= 0) px=-x;
        if(py+y+itemHeight >= vh) py=vh-y-itemHeight;
        if(py+y <= 0) py=-y;
        }

      // Showing list view
      else{
        hh=header->getDefaultHeight();
        y=hh+index*itemHeight;
        if(py+y+itemHeight >= vh+hh) py=hh+vh-y-itemHeight;
        if(py+y <= hh) py=hh-y;
        }

      // Scroll into view
      setPosition(px,py);

      // Done it
      viewable=-1;
      }
    }
  }


// Get item at position x,y
FXint FXIconList::getItemAt(FXint x,FXint y) const {
  FXint ix,iy;
  FXint r,c,index;
  y-=pos_y;
  x-=pos_x;
  if(options&(ICONLIST_BIG_ICONS|ICONLIST_MINI_ICONS)){
    c=x/itemWidth;
    r=y/itemHeight;
    if(c<0 || c>=ncols || r<0 || r>=nrows) return -1;
    index=(options&ICONLIST_COLUMNS) ? ncols*r+c : nrows*c+r;
    if(index<0 || index>=items.no()) return -1;
    ix=itemWidth*c;
    iy=itemHeight*r;
    if(items[index]->hitItem(this,x-ix,y-iy)==0) return -1;
    }
  else{
    y-=header->getDefaultHeight();
    index=y/itemHeight;
    if(index<0 || index>=items.no()) return -1;
    }
  return index;
  }


// Compare strings up to n
static FXint comp(const FXchar* s1,const FXchar* s2,FXint n){
  if(0<n){
    FXint c1,c2;
    do{
      c1=(FXuchar)*s1++;
      c2=(FXuchar)*s2++;
      }
    while((c1==c2) && (' '<=c1) && --n);
    if(c1<' ') c1=0;
    if(c2<' ') c2=0;
    return c1-c2;
    }
  return 0;
  }


// Compare strings case insensitive up to n
static FXint compcase(const FXchar* s1,const FXchar* s2,FXint n){
  if(0<n){
    FXint c1,c2;
    do{
      c1=Ascii::toLower((FXuchar)*s1++);
      c2=Ascii::toLower((FXuchar)*s2++);
      }
    while((c1==c2) && (' '<=c1) && --n);
    if(c1<' ') c1=0;
    if(c2<' ') c2=0;
    return c1-c2;
    }
  return 0;
  }


typedef FXint (*FXCompareFunc)(const FXchar*,const FXchar*,FXint);


// Get item by name
FXint FXIconList::findItem(const FXString& string,FXint start,FXuint flgs) const {
  FXCompareFunc comparefunc=(flgs&SEARCH_IGNORECASE) ? compcase : comp;
  FXint index,len;
  if(0<items.no()){
    len=(flgs&SEARCH_PREFIX)?string.length():2147483647;
    if(flgs&SEARCH_BACKWARD){
      if(start<0) start=items.no()-1;
      for(index=start; 0<=index; index--){
        if((*comparefunc)(items[index]->getText().text(),string.text(),len)==0) return index;
        }
      if(!(flgs&SEARCH_WRAP)) return -1;
      for(index=items.no()-1; start<index; index--){
        if((*comparefunc)(items[index]->getText().text(),string.text(),len)==0) return index;
        }
      }
    else{
      if(start<0) start=0;
      for(index=start; index<items.no(); index++){
        if((*comparefunc)(items[index]->getText().text(),string.text(),len)==0) return index;
        }
      if(!(flgs&SEARCH_WRAP)) return -1;
      for(index=0; index<start; index++){
        if((*comparefunc)(items[index]->getText().text(),string.text(),len)==0) return index;
        }
      }
    }
  return -1;
  }


// Get item by data
FXint FXIconList::findItemByData(FXptr ptr,FXint start,FXuint flgs) const {
  FXint index;
  if(0<items.no()){
    if(flgs&SEARCH_BACKWARD){
      if(start<0) start=items.no()-1;
      for(index=start; 0<=index; index--){
        if(items[index]->getData()==ptr) return index;
        }
      if(!(flgs&SEARCH_WRAP)) return -1;
      for(index=items.no()-1; start<index; index--){
        if(items[index]->getData()==ptr) return index;
        }
      }
    else{
      if(start<0) start=0;
      for(index=start; index<items.no(); index++){
        if(items[index]->getData()==ptr) return index;
        }
      if(!(flgs&SEARCH_WRAP)) return -1;
      for(index=0; index<start; index++){
        if(items[index]->getData()==ptr) return index;
        }
      }
    }
  return -1;
  }


// Did we hit the item, and which part of it did we hit
FXint FXIconList::hitItem(FXint index,FXint x,FXint y,FXint ww,FXint hh) const {
  FXint ix,iy,r,c,hit=0;
  if(0<=index && index<items.no()){
    x-=pos_x;
    y-=pos_y;
    if(!(options&(ICONLIST_BIG_ICONS|ICONLIST_MINI_ICONS))) y-=header->getDefaultHeight();
    if(options&(ICONLIST_BIG_ICONS|ICONLIST_MINI_ICONS)){
      if(options&ICONLIST_COLUMNS){
        r=index/ncols;
        c=index%ncols;
        }
      else{
        c=index/nrows;
        r=index%nrows;
        }
      }
    else{
      r=index;
      c=0;
      }
    ix=itemWidth*c;
    iy=itemHeight*r;
    hit=items[index]->hitItem(this,x-ix,y-iy,ww,hh);
    }
  return hit;
  }


// Repaint
void FXIconList::updateItem(FXint index) const {
  if(xid && 0<=index && index<items.no()){
    if(options&(ICONLIST_BIG_ICONS|ICONLIST_MINI_ICONS)){
      if(options&ICONLIST_COLUMNS){
        FXASSERT(ncols>0);
        update(pos_x+itemWidth*(index%ncols),pos_y+itemHeight*(index/ncols),itemWidth,itemHeight);
        }
      else{
        FXASSERT(nrows>0);
        update(pos_x+itemWidth*(index/nrows),pos_y+itemHeight*(index%nrows),itemWidth,itemHeight);
        }
      }
    else{
      update(0,pos_y+header->getDefaultHeight()+index*itemHeight,width,itemHeight);
      }
    }
  }


// Enable one item
FXbool FXIconList::enableItem(FXint index){
  if(index<0 || items.no()<=index){ fxerror("%s::enableItem: index out of range.\n",getClassName()); }
  if(!items[index]->isEnabled()){
    items[index]->setEnabled(true);
    updateItem(index);
    return true;
    }
  return false;
  }


// Disable one item
FXbool FXIconList::disableItem(FXint index){
  if(index<0 || items.no()<=index){ fxerror("%s::disableItem: index out of range.\n",getClassName()); }
  if(items[index]->isEnabled()){
    items[index]->setEnabled(false);
    updateItem(index);
    return true;
    }
  return false;
  }


// Select one item
FXbool FXIconList::selectItem(FXint index,FXbool notify){
  if(index<0 || items.no()<=index){ fxerror("%s::selectItem: index out of range.\n",getClassName()); }
  if(!items[index]->isSelected()){
    switch(options&SELECT_MASK){
      case ICONLIST_SINGLESELECT:
      case ICONLIST_BROWSESELECT:
        killSelection(notify);
      case ICONLIST_EXTENDEDSELECT:
      case ICONLIST_MULTIPLESELECT:
        items[index]->setSelected(true);
        updateItem(index);
        if(notify && target){target->tryHandle(this,FXSEL(SEL_SELECTED,message),(void*)(FXival)index);}
        break;
      }
    return true;
    }
  return false;
  }


// Deselect one item
FXbool FXIconList::deselectItem(FXint index,FXbool notify){
  if(index<0 || items.no()<=index){ fxerror("%s::deselectItem: index out of range.\n",getClassName()); }
  if(items[index]->isSelected()){
    switch(options&SELECT_MASK){
      case ICONLIST_EXTENDEDSELECT:
      case ICONLIST_MULTIPLESELECT:
      case ICONLIST_SINGLESELECT:
        items[index]->setSelected(false);
        updateItem(index);
        if(notify && target){target->tryHandle(this,FXSEL(SEL_DESELECTED,message),(void*)(FXival)index);}
        break;
      }
    return true;
    }
  return false;
  }


// Toggle one item
FXbool FXIconList::toggleItem(FXint index,FXbool notify){
  if(index<0 || items.no()<=index){ fxerror("%s::toggleItem: index out of range.\n",getClassName()); }
  switch(options&SELECT_MASK){
    case ICONLIST_BROWSESELECT:
      if(!items[index]->isSelected()){
        killSelection(notify);
        items[index]->setSelected(true);
        updateItem(index);
        if(notify && target){target->tryHandle(this,FXSEL(SEL_SELECTED,message),(void*)(FXival)index);}
        }
      break;
    case ICONLIST_SINGLESELECT:
      if(!items[index]->isSelected()){
        killSelection(notify);
        items[index]->setSelected(true);
        updateItem(index);
        if(notify && target){target->tryHandle(this,FXSEL(SEL_SELECTED,message),(void*)(FXival)index);}
        }
      else{
        items[index]->setSelected(false);
        updateItem(index);
        if(notify && target){target->tryHandle(this,FXSEL(SEL_DESELECTED,message),(void*)(FXival)index);}
        }
      break;
    case ICONLIST_EXTENDEDSELECT:
    case ICONLIST_MULTIPLESELECT:
      if(!items[index]->isSelected()){
        items[index]->setSelected(true);
        updateItem(index);
        if(notify && target){target->tryHandle(this,FXSEL(SEL_SELECTED,message),(void*)(FXival)index);}
        }
      else{
        items[index]->setSelected(false);
        updateItem(index);
        if(notify && target){target->tryHandle(this,FXSEL(SEL_DESELECTED,message),(void*)(FXival)index);}
        }
      break;
    }
  return true;
  }


// Select items in rectangle
FXbool FXIconList::selectInRectangle(FXint x,FXint y,FXint w,FXint h,FXbool notify){
  FXint r,c,index;
  FXbool changed=false;
  if(options&(ICONLIST_BIG_ICONS|ICONLIST_MINI_ICONS)){
    for(r=0; r<nrows; r++){
      for(c=0; c<ncols; c++){
        index=(options&ICONLIST_COLUMNS) ? ncols*r+c : nrows*c+r;
        if(index<items.no()){
          if(hitItem(index,x,y,w,h)){
            changed|=selectItem(index,notify);
            }
          }
        }
      }
    }
  else{
    for(index=0; index<items.no(); index++){
      if(hitItem(index,x,y,w,h)){
        changed|=selectItem(index,notify);
        }
      }
    }
  return changed;
  }


// Extend selection
FXbool FXIconList::extendSelection(FXint index,FXbool notify){
  FXbool changes=false;
  if(0<=index && 0<=anchor && 0<=extent){
    FXint i1,i2,i3,i;

    // Find segments
    i1=index;
    if(anchor<i1){i2=i1;i1=anchor;}
    else{i2=anchor;}
    if(extent<i1){i3=i2;i2=i1;i1=extent;}
    else if(extent<i2){i3=i2;i2=extent;}
    else{i3=extent;}

    // First segment
    for(i=i1; i<i2; i++){

      // item===extent---anchor
      // item===anchor---extent
      if(i1==index){
        if(!items[i]->isSelected()){
          items[i]->setSelected(true);
          updateItem(i);
          changes=true;
          if(notify && target){target->tryHandle(this,FXSEL(SEL_SELECTED,message),(void*)(FXival)i);}
          }
        }

      // extent===anchor---item
      // extent===item-----anchor
      else if(i1==extent){
        if(items[i]->isSelected()){
          items[i]->setSelected(false);
          updateItem(i);
          changes=true;
          if(notify && target){target->tryHandle(this,FXSEL(SEL_DESELECTED,message),(void*)(FXival)i);}
          }
        }
      }

    // Second segment
    for(i=i2+1; i<=i3; i++){

      // extent---anchor===item
      // anchor---extent===item
      if(i3==index){
        if(!items[i]->isSelected()){
          items[i]->setSelected(true);
          updateItem(i);
          changes=true;
          if(notify && target){target->tryHandle(this,FXSEL(SEL_SELECTED,message),(void*)(FXival)i);}
          }
        }

      // item-----anchor===extent
      // anchor---item=====extent
      else if(i3==extent){
        if(items[i]->isSelected()){
          items[i]->setSelected(false);
          updateItem(i);
          changes=true;
          if(notify && target){target->tryHandle(this,FXSEL(SEL_DESELECTED,message),(void*)(FXival)i);}
          }
        }
      }
    extent=index;
    }
  return changes;
  }


// Select all items
FXbool FXIconList::selectAll(FXbool notify){
  FXbool changes=false;
  for(FXint i=0; i<items.no(); i++){
    if(!items[i]->isSelected()){
      items[i]->setSelected(true);
      updateItem(i);
      changes=true;
      if(notify && target){target->tryHandle(this,FXSEL(SEL_SELECTED,message),(void*)(FXival)i);}
      }
    }
  return changes;
  }


// Kill selection
FXbool FXIconList::killSelection(FXbool notify){
  FXbool changes=false;
  for(FXint i=0; i<items.no(); i++){
    if(items[i]->isSelected()){
      items[i]->setSelected(false);
      updateItem(i);
      changes=true;
      if(notify && target){target->tryHandle(this,FXSEL(SEL_DESELECTED,message),(void*)(FXival)i);}
      }
    }
  return changes;
  }


// Lasso changed, so select/unselect items based on difference between new and old lasso box
void FXIconList::lassoChanged(FXint ox,FXint oy,FXint ow,FXint oh,FXint nx,FXint ny,FXint nw,FXint nh,FXbool notify){
  FXint x0,y0,x1,y1,rlo,rhi,clo,chi,r,c;
  FXint ohit,nhit,index;
  if(options&(ICONLIST_BIG_ICONS|ICONLIST_MINI_ICONS)){

    // Union rectangle
    x0=FXMIN(ox,nx); x1=FXMAX(ox+ow,nx+nw);
    y0=FXMIN(oy,ny); y1=FXMAX(oy+oh,ny+nh);

    // Affected row range
    rlo=(y0-pos_y)/itemHeight; if(rlo<0) rlo=0;
    rhi=(y1-pos_y)/itemHeight; if(rhi>=nrows) rhi=nrows-1;

    // Affected column range
    clo=(x0-pos_x)/itemWidth; if(clo<0) clo=0;
    chi=(x1-pos_x)/itemWidth; if(chi>=ncols) chi=ncols-1;

    // Change selection
    for(r=rlo; r<=rhi; r++){
      for(c=clo; c<=chi; c++){
        index=(options&ICONLIST_COLUMNS) ? ncols*r+c : nrows*c+r;
        if(index<items.no()){
          ohit=hitItem(index,ox,oy,ow,oh);
          nhit=hitItem(index,nx,ny,nw,nh);
          if(ohit && !nhit){      // In old rectangle and not in new rectangle
            deselectItem(index,notify);
            }
          else if(!ohit && nhit){ // Not in old rectangle and in new rectangle
            selectItem(index,notify);
            }
          }
        }
      }
    }
  else{

    // Affected lines
    y0=FXMIN(oy,ny); y1=FXMAX(oy+oh,ny+nh);

    // Exposed rows
    rlo=(y0-pos_y-header->getDefaultHeight())/itemHeight; if(rlo<0) rlo=0;
    rhi=(y1-pos_y-header->getDefaultHeight())/itemHeight; if(rhi>=items.no()) rhi=items.no()-1;

    // Change selection
    for(index=rlo; index<=rhi; index++){
      ohit=hitItem(index,ox,oy,ow,oh);
      nhit=hitItem(index,nx,ny,nw,nh);
      if(ohit && !nhit){          // Was in old, not in new
        deselectItem(index,notify);
        }
      else if(!ohit && nhit){     // Not in old, but in new
        selectItem(index,notify);
        }
      }
    }
  }


// Update value from a message
long FXIconList::onCmdSetValue(FXObject*,FXSelector,void* ptr){
  setCurrentItem((FXint)(FXival)ptr);
  return 1;
  }


// Obtain value from list
long FXIconList::onCmdGetIntValue(FXObject*,FXSelector,void* ptr){
  *((FXint*)ptr)=getCurrentItem();
  return 1;
  }


// Update value from a message
long FXIconList::onCmdSetIntValue(FXObject*,FXSelector,void* ptr){
  setCurrentItem(*((FXint*)ptr));
  return 1;
  }


// Start motion timer while in this window
long FXIconList::onEnter(FXObject* sender,FXSelector sel,void* ptr){
  FXScrollArea::onEnter(sender,sel,ptr);
  getApp()->addTimeout(this,ID_TIPTIMER,getApp()->getMenuPause());
  return 1;
  }


// Stop motion timer when leaving window
long FXIconList::onLeave(FXObject* sender,FXSelector sel,void* ptr){
  FXScrollArea::onLeave(sender,sel,ptr);
  getApp()->removeTimeout(this,ID_TIPTIMER);
  return 1;
  }


// We timed out, i.e. the user didn't move for a while
long FXIconList::onTipTimer(FXObject*,FXSelector,void*){
  FXTRACE((250,"%s::onTipTimer %p\n",getClassName(),this));
  flags|=FLAG_TIP;
  return 1;
  }


// We were asked about tip text
long FXIconList::onQueryTip(FXObject* sender,FXSelector sel,void* ptr){
  FXint index,cx,cy; FXuint btns;
  if(FXScrollArea::onQueryTip(sender,sel,ptr)) return 1;
  if(flags&FLAG_TIP){
    if(getCursorPosition(cx,cy,btns) && (index=getItemAt(cx,cy))>=0){
      FXString string=getItem(index)->getTipText();
      sender->handle(this,FXSEL(SEL_COMMAND,ID_SETSTRINGVALUE),(void*)&string);
      return 1;
      }
    }
  return 0;
  }

// We were asked about status text
long FXIconList::onQueryHelp(FXObject* sender,FXSelector sel,void* ptr){
  if(FXScrollArea::onQueryHelp(sender,sel,ptr)) return 1;
  if((flags&FLAG_HELP) && !help.empty()){
    sender->handle(this,FXSEL(SEL_COMMAND,ID_SETSTRINGVALUE),(void*)&help);
    return 1;
    }
  return 0;
  }


// Gained focus
long FXIconList::onFocusIn(FXObject* sender,FXSelector sel,void* ptr){
  FXScrollArea::onFocusIn(sender,sel,ptr);
  if(0<=current){
    FXASSERT(current<items.no());
    items[current]->setFocus(true);
    updateItem(current);
    }
  return 1;
  }


// Lost focus
long FXIconList::onFocusOut(FXObject* sender,FXSelector sel,void* ptr){
  FXScrollArea::onFocusOut(sender,sel,ptr);
  if(0<=current){
    FXASSERT(current<items.no());
    items[current]->setFocus(false);
    updateItem(current);
    }
  return 1;
  }


// Draw item list
long FXIconList::onPaint(FXObject*,FXSelector,void* ptr){
  FXint rlo,rhi,clo,chi,x,y,w,h,r,c,index;
  FXEvent* event=(FXEvent*)ptr;
  FXDCWindow dc(this,event);

  // Set font
  dc.setFont(font);

  // Icon mode
  if(options&(ICONLIST_BIG_ICONS|ICONLIST_MINI_ICONS)){

    // Exposed rows
    rlo=(event->rect.y-pos_y)/itemHeight;
    rhi=(event->rect.y+event->rect.h-pos_y)/itemHeight;
    if(rlo<0) rlo=0;
    if(rhi>=nrows) rhi=nrows-1;

    // Exposed columns
    clo=(event->rect.x-pos_x)/itemWidth;
    chi=(event->rect.x+event->rect.w-pos_x)/itemWidth;
    if(clo<0) clo=0;
    if(chi>=ncols) chi=ncols-1;

    for(r=rlo; r<=rhi; r++){
      y=pos_y+r*itemHeight;
      for(c=clo; c<=chi; c++){
        x=pos_x+c*itemWidth;
        index=(options&ICONLIST_COLUMNS) ? ncols*r+c : nrows*c+r;
        dc.setForeground(backColor);
        if(index<items.no()){
          items[index]->draw(this,dc,x,y,itemWidth,itemHeight);
          }
        else{
          dc.fillRectangle(x,y,itemWidth,itemHeight);
          }
        }
      }

    // Background below
    y=pos_y+(rhi+1)*itemHeight;
    if(y<event->rect.y+event->rect.h){
      dc.setForeground(backColor);
      dc.fillRectangle(event->rect.x,y,event->rect.w,event->rect.y+event->rect.h-y);
      }

    // Background to the right
    x=pos_x+(chi+1)*itemWidth;
    if(x<event->rect.x+event->rect.w){
      dc.setForeground(backColor);
      dc.fillRectangle(x,event->rect.y,event->rect.x+event->rect.w-x,event->rect.h);
      }
    }

  // Detail mode
  else{

    // Exposed rows
    rlo=(event->rect.y-pos_y-header->getDefaultHeight())/itemHeight;
    rhi=(event->rect.y+event->rect.h-pos_y-header->getDefaultHeight())/itemHeight;
    if(rlo<0) rlo=0;
    if(rhi>=items.no()) rhi=items.no()-1;

    // Repaint the items
    y=pos_y+rlo*itemHeight+header->getDefaultHeight();
    for(index=rlo; index<=rhi; index++,y+=itemHeight){
      dc.setForeground(backColor);
      items[index]->draw(this,dc,pos_x,y,header->getTotalSize(),itemHeight);
      }

    // Background below
    y=pos_y+(rhi+1)*itemHeight+header->getDefaultHeight();
    if(y<event->rect.y+event->rect.h){
      dc.setForeground(backColor);
      dc.fillRectangle(event->rect.x,y,event->rect.w,event->rect.y+event->rect.h-y);
      }

    // Background to the right
    x=pos_x+header->getTotalSize();
    if(x<event->rect.x+event->rect.w){
      dc.setForeground(backColor);
      dc.fillRectangle(x,event->rect.y,event->rect.x+event->rect.w-x,event->rect.h);
      }
    }

  // Gray selection rectangle; look ma, no blending!
  if(flags&FLAG_LASSO){
    if(anchorx!=currentx && anchory!=currenty){
      FXMINMAX(x,w,anchorx,currentx); w-=x;
      FXMINMAX(y,h,anchory,currenty); h-=y;
      dc.setFunction(BLT_SRC_AND_DST);
      dc.setForeground(FXRGB(0xD5,0xD5,0xD5));
      dc.fillRectangle(x+pos_x,y+pos_y,w,h);
      dc.setForeground(FXRGB(0x55,0x55,0x55));
      dc.drawRectangle(x+pos_x,y+pos_y,w-1,h-1);
      }
    }

  return 1;
  }


// Start lasso operation
void FXIconList::startLasso(FXint ax,FXint ay){
  anchorx=currentx=ax;
  anchory=currenty=ay;
  flags|=FLAG_LASSO;
  }


// Update lasso area
void FXIconList::updateLasso(FXint cx,FXint cy){
  FXint slx,shx,sly,shy,lx,hx,ly,hy;
  FXMINMAX(slx,shx,currentx,cx);
  FXMINMAX(sly,shy,currenty,cy);
  lx=FXMIN(anchorx,slx);
  ly=FXMIN(anchory,sly);
  hx=FXMAX(anchorx,shx);
  hy=FXMAX(anchory,shy);
  currentx=cx;
  currenty=cy;
  update(pos_x+lx,pos_y+sly-1,hx-lx,shy-sly+2);
#ifdef WIN32
  repaint(pos_x+lx,pos_y+sly-1,hx-lx,shy-sly+2);
#endif
  update(pos_x+slx-1,pos_y+ly,shx-slx+2,hy-ly);
#ifdef WIN32
  repaint(pos_x+slx-1,pos_y+ly,shx-slx+2,hy-ly);
#endif
  }


// End lasso operation
void FXIconList::endLasso(){
  FXint lx,ly,hx,hy;
  FXMINMAX(lx,hx,anchorx,currentx);
  FXMINMAX(ly,hy,anchory,currenty);
  update(pos_x+lx,pos_y+ly,hx-lx,hy-ly);
  flags&=~FLAG_LASSO;
  }


// Arrange by rows
long FXIconList::onCmdArrangeByRows(FXObject*,FXSelector,void*){
  options&=~ICONLIST_COLUMNS;
  recalc();
  return 1;
  }


// Update sender
long FXIconList::onUpdArrangeByRows(FXObject* sender,FXSelector,void*){
  sender->handle(this,(options&ICONLIST_COLUMNS)?FXSEL(SEL_COMMAND,ID_UNCHECK):FXSEL(SEL_COMMAND,ID_CHECK),nullptr);
  sender->handle(this,(options&(ICONLIST_MINI_ICONS|ICONLIST_BIG_ICONS))?FXSEL(SEL_COMMAND,ID_ENABLE):FXSEL(SEL_COMMAND,ID_DISABLE),nullptr);
  return 1;
  }


// Arrange by columns
long FXIconList::onCmdArrangeByColumns(FXObject*,FXSelector,void*){
  options|=ICONLIST_COLUMNS;
  recalc();
  return 1;
  }


// Update sender
long FXIconList::onUpdArrangeByColumns(FXObject* sender,FXSelector,void*){
  sender->handle(this,(options&ICONLIST_COLUMNS)?FXSEL(SEL_COMMAND,ID_CHECK):FXSEL(SEL_COMMAND,ID_UNCHECK),nullptr);
  sender->handle(this,(options&(ICONLIST_MINI_ICONS|ICONLIST_BIG_ICONS))?FXSEL(SEL_COMMAND,ID_ENABLE):FXSEL(SEL_COMMAND,ID_DISABLE),nullptr);
  return 1;
  }


// Show detailed list
long FXIconList::onCmdShowDetails(FXObject*,FXSelector,void*){
  options&=~ICONLIST_MINI_ICONS;
  options&=~ICONLIST_BIG_ICONS;
  recalc();
  return 1;
  }


// Update sender
long FXIconList::onUpdShowDetails(FXObject* sender,FXSelector,void*){
  sender->handle(this,(options&(ICONLIST_MINI_ICONS|ICONLIST_BIG_ICONS))?FXSEL(SEL_COMMAND,ID_UNCHECK):FXSEL(SEL_COMMAND,ID_CHECK),nullptr);
  return 1;
  }


// Show big icons
long FXIconList::onCmdShowBigIcons(FXObject*,FXSelector,void*){
  options&=~ICONLIST_MINI_ICONS;
  options|=ICONLIST_BIG_ICONS;
  recalc();
  return 1;
  }


// Update sender
long FXIconList::onUpdShowBigIcons(FXObject* sender,FXSelector,void*){
  sender->handle(this,(options&ICONLIST_BIG_ICONS)?FXSEL(SEL_COMMAND,ID_CHECK):FXSEL(SEL_COMMAND,ID_UNCHECK),nullptr);
  return 1;
  }


// Show small icons
long FXIconList::onCmdShowMiniIcons(FXObject*,FXSelector,void*){
  options|=ICONLIST_MINI_ICONS;
  options&=~ICONLIST_BIG_ICONS;
  recalc();
  return 1;
  }


// Update sender
long FXIconList::onUpdShowMiniIcons(FXObject* sender,FXSelector,void*){
  sender->handle(this,(options&ICONLIST_MINI_ICONS)?FXSEL(SEL_COMMAND,ID_CHECK):FXSEL(SEL_COMMAND,ID_UNCHECK),nullptr);
  return 1;
  }


// Select all items
long FXIconList::onCmdSelectAll(FXObject*,FXSelector,void*){
  for(int i=0; i<items.no(); i++) selectItem(i,true);
  return 1;
  }


// Deselect all items
long FXIconList::onCmdDeselectAll(FXObject*,FXSelector,void*){
  for(int i=0; i<items.no(); i++) deselectItem(i,true);
  return 1;
  }


// Select inverse of current selection
long FXIconList::onCmdSelectInverse(FXObject*,FXSelector,void*){
  for(int i=0; i<items.no(); i++) toggleItem(i,true);
  return 1;
  }


// Compare sectioned strings
FXint FXIconList::compareSection(const FXchar *p,const FXchar* q,FXint s){
  FXint c1,c2,x;
  for(x=s; x && *p; x-=(*p++=='\t')){}
  for(x=s; x && *q; x-=(*q++=='\t')){}
  do{
    c1=*p++;
    c2=*q++;
    }
  while((c1==c2) && (' '<=c1));
  if(c1<' ') c1=0;
  if(c2<' ') c2=0;
  return c1-c2;
  }


// Compare sectioned strings, case-insensitive
FXint FXIconList::compareSectionCase(const FXchar *p,const FXchar* q,FXint s){
  FXint c1,c2,x;
  for(x=s; x && *p; x-=(*p++=='\t')){}
  for(x=s; x && *q; x-=(*q++=='\t')){}
  do{
    c1=Ascii::toLower(*p++);
    c2=Ascii::toLower(*q++);
    }
  while((c1==c2) && (' '<=c1));
  if(c1<' ') c1=0;
  if(c2<' ') c2=0;
  return c1-c2;
  }


// Sort items in ascending order
FXint FXIconList::ascending(const FXIconItem* a,const FXIconItem* b){
  return compareSection(a->getText().text(),b->getText().text(),0);
  }


// Sort items in descending order
FXint FXIconList::descending(const FXIconItem* a,const FXIconItem* b){
  return compareSection(b->getText().text(),a->getText().text(),0);
  }


// Sort items in ascending order, case insensitive
FXint FXIconList::ascendingCase(const FXIconItem* a,const FXIconItem* b){
  return compareSectionCase(a->getText().text(),b->getText().text(),0);
  }


// Sort items in descending order, case insensitive
FXint FXIconList::descendingCase(const FXIconItem* a,const FXIconItem* b){
  return compareSectionCase(b->getText().text(),a->getText().text(),0);
  }


// Sort the items based on the sort function
void FXIconList::sortItems(){
  FXIconItem *v,*c=0;
  FXbool exch=false;
  FXint i,j,h;
  if(sortfunc){
    if(0<=current){
      c=items[current];
      }
    for(h=1; h<=items.no()/9; h=3*h+1){}
    for(; h>0; h/=3){
      for(i=h+1;i<=items.no();i++){
        v=items[i-1];
        j=i;
        while(j>h && sortfunc(items[j-h-1],v)>0){
          items[j-1]=items[j-h-1];
          exch=true;
          j-=h;
          }
        items[j-1]=v;
        }
      }
    if(0<=current){
      for(i=0; i<items.no(); i++){
        if(items[i]==c){ current=i; break; }
        }
      }
    if(exch) recalc();
    }
  }


// Set current item
void FXIconList::setCurrentItem(FXint index,FXbool notify){
  if(index<-1 || items.no()<=index){ fxerror("%s::setCurrentItem: index out of range.\n",getClassName()); }
  if(index!=current){

    // Deactivate old item
    if(0<=current){

      // No visible change if it doen't have the focus
      if(hasFocus()){
        items[current]->setFocus(false);
        updateItem(current);
        }
      }

    current=index;

    // Activate new item
    if(0<=current){

      // No visible change if it doen't have the focus
      if(hasFocus()){
        items[current]->setFocus(true);
        updateItem(current);
        }
      }

    // Notify item change
    if(notify && target){
      target->tryHandle(this,FXSEL(SEL_CHANGED,message),(void*)(FXival)current);
      }
    }

  // In browse selection mode, select item
  if((options&SELECT_MASK)==ICONLIST_BROWSESELECT && 0<=current && items[current]->isEnabled()){
    selectItem(current,notify);
    }
  }


// Set anchor item
void FXIconList::setAnchorItem(FXint index){
  if(index<-1 || items.no()<=index){ fxerror("%s::setAnchorItem: index out of range.\n",getClassName()); }
  anchor=index;
  extent=index;
  }


// Zero out lookup string
long FXIconList::onLookupTimer(FXObject*,FXSelector,void*){
  lookup=FXString::null;
  return 1;
  }


// Key Press
long FXIconList::onKeyPress(FXObject*,FXSelector,void* ptr){
  FXEvent* event=(FXEvent*)ptr;
  FXint index=current;
  flags&=~FLAG_TIP;
  if(!isEnabled()) return 0;
  if(target && target->tryHandle(this,FXSEL(SEL_KEYPRESS,message),ptr)) return 1;
  switch(event->code){
    case KEY_Control_L:
    case KEY_Control_R:
    case KEY_Shift_L:
    case KEY_Shift_R:
    case KEY_Alt_L:
    case KEY_Alt_R:
      if(flags&FLAG_DODRAG){handle(this,FXSEL(SEL_DRAGGED,0),ptr);}
      return 1;
    case KEY_Page_Up:
    case KEY_KP_Page_Up:
      lookup=FXString::null;
      setPosition(pos_x,pos_y+verticalScrollBar()->getPage());
      return 1;
    case KEY_Page_Down:
    case KEY_KP_Page_Down:
      lookup=FXString::null;
      setPosition(pos_x,pos_y-verticalScrollBar()->getPage());
      return 1;
    case KEY_Right:
    case KEY_KP_Right:
      if(!(options&(ICONLIST_BIG_ICONS|ICONLIST_MINI_ICONS))){
        setPosition(pos_x-10,pos_y);
        return 1;
        }
      if(options&ICONLIST_COLUMNS) index+=1; else index+=nrows;
      goto hop;
    case KEY_Left:
    case KEY_KP_Left:
      if(!(options&(ICONLIST_BIG_ICONS|ICONLIST_MINI_ICONS))){
        setPosition(pos_x+10,pos_y);
        return 1;
        }
      if(options&ICONLIST_COLUMNS) index-=1; else index-=nrows;
      goto hop;
    case KEY_Up:
    case KEY_KP_Up:
      if(options&ICONLIST_COLUMNS) index-=ncols; else index-=1;
      goto hop;
    case KEY_Down:
    case KEY_KP_Down:
      if(options&ICONLIST_COLUMNS) index+=ncols; else index+=1;
      goto hop;
    case KEY_Home:
    case KEY_KP_Home:
      index=0;
      goto hop;
    case KEY_End:
    case KEY_KP_End:
      index=items.no()-1;
hop:  lookup=FXString::null;
      if(0<=index && index<items.no()){
        setCurrentItem(index,true);
        makeItemVisible(index);
        if(items[index]->isEnabled()){
          if((options&SELECT_MASK)==ICONLIST_EXTENDEDSELECT){
            if(event->state&SHIFTMASK){
              if(0<=anchor){
                selectItem(anchor,true);
                extendSelection(index,true);
                }
              else{
                selectItem(index,true);
                }
              }
            else if(!(event->state&CONTROLMASK)){
              killSelection(true);
              selectItem(index,true);
              setAnchorItem(index);
              }
            }
          }
        }
      handle(this,FXSEL(SEL_CLICKED,0),(void*)(FXival)current);
      if(0<=current && items[current]->isEnabled()){
        handle(this,FXSEL(SEL_COMMAND,0),(void*)(FXival)current);
        }
      return 1;
    case KEY_space:
    case KEY_KP_Space:
      lookup=FXString::null;
      if(0<=current && items[current]->isEnabled()){
        switch(options&SELECT_MASK){
          case ICONLIST_EXTENDEDSELECT:
            if(event->state&SHIFTMASK){
              if(0<=anchor){
                selectItem(anchor,true);
                extendSelection(current,true);
                }
              else{
                selectItem(current,true);
                }
              }
            else if(event->state&CONTROLMASK){
              toggleItem(current,true);
              }
            else{
              killSelection(true);
              selectItem(current,true);
              }
            break;
          case ICONLIST_MULTIPLESELECT:
          case ICONLIST_SINGLESELECT:
            toggleItem(current,true);
            break;
          }
        setAnchorItem(current);
        }
      handle(this,FXSEL(SEL_CLICKED,0),(void*)(FXival)current);
      if(0<=current && items[current]->isEnabled()){
        handle(this,FXSEL(SEL_COMMAND,0),(void*)(FXival)current);
        }
      return 1;
    case KEY_Return:
    case KEY_KP_Enter:
      lookup=FXString::null;
      handle(this,FXSEL(SEL_DOUBLECLICKED,0),(void*)(FXival)current);
      if(0<=current && items[current]->isEnabled()){
        handle(this,FXSEL(SEL_COMMAND,0),(void*)(FXival)current);
        }
      return 1;
    default:
      if((FXuchar)event->text[0]<' ') return 0;
      if(event->state&(CONTROLMASK|ALTMASK)) return 0;
      if(!Ascii::isPrint(event->text[0])) return 0;
      lookup.append(event->text);
      getApp()->addTimeout(this,ID_LOOKUPTIMER,getApp()->getTypingSpeed());
      index=findItem(lookup,current,SEARCH_FORWARD|SEARCH_WRAP|SEARCH_PREFIX);
      if(0<=index){
	setCurrentItem(index,true);
	makeItemVisible(index);
	if(items[index]->isEnabled()){
	  if((options&SELECT_MASK)==ICONLIST_EXTENDEDSELECT){
	    killSelection(true);
	    selectItem(index,true);
	    }
	  setAnchorItem(index);
	  }
        }
      handle(this,FXSEL(SEL_CLICKED,0),(void*)(FXival)current);
      if(0<=current && items[current]->isEnabled()){
        handle(this,FXSEL(SEL_COMMAND,0),(void*)(FXival)current);
        }
      return 1;
    }
  return 0;
  }


// Key Release
long FXIconList::onKeyRelease(FXObject*,FXSelector,void* ptr){
  FXEvent* event=(FXEvent*)ptr;
  if(!isEnabled()) return 0;
  if(target && target->tryHandle(this,FXSEL(SEL_KEYRELEASE,message),ptr)) return 1;
  switch(event->code){
    case KEY_Shift_L:
    case KEY_Shift_R:
    case KEY_Control_L:
    case KEY_Control_R:
    case KEY_Alt_L:
    case KEY_Alt_R:
      if(flags&FLAG_DODRAG){handle(this,FXSEL(SEL_DRAGGED,0),ptr);}
      return 1;
    }
  return 0;
  }


// Autoscrolling timer
long FXIconList::onAutoScroll(FXObject* sender,FXSelector sel,void* ptr){
  FXEvent* event=(FXEvent*)ptr;
  FXint olx,orx,oty,oby,nlx,nrx,nty,nby;

  // Scroll the content
  FXScrollArea::onAutoScroll(sender,sel,ptr);

  // Lasso mode
  if(flags&FLAG_LASSO){

    // Select items in lasso
    FXMINMAX(olx,orx,anchorx,currentx);
    FXMINMAX(oty,oby,anchory,currenty);
    updateLasso(event->win_x-pos_x,event->win_y-pos_y);
    FXMINMAX(nlx,nrx,anchorx,currentx);
    FXMINMAX(nty,nby,anchory,currenty);
    lassoChanged(pos_x+olx,pos_y+oty,orx-olx+1,oby-oty+1,pos_x+nlx,pos_y+nty,nrx-nlx+1,nby-nty+1,true);
    return 1;
    }

  // Content scrolled, so perhaps something else under cursor
  if(flags&FLAG_DODRAG){
    handle(this,FXSEL(SEL_DRAGGED,0),ptr);
    return 1;
    }

  return 0;
  }


// Mouse moved
long FXIconList::onMotion(FXObject*,FXSelector,void* ptr){
  FXint olx,orx,oty,oby,nlx,nrx,nty,nby;
  FXEvent* event=(FXEvent*)ptr;
  FXuint flg=flags;

  // Kill the tip
  flags&=~FLAG_TIP;

  // Kill the tip timer
  getApp()->removeTimeout(this,ID_TIPTIMER);

  // Right mouse scrolling
  if(flags&FLAG_SCROLLING){
    setPosition(event->win_x-grabx,event->win_y-graby);
    return 1;
    }

  // Lasso selection mode
  if(flags&FLAG_LASSO){
    if(startAutoScroll(event,false)) return 1;

    // Select items in lasso
    FXMINMAX(olx,orx,anchorx,currentx);
    FXMINMAX(oty,oby,anchory,currenty);
    updateLasso(event->win_x-pos_x,event->win_y-pos_y);
    FXMINMAX(nlx,nrx,anchorx,currentx);
    FXMINMAX(nty,nby,anchory,currenty);
    lassoChanged(pos_x+olx,pos_y+oty,orx-olx+1,oby-oty+1,pos_x+nlx,pos_y+nty,nrx-nlx+1,nby-nty+1,true);
    return 1;
    }

  // Drag and drop mode
  if(flags&FLAG_DODRAG){
    if(startAutoScroll(event,true)) return 1;
    handle(this,FXSEL(SEL_DRAGGED,0),ptr);
    return 1;
    }

  // Tentative drag and drop
  if(flags&FLAG_TRYDRAG){
    if(event->moved){
      flags&=~FLAG_TRYDRAG;
      if(handle(this,FXSEL(SEL_BEGINDRAG,0),ptr)){
        flags|=FLAG_DODRAG;
        }
      }
    return 1;
    }

  // Reset tip timer if nothing's going on
  getApp()->addTimeout(this,ID_TIPTIMER,getApp()->getMenuPause());

  // Force GUI update only when needed
  return (flg&FLAG_TIP);
  }


// Pressed a button
long FXIconList::onLeftBtnPress(FXObject*,FXSelector,void* ptr){
  FXEvent* event=(FXEvent*)ptr;
  FXint index;
  flags&=~FLAG_TIP;
  handle(this,FXSEL(SEL_FOCUS_SELF,0),ptr);
  if(isEnabled()){
    grab();
    flags&=~FLAG_UPDATE;

    // First change callback
    if(target && target->tryHandle(this,FXSEL(SEL_LEFTBUTTONPRESS,message),ptr)) return 1;

    // Locate item
    index=getItemAt(event->win_x,event->win_y);

    // No item
    if(index<0){

      // Start lasso
      if((options&SELECT_MASK)==ICONLIST_EXTENDEDSELECT){

        // Kill selection
        if(!(event->state&(SHIFTMASK|CONTROLMASK))){
          killSelection(true);
          }

        // Start lasso
        startLasso(event->win_x-pos_x,event->win_y-pos_y);
        }
      return 1;
      }

    // Previous selection state
    state=items[index]->isSelected();

    // Change current item
    setCurrentItem(index,true);

    // Change item selection
    switch(options&SELECT_MASK){
      case ICONLIST_EXTENDEDSELECT:
        if(event->state&SHIFTMASK){
          if(0<=anchor){
            if(items[anchor]->isEnabled()) selectItem(anchor,true);
            extendSelection(index,true);
            }
          else{
            if(items[index]->isEnabled()) selectItem(index,true);
            setAnchorItem(index);
            }
          }
        else if(event->state&CONTROLMASK){
          if(items[index]->isEnabled() && !state) selectItem(index,true);
          setAnchorItem(index);
          }
        else{
          if(items[index]->isEnabled() && !state){ killSelection(true); selectItem(index,true); }
          setAnchorItem(index);
          }
        break;
      case ICONLIST_MULTIPLESELECT:
      case ICONLIST_SINGLESELECT:
        if(items[index]->isEnabled() && !state) selectItem(index,true);
        break;
      }

    // Are we dragging?
    if(state && items[index]->isSelected() && items[index]->isDraggable()){
      flags|=FLAG_TRYDRAG;
      }

    flags|=FLAG_PRESSED;
    return 1;
    }
  return 0;
  }


// Released button
long FXIconList::onLeftBtnRelease(FXObject*,FXSelector,void* ptr){
  FXEvent* event=(FXEvent*)ptr;
  FXuint flg=flags;
  if(isEnabled()){
    ungrab();
    stopAutoScroll();
    flags|=FLAG_UPDATE;
    flags&=~(FLAG_PRESSED|FLAG_TRYDRAG|FLAG_LASSO|FLAG_DODRAG);

    // First chance callback
    if(target && target->tryHandle(this,FXSEL(SEL_LEFTBUTTONRELEASE,message),ptr)) return 1;

    // Was lassoing
    if(flg&FLAG_LASSO){
      endLasso();
      return 1;
      }

    // Was dragging
    if(flg&FLAG_DODRAG){
      handle(this,FXSEL(SEL_ENDDRAG,0),ptr);
      return 1;
      }

    // Must have pressed
    if(flg&FLAG_PRESSED){

      // Selection change
      switch(options&SELECT_MASK){
        case ICONLIST_EXTENDEDSELECT:
          if(0<=current && items[current]->isEnabled()){
            if(event->state&CONTROLMASK){
              if(state) deselectItem(current,true);
              }
            else if(!(event->state&SHIFTMASK)){
              if(state){ killSelection(true); selectItem(current,true); }
              }
            }
          break;
        case ICONLIST_MULTIPLESELECT:
        case ICONLIST_SINGLESELECT:
          if(0<=current && items[current]->isEnabled()){
            if(state) deselectItem(current,true);
            }
          break;
        }

      // Update anchor
      setAnchorItem(current);

      // Generate clicked callbacks
      if(event->click_count==1){
        handle(this,FXSEL(SEL_CLICKED,0),(void*)(FXival)current);
        }
      else if(event->click_count==2){
        handle(this,FXSEL(SEL_DOUBLECLICKED,0),(void*)(FXival)current);
        }
      else if(event->click_count==3){
        handle(this,FXSEL(SEL_TRIPLECLICKED,0),(void*)(FXival)current);
        }

      // Command callback only when clicked on item
      if(0<=current && items[current]->isEnabled()){
        handle(this,FXSEL(SEL_COMMAND,0),(void*)(FXival)current);
        }
      }
    return 1;
    }
  return 0;
  }


// Pressed right button
long FXIconList::onRightBtnPress(FXObject*,FXSelector,void* ptr){
  FXEvent* event=(FXEvent*)ptr;
  flags&=~FLAG_TIP;
  handle(this,FXSEL(SEL_FOCUS_SELF,0),ptr);
  if(isEnabled()){
    grab();
    flags&=~FLAG_UPDATE;
    if(target && target->tryHandle(this,FXSEL(SEL_RIGHTBUTTONPRESS,message),ptr)) return 1;
    flags|=FLAG_SCROLLING;
    grabx=event->win_x-pos_x;
    graby=event->win_y-pos_y;
    return 1;
    }
  return 0;
  }


// Released right button
long FXIconList::onRightBtnRelease(FXObject*,FXSelector,void* ptr){
  if(isEnabled()){
    ungrab();
    flags&=~FLAG_SCROLLING;
    flags|=FLAG_UPDATE;
    if(target && target->tryHandle(this,FXSEL(SEL_RIGHTBUTTONRELEASE,message),ptr)) return 1;
    return 1;
    }
  return 0;
  }


// The widget lost the grab for some reason
long FXIconList::onUngrabbed(FXObject* sender,FXSelector sel,void* ptr){
  FXScrollArea::onUngrabbed(sender,sel,ptr);
  flags&=~(FLAG_DODRAG|FLAG_LASSO|FLAG_TRYDRAG|FLAG_PRESSED|FLAG_CHANGED|FLAG_SCROLLING);
  flags|=FLAG_UPDATE;
  stopAutoScroll();
  return 1;
  }


// Command message
long FXIconList::onCommand(FXObject*,FXSelector,void* ptr){
  return target && target->tryHandle(this,FXSEL(SEL_COMMAND,message),ptr);
  }


// Clicked in list
long FXIconList::onClicked(FXObject*,FXSelector,void* ptr){
  return target && target->tryHandle(this,FXSEL(SEL_CLICKED,message),ptr);
  }


// Double Clicked in list; ptr may or may not point to an item
long FXIconList::onDoubleClicked(FXObject*,FXSelector,void* ptr){
  return target && target->tryHandle(this,FXSEL(SEL_DOUBLECLICKED,message),ptr);
  }


// Triple Clicked in list; ptr may or may not point to an item
long FXIconList::onTripleClicked(FXObject*,FXSelector,void* ptr){
  return target && target->tryHandle(this,FXSEL(SEL_TRIPLECLICKED,message),ptr);
  }


// Create custom item
FXIconItem *FXIconList::createItem(const FXString& text,FXIcon *big,FXIcon* mini,FXptr ptr){
  return new FXIconItem(text,big,mini,ptr);
  }


// Retrieve item
FXIconItem *FXIconList::getItem(FXint index) const {
  if(index<0 || items.no()<=index){ fxerror("%s::getItem: index out of range.\n",getClassName()); }
  return items[index];
  }


// Replace item with another
FXint FXIconList::setItem(FXint index,FXIconItem* item,FXbool notify){
  if(index<0 || items.no()<=index){ fxerror("%s::setItem: index out of range.\n",getClassName()); }
  if(items[index]!=item){
    FXIconItem *orig=items[index];

    // Must have item
    if(!item){ fxerror("%s::setItem: item is NULL.\n",getClassName()); }

    // Notify old item will be deleted
    if(notify && target){
      target->tryHandle(this,FXSEL(SEL_DELETED,message),(void*)(FXival)index);
      }

    // Keep item state bits
    item->setFocus(orig->hasFocus());
    item->setSelected(orig->isSelected());

    // Add new
    items[index]=item;

    // Notify new item has been inserted
    if(notify && target){
      target->tryHandle(this,FXSEL(SEL_INSERTED,message),(void*)(FXival)index);
      if(current==index){
        target->tryHandle(this,FXSEL(SEL_CHANGED,message),(void*)(FXival)current);
        }
      }

    // Delete old
    delete orig;

    // Redo layout
    recalc();
    }
  return index;
  }


// Replace item with another
FXint FXIconList::setItem(FXint index,const FXString& text,FXIcon *big,FXIcon* mini,FXptr ptr,FXbool notify){
  return setItem(index,createItem(text,big,mini,ptr),notify);
  }


// Insert item
FXint FXIconList::insertItem(FXint index,FXIconItem* item,FXbool notify){
  FXint old=current;

  // Must have item
  if(!item){ fxerror("%s::insertItem: item is NULL.\n",getClassName()); }

  // Must be in range
  if(index<0 || items.no()<index){ fxerror("%s::insertItem: index out of range.\n",getClassName()); }

  // Add item to list
  items.insert(index,item);

  // Adjust indices
  if(anchor>=index)  anchor++;
  if(extent>=index)  extent++;
  if(current>=index) current++;
  if(viewable>=index) viewable++;
  if(current<0 && items.no()==1) current=0;

  // Notify item has been inserted
  if(notify && target){
    target->tryHandle(this,FXSEL(SEL_INSERTED,message),(void*)(FXival)index);
    if(old!=current){
      target->tryHandle(this,FXSEL(SEL_CHANGED,message),(void*)(FXival)current);
      }
    }

  // Was new item
  if(index==current){
    items[current]->setFocus(hasFocus());
    if((options&SELECT_MASK)==ICONLIST_BROWSESELECT && items[current]->isEnabled()){
      selectItem(current,notify);
      }
    }

  // Redo layout
  recalc();
  return index;
  }


// Insert item
FXint FXIconList::insertItem(FXint index,const FXString& text,FXIcon *big,FXIcon* mini,FXptr ptr,FXbool notify){
  return insertItem(index,createItem(text,big,mini,ptr),notify);
  }


// Append item
FXint FXIconList::appendItem(FXIconItem* item,FXbool notify){
  return insertItem(items.no(),item,notify);
  }


// Append item
FXint FXIconList::appendItem(const FXString& text,FXIcon *big,FXIcon* mini,FXptr ptr,FXbool notify){
  return insertItem(items.no(),createItem(text,big,mini,ptr),notify);
  }


// Prepend item
FXint FXIconList::prependItem(FXIconItem* item,FXbool notify){
  return insertItem(0,item,notify);
  }


// Prepend item
FXint FXIconList::prependItem(const FXString& text,FXIcon *big,FXIcon* mini,FXptr ptr,FXbool notify){
  return insertItem(0,createItem(text,big,mini,ptr),notify);
  }


// Fill list by appending items from array of strings
FXint FXIconList::fillItems(const FXchar** strings,FXIcon *big,FXIcon* mini,FXptr ptr,FXbool notify){
  FXint n=0;
  if(strings){
    while(strings[n]){
      appendItem(strings[n++],big,mini,ptr,notify);
      }
    }
  return n;
  }


// Fill list by appending items from array of strings
FXint FXIconList::fillItems(const FXString* strings,FXIcon *big,FXIcon* mini,FXptr ptr,FXbool notify){
  FXint n=0;
  if(strings){
    while(!strings[n].empty()){
      appendItem(strings[n++],big,mini,ptr,notify);
      }
    }
  return n;
  }


// Fill list by appending items from newline separated strings
FXint FXIconList::fillItems(const FXString& strings,FXIcon *big,FXIcon* mini,FXptr ptr,FXbool notify){
  FXint beg=0,end=0,n=0;
  while(end<strings.length()){
    beg=end;
    while(end<strings.length() && strings[end]!='\n' && strings[end]!='\r') end++;
    appendItem(strings.mid(beg,end-beg),big,mini,ptr,notify);
    while(strings[end]=='\n' || strings[end]=='\r') end++;
    n++;
    }
  return n;
  }


// Move item from oldindex to newindex
FXint FXIconList::moveItem(FXint newindex,FXint oldindex,FXbool notify){
  FXint old=current;
  FXIconItem *item;

  // Must be in range
  if(newindex<0 || oldindex<0 || items.no()<=newindex || items.no()<=oldindex){ fxerror("%s::moveItem: index out of range.\n",getClassName()); }

  // Did it change?
  if(oldindex!=newindex){

    // Move item
    item=items[oldindex];
    items.erase(oldindex);
    items.insert(newindex,item);

    // Move item down
    if(newindex<oldindex){
      if(newindex<=anchor && anchor<oldindex) anchor++;
      if(newindex<=extent && extent<oldindex) extent++;
      if(newindex<=current && current<oldindex) current++;
      if(newindex<=viewable && viewable<oldindex) viewable++;
      }

    // Move item up
    else{
      if(oldindex<anchor && anchor<=newindex) anchor--;
      if(oldindex<extent && extent<=newindex) extent--;
      if(oldindex<current && current<=newindex) current--;
      if(oldindex<viewable && viewable<=newindex) viewable--;
      }

    // Adjust if it was equal
    if(anchor==oldindex) anchor=newindex;
    if(extent==oldindex) extent=newindex;
    if(current==oldindex) current=newindex;
    if(viewable==oldindex) viewable=newindex;

    // Current item may have changed
    if(notify && target && old!=current){
      target->tryHandle(this,FXSEL(SEL_CHANGED,message),(void*)(FXival)current);
      }

    // Redo layout
    recalc();
    }
  return newindex;
  }


// Extract node from list
FXIconItem* FXIconList::extractItem(FXint index,FXbool notify){
  FXIconItem *result;
  FXint old=current;

  // Must be in range
  if(index<0 || items.no()<=index){ fxerror("%s::extractItem: index out of range.\n",getClassName()); }

  // Notify item will be deleted
  if(notify && target){target->tryHandle(this,FXSEL(SEL_DELETED,message),(void*)(FXival)index);}

  // Extract item
  result=items[index];

  // Remove from list
  items.erase(index);

  // Adjust indices
  if(anchor>index || anchor>=items.no())  anchor--;
  if(extent>index || extent>=items.no())  extent--;
  if(current>index || current>=items.no()) current--;
  if(viewable>index || viewable>=items.no())  viewable--;

  // Current item has changed
  if(notify && target && index<=old){
    target->tryHandle(this,FXSEL(SEL_CHANGED,message),(void*)(FXival)current);
    }

  // Deleted current item
  if(0<=current && index==old){
    items[current]->setFocus(hasFocus());
    if((options&SELECT_MASK)==ICONLIST_BROWSESELECT && items[current]->isEnabled()){
      selectItem(current,notify);
      }
    }

  // Redo layout
  recalc();

  // Return item
  return result;
  }


// Remove node from list
void FXIconList::removeItem(FXint index,FXbool notify){
  FXint old=current;

  // Must be in range
  if(index<0 || items.no()<=index){ fxerror("%s::removeItem: index out of range.\n",getClassName()); }

  // Notify item will be deleted
  if(notify && target){
    target->tryHandle(this,FXSEL(SEL_DELETED,message),(void*)(FXival)index);
    }

  // Delete item
  delete items[index];

  // Remove from list
  items.erase(index);

  // Adjust indices
  if(anchor>index || anchor>=items.no())  anchor--;
  if(extent>index || extent>=items.no())  extent--;
  if(current>index || current>=items.no()) current--;
  if(viewable>index || viewable>=items.no())  viewable--;

  // Current item has changed
  if(notify && target && index<=old){
    target->tryHandle(this,FXSEL(SEL_CHANGED,message),(void*)(FXival)current);
    }

  // Deleted current item
  if(0<=current && index==old){
    items[current]->setFocus(hasFocus());
    if((options&SELECT_MASK)==ICONLIST_BROWSESELECT && items[current]->isEnabled()){
      selectItem(current,notify);
      }
    }

  // Redo layout
  recalc();
  }


// Remove all items
void FXIconList::clearItems(FXbool notify){
  FXint old=current;

  // Delete items
  for(FXint index=items.no()-1; 0<=index; index--){
    if(notify && target){
      target->tryHandle(this,FXSEL(SEL_DELETED,message),(void*)(FXival)index);
      }
    delete items[index];
    }

  // Free array
  items.clear();

  // Adjust indices
  current=-1;
  anchor=-1;
  extent=-1;
  viewable=-1;

  // Current item has changed
  if(notify && target && old!=-1){
    target->tryHandle(this,FXSEL(SEL_CHANGED,message),(void*)(FXival)-1);
    }

  // Redo layout
  recalc();
  }


// Change the font
void FXIconList::setFont(FXFont* fnt){
  if(!fnt){ fxerror("%s::setFont: NULL font specified.\n",getClassName()); }
  if(font!=fnt){
    font=fnt;
    recalc();
    update();
    }
  }


// Set text color
void FXIconList::setTextColor(FXColor clr){
  if(clr!=textColor){
    textColor=clr;
    update();
    }
  }


// Set select background color
void FXIconList::setSelBackColor(FXColor clr){
  if(clr!=selbackColor){
    selbackColor=clr;
    update();
    }
  }


// Set selected text color
void FXIconList::setSelTextColor(FXColor clr){
  if(clr!=seltextColor){
    seltextColor=clr;
    update();
    }
  }


// Set text width
void FXIconList::setItemSpace(FXint s){
  if(s<1) s=1;
  if(itemSpace!=s){
    itemSpace=s;
    recalc();
    }
  }


// Change list style
void FXIconList::setListStyle(FXuint style){
  FXuint opts=(options&~ICONLIST_MASK) | (style&ICONLIST_MASK);
  if(options!=opts){
    options=opts;
    recalc();
    }
  }


// Get list style
FXuint FXIconList::getListStyle() const {
  return (options&ICONLIST_MASK);
  }


// Change help text
void FXIconList::setHelpText(const FXString& text){
  help=text;
  }


// Save data
void FXIconList::save(FXStream& store) const {
  FXScrollArea::save(store);
  store << header;
  items.save(store);
  store << nrows;
  store << ncols;
  store << anchor;
  store << current;
  store << extent;
  store << font;
  store << textColor;
  store << selbackColor;
  store << seltextColor;
  store << itemSpace;
  store << itemWidth;
  store << itemHeight;
  store << help;
  }


// Load data
void FXIconList::load(FXStream& store){
  FXScrollArea::load(store);
  store >> header;
  items.load(store);
  store >> nrows;
  store >> ncols;
  store >> anchor;
  store >> current;
  store >> extent;
  store >> font;
  store >> textColor;
  store >> selbackColor;
  store >> seltextColor;
  store >> itemSpace;
  store >> itemWidth;
  store >> itemHeight;
  store >> help;
  }


// Cleanup
FXIconList::~FXIconList(){
  getApp()->removeTimeout(this,ID_TIPTIMER);
  getApp()->removeTimeout(this,ID_LOOKUPTIMER);
  clearItems(false);
  header=(FXHeader*)-1L;
  font=(FXFont*)-1L;
  }

}
