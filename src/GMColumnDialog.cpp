/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2007-2014 by Sander Jansen. All Rights Reserved      *
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
#include "GMList.h"
#include "GMTrackList.h"
#include "GMColumnDialog.h"

#define ICON_SPACING             4    // Spacing between icon and label
#define SIDE_SPACING             6    // Left or right spacing between items
#define LINE_SPACING             4    // Line spacing between items


class FXCheckListItem : public GMListItem {
  FXDECLARE(FXCheckListItem)
  friend class FXList;
protected:
  FXCheckListItem(){}
  virtual void draw(const GMList* list,FXDC& dc,FXint x,FXint y,FXint w,FXint h) const;
  virtual FXint hitItem(const FXList* list,FXint x,FXint y) const;
public:
  enum {
    CHECKED = 32
    };
public:
  /// Construct new item with given text, icon, and user-data
  FXCheckListItem(const FXString& text,FXbool check=true,void* ptr=NULL):GMListItem(text,NULL,ptr) {if (check) state|=CHECKED; }

  /// Return width of item as drawn in list
  virtual FXint getWidth(const FXList* list) const;

  /// Return height of item as drawn in list
  virtual FXint getHeight(const FXList* list) const;

  FXbool checked() const { return (state&CHECKED); }

  void toggle() { if (state&CHECKED) state&=~CHECKED; else state|=CHECKED; }
  };

FXIMPLEMENT(FXCheckListItem,FXListItem,NULL,0);

void FXCheckListItem::draw(const GMList* list,FXDC& dc,FXint xx,FXint yy,FXint ww,FXint hh) const {
//#endif
  FXFont *font=list->getFont();
  FXint ih=0,th=0;
  if(icon) ih=icon->getHeight();
  if(!label.empty()) th=font->getFontHeight();
  if(isSelected())
    dc.setForeground(list->getSelBackColor());
  else
    dc.setForeground(list->getBackColor());     // FIXME maybe paint background in onPaint?
  dc.fillRectangle(xx,yy,ww,hh);

  if(hasFocus()){
    dc.drawFocusRectangle(xx+1,yy+1,ww-2,hh-2);
    }
  xx+=SIDE_SPACING/2;

  FXint yyy=yy+(hh-10)/2;

  if(!isEnabled())
    dc.setForeground(makeShadowColor(list->getBackColor()));
  else
    dc.setForeground(list->getBackColor());

  dc.fillRectangle(xx+1,yyy+1,9,9);
  dc.setForeground(list->getApp()->getBorderColor());

//    dc.setForeground(shadowColor);
  dc.drawRectangle(xx,yyy,10,10);


//  dc.drawRectangle(xx,yyy,9,9);

  dc.setForeground(list->getTextColor());
  // Draw the check
  if(state&CHECKED){
    FXSegment seg[6];
    seg[0].x1=2+xx; seg[0].y1=4+yyy; seg[0].x2=4+xx; seg[0].y2=6+yyy;
    seg[1].x1=2+xx; seg[1].y1=5+yyy; seg[1].x2=4+xx; seg[1].y2=7+yyy;
    seg[2].x1=2+xx; seg[2].y1=6+yyy; seg[2].x2=4+xx; seg[2].y2=8+yyy;
    seg[3].x1=4+xx; seg[3].y1=6+yyy; seg[3].x2=8+xx; seg[3].y2=2+yyy;
    seg[4].x1=4+xx; seg[4].y1=7+yyy; seg[4].x2=8+xx; seg[4].y2=3+yyy;
    seg[5].x1=4+xx; seg[5].y1=8+yyy; seg[5].x2=8+xx; seg[5].y2=4+yyy;
    dc.drawLineSegments(seg,6);
    }

  xx+=ICON_SPACING+9;
  if(icon){
    dc.drawIcon(icon,xx,yy+(hh-ih)/2);
    xx+=ICON_SPACING+icon->getWidth();
    }

  if(!label.empty()){
    dc.setFont(font);
    if(!isEnabled())
      dc.setForeground(makeShadowColor(list->getBackColor()));
    else if(isSelected())
      dc.setForeground(list->getSelTextColor());
    else
      dc.setForeground(list->getTextColor());
    dc.drawText(xx,yy+(hh-th)/2+font->getFontAscent(),label);
    }
  }


