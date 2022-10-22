/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2006-2021 by Sander Jansen. All Rights Reserved      *
*                               ---                                            *
* This program is free software: you can redistribute it and/or modify         *
* it under the terms of the GNU General Public License as published by         *
* the Free Software Foundation, either version 3 of the License, or            *
* (at your option) any later version.                                          *
*                                                                              *
* This program is distributed in the hope that it will be useful,              *
* but WITHOUT ANY WARRANTY; without even the implied warranty of               *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                *
* GNU General Public License for more details.                                 *
*                                                                              *
* You should have received a copy of the GNU General Public License            *
* along with this program.  If not, see http://www.gnu.org/licenses.           *
********************************************************************************/
#include "gmdefs.h"
#include "gmutils.h"
#include "GMTrack.h"
#include "GMApp.h"
#include "GMTrackList.h"
#include "GMSource.h"
#include "GMPlayerManager.h"
#include "GMTrackView.h"
#include "GMCoverCache.h"

#include "GMList.h"

#define ICON_SPACING             4    // Spacing between icon and label
#define SIDE_SPACING             6    // Left or right spacing between items
#define LINE_SPACING             4    // Line spacing between items


static inline FXbool begins_with_keyword(const FXString & t){
  const FXStringList & keywords = GMPlayerManager::instance()->getPreferences().gui_sort_keywords;
  for (FXint i=0;i<keywords.no();i++){
    if (FXString::comparecase(t,keywords[i],keywords[i].length())==0) return true;
    }
  return false;
  }

FXint genre_list_sort(const FXListItem* pa,const FXListItem* pb){
  return FXString::comparecase(pa->getText(),pb->getText());
  }
FXint genre_list_sort_reverse(const FXListItem* pa,const FXListItem* pb){
  return -FXString::comparecase(pa->getText(),pb->getText());
  }


FXint generic_name_sort(const FXListItem* pa,const FXListItem* pb){
  FXint a=0,b=0;
  if (begins_with_keyword(pa->getText())) a=FXMIN(pa->getText().length()-1,pa->getText().find(' ')+1);
  if (begins_with_keyword(pb->getText())) b=FXMIN(pb->getText().length()-1,pb->getText().find(' ')+1);
  return FXString::comparecase(&pa->getText()[a],&pb->getText()[b]);
  }

FXint generic_name_sort_reverse(const FXListItem* pa,const FXListItem* pb){
  FXint a=0,b=0;
  if (begins_with_keyword(pa->getText())) a=FXMIN(pa->getText().length()-1,pa->getText().find(' ')+1);
  if (begins_with_keyword(pb->getText())) b=FXMIN(pb->getText().length()-1,pb->getText().find(' ')+1);
  return FXString::comparecase(&pb->getText()[b],&pa->getText()[a]);
  }

FXint source_list_sort(const FXTreeItem* pa,const FXTreeItem* pb){
  const GMSource * const sa = static_cast<const GMSource*>(pa->getData());
  const GMSource * const sb = static_cast<const GMSource*>(pb->getData());
  if (sa->getType()>sb->getType()) return 1;
  else if (sa->getType()<sb->getType()) return -1;
  FXint a=0,b=0;
  if (begins_with_keyword(pa->getText())) a=FXMIN(pa->getText().length()-1,pa->getText().find(' ')+1);
  if (begins_with_keyword(pb->getText())) b=FXMIN(pb->getText().length()-1,pb->getText().find(' ')+1);
  return FXString::comparenatural(&pa->getText()[a],&pb->getText()[b]);
  }

FXint source_list_sort_reverse(const FXTreeItem* pa,const FXTreeItem* pb){
  const GMSource * const sa = static_cast<const GMSource*>(pa->getData());
  const GMSource * const sb = static_cast<const GMSource*>(pb->getData());
  if (sa->getType()>sb->getType()) return 1;
  else if (sa->getType()<sb->getType()) return -1;
  FXint a=0,b=0;
  if (begins_with_keyword(pa->getText())) a=FXMIN(pa->getText().length()-1,pa->getText().find(' ')+1);
  if (begins_with_keyword(pb->getText())) b=FXMIN(pb->getText().length()-1,pb->getText().find(' ')+1);
  return -FXString::comparenatural(&pa->getText()[a],&pb->getText()[b]);
  }



