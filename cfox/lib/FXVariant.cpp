/********************************************************************************
*                                                                               *
*                          V a r i a n t   T y p e                              *
*                                                                               *
*********************************************************************************
* Copyright (C) 2013,2017 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "fxunicode.h"
#include "FXElement.h"
#include "FXArray.h"
#include "FXString.h"
#include "FXException.h"
#include "FXVariant.h"
#include "FXVariantArray.h"
#include "FXVariantMap.h"

/*
  Notes:
  - General purpose variant type to hold run-time determined values.
  - Probably should simplify storage of all integer types to either FXlong
    or FXulong; this will lead to fewer cases and cost no extra storage
    at all since the union is the size of the biggest type, anyway.
  - Object member operator or array indexing operator have two flavors;
    the non-const version will change the type of the variant automatically,
    and possibly return newly created variant objects.
    The const operators will return default Null variant if referencing non-
    existing members.
  - New mebers or array entries will be automatically created in this case.
  - Converting any non-empty string to type bool yields true; this was changed
    from older implementation.  New implementation makes more sense.
  - Likewise, converting non-empty map or array to boolean also yields true.
*/

using namespace FX;

namespace FX {

/*******************************************************************************/

// Default variant
const FXVariant FXVariant::null;


// Initialize with default value for type t
FXVariant& FXVariant::init(VType t){
  type=t;
  switch(type){
  case VNull:
  case VBool:
  case VChar:
  case VInt:
  case VUInt:
  case VLong:
  case VULong:
  case VPointer:
    value.u=0;
    break;
  case VFloat:
  case VDouble:
    value.d=0.0;
    break;
  case VString:
    construct(reinterpret_cast<FXString*>(&value.p));
    break;
  case VArray:
    construct(reinterpret_cast<FXVariantArray*>(&value.p));
    break;
  case VMap:
    construct(reinterpret_cast<FXVariantMap*>(&value.p));
    break;
    }
  return *this;
  }


// Make a copy
FXVariant& FXVariant::copy(const FXVariant& other){
  if(this!=&other){
    reset();
    type=other.type;
    switch(type){
    case VNull:
    case VBool:
    case VChar:
    case VInt:
    case VUInt:
    case VLong:
    case VULong:
    case VFloat:
    case VDouble:
    case VPointer:
      value=other.value;
      break;
    case VString:
      construct(reinterpret_cast<FXString*>(&value.p),*reinterpret_cast<const FXString*>(&other.value.p));
      break;
    case VArray:
      construct(reinterpret_cast<FXVariantArray*>(&value.p),*reinterpret_cast<const FXVariantArray*>(&other.value.p));
      break;
    case VMap:
      construct(reinterpret_cast<FXVariantMap*>(&value.p),*reinterpret_cast<const FXVariantMap*>(&other.value.p));
      break;
      }
    }
  return *this;
  }

/*******************************************************************************/

// Initialize Null variant
FXVariant::FXVariant():type(VNull){
  FXASSERT(sizeof(value)>=sizeof(FXString) && sizeof(value)>=sizeof(FXVariantArray) &&  sizeof(value)>=sizeof(FXVariantMap));
  value.u=0;
  }


// Copy constructor
FXVariant::FXVariant(const FXVariant& other):type(VNull){
  FXASSERT(sizeof(value)>=sizeof(FXString) && sizeof(value)>=sizeof(FXVariantArray) &&  sizeof(value)>=sizeof(FXVariantMap));
  copy(other);
  }


// Construct and initialize with bool
FXVariant::FXVariant(FXbool val):type(VBool){
  FXASSERT(sizeof(value)>=sizeof(FXString) && sizeof(value)>=sizeof(FXVariantArray) &&  sizeof(value)>=sizeof(FXVariantMap));
  value.u=val;
  }


// Construct and initialize with char
FXVariant::FXVariant(FXchar val):type(VChar){
  FXASSERT(sizeof(value)>=sizeof(FXString) && sizeof(value)>=sizeof(FXVariantArray) &&  sizeof(value)>=sizeof(FXVariantMap));
  value.u=val;
  }


// Construct and initialize with int
FXVariant::FXVariant(FXint val):type(VInt){
  FXASSERT(sizeof(value)>=sizeof(FXString) && sizeof(value)>=sizeof(FXVariantArray) &&  sizeof(value)>=sizeof(FXVariantMap));
  value.i=val;
  }


// Construct and initialize with unsigned int
FXVariant::FXVariant(FXuint val):type(VUInt){
  FXASSERT(sizeof(value)>=sizeof(FXString) && sizeof(value)>=sizeof(FXVariantArray) &&  sizeof(value)>=sizeof(FXVariantMap));
  value.u=val;
  }


// Construct and initialize with long
FXVariant::FXVariant(FXlong val):type(VLong){
  FXASSERT(sizeof(value)>=sizeof(FXString) && sizeof(value)>=sizeof(FXVariantArray) &&  sizeof(value)>=sizeof(FXVariantMap));
  value.i=val;
  }


// Construct and initialize with unsigned long
FXVariant::FXVariant(FXulong val):type(VULong){
  FXASSERT(sizeof(value)>=sizeof(FXString) && sizeof(value)>=sizeof(FXVariantArray) &&  sizeof(value)>=sizeof(FXVariantMap));
  value.u=val;
  }


// Construct and initialize with float
FXVariant::FXVariant(FXfloat val):type(VFloat){
  FXASSERT(sizeof(value)>=sizeof(FXString) && sizeof(value)>=sizeof(FXVariantArray) &&  sizeof(value)>=sizeof(FXVariantMap));
  value.d=val;
  }


// Construct and initialize with double
FXVariant::FXVariant(FXdouble val):type(VDouble){
  FXASSERT(sizeof(value)>=sizeof(FXString) && sizeof(value)>=sizeof(FXVariantArray) &&  sizeof(value)>=sizeof(FXVariantMap));
  value.d=val;
  }


// Construct and initialize with pointer
FXVariant::FXVariant(FXptr val):type(VPointer){
  FXASSERT(sizeof(value)>=sizeof(FXString) && sizeof(value)>=sizeof(FXVariantArray) &&  sizeof(value)>=sizeof(FXVariantMap));
  value.p=val;
  }


// Construct and initialize with string
FXVariant::FXVariant(const FXchar *val):type(VString){
  FXASSERT(sizeof(value)>=sizeof(FXString) && sizeof(value)>=sizeof(FXVariantArray) &&  sizeof(value)>=sizeof(FXVariantMap));
  construct(reinterpret_cast<FXString*>(&value.p),val);
  }


// Construct and initialize with string
FXVariant::FXVariant(const FXString& val):type(VString){
  FXASSERT(sizeof(value)>=sizeof(FXString) && sizeof(value)>=sizeof(FXVariantArray) &&  sizeof(value)>=sizeof(FXVariantMap));
  construct(reinterpret_cast<FXString*>(&value.p),val);
  }


// Construct and initialize with array
FXVariant::FXVariant(const FXVariantArray& val):type(VArray){
  FXASSERT(sizeof(value)>=sizeof(FXString) && sizeof(value)>=sizeof(FXVariantArray) &&  sizeof(value)>=sizeof(FXVariantMap));
  construct(reinterpret_cast<FXVariantArray*>(&value.p),val);
  }


// Construct and initialize with map
FXVariant::FXVariant(const FXVariantMap& val):type(VMap){
  FXASSERT(sizeof(value)>=sizeof(FXString) && sizeof(value)>=sizeof(FXVariantArray) &&  sizeof(value)>=sizeof(FXVariantMap));
  construct(reinterpret_cast<FXVariantMap*>(&value.p),val);
  }

/*******************************************************************************/

// Change type
void FXVariant::setType(VType t){
  reset();
  init(t);
  }


// Return size of array
FXival FXVariant::no() const {
  return (type==VArray) ? reinterpret_cast<const FXVariantArray*>(&value.p)->no() : 0;
  }


// Change number of elements in array
FXbool FXVariant::no(FXival n){
  if(type!=VArray){
    reset();
    init(VArray);
    }
  return reinterpret_cast<FXVariantArray*>(&value.p)->no(n);
  }


// Check if key is mapped
FXbool FXVariant::has(const FXchar* key) const {
  return (type==VMap) && (reinterpret_cast<const FXVariantMap*>(&value.p)->has(key));
  }


// Convert to bool
FXbool FXVariant::toBool() const {
  switch(type){
  case VBool:
  case VChar:
  case VInt:
  case VUInt:
  case VLong:
  case VULong:
  case VPointer:
    return !!value.u;
  case VFloat:
  case VDouble:
    return !!value.d;
  case VString:
    return !reinterpret_cast<const FXString*>(&value.p)->empty();       // True for non-empty string
  case VArray:
    return !!reinterpret_cast<const FXVariantArray*>(&value.p)->no();   // True for non-empty array
  case VMap:
    return !reinterpret_cast<const FXVariantMap*>(&value.p)->empty();   // True for non-empty map
  default:
    return false;
    }
  return false;
  }


// Convert to pointer
FXptr FXVariant::toPtr() const {
  return (type==VPointer) ? value.p : NULL;
  }


// Convert to char pointer
const FXchar* FXVariant::toChars() const {
  return (type==VString) ? value.s : FXString::null;
  }


// Convert to int
FXint FXVariant::toInt(FXbool* ok) const {
  return (FXint)toLong(ok);
  }


// Convert to unsigned int
FXuint FXVariant::toUInt(FXbool* ok) const {
  return (FXuint)toULong(ok);
  }


// Convert to long
FXlong FXVariant::toLong(FXbool* ok) const {
  switch(type){
  case VBool:
  case VChar:
  case VInt:
  case VUInt:
  case VLong:
  case VULong:
    if(ok) *ok=true;
    return value.i;
  case VFloat:
  case VDouble:
    if(ok) *ok=true;
    return (FXlong)value.d;
  case VString:
    return reinterpret_cast<const FXString*>(&value.p)->toLong(10,ok);
  default:
    if(ok) *ok=false;
    return 0;
    }
  return 0;
  }


// Convert to unsigned long
FXulong FXVariant::toULong(FXbool* ok) const {
  switch(type){
  case VBool:
  case VChar:
  case VInt:
  case VUInt:
  case VLong:
  case VULong:
    if(ok) *ok=true;
    return value.u;
  case VFloat:
  case VDouble:
    if(ok) *ok=true;
    return (FXulong)value.d;
  case VString:
    return reinterpret_cast<const FXString*>(&value.p)->toULong(10,ok);
  default:
    if(ok) *ok=false;
    return 0;
    }
  return 0;
  }


// Convert to float
FXfloat FXVariant::toFloat(FXbool* ok) const {
  switch(type){
  case VBool:
  case VInt:
  case VLong:
    if(ok) *ok=true;
    return (FXfloat)value.i;
  case VChar:
  case VUInt:
  case VULong:
    if(ok) *ok=true;
#if _MSC_VER <= 1200
    return (FXfloat)(FXuint)value.u;
#else
    return (FXfloat)value.u;
#endif
  case VFloat:
  case VDouble:
    if(ok) *ok=true;
    return (FXfloat)value.d;
  case VString:
    return reinterpret_cast<const FXString*>(&value.p)->toFloat(ok);
  default:
    if(ok) *ok=false;
    return 0.0f;
    }
  return 0.0f;
  }


// Convert to double
FXdouble FXVariant::toDouble(FXbool* ok) const {
  switch(type){
  case VBool:
  case VInt:
  case VLong:
    if(ok) *ok=true;
    return (FXdouble)value.i;
  case VChar:
  case VUInt:
  case VULong:
    if(ok) *ok=true;
#if _MSC_VER <= 1200
    return (FXdouble)(FXuint)value.u;
#else
    return (FXdouble)value.u;
#endif
  case VFloat:
  case VDouble:
    if(ok) *ok=true;
    return value.d;
  case VString:
    return reinterpret_cast<const FXString*>(&value.p)->toDouble(ok);
  default:
    if(ok) *ok=false;
    return 0.0;
    }
  return 0.0;
  }


// Convert to string
FXString FXVariant::toString(FXbool* ok) const {
  const FXchar truth[2][6]={"false","true"};
  switch(type){
  case VBool:
    if(ok) *ok=true;
    return FXString(truth[value.u&1]);
  case VChar:
    if(ok) *ok=true;
    return FXString((FXchar)value.u,1);
  case VInt:
  case VLong:
    if(ok) *ok=true;
    return FXString::value(value.i);
  case VUInt:
  case VULong:
    if(ok) *ok=true;
    return FXString::value(value.u);
  case VFloat:
  case VDouble:
    if(ok) *ok=true;
    return FXString::value(value.d,16);
  case VString:
    if(ok) *ok=true;
    return *reinterpret_cast<const FXString*>(&value.p);
  default:
    if(ok) *ok=false;
    return FXString::null;
    }
  return FXString::null;
  }

/*******************************************************************************/

// Assign with bool
FXVariant& FXVariant::operator=(FXbool val){
  reset();
  value.u=val;
  type=VBool;
  return *this;
  }


// Assign with char
FXVariant& FXVariant::operator=(FXchar val){
  reset();
  value.u=val;
  type=VChar;
  return *this;
  }


// Assign with int
FXVariant& FXVariant::operator=(FXint val){
  reset();
  value.i=val;
  type=VInt;
  return *this;
  }


// Assign with unsigned int
FXVariant& FXVariant::operator=(FXuint val){
  reset();
  value.u=val;
  type=VUInt;
  return *this;
  }


// Assign with long
FXVariant& FXVariant::operator=(FXlong val){
  reset();
  value.i=val;
  type=VLong;
  return *this;
  }


// Assign with unsigned long
FXVariant& FXVariant::operator=(FXulong val){
  reset();
  value.u=val;
  type=VULong;
  return *this;
  }


// Assign with float
FXVariant& FXVariant::operator=(FXfloat val){
  reset();
  value.d=val;
  type=VFloat;
  return *this;
  }


// Assign with double
FXVariant& FXVariant::operator=(FXdouble val){
  reset();
  value.d=val;
  type=VDouble;
  return *this;
  }


// Assign with pointer
FXVariant& FXVariant::operator=(FXptr val){
  reset();
  value.p=val;
  type=VPointer;
  return *this;
  }


// Assign with string
FXVariant& FXVariant::operator=(const FXchar* val){
  reset();
  construct(reinterpret_cast<FXString*>(&value.p),val);
  type=VString;
  return *this;
  }


// Assign with string
FXVariant& FXVariant::operator=(const FXString& val){
  reset();
  construct(reinterpret_cast<FXString*>(&value.p),val);
  type=VString;
  return *this;
  }


// Assign with array
FXVariant& FXVariant::operator=(const FXVariantArray& val){
  reset();
  construct(reinterpret_cast<FXVariantArray*>(&value.p),val);
  type=VArray;
  return *this;
  }


// Assign with map
FXVariant& FXVariant::operator=(const FXVariantMap& val){
  reset();
  construct(reinterpret_cast<FXVariantMap*>(&value.p),val);
  type=VMap;
  return *this;
  }

/*******************************************************************************/

// Assign with variant
FXVariant& FXVariant::operator=(const FXVariant& val){
  return copy(val);
  }


// Adopt variant from another
FXVariant& FXVariant::adopt(FXVariant& other){
  if(this!=&other){
    reset();
    init(other.type);
    switch(type){
    case VNull:
    case VBool:
    case VChar:
    case VInt:
    case VUInt:
    case VLong:
    case VULong:
    case VPointer:
    case VFloat:
    case VDouble:
      value=other.value;
      break;
    case VString:
      reinterpret_cast<FXString*>(&value.p)->adopt(*reinterpret_cast<FXString*>(&other.value.p));
      break;
    case VArray:
      reinterpret_cast<FXVariantArray*>(&value.p)->adopt(*reinterpret_cast<FXVariantArray*>(&other.value.p));
      break;
    case VMap:
      reinterpret_cast<FXVariantMap*>(&value.p)->adopt(*reinterpret_cast<FXVariantMap*>(&other.value.p));
      break;
      }
    other.reset();
    }
  return *this;
  }


// Adopt variant array
FXVariant& FXVariant::adopt(FXVariantArray& other){
  reset();
  init(VArray);
  reinterpret_cast<FXVariantArray*>(&value.p)->adopt(other);
  return *this;
  }


// Adopt a variant map
FXVariant& FXVariant::adopt(FXVariantMap& other){
  reset();
  init(VMap);
  reinterpret_cast<FXVariantMap*>(&value.p)->adopt(other);
  return *this;
  }

/*******************************************************************************/

// Return value of object member
FXVariant& FXVariant::at(const FXchar* key){
  if(type!=VMap){
    reset();
    init(VMap);
    }
  return reinterpret_cast<FXVariantMap*>(&value.p)->at(key);
  }


// Return value of object member
const FXVariant& FXVariant::at(const FXchar* key) const {
  if(type==VMap){
    return reinterpret_cast<const FXVariantMap*>(&value.p)->at(key);
    }
  return FXVariant::null;
  }


// Return value of object member
FXVariant& FXVariant::at(const FXString& key){
  if(type!=VMap){
    reset();
    init(VMap);
    }
  return reinterpret_cast<FXVariantMap*>(&value.p)->at(key);
  }


// Return value of object member
const FXVariant& FXVariant::at(const FXString& key) const {
  if(type==VMap){
    return reinterpret_cast<const FXVariantMap*>(&value.p)->at(key);
    }
  return FXVariant::null;
  }

/*******************************************************************************/

// Return value of array member
FXVariant& FXVariant::at(FXival idx){
  if(idx<0){ throw FXRangeException("FXVariant: index out of range\n"); }
  if(type!=VArray){
    reset();
    init(VArray);
    }
  if(idx>=reinterpret_cast<FXVariantArray*>(&value.p)->no()){
    if(!reinterpret_cast<FXVariantArray*>(&value.p)->append(FXVariant::null,idx-reinterpret_cast<FXVariantArray*>(&value.p)->no()+1)){
      throw FXMemoryException("FXVariant: out of memory\n");
      }
    }
  return reinterpret_cast<FXVariantArray*>(&value.p)->at(idx);
  }


// Return value of array member
const FXVariant& FXVariant::at(FXival idx) const {
  if(idx<0){ throw FXRangeException("FXVariant: index out of range\n"); }
  if(type==VArray && idx<reinterpret_cast<const FXVariantArray*>(&value.p)->no()){
    return reinterpret_cast<const FXVariantArray*>(&value.p)->at(idx);
    }
  return FXVariant::null;
  }

/*******************************************************************************/

// Clear the data
void FXVariant::clear(){
  switch(type){
  case VNull:
  case VBool:
  case VChar:
  case VInt:
  case VUInt:
  case VLong:
  case VULong:
  case VPointer:
    value.u=0;
    break;
  case VFloat:
  case VDouble:
    value.d=0.0;
    break;
  case VString:
    reinterpret_cast<FXString*>(&value.p)->clear();
    break;
  case VArray:
    reinterpret_cast<FXVariantArray*>(&value.p)->clear();
    break;
  case VMap:
    reinterpret_cast<FXVariantMap*>(&value.p)->clear();
    break;
    }
  }


// Reset to Null
void FXVariant::reset(){
  switch(type){
  case VNull:
  case VBool:
  case VChar:
  case VInt:
  case VUInt:
  case VLong:
  case VULong:
  case VFloat:
  case VDouble:
  case VPointer:
    break;
  case VString:
    destruct(reinterpret_cast<FXString*>(&value.p));
    break;
  case VArray:
    destruct(reinterpret_cast<FXVariantArray*>(&value.p));
    break;
  case VMap:
    destruct(reinterpret_cast<FXVariantMap*>(&value.p));
    break;
    }
  value.u=0;
  type=VNull;
  }


// Destroy
FXVariant::~FXVariant(){
  reset();
  }

}
