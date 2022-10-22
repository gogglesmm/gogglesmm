/********************************************************************************
*                                                                               *
*  R e v e r s e   D i c t i o n a r y   O f   P o i n t e r s   T o   T y p e  *
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
#ifndef REVERSEDICTIONARYOF_H
#define REVERSEDICTIONARYOF_H

namespace FX {


/**
* Dictionary of pointers to TYPE.
* Two special key values are disallowed: NULL and the pointer value (-1L); NULL is
* used to designate an unoccupied slot, while (-1L) is used to designate a formerly
* occupied slot.
*/
template<typename TYPE>
class FXReverseDictionaryOf : public FXReverseDictionary {
public:

  /// Default constructor
  FXReverseDictionaryOf(){}

  /// Copy constructor
  FXReverseDictionaryOf(const FXReverseDictionaryOf<TYPE>& other):FXReverseDictionary(other){ }

  /// Assignment operator
  FXReverseDictionaryOf<TYPE>& operator=(const FXReverseDictionaryOf<TYPE>& other){ return reinterpret_cast<FXReverseDictionaryOf<TYPE>&>(FXReverseDictionary::operator=(other)); }

  /// Adopt reverse dictionary from another
  FXReverseDictionaryOf<TYPE>& adopt(FXReverseDictionaryOf<TYPE>& other){ return reinterpret_cast<FXReverseDictionaryOf<TYPE>&>(FXReverseDictionary::adopt(other)); }

  /// Find position of given key, returning -1 if not found
  FXival find(TYPE* ky) const { return FXReverseDictionary::find(ky); }

  /// Check if key is mapped
  FXbool has(TYPE* ky) const { return FXReverseDictionary::has(ky); }

  /// Return reference to slot assocated with given key
  FXString& at(TYPE* ky){ return FXReverseDictionary::at(ky); }

  /// Return constant reference to slot assocated with given key
  const FXString& at(TYPE* ky) const { return FXReverseDictionary::at(ky); }

  /// Return reference to slot assocated with given key
  FXString& operator[](TYPE* ky){ return FXReverseDictionary::at(ky); }

  /// Return constant reference to slot assocated with given key
  const FXString& operator[](TYPE* ky) const { return FXReverseDictionary::at(ky); }

  /// Insert association with given key; return old value, if any
  FXString insert(TYPE* ky,const FXString& str=FXString::null){ return FXReverseDictionary::insert(ky,str); }

  /// Remove association with given key; return old value, if any
  FXString remove(TYPE* ky){ return FXReverseDictionary::remove(ky); }

  /// Erase data at pos in the table; return old value, if any
  FXString erase(FXival pos){ return FXReverseDictionary::erase(pos); }

  /// Return key at position pos
  TYPE* key(FXival pos) const { return reinterpret_cast<TYPE*&>(FXReverseDictionary::key(pos)); }

  /// Return reference to slot at position pos
  FXString& data(FXival pos){ return FXReverseDictionary::data(pos); }

  /// Return constant reference to slot at position pos
  const FXString& data(FXival pos) const { return FXReverseDictionary::data(pos); }
  };

}

#endif
