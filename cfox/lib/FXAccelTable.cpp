/********************************************************************************
*                                                                               *
*                   A c c e l e r a t o r   T a b l e   C l a s s               *
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
#include "fxchar.h"
#include "fxkeys.h"
#include "fxmath.h"
#include "fxascii.h"
#include "fxunicode.h"
#include "FXArray.h"
#include "FXHash.h"
#include "FXMutex.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXElement.h"
#include "FXSize.h"
#include "FXPoint.h"
#include "FXRectangle.h"
#include "FXObject.h"
#include "FXStringDictionary.h"
#include "FXSettings.h"
#include "FXRegistry.h"
#include "FXEvent.h"
#include "FXWindow.h"
#include "FXApp.h"
#include "FXAccelTable.h"
#include "fxpriv.h"

/*
  Notes:
  - Routines parseAccel(), unparseAccel(), and parseHotKey() now static members of
    FXAccelTable.

  - Need to call parseAccel() is much reduced since addition of convenience functions
    that take a string as input; these new routines overload addAccel(), removeAccel(),
    hasAccel() and targetOfAccel().

  - Thus, often no need to call parseAccel() directly in many cases, and much less
    cumbersome stuff to learn.

  - We need to deal with X11 unicode keysyms (with 0x01000000 flag) in some way.
*/

#define TOPIC_CONSTRUCT  1000
#define TOPIC_KEYBOARD   1009
#define TOPIC_ACCEL      1011

#define EMPTYSLOT       0xfffffffe   // Previously used, now empty slot
#define UNUSEDSLOT      0xffffffff   // Unsused slot marker


using namespace FX;

/*******************************************************************************/

