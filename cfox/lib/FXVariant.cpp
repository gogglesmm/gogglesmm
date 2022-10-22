/********************************************************************************
*                                                                               *
*                          V a r i a n t   T y p e                              *
*                                                                               *
*********************************************************************************
* Copyright (C) 2013,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
  - Variant is a "discriminated union" type that may hold a integer, floating
    point number, string, or respectively an array or key/value collection of
    variants.  Thus, a single Variant can hold an arbitrarily complex collection
    of information.
  - As such, this makes for a convenient data structure to serialize and deserialize
    JSON files; JSON syntax in fact maps almost 1:1 to Variant capabilities.
  - Access to Variant's information is most typically performed using overloaded
    conversion operators [reading information from variants], and overloaded
    assignment operators [writing data to variants].
  - A few important caveats are worth mentioning for effiencent use of this flexible
    data structure:

      1 When writing to variant as an array, its best to reference the last element
        first, or if the number of elements is known in advance, to set the size
        explicitly prior to assigning elements.  Even though Variant automatically
        adapts the size of the array based on the index being accessed, such usage
        may lead to much unneccessary reallocations and copying; if things get big
        that may be performance bottleneck.

      2 Be aware that variant map may get resized as more key/value pairs are added;
        this means don't hang on to references to Variants that may be affected by
        the resize.

      3 Simple numbers (booleans, integers, floats, etc.) are VERY efficient to store;
        consequently, its fine to store a fairly large array into Variant, as long as
        one keeps point (1) above in mind.

      4 No artificial limits to sizes.  Variant should handle arbitrarily large data-
        structures, but efficient use needs to observe (1) above.

  - Object member operator or array indexing operator have two flavors; the non-const
    version will change the type of the variant automatically, and possibly return
    reference to newly created variant object.
    The const operators will return default Null variant if referencing non-existing
    members.
  - Probably should simplify storage of all integer types to either FXlong or FXulong;
    this will lead to fewer cases and cost no extra storage at all since the union is
    the size of the biggest type, anyway.
*/

// Largest and smallest long values
#ifndef LLONG_MAX
#define LLONG_MAX  FXLONG(9223372036854775807)
#endif
#ifndef LLONG_MIN
#define LLONG_MIN  (-LLONG_MAX-FXLONG(1))
#endif
#ifndef ULLONG_MAX
#define ULLONG_MAX FXULONG(18446744073709551615)
#endif

using namespace FX;

