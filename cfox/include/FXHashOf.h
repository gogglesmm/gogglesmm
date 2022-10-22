/********************************************************************************
*                                                                               *
*         H a s h   T a b l e   O f   P o i n t e r s   T o   T y p e           *
*                                                                               *
*********************************************************************************
* Copyright (C) 2003,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#ifndef FXHASHOF_H
#define FXHASHOF_H

namespace FX {


/**
* A hash table mapping pointers to KEYTYPE to pointers of VALUETYPE.
* Two special key values are disallowed: NULL and the pointer value (-1L);
* NULL is used to designate an unoccupied slot, while (-1L) is used to designate
* a formerly occupied slot.
*/
template<typename KEYTYPE,typename VALUETYPE>
class FXHashOf : public FXHash {
public:

  /// Default constructor
  FXHashOf(){}

  /// Copy constructor
  FXHashOf(const FXHashOf<KEYTYPE,VALUETYPE>& other):FXHash(other){}

  /// Assignment operator
  FXHashOf<KEYTYPE,VALUETYPE>& operator=(const FXHashOf<KEYTYPE,VALUETYPE>& other){ return reinterpret_cast<FXHashOf<KEYTYPE,VALUETYPE>&>(FXHash::operator=(other)); }

  /// Adopt objects from orig, leaving orig empty
  FXHashOf<KEYTYPE,VALUETYPE>& adopt(FXHashOf<KEYTYPE,VALUETYPE>& other){ return reinterpret_cast<FXHashOf<KEYTYPE,VALUETYPE>&>(FXHash::adopt(other)); }

  /// Find position of given key, returning -1 if not found.
  FXival find(KEYTYPE* ky) const { return FXHash::find(ky); }

  /// Check if key is mapped.
  FXbool has(KEYTYPE* ky) const { return FXHash::has(ky); }

  /// Return reference to slot assocated with given key
  VALUETYPE*& at(KEYTYPE* ky){ return reinterpret_cast<VALUETYPE*&>(FXHash::at(ky)); }

  /// Return constant reference to slot assocated with given key
  VALUETYPE *const& at(KEYTYPE* ky) const { return reinterpret_cast<VALUETYPE *const&>(FXHash::at(ky)); }

  /// Return reference to slot assocated with given key
  VALUETYPE*& operator[](KEYTYPE* ky){ return reinterpret_cast<VALUETYPE*&>(FXHash::at(ky)); }

  /// Return constant reference to slot assocated with given key
  VALUETYPE *const& operator[](KEYTYPE* ky) const { return reinterpret_cast<VALUETYPE *const&>(FXHash::at(ky)); }

  /// Insert association with given key; return old value, if any
  VALUETYPE* insert(KEYTYPE* ky,VALUETYPE* ptr=nullptr){ return reinterpret_cast<VALUETYPE*>(FXHash::insert(ky,ptr)); }

  /// Remove association with given key; return old value, if any
  VALUETYPE* remove(KEYTYPE* ky){ return reinterpret_cast<VALUETYPE*>(FXHash::remove(ky)); }

  /// Erase data at pos in the table; return old value, if any
  VALUETYPE* erase(FXival pos){ return reinterpret_cast<VALUETYPE*>(FXHash::erase(pos)); }

  /// Return key at position pos
  KEYTYPE* key(FXival pos) const { return reinterpret_cast<KEYTYPE*>(FXHash::key(pos)); }

  /// Return reference to slot at position pos
  VALUETYPE*& data(FXival pos){ return reinterpret_cast<VALUETYPE*&>(FXHash::data(pos)); }

  /// Return constant reference to slot at position pos
  VALUETYPE *const& data(FXival pos) const { return reinterpret_cast<VALUETYPE *const&>(FXHash::data(pos)); }
  };

}

#endif
