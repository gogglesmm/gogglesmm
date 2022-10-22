/********************************************************************************
*                                                                               *
*                       C o m b o   B o x   O b j e c t                         *
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
#include "FXStream.h"
#include "FXString.h"
#include "FXSize.h"
#include "FXPoint.h"
#include "FXRectangle.h"
#include "FXObjectList.h"
#include "FXStringDictionary.h"
#include "FXSettings.h"
#include "FXRegistry.h"
#include "FXEvent.h"
#include "FXWindow.h"
#include "FXApp.h"
#include "FXWindow.h"
#include "FXFrame.h"
#include "FXLabel.h"
#include "FXTextField.h"
#include "FXButton.h"
#include "FXMenuButton.h"
#include "FXComposite.h"
#include "FXPacker.h"
#include "FXShell.h"
#include "FXPopup.h"
#include "FXScrollBar.h"
#include "FXScrollArea.h"
#include "FXList.h"
#include "FXComboBox.h"


/*
  Notes:
  - Handling typed text:
    a) Pass string to target only.
    b) Pass string to target & add to list [begin, after/before current, or end].
    c) Pass string to target & replace current item's label.
  - FXComboBox is a text field which may be filled from an FXList.
  - FXComboBox is a text field which in turn may fill an FXList also.
  - In most other respects, it behaves like a FXTextField.
  - Need to catch up/down arrow keys.
  - Combobox turns OFF GUI Updating while being manipulated.
  - Need some way to size the FXList automatically to the number of items.
  - If you leave the list then getCurrentItem() returns the last item under
    cursor which is not the same item shown in FXComboBox.
  - FXComboBox::getItem() and FXComboBox::getItemText() are the same; this
    should probably change.
  - No reaction to up and down arrow while disabled.
*/

#define COMBOBOX_INS_MASK   (COMBOBOX_REPLACE|COMBOBOX_INSERT_BEFORE|COMBOBOX_INSERT_AFTER|COMBOBOX_INSERT_FIRST|COMBOBOX_INSERT_LAST)
#define COMBOBOX_MASK       (COMBOBOX_STATIC|COMBOBOX_INS_MASK)

using namespace FX;

/*******************************************************************************/