// See if item got hit, and where: 0 is outside, 1 is icon, 2 is text
FXint FXCheckListItem::hitItem(const FXList* list,FXint xx,FXint yy) const {
  FXFont *font=list->getFont();
  FXint iw=0,ih=0,tw=0,th=0,cx,ix,iy,tx,ty,h;
  if(icon){
    iw=icon->getWidth();
    ih=icon->getHeight();
    }
  if(!label.empty()){
    tw=4+font->getTextWidth(label.text(),label.length());
    th=4+font->getFontHeight();
    }
  h=LINE_SPACING+FXMAX3(th,ih,9);
  cx=SIDE_SPACING/2;
  ix=cx+9+ICON_SPACING;
  tx=cx+9+ICON_SPACING;
  if(iw) tx+=iw+ICON_SPACING;
  iy=(h-ih)/2;
  ty=(h-th)/2;

  if (cx<=xx && xx<cx+9) return 3;
  // In icon?
  if(ix<=xx && iy<=yy && xx<ix+iw && yy<iy+ih) return 1;

  // In text?
  if(tx<=xx && ty<=yy && xx<tx+tw && yy<ty+th) return 2;

  // Outside
  return 0;
  }


// Get width of item
FXint FXCheckListItem::getWidth(const FXList* list) const {
  FXFont *font=list->getFont();
  FXint w=9;
  if(icon){
    w=icon->getWidth()+ICON_SPACING;
    }
  if(!label.empty()){
    if(w) w+=ICON_SPACING;
    w+=font->getTextWidth(label.text(),label.length());
    }
  return SIDE_SPACING+w;
  }


// Get height of item
FXint FXCheckListItem::getHeight(const FXList* list) const {
  FXFont *font=list->getFont();
  FXint th=0,ih=0;
  if(icon){
    ih=icon->getHeight();
    }
  if(!label.empty()){
    th=font->getFontHeight();
    }
  return LINE_SPACING+FXMAX3(th,ih,9);
  }


FXDEFMAP(GMColumnDialog) GMColumnDialogMap[]={
  FXMAPFUNC(SEL_UPDATE,GMColumnDialog::ID_MOVE_UP,GMColumnDialog::onUpdMoveUp),
  FXMAPFUNC(SEL_UPDATE,GMColumnDialog::ID_MOVE_DOWN,GMColumnDialog::onUpdMoveDown),
  FXMAPFUNC(SEL_COMMAND,GMColumnDialog::ID_MOVE_UP,GMColumnDialog::onCmdMoveUp),
  FXMAPFUNC(SEL_COMMAND,GMColumnDialog::ID_MOVE_DOWN,GMColumnDialog::onCmdMoveDown),
  FXMAPFUNC(SEL_LEFTBUTTONPRESS,GMColumnDialog::ID_LIST,GMColumnDialog::onListLeftBtnPress),

  };

FXIMPLEMENT(GMColumnDialog,FXDialogBox,GMColumnDialogMap,ARRAYNUMBER(GMColumnDialogMap));

GMColumnDialog::GMColumnDialog(){
  }

