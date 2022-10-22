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
#ifndef FXVARIANT_H
#define FXVARIANT_H

namespace FX {


class FXString;
class FXVariantMap;
class FXVariantArray;


/**
* A Variant type can hold any kind of object, be it a boolean, integer, real, string,
* or even array of Variants or dictionaries of variants.
* Complex hierarchies of Variants can be loaded (and saved) using the JSON parser.
* When writing Variants, dictionaries and arrays are automatically grown.  When
* reading Variants, non-existing dictionary entries or indexes outside arrays will read
* as 0 (for numbers), the empty string, or as an empty dictionary or array.
* For efficiency, you can hold references to Variants, for example to avoid repeatedly
* accessing dictionaries or arrays with the same key or index. However, be aware that
* adding or removing sub-items to dictionaries or arrays may cause reallocations of
* existing items and thus some care must be exercised when doing this.
*/
class FXAPI FXVariant {
public:
  enum Type {
    NullType=0,         // Simple types
    BoolType,
    CharType,
    IntType,
    UIntType,
    LongType,
    ULongType,
    FloatType,
    DoubleType,
    PointerType,
    StringType,         // Complex types
    ArrayType,
    MapType
    };
private:
  union Value {
    FXlong   i;         // Signed integral types
    FXulong  u;         // Unsigned integral types
    FXdouble d;         // Floating point types
    FXchar*  s;         // Character string
    FXptr    p;         // Pointer types
    };
private:
  Value      value;     // Current value
  Type       type;      // Type of value
private:
  FXbool init(Type t);
public:

  /// Default constructor makes null type
  FXVariant();

  /// Copy constructor
  FXVariant(const FXVariant& other);

  /// Construct and initialize with bool
  explicit FXVariant(FXbool val);

  /// Construct and initialize with char
  explicit FXVariant(FXchar val);

  /// Construct and initialize with int
  explicit FXVariant(FXint val);

  /// Construct and initialize with unsigned int
  explicit FXVariant(FXuint val);

  /// Construct and initialize with long
  explicit FXVariant(FXlong val);

  /// Construct and initialize with unsigned long
  explicit FXVariant(FXulong val);

  /// Construct and initialize with float
  explicit FXVariant(FXfloat val);

  /// Construct and initialize with double
  explicit FXVariant(FXdouble val);

  /// Construct and initialize with pointer
  explicit FXVariant(FXptr val);

  /// Construct and initialize with constant string
  explicit FXVariant(const FXchar *val);

  /// Construct and initialize with string
  explicit FXVariant(const FXString& val);

  /// Change type
  void setType(Type t);

  /// Return type
  Type getType() const { return type; }

  /// Return size of array
  FXival no() const;

  /// Change number of elements in array
  FXbool no(FXival n);

  /// Is it a null?
  FXbool isNull() const { return type==NullType; }

  /// Is it a bool?
  FXbool isBool() const { return type==BoolType; }

  /// Is it a character?
  FXbool isChar() const { return type==CharType; }

  /// Is it a int?
  FXbool isInt() const { return type==IntType; }

  /// Is it a unsigned int?
  FXbool isUInt() const { return type==UIntType; }

  /// Is it a long?
  FXbool isLong() const { return type==LongType; }

  /// Is it a unsigned long?
  FXbool isULong() const { return type==ULongType; }

  /// Is it a float?
  FXbool isFloat() const { return type==FloatType; }

  /// Is it a double?
  FXbool isDouble() const { return type==DoubleType; }

  /// Is it a integer (bool, char, ..., or long)?
  FXbool isInteger() const { return BoolType<=type && type<=ULongType; }

  /// Is it a real (float or double)?
  FXbool isReal() const { return FloatType<=type && type<=DoubleType; }

  /// Is it any kind of number?
  FXbool isNumber() const { return BoolType<=type && type<=DoubleType; }

  /// Is it a pointer?
  FXbool isPtr() const { return type==PointerType; }

  /// Is it a string?
  FXbool isString() const { return type==StringType; }

  /// Is it a array?
  FXbool isArray() const { return type==ArrayType; }

  /// Is it a map?
  FXbool isMap() const { return type==MapType; }

  /// Convert to bool; always OK
  FXbool toBool() const;

  /// Convert to pointer
  FXptr toPtr() const;

  /// Convert to int
  FXint toInt(FXbool* ok=nullptr) const;

  /// Convert to unsigned int
  FXuint toUInt(FXbool* ok=nullptr) const;

  /// Convert to long
  FXlong toLong(FXbool* ok=nullptr) const;

  /// Convert to unsigned long
  FXulong toULong(FXbool* ok=nullptr) const;

  /// Convert to float
  FXfloat toFloat(FXbool* ok=nullptr) const;

  /// Convert to double
  FXdouble toDouble(FXbool* ok=nullptr) const;

  /// Convert to char pointer
  const FXchar* toChars() const;

  /// Convert to string
  FXString toString(FXbool* ok=nullptr) const;

  /// Convert to bool
  operator FXbool() const { return toBool(); }

  /// Convert to pointer
  operator FXptr() const { return toPtr(); }

  /// Convert to char
  operator FXchar() const { return toInt(); }

  /// Convert to char
  operator FXuchar() const { return toUInt(); }

  /// Convert to short
  operator FXshort() const { return toInt(); }

  /// Convert to unsigned short
  operator FXushort() const { return toUInt(); }