namespace FX {

// Map
FXDEFMAP(FXComboBox) FXComboBoxMap[]={
  FXMAPFUNC(SEL_FOCUS_UP,0,FXComboBox::onFocusUp),
  FXMAPFUNC(SEL_FOCUS_DOWN,0,FXComboBox::onFocusDown),
  FXMAPFUNC(SEL_FOCUS_SELF,0,FXComboBox::onFocusSelf),
  FXMAPFUNC(SEL_UPDATE,FXComboBox::ID_TEXT,FXComboBox::onUpdFmText),
  FXMAPFUNC(SEL_CLICKED,FXComboBox::ID_LIST,FXComboBox::onListClicked),
  FXMAPFUNC(SEL_COMMAND,FXComboBox::ID_LIST,FXComboBox::onListCommand),
  FXMAPFUNC(SEL_LEFTBUTTONPRESS,FXComboBox::ID_TEXT,FXComboBox::onTextButton),
  FXMAPFUNC(SEL_MOUSEWHEEL,FXComboBox::ID_TEXT,FXComboBox::onMouseWheel),
  FXMAPFUNC(SEL_CHANGED,FXComboBox::ID_TEXT,FXComboBox::onTextChanged),
  FXMAPFUNC(SEL_COMMAND,FXComboBox::ID_TEXT,FXComboBox::onTextCommand),
  FXMAPFUNC(SEL_COMMAND,FXWindow::ID_SETVALUE,FXComboBox::onFwdToText),
  FXMAPFUNC(SEL_COMMAND,FXWindow::ID_SETINTVALUE,FXComboBox::onFwdToText),
  FXMAPFUNC(SEL_COMMAND,FXWindow::ID_SETREALVALUE,FXComboBox::onFwdToText),
  FXMAPFUNC(SEL_COMMAND,FXWindow::ID_SETSTRINGVALUE,FXComboBox::onFwdToText),
  FXMAPFUNC(SEL_COMMAND,FXWindow::ID_GETINTVALUE,FXComboBox::onFwdToText),
  FXMAPFUNC(SEL_COMMAND,FXWindow::ID_GETREALVALUE,FXComboBox::onFwdToText),
  FXMAPFUNC(SEL_COMMAND,FXWindow::ID_GETSTRINGVALUE,FXComboBox::onFwdToText),
  };


// Object implementation
FXIMPLEMENT(FXComboBox,FXPacker,FXComboBoxMap,ARRAYNUMBER(FXComboBoxMap))


// Combo box
FXComboBox::FXComboBox(FXComposite *p,FXint cols,FXObject* tgt,FXSelector sel,FXuint opts,FXint x,FXint y,FXint w,FXint h,FXint pl,FXint pr,FXint pt,FXint pb):FXPacker(p,opts,x,y,w,h,0,0,0,0,0,0){
  flags|=FLAG_ENABLED;
  target=tgt;
  message=sel;
  field=new FXTextField(this,cols,this,FXComboBox::ID_TEXT,0,0,0,0,0,pl,pr,pt,pb);
  if(options&COMBOBOX_STATIC) field->setEditable(false);
  pane=new FXPopup(this,FRAME_LINE);
  list=new FXList(pane,this,FXComboBox::ID_LIST,LIST_BROWSESELECT|LIST_AUTOSELECT|LAYOUT_FILL_X|LAYOUT_FILL_Y|SCROLLERS_TRACK|HSCROLLER_NEVER);
  if(options&COMBOBOX_STATIC) list->setScrollStyle(SCROLLERS_TRACK|HSCROLLING_OFF);
  button=new FXMenuButton(this,FXString::null,nullptr,pane,FRAME_RAISED|FRAME_THICK|MENUBUTTON_DOWN|MENUBUTTON_ATTACH_RIGHT,0,0,0,0,0,0,0,0);
  button->setXOffset(border);
  button->setYOffset(border);
  flags&=~FLAG_UPDATE;  // Never GUI update
  }


// Create window
void FXComboBox::create(){
  FXPacker::create();
  pane->create();
  }


// Detach window
void FXComboBox::detach(){
  FXPacker::detach();
  pane->detach();
  }


// Destroy window
void FXComboBox::destroy(){
  pane->destroy();
  FXPacker::destroy();
  }


// Enable the window
void FXComboBox::enable(){
  if(!isEnabled()){
    FXPacker::enable();
    field->enable();
    button->enable();
    }
  }


// Disable the window
void FXComboBox::disable(){
  if(isEnabled()){
    FXPacker::disable();
    field->disable();
    button->disable();
    }
  }


// Get default width
FXint FXComboBox::getDefaultWidth(){
  FXint ww=field->getDefaultWidth()+button->getDefaultWidth()+(border<<1);
  FXint pw=pane->getDefaultWidth();
  return FXMAX(ww,pw);
  }


// Get default height
FXint FXComboBox::getDefaultHeight(){
  FXint th=field->getDefaultHeight();
  FXint bh=button->getDefaultHeight();
  return FXMAX(th,bh)+(border<<1);
  }


// Recalculate layout
void FXComboBox::layout(){
  FXint buttonWidth,textWidth,itemHeight;
  itemHeight=height-(border<<1);
  buttonWidth=button->getDefaultWidth();
  textWidth=width-buttonWidth-(border<<1);
  field->position(border,border,textWidth,itemHeight);
  button->position(border+textWidth,border,buttonWidth,itemHeight);
  pane->resize(width,pane->getDefaultHeight());
  flags&=~FLAG_DIRTY;
  }


// Forward GUI update of text field to target; but only if pane is not popped
long FXComboBox::onUpdFmText(FXObject*,FXSelector,void*){
  return target && !isMenuShown() && target->tryHandle(this,FXSEL(SEL_UPDATE,message),nullptr);
  }


// Command handled in the text field
long FXComboBox::onFwdToText(FXObject* sender,FXSelector sel,void* ptr){
  return field->handle(sender,sel,ptr);
  }


// Clicked inside or outside an item in the list; unpost the pane
long FXComboBox::onListClicked(FXObject*,FXSelector,void*){
  return button->handle(this,FXSEL(SEL_COMMAND,ID_UNPOST),nullptr);
  }


// Clicked on an item in the list; issue a callback
long FXComboBox::onListCommand(FXObject*,FXSelector,void* ptr){
  field->setText(list->getItemText((FXint)(FXival)ptr));
  if(!(options&COMBOBOX_STATIC)) field->selectAll();
  return target && target->tryHandle(this,FXSEL(SEL_COMMAND,message),(void*)getText().text());
  }


// Pressed left button in text field
long FXComboBox::onTextButton(FXObject*,FXSelector,void*){
  if(options&COMBOBOX_STATIC){
    button->showMenu(true);
    return 1;
    }
  return 0;
  }


// Text has changed
long FXComboBox::onTextChanged(FXObject*,FXSelector,void* ptr){
  return target && target->tryHandle(this,FXSEL(SEL_CHANGED,message),ptr);
  }


// Text has changed
long FXComboBox::onTextCommand(FXObject*,FXSelector,void* ptr){
  FXint index=list->getCurrentItem();
  if(!(options&COMBOBOX_STATIC)){
    switch(options&COMBOBOX_INS_MASK){
      case COMBOBOX_REPLACE:
        if(0<=index) setItem(index,(FXchar*)ptr,getItemData(index));
        break;
      case COMBOBOX_INSERT_BEFORE:
        if(0<=index) insertItem(index,(FXchar*)ptr);
        break;
      case COMBOBOX_INSERT_AFTER:
        if(0<=index) insertItem(index+1,(FXchar*)ptr);
        break;
      case COMBOBOX_INSERT_FIRST:
        insertItem(0,(FXchar*)ptr);
        break;
      case COMBOBOX_INSERT_LAST:
        appendItem((FXchar*)ptr);
        break;
      }
    }
  return target && target->tryHandle(this,FXSEL(SEL_COMMAND,message),ptr);
  }


// Bounce focus to the text field
long FXComboBox::onFocusSelf(FXObject* sender,FXSelector,void* ptr){
  return field->handle(sender,FXSEL(SEL_FOCUS_SELF,0),ptr);
  }


// Select upper item
long FXComboBox::onFocusUp(FXObject*,FXSelector,void*){
  if(isEnabled()){
    FXint index=getCurrentItem();
    if(index<0) index=getNumItems()-1;
    else if(0<index) index--;
    if(0<=index && index<getNumItems()){
      setCurrentItem(index,true);
      }
    return 1;
    }
  return 0;
  }


// Select lower item
long FXComboBox::onFocusDown(FXObject*,FXSelector,void*){
  if(isEnabled()){
    FXint index=getCurrentItem();
    if(index<0) index=0;
    else if(index<getNumItems()-1) index++;
    if(0<=index && index<getNumItems()){
      setCurrentItem(index,true);
      }
    return 1;
    }
  return 0;
  }


// Mouse wheel
long FXComboBox::onMouseWheel(FXObject*,FXSelector,void* ptr){
  FXEvent* event=(FXEvent*)ptr;
  if(isEnabled()){
    FXint index=getCurrentItem();
    if(event->code<0){
      if(index<0) index=0;
      else if(index<getNumItems()-1) index++;
      }
    else if(event->code>0){
      if(index<0) index=getNumItems()-1;
      else if(0<index) index--;
      }
    if(0<=index && index<getNumItems()){
      setCurrentItem(index,true);
      }
    return 1;
    }
  return 0;
  }


// Return true if editable
FXbool FXComboBox::isEditable() const {
  return field->isEditable();
  }


// Set widget is editable or not
void FXComboBox::setEditable(FXbool edit){
  field->setEditable(edit);
  }


// Set text
void FXComboBox::setText(const FXString& text,FXbool notify){
  if(field->getText()!=text){
    field->setText(text);
    if(notify && target){target->tryHandle(this,FXSEL(SEL_COMMAND,message),(void*)getText().text());}
    }
  }


// Obtain text
FXString FXComboBox::getText() const {
  return field->getText();
  }


// Set number of text columns
void FXComboBox::setNumColumns(FXint cols){
  field->setNumColumns(cols);
  }


// Get number of text columns
FXint FXComboBox::getNumColumns() const {
  return field->getNumColumns();
  }


// Get number of items
FXint FXComboBox::getNumItems() const {
  return list->getNumItems();
  }


// Get number of visible items
FXint FXComboBox::getNumVisible() const {
  return list->getNumVisible();
  }


// Set number of visible items
void FXComboBox::setNumVisible(FXint nvis){
  list->setNumVisible(nvis);
  }


// Is item current
FXbool FXComboBox::isItemCurrent(FXint index) const {
  return list->isItemCurrent(index);
  }


// Change current item
void FXComboBox::setCurrentItem(FXint index,FXbool notify){
  FXint current=list->getCurrentItem();
  if(current!=index){
    list->setCurrentItem(index);
    list->makeItemVisible(index);
    if(0<=index){
      setText(list->getItemText(index),notify);
      }
    else{
      setText(FXString::null,notify);
      }
    }
  }


// Get current item
FXint FXComboBox::getCurrentItem() const {
  return list->getCurrentItem();
  }


// Retrieve item
FXString FXComboBox::getItem(FXint index) const {
  return list->getItem(index)->getText();
  }


// Replace text of item at index
FXint FXComboBox::setItem(FXint index,const FXString& text,FXptr ptr,FXbool notify){
  if(index<0 || list->getNumItems()<=index){ fxerror("%s::setItem: index out of range.\n",getClassName()); }
  list->setItem(index,text,nullptr,ptr);
  recalc();
  if(isItemCurrent(index)){
    setText(text,notify);
    }
  return index;
  }


// Fill list by appending items from array of strings
FXint FXComboBox::fillItems(const FXchar *const *strings,FXbool notify){
  FXint numberofitems=list->getNumItems();
  FXint n=list->fillItems(strings);
  recalc();
  if(numberofitems<=list->getCurrentItem()){
    setText(list->getItemText(list->getCurrentItem()),notify);
    }
  return n;
  }


// Fill list by appending items from array of strings
FXint FXComboBox::fillItems(const FXString* strings,FXbool notify){
  FXint numberofitems=list->getNumItems();
  FXint n=list->fillItems(strings);
  recalc();
  if(numberofitems<=list->getCurrentItem()){
    setText(list->getItemText(list->getCurrentItem()),notify);
    }
  return n;
  }


// Fill list by appending items from newline separated strings
FXint FXComboBox::fillItems(const FXString& strings,FXbool notify){
  FXint numberofitems=list->getNumItems();
  FXint n=list->fillItems(strings);
  recalc();
  if(numberofitems<=list->getCurrentItem()){
    setText(list->getItemText(list->getCurrentItem()),notify);
    }
  return n;
  }


// Insert item at index
FXint FXComboBox::insertItem(FXint index,const FXString& text,FXptr ptr,FXbool notify){
  if(index<0 || list->getNumItems()<index){ fxerror("%s::insertItem: index out of range.\n",getClassName()); }
  list->insertItem(index,text,nullptr,ptr);
  recalc();
  if(isItemCurrent(index)){
    setText(text,notify);
    }
  return index;
  }


// Append item
FXint FXComboBox::appendItem(const FXString& text,FXptr ptr,FXbool notify){
  FXint index=list->appendItem(text,nullptr,ptr);
  recalc();
  if(isItemCurrent(index)){
    setText(text,notify);
    }
  return index;
  }


// Prepend item
FXint FXComboBox::prependItem(const FXString& text,void* ptr,FXbool notify){
  FXint index=list->prependItem(text,nullptr,ptr);
  recalc();
  if(isItemCurrent(index)){
    setText(text,notify);
    }
  return index;
  }


// Move item from oldindex to newindex
FXint FXComboBox::moveItem(FXint newindex,FXint oldindex,FXbool notify){
  if(newindex<0 || list->getNumItems()<=newindex || oldindex<0 || list->getNumItems()<=oldindex){ fxerror("%s::moveItem: index out of range.\n",getClassName()); }
  FXint current=list->getCurrentItem();
  list->moveItem(newindex,oldindex);
  recalc();
  if(current!=list->getCurrentItem()){
    current=list->getCurrentItem();
    if(0<=current){
      setText(list->getItemText(current),notify);
      }
    else{
      setText(FXString::null,notify);
      }
    }
  return newindex;
  }


// Remove given item
void FXComboBox::removeItem(FXint index,FXbool notify){
  FXint current=list->getCurrentItem();
  list->removeItem(index);
  recalc();
  if(index==current){
    current=list->getCurrentItem();
    if(0<=current){
      setText(list->getItemText(current),notify);
      }
    else{
      setText(FXString::null,notify);
      }
    }
  }


// Remove all items
void FXComboBox::clearItems(FXbool notify){
  list->clearItems();
  recalc();
  setText(FXString::null,notify);
  }


// Get item by name
FXint FXComboBox::findItem(const FXString& string,FXint start,FXuint flgs) const {
  return list->findItem(string,start,flgs);
  }


// Get item by data
FXint FXComboBox::findItemByData(FXptr ptr,FXint start,FXuint flgs) const {
  return list->findItemByData(ptr,start,flgs);
  }


// Set item text
void FXComboBox::setItemText(FXint index,const FXString& txt){
  list->setItemText(index,txt);
  recalc();
  if(isItemCurrent(index)){
    setText(txt);
    }
  }


// Get item text
FXString FXComboBox::getItemText(FXint index) const {
  return list->getItemText(index);
  }


// Set item data
void FXComboBox::setItemData(FXint index,FXptr ptr) const {
  list->setItemData(index,ptr);
  }


// Get item data
FXptr FXComboBox::getItemData(FXint index) const {
  return list->getItemData(index);
  }


// Return true if item is enabled
FXbool FXComboBox::isItemEnabled(FXint index) const {
  return list->isItemEnabled(index);
  }


// Enable item
FXbool FXComboBox::enableItem(FXint index){
  return list->enableItem(index);
  }


// Disable item
FXbool FXComboBox::disableItem(FXint index){
  return list->disableItem(index);
  }


// Show menu
void FXComboBox::showMenu(FXbool shw){
  button->showMenu(shw);
  }


// Is the pane shown
FXbool FXComboBox::isMenuShown() const {
  return button->isMenuShown();
  }


// Set font
void FXComboBox::setFont(FXFont* fnt){
  if(!fnt){ fxerror("%s::setFont: NULL font specified.\n",getClassName()); }
  field->setFont(fnt);
  list->setFont(fnt);
  recalc();
  }


// Obtain font
FXFont* FXComboBox::getFont() const {
  return field->getFont();
  }


// Change combobox style
void FXComboBox::setComboStyle(FXuint mode){
  FXuint opts=(options&~COMBOBOX_MASK)|(mode&COMBOBOX_MASK);
  if(opts!=options){
    options=opts;
    if(options&COMBOBOX_STATIC){
      field->setEditable(false);                                // Non-editable
      list->setScrollStyle(SCROLLERS_TRACK|HSCROLLING_OFF);     // No scrolling
      }
    else{
      field->setEditable(true);                                 // Editable
      list->setScrollStyle(SCROLLERS_TRACK|HSCROLLER_NEVER);    // Scrollable, but no scrollbar
      }
    recalc();
    }
  }


// Change popup pane shrinkwrap mode
void FXComboBox::setShrinkWrap(FXbool flag){
  pane->setShrinkWrap(flag);
  }


// Return popup pane shrinkwrap mode
FXbool FXComboBox::getShrinkWrap() const {
  return pane->getShrinkWrap();
  }


// Get combobox style
FXuint FXComboBox::getComboStyle() const {
  return (options&COMBOBOX_MASK);
  }


// Set text justify style
void FXComboBox::setJustify(FXuint style){
  field->setJustify(style);
  }


// Get text justify style
FXuint FXComboBox::getJustify() const {
  return field->getJustify();
  }


// Set window background color
void FXComboBox::setBackColor(FXColor clr){
  field->setBackColor(clr);
  list->setBackColor(clr);
  }


// Get background color
FXColor FXComboBox::getBackColor() const {
  return field->getBackColor();
  }


// Set text color
void FXComboBox::setTextColor(FXColor clr){
  field->setTextColor(clr);
  list->setTextColor(clr);
  }


// Return text color
FXColor FXComboBox::getTextColor() const {
  return field->getTextColor();
  }


// Set select background color
void FXComboBox::setSelBackColor(FXColor clr){
  field->setSelBackColor(clr);
  list->setSelBackColor(clr);
  }


// Return selected background color
FXColor FXComboBox::getSelBackColor() const {
  return field->getSelBackColor();
  }


// Set selected text color
void FXComboBox::setSelTextColor(FXColor clr){
  field->setSelTextColor(clr);
  list->setSelTextColor(clr);
  }


// Return selected text color
FXColor FXComboBox::getSelTextColor() const {
  return field->getSelTextColor();
  }


// Sort items using current sort function
void FXComboBox::sortItems(){
  list->sortItems();
  }


// Return sort function
FXListSortFunc FXComboBox::getSortFunc() const {
  return list->getSortFunc();
  }


// Change sort function
void FXComboBox::setSortFunc(FXListSortFunc func){
  list->setSortFunc(func);
  }


// Set help text
void FXComboBox::setHelpText(const FXString& txt){
  field->setHelpText(txt);
  }


// Get help text
const FXString& FXComboBox::getHelpText() const {
  return field->getHelpText();
  }


// Set tip text
void FXComboBox::setTipText(const FXString& txt){
  field->setTipText(txt);
  }


// Get tip text
const FXString& FXComboBox::getTipText() const {
  return field->getTipText();
  }


// Save object to stream
void FXComboBox::save(FXStream& store) const {
  FXPacker::save(store);
  store << field;
  store << button;
  store << list;
  store << pane;
  }


// Load object from stream
void FXComboBox::load(FXStream& store){
  FXPacker::load(store);
  store >> field;
  store >> button;
  store >> list;
  store >> pane;
  }


// Delete it
FXComboBox::~FXComboBox(){
  delete pane;
  pane=(FXPopup*)-1L;
  field=(FXTextField*)-1L;
  button=(FXMenuButton*)-1L;
  list=(FXList*)-1L;
  }

}