// Object implementation
FXIMPLEMENT(GMListItem,FXListItem,nullptr,0)



// Draw item
void GMListItem::draw(const GMList* list,FXDC& dc,FXint xx,FXint yy,FXint ww,FXint hh) const {
  FXFont *font=list->getFont();
  FXint ih=0,th=0;
  if(icon) ih=icon->getHeight();
  if(!label.empty()) th=font->getFontHeight();
  if(isSelected())
    dc.setForeground(list->getSelBackColor());
//  else
//    dc.setForeground(list->getBackColor());     // FIXME maybe paint background in onPaint?

  dc.fillRectangle(xx,yy,ww,hh);
  if(hasFocus()){
    dc.drawFocusRectangle(xx+1,yy+1,ww-2,hh-2);
    }
  xx+=SIDE_SPACING/2;
  if(icon){
    dc.drawIcon(icon,xx,yy+(hh-ih)/2);
    xx+=ICON_SPACING+icon->getWidth();
    }
  if(!label.empty()){
    if(!isEnabled())
      dc.setForeground(makeShadowColor(list->getBackColor()));
    else if(isSelected())
      dc.setForeground(list->getSelTextColor());
    else
      dc.setForeground(list->getTextColor());
    dc.drawText(xx,yy+(hh-th)/2+font->getFontAscent(),label);
    }
  }



// Map
FXDEFMAP(GMList) GMListMap[]={
  FXMAPFUNC(SEL_PAINT,0,GMList::onPaint),
  };

// Object implementation
FXIMPLEMENT(GMList,FXList,GMListMap,ARRAYNUMBER(GMListMap))


GMList::GMList(FXComposite *p,FXObject* tgt,FXSelector sel,FXuint opts,FXint x,FXint y,FXint w,FXint h) : FXList(p,tgt,sel,opts,x,y,w,h) {
  rowcolor=GMPlayerManager::instance()->getPreferences().gui_row_color;
  thickfont=font;

  delete vertical;
  delete horizontal;
  delete corner;

  vertical=new GMScrollBar(this,this,GMList::ID_VSCROLLED,SCROLLBAR_VERTICAL);
  horizontal=new GMScrollBar(this,this,GMList::ID_HSCROLLED,SCROLLBAR_HORIZONTAL);
  corner=new GMScrollCorner(this);

  }

GMList::~GMList(){
  }

// Create custom item
FXListItem *GMList::createItem(const FXString& text,FXIcon* icon,void* ptr){
  return new GMListItem(text,icon,ptr);
  }

// Draw item list
long GMList::onPaint(FXObject*,FXSelector,void* ptr){
  FXEvent* event=(FXEvent*)ptr;
  FXDCWindow dc(this,event);
  FXint i,y,h;

  // Set font
  dc.setFont(font);

  // Paint items
  y=pos_y;
  for(i=0; i<items.no(); i++){
    h=items[i]->getHeight(this);
    if(event->rect.y<=y+h && y<event->rect.y+event->rect.h){
      if (i%2)
        dc.setForeground(rowcolor);
      else
        dc.setForeground(backColor);

      if (items[i]->getData()==(void*)(FXival)-1)
        dc.setFont(thickfont);
      else
        dc.setFont(font);
      dynamic_cast<GMListItem*>(items[i])->draw(this,dc,pos_x,y,FXMAX(listWidth,getVisibleWidth()),h);
      }
    y+=h;
    }

  // Paint blank area below items
  if(y<event->rect.y+event->rect.h){
    dc.setForeground(backColor);
    dc.fillRectangle(event->rect.x,y,event->rect.w,event->rect.y+event->rect.h-y);
    }
  return 1;
  }


// Object implementation
FXIMPLEMENT(GMTreeItem,FXTreeItem,nullptr,0)

GMTreeItem::GMTreeItem(){
  }