GMColumnDialog::GMColumnDialog(FXWindow *window,GMColumnList & cols) : FXDialogBox(window,FXString::null,DECOR_ALL,0,0,0,0,3,3,3,3) {

  setTitle(tr("Configure Columns"));

  FXHorizontalFrame * buttonframe = new FXHorizontalFrame(this,LAYOUT_FILL_X|LAYOUT_SIDE_BOTTOM,0,0,0,0,0,0,0,0);
  new GMButton(buttonframe,tr("&Accept"),NULL,this,FXDialogBox::ID_ACCEPT,BUTTON_INITIAL|BUTTON_DEFAULT|LAYOUT_RIGHT|FRAME_RAISED|FRAME_THICK,0,0,0,0,20,20);
  new GMButton(buttonframe,tr("&Cancel"),NULL,this,FXDialogBox::ID_CANCEL,BUTTON_DEFAULT|LAYOUT_RIGHT|FRAME_RAISED|FRAME_THICK,0,0,0,0,20,20);

  new FXSeparator(this,SEPARATOR_GROOVE|LAYOUT_FILL_X|LAYOUT_SIDE_BOTTOM);

  new FXLabel(this,tr("Choose the order of information to appear\nin the track list."),NULL,LAYOUT_SIDE_TOP|JUSTIFY_LEFT);


  FXHorizontalFrame * main = new FXHorizontalFrame(this,LAYOUT_FILL_X|LAYOUT_FILL_Y,0,0,0,0,0,0,0,0);

  FXVerticalFrame * control = new FXVerticalFrame(main,LAYOUT_FILL_Y|LAYOUT_RIGHT|PACK_UNIFORM_WIDTH,0,0,0,0,0,0,0);
  new GMButton(control,tr("Move Up"),NULL,this,ID_MOVE_UP);
  new GMButton(control,tr("Move Down"),NULL,this,ID_MOVE_DOWN);

  GMScrollFrame * sunken = new GMScrollFrame(main);
  list = new GMList(sunken,this,ID_LIST,LAYOUT_FILL_X|LAYOUT_FILL_Y|LIST_BROWSESELECT);

  FXbool insert=false;
  for (FXint i=0;i<cols.no();i++){
    insert=false;
    for (FXint j=0;j<list->getNumItems();j++) {
      if (cols[i].index < ((GMColumn*)list->getItemData(j))->index) {
        list->insertItem(j,new FXCheckListItem(cols[i].name,cols[i].show,&cols[i]));
        insert=true;
        break;
        }
      }
    if (!insert) list->appendItem(new FXCheckListItem(cols[i].name,cols[i].show,&cols[i]));
    }
  list->setNumVisible(10);
  }

void GMColumnDialog::saveIndex() {
  for (FXint i=0;i<list->getNumItems();i++){
    GMColumn * c = (GMColumn*)list->getItemData(i);
    c->index = i;
    c->show  = ((FXCheckListItem*)list->getItem(i))->checked();
    }
  }

long GMColumnDialog::onListLeftBtnPress(FXObject*,FXSelector,void*ptr){
  FXEvent* event=(FXEvent*)ptr;
  FXint index,code;
  index=list->getItemAt(event->win_x,event->win_y);
  if (index>=0) {
    code=list->hitItem(index,event->win_x,event->win_y);
    if (code==3) { ((FXCheckListItem*)list->getItem(index))->toggle(); list->updateItem(index); }
    }
  return 0;
  }


long GMColumnDialog::onCmdMoveUp(FXObject*,FXSelector,void*){
  list->moveItem(list->getCurrentItem()-1,list->getCurrentItem());
  return 1;
  }

long GMColumnDialog::onUpdMoveUp(FXObject*sender,FXSelector,void*){
  if (list->getCurrentItem()>0)
    sender->handle(this,FXSEL(SEL_COMMAND,ID_ENABLE),NULL);
  else
    sender->handle(this,FXSEL(SEL_COMMAND,ID_DISABLE),NULL);
  return 1;
  }

long GMColumnDialog::onCmdMoveDown(FXObject*,FXSelector,void*){
  list->moveItem(list->getCurrentItem()+1,list->getCurrentItem());
  return 1;
  }

long GMColumnDialog::onUpdMoveDown(FXObject*sender,FXSelector,void*){
  if (list->getCurrentItem()<list->getNumItems()-1)
    sender->handle(this,FXSEL(SEL_COMMAND,ID_ENABLE),NULL);
  else
    sender->handle(this,FXSEL(SEL_COMMAND,ID_DISABLE),NULL);
  return 1;
  }
