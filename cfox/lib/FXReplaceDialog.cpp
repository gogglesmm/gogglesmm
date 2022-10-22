/********************************************************************************
*                                                                               *
*                      T e x t   R e p l a c e   D i a l o g                    *
*                                                                               *
*********************************************************************************
* Copyright (C) 2000,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "FXApp.h"
#include "FXGIFIcon.h"
#include "FXWindow.h"
#include "FXFrame.h"
#include "FXArrowButton.h"
#include "FXSeparator.h"
#include "FXLabel.h"
#include "FXButton.h"
#include "FXCheckButton.h"
#include "FXRadioButton.h"
#include "FXPacker.h"
#include "FXScrollBar.h"
#include "FXTextField.h"
#include "FXVerticalFrame.h"
#include "FXHorizontalFrame.h"
#include "FXReplaceDialog.h"



/*
  Notes:

  - Keep history of strings previously searched, and allow up/down arrows
    to "scroll" back to this history.
  - Simplified: just keep dialog open until explicitly closed.  This allows
    you to hammer return repeatedly to search and replace.
  - We think ^N and ^P is more natural to scroll through history; then we
    can use the arrows to search up and down in the content.
*/

// Padding for buttons
#define HORZ_PAD      12
#define VERT_PAD      2

using namespace FX;

/*******************************************************************************/

