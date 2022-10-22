/********************************************************************************
*                                                                               *
*                          S e t t i n g s   C l a s s                          *
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
#include "fxascii.h"
#include "FXArray.h"
#include "FXHash.h"
#include "FXStream.h"
#include "FXElement.h"
#include "FXException.h"
#include "FXString.h"
#include "FXColors.h"
#include "FXFile.h"
#include "FXStringDictionary.h"
#include "FXSettings.h"

/*
  Notes:

  - Format for settings database file:

    [SectionKey]
    EntryKey=string-with-no-spaces
    EntryKey="string\nwith a\nnewline in it\n"
    EntryKey=" string with leading and trailing spaces and \"embedded\" in it  "
    EntryKey=string with no leading or trailing spaces

  - EntryKey may is of the form "ali baba", "ali-baba", "ali_baba", or "ali.baba".

  - Leading/trailing spaces are NOT part of the EntryKey.

  - Escape sequences now allow octal (\377) as well as hex (\xff) codes.

  - EntryKey format should be like values.

  - Extensive error checking in unparseFile() to ensure no settings data is
    lost when disk is full.
*/


#define EMPTY     (const_cast<Entry*>((const Entry*)(__settings__empty__+3)))
#define BSHIFT    5

using namespace FX;

/*******************************************************************************/