  /// Convert to int
  operator FXint() const { return toInt(); }

  /// Convert to unsigned int
  operator FXuint() const { return toUInt(); }

  /// Convert to long
  operator FXlong() const { return toLong(); }

  /// Convert to unsigned long
  operator FXulong() const { return toULong(); }

  /// Convert to float
  operator FXfloat() const { return toFloat(); }

  /// Convert to double
  operator FXdouble() const { return toDouble(); }

  /// Convert to string
  operator FXString() const { return toString(); }

  /// Assign with bool
  FXVariant& operator=(FXbool val);

  /// Assign with char
  FXVariant& operator=(FXchar val);

  /// Assign with int
  FXVariant& operator=(FXint val);

  /// Assign with unsigned int
  FXVariant& operator=(FXuint val);

  /// Assign with long
  FXVariant& operator=(FXlong val);

  /// Assign with unsigned long
  FXVariant& operator=(FXulong val);

  /// Assign with float
  FXVariant& operator=(FXfloat val);

  /// Assign with double
  FXVariant& operator=(FXdouble val);

  /// Assign with pointer
  FXVariant& operator=(FXptr val);

  /// Assign with constant string
  FXVariant& operator=(const FXchar* val);

  /// Assign with string
  FXVariant& operator=(const FXString& val);

  /// Assign with variant
  FXVariant& operator=(const FXVariant& val);

  /// Assign with variant
  FXVariant& assign(const FXVariant& other);

  /// Adopt variant from another
  FXVariant& adopt(FXVariant& other);

  /// Return value of object member
  FXVariant& at(const FXchar* key);

  /// Return value of object member
  const FXVariant& at(const FXchar* key) const;

  /// Return value of object member
  FXVariant& operator[](const FXchar* key){ return at(key); }

  /// Return value of object member
  const FXVariant& operator[](const FXchar* key) const { return at(key); }

  /// Return value of object member
  FXVariant& at(const FXString& key);

  /// Return value of object member
  const FXVariant& at(const FXString& key) const;

  /// Return value of object member
  FXVariant& operator[](const FXString& key){ return at(key); }

  /// Return value of object member
  const FXVariant& operator[](const FXString& key) const { return at(key); }

  /// Return value of array member
  FXVariant& at(FXival idx);

  /// Return value of array member
  const FXVariant& at(FXival idx) const;

  /// Return value of array member
  FXVariant& operator[](FXint idx){ return at(idx); }
  const FXVariant& operator[](FXint idx) const { return at(idx); }

  /// Return value of array member
  FXVariant& operator[](FXival idx){ return at(idx); }
  const FXVariant& operator[](FXival idx) const { return at(idx); }

  /// Check if key is mapped
  FXbool has(const FXchar* key) const;

  /// Check if key is mapped
  FXbool has(const FXString& key) const { return has(key.text()); }

  /// Return the value of the variant as a pointer; variant type MUST be PointerType
  FXptr& asPtr(){ return value.p; }

  /// Return the value of the variant as a pointer; variant type MUST be PointerType
  const FXptr& asPtr() const { return value.p; }

  /// Return the value of the variant as a long; variant type MUST be LongType
  FXlong& asLong(){ return value.i; }

  /// Return the value of the variant as a long; variant type MUST be LongType
  const FXlong& asLong() const { return value.i; }

  /// Return the value of the variant as an unsigned long; variant type MUST be ULongType
  FXulong& asULong(){ return value.u; }

  /// Return the value of the variant as an unsigned long; variant type MUST be ULongType
  const FXulong& asULong() const { return value.u; }

  /// Return the value of the variant as a double; variant type MUST be DoubleType
  FXdouble& asDouble(){ return value.d; }

  /// Return the value of the variant as a double; variant type MUST be DoubleType
  const FXdouble& asDouble() const { return value.d; }

  /// Return the value of the variant as a char pointer; variant type MUST be StringType
  const FXchar* asChars() const { return value.s; }

  /// Return the value of the variant as a string-reference; variant type MUST be StringType
  FXString& asString(){ return *reinterpret_cast<FXString*>(&value.p); }

  /// Return the value of the variant as a const string-reference; variant type MUST be StringType
  const FXString& asString() const { return *reinterpret_cast<const FXString*>(&value.p); }

  /// Return the value of the variant as an array-reference; variant type MUST be ArrayType
  FXVariantArray& asArray(){ return *reinterpret_cast<FXVariantArray*>(&value.p); }

  /// Return the value of the variant as a const array-reference; variant type MUST be ArrayType
  const FXVariantArray& asArray() const { return *reinterpret_cast<const FXVariantArray*>(&value.p); }

  /// Return the value of the variant as an map-reference; variant type MUST be MapType
  FXVariantMap& asMap(){ return *reinterpret_cast<FXVariantMap*>(&value.p); }

  /// Return the value of the variant as a const map-reference; variant type MUST be MapType
  const FXVariantMap& asMap() const { return *reinterpret_cast<const FXVariantMap*>(&value.p); }

  /// Remove variant at key from map
  FXbool remove(const FXchar* key);

  /// Remove variant at key from map
  FXbool remove(const FXString& key){ return remove(key.text()); }

  /// Erase variant at idx from array
  FXbool erase(FXival idx);

  /// Clear the data
  FXbool clear();

  /// Default constant variant
  static const FXVariant null;

  /// Destroy
 ~FXVariant();
  };

}

#endif