namespace FX {


// Search and replace dialog registry section name
const FXchar FXReplaceDialog::sectionName[]="SearchReplace";

// Value key names
static const FXchar skey[20][3]={"SA","SB","SC","SD","SE","SF","SG","SH","SI","SJ","SK","SL","SM","SN","SO","SP","SQ","SR","SS","ST"};
static const FXchar rkey[20][3]={"RA","RB","RC","RD","RE","RF","RG","RH","RI","RJ","RK","RL","RM","RN","RO","RP","RQ","RR","RS","RT"};
static const FXchar mkey[20][3]={"MA","MB","MC","MD","ME","MF","MG","MH","MI","MJ","MK","ML","MM","MN","MO","MP","MQ","MR","MS","MT"};


// Map
FXDEFMAP(FXReplaceDialog) FXReplaceDialogMap[]={
  FXMAPFUNC(SEL_UPDATE,FXReplaceDialog::ID_SEARCH,FXReplaceDialog::onUpdSearch),
  FXMAPFUNC(SEL_COMMAND,FXReplaceDialog::ID_SEARCH,FXReplaceDialog::onCmdSearch),
  FXMAPFUNC(SEL_UPDATE,FXReplaceDialog::ID_REPLACE,FXReplaceDialog::onUpdSearch),
  FXMAPFUNC(SEL_COMMAND,FXReplaceDialog::ID_REPLACE,FXReplaceDialog::onCmdReplace),
  FXMAPFUNC(SEL_COMMAND,FXReplaceDialog::ID_SEARCH_TEXT,FXReplaceDialog::onCmdSearch),
  FXMAPFUNC(SEL_KEYPRESS,FXReplaceDialog::ID_SEARCH_TEXT,FXReplaceDialog::onSearchKey),
  FXMAPFUNC(SEL_MOUSEWHEEL,FXReplaceDialog::ID_SEARCH_TEXT,FXReplaceDialog::onWheelSearch),
  FXMAPFUNC(SEL_COMMAND,FXReplaceDialog::ID_REPLACE_TEXT,FXReplaceDialog::onCmdReplace),
  FXMAPFUNC(SEL_KEYPRESS,FXReplaceDialog::ID_REPLACE_TEXT,FXReplaceDialog::onReplaceKey),
  FXMAPFUNC(SEL_MOUSEWHEEL,FXReplaceDialog::ID_REPLACE_TEXT,FXReplaceDialog::onWheelReplace),
  FXMAPFUNC(SEL_UPDATE,FXReplaceDialog::ID_REPLACE_SEL,FXReplaceDialog::onUpdSearch),
  FXMAPFUNC(SEL_COMMAND,FXReplaceDialog::ID_REPLACE_SEL,FXReplaceDialog::onCmdReplaceSel),
  FXMAPFUNC(SEL_UPDATE,FXReplaceDialog::ID_REPLACE_ALL,FXReplaceDialog::onUpdSearch),
  FXMAPFUNC(SEL_COMMAND,FXReplaceDialog::ID_REPLACE_ALL,FXReplaceDialog::onCmdReplaceAll),
  FXMAPFUNC(SEL_UPDATE,FXReplaceDialog::ID_DIR,FXReplaceDialog::onUpdDir),
  FXMAPFUNC(SEL_COMMAND,FXReplaceDialog::ID_DIR,FXReplaceDialog::onCmdDir),
  FXMAPFUNC(SEL_UPDATE,FXReplaceDialog::ID_WRAP,FXReplaceDialog::onUpdWrap),
  FXMAPFUNC(SEL_COMMAND,FXReplaceDialog::ID_WRAP,FXReplaceDialog::onCmdWrap),
  FXMAPFUNC(SEL_UPDATE,FXReplaceDialog::ID_CASE,FXReplaceDialog::onUpdCase),
  FXMAPFUNC(SEL_COMMAND,FXReplaceDialog::ID_CASE,FXReplaceDialog::onCmdCase),
  FXMAPFUNC(SEL_UPDATE,FXReplaceDialog::ID_REGEX,FXReplaceDialog::onUpdRegex),
  FXMAPFUNC(SEL_COMMAND,FXReplaceDialog::ID_REGEX,FXReplaceDialog::onCmdRegex),
  FXMAPFUNC(SEL_COMMAND,FXReplaceDialog::ID_SEARCH_UP,FXReplaceDialog::onCmdSearchHistUp),
  FXMAPFUNC(SEL_COMMAND,FXReplaceDialog::ID_SEARCH_DN,FXReplaceDialog::onCmdSearchHistDn),
  FXMAPFUNC(SEL_COMMAND,FXReplaceDialog::ID_REPLACE_UP,FXReplaceDialog::onCmdReplaceHistUp),
  FXMAPFUNC(SEL_COMMAND,FXReplaceDialog::ID_REPLACE_DN,FXReplaceDialog::onCmdReplaceHistDn),
  };


// Object implementation
FXIMPLEMENT(FXReplaceDialog,FXDialogBox,FXReplaceDialogMap,ARRAYNUMBER(FXReplaceDialogMap))


// Search and Replace Dialog
FXReplaceDialog::FXReplaceDialog(FXWindow* own,const FXString& caption,FXIcon* icn,FXuint opts,FXint x,FXint y,FXint w,FXint h):FXDialogBox(own,caption,opts|DECOR_TITLE|DECOR_BORDER|DECOR_RESIZE,x,y,w,h,10,10,10,10, 10,10){
  FXHorizontalFrame* buttons=new FXHorizontalFrame(this,LAYOUT_SIDE_BOTTOM|LAYOUT_RIGHT,0,0,0,0,0,0,0,0);
  search=new FXButton(buttons,tr("&Search\tSearch\tSearch for pattern."),nullptr,this,ID_SEARCH,BUTTON_INITIAL|BUTTON_DEFAULT|FRAME_RAISED|FRAME_THICK|LAYOUT_FILL_Y,0,0,0,0,HORZ_PAD,HORZ_PAD,VERT_PAD,VERT_PAD);
  replace=new FXButton(buttons,tr("&Replace\tReplace\tSearch and replace pattern."),nullptr,this,ID_REPLACE,BUTTON_DEFAULT|FRAME_RAISED|FRAME_THICK|LAYOUT_FILL_Y,0,0,0,0,HORZ_PAD,HORZ_PAD,VERT_PAD,VERT_PAD);
  replacesel=new FXButton(buttons,tr("Replace Sel\tReplace in selection\tReplace all in selection."),nullptr,this,ID_REPLACE_SEL,BUTTON_DEFAULT|FRAME_RAISED|FRAME_THICK|LAYOUT_CENTER_Y,0,0,0,0,6,6,VERT_PAD,VERT_PAD);
  replaceall=new FXButton(buttons,tr("Replace All\tReplace all\tReplace all."),nullptr,this,ID_REPLACE_ALL,BUTTON_DEFAULT|FRAME_RAISED|FRAME_THICK|LAYOUT_CENTER_Y,0,0,0,0,6,6,VERT_PAD,VERT_PAD);
  cancel=new FXButton(buttons,tr("&Close"),nullptr,this,ID_CANCEL,BUTTON_DEFAULT|FRAME_RAISED|FRAME_THICK|LAYOUT_FILL_Y,0,0,0,0,HORZ_PAD,HORZ_PAD,VERT_PAD,VERT_PAD);
  new FXHorizontalSeparator(this,SEPARATOR_GROOVE|LAYOUT_SIDE_BOTTOM|LAYOUT_FILL_X);
  FXHorizontalFrame* toppart=new FXHorizontalFrame(this,LAYOUT_SIDE_BOTTOM|LAYOUT_FILL_X|LAYOUT_CENTER_Y,0,0,0,0, 0,0,0,0, 10,10);
  new FXLabel(toppart,FXString::null,icn,ICON_BEFORE_TEXT|JUSTIFY_CENTER_X|JUSTIFY_CENTER_Y|LAYOUT_FILL_Y|LAYOUT_FILL_X);
  FXVerticalFrame* entry=new FXVerticalFrame(toppart,LAYOUT_FILL_X|LAYOUT_CENTER_Y,0,0,0,0, 0,0,0,0);
  searchlabel=new FXLabel(entry,tr("Search &For:"),nullptr,JUSTIFY_LEFT|ICON_BEFORE_TEXT|LAYOUT_TOP|LAYOUT_LEFT|LAYOUT_FILL_X);
  searchbox=new FXHorizontalFrame(entry,FRAME_SUNKEN|FRAME_THICK|LAYOUT_FILL_X|LAYOUT_CENTER_Y,0,0,0,0, 0,0,0,0, 0,0);
  searchtext=new FXTextField(searchbox,26,this,ID_SEARCH_TEXT,TEXTFIELD_ENTER_ONLY|LAYOUT_FILL_X|LAYOUT_FILL_Y,0,0,0,0,4,4,2,2);
  FXVerticalFrame* searcharrows=new FXVerticalFrame(searchbox,LAYOUT_RIGHT|LAYOUT_FILL_Y,0,0,0,0, 0,0,0,0, 0,0);
  FXArrowButton* ar1=new FXArrowButton(searcharrows,this,ID_SEARCH_UP,FRAME_RAISED|FRAME_THICK|ARROW_UP|ARROW_REPEAT|LAYOUT_FILL_Y|LAYOUT_FIX_WIDTH, 0,0,16,0, 1,1,1,1);
  FXArrowButton* ar2=new FXArrowButton(searcharrows,this,ID_SEARCH_DN,FRAME_RAISED|FRAME_THICK|ARROW_DOWN|ARROW_REPEAT|LAYOUT_FILL_Y|LAYOUT_FIX_WIDTH, 0,0,16,0, 1,1,1,1);
  ar1->setArrowSize(3);
  ar2->setArrowSize(3);
  replacelabel=new FXLabel(entry,tr("Re&place With:"),nullptr,LAYOUT_LEFT);
  replacebox=new FXHorizontalFrame(entry,FRAME_SUNKEN|FRAME_THICK|LAYOUT_FILL_X|LAYOUT_CENTER_Y,0,0,0,0, 0,0,0,0, 0,0);
  replacetext=new FXTextField(replacebox,26,this,ID_REPLACE_TEXT,TEXTFIELD_ENTER_ONLY|LAYOUT_FILL_X|LAYOUT_FILL_Y,0,0,0,0,4,4,2,2);
  FXVerticalFrame* replacearrows=new FXVerticalFrame(replacebox,LAYOUT_RIGHT|LAYOUT_FILL_Y,0,0,0,0, 0,0,0,0, 0,0);
  FXArrowButton* ar3=new FXArrowButton(replacearrows,this,ID_REPLACE_UP,FRAME_RAISED|FRAME_THICK|ARROW_UP|ARROW_REPEAT|LAYOUT_FILL_Y|LAYOUT_FIX_WIDTH, 0,0,16,0, 1,1,1,1);
  FXArrowButton* ar4=new FXArrowButton(replacearrows,this,ID_REPLACE_DN,FRAME_RAISED|FRAME_THICK|ARROW_DOWN|ARROW_REPEAT|LAYOUT_FILL_Y|LAYOUT_FIX_WIDTH, 0,0,16,0, 1,1,1,1);
  ar3->setArrowSize(3);
  ar4->setArrowSize(3);
  FXHorizontalFrame* options1=new FXHorizontalFrame(entry,LAYOUT_FILL_X,0,0,0,0, 0,0,0,0);
  new FXCheckButton(options1,tr("&Expression\tRegular Expression\tPerform regular-expression match."),this,ID_REGEX,ICON_BEFORE_TEXT|LAYOUT_CENTER_X);
  new FXCheckButton(options1,tr("&Ignore Case\tCase Insensitive\tPerform case-insentive match."),this,ID_CASE,ICON_BEFORE_TEXT|LAYOUT_CENTER_X);
  new FXCheckButton(options1,tr("&Wrap\tWrap Around\tWrap around when searching."),this,ID_WRAP,ICON_BEFORE_TEXT|LAYOUT_CENTER_X);
  new FXCheckButton(options1,tr("&Backward\tSearch Direction\tChange search direction."),this,ID_DIR,ICON_BEFORE_TEXT|LAYOUT_CENTER_X);
  replacetext->setHelpText(tr("Text to replace with."));
  searchmode=SEARCH_EXACT|SEARCH_FORWARD|SEARCH_WRAP;
  loadHistory();
  }


// Set text or pattern to search for
void FXReplaceDialog::setSearchText(const FXString& text){
  searchtext->setText(text);
  }


// Return text or pattern the user has entered
FXString FXReplaceDialog::getSearchText() const {
  return searchtext->getText();
  }


// Set text or pattern to search with
void FXReplaceDialog::setReplaceText(const FXString& text){
  replacetext->setText(text);
  }


// Return text or pattern the user has entered
FXString FXReplaceDialog::getReplaceText() const {
  return replacetext->getText();
  }


// Change search text color
void FXReplaceDialog::setSearchTextColor(FXColor clr){
  searchtext->setBackColor(clr);
  }


// Return search text color
FXColor FXReplaceDialog::getSearchTextColor() const {
  return searchtext->getBackColor();
  }


// Change replace text color
void FXReplaceDialog::setReplaceTextColor(FXColor clr){
  replacetext->setBackColor(clr);
  }


// Return replace text color
FXColor FXReplaceDialog::getReplaceTextColor() const {
  return replacetext->getBackColor();
  }


// Append entry
void FXReplaceDialog::appendHistory(const FXString& pat,const FXString& sub,FXuint opt){
  if(!pat.empty()){
    if(searchHistory[0]!=pat){
      for(FXuval i=ARRAYNUMBER(searchHistory)-1; i>0; --i){
        swap(searchHistory[i],searchHistory[i-1]);
        swap(replacHistory[i],replacHistory[i-1]);
        swap(optionHistory[i],optionHistory[i-1]);
        }
      }
    searchHistory[0]=pat;
    replacHistory[0]=sub;
    optionHistory[0]=opt;
    activeHistory=-1;
    }
  }


// Load registy
void FXReplaceDialog::loadHistory(){
  for(FXuval i=0; i<ARRAYNUMBER(searchHistory); ++i){
    searchHistory[i]=getApp()->reg().readStringEntry(sectionName,skey[i],FXString::null);
    if(searchHistory[i].empty()) break;
    replacHistory[i]=getApp()->reg().readStringEntry(sectionName,rkey[i],FXString::null);
    optionHistory[i]=getApp()->reg().readUIntEntry(sectionName,mkey[i],SEARCH_EXACT|SEARCH_FORWARD|SEARCH_WRAP);
    }
  activeHistory=-1;
  }


// Save registry
void FXReplaceDialog::saveHistory(){
  for(FXuval i=0; i<ARRAYNUMBER(searchHistory); ++i){
    if(!searchHistory[i].empty()){
      getApp()->reg().writeStringEntry(sectionName,skey[i],searchHistory[i].text());
      getApp()->reg().writeStringEntry(sectionName,rkey[i],replacHistory[i].text());
      getApp()->reg().writeUIntEntry(sectionName,mkey[i],optionHistory[i]);
      }
    else{
      getApp()->reg().deleteEntry(sectionName,skey[i]);
      getApp()->reg().deleteEntry(sectionName,rkey[i]);
      getApp()->reg().deleteEntry(sectionName,mkey[i]);
      }
    }
  }


// Return with code for search, and close dialog
long FXReplaceDialog::onCmdSearch(FXObject*,FXSelector,void*){
  appendHistory(getSearchText(),getReplaceText(),getSearchMode());
  getApp()->stopModal(this,SEARCH);
  return 1;
  }


// Grey out buttons if no search text
long FXReplaceDialog::onUpdSearch(FXObject* sender,FXSelector,void*){
  sender->handle(this,searchtext->getText().empty()?FXSEL(SEL_COMMAND,ID_DISABLE):FXSEL(SEL_COMMAND,ID_ENABLE),nullptr);
  return 1;
  }


// Return with code for replace, and close dialog
long FXReplaceDialog::onCmdReplace(FXObject*,FXSelector,void*){
  appendHistory(getSearchText(),getReplaceText(),getSearchMode());
  getApp()->stopModal(this,REPLACE);
  return 1;
  }


// Return with code for replace-in-selection, and close dialog
long FXReplaceDialog::onCmdReplaceSel(FXObject*,FXSelector,void*){
  appendHistory(getSearchText(),getReplaceText(),getSearchMode());
  getApp()->stopModal(this,REPLACE_SEL);
  return 1;
  }


// Return with code for replace-all, and close dialog
long FXReplaceDialog::onCmdReplaceAll(FXObject*,FXSelector,void*){
  appendHistory(getSearchText(),getReplaceText(),getSearchMode());
  getApp()->stopModal(this,REPLACE_ALL);
  return 1;
  }


// Change search direction
long FXReplaceDialog::onCmdDir(FXObject*,FXSelector,void*){
  searchmode^=(SEARCH_FORWARD|SEARCH_BACKWARD);
  return 1;
  }


// Update search direction
long FXReplaceDialog::onUpdDir(FXObject* sender,FXSelector,void*){
  sender->handle(this,(searchmode&SEARCH_BACKWARD)?FXSEL(SEL_COMMAND,ID_CHECK):FXSEL(SEL_COMMAND,ID_UNCHECK),nullptr);
  return 1;
  }


// Change wrap mode
long FXReplaceDialog::onCmdWrap(FXObject*,FXSelector,void*){
  searchmode^=SEARCH_WRAP;
  return 1;
  }


// Update wrap mode
long FXReplaceDialog::onUpdWrap(FXObject* sender,FXSelector,void*){
  sender->handle(this,(searchmode&SEARCH_WRAP)?FXSEL(SEL_COMMAND,ID_CHECK):FXSEL(SEL_COMMAND,ID_UNCHECK),nullptr);
  return 1;
  }


// Change case sensitive mode
long FXReplaceDialog::onCmdCase(FXObject*,FXSelector,void*){
  searchmode^=SEARCH_IGNORECASE;
  return 1;
  }


// Update case sensitive mode
long FXReplaceDialog::onUpdCase(FXObject* sender,FXSelector,void*){
  sender->handle(this,(searchmode&SEARCH_IGNORECASE)?FXSEL(SEL_COMMAND,ID_CHECK):FXSEL(SEL_COMMAND,ID_UNCHECK),nullptr);
  return 1;
  }


// Change search mode
long FXReplaceDialog::onCmdRegex(FXObject*,FXSelector,void*){
  searchmode^=SEARCH_REGEX;
  return 1;
  }


// Update search mode
long FXReplaceDialog::onUpdRegex(FXObject* sender,FXSelector,void*){
  sender->handle(this,(searchmode&SEARCH_REGEX)?FXSEL(SEL_COMMAND,ID_CHECK):FXSEL(SEL_COMMAND,ID_UNCHECK),nullptr);
  return 1;
  }


// Scroll back in search history
long FXReplaceDialog::onCmdSearchHistUp(FXObject*,FXSelector,void*){
  if(activeHistory+1<(FXint)ARRAYNUMBER(searchHistory) && !searchHistory[activeHistory+1].empty()){
    activeHistory++;
    FXASSERT(0<=activeHistory && activeHistory<(FXint)ARRAYNUMBER(searchHistory));
    setSearchText(searchHistory[activeHistory]);
    setReplaceText(replacHistory[activeHistory]);
    setSearchMode(optionHistory[activeHistory]);
    }
  else{
    getApp()->beep();
    }
  return 1;
  }


// Scroll forward in search history
long FXReplaceDialog::onCmdSearchHistDn(FXObject*,FXSelector,void*){
  if(0<activeHistory){
    activeHistory--;
    FXASSERT(0<=activeHistory && activeHistory<(FXint)ARRAYNUMBER(searchHistory));
    setSearchText(searchHistory[activeHistory]);
    setReplaceText(replacHistory[activeHistory]);
    setSearchMode(optionHistory[activeHistory]);
    }
  else{
    activeHistory=-1;
    setSearchText(FXString::null);
    setReplaceText(FXString::null);
    setSearchMode(SEARCH_EXACT|SEARCH_FORWARD|SEARCH_WRAP);
    }
  return 1;
  }


// Scroll back in replace history
long FXReplaceDialog::onCmdReplaceHistUp(FXObject*,FXSelector,void*){
  if(activeHistory+1<(FXint)ARRAYNUMBER(searchHistory) && !searchHistory[activeHistory+1].empty()){
    activeHistory++;
    FXASSERT(0<=activeHistory && activeHistory<(FXint)ARRAYNUMBER(searchHistory));
    setReplaceText(replacHistory[activeHistory]);
    }
  else{
    getApp()->beep();
    }
  return 1;
  }


// Scroll back in replace history
long FXReplaceDialog::onCmdReplaceHistDn(FXObject*,FXSelector,void*){
  if(0<activeHistory){
    activeHistory--;
    FXASSERT(0<=activeHistory && activeHistory<(FXint)ARRAYNUMBER(searchHistory));
    setReplaceText(replacHistory[activeHistory]);
    }
  else{
    activeHistory=-1;
    setReplaceText(FXString::null);
    }
  return 1;
  }


// Keyboard press in search text field
long FXReplaceDialog::onSearchKey(FXObject*,FXSelector,void* ptr){
  setSearchTextColor(getApp()->getBackColor());
  setReplaceTextColor(getApp()->getBackColor());
  switch(((FXEvent*)ptr)->code){
    case KEY_Page_Up:
      searchmode=(searchmode&~SEARCH_FORWARD)|SEARCH_BACKWARD;
      return onCmdSearch(this,FXSEL(SEL_COMMAND,ID_SEARCH),nullptr);
    case KEY_Page_Down:
      searchmode=(searchmode&~SEARCH_BACKWARD)|SEARCH_FORWARD;
      return onCmdSearch(this,FXSEL(SEL_COMMAND,ID_SEARCH),nullptr);
    case KEY_Up:
    case KEY_KP_Up:
      return onCmdSearchHistUp(this,FXSEL(SEL_COMMAND,ID_SEARCH_UP),nullptr);
    case KEY_Down:
    case KEY_KP_Down:
      return onCmdSearchHistDn(this,FXSEL(SEL_COMMAND,ID_SEARCH_DN),nullptr);
    case KEY_i:
      if(!(((FXEvent*)ptr)->state&CONTROLMASK)) return 0;
      searchmode^=SEARCH_IGNORECASE;
      return 1;
    case KEY_e:
      if(!(((FXEvent*)ptr)->state&CONTROLMASK)) return 0;
      searchmode^=SEARCH_REGEX;
      return 1;
    case KEY_w:
      if(!(((FXEvent*)ptr)->state&CONTROLMASK)) return 0;
      searchmode^=SEARCH_WRAP;
      return 1;
    case KEY_b:
      if(!(((FXEvent*)ptr)->state&CONTROLMASK)) return 0;
      searchmode^=(SEARCH_FORWARD|SEARCH_BACKWARD);
      return 1;
    }
  return 0;
  }


// Keyboard press in replace text field
long FXReplaceDialog::onReplaceKey(FXObject*,FXSelector,void* ptr){
  setReplaceTextColor(getApp()->getBackColor());
  switch(((FXEvent*)ptr)->code){
    case KEY_Up:
    case KEY_KP_Up:
      return onCmdReplaceHistUp(this,FXSEL(SEL_COMMAND,ID_REPLACE_UP),nullptr);
    case KEY_Down:
    case KEY_KP_Down:
      return onCmdReplaceHistDn(this,FXSEL(SEL_COMMAND,ID_REPLACE_DN),nullptr);
    }
  return 0;
  }


// Wheeled in search text
long FXReplaceDialog::onWheelSearch(FXObject*,FXSelector,void* ptr){
  if(((FXEvent*)ptr)->code>0){
    return onCmdSearchHistUp(this,FXSEL(SEL_COMMAND,ID_SEARCH_UP),nullptr);
    }
  if(((FXEvent*)ptr)->code<0){
    return onCmdSearchHistDn(this,FXSEL(SEL_COMMAND,ID_SEARCH_DN),nullptr);
    }
  return 1;
  }


// Wheeled in replace text
long FXReplaceDialog::onWheelReplace(FXObject*,FXSelector,void* ptr){
  if(((FXEvent*)ptr)->code>0){
    return onCmdReplaceHistUp(this,FXSEL(SEL_COMMAND,ID_REPLACE_UP),nullptr);
    }
  if(((FXEvent*)ptr)->code<0){
    return onCmdReplaceHistDn(this,FXSEL(SEL_COMMAND,ID_REPLACE_DN),nullptr);
    }
  return 1;
  }


// Force the initial text to be seleced
FXuint FXReplaceDialog::execute(FXuint placement){
  create();
  searchtext->setFocus();
  show(placement);
  activeHistory=-1;
  return getApp()->runModalFor(this);
  }


// Save data
void FXReplaceDialog::save(FXStream& store) const {
  FXDialogBox::save(store);
  store << searchlabel;
  store << searchtext;
  store << searchbox;
  store << replacelabel;
  store << replacetext;
  store << replacebox;
  store << search;
  store << replace;
  store << replacesel;
  store << replaceall;
  store << cancel;
  store << searchmode;
  }


// Load data
void FXReplaceDialog::load(FXStream& store){
  FXDialogBox::load(store);
  store >> searchlabel;
  store >> searchtext;
  store >> searchbox;
  store >> replacelabel;
  store >> replacetext;
  store >> replacebox;
  store >> search;
  store >> replace;
  store >> replacesel;
  store >> replaceall;
  store >> cancel;
  store >> searchmode;
  }


// Cleanup
FXReplaceDialog::~FXReplaceDialog(){
  saveHistory();
  searchlabel=(FXLabel*)-1L;
  searchtext=(FXTextField*)-1L;
  searchbox=(FXHorizontalFrame*)-1L;
  replacelabel=(FXLabel*)-1L;
  replacetext=(FXTextField*)-1L;
  replacebox=(FXHorizontalFrame*)-1L;
  search=(FXButton*)-1L;
  replace=(FXButton*)-1L;
  replacesel=(FXButton*)-1L;
  replaceall=(FXButton*)-1L;
  cancel=(FXButton*)-1L;
  }

}