namespace FX {


// Map
FXDEFMAP(FXAccelTable) FXAccelTableMap[]={
  FXMAPFUNC(SEL_KEYPRESS,0,FXAccelTable::onKeyPress),
  FXMAPFUNC(SEL_KEYRELEASE,0,FXAccelTable::onKeyRelease),
  };


// Object implementation
FXIMPLEMENT(FXAccelTable,FXObject,FXAccelTableMap,ARRAYNUMBER(FXAccelTableMap))


// Make empty accelerator table
FXAccelTable::FXAccelTable(){
  FXTRACE((TOPIC_CONSTRUCT,"%p->FXAccelTable::FXAccelTable\n",this));
  allocElms(key,1);
  key[0].code=UNUSEDSLOT;
  key[0].target=nullptr;
  key[0].messagedn=0;
  key[0].messageup=0;
  max=0;
  num=0;
  }


// Resize hash table, and rehash old stuff into it
void FXAccelTable::resize(FXuint m){
  FXuint p,i,c;
  FXAccelKey *newkey;
  allocElms(newkey,m+1);
  for(i=0; i<=m; i++){
    newkey[i].code=UNUSEDSLOT;
    newkey[i].target=nullptr;
    newkey[i].messagedn=0;
    newkey[i].messageup=0;
    }
  for(i=0; i<=max; i++){
    if((c=key[i].code)>=EMPTYSLOT) continue;
    p=(c*13)&m;
    while(newkey[p].code!=UNUSEDSLOT) p=(p+1)&m;
    newkey[p]=key[i];
    }
  freeElms(key);
  key=newkey;
  max=m;
  }

/*******************************************************************************/

// Add accelerator key combination into the accelerator table
void FXAccelTable::addAccel(FXHotKey hotkey,FXObject* target,FXSelector seldn,FXSelector selup){
  if(hotkey){
    FXTRACE((TOPIC_ACCEL,"%p->FXAccelTable::addAccel: code=%04x state=%04x\n",this,(FXushort)hotkey,(FXushort)(hotkey>>16)));
    FXuint p=(hotkey*13)&max;
    FXuint c;
    FXASSERT(hotkey!=UNUSEDSLOT);
    FXASSERT(hotkey!=EMPTYSLOT);
    while((c=key[p].code)!=UNUSEDSLOT){ // Check if in table already
      if(c==hotkey) goto x;
      p=(p+1)&max;
      }
    ++num;
    if(max<(num<<1)) resize((max<<1)+1);
    FXASSERT(num<=max);
    p=(hotkey*13)&max;                  // Locate first unused or empty slot
    while(key[p].code<EMPTYSLOT){
      p=(p+1)&max;
      }
x:  key[p].code=hotkey;                 // Add or replace accelerator info
    key[p].target=target;
    key[p].messagedn=seldn;
    key[p].messageup=selup;
    }
  }


// Parse key-combination description and add it into the accelerator table
void FXAccelTable::addAccel(const FXString& string,FXObject* target,FXSelector seldn,FXSelector selup){
  addAccel(parseAccel(string),target,seldn,selup);
  }


// Parse key-combination description and add it into the accelerator table
void FXAccelTable::addAccel(const FXchar* string,FXObject* target,FXSelector seldn,FXSelector selup){
  addAccel(parseAccel(string),target,seldn,selup);
  }

/*******************************************************************************/

// Remove accelerator key combination from the accelerator table
void FXAccelTable::removeAccel(FXHotKey hotkey){
  if(hotkey){
    FXTRACE((TOPIC_ACCEL,"%p->FXAccelTable::removeAccel: code=%04x state=%04x\n",this,(FXushort)hotkey,(FXushort)(hotkey>>16)));
    FXuint p=(hotkey*13)&max;
    FXuint c;
    FXASSERT(hotkey!=UNUSEDSLOT);
    FXASSERT(hotkey!=EMPTYSLOT);
    while((c=key[p].code)!=hotkey){
      if(c==UNUSEDSLOT) return;
      p=(p+1)&max;
      }
    if(key[(p+1)&max].code==UNUSEDSLOT){// Last in chain
      key[p].code=UNUSEDSLOT;
      }
    else{                               // Middle of chain
      key[p].code=EMPTYSLOT;
      }
    key[p].target=nullptr;
    key[p].messagedn=0;
    key[p].messageup=0;
    if(max>=(num<<2)) resize(max>>1);
    --num;
    FXASSERT(num<=max);
    }
  }


// Parse key-combination description and remove it from the accelerator table
void FXAccelTable::removeAccel(const FXString& string){
  removeAccel(parseAccel(string));
  }


// Parse key-combination description and remove it from the accelerator table
void FXAccelTable::removeAccel(const FXchar* string){
  removeAccel(parseAccel(string));
  }

/*******************************************************************************/

// Return true if accelerator accelerator key-combination is in accelerator table
FXbool FXAccelTable::hasAccel(FXHotKey hotkey) const {
  if(hotkey){
    FXuint p=(hotkey*13)&max;
    FXuint c;
    FXASSERT(hotkey!=UNUSEDSLOT);
    FXASSERT(hotkey!=EMPTYSLOT);
    while((c=key[p].code)!=hotkey){
      if(c==UNUSEDSLOT) return false;
      p=(p+1)&max;
      }
    return true;
    }
  return false;
  }


// Parse key-combination description and return true if it is in the accelerator table
FXbool FXAccelTable::hasAccel(const FXString& string) const {
  return hasAccel(parseAccel(string));
  }


// Parse key-combination description and return true if it is in the accelerator table
FXbool FXAccelTable::hasAccel(const FXchar* string) const {
  return hasAccel(parseAccel(string));
  }

/*******************************************************************************/

// Return target object of the given accelerator key-combination
FXObject* FXAccelTable::targetOfAccel(FXHotKey hotkey) const {
  if(hotkey){
    FXuint p=(hotkey*13)&max;
    FXuint c;
    FXASSERT(hotkey!=UNUSEDSLOT);
    FXASSERT(hotkey!=EMPTYSLOT);
    while((c=key[p].code)!=hotkey){
      if(c==UNUSEDSLOT) return nullptr;
      p=(p+1)&max;
      }
    return key[p].target;
    }
  return nullptr;
  }


// Parse key-combination description and return its target
FXObject* FXAccelTable::targetOfAccel(const FXString& string) const {
  return targetOfAccel(parseAccel(string));
  }


// Parse key-combination description and return its target
FXObject* FXAccelTable::targetOfAccel(const FXchar* string) const {
  return targetOfAccel(parseAccel(string));
  }

/*******************************************************************************/

// Keyboard press; forward to accelerator target
long FXAccelTable::onKeyPress(FXObject* sender,FXSelector,void* ptr){
  FXTRACE((TOPIC_KEYBOARD,"%p->FXAccelTable::onKeyPress keysym=0x%04x state=%04x\n",this,((FXEvent*)ptr)->code,((FXEvent*)ptr)->state));
  FXEvent* event=(FXEvent*)ptr;
  FXuint code=MKUINT(event->code,event->state&(SHIFTMASK|CONTROLMASK|ALTMASK));
  FXuint p=(code*13)&max;
  FXuint c;
  FXASSERT(code!=UNUSEDSLOT);
  FXASSERT(code!=EMPTYSLOT);
  while((c=key[p].code)!=code){
    if(c==UNUSEDSLOT) return 0;
    p=(p+1)&max;
    }
  if(key[p].target && key[p].messagedn){
    key[p].target->tryHandle(sender,key[p].messagedn,ptr);
    }
  return 1;
  }


// Keyboard release; forward to accelerator target
long FXAccelTable::onKeyRelease(FXObject* sender,FXSelector,void* ptr){
  FXTRACE((TOPIC_KEYBOARD,"%p->FXAccelTable::onKeyRelease keysym=0x%04x state=%04x\n",this,((FXEvent*)ptr)->code,((FXEvent*)ptr)->state));
  FXEvent* event=(FXEvent*)ptr;
  FXuint code=MKUINT(event->code,event->state&(SHIFTMASK|CONTROLMASK|ALTMASK));
  FXuint p=(code*13)&max;
  FXuint c;
  FXASSERT(code!=UNUSEDSLOT);
  FXASSERT(code!=EMPTYSLOT);
  while((c=key[p].code)!=code){
    if(c==UNUSEDSLOT) return 0;
    p=(p+1)&max;
    }
  if(key[p].target && key[p].messageup){
    key[p].target->tryHandle(sender,key[p].messageup,ptr);
    }
  return 1;
  }

/*******************************************************************************/

// Parse accelerator from string, yielding modifier and key code
FXHotKey FXAccelTable::parseAccel(const FXString& string){
  return FXAccelTable::parseAccel(string.text());
  }


// Parse accelerator from string, yielding modifier and key code
FXHotKey FXAccelTable::parseAccel(const FXchar* string){
  const FXchar* ptr=string;
  FXuint code=0;
  FXuint mods=0;

  // Parse leading space
  while(*ptr && Ascii::isSpace(*ptr)) ptr++;

  // Parse modifiers
  while(*ptr){

    // Modifier
    if(FXString::comparecase(ptr,"ctl",3)==0){ mods|=CONTROLMASK; ptr+=3; }
    else if(FXString::comparecase(ptr,"ctrl",4)==0){ mods|=CONTROLMASK; ptr+=4; }
    else if(FXString::comparecase(ptr,"alt",3)==0){ mods|=ALTMASK; ptr+=3; }
    else if(FXString::comparecase(ptr,"meta",4)==0){ mods|=METAMASK; ptr+=4; }
    else if(FXString::comparecase(ptr,"shift",5)==0){ mods|=SHIFTMASK; ptr+=5; }
    else break;

    // Separator
    if(*ptr=='+' || *ptr=='-' || Ascii::isSpace(*ptr)) ptr++;
    }

  // Test for some special keys
  if(FXString::comparecase(ptr,"home",4)==0){
    code=KEY_Home;
    }
  else if(FXString::comparecase(ptr,"end",3)==0){
    code=KEY_End;
    }
  else if(FXString::comparecase(ptr,"pgup",4)==0){
    code=KEY_Page_Up;
    }
  else if(FXString::comparecase(ptr,"pgdn",4)==0){
    code=KEY_Page_Down;
    }
  else if(FXString::comparecase(ptr,"left",4)==0){
    code=KEY_Left;
    }
  else if(FXString::comparecase(ptr,"right",5)==0){
    code=KEY_Right;
    }
  else if(FXString::comparecase(ptr,"up",2)==0){
    code=KEY_Up;
    }
  else if(FXString::comparecase(ptr,"down",4)==0){
    code=KEY_Down;
    }
  else if(FXString::comparecase(ptr,"ins",3)==0){
    code=KEY_Insert;
    }
  else if(FXString::comparecase(ptr,"del",3)==0){
    code=KEY_Delete;
    }
  else if(FXString::comparecase(ptr,"delete",6)==0){
    code=KEY_Delete;
    }
  else if(FXString::comparecase(ptr,"esc",3)==0){
    code=KEY_Escape;
    }
  else if(FXString::comparecase(ptr,"tab",3)==0){
    code=KEY_Tab;
    }
  else if(FXString::comparecase(ptr,"return",6)==0){
    code=KEY_Return;
    }
  else if(FXString::comparecase(ptr,"enter",5)==0){
    code=KEY_Return;
    }
  else if(FXString::comparecase(ptr,"back",4)==0){
    code=KEY_BackSpace;
    }
  else if(FXString::comparecase(ptr,"backspace",9)==0){
    code=KEY_BackSpace;
    }
  else if(FXString::comparecase(ptr,"spc",3)==0){
    code=KEY_space;
    }
  else if(FXString::comparecase(ptr,"space",5)==0){
    code=KEY_space;
    }

  // Test for function keys
  else if(Ascii::toLower(*ptr)=='f' && Ascii::isDigit(*(ptr+1))){
    if(Ascii::isDigit(*(ptr+2))){
      code=KEY_F1+10*(*(ptr+1)-'0')+(*(ptr+2)-'0')-1;
      }
    else{
      code=KEY_F1+*(ptr+1)-'0'-1;
      }
    }

  // Test if hexadecimal code designator
  else if(*ptr=='#' && Ascii::isHexDigit(*(ptr+1))){
    code=Ascii::digitValue(*(ptr+1));
    ptr+=2;
    while(Ascii::isHexDigit(*ptr)){
      code=(code<<4)+Ascii::digitValue(*ptr);
      ptr++;
      }
    }

  // Test if its a single character accelerator
  else if(Ascii::isPrint(*ptr)){
    if(mods&SHIFTMASK)
      code=Ascii::toUpper(*ptr)+KEY_space-' ';
    else
      code=Ascii::toLower(*ptr)+KEY_space-' ';
    }

  FXTRACE((TOPIC_ACCEL,"parseAccel(%s) = code=%04x mods=%04x\n",string,code,mods));
  return MKUINT(code,mods);
  }

/*******************************************************************************/

// Unparse hot key back into a string
FXString FXAccelTable::unparseAccel(FXHotKey key){
  FXuint mods=(key&0xffff0000)>>16;
  FXuint code=(key&0xffff);
  FXString s;

  // Handle modifier keys
  if(mods&CONTROLMASK) s+="Ctrl+";
  if(mods&ALTMASK) s+="Alt+";
  if(mods&SHIFTMASK) s+="Shift+";
  if(mods&METAMASK) s+="Meta+";

  // Handle some special keys
  switch(code){
    case KEY_Home:
      s+="Home";
      break;
    case KEY_End:
      s+="End";
      break;
    case KEY_Page_Up:
      s+="PgUp";
      break;
    case KEY_Page_Down:
      s+="PgDn";
      break;
    case KEY_Left:
      s+="Left";
      break;
    case KEY_Right:
      s+="Right";
      break;
    case KEY_Up:
      s+="Up";
      break;
    case KEY_Down:
      s+="Down";
      break;
    case KEY_Insert:
      s+="Ins";
      break;
    case KEY_Delete:
      s+="Del";
      break;
    case KEY_Escape:
      s+="Esc";
      break;
    case KEY_Tab:
      s+="Tab";
      break;
    case KEY_Return:
      s+="Return";
      break;
    case KEY_BackSpace:
      s+="Back";
      break;
    case KEY_space:
      s+="Space";
      break;
    case KEY_F1:
    case KEY_F2:
    case KEY_F3:
    case KEY_F4:
    case KEY_F5:
    case KEY_F6:
    case KEY_F7:
    case KEY_F8:
    case KEY_F9:
    case KEY_F10:
    case KEY_F11:
    case KEY_F12:
    case KEY_F13:
    case KEY_F14:
    case KEY_F15:
    case KEY_F16:
    case KEY_F17:
    case KEY_F18:
    case KEY_F19:
    case KEY_F20:
    case KEY_F21:
    case KEY_F22:
    case KEY_F23:
    case KEY_F24:
    case KEY_F25:
    case KEY_F26:
    case KEY_F27:
    case KEY_F28:
    case KEY_F29:
    case KEY_F30:
    case KEY_F31:
    case KEY_F32:
    case KEY_F33:
    case KEY_F34:
    case KEY_F35:
      s+=FXString::value("F%d",code-KEY_F1+1);
      break;
    default:
      if(Ascii::isPrint(code)){
        if(mods&SHIFTMASK)
          s+=Ascii::toUpper(code);
        else
          s+=Ascii::toLower(code);
        }
      else{
        s+=FXString::value("#%04X",code);
        }
      break;
    }
  return s;
  }

/*******************************************************************************/

// Parse hot key from string
FXHotKey parseHotKey(const FXString& string){
  const FXchar* ptr=string.text();
  FXuint code=0;
  FXuint mods=0;
  while(*ptr){
    if(*ptr=='&'){
      if(*(ptr+1)!='&'){
        FXwchar w=wc(ptr+1);
        if(Unicode::isAlphaNumeric(w)){
          mods=ALTMASK;
          code=fxucs2keysym(Unicode::toLower(w));
          }
        break;
        }
      ptr++;
      }
    ptr++;
    }
  FXTRACE((TOPIC_ACCEL,"parseHotKey(%s) = code=%04x mods=%04x\n",string.text(),code,mods));
  return MKUINT(code,mods);
  }


// Obtain hot key offset in string
FXint findHotKey(const FXString& string){
  FXint pos=0;
  FXint n=0;
  while(pos<string.length()){
    if(string[pos]=='&'){
      if(string[pos+1]!='&'){
        return n;
        }
      pos++;
      }
    pos++;
    n++;
    }
  return -1;
  }


// Strip hot key from string
FXString stripHotKey(const FXString& string){
  FXString result=string;
  FXint len=result.length();
  FXint i,j;
  for(i=j=0; j<len; j++){
    if(result[j]=='&'){
      if(result[j+1]!='&') continue;
      j++;
      }
    result[i++]=result[j];
    }
  result.trunc(i);
  return result;
  }

/*******************************************************************************/

// Save data
void FXAccelTable::save(FXStream& store) const {
  FXObject::save(store);
  store << max;
  store << num;
  for(FXuint i=0; i<=max; i++){
    store << key[i].code;
    store << key[i].target;
    store << key[i].messagedn;
    store << key[i].messageup;
    }
  }


// Load data
void FXAccelTable::load(FXStream& store){
  FXObject::load(store);
  store >> max;
  store >> num;
  resizeElms(key,max+1);
  for(FXuint i=0; i<=max; i++){
    store >> key[i].code;
    store >> key[i].target;
    store >> key[i].messagedn;
    store >> key[i].messageup;
    }
  }


// Destroy table
FXAccelTable::~FXAccelTable(){
  FXTRACE((TOPIC_CONSTRUCT,"%p->FXAccelTable::~FXAccelTable\n",this));
  freeElms(key);
  key=(FXAccelKey*)-1L;
  }


}
