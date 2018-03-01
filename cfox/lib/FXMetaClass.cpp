/********************************************************************************
*                                                                               *
*                         M e t a C l a s s   O b j e c t                       *
*                                                                               *
*********************************************************************************
* Copyright (C) 1997,2018 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "FXStream.h"
#include "FXMetaClass.h"
#include "FXObject.h"
#include "FXElement.h"
#include "FXException.h"
#include "FXRectangle.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXEvent.h"

#include "FXDebugTarget.h"

/*
  Notes:

  - We need a table of all metaclasses, as we should be able to create any type
    of object during deserialization.
  - For MacOS/X support, we moved fxmalloc() and co. here; the reason is that
    when FOX is loaded as a DLL into FXRuby, these symbols need to be resolvable
    in order for the DLL startup code to run properly for the meta class
    initializers; afterward everything's OK.
  - For abstract classes, FXMetaClass contains nullObject() as manufacture function;
    this will always return NULL as abstract classes can't be instantiated.
  - Possibly store hash into FXMetaClass during construction.  Benefits are:
      - No need to recompute it during destruction or growth of hash table.
      - Quick equality test inside getMetaClassFromName().
      - Very minor space penalty.
*/


using namespace FX;

namespace FX {

/***********************  Memory Allocation Functions  *************************/

// Allocate memory
FXbool fxmalloc(void** ptr,FXuval size){
  *ptr=NULL;
  if(size!=0){
    if((*ptr=malloc(size))==NULL) return false;
    }
  return true;
  }


// Allocate cleaned memory
FXbool fxcalloc(void** ptr,FXuval size){
  *ptr=NULL;
  if(size!=0){
    if((*ptr=calloc(size,1))==NULL) return false;
    }
  return true;
  }


// Resize memory
FXbool fxresize(void** ptr,FXuval size){
  register void *p=NULL;
  if(size!=0){
    if((p=realloc(*ptr,size))==NULL) return false;
    }
  else{
    if(*ptr) free(*ptr);
    }
  *ptr=p;
  return true;
  }


// Allocate and initialize memory
FXbool fxmemdup(void** ptr,const void* src,FXuval size){
  *ptr=NULL;
  if(size!=0 && src!=NULL){
    if((*ptr=malloc(size))==NULL) return false;
    memcpy(*ptr,src,size);
    }
  return true;
  }


// String duplicate
FXchar *fxstrdup(const FXchar* str){
  register FXchar *ptr=NULL;
  if(str!=NULL){
    register FXint size=strlen(str)+1;
    if((ptr=(FXchar*)malloc(size))!=NULL){
      memcpy(ptr,str,size);
      }
    }
  return ptr;
  }


// Free memory, resets ptr to NULL afterward
void fxfree(void** ptr){
  if(*ptr){
    free(*ptr);
    *ptr=NULL;
    }
  }


/*************************  FXMetaClass Implementation  ************************/

// Empty but previously used hash table slot
#define EMPTY  ((FXMetaClass*)-1L)

// Hash table of metaclasses initialized at load-time
const FXMetaClass** FXMetaClass::metaClassTable=NULL;
FXuint              FXMetaClass::metaClassSlots=0;
FXuint              FXMetaClass::metaClassCount=0;


// Hash function for string
static inline FXuint hashstring(const FXchar* str){
  register FXuint result=0;
  register FXuchar c;
  while((c=*str++)!='\0'){
    result=((result<<5)+result)^c;
    }
  return result;
  }


// Resize global hash table
void FXMetaClass::resize(FXuint slots){
  register const FXMetaClass *ptr;
  const FXMetaClass **table;
  register FXuint p,x,s;
  callocElms(table,slots);
  for(s=0; s<metaClassSlots; ++s){
    if((ptr=metaClassTable[s])!=NULL && ptr!=EMPTY){
      p=hashstring(ptr->className);
      x=(p<<1)|1;
      while(table[p=(p+x)&(slots-1)]!=NULL){
        }
      table[p]=ptr;
      }
    }
  freeElms(metaClassTable);
  metaClassTable=table;
  metaClassSlots=slots;
  }


// Constructor adds metaclass to the table
FXMetaClass::FXMetaClass(const FXchar* name,FXObject *(fac)(),const FXMetaClass* base,const void* ass,FXuint nass,FXuint assz):className(name),manufacture(fac),baseClass(base),assoc(ass),nassocs(nass),assocsz(assz){
  register FXuint p=hashstring(className);
  register FXuint x=(p<<1)|1;
  if((++metaClassCount<<1) > metaClassSlots){
    resize(metaClassSlots?metaClassSlots<<1:1);
    }
  FXASSERT(metaClassSlots>=metaClassCount);
  while(metaClassTable[p=(p+x)&(metaClassSlots-1)]!=NULL){
    }
  metaClassTable[p]=this;
  }


// Create an object instance
FXObject* FXMetaClass::makeInstance() const {
  return (*manufacture)();
  }


// Find function
const void* FXMetaClass::search(FXSelector key) const {
  register const FXObject::FXMapEntry* lst=(const FXObject::FXMapEntry*)assoc;
  register FXuint inc=assocsz;
  register FXuint n=nassocs;
  while(n--){
    if(__unlikely(key<=lst->keyhi) && __likely(lst->keylo<=key)) return lst;
    lst=(const FXObject::FXMapEntry*) (((const FXchar*)lst)+inc);
    }
  return NULL;
  }


// Test if subclass
FXbool FXMetaClass::isSubClassOf(const FXMetaClass* metaclass) const {
  register const FXMetaClass* cls;
  for(cls=this; cls; cls=cls->baseClass){
    if(cls==metaclass) return true;
    }
  return false;
  }


// Find the FXMetaClass belonging to class name
const FXMetaClass* FXMetaClass::getMetaClassFromName(const FXchar* name){
  if(metaClassSlots && name){
    register FXuint p=hashstring(name);
    register FXuint x=(p<<1)|1;
    while(metaClassTable[p=(p+x)&(metaClassSlots-1)]!=NULL){
      if(metaClassTable[p]!=EMPTY && strcmp(metaClassTable[p]->className,name)==0){
        return metaClassTable[p];
        }
      }
    }
  return NULL;
  }


// Make NULL object; used for abstract classes that may not be instantiated
FXObject* FXMetaClass::nullObject(){
  return NULL;
  }


// Destructor removes metaclass from the table
FXMetaClass::~FXMetaClass(){
  register FXuint p=hashstring(className);
  register FXuint x=(p<<1)|1;
  while(metaClassTable[p=(p+x)&(metaClassSlots-1)]!=this){
    }
  metaClassTable[p]=EMPTY;
  if((--metaClassCount<<1) <= metaClassSlots){
    resize(metaClassSlots>>1);
    }
  FXASSERT(metaClassSlots>=metaClassCount);
  }

}