namespace FX {

/*******************************************************************************/

// Default variant
const FXVariant FXVariant::null;


// Initialize with default value for type t
FXbool FXVariant::init(Type t){
  switch(t){
  case BoolType:
  case CharType:
  case IntType:
  case UIntType:
  case LongType:
  case ULongType:
  case PointerType:
    value.u=0;
    type=t;
    return true;
  case FloatType:
  case DoubleType:
    value.d=0.0;
    type=t;
    return true;
  case StringType:
    construct(reinterpret_cast<FXString*>(&value.p));
    type=t;
    return true;
  case ArrayType:
    construct(reinterpret_cast<FXVariantArray*>(&value.p));
    type=t;
    return true;
  case MapType:
    construct(reinterpret_cast<FXVariantMap*>(&value.p));
    type=t;
    return true;
  default:
    return true;
    }
  return true;
  }


// Clear the data
FXbool FXVariant::clear(){
  switch(type){
  case BoolType:
  case CharType:
  case IntType:
  case UIntType:
  case LongType:
  case ULongType:
  case PointerType:
  case FloatType:
  case DoubleType:
    value.u=0L;
    type=NullType;
    return true;
  case StringType:
    destruct(reinterpret_cast<FXString*>(&value.p));
    value.u=0L;
    type=NullType;
    return true;
  case ArrayType:
    destruct(reinterpret_cast<FXVariantArray*>(&value.p));
    value.u=0L;
    type=NullType;
    return true;
  case MapType:
    destruct(reinterpret_cast<FXVariantMap*>(&value.p));
    value.u=0L;
    type=NullType;
    return true;
  default:
    return true;
    }
  return true;
  }


// Make a copy
FXVariant& FXVariant::assign(const FXVariant& other){
  if(this!=&other){
    clear();
    switch(other.type){
    case BoolType:
    case CharType:
    case IntType:
    case UIntType:
    case LongType:
    case ULongType:
    case FloatType:
    case DoubleType:
    case PointerType:
      value=other.value;
      type=other.type;
      return *this;
    case StringType:
      construct(reinterpret_cast<FXString*>(&value.p),*reinterpret_cast<const FXString*>(&other.value.p));
      type=other.type;
      return *this;
    case ArrayType:
      construct(reinterpret_cast<FXVariantArray*>(&value.p),*reinterpret_cast<const FXVariantArray*>(&other.value.p));
      type=other.type;
      return *this;
    case MapType:
      construct(reinterpret_cast<FXVariantMap*>(&value.p),*reinterpret_cast<const FXVariantMap*>(&other.value.p));
      type=other.type;
      return *this;
    default:
      return *this;
      }
    }
  return *this;
  }


// Adopt variant from another
FXVariant& FXVariant::adopt(FXVariant& other){
  if(this!=&other){
    swap(value,other.value);
    swap(type,other.type);
    other.clear();
    }
  return *this;
  }

/*******************************************************************************/

// Initialize null variant
FXVariant::FXVariant():type(NullType){
  FXASSERT(sizeof(value)>=sizeof(FXString) && sizeof(value)>=sizeof(FXVariantArray) &&  sizeof(value)>=sizeof(FXVariantMap));
  value.u=0;
  }


// Copy constructor
FXVariant::FXVariant(const FXVariant& other):type(NullType){
  FXASSERT(sizeof(value)>=sizeof(FXString) && sizeof(value)>=sizeof(FXVariantArray) &&  sizeof(value)>=sizeof(FXVariantMap));
  assign(other);
  }


// Construct and initialize with bool
FXVariant::FXVariant(FXbool val):type(BoolType){
  FXASSERT(sizeof(value)>=sizeof(FXString) && sizeof(value)>=sizeof(FXVariantArray) &&  sizeof(value)>=sizeof(FXVariantMap));
  value.u=val;
  }


// Construct and initialize with char
FXVariant::FXVariant(FXchar val):type(CharType){
  FXASSERT(sizeof(value)>=sizeof(FXString) && sizeof(value)>=sizeof(FXVariantArray) &&  sizeof(value)>=sizeof(FXVariantMap));
  value.u=val;
  }


// Construct and initialize with int
FXVariant::FXVariant(FXint val):type(IntType){
  FXASSERT(sizeof(value)>=sizeof(FXString) && sizeof(value)>=sizeof(FXVariantArray) &&  sizeof(value)>=sizeof(FXVariantMap));
  value.i=val;
  }


// Construct and initialize with unsigned int
FXVariant::FXVariant(FXuint val):type(UIntType){
  FXASSERT(sizeof(value)>=sizeof(FXString) && sizeof(value)>=sizeof(FXVariantArray) &&  sizeof(value)>=sizeof(FXVariantMap));
  value.u=val;
  }


// Construct and initialize with long
FXVariant::FXVariant(FXlong val):type(LongType){
  FXASSERT(sizeof(value)>=sizeof(FXString) && sizeof(value)>=sizeof(FXVariantArray) &&  sizeof(value)>=sizeof(FXVariantMap));
  value.i=val;
  }


// Construct and initialize with unsigned long
FXVariant::FXVariant(FXulong val):type(ULongType){
  FXASSERT(sizeof(value)>=sizeof(FXString) && sizeof(value)>=sizeof(FXVariantArray) &&  sizeof(value)>=sizeof(FXVariantMap));
  value.u=val;
  }


// Construct and initialize with float
FXVariant::FXVariant(FXfloat val):type(FloatType){
  FXASSERT(sizeof(value)>=sizeof(FXString) && sizeof(value)>=sizeof(FXVariantArray) &&  sizeof(value)>=sizeof(FXVariantMap));
  value.d=val;
  }


// Construct and initialize with double
FXVariant::FXVariant(FXdouble val):type(DoubleType){
  FXASSERT(sizeof(value)>=sizeof(FXString) && sizeof(value)>=sizeof(FXVariantArray) &&  sizeof(value)>=sizeof(FXVariantMap));
  value.d=val;
  }


// Construct and initialize with pointer
FXVariant::FXVariant(FXptr val):type(PointerType){
  FXASSERT(sizeof(value)>=sizeof(FXString) && sizeof(value)>=sizeof(FXVariantArray) &&  sizeof(value)>=sizeof(FXVariantMap));
  value.p=val;
  }


// Construct and initialize with string
FXVariant::FXVariant(const FXchar *val):type(StringType){
  FXASSERT(sizeof(value)>=sizeof(FXString) && sizeof(value)>=sizeof(FXVariantArray) &&  sizeof(value)>=sizeof(FXVariantMap));
  construct(reinterpret_cast<FXString*>(&value.p),val);
  }


// Construct and initialize with string
FXVariant::FXVariant(const FXString& val):type(StringType){
  FXASSERT(sizeof(value)>=sizeof(FXString) && sizeof(value)>=sizeof(FXVariantArray) &&  sizeof(value)>=sizeof(FXVariantMap));
  construct(reinterpret_cast<FXString*>(&value.p),val);
  }

/*******************************************************************************/

// Change type
void FXVariant::setType(Type t){
  clear();
  init(t);
  }


// Return size of array
FXival FXVariant::no() const {
  return (type==ArrayType) ? reinterpret_cast<const FXVariantArray*>(&value.p)->no() : 0;
  }


// Change number of elements in array
FXbool FXVariant::no(FXival n){
  if(type!=ArrayType){ clear(); init(ArrayType); }
  return reinterpret_cast<FXVariantArray*>(&value.p)->no(n);
  }


// Check if key is mapped
FXbool FXVariant::has(const FXchar* key) const {
  return (type==MapType) && (reinterpret_cast<const FXVariantMap*>(&value.p)->has(key));
  }


// Convert to bool
FXbool FXVariant::toBool() const {
  switch(type){
  case BoolType:
  case CharType:
  case IntType:
  case UIntType:
  case LongType:
  case ULongType:
  case PointerType:
    return !!value.u;                                   // True if non-0
  case FloatType:
  case DoubleType:
    return !!value.d;                                   // True if non-0
  case StringType:
    return !asString().empty();                         // True for non-empty string
  case ArrayType:
    return !!asArray().no();                            // True for non-empty array
  case MapType:
    return !asMap().empty();                            // True for non-empty map
  default:
    return false;                                       // Always false
    }
  return false;
  }


// Convert to pointer
FXptr FXVariant::toPtr() const {
  return (type==PointerType) ? value.p : nullptr;       // NULL anything not a pointer
  }


// Convert to int
FXint FXVariant::toInt(FXbool* ok) const {
  FXbool flag=false;
  FXlong result=0;
  switch(type){
  case BoolType:
  case CharType:
  case IntType:
  case UIntType:
  case LongType:
  case ULongType:
    result=value.i;
    if(__unlikely(result>INT_MAX)){ result=INT_MAX; break; }
    if(__unlikely(result<INT_MIN)){ result=INT_MIN; break; }
    flag=true;
    break;
  case FloatType:
  case DoubleType:
    if(__unlikely(value.d<-2147483648.0)){ result=INT_MIN; break; }
    if(__unlikely(value.d>=2147483648.0)){ result=INT_MAX; break; }
    result=Math::lrint(value.d);        // Nearest int
    flag=true;
    break;
  case StringType:
    return asString().toInt(10,ok);
  default:
    break;
    }
  if(__unlikely(ok)){ *ok=flag; }
  return (FXint)result;
  }


// Convert to unsigned int
FXuint FXVariant::toUInt(FXbool* ok) const {
  FXbool flag=false;
  FXlong result=0;
  switch(type){
  case BoolType:
  case CharType:
  case IntType:
  case UIntType:
  case LongType:
  case ULongType:
    result=value.i;
    if(__unlikely(result<=0)){ result=0; break; }
    if(__unlikely(result>=UINT_MAX)){ result=UINT_MAX; break; }
    flag=true;
    break;
  case FloatType:
  case DoubleType:
    if(__unlikely(value.d<0.0)){ result=0; break; }
    if(__unlikely(value.d>=4294967296.0)){ result=UINT_MAX; break; }
    result=Math::lrint(value.d);        // Nearest unsigned int
    flag=true;
    break;
  case StringType:
    return asString().toUInt(10,ok);
  default:
    break;
    }
  if(__unlikely(ok)){ *ok=flag; }
  return (FXuint)result;
  }


// Convert to long
FXlong FXVariant::toLong(FXbool* ok) const {
  FXbool flag=false;
  FXlong result=0;
  switch(type){
  case BoolType:
  case CharType:
  case IntType:
  case UIntType:
  case LongType:
  case ULongType:
    result=value.i;
    flag=true;
    break;
  case FloatType:
  case DoubleType:
    if(__unlikely(value.d<-9223372036854775808.0)){ result=LLONG_MIN; break; }
    if(__unlikely(value.d>=9223372036854775808.0)){ result=LLONG_MAX; break; }
    result=Math::lrint(value.d);        // Nearest long
    flag=true;
    break;
  case StringType:
    return asString().toLong(10,ok);
  default:
    break;
    }
  if(__unlikely(ok)){ *ok=flag; }
  return result;
  }


// Convert to unsigned long
FXulong FXVariant::toULong(FXbool* ok) const {
  FXbool flag=false;
  FXulong result=0;
  switch(type){
  case BoolType:
  case CharType:
  case IntType:
  case UIntType:
  case LongType:
  case ULongType:
    result=value.u;
    flag=true;
    break;
  case FloatType:
  case DoubleType:
    if(__unlikely(value.d<0.0)){ result=0; break; }
    if(__unlikely(value.d>=18446744073709551616.0)){ result=ULLONG_MAX; break; }
    result=(FXulong)(value.d+0.5);      // Nearest unsigned long
    flag=true;
    break;
  case StringType:
    return asString().toULong(10,ok);
  default:
    break;
    }
  if(__unlikely(ok)){ *ok=flag; }
  return result;
  }


// Convert to float
FXfloat FXVariant::toFloat(FXbool* ok) const {
  FXbool flag=false;
  FXfloat result=0.0f;
  switch(type){
  case BoolType:
  case IntType:
  case LongType:
    result=(FXfloat)value.i;
    flag=true;
    break;
  case CharType:
  case UIntType:
  case ULongType:
    result=(FXfloat)value.u;
    flag=true;
    break;
  case FloatType:
  case DoubleType:
    result=(FXfloat)value.d;
    flag=true;
    break;
  case StringType:
    return asString().toFloat(ok);
  default:
    break;
    }
  if(__unlikely(ok)){ *ok=flag; }
  return result;
  }


// Convert to double
FXdouble FXVariant::toDouble(FXbool* ok) const {
  FXbool flag=false;
  FXdouble result=0.0;
  switch(type){
  case BoolType:
  case IntType:
  case LongType:
    result=(FXdouble)value.i;
    flag=true;
    break;
  case CharType:
  case UIntType:
  case ULongType:
    result=(FXdouble)value.u;
    flag=true;
    break;
  case FloatType:
  case DoubleType:
    result=value.d;
    flag=true;
    break;
  case StringType:
    return asString().toDouble(ok);
  default:
    break;
    }
  if(__unlikely(ok)){ *ok=flag; }
  return result;
  }


// Convert to char pointer
const FXchar* FXVariant::toChars() const {
  return (type==StringType) ? value.s : FXString::null;
  }


// Convert to string
FXString FXVariant::toString(FXbool* ok) const {
  const FXchar truth[2][6]={"false","true"};
  switch(type){
  case BoolType:
    if(ok) *ok=true;
    return FXString(truth[value.u&1]);
  case CharType:
    if(ok) *ok=true;
    return FXString((FXchar)value.u,1);
  case IntType:
  case LongType:
    if(ok) *ok=true;
    return FXString::value(value.i);
  case UIntType:
  case ULongType:
    if(ok) *ok=true;
    return FXString::value(value.u);
  case FloatType:
  case DoubleType:
    if(ok) *ok=true;
    return FXString::value(value.d,16);
  case StringType:
    if(ok) *ok=true;
    return asString();
  default:
    if(ok) *ok=false;
    return FXString::null;
    }
  return FXString::null;
  }

/*******************************************************************************/

// Assign with bool
FXVariant& FXVariant::operator=(FXbool val){
  clear();
  value.u=val;
  type=BoolType;
  return *this;
  }


// Assign with char
FXVariant& FXVariant::operator=(FXchar val){
  clear();
  value.u=val;
  type=CharType;
  return *this;
  }


// Assign with int
FXVariant& FXVariant::operator=(FXint val){
  clear();
  value.i=val;
  type=IntType;
  return *this;
  }


// Assign with unsigned int
FXVariant& FXVariant::operator=(FXuint val){
  clear();
  value.u=val;
  type=UIntType;
  return *this;
  }


// Assign with long
FXVariant& FXVariant::operator=(FXlong val){
  clear();
  value.i=val;
  type=LongType;
  return *this;
  }


// Assign with unsigned long
FXVariant& FXVariant::operator=(FXulong val){
  clear();
  value.u=val;
  type=ULongType;
  return *this;
  }


// Assign with float
FXVariant& FXVariant::operator=(FXfloat val){
  clear();
  value.d=val;
  type=FloatType;
  return *this;
  }


// Assign with double
FXVariant& FXVariant::operator=(FXdouble val){
  clear();
  value.d=val;
  type=DoubleType;
  return *this;
  }


// Assign with pointer
FXVariant& FXVariant::operator=(FXptr val){
  clear();
  value.p=val;
  type=PointerType;
  return *this;
  }


// Assign with string
FXVariant& FXVariant::operator=(const FXchar* val){
  clear();
  construct(reinterpret_cast<FXString*>(&value.p),val);
  type=StringType;
  return *this;
  }


// Assign with string
FXVariant& FXVariant::operator=(const FXString& val){
  clear();
  construct(reinterpret_cast<FXString*>(&value.p),val);
  type=StringType;
  return *this;
  }


// Assign with variant
FXVariant& FXVariant::operator=(const FXVariant& val){
  return assign(val);
  }

/*******************************************************************************/

// Remove entry from the table
FXbool FXVariant::remove(const FXchar* key){
  if(type==MapType && key){
    return reinterpret_cast<FXVariantMap*>(&value.p)->remove(key);
    }
  return false;
  }


// Erase entry at pos in the table
FXbool FXVariant::erase(FXival idx){
  if(type==ArrayType && 0<=idx && reinterpret_cast<const FXVariantArray*>(&value.p)->no()){
    return reinterpret_cast<FXVariantArray*>(&value.p)->erase(idx);
    }
  return false;
  }

/*******************************************************************************/

// Return value of object member
FXVariant& FXVariant::at(const FXchar* key){
  if(type!=MapType){ clear(); init(MapType); }
  return reinterpret_cast<FXVariantMap*>(&value.p)->at(key);
  }


// Return value of object member
const FXVariant& FXVariant::at(const FXchar* key) const {
  if(type!=MapType){ return FXVariant::null; }
  return reinterpret_cast<const FXVariantMap*>(&value.p)->at(key);
  }


// Return value of object member
FXVariant& FXVariant::at(const FXString& key){
  if(type!=MapType){ clear(); init(MapType); }
  return reinterpret_cast<FXVariantMap*>(&value.p)->at(key);
  }


// Return value of object member
const FXVariant& FXVariant::at(const FXString& key) const {
  if(type!=MapType){ return FXVariant::null; }
  return reinterpret_cast<const FXVariantMap*>(&value.p)->at(key);
  }

/*******************************************************************************/

// Return value of array member
FXVariant& FXVariant::at(FXival idx){
  if(idx<0){ throw FXRangeException("FXVariant: index out of range\n"); }
  if(type!=ArrayType){ clear(); init(ArrayType); }
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
  if(type==ArrayType && idx<reinterpret_cast<const FXVariantArray*>(&value.p)->no()){
    return reinterpret_cast<const FXVariantArray*>(&value.p)->at(idx);
    }
  return FXVariant::null;
  }

/*******************************************************************************/

// Destroy
FXVariant::~FXVariant(){
  clear();
  }

}
