/********************************************************************************
*                                                                               *
*                  S t r i n g   D i c t i o n a r y    C l a s s               *
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
#include "FXElement.h"
#include "FXException.h"
#include "FXHash.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXStringDictionary.h"

/*
  Notes:
  - FXStringDictionary is a hash table from FXString to FXString.
  - Changed from earlier implementation which was derived from FXDictionary.
  - Reason is we need all values to be initialized to special empty-string value;
    this includes the unmapped slots! (in particular, the special empty-table value
    as well!).
  - While we may be able to make it kind of work this way is better and at any rate,
    its not a very large class, but one which has high use.
*/

#define EMPTY     (const_cast<Entry*>((const Entry*)(__stringdictionary__empty__+3)))
#define BSHIFT    5

using namespace FX;

/*******************************************************************************/

namespace FX {


// Empty string dictionary table value
extern const FXint __string__empty__[];
extern const FXival __stringdictionary__empty__[];
const FXival __stringdictionary__empty__[7]={1,0,1,(FXival)(__string__empty__+1),(FXival)(__string__empty__+1),0,0};


// Adjust the size of the table
FXbool FXStringDictionary::no(FXival n){
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
FXbool FXStringDictionary::resize(FXival n){
  FXStringDictionary elbat;
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
          elbat.table[x].mark=table[i].mark;            // Copy mark
          }
        }
      elbat.free(n-used());     // All non-empty slots now free
      elbat.used(used());       // Used slots not changed
      }
    adopt(elbat);
    return true;
    }
  return false;
  }


// Construct empty dictionary
FXStringDictionary::FXStringDictionary():table(EMPTY){
  FXASSERT(sizeof(FXStringDictionary)==sizeof(FXptr));
  FXASSERT(sizeof(Entry)<=sizeof(FXival)*4);
  }


// Construct from another string dictionary
FXStringDictionary::FXStringDictionary(const FXStringDictionary& other):table(EMPTY){
  FXASSERT(sizeof(FXStringDictionary)==sizeof(FXptr));
  FXASSERT(sizeof(Entry)<=sizeof(FXival)*4);
  if(1<other.no()){
    if(__unlikely(!no(other.no()))){ throw FXMemoryException("FXStringDictionary::FXStringDictionary: out of memory\n"); }
    copyElms(table,other.table,no());
    free(other.free());
    used(other.used());
    }
  }


// Assignment operator
FXStringDictionary& FXStringDictionary::operator=(const FXStringDictionary& other){
  if(__likely(table!=other.table)){
    if(1<other.no()){
      if(__unlikely(!no(other.no()))){ throw FXMemoryException("FXStringDictionary::operator=: out of memory\n"); }
      copyElms(table,other.table,no());
      free(other.free());
      used(other.used());
      }
    else{
      no(1);
      }
    }
  return *this;
  }


// Adopt string dictionary from another
FXStringDictionary& FXStringDictionary::adopt(FXStringDictionary& other){
  if(__likely(table!=other.table)){
    swap(table,other.table);
    other.clear();
    }
  return *this;
  }


// Find position of given key
FXival FXStringDictionary::find(const FXchar* ky) const {
  if(__unlikely(!ky || !*ky)){ throw FXRangeException("FXStringDictionary::find: null or empty key\n"); }
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


// Return reference to string assocated with key
FXString& FXStringDictionary::at(const FXchar* ky,FXbool mrk){
  FXuval p,b,h,x;
  if(__unlikely(!ky || !*ky)){ throw FXRangeException("FXStringDictionary::at: null or empty key\n"); }
  p=b=h=FXString::hash(ky);
  FXASSERT(h);
  while(table[x=p&(no()-1)].hash){
    if(table[x].hash==h && table[x].key==ky) goto x;   // Return existing slot
    p=(p<<2)+p+b+1;
    b>>=BSHIFT;
    }
  if(__unlikely(free()<=1+(no()>>2)) && __unlikely(!resize(no()<<1))){ throw FXMemoryException("FXStringDictionary::at: out of memory\n"); }
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
x:table[x].mark=mrk;
  return table[x].data;
  }


// Return constant reference to string assocated with key
const FXString& FXStringDictionary::at(const FXchar* ky) const {
  if(__unlikely(!ky || !*ky)){ throw FXRangeException("FXStringDictionary::at: null or empty key\n"); }
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


// Remove string associated with given key
FXbool FXStringDictionary::remove(const FXchar* ky){
  if(__unlikely(!ky || !*ky)){ throw FXRangeException("FXStringDictionary::remove: null or empty key\n"); }
  if(__likely(!empty())){
    FXuval p,b,h,x;
    p=b=h=FXString::hash(ky);
    FXASSERT(h);
    while(table[x=p&(no()-1)].hash!=h || table[x].key!=ky){
      if(!table[x].hash) return false;
      p=(p<<2)+p+b+1;
      b>>=BSHIFT;
      }
    table[x].key.clear();                                 // Void the slot (not empty!)
    table[x].data.clear();
    used(used()-1);
    if(__unlikely(used()<=(no()>>2))) resize(no()>>1);
    return true;
    }
  return false;
  }


// Erase string at pos in the table
FXbool FXStringDictionary::erase(FXival pos){
  if(__unlikely(pos<0 || no()<=pos)){ throw FXRangeException("FXStringDictionary::erase: argument out of range\n"); }
  if(!table[pos].key.empty()){
    table[pos].key.clear();                             // Void the slot (not empty!)
    table[pos].data.clear();
    used(used()-1);
    if(__unlikely(used()<=(no()>>2))) resize(no()>>1);
    return true;
    }
  return false;
  }


// Clear entire table
FXbool FXStringDictionary::clear(){
  return no(1);
  }


// Destroy table
FXStringDictionary::~FXStringDictionary(){
  clear();
  }

}
