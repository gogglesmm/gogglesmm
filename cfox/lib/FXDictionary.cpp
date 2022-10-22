/********************************************************************************
*                                                                               *
*                          D i c t i o n a r y    C l a s s                     *
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
#include "FXString.h"
#include "FXDictionary.h"

/*
  Notes:
  - We store the hash key, so that 99.999% of the time we can compare hash numbers;
    only when hash numbers match do we need to compare keys. With a pretty decent hash
    function, the number of calls to strcmp() should be roughly the same as the number
    of successful lookups.

  - When entry is removed, its key and data are cleared, but its hash value remains the
    same; in other words, voided slots have empty key but non-zero hash value, and free slots
    have empty key AND zero hash value.

  - Invariants:

      1 Always at least one table entry marked as "free" i.e. key==empty and hash==0.

      2 Table grows when number of free slots becomes less than or equal to 1+N/4.

      3 Table shrinks when used number of slots becomes less than or equal to N/4.

  - Minimum table size is 1 (one free slot). Thus, for a table to have one element its
    size must be at least  2.

  - Empty table is represented by magic EMPTY table value.  This is compile-time constant
    data and will never change.

  - NULL keys or empty-string keys are not allowed.

  - Similar to FXVariantMap; reimplemented to support plain void* as payload.
*/

#define EMPTY     (const_cast<Entry*>((const Entry*)(__dictionary__empty__+3)))
#define BSHIFT    5

using namespace FX;

/*******************************************************************************/

namespace FX {


// Empty dictionary table value
extern const FXint __string__empty__[];
extern const FXival __dictionary__empty__[];
const FXival __dictionary__empty__[6]={1,0,1,(FXival)(__string__empty__+1),0,0};


// Adjust the size of the table
FXbool FXDictionary::no(FXival n){
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
FXbool FXDictionary::resize(FXival n){
  FXDictionary elbat;
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
          elbat.table[x].data=table[i].data;
          elbat.table[x].hash=(FXuint)h;                // And copy the hash value
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
FXDictionary::FXDictionary():table(EMPTY){
  FXASSERT(sizeof(FXDictionary)==sizeof(void*));
  FXASSERT(sizeof(Entry)<=sizeof(FXival)*3);
  }


// Construct from another dictionary
FXDictionary::FXDictionary(const FXDictionary& other):table(EMPTY){
  FXASSERT(sizeof(FXDictionary)==sizeof(void*));
  FXASSERT(sizeof(Entry)<=sizeof(FXival)*3);
  if(1<other.no()){
    if(__unlikely(!no(other.no()))){ throw FXMemoryException("FXDictionary::FXDictionary: out of memory\n"); }
    copyElms(table,other.table,no());
    free(other.free());
    used(other.used());
    }
  }


// Assignment operator
FXDictionary& FXDictionary::operator=(const FXDictionary& other){
  if(__likely(table!=other.table)){
    if(1<other.no()){
      if(__unlikely(!no(other.no()))){ throw FXMemoryException("FXDictionary::operator=: out of memory\n"); }
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


// Adopt dictionary from another
FXDictionary& FXDictionary::adopt(FXDictionary& other){
  if(__likely(table!=other.table)){
    swap(table,other.table);
    other.clear();
    }
  return *this;
  }


// Find position of given key
FXival FXDictionary::find(const FXchar* ky) const {
  if(__unlikely(!ky || !*ky)){ throw FXRangeException("FXDictionary::find: null or empty key\n"); }
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


// Return reference to slot assocated with given key
void*& FXDictionary::at(const FXchar* ky){
  FXuval p,b,h,x;
  if(__unlikely(!ky || !*ky)){ throw FXRangeException("FXDictionary::at: null or empty key\n"); }
  p=b=h=FXString::hash(ky);
  FXASSERT(h);
  while(table[x=p&(no()-1)].hash){
    if(table[x].hash==h && table[x].key==ky) goto x;   // Return existing slot
    p=(p<<2)+p+b+1;
    b>>=BSHIFT;
    }
  if(__unlikely(free()<=1+(no()>>2)) && __unlikely(!resize(no()<<1))){ throw FXMemoryException("FXDictionary::at: out of memory\n"); }
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
x:return table[x].data;
  }


// Return constant reference to slot assocated with given key
void *const& FXDictionary::at(const FXchar* ky) const {
  if(__unlikely(!ky || !*ky)){ throw FXRangeException("FXDictionary::at: null or empty key\n"); }
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


// Remove association with given key; return old value, if any
void* FXDictionary::remove(const FXchar* ky){
  void* old=nullptr;
  if(__unlikely(!ky || !*ky)){ throw FXRangeException("FXDictionary::remove: null or empty key\n"); }
  if(__likely(!empty())){
    FXuval p,b,h,x;
    p=b=h=FXString::hash(ky);
    FXASSERT(h);
    while(table[x=p&(no()-1)].hash!=h || table[x].key!=ky){
      if(!table[x].hash) goto x;
      p=(p<<2)+p+b+1;
      b>>=BSHIFT;
      }
    table[x].key.clear();                                 // Void the slot (not empty!)
    old=table[x].data;
    table[x].data=nullptr;
    used(used()-1);
    if(__unlikely(used()<=(no()>>2))) resize(no()>>1);
    }
x:return old;
  }


// Erase data at pos in the table; return old value, if any
void* FXDictionary::erase(FXival pos){
  if(__unlikely(pos<0 || no()<=pos)){ throw FXRangeException("FXDictionary::erase: argument out of range\n"); }
  if(!table[pos].key.empty()){
    void* old=table[pos].data;
    table[pos].key.clear();                             // Void the slot (not empty!)
    table[pos].data=nullptr;
    used(used()-1);
    if(__unlikely(used()<=(no()>>2))) resize(no()>>1);
    return old;
    }
  return nullptr;
  }


// Clear entire table
FXbool FXDictionary::clear(){
  return no(1);
  }


// Destroy table
FXDictionary::~FXDictionary(){
  clear();
  }

}