// Get item height
FXint GMTreeItem::getHeight(const FXTreeList* list) const {
  FXFont *font=list->getFont();
  FXint th=0,oih=0,cih=0;
  if(openIcon) oih=openIcon->getHeight();
  if(closedIcon) cih=closedIcon->getHeight();
  if(!label.empty()) th=font->getFontHeight();
//  if (oih>16) oih+=4;
//  if (cih>16) cih+=4;
  return FXMAX3(th,oih,cih)+4;
  }


// Draw item
void GMTreeItem::draw(const FXTreeList* list,FXDC& dc,FXint xx,FXint yy,FXint /* ww*/,FXint hh) const {
  FXIcon *icon=(state&OPENED)?openIcon:closedIcon;
  FXFont *font=list->getFont();
  FXint th=0,tw=0,ih=0,iw=0;//ox=xx,oy=yy;
  xx+=SIDE_SPACING/2;
  if(icon){
    iw=icon->getWidth();
    ih=icon->getHeight();
    dc.drawIcon(icon,xx,yy+(hh-ih)/2);
    xx+=ICON_SPACING+iw;
    }

  if(!label.empty()){
    tw=4+font->getTextWidth(label.text(),label.length());
    th=4+font->getFontHeight();
    yy+=(hh-th)/2;
//    if(isSelected()){
//      dc.setForeground(list->getSelBackColor());
//      dc.fillRectangle(xx,yy,tw,th);
//      dc.fillRectangle(ox,oy,ww,hh);
//      }
    if(hasFocus()){
      dc.drawFocusRectangle(xx+1,yy+1,tw-2,th-2);
      }
    if(!isEnabled())
      dc.setForeground(makeShadowColor(list->getBackColor()));
    else if(isSelected())
      dc.setForeground(list->getSelTextColor());
    else
      dc.setForeground(list->getTextColor());
    dc.drawText(xx+2,yy+font->getFontAscent()+2,label);
    }
  }



#define DEFAULT_INDENT      8   // Indent between parent and child
#define HALFBOX_SIZE        4   // Half box size
#define BOX_FUDGE           3   // Fudge border around box


// Map
FXDEFMAP(GMTreeList) GMTreeListMap[]={
  FXMAPFUNC(SEL_PAINT,0,GMTreeList::onPaint)
  };

// Object implementation
FXIMPLEMENT(GMTreeList,FXTreeList,GMTreeListMap,ARRAYNUMBER(GMTreeListMap))


/// Construct a new, initially empty tree list
GMTreeList::GMTreeList(FXComposite *p,FXObject* tgt,FXSelector sel,FXuint opts,FXint x,FXint y,FXint w,FXint h) : FXTreeList(p,tgt,sel,opts,x,y,w,h) {
  rowcolor=GMPlayerManager::instance()->getPreferences().gui_row_color;
  GMScrollArea::replaceScrollbars(this);
  }

FXTreeItem* GMTreeList::createItem(const FXString& text,FXIcon* oi,FXIcon* ci,void* ptr) {
  return (FXTreeItem*)new GMTreeItem(text,oi,ci,ptr);
  }

