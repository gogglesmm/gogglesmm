/********************************************************************************
*                                                                               *
*                 R e v e r s e   D i c t i o n a r y    C l a s s              *
*                                                                               *
*********************************************************************************
* Copyright (C) 2018,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "FXReverseDictionary.h"

/*
  Notes:
  - Reverse dictionary maps pointers to strings.
*/


#define EMPTY     (const_cast<Entry*>((const Entry*)(__reversedictionary__empty__+3)))
#define HASH(x)   ((FXival)(x)^(((FXival)(x))>>13))
#define VOID      ((const void*)-1L)
#define LEGAL(p)  ((p)!=nullptr && (p)!=VOID)
#define BSHIFT    5

using namespace FX;

/*******************************************************************************/

namespace FX {

// Empty dictionary table value
extern const FXint __string__empty__[];
extern const FXival __reversedictionary__empty__[];
const FXival __reversedictionary__empty__[5]={1,0,1,0,(FXival)(__string__empty__+1)};


// Resize buffer for the dictionary.
FXbool FXReverseDictionary::no(FXival n){
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


// Resize dictionary; new size must be a power of two.
// A new table is allocated and current entries are rehashed into the new
// table; then the tables are switched and the old table is deleted.
FXbool FXReverseDictionary::resize(FXival n){
  FXReverseDictionary elbat;
  FXASSERT((n&(n-1))==0);
  FXASSERT((n-used())>0);
  if(elbat.no(n)){
    if(1<elbat.no() && 1<no()){
      const void* ky;
      FXuval p,b,x;
      FXival i;
      for(i=0; i<no(); ++i){
        ky=table[i].key;
        if(LEGAL(ky)){
          p=b=HASH(ky);
          while(elbat.table[x=p&(n-1)].key){
            p=(p<<2)+p+b+1;
            b>>=BSHIFT;
            }
          elbat.table[x].key=ky;
          elbat.table[x].data.adopt(table[i].data);
          }
        }
      elbat.free(n-used());
      elbat.used(used());
      }
    adopt(elbat);
    return true;
    }
  return false;
  }


// Initialize dictionary to empty.
FXReverseDictionary::FXReverseDictionary():table(EMPTY){
  FXASSERT(sizeof(FXReverseDictionary)==sizeof(void*));
  FXASSERT(sizeof(Entry)<=sizeof(FXival)*2);
  }


// Copy constructor initializes this dictionary with the data from another.
FXReverseDictionary::FXReverseDictionary(const FXReverseDictionary& other):table(EMPTY){
  FXASSERT(sizeof(FXReverseDictionary)==sizeof(void*));
  FXASSERT(sizeof(Entry)<=sizeof(FXival)*2);
  if(1<other.no()){
    if(!no(other.no())){ throw FXMemoryException("FXReverseDictionary::FXReverseDictionary: out of memory\n"); }
    copyElms(table,other.table,no());
    free(other.free());
    used(other.used());
    }
  }


// Assign this dictionary with the data from another.
FXReverseDictionary& FXReverseDictionary::operator=(const FXReverseDictionary& other){
  if(table!=other.table){
    if(1<other.no()){
      if(!no(other.no())){ throw FXMemoryException("FXReverseDictionary::operator=: out of memory\n"); }
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


// Adopt data from other dictionary; this means the other dictionary now becomes
// empty, and any data held by this dictionary is lost.
FXReverseDictionary& FXReverseDictionary::adopt(FXReverseDictionary& other){
  if(table!=other.table){
    swap(table,other.table);
    other.clear();
    }
  return *this;
  }


// Locate slot associated with key, if key is non-NULL.
// If not found, or key is NULL, return -1.
FXival FXReverseDictionary::find(const void* ky) const {
  if(LEGAL(ky)){
    FXuval p,b,x;
    p=b=HASH(ky);
    while(table[x=p&(no()-1)].key){
      if(table[x].key==ky) return x;
      p=(p<<2)+p+b+1;
      b>>=BSHIFT;
      }
    }
  return -1;
  }


// Access element at given key, if key is non-NULL.
// If found, return reference to the data element associated with the key.
// If not found, optionally resize the array if needed, and then return a
// reference to (possibly newly created) data element.
// Write access to data associated with a NULL key will generate exception.
FXString& FXReverseDictionary::at(const void* ky){
  if(LEGAL(ky)){
    FXuval p,b,h,x;
    p=b=h=HASH(ky);
    while(table[x=p&(no()-1)].key){
      if(table[x].key==ky) goto x;
      p=(p<<2)+p+b+1;
      b>>=BSHIFT;
      }
    if(free()<=1+(no()>>2) && !resize(no()<<1)){ throw FXMemoryException("FXReverseDictionary::at: out of memory\n"); }
    p=b=h;
    while(table[x=p&(no()-1)].key){
      if(table[x].key==VOID) goto y;
      p=(p<<2)+p+b+1;
      b>>=BSHIFT;
      }
    free(free()-1);
y:  used(used()+1);
    table[x].key=ky;
x:  return table[x].data;
    }
  return *((FXString*)nullptr);              // Can NOT be referenced; will generate segfault!
  }


// Access element at given key, if key is non-NULL.
// If found, return const-reference to the data element; if not found,
// or if the key is NULL, return const-reference to the special empty data.
const FXString& FXReverseDictionary::at(const void* ky) const {
  if(LEGAL(ky)){
    FXuval p,b,x;
    p=b=HASH(ky);
    while(table[x=p&(no()-1)].key){
      if(table[x].key==ky) return table[x].data;
      p=(p<<2)+p+b+1;
      b>>=BSHIFT;
      }
    }
  return EMPTY[0].data;
  }


// Remove data at given key, if key is non-NULL.
// A resize is triggered when the occupancy drops below the next smaller power of two.
FXString FXReverseDictionary::remove(const void* ky){
  if(LEGAL(ky)){
    FXString old;
    FXuval p,b,x;
    p=b=HASH(ky);
    while(table[x=p&(no()-1)].key!=ky){
      if(table[x].key==nullptr) return FXString::null;
      p=(p<<2)+p+b+1;
      b>>=BSHIFT;
      }
    table[x].key=VOID;                  // Void the slot (not empty!)
    old=table[x].data;
    table[x].data.clear();
    used(used()-1);
    if(used()<=(no()>>2)) resize(no()>>1);
    }
  return FXString::null;
  }


// Erase element at pos, if it wasn't empty already.
// A resize is triggered when the occupancy drops below the next smaller power of two.
FXString FXReverseDictionary::erase(FXival pos){
  if(pos<0 || no()<=pos){ throw FXRangeException("FXReverseDictionary::erase: argument out of range\n"); }
  if(LEGAL(table[pos].key)){
    FXString old;
    table[pos].key=VOID;                // Void the slot (not empty!)
    old=table[pos].data;
    table[pos].data.clear();
    used(used()-1);
    if(used()<=(no()>>2)) resize(no()>>1);
    return old;
    }
  return FXString::null;
  }


// Resize to one element (the special empty element) when cleared.
FXbool FXReverseDictionary::clear(){
  return no(1);
  }


// When destroyed, clear the dictionary.
FXReverseDictionary::~FXReverseDictionary(){
  clear();
  }

}