namespace FX {


// Empty string dictionary table value
extern const FXint __string__empty__[];
extern const FXival __stringdictionary__empty__[];
extern const FXival __settings__empty__[];
const FXival __settings__empty__[6]={1,0,1,(FXival)(__string__empty__+1),(FXival)(__stringdictionary__empty__+3),0};


// Adjust the size of the table
FXbool FXSettings::no(FXival n){
  FXival m=no();
  if(__likely(m!=n)){
    Entry* elbat;
    void*  p;

    // Release old table
    if(1<m){
      destructElms(table,m);
      ::free(((FXival*)table)-3);
      table=EMPTY;
      }

    // Allocate new table
    if(1<n){
      if(__unlikely((p=::calloc(sizeof(FXival)*3+sizeof(Entry)*n,1))==nullptr)) return false;
      elbat=(Entry*)(((FXival*)p)+3);
      ((FXival*)elbat)[-3]=n;
      ((FXival*)elbat)[-2]=0;
      ((FXival*)elbat)[-1]=n;
      constructElms(elbat,n);
      table=elbat;
      }
    }
  return true;
  }


// Resize the table to the given size; the size must be a power of two
FXbool FXSettings::resize(FXival n){
  FXSettings elbat;
  FXASSERT((n&(n-1))==0);       // Power of 2
  FXASSERT((n-used())>0);       // At least one free slot
  if(elbat.no(n)){
    if(1<elbat.no() && 1<no()){
      FXuval p,b,h,x; FXival i;
      for(i=0; i<no(); ++i){                  // Hash existing entries into new table
        if(!table[i].key.empty()){
          p=b=h=table[i].hash;
          while(elbat.table[x=p&(n-1)].hash){ // Locate slot
            p=(p<<2)+p+b+1;
            b>>=BSHIFT;
            }
          elbat.table[x].key.adopt(table[i].key);       // Steal the string buffers
          elbat.table[x].data.adopt(table[i].data);
          elbat.table[x].hash=(FXuint)h;                // And copy the hash value
          }
        }
      elbat.free(n-used());     // All non-empty slots now free
      elbat.used(used());       // Used slots not changed
      elbat.setModified(isModified());
      }
    adopt(elbat);
    return true;
    }
  return false;
  }


// Construct settings database
FXSettings::FXSettings():table(EMPTY),modified(false){
  FXASSERT(sizeof(Entry)<=sizeof(FXival)*3);
  }


// Construct from another settings database
FXSettings::FXSettings(const FXSettings& other):table(EMPTY),modified(other.modified){
  FXASSERT(sizeof(Entry)<=sizeof(FXival)*3);
  if(1<other.no()){
    if(__unlikely(!no(other.no()))){ throw FXMemoryException("FXSettings::FXSettings: out of memory\n"); }
    copyElms(table,other.table,no());
    free(other.free());
    used(other.used());
    }
  }


// Assignment operator
FXSettings& FXSettings::operator=(const FXSettings& other){
  if(__likely(table!=other.table)){
    if(1<other.no()){
      if(__unlikely(!no(other.no()))){ throw FXMemoryException("FXSettings::operator=: out of memory\n"); }
      copyElms(table,other.table,no());
      free(other.free());
      used(other.used());
      }
    else{
      no(1);
      }
    modified=true;
    }
  return *this;
  }


// Adopt settings database from another
FXSettings& FXSettings::adopt(FXSettings& other){
  if(__likely(table!=other.table)){
    swap(table,other.table);
    other.clear();
    other.modified=true;
    modified=true;
    }
  return *this;
  }


// Find position of given key
FXival FXSettings::find(const FXchar* ky) const {
  if(__unlikely(!ky || !*ky)){ throw FXRangeException("FXSettings::find: null or empty key\n"); }
  if(__likely(!empty())){
    FXuval p,b,x,h;
    p=b=h=FXString::hash(ky);
    FXASSERT(h);
    while(table[x=p&(no()-1)].hash){
      if(table[x].hash==h && table[x].key==ky) return x;
      p=(p<<2)+p+b+1;
      b>>=BSHIFT;
      }
    }
  return -1;
  }


// Return reference to string dictionary assocated with key
FXStringDictionary& FXSettings::at(const FXchar* ky){
  FXuval p,b,h,x;
  if(__unlikely(!ky || !*ky)){ throw FXRangeException("FXSettings::at: null or empty key\n"); }
  p=b=h=FXString::hash(ky);
  FXASSERT(h);
  while(table[x=p&(no()-1)].hash){
    if(table[x].hash==h && table[x].key==ky) goto x;   // Return existing slot
    p=(p<<2)+p+b+1;
    b>>=BSHIFT;
    }
  if(__unlikely(free()<=1+(no()>>2)) && __unlikely(!resize(no()<<1))){ throw FXMemoryException("FXSettings::at: out of memory\n"); }
  p=b=h;
  while(table[x=p&(no()-1)].hash){
    if(table[x].key.empty()) goto y;                    // Return voided slot
    p=(p<<2)+p+b+1;
    b>>=BSHIFT;
    }
  free(free()-1);                                       // Put into empty slot
y:used(used()+1);
  table[x].key=ky;
  table[x].hash=(FXuint)h;
x:modified=true;                                        // Assume its to be written
  return table[x].data;
  }


// Return constant reference to string dictionary assocated with key
const FXStringDictionary& FXSettings::at(const FXchar* ky) const {
  if(__unlikely(!ky || !*ky)){ throw FXRangeException("FXSettings::at: null or empty key\n"); }
  if(__likely(!empty())){
    FXuval p,b,x,h;
    p=b=h=FXString::hash(ky);
    FXASSERT(h);
    while(table[x=p&(no()-1)].hash){
      if(table[x].hash==h && table[x].key==ky) return table[x].data;
      p=(p<<2)+p+b+1;
      b>>=BSHIFT;
      }
    }
  return EMPTY[0].data;
  }


// Parse filename
FXbool FXSettings::parseFile(const FXString& filename,FXbool mrk){
  FXTRACE((100,"FXSettings::parseFile(%s,%d)\n",filename.text(),mrk));
  FXFile file(filename,FXFile::Reading);
  if(file.isOpen()){
    FXString string;
    string.length(file.size());
    if(file.readBlock(string.text(),string.length())==string.length()){
      return parse(string,mrk);
      }
    }
  return false;
  }


// Unparse registry file
FXbool FXSettings::unparseFile(const FXString& filename){
  FXTRACE((100,"FXSettings::unparseFile(%s)\n",filename.text()));
  FXFile file(filename,FXFile::Writing);
  if(file.isOpen()){
    FXString string;
    if(unparse(string)){
      return file.writeBlock(string.text(),string.length())==string.length();
      }
    }
  return false;
  }


// Parse single string to populate settings
FXbool FXSettings::parse(const FXString& str,FXbool mrk){
  FXint lineno=1,p=0,b,e;
  FXString section,name,value;

  // Skip over byte-order mark
  if(str[p]=='\xef' && str[p+1]=='\xbb' && str[p+2]=='\xbf') p+=3;

  // Parse one line at a time
  while(str[p]){

    // Skip leading blanks
    while(Ascii::isBlank(str[p])) ++p;

    // Parse section name
    if(str[p]=='['){

      b=++p;

      // Scan over section name
      while(str[p] && str[p]!=']' && str[p]!='\r' && str[p]!='\n'){
        if(Ascii::isControl(str[p])){ fxwarning("%d: control character in section name.\n",lineno); goto nxt; }
        ++p;
        }

      // Check errors
      if(str[p]!=']'){ fxwarning("%d: expected ']' to close section name.\n",lineno); goto nxt; }

      e=p++;

      // Grab name
      section=str.mid(b,e-b);
      }

    // Parse name-value pair
    else if(str[p]!='#' && str[p]!=';' && str[p]!='\r' && str[p]!='\n'){

      // Should have seen section prior to this
      if(section.empty()){ fxwarning("%d: entry should follow a section.\n",lineno); goto nxt; }

      b=p;

      // Scan key name
      while(str[p] && str[p]!='=' && str[p]!='\r' && str[p]!='\n'){
        if(Ascii::isControl(str[p])){ fxwarning("%d: control character in entry name.\n",lineno); goto nxt; }
        ++p;
        }

      // Check errors
      if(str[p]!='='){ fxwarning("%d: expected '=' to follow entry name.\n",lineno); goto nxt; }

      e=p++;

      // Remove trailing spaces after name
      while(b<e && Ascii::isBlank(str[e-1])) e--;

      // Grab name
      name=str.mid(b,e-b);

      // Skip leading spaces
      while(Ascii::isBlank(str[p])) p++;

      // Mark value
      b=p;

      // Scan value
      while(str[p] && str[p]!='\n' && str[p]!='\r'){
        if(Ascii::isControl(str[p])){ fxwarning("%d: control character in entry value.\n",lineno); goto nxt; }
        ++p;
        }

      e=p;

      // Remove trailing spaces after value
      while(b<e && Ascii::isBlank(str[e-1])) e--;

      // Grab the unescaped value
      value=FXString::unescape(str.mid(b,e-b),'"','"');

      // Add entry to current section
      at(section).at(name,mrk)=value;
      }

    // Skip to end of line
nxt:while(str[p]){
      if(str[p++]=='\n'){ lineno++; break; }
      }
    }
  return true;
  }


// Unparse settings to a single string
FXbool FXSettings::unparse(FXString& str) const {
  FXival sec,ent,flg;

  // Clear output
  str.clear();

  // Any keys at all?
  if(!empty()){

    // Loop over all sections
    for(sec=0; sec<no(); ++sec){

      // Get group, if any
      if(!empty(sec)){

        // Loop over all entries
        for(ent=flg=0; ent<data(sec).no(); ++ent){

          // Is key-value pair marked?
          if(!data(sec).empty(ent) && data(sec).mark(ent)){

            // Write section name if not written yet
            if(!flg){
              str.append("[");
              str.append(key(sec));      // FIXME should escape group
              str.append("]" ENDLINE);
              flg=1;
              }

            // Write marked key-value pairs only
            str.append(data(sec).key(ent));
            str.append("=");
            if(FXString::shouldEscape(data(sec).data(ent),'"','"')){
              str.append(FXString::escape(data(sec).data(ent),'"','"'));
              }
            else{
              str.append(data(sec).data(ent));
              }
            str.append(ENDLINE);
            }
          }

        // Blank line after end
        if(flg){
          str.append(ENDLINE);
          }
        }
      }
    }
  return true;
  }


// Read a formatted registry entry
FXint FXSettings::readFormatEntry(const FXchar* section,const FXchar* name,const FXchar* fmt,...) const {
  const FXString& value=at(section).at(name);
  FXint result=0;
  if(!value.empty()){
    va_list args;
    va_start(args,fmt);
    result=value.vscan(fmt,args);
    va_end(args);
    }
  return result;
  }


// Read a formatted registry entry
FXint FXSettings::readFormatEntry(const FXString& section,const FXchar* name,const FXchar* fmt,...) const {
  const FXString& value=at(section).at(name);
  FXint result=0;
  if(!value.empty()){
    va_list args;
    va_start(args,fmt);
    result=value.vscan(fmt,args);
    va_end(args);
    }
  return result;
  }


// Read a formatted registry entry
FXint FXSettings::readFormatEntry(const FXString& section,const FXString& name,const FXchar* fmt,...) const {
  const FXString& value=at(section).at(name);
  FXint result=0;
  if(!value.empty()){
    va_list args;
    va_start(args,fmt);
    result=value.vscan(fmt,args);
    va_end(args);
    }
  return result;
  }


// Write a formatted registry entry
FXint FXSettings::writeFormatEntry(const FXchar* section,const FXchar* name,const FXchar* fmt,...){
  FXint result;
  va_list args;
  va_start(args,fmt);
  result=at(section).at(name,true).vformat(fmt,args);
  va_end(args);
  return result;
  }


// Write a formatted registry entry
FXint FXSettings::writeFormatEntry(const FXString& section,const FXchar* name,const FXchar* fmt,...){
  FXint result;
  va_list args;
  va_start(args,fmt);
  result=at(section).at(name,true).vformat(fmt,args);
  va_end(args);
  return result;
  }


// Write a formatted registry entry
FXint FXSettings::writeFormatEntry(const FXString& section,const FXString& name,const FXchar* fmt,...){
  FXint result;
  va_list args;
  va_start(args,fmt);
  result=at(section).at(name,true).vformat(fmt,args);
  va_end(args);
  return result;
  }


// Read a string-valued registry entry
const FXchar* FXSettings::readStringEntry(const FXchar* section,const FXchar* name,const FXchar* def) const {
  const FXStringDictionary& dict=at(section);
  FXival slot=dict.find(name);
  if(0<=slot){
    return dict.data(slot).text();      // Use value at slot even if value was empty string!
    }
  return def;
  }


// Read a string-valued registry entry
const FXchar* FXSettings::readStringEntry(const FXString& section,const FXchar* name,const FXchar* def) const {
  return readStringEntry(section.text(),name,def);
  }


// Read a string-valued registry entry
const FXchar* FXSettings::readStringEntry(const FXString& section,const FXString& name,const FXchar* def) const {
  return readStringEntry(section.text(),name.text(),def);
  }


// Write a string-valued registry entry
FXbool FXSettings::writeStringEntry(const FXchar* section,const FXchar* name,const FXchar* val){
  at(section).at(name,true)=val;
  return true;
  }


// Write a string-valued registry entry
FXbool FXSettings::writeStringEntry(const FXString& section,const FXchar* name,const FXchar* val){
  return writeStringEntry(section.text(),name,val);
  }


// Write a string-valued registry entry
FXbool FXSettings::writeStringEntry(const FXString& section,const FXString& name,const FXchar* val){
  return writeStringEntry(section.text(),name.text(),val);
  }


// Read a int-valued registry entry
FXint FXSettings::readIntEntry(const FXchar* section,const FXchar* name,FXint def) const {
  const FXString& value=at(section).at(name);
  if(!value.empty()){
    FXint result;
    FXbool ok;
    result=value.toInt(10,&ok);
    if(ok) return result;
    }
  return def;
  }


// Read a int-valued registry entry
FXint FXSettings::readIntEntry(const FXString& section,const FXchar* name,FXint def) const {
  return readIntEntry(section.text(),name,def);
  }


// Read a int-valued registry entry
FXint FXSettings::readIntEntry(const FXString& section,const FXString& name,FXint def) const {
  return readIntEntry(section.text(),name.text(),def);
  }


// Write a int-valued registry entry
FXbool FXSettings::writeIntEntry(const FXchar* section,const FXchar* name,FXint val){
  at(section).at(name,true).fromInt(val);
  return true;
  }


// Write a int-valued registry entry
FXbool FXSettings::writeIntEntry(const FXString& section,const FXchar* name,FXint val){
  return writeIntEntry(section.text(),name,val);
  }


// Write a int-valued registry entry
FXbool FXSettings::writeIntEntry(const FXString& section,const FXString& name,FXint val){
  return writeIntEntry(section.text(),name.text(),val);
  }


// Read a unsigned int-valued registry entry
FXuint FXSettings::readUIntEntry(const FXchar* section,const FXchar* name,FXuint def) const {
  const FXString& value=at(section).at(name);
  if(!value.empty()){
    FXuint result;
    FXbool ok;
    result=value.toUInt(10,&ok);
    if(ok) return result;
    }
  return def;
  }


// Read a unsigned int-valued registry entry
FXuint FXSettings::readUIntEntry(const FXString& section,const FXchar* name,FXuint def) const {
  return readUIntEntry(section.text(),name,def);
  }


// Read a unsigned int-valued registry entry
FXuint FXSettings::readUIntEntry(const FXString& section,const FXString& name,FXuint def) const {
  return readUIntEntry(section.text(),name.text(),def);
  }


// Write a unsigned int-valued registry entry
FXbool FXSettings::writeUIntEntry(const FXchar* section,const FXchar* name,FXuint val){
  at(section).at(name,true).fromUInt(val);
  return true;
  }

// Write a unsigned int-valued registry entry
FXbool FXSettings::writeUIntEntry(const FXString& section,const FXchar* name,FXuint val){
  return writeUIntEntry(section.text(),name,val);
  }


// Write a unsigned int-valued registry entry
FXbool FXSettings::writeUIntEntry(const FXString& section,const FXString& name,FXuint val){
  return writeUIntEntry(section.text(),name.text(),val);
  }


// Read a 64-bit long integer registry entry
FXlong FXSettings::readLongEntry(const FXchar* section,const FXchar* name,FXlong def) const {
  const FXString& value=at(section).at(name);
  if(!value.empty()){
    FXlong result;
    FXbool ok;
    result=value.toLong(10,&ok);
    if(ok) return result;
    }
  return def;
  }


// Read a 64-bit long integer registry entry
FXlong FXSettings::readLongEntry(const FXString& section,const FXchar* name,FXlong def) const {
  return readLongEntry(section.text(),name,def);
  }


// Read a 64-bit long integer registry entry
FXlong FXSettings::readLongEntry(const FXString& section,const FXString& name,FXlong def) const {
  return readLongEntry(section.text(),name.text(),def);
  }


// Write a 64-bit long integer registry entry
FXbool FXSettings::writeLongEntry(const FXchar* section,const FXchar* name,FXlong val){
  at(section).at(name,true).fromLong(val);
  return true;
  }


// Write a 64-bit long integer registry entry
FXbool FXSettings::writeLongEntry(const FXString& section,const FXchar* name,FXlong val){
  return writeLongEntry(section.text(),name,val);
  }


// Write a 64-bit long integer registry entry
FXbool FXSettings::writeLongEntry(const FXString& section,const FXString& name,FXlong val){
  return writeLongEntry(section.text(),name.text(),val);
  }


// Read a 64-bit unsigned long integer registry entry
FXulong FXSettings::readULongEntry(const FXchar* section,const FXchar* name,FXulong def) const {
  const FXString& value=at(section).at(name);
  if(!value.empty()){
    FXulong result;
    FXbool ok;
    result=value.toULong(10,&ok);
    if(ok) return result;
    }
  return def;
  }


// Read a 64-bit unsigned long integer registry entry
FXulong FXSettings::readULongEntry(const FXString& section,const FXchar* name,FXulong def) const {
  return readULongEntry(section.text(),name,def);
  }


// Read a 64-bit unsigned long integer registry entry
FXulong FXSettings::readULongEntry(const FXString& section,const FXString& name,FXulong def) const {
  return readULongEntry(section.text(),name.text(),def);
  }


// Write a 64-bit long integer registry entry
FXbool FXSettings::writeULongEntry(const FXchar* section,const FXchar* name,FXulong val){
  at(section).at(name,true).fromULong(val);
  return true;
  }


// Write a 64-bit long integer registry entry
FXbool FXSettings::writeULongEntry(const FXString& section,const FXchar* name,FXulong val){
  return writeULongEntry(section.text(),name,val);
  }


// Write a 64-bit long integer registry entry
FXbool FXSettings::writeULongEntry(const FXString& section,const FXString& name,FXulong val){
  return writeULongEntry(section.text(),name.text(),val);
  }


// Read a double-valued registry entry
FXdouble FXSettings::readRealEntry(const FXchar* section,const FXchar* name,FXdouble def) const {
  const FXString& value=at(section).at(name);
  if(!value.empty()){
    FXdouble result;
    FXbool ok;
    result=value.toDouble(&ok);
    if(ok) return result;
    }
  return def;
  }

// Read a double-valued registry entry
FXdouble FXSettings::readRealEntry(const FXString& section,const FXchar* name,FXdouble def) const {
  return readRealEntry(section.text(),name,def);
  }


// Read a double-valued registry entry
FXdouble FXSettings::readRealEntry(const FXString& section,const FXString& name,FXdouble def) const {
  return readRealEntry(section.text(),name.text(),def);
  }


// Write a double-valued registry entry
FXbool FXSettings::writeRealEntry(const FXchar* section,const FXchar* name,FXdouble val){
  at(section).at(name,true).fromDouble(val,16,2);
  return true;
  }


// Write a double-valued registry entry
FXbool FXSettings::writeRealEntry(const FXString& section,const FXchar* name,FXdouble val){
  return writeRealEntry(section.text(),name,val);
  }


// Write a double-valued registry entry
FXbool FXSettings::writeRealEntry(const FXString& section,const FXString& name,FXdouble val){
  return writeRealEntry(section.text(),name.text(),val);
  }


// Read a color registry entry
FXColor FXSettings::readColorEntry(const FXchar* section,const FXchar* name,FXColor def) const {
  const FXString& value=at(section).at(name);
  if(!value.empty()){
    return colorFromName(value);
    }
  return def;
  }


// Read a color registry entry
FXColor FXSettings::readColorEntry(const FXString& section,const FXchar* name,FXColor def) const {
  return readColorEntry(section.text(),name,def);
  }


// Read a color registry entry
FXColor FXSettings::readColorEntry(const FXString& section,const FXString& name,FXColor def) const {
  return readColorEntry(section.text(),name.text(),def);
  }


// Write a color registry entry
FXbool FXSettings::writeColorEntry(const FXchar* section,const FXchar* name,FXColor val){
  at(section).at(name,true)=nameFromColor(val);
  return true;
  }


// Write a color registry entry
FXbool FXSettings::writeColorEntry(const FXString& section,const FXchar* name,FXColor val){
  return writeColorEntry(section.text(),name,val);
  }


// Write a color registry entry
FXbool FXSettings::writeColorEntry(const FXString& section,const FXString& name,FXColor val){
  return writeColorEntry(section.text(),name.text(),val);
  }


// Read a boolean registry entry
FXbool FXSettings::readBoolEntry(const FXchar* section,const FXchar* name,FXbool def) const {
  const FXString& value=at(section).at(name);
  if(!value.empty()){
    if(FXString::comparecase(value,"true")==0) return true;
    if(FXString::comparecase(value,"false")==0) return false;
    if(FXString::comparecase(value,"yes")==0) return true;
    if(FXString::comparecase(value,"no")==0) return false;
    if(FXString::comparecase(value,"on")==0) return true;
    if(FXString::comparecase(value,"off")==0) return false;
    if(FXString::comparecase(value,"1")==0) return true;
    if(FXString::comparecase(value,"0")==0) return false;
    }
  return def;
  }


// Read a boolean registry entry
FXbool FXSettings::readBoolEntry(const FXString& section,const FXchar* name,FXbool def) const {
  return readBoolEntry(section.text(),name,def);
  }


// Read a boolean registry entry
FXbool FXSettings::readBoolEntry(const FXString& section,const FXString& name,FXbool def) const {
  return readBoolEntry(section.text(),name.text(),def);
  }


// Write a boolean registry entry
FXbool FXSettings::writeBoolEntry(const FXchar* section,const FXchar* name,FXbool val){
  at(section).at(name,true)=val?"true":"false";
  return true;
  }


// Write a boolean registry entry
FXbool FXSettings::writeBoolEntry(const FXString& section,const FXchar* name,FXbool val){
  return writeBoolEntry(section.text(),name,val);
  }


// Write a boolean registry entry
FXbool FXSettings::writeBoolEntry(const FXString& section,const FXString& name,FXbool val){
  return writeBoolEntry(section.text(),name.text(),val);
  }


// See if entry exists
FXbool FXSettings::existingEntry(const FXchar* section,const FXchar* name) const {
  return 0<=at(section).find(name);
  }


// See if entry exists
FXbool FXSettings::existingEntry(const FXString& section,const FXchar* name) const {
  return existingEntry(section.text(),name);
  }


// See if entry exists
FXbool FXSettings::existingEntry(const FXString& section,const FXString& name) const {
  return existingEntry(section.text(),name.text());
  }


// See if section exists
FXbool FXSettings::existingSection(const FXchar* section) const {
  return 0<=find(section);
  }


// See if section exists
FXbool FXSettings::existingSection(const FXString& section) const {
  return existingSection(section.text());
  }


// Delete a registry entry
void FXSettings::deleteEntry(const FXchar* section,const FXchar* name){
  at(section).remove(name);
  }


// Delete a registry entry
void FXSettings::deleteEntry(const FXString& section,const FXchar* name){
  deleteEntry(section.text(),name);
  }


// Delete a registry entry
void FXSettings::deleteEntry(const FXString& section,const FXString& name){
  deleteEntry(section.text(),name.text());
  }


// Delete section
void FXSettings::deleteSection(const FXchar* section){
  if(__unlikely(!section || !*section)){ throw FXRangeException("FXSettings::deleteSection: null or empty key\n"); }
  if(__likely(!empty())){
    FXuval p,b,h,x;
    p=b=h=FXString::hash(section);
    FXASSERT(h);
    while(table[x=p&(no()-1)].hash!=h || table[x].key!=section){
      if(!table[x].hash) return;
      p=(p<<2)+p+b+1;
      b>>=BSHIFT;
      }
    table[x].key.clear();                                 // Void the slot (not empty!)
    table[x].data.clear();
    used(used()-1);
    modified=true;
    if(__unlikely(used()<=(no()>>2))) resize(no()>>1);
    }
  }


// Delete section
void FXSettings::deleteSection(const FXString& section){
  deleteSection(section.text());
  }


// Clear all sections
void FXSettings::clear(){
  modified=true;
  no(1);
  }


// Clean up
FXSettings::~FXSettings(){
  clear();
  }

}