// Draw item list
long GMTreeList::onPaint(FXObject*,FXSelector,void* ptr){
  FXEvent* event=(FXEvent*)ptr;
  FXTreeItem* item=firstitem;
  FXTreeItem* p;
  FXint yh,xh,x,y,w,h,xp,hh,cc=0;
  FXDCWindow dc(this,event);
  dc.setFont(font);
  x=pos_x;
  y=pos_y;
  if(options&TREELIST_ROOT_BOXES) x+=(4+indent);
  while(item && y<event->rect.y+event->rect.h){
    w=item->getWidth(this);
    h=item->getHeight(this);
    cc++;
    if(event->rect.y<=y+h){

      // Draw item
      dc.setForeground(backColor);
      dc.fillRectangle(0,y,width,h);

      if (!item->isSelected()) {
        if (cc%2) {
//          dc.setForeground(backColor);
//          dc.fillRectangle(0,y,x+2,h);

          dc.setForeground(rowcolor);
          dc.fillRectangle(x,y,width-x,h);
  //        dc.fillRectangle(x+2,y,width-x-2,h);
          }
        else {
          dc.setForeground(backColor);
          dc.fillRectangle(0,y,width,h);
          }
        }
      else {
//        dc.setForeground(backColor);
//        dc.fillRectangle(0,y,x+2,h);

        dc.setForeground(getSelBackColor());
//        dc.fillRectangle(x+2,y,width-x-2,h);
        dc.fillRectangle(x,y,width-x,h);
        }

      dynamic_cast<GMTreeItem*>(item)->draw(this,dc,x,y,w,h);


      // Show other paraphernalia such as dotted lines and expand-boxes
      if((options&(TREELIST_SHOWS_LINES|TREELIST_SHOWS_BOXES)) && (((GMTreeItem*)item)->parent || (options&TREELIST_ROOT_BOXES))){
        hh=h/2;
        yh=y+hh;
        xh=x-indent+(SIDE_SPACING/2);
        dc.setForeground(lineColor);
        dc.setBackground(backColor);
        dc.setStipple(STIPPLE_GRAY,pos_x&1,pos_y&1);
        if(options&TREELIST_SHOWS_LINES){                   // Connect items with lines
          p=((GMTreeItem*)item)->parent;
          xp=xh;
          dc.setFillStyle(FILL_OPAQUESTIPPLED);
          while(p){
            xp-=(indent+p->getHeight(this)/2);
            if(((GMTreeItem*)p)->next) dc.fillRectangle(xp,y,1,h);
            p=((GMTreeItem*)p)->parent;
            }
          if((options&TREELIST_SHOWS_BOXES) && (item->hasItems() || item->getFirst())){
            if(((GMTreeItem*)item)->prev || ((GMTreeItem*)item)->parent) dc.fillRectangle(xh,y,1,yh-y-HALFBOX_SIZE);
            if(((GMTreeItem*)item)->next) dc.fillRectangle(xh,yh+HALFBOX_SIZE,1,y+h-yh-HALFBOX_SIZE);
            }
          else{
            if(((GMTreeItem*)item)->prev || ((GMTreeItem*)item)->parent) dc.fillRectangle(xh,y,1,hh);
            if(((GMTreeItem*)item)->next) dc.fillRectangle(xh,yh,1,h);
            dc.fillRectangle(xh,yh,x+(SIDE_SPACING/2)-2-xh,1);
            }
          dc.setFillStyle(FILL_SOLID);
          }

        // Boxes before items for expand/collapse of item
        if((options&TREELIST_SHOWS_BOXES) && (item->hasItems() || item->getFirst())){
          dc.setFillStyle(FILL_OPAQUESTIPPLED);
          dc.fillRectangle(xh+4,yh,(SIDE_SPACING/2)-2,1);
          dc.setFillStyle(FILL_SOLID);
          dc.drawRectangle(xh-HALFBOX_SIZE,yh-HALFBOX_SIZE,HALFBOX_SIZE+HALFBOX_SIZE,HALFBOX_SIZE+HALFBOX_SIZE);
          dc.setForeground(textColor);
          dc.fillRectangle(xh-HALFBOX_SIZE+2,yh,HALFBOX_SIZE+HALFBOX_SIZE-3,1);
          if(!(options&TREELIST_AUTOSELECT) && !item->isExpanded()){
            dc.fillRectangle(xh,yh-HALFBOX_SIZE+2,1,HALFBOX_SIZE+HALFBOX_SIZE-3);
            }
          }
        }
      }

    y+=h;

    // Move on to the next item
    if(((GMTreeItem*)item)->first && ((options&TREELIST_AUTOSELECT) || ((GMTreeItem*)item)->isExpanded())){
      x+=(indent+h/2);
      item=((GMTreeItem*)item)->first;
      continue;
      }
    while(!((GMTreeItem*)item)->next && ((GMTreeItem*)item)->parent){
      item=((GMTreeItem*)item)->parent;
      x-=(indent+item->getHeight(this)/2);
      }
    item=((GMTreeItem*)item)->next;
    }
  if(y<event->rect.y+event->rect.h){
    dc.setForeground(backColor);
    dc.fillRectangle(event->rect.x,y,event->rect.w,event->rect.y+event->rect.h-y);
    }
  return 1;
  }
