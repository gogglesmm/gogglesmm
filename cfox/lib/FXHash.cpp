/********************************************************************************
*                                                                               *
*                       H a s h   T a b l e   C l a s s                         *
*                                                                               *
*********************************************************************************
* Copyright (C) 2003,2020 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "FXArray.h"
#include "FXHash.h"
#include "FXException.h"
#include "FXElement.h"


/*
  Notes:
  - The table hashes pointers to pointers, or things which fit into a pointer to
    things which fit into a pointers.

  - The hash table does not know anything about the keys, or the values; thus, subclasses
    are responsible for managing the memory of the contents.

  - Initially the table is initialized to the empty-table pointer.  This will have only
    a single free slot; this means the table pointer is NEVER null.

  - The hash-table object requires only one pointer's worth of memory.  Thus an empty
    hash table requires very little space, and no table is allocated until at least one
    element is added.

  - Probe position p, probe increment b, and index x MUST be unsigned.

  - The table resize algorithm:

      - Table maintains count of used slots, and count of free slots; when an entry
        is removed, used slot count will be decremented, but free slot count will
        stay the same, because the slot is voided, not freed.  This is because the
        removed slot may still be part of another entry's probe sequence.
        When an entry is added, it will be placed in a voided slot if possible;
        if not, then it will be placed in a free slot.

      - When the table is resized, all entries wil be re-hashed, and voided slots be
        reclaimed as free slots.

      - Table is grown PRIOR to adding entries, and shrunk AFTER removing entries.

      - Table is grown when the number of remaining free slots is less than or
        equal to 1/4 of the table size (plus 1, because the resize occurs PRIOR to
        adding the entry).

      - Table is shrunk when the number of used slots is less than or equal to 1/4
        of the table size.

      - Table must always maintain at least one free slot to insure that probing
        sequence is always finite (all locations are eventually visited, and thus
        the probing sequence is guaranteed to be finite).

      - Thus the new implementation will be at least 1/4 used, and at most 3/4 filled
        (used and voided slots do not exceed 3/4 of table size).  The "hysteresis"
        between growing and shrinking means that adding, then removing a single item
        will not cause a table resize (except when the table is near empty, of course).
*/

#define EMPTY     ((Entry*)(__hash__empty__+3))
#define NOMEMORY  ((Entry*)(((FXival*)NULL)+3))
#define HASH(x)   ((FXival)(x)^(((FXival)(x))>>13))
#define VOID      ((FXptr)-1L)
#define LEGAL(p)  ((p)!=NULL && (p)!=VOID)
#define BSHIFT    5


using namespace FX;

/*******************************************************************************/

