/********************************************************************************
*                                                                               *
*         D i c t i o n a r y   O f   P o i n t e r s   T o   T y p e           *
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
#ifndef FXDICTIONARYOF_H
#define FXDICTIONARYOF_H

namespace FX {


/**
* Dictionary of pointers to TYPE.
*/
template<typename TYPE>
class FXDictionaryOf : public FXDictionary {
public:

  /// Default constructor
  FXDictionaryOf(){}

  /// Copy constructor
  FXDictionaryOf(const FXDictionaryOf<TYPE>& other):FXDictionary(other){ }

  /// Assignment operator
  FXDictionaryOf<TYPE>& operator=(const FXDictionaryOf<TYPE>& other){ return reinterpret_cast<FXDictionaryOf<TYPE>&>(FXDictionary::operator=(other)); }

  /// Adopt dictionary from another
  FXDictionaryOf<TYPE>& adopt(FXDictionaryOf<TYPE>& other){ return reinterpret_cast<FXDictionaryOf<TYPE>&>(FXDictionary::adopt(other)); }

  /// Return reference to slot assocated with given key
  TYPE*& at(const FXchar* ky){ return reinterpret_cast<TYPE*&>(FXDictionary::at(ky)); }

  /// Return constant reference to slot assocated with given key
  TYPE *const& at(const FXchar* ky) const { return reinterpret_cast<TYPE *const&>(FXDictionary::at(ky)); }

  /// Return reference to slot assocated with given key
  TYPE*& at(const FXString& ky){ return reinterpret_cast<TYPE*&>(FXDictionary::at(ky.text())); }

  /// Return constant reference to slot assocated with given key
  TYPE *const& at(const FXString& ky) const { return reinterpret_cast<TYPE *const&>(FXDictionary::at(ky.text())); }

  /// Return reference to slot assocated with given key
  TYPE*& operator[](const FXchar* ky){ return reinterpret_cast<TYPE*&>(FXDictionary::at(ky)); }

  /// Return constant reference to slot assocated with given key
  TYPE *const& operator[](const FXchar* ky) const { return reinterpret_cast<TYPE *const&>(FXDictionary::at(ky)); }

  /// Return reference to slot assocated with given key
  TYPE*& operator[](const FXString& ky){ return reinterpret_cast<TYPE*&>(FXDictionary::at(ky.text())); }

  /// Return constant reference to slot assocated with given key
  TYPE *const& operator[](const FXString& ky) const { return reinterpret_cast<TYPE *const&>(FXDictionary::at(ky.text())); }

  /// Insert association with given key; return old value, if any
  TYPE* insert(const FXchar* ky,TYPE* ptr=nullptr){ return reinterpret_cast<TYPE*>(FXDictionary::insert(ky,ptr)); }

  /// Insert association with given key; return old value, if any
  TYPE* insert(const FXString& ky,TYPE* ptr=nullptr){ return insert(ky.text(),ptr); }

  /// Remove association with given key; return old value, if any
  TYPE* remove(const FXchar* ky){ return reinterpret_cast<TYPE*>(FXDictionary::remove(ky)); }

  /// Remove association with given key; return old value, if any
  TYPE* remove(const FXString& ky){ return reinterpret_cast<TYPE*>(FXDictionary::remove(ky.text())); }

  /// Erase data at pos in the table; return old value, if any
  TYPE* erase(FXival pos){ return reinterpret_cast<TYPE*>(FXDictionary::erase(pos)); }

  /// Return reference to slot at position pos
  TYPE*& data(FXival pos){ return reinterpret_cast<TYPE*&>(FXDictionary::data(pos)); }

  /// Return constant reference to slot at position pos
  TYPE *const& data(FXival pos) const { return reinterpret_cast<TYPE *const&>(FXDictionary::data(pos)); }
  };

}

#endif
