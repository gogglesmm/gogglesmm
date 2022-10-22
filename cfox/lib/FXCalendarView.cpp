/********************************************************************************
*                                                                               *
*                   B a s e   C a l e n d a r   W i d g e t                     *
*                                                                               *
*********************************************************************************
* Copyright (C) 2006,2022 by Sander Jansen.   All Rights Reserved.              *
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
#include "fxascii.h"
#include "fxkeys.h"
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
#include "FXEvent.h"
#include "FXWindow.h"
#include "FXDCWindow.h"
#include "FXApp.h"
#include "FXFont.h"
#include "FXDate.h"
#include "FXCalendarView.h"

/*
  Notes:

  - ISO8601 weeknumbers are implemented for weeks starting at monday and ending at sunday
*/

#define SELECT_MASK   (CALENDAR_SINGLESELECT|CALENDAR_BROWSESELECT)
#define CALENDAR_MASK (CALENDAR_SINGLESELECT|CALENDAR_BROWSESELECT|CALENDAR_WEEKNUMBERS|CALENDAR_STATIC|CALENDAR_HIDEOTHER)

using namespace FX;

/*******************************************************************************/

namespace FX {


// Map
FXDEFMAP(FXCalendarView) FXCalendarViewMap[]={
  FXMAPFUNC(SEL_PAINT,0,FXCalendarView::onPaint),
  FXMAPFUNC(SEL_LEFTBUTTONPRESS,0,FXCalendarView::onLeftBtnPress),
  FXMAPFUNC(SEL_LEFTBUTTONRELEASE,0,FXCalendarView::onLeftBtnRelease),
  FXMAPFUNC(SEL_KEYPRESS,0,FXCalendarView::onKeyPress),
  FXMAPFUNC(SEL_FOCUSIN,0,FXCalendarView::onFocusIn),
  FXMAPFUNC(SEL_FOCUSOUT,0,FXCalendarView::onFocusOut),
  FXMAPFUNC(SEL_CLICKED,0,FXCalendarView::onClicked),
  FXMAPFUNC(SEL_DOUBLECLICKED,0,FXCalendarView::onDoubleClicked),
  FXMAPFUNC(SEL_TRIPLECLICKED,0,FXCalendarView::onTripleClicked),
  FXMAPFUNC(SEL_COMMAND,0,FXCalendarView::onCommand),
  FXMAPFUNC(SEL_COMMAND,FXWindow::ID_SETVALUE,FXCalendarView::onCmdSetValue),
  FXMAPFUNC(SEL_COMMAND,FXWindow::ID_SETINTVALUE,FXCalendarView::onCmdSetIntValue),
  FXMAPFUNC(SEL_COMMAND,FXWindow::ID_GETINTVALUE,FXCalendarView::onCmdGetIntValue),
  };


// Implementation
FXIMPLEMENT(FXCalendarView,FXWindow,FXCalendarViewMap,ARRAYNUMBER(FXCalendarViewMap))


// For serialization
FXCalendarView::FXCalendarView(){
  flags|=FLAG_SHOWN|FLAG_ENABLED;
  font=(FXFont*)-1L;
  }


// Construct and initialize
FXCalendarView::FXCalendarView(FXComposite *p,FXObject* tgt,FXSelector sel,FXuint opts,FXint x,FXint y,FXint w,FXint h):FXWindow(p,opts,x,y,w,h){
  flags|=FLAG_SHOWN|FLAG_ENABLED;
  font=getApp()->getNormalFont();
  target=tgt;
  message=sel;
  has_selection=false;
  selected=current=FXDate::localDate();
  firstday=FXDate::Sun; // Sunday

  // Setup Default Colors
  todayColor=FXRGB(0,180,0);
  titleColor=getApp()->getBackColor();
  titleBackColor=getApp()->getShadowColor();
  dayColor=getApp()->getForeColor();
  otherDayColor=getApp()->getShadowColor();
  weekendColor=FXRGB(180,0,0);
  otherWeekendColor=makeHiliteColor(FXRGB(200,100,100));
  backColor=getApp()->getBackColor();

  // Update for date
  updateview(false);
  }


// Create X window
void FXCalendarView::create(){
  FXWindow::create();
  font->create();
  }


// Detach window
void FXCalendarView::detach(){
  FXWindow::detach();
  font->detach();
  }


// Enable the window
void FXCalendarView::enable(){
  if(!(flags&FLAG_ENABLED)){
    FXWindow::enable();
    update();
    }
  }


// Disable the window
void FXCalendarView::disable(){
  if(flags&FLAG_ENABLED){
    FXWindow::disable();
    update();
    }
  }


// Set the Calendar Style
void FXCalendarView::setCalendarStyle(FXuint style){
  FXuint opts=((style^options)&CALENDAR_MASK)^options;
  if(options!=opts){
    options=opts;
    recalc();
    layout();
    update();
    }
  }


// Get the Calendar Style
FXuint FXCalendarView::getCalendarStyle() const{
  return (options&CALENDAR_MASK);
  }


// Set the display color of titles
void FXCalendarView::setTitleColor(FXColor clr){
  if(titleColor!=clr){
    titleColor=clr;
    update();
    }
  }


// Set the display background color of titles
void FXCalendarView::setTitleBackColor(FXColor clr){
  if(titleBackColor!=clr){
    titleBackColor=clr;
    update();
    }
  }


// Set the display color of non-weekend days
void FXCalendarView::setDayColor(FXColor clr){
  if(dayColor!=clr){
    dayColor=clr;
    update();
    }
  }


// Set the display color of non-weekend days not in the current month
void FXCalendarView::setOtherDayColor(FXColor clr){
  if(otherDayColor!=clr){
    otherDayColor=clr;
    update();
    }
  }


// Set the display color of today
void FXCalendarView::setTodayColor(FXColor clr){
  if(todayColor!=clr){
    todayColor=clr;
    update();
    }
  }


// Set the display color of days in the weekend
void FXCalendarView::setWeekendColor(FXColor clr){
  if(weekendColor!=clr){
    weekendColor=clr;
    update();
    }
  }


// Set the display color of days in the weekend not in the current month
void FXCalendarView::setOtherWeekendColor(FXColor clr){
  if(otherWeekendColor!=clr){
    otherWeekendColor=clr;
    update();
    }
  }


// Compute default width
FXint FXCalendarView::getDefaultWidth(){
  FXint cw=0,tw,i;
  FXString text;
  for(i=0; i<7; i++){
    text=tr(FXDate::dayNameShort(i));
    tw=font->getTextWidth(text);
    if(tw>cw) cw=tw;
    }
  if(options&CALENDAR_WEEKNUMBERS){
    text=tr("Wk");
    tw=font->getTextWidth(text);
    if(tw>cw) cw=tw;
    return (cw+4)*8;
    }
  return (cw+4)*7;
  }


// Compute default height
FXint FXCalendarView::getDefaultHeight(){
  return (font->getFontHeight()+4)*7;
  }


// Into focus chain
void FXCalendarView::setFocus(){
  FXWindow::setFocus();
  setDefault(true);
  flags&=~FLAG_UPDATE;
  }


// Out of focus chain
void FXCalendarView::killFocus(){
  FXWindow::killFocus();
  setDefault(maybe);
  flags|=FLAG_UPDATE;
  }


// Set focus on given date
void FXCalendarView::moveFocus(FXDate f){
  if((options&CALENDAR_STATIC) && f.month()!=month) return;
  setCurrentDate(f,true);
  }


// Get date at x,y if any
FXbool FXCalendarView::getDateAt(FXint x,FXint y,FXDate& date) const {
  FXint ncols=(options&CALENDAR_WEEKNUMBERS)?8:7;
  FXint nrows=7;
  FXint cw=width/ncols;
  FXint ch=height/nrows;
  FXint cwr=width%ncols;
  FXint chr=height%nrows;
  FXint col=0,row=0,xx=0,yy=0;
  FXint columnwidth[8];
  FXint rowheight[7];
  FXint i,e;

  // Calculate column widths
  for(i=e=0; i<ncols; i++){
    columnwidth[i]=cw;
    e+=cwr;
    if(e>=ncols){ columnwidth[i]++; e-=ncols; }
    }

  // Calculate row heights
  for(i=e=0; i<nrows; i++){
    rowheight[i]=ch;
    e+=chr;
    if(e>=nrows){ rowheight[i]++; e-=nrows; }
    }

  // Filter out obvious cases (clicked on headers)
  if(y<rowheight[0]) return false;
  if(options&CALENDAR_WEEKNUMBERS && x<columnwidth[0]) return false;

  // Determine where we clicked
  while(xx<x && col<ncols) xx+=columnwidth[col++];
  while(yy<y && row<nrows) yy+=rowheight[row++];
  if(options&CALENDAR_WEEKNUMBERS) col-=1;
  col-=1;
  row-=2;
  date=ds+(row*7)+col;
  return true;
  }


// Redraw widget
long FXCalendarView::onPaint(FXObject*,FXSelector,void* ptr){
  FXDate now=FXDate::localDate();
  FXint coloffset=(options&CALENDAR_WEEKNUMBERS)?1:0;
  FXbool displayselected=(has_selection || ((options&SELECT_MASK)==CALENDAR_BROWSESELECT));
  FXint ncols=(options&CALENDAR_WEEKNUMBERS)?8:7;
  FXint nrows=7;
  FXint cw=width/ncols;
  FXint ch=height/nrows;
  FXint cwr=width%ncols;
  FXint chr=height%nrows;
  FXint textwidth=font->getTextWidth("88",2);
  FXint fontheight=font->getFontHeight();
  FXint i=0,c=0,r=0,e=0;
  FXint xx=0,yy=0;
  FXint columnwidth[8];      // Width of each column
  FXint columnoffset[8];     // Text Offset in each column
  FXint rowheight[7];        // Height of each column
  FXString text;
  FXDate pd=ds;

  // The acual painting
  FXDCWindow dc(this,(FXEvent*)ptr);

  // Calculate column widths
  for(i=0,e=0; i<ncols; i++){
    columnwidth[i]=cw;
    e+=cwr;
    if(e>=ncols){ columnwidth[i]++; e-=ncols; }
    columnoffset[i]=(columnwidth[i]-textwidth)/2;
    }

  // Calculate row heights
  for(i=0,e=0; i<nrows; i++){
    rowheight[i]=ch;
    e+=chr;
    if(e>=nrows){ rowheight[i]++; e-=nrows; }
    }

  // Paint backgrounds
  if(isEnabled()){
    dc.setForeground(backColor);
    }
  else{
    dc.setForeground(getApp()->getBaseColor());
    }

  dc.fillRectangle(0,rowheight[0],width,height-rowheight[0]);
  dc.setForeground(titleBackColor);
  dc.fillRectangle(0,0,width,rowheight[0]);

  // Paint separator between week numbers and days
  if(options&CALENDAR_WEEKNUMBERS) {
    dc.setForeground(getApp()->getForeColor());
    dc.fillRectangle(columnwidth[0]-1,0,1,height);
    }

  // Paint header
  dc.setForeground(titleColor);
  dc.setFont(font);

  // Week number caption
  if(options&CALENDAR_WEEKNUMBERS){
    text=tr("Wk");
    dc.drawText((columnwidth[0]-font->getTextWidth(text))/2,(rowheight[0]-fontheight)/2+font->getFontAscent(),text);
    xx+=columnwidth[0];
    }

  // Day of week captions
  for(c=firstday,i=coloffset; c<firstday+7; c++,i++){
    text=tr(FXDate::dayNameShort(c%7));
    if(c%7==0 || c%7==6){
      dc.setForeground(weekendColor);
      }
    else{
      dc.setForeground(titleColor);
      }
    dc.drawText(xx+(columnwidth[i]-font->getTextWidth(text))/2,(rowheight[0]-fontheight)/2+font->getFontAscent(),text);
    xx+=columnwidth[i];
    }

  // Next row
  yy+=rowheight[0];

  // Paint days
  dc.setForeground(getApp()->getForeColor());
  for(r=1; r<nrows; r++){
    xx=0;

    if(options&CALENDAR_WEEKNUMBERS){
      text.fromInt(pd.weekOfYear());
      dc.setForeground(dayColor);

      // Draw Text
      dc.drawText(xx+columnwidth[0]-font->getTextWidth(text)-columnoffset[0],yy+(rowheight[r]-fontheight)/2+font->getFontAscent(),text);
      xx+=columnwidth[0];
      }


    for(c=coloffset; c<ncols; c++){
      if((options&CALENDAR_HIDEOTHER) && pd.month()!=month){
        ++pd; // Next day
        xx+=columnwidth[c]; // Next column
        continue;
        }

      text.fromInt(pd.day());

      // Determine drawing colors
      if(displayselected && selected==pd){
        if(isEnabled()){
          dc.setForeground(getApp()->getSelbackColor());
          }
        else{
          dc.setForeground(getApp()->getShadowColor());
          }
        dc.fillRectangle(xx,yy,columnwidth[c],rowheight[r]);
        dc.setForeground(getApp()->getSelforeColor());
        }
      else{
        if(pd==now){
          dc.setForeground(todayColor);
          }
        else{
          if(pd.dayOfWeek()==0 || pd.dayOfWeek()==6){
            if(pd.month()==month)
              dc.setForeground(weekendColor);
            else
              dc.setForeground(otherWeekendColor);
            }
          else{
            if(pd.month()==month)
              dc.setForeground(dayColor);
            else
              dc.setForeground(otherDayColor);
            }
          }
        }

      // Draw Text
      dc.drawText(xx+columnwidth[c]-font->getTextWidth(text)-columnoffset[c],yy+(rowheight[r]-fontheight)/2+font->getFontAscent(),text);

      if(hasFocus() && current==pd){
        dc.drawFocusRectangle(xx,yy,columnwidth[c],rowheight[r]);
        }

      ++pd; // Next day
      xx+=columnwidth[c]; // Next column
      }

    // Next Row
    yy+=rowheight[r];
    }
  return 1;
  }



// Pressed left button
long FXCalendarView::onLeftBtnPress(FXObject*,FXSelector,void* ptr){
  FXEvent* ev=(FXEvent*)ptr;
  flags&=~FLAG_TIP;
  handle(this,FXSEL(SEL_FOCUS_SELF,0),ptr);
  if(isEnabled()){
    grab();
    if(target && target->tryHandle(this,FXSEL(SEL_LEFTBUTTONPRESS,message),ptr)) return 1;
    flags&=~FLAG_UPDATE;
    FXDate cd;
    if(getDateAt(ev->click_x,ev->click_y,cd)){
      if(((options&CALENDAR_STATIC) || (options&CALENDAR_HIDEOTHER)) && cd.month()!=month){
        flags|=FLAG_PRESSED;
        return 1;
        }
      setCurrentDate(cd,true);
      if((options&SELECT_MASK)==CALENDAR_SINGLESELECT){
        if(has_selection && current==selected)
          state=true;
        else
          state=false;
        selectDate(cd,true);
        }
      }
    flags|=FLAG_PRESSED;
    return 1;
    }
  return 0;
  }


// If window can have focus
FXbool FXCalendarView::canFocus() const {
  return true;
  }


// Gained focus
long FXCalendarView::onFocusIn(FXObject* sender,FXSelector sel,void* ptr){
  FXWindow::onFocusIn(sender,sel,ptr);
  markdirty(current);
  return 1;
  }


// Lost focus
long FXCalendarView::onFocusOut(FXObject* sender,FXSelector sel,void* ptr){
  FXWindow::onFocusOut(sender,sel,ptr);
  markdirty(current);
  return 1;
  }


// Released left button
long FXCalendarView::onLeftBtnRelease(FXObject*,FXSelector,void* ptr){
  FXEvent* event=(FXEvent*)ptr;
  if(isEnabled()){
    ungrab();
    flags|=FLAG_UPDATE;
    flags&=~FLAG_PRESSED;
    if(target && target->tryHandle(this,FXSEL(SEL_LEFTBUTTONRELEASE,message),ptr)) return 1;

    if((options&SELECT_MASK)==CALENDAR_SINGLESELECT){
      if(state){
        killSelection(true);
        }
      }

    // Generate clicked callbacks
    if(event->click_count==1){
      handle(this,FXSEL(SEL_CLICKED,0),(void*)(FXuval)current.getJulian());
      }
    else if(event->click_count==2){
      handle(this,FXSEL(SEL_DOUBLECLICKED,0),(void*)(FXuval)current.getJulian());
      }
    else if(event->click_count==3){
      handle(this,FXSEL(SEL_TRIPLECLICKED,0),(void*)(FXuval)current.getJulian());
      }

    handle(this,FXSEL(SEL_COMMAND,0),(void*)(FXuval)current.getJulian());
    return 1;
    }
  return 0;
  }


// Command message
long FXCalendarView::onCommand(FXObject*,FXSelector,void* ptr){
  return target && target->tryHandle(this,FXSEL(SEL_COMMAND,message),ptr);
  }


// Clicked in list
long FXCalendarView::onClicked(FXObject*,FXSelector,void* ptr){
  return target && target->tryHandle(this,FXSEL(SEL_CLICKED,message),ptr);
  }


// Double clicked in list; ptr may or may not point to an item
long FXCalendarView::onDoubleClicked(FXObject*,FXSelector,void* ptr){
  return target && target->tryHandle(this,FXSEL(SEL_DOUBLECLICKED,message),ptr);
  }


// Triple clicked in list; ptr may or may not point to an item
long FXCalendarView::onTripleClicked(FXObject*,FXSelector,void* ptr){
  return target && target->tryHandle(this,FXSEL(SEL_TRIPLECLICKED,message),ptr);
  }


// Update value from a message
long FXCalendarView::onCmdSetValue(FXObject*,FXSelector,void* ptr){
  setCurrentDate(FXDate((FXuint)(FXuval)ptr));
  return 1;
  }


// Obtain value from list
long FXCalendarView::onCmdGetIntValue(FXObject*,FXSelector,void* ptr){
  *((FXuint*)ptr)=getCurrentDate().getJulian();
  return 1;
  }


// Update value from a message
long FXCalendarView::onCmdSetIntValue(FXObject*,FXSelector,void* ptr){
  setCurrentDate(FXDate(*((FXuint*)ptr)));
  return 1;
  }


// Key pressed
long FXCalendarView::onKeyPress(FXObject*sender,FXSelector sel,void* ptr){
  FXEvent* event=(FXEvent*)ptr;
  FXDate date=current;
  flags&=~FLAG_TIP;

  // Bounce to focus widget
  if(getFocus() && getFocus()->handle(sender,sel,ptr)) return 1;

  if(!isEnabled()) return 0;

  // Try target first
  if(target && target->tryHandle(this,FXSEL(SEL_KEYPRESS,message),ptr)) return 1;

  // Eat keystroke
  switch(event->code){
    case KEY_Up:
    case KEY_KP_Up:
      moveFocus(current-7);
      return 1;
      break;
    case KEY_Down:
    case KEY_KP_Down:
      moveFocus(current+7);
      return 1;
      break;
    case KEY_Right:
    case KEY_KP_Right:
      moveFocus(current+1);
      return 1;
      break;
    case KEY_Left:
    case KEY_KP_Left:
      moveFocus(current-1);
      return 1;
      break;
    case KEY_space:
    case KEY_Return:
    case KEY_KP_Enter:
      if(has_selection && current==selected)
        killSelection(true);
      else
        selectDate(current);
      return 1;
      break;
    case KEY_Page_Down:
    case KEY_KP_Page_Down:
      date.addMonths(1);
      setCurrentDate(date,true);
      return 1;
      break;
    case KEY_Page_Up:
    case KEY_KP_Page_Up:
      date.addMonths(-1);
      setCurrentDate(date,true);
      return 1;
      break;
    case KEY_Home:
    case KEY_KP_Home:
      setCurrentDate(FXDate::localDate(),true);
      return 1;
      break;
    case KEY_End:
    case KEY_KP_End:
      if(has_selection)
        setCurrentDate(selected,true);
      else
        getApp()->beep();
      return 1;
      break;
     }
  return 0;
  }


// Mark as dirty
void FXCalendarView::markdirty(FXDate d){
  if(xid){
    FXint ncols = ((options&CALENDAR_WEEKNUMBERS) ? 8 : 7);
    FXint nrows = 7;
    FXint cw    = width / ncols;
    FXint ch    = height / nrows;
    FXint cwr   = width % ncols;
    FXint chr   = height % nrows;
    FXint days  = d-ds;
    FXint col   = (days%7) + ((options&CALENDAR_WEEKNUMBERS) ? 1 : 0);
    FXint row   = (days/7) + 1;
    FXint xx=0,yy=0,ww=0,hh=0,i=0,e=0;

    // Calculate xx
    for(i=0,e=0; i<col; i++){
      xx+=cw;
      e+=cwr;
      if(e>=ncols){ xx++; e-=ncols; }
      }

    // Calculate ww
    ww=cw;
    e+=cwr;
    if(e>=ncols){ ww++; e-=ncols; }

    // Calculate yy
    for(i=0,e=0; i<row; i++){
      yy+=ch;
      e+=chr;
      if(e>=nrows){ yy++; e-=nrows; }
      }

    // Calculate hh
    hh=ch;
    e+=chr;
    if(e>=nrows){ hh++; e-=nrows; }

    update(xx,yy,ww,hh);
    }
  }


// Update view
void FXCalendarView::updateview(FXbool notify){
  FXDate ms=(current-(current.day()-1));
  month=current.month();
  ds=ms-(7-(firstday-ms.dayOfWeek()))%7;
  if(target && notify) target->tryHandle(this,FXSEL(SEL_REPLACED,message),(void*)(FXival)current.getJulian());
  }


// Set the current month; current day will be properly updated for the choosen month
void FXCalendarView::setCurrentMonth(FXint m,FXbool notify){
  if(m<1 || m>12){ fxerror("%s::setCurrentMonth: Invalid month argument.\n",getClassName()); }
  if(month!=m){

    // Set a new current date
    FXint day=current.day();
    current.setDate(current.year(),m,1);
    if(current.daysInMonth()>day)
      current.setDate(current.year(),m,day);
    else
      current.setDate(current.year(),m,current.daysInMonth());

    // Update the GUI and do the same as setCurrentDate would do.
    // Don't call setCurrentDate since it wouldn't update the complete view.
    updateview();
    update(0,0,width,height);

    // Notify item change
    if(notify && target){ target->tryHandle(this,FXSEL(SEL_CHANGED,message),(void*)(FXival)current.getJulian()); }

    // In browse select mode, select this item
    if((options&SELECT_MASK)==CALENDAR_BROWSESELECT){
      selectDate(current,notify);
      }
    }
  }


// Get the selected date, if any
FXbool FXCalendarView::getSelectedDate(FXDate& date) const {
  if(((options&SELECT_MASK)==CALENDAR_SINGLESELECT)){
    if(has_selection){
      date=selected;
      return true;
      }
    return false;
    }
  else{
    date=selected;
    return true;
    }
  }


// Select Date
void FXCalendarView::selectDate(FXDate d,FXbool notify){
  if(d!=selected || !has_selection){

    has_selection=true;

    // Within the current month view
    if((selected>=ds) && (selected<=(ds+41))){
      markdirty(selected);
      }

    // Within the current month view
    if((d.month()==month) && (d.year()==selected.year())){
      selected=d;
      markdirty(selected);
      }
    else{
      selected=d;
      updateview();
      update(0,0,width,height);
      }

    if(notify && target){ target->tryHandle(this,FXSEL(SEL_SELECTED,message),(void*)(FXival)selected.getJulian()); }
    }
  }


// Deselect Date
void FXCalendarView::killSelection(FXbool notify){
  if(((options&SELECT_MASK)==CALENDAR_SINGLESELECT) && has_selection){
    has_selection=false;
    if(notify && target){ target->tryHandle(this,FXSEL(SEL_DESELECTED,message),(void*)(FXival)selected.getJulian()); }
    markdirty(selected);
    }
  }


// Set Date
void FXCalendarView::setCurrentDate(FXDate d,FXbool notify){
  if(d!=current){

    // Update View and repaint everyting
    if(d<ds || d>(ds+41) || (((options&CALENDAR_HIDEOTHER) || ((options&SELECT_MASK)==CALENDAR_BROWSESELECT)) && d.month()!=month)){
      current=d;
      updateview();
      update(0,0,width,height);
      }

    // Within the current month view. just mark fields used dirty
    else{
      markdirty(current);
      markdirty(d);
      current=d;
      }

    // Notify item change
    if(notify && target){ target->tryHandle(this,FXSEL(SEL_CHANGED,message),(void*)(FXival)current.getJulian()); }
    }

  // In browse select mode, select this item
  if((options&SELECT_MASK)==CALENDAR_BROWSESELECT){
    selectDate(current,notify);
    }
  }


// Set the first day of the week [0...6]
void FXCalendarView::setFirstDay(FXint d){
  if(d<0 || d>6){ fxerror("%s::setFirstDay: invalid day argument.\n",getClassName()); }
  if(firstday!=d){
    firstday=d;
    updateview();
    update();
    }
  }


// Change the font
void FXCalendarView::setFont(FXFont *fnt){
  if(!fnt){ fxerror("%s::setFont: NULL font specified.\n",getClassName()); }
  if(font!=fnt){
    font=fnt;
    recalc();
    update();
    }
  }


// Destroy it
FXCalendarView::~FXCalendarView(){
  font=(FXFont*)-1L;
  }

}