namespace FX {


// Empty object list
extern const FXival __hash__empty__[];
const FXival __hash__empty__[7]={1,0,1,0,0};


// Make empty table
FXHash::FXHash():table(EMPTY){
  }


// Construct from another table
FXHash::FXHash(const FXHash& other):table(EMPTY){
  if(__likely(1<other.no() && no(other.no()))){
    copyElms(table,other.table,no());
    free(other.free());
    used(other.used());
    }
  }


// Adjust the size of the table
FXbool FXHash::no(FXival n){
  FXival m=no();
  if(__likely(m!=n)){
    Entry *elbat;

    // Release old table
    if(1<m){
      destructElms(table,m);
      ::free(((FXival*)table)-3);
      table=EMPTY;
      }

    // Allocate new table
    if(1<n){
      if((elbat=(Entry*)(((FXival*)::calloc(sizeof(FXival)*3+sizeof(Entry)*n,1))+3))==NOMEMORY) return false;
      ((FXival*)elbat)[-3]=n;
      ((FXival*)elbat)[-2]=0;
      ((FXival*)elbat)[-1]=n;
      constructElms(elbat,n);
      table=elbat;
      }
    }
  return true;
  }


// Resize the table to the given size, keeping contents
FXbool FXHash::resize(FXival n){
  FXHash elbat;
  FXASSERT((n&(n-1))==0);       // Power of w
  FXASSERT((n-used())>0);       // At least one free slot
  if(elbat.no(n)){
    if(1<elbat.no() && 1<no()){
      FXptr key,data;
      FXuval p,b,x;
      FXival i;
      for(i=0; i<no(); ++i){    // Hash existing entries into new table
        key=table[i].key;
        data=table[i].data;
        if(LEGAL(key)){
          p=b=HASH(key);
          while(elbat.table[x=p&(n-1)].key){    // Locate slot
            p=(p<<2)+p+b+1;
            b>>=BSHIFT;
            }
          elbat.table[x].key=key;
          elbat.table[x].data=data;
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


// Assign from another table
FXHash& FXHash::operator=(const FXHash& other){
  if(__likely(table!=other.table && no(other.no()))){
    copyElms(table,other.table,no());
    free(other.free());
    used(other.used());
    }
  return *this;
  }


// Adopt table from another
FXHash& FXHash::adopt(FXHash& other){
  if(__likely(table!=other.table)){
    swap(table,other.table);
    other.clear();
    }
  return *this;
  }


// Find position of given key
FXival FXHash::find(FXptr ky) const {
  if(__likely(LEGAL(ky))){
    FXuval p,b,x;
    p=b=HASH(ky);
    while(__likely(table[x=p&(no()-1)].key)){
      if(__likely(table[x].key==ky)) return x;
      p=(p<<2)+p+b+1;
      b>>=BSHIFT;
      }
    }
  return -1;
  }


// Return reference to slot assocated with given key
FXptr& FXHash::at(FXptr ky){
  if(__likely(LEGAL(ky))){
    FXuval p,b,h,x;
    p=b=h=HASH(ky);
    while(table[x=p&(no()-1)].key){
      if(table[x].key==ky) goto x;           // Replace existing slot
      p=(p<<2)+p+b+1;
      b>>=BSHIFT;
      }
    if(__unlikely(free()<=1+(no()>>2)) && __unlikely(!resize(no()<<1))){ throw FXMemoryException("FXHash::at: out of memory\n"); }
    p=b=h;
    while(table[x=p&(no()-1)].key){
      if(table[x].key==VOID) goto y;           // Put into voided slot
      p=(p<<2)+p+b+1;
      b>>=BSHIFT;
      }
    free(free()-1);                             // Put into empty slot
y:  used(used()+1);
    table[x].key=ky;
x:  return table[x].data;
    }
  return *((FXptr*)NULL);               // Can NOT be referenced; will generate segfault!
  }


// Return constant reference to slot assocated with given key
const FXptr& FXHash::at(FXptr ky) const {
  if(__likely(LEGAL(ky))){
    FXuval p,b,x;
    p=b=HASH(ky);
    while(table[x=p&(no()-1)].key){
      if(table[x].key==ky) return table[x].data;        // Return existing slot
      p=(p<<2)+p+b+1;
      b>>=BSHIFT;
      }
    }
  return EMPTY[0].data;                 // Can be safely referenced, will read as NULL
  }


// Remove association from the table
FXptr FXHash::remove(FXptr ky){
  FXptr old=NULL;
  if(__likely(LEGAL(ky))){
    FXuval p,b,x;
    p=b=HASH(ky);
    while(table[x=p&(no()-1)].key!=ky){
      if(table[x].key==NULL) return NULL;
      p=(p<<2)+p+b+1;
      b>>=BSHIFT;
      }
    old=table[x].data;
    table[x].key=VOID;                         // Void the slot (not empty!)
    table[x].data=NULL;
    used(used()-1);
    if(__unlikely(used()<=(no()>>2))) resize(no()>>1);
    }
  return old;
  }


// Erase data at pos in the table, returning old pointer
FXptr FXHash::erase(FXival pos){
  FXptr old=NULL;
  if(__unlikely(pos<0 || no()<=pos)){ throw FXRangeException("FXHash::erase: argument out of range\n"); }
  if(__likely(LEGAL(table[pos].key))){
    old=table[pos].data;
    table[pos].key=VOID;                        // Void the slot (not empty!)
    table[pos].data=NULL;
    used(used()-1);
    if(__unlikely(used()<=(no()>>2))) resize(no()>>1);
    }
  return old;
  }


// Clear hash table, marking all slots as free
void FXHash::clear(){
  no(1);
  }


// Destroy table
FXHash::~FXHash(){
  clear();
  }

}
