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
#include "FXFrame.h"
#include "FXLabel.h"
#include "FXComposite.h"
#include "FXPacker.h"
#include "FXButton.h"
#include "FXArrowButton.h"
#include "FXMenuButton.h"
#include "FXToolTip.h"
#include "FXOptionMenu.h"
#include "FXHorizontalFrame.h"
#include "FXFont.h"
#include "FXDate.h"
#include "FXPopup.h"
#include "FXCalendarView.h"
#include "FXCalendar.h"

/*
  Notes:
  - Most of calendar is implemented in FXCalendarView
*/

#define CALENDAR_MASK (CALENDAR_SINGLESELECT|CALENDAR_BROWSESELECT|CALENDAR_WEEKNUMBERS|CALENDAR_STATIC|CALENDAR_HIDEOTHER)


using namespace FX;

/*******************************************************************************/

namespace FX {


// Map
FXDEFMAP(FXCalendar) FXCalendarMap[]={
  FXMAPFUNC(SEL_REPLACED,FXCalendar::ID_CALENDAR,FXCalendar::onCmdDate),
  FXMAPFUNCS(SEL_COMMAND,FXCalendar::ID_MONTH_START,FXCalendar::ID_MONTH_END,FXCalendar::onCmdMonth),
  FXMAPFUNC(SEL_COMMAND,FXCalendar::ID_NEXTYEAR,FXCalendar::onCmdNextYear),
  FXMAPFUNC(SEL_COMMAND,FXCalendar::ID_PREVYEAR,FXCalendar::onCmdPrevYear),
  FXMAPFUNC(SEL_COMMAND,FXCalendar::ID_NEXTMONTH,FXCalendar::onCmdNextMonth),
  FXMAPFUNC(SEL_COMMAND,FXCalendar::ID_PREVMONTH,FXCalendar::onCmdPrevMonth),
  FXMAPFUNC(SEL_COMMAND,FXCalendar::ID_MONTH,FXCalendar::onCmdSelectMonth),
  FXMAPFUNC(SEL_COMMAND,FXWindow::ID_SETVALUE,FXCalendar::onFwdToView),
  FXMAPFUNC(SEL_COMMAND,FXWindow::ID_SETINTVALUE,FXCalendar::onFwdToView),
  FXMAPFUNC(SEL_COMMAND,FXWindow::ID_GETINTVALUE,FXCalendar::onFwdToView),
  FXMAPFUNC(SEL_COMMAND,FXCalendar::ID_CALENDAR,FXCalendar::onFwdToTarget),
  FXMAPFUNC(SEL_CHANGED,FXCalendar::ID_CALENDAR,FXCalendar::onFwdToTarget),
  FXMAPFUNC(SEL_UPDATE,FXCalendar::ID_CALENDAR,FXCalendar::onFwdToTarget),
  };


// Implementation
FXIMPLEMENT(FXCalendar,FXPacker,FXCalendarMap,ARRAYNUMBER(FXCalendarMap));


FXCalendar::FXCalendar(){
  flags|=FLAG_ENABLED;
  }


// Construct and initialize calendar widget
FXCalendar::FXCalendar(FXComposite *p,FXObject* tgt,FXSelector sel,FXuint opts,FXint x,FXint y,FXint w,FXint h):FXPacker(p,opts,x,y,w,h,0,0,0,0,0,0){
  flags|=FLAG_ENABLED;
  target=tgt;
  message=sel;

  // Caption above calendar
  frame=new FXHorizontalFrame(this,LAYOUT_SIDE_TOP|LAYOUT_FILL_X,0,0,0,0,0,0,0,0,0,0);

  // Popup for months
  monthpopup=new FXPopup(this);
  months[ 0]=new FXOption(monthpopup,tr(FXDate::monthName( 1)),nullptr,this,ID_MONTH_START+ 0,OPTIONMENU_NOGLYPH|LAYOUT_LEFT|JUSTIFY_CENTER_X|ICON_AFTER_TEXT);
  months[ 1]=new FXOption(monthpopup,tr(FXDate::monthName( 2)),nullptr,this,ID_MONTH_START+ 1,OPTIONMENU_NOGLYPH|LAYOUT_LEFT|JUSTIFY_CENTER_X|ICON_AFTER_TEXT);
  months[ 2]=new FXOption(monthpopup,tr(FXDate::monthName( 3)),nullptr,this,ID_MONTH_START+ 2,OPTIONMENU_NOGLYPH|LAYOUT_LEFT|JUSTIFY_CENTER_X|ICON_AFTER_TEXT);
  months[ 3]=new FXOption(monthpopup,tr(FXDate::monthName( 4)),nullptr,this,ID_MONTH_START+ 3,OPTIONMENU_NOGLYPH|LAYOUT_LEFT|JUSTIFY_CENTER_X|ICON_AFTER_TEXT);
  months[ 4]=new FXOption(monthpopup,tr(FXDate::monthName( 5)),nullptr,this,ID_MONTH_START+ 4,OPTIONMENU_NOGLYPH|LAYOUT_LEFT|JUSTIFY_CENTER_X|ICON_AFTER_TEXT);
  months[ 5]=new FXOption(monthpopup,tr(FXDate::monthName( 6)),nullptr,this,ID_MONTH_START+ 5,OPTIONMENU_NOGLYPH|LAYOUT_LEFT|JUSTIFY_CENTER_X|ICON_AFTER_TEXT);
  months[ 6]=new FXOption(monthpopup,tr(FXDate::monthName( 7)),nullptr,this,ID_MONTH_START+ 6,OPTIONMENU_NOGLYPH|LAYOUT_LEFT|JUSTIFY_CENTER_X|ICON_AFTER_TEXT);
  months[ 7]=new FXOption(monthpopup,tr(FXDate::monthName( 8)),nullptr,this,ID_MONTH_START+ 7,OPTIONMENU_NOGLYPH|LAYOUT_LEFT|JUSTIFY_CENTER_X|ICON_AFTER_TEXT);
  months[ 8]=new FXOption(monthpopup,tr(FXDate::monthName( 9)),nullptr,this,ID_MONTH_START+ 8,OPTIONMENU_NOGLYPH|LAYOUT_LEFT|JUSTIFY_CENTER_X|ICON_AFTER_TEXT);
  months[ 9]=new FXOption(monthpopup,tr(FXDate::monthName(10)),nullptr,this,ID_MONTH_START+ 9,OPTIONMENU_NOGLYPH|LAYOUT_LEFT|JUSTIFY_CENTER_X|ICON_AFTER_TEXT);
  months[10]=new FXOption(monthpopup,tr(FXDate::monthName(11)),nullptr,this,ID_MONTH_START+10,OPTIONMENU_NOGLYPH|LAYOUT_LEFT|JUSTIFY_CENTER_X|ICON_AFTER_TEXT);
  months[11]=new FXOption(monthpopup,tr(FXDate::monthName(12)),nullptr,this,ID_MONTH_START+11,OPTIONMENU_NOGLYPH|LAYOUT_LEFT|JUSTIFY_CENTER_X|ICON_AFTER_TEXT);

  // Month selector
  arrows[0]=new FXArrowButton(frame,this,ID_PREVMONTH,ARROW_LEFT|ARROW_REPEAT|ARROW_TOOLBAR|FRAME_RAISED|LAYOUT_FILL_Y,0,0,0,0,2,2,4,4);
  month=new FXOptionMenu(frame,monthpopup,OPTIONMENU_NOGLYPH|OPTIONMENU_TOOLBAR|FRAME_RAISED|JUSTIFY_CENTER_X|JUSTIFY_CENTER_Y|LAYOUT_CENTER_Y,0,0,0,0);
  month->setTarget(this);
  month->setSelector(ID_MONTH);
  arrows[1]=new FXArrowButton(frame,this,ID_NEXTMONTH,ARROW_RIGHT|ARROW_REPEAT|ARROW_TOOLBAR|FRAME_RAISED|LAYOUT_FILL_Y,0,0,0,0,2,2,4,4);

  // Year selector
  arrows[2]=new FXArrowButton(frame,this,ID_NEXTYEAR,ARROW_RIGHT|ARROW_REPEAT|ARROW_TOOLBAR|FRAME_RAISED|LAYOUT_FILL_Y|LAYOUT_RIGHT,0,0,0,0,2,2,4,4);
  year=new FXLabel(frame,"0000",nullptr,LAYOUT_RIGHT|LAYOUT_CENTER_Y);
  arrows[3]=new FXArrowButton(frame,this,ID_PREVYEAR,ARROW_LEFT|ARROW_REPEAT|ARROW_TOOLBAR|FRAME_RAISED|LAYOUT_FILL_Y|LAYOUT_RIGHT,0,0,0,0,2,2,4,4);

  // Main widget
  view=new FXCalendarView(this,this,ID_CALENDAR,(options&CALENDAR_MASK)|LAYOUT_FILL_X|LAYOUT_FILL_Y|LAYOUT_SIDE_BOTTOM);

  // Fix the options
  if(options&FRAME_SUNKEN && !(options&FRAME_RAISED)){
    if(options&FRAME_THICK)
      frame->setFrameStyle(FRAME_RAISED|FRAME_THICK);
    else
      frame->setFrameStyle(FRAME_RAISED);
    }
  else{
    frame->setFrameStyle(FRAME_NONE);
    }
  }


// Create X window
void FXCalendar::create(){
  FXDate date(view->getCurrentDate());
  FXPacker::create();
  year->setText(FXString::value(date.year()));
  month->setCurrentNo(date.month()-1);
  }


// Enable the window
void FXCalendar::enable(){
  FXPacker::enable();
  view->enable();
  year->enable();
  month->enable();
  arrows[0]->enable();
  arrows[1]->enable();
  arrows[2]->enable();
  arrows[3]->enable();
  }


// Disable the window
void FXCalendar::disable(){
  FXPacker::disable();
  view->disable();
  year->disable();
  month->disable();
  arrows[0]->disable();
  arrows[1]->disable();
  arrows[2]->disable();
  arrows[3]->disable();
  }


// Set date
void FXCalendar::setCurrentDate(FXDate date,FXbool notify){
  view->setCurrentDate(date,notify);
  }


// Get the current date
FXDate FXCalendar::getCurrentDate() const {
  return view->getCurrentDate();
  }


// Set the current month
void FXCalendar::setCurrentMonth(FXint mo,FXbool notify){
  view->setCurrentMonth(mo,notify);
  }


// Return the current month shown
FXint FXCalendar::getCurrentMonth() const {
  return view->getCurrentMonth();
  }


// Set the Calendar Style
void FXCalendar::setCalendarStyle(FXuint style){
  view->setCalendarStyle(style);
  }


// Get the Calendar Style
FXuint FXCalendar::getCalendarStyle() const {
  return view->getCalendarStyle();
  }


// Set the first day of the week [0 -> 6]
void FXCalendar::setFirstDay(FXint d){
  view->setFirstDay(d);
  view->update();
  }


// Get the first day of the week [0 -> 6]
FXint FXCalendar::getFirstDay() const {
  return view->getFirstDay();
  }


// Change the Frame Style
void FXCalendar::setFrameStyle(FXuint s){
  FXPacker::setFrameStyle(s);
  if(options&FRAME_SUNKEN && !(options&FRAME_RAISED)){
    if(options&FRAME_THICK)
      frame->setFrameStyle(FRAME_RAISED|FRAME_THICK);
    else
      frame->setFrameStyle(FRAME_RAISED);
    }
  else{
    frame->setFrameStyle(FRAME_NONE);
    }
  }


// Set the back color
void FXCalendar::setBackColor(FXColor c){
  FXPacker::setBackColor(c);
  view->setBackColor(c);
  }


// Get the back color
FXColor FXCalendar::getBackColor() const {
  return view->getBackColor();
  }


// Set the display color of titles
void FXCalendar::setTitleColor(FXColor c){
  view->setTitleColor(c);
  }


// Get the display color of titles
FXColor FXCalendar::getTitleColor() const {
  return view->getTitleColor();
  }


// Set the display color of titles
void FXCalendar::setTitleBackColor(FXColor c){
  view->setTitleBackColor(c);
  }


// Get the display color of titles
FXColor FXCalendar::getTitleBackColor() const {
  return view->getTitleBackColor();
  }


// Set the display color of non-weekend days
void FXCalendar::setDayColor(FXColor c){
  view->setDayColor(c);
  }


// Get the display color of non-weekend days
FXColor FXCalendar::getDayColor() const {
  return view->getDayColor();
  }


// Set the display color of non-weekend days not in the current month
void FXCalendar::setOtherDayColor(FXColor c){
  view->setOtherDayColor(c);
  }


// Get the display color of non-weekend days not in the current month
FXColor FXCalendar::getOtherDayColor() const {
  return view->getOtherDayColor();
  }


// Set the display color of today
void FXCalendar::setTodayColor(FXColor c){
  view->setTodayColor(c);
  }


// Get the display color of today
FXColor FXCalendar::getTodayColor() const {
  return view->getTodayColor();
  }


// Set the display color of days in the weekend
void FXCalendar::setWeekendColor(FXColor c){
  view->setWeekendColor(c);
  }


// Get the display color of days in the weekend
FXColor FXCalendar::getWeekendColor() const {
  return view->getWeekendColor();
  }


// Set the display color of days in the weekend not in the current month
void FXCalendar::setOtherWeekendColor(FXColor c){
  view->setOtherWeekendColor(c);
  }


// Get the display color of days in the weekend not in the current month
FXColor FXCalendar::getOtherWeekendColor() const {
  return view->getOtherWeekendColor();
  }


// Set font used by the header
void FXCalendar::setHeaderFont(FXFont *fnt){
  year->setFont(fnt);
  months[0]->setFont(fnt);
  months[1]->setFont(fnt);
  months[2]->setFont(fnt);
  months[3]->setFont(fnt);
  months[4]->setFont(fnt);
  months[5]->setFont(fnt);
  months[6]->setFont(fnt);
  months[7]->setFont(fnt);
  months[8]->setFont(fnt);
  months[9]->setFont(fnt);
  months[10]->setFont(fnt);
  months[11]->setFont(fnt);
  }


// Get font used by the header
FXFont* FXCalendar::getHeaderFont() const {
  return year->getFont();
  }


// Set font used by the calendar
void FXCalendar::setCalendarFont(FXFont *fnt){
  view->setFont(fnt);
  }


// Get font used by the calendar
FXFont* FXCalendar::getCalendarFont() const {
  return view->getFont();
  }


// Switch date
long FXCalendar::onCmdDate(FXObject*,FXSelector,void* ptr){
  FXDate date((FXuint)(FXival)(ptr));
  year->setText(FXString::value(date.year()));
  month->setCurrentNo(view->getCurrentMonth()-1);
  if(target) target->tryHandle(this,FXSEL(SEL_CHANGED,message),ptr);
  return 1;
  }


// Switch month
long FXCalendar::onCmdMonth(FXObject*,FXSelector sel,void*){
  view->setCurrentMonth((FXSELID(sel)-ID_MONTH_START)+1,true);
  return 1;
  }


// Select month
long FXCalendar::onCmdSelectMonth(FXObject*,FXSelector,void*ptr){
  view->setCurrentMonth(1+(FXint)(FXival)(ptr),true);
  return 1;
  }


// Go to next year
long FXCalendar::onCmdNextYear(FXObject*,FXSelector,void*){
  FXDate date=view->getCurrentDate();
  date.addYears(1);
  view->setCurrentDate(date,true);
  return 1;
  }


// Go to previous year
long FXCalendar::onCmdPrevYear(FXObject*,FXSelector,void*){
  FXDate date=view->getCurrentDate();
  date.addYears(-1);
  view->setCurrentDate(date,true);
  return 1;
  }


// Go to next month
long FXCalendar::onCmdNextMonth(FXObject*,FXSelector,void*){
  FXDate date=view->getCurrentDate();
  date.addMonths(1);
  view->setCurrentDate(date,true);
  return 1;
  }


// Go to previous month
long FXCalendar::onCmdPrevMonth(FXObject*,FXSelector,void*){
  FXDate date=view->getCurrentDate();
  date.addMonths(-1);
  view->setCurrentDate(date,true);
  return 1;
  }


// Forward to calendar view
long FXCalendar::onFwdToView(FXObject*sender,FXSelector sel,void*ptr){
  return view->handle(sender,sel,ptr);
  }


// Forward from calendar view
long FXCalendar::onFwdToTarget(FXObject*,FXSelector sel,void* ptr){
  return target && target->tryHandle(this,FXSEL(FXSELTYPE(sel),message),ptr);
  }


// Destroy
FXCalendar::~FXCalendar(){
  delete monthpopup;
  view=(FXCalendarView*)-1L;
  year=(FXLabel*)-1L;
  month=(FXOptionMenu*)-1L;
  monthpopup=(FXPopup*)-1L;
  frame=(FXHorizontalFrame*)-1L;
  months[0]=(FXOption*)-1L;
  months[1]=(FXOption*)-1L;
  months[2]=(FXOption*)-1L;
  months[3]=(FXOption*)-1L;
  months[4]=(FXOption*)-1L;
  months[5]=(FXOption*)-1L;
  months[6]=(FXOption*)-1L;
  months[7]=(FXOption*)-1L;
  months[8]=(FXOption*)-1L;
  months[9]=(FXOption*)-1L;
  months[10]=(FXOption*)-1L;
  months[11]=(FXOption*)-1L;
  arrows[0]=(FXArrowButton*)-1L;
  arrows[1]=(FXArrowButton*)-1L;
  arrows[2]=(FXArrowButton*)-1L;
  arrows[3]=(FXArrowButton*)-1L;
  }


}
