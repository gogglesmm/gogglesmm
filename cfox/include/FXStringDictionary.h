/********************************************************************************
*                                                                               *
*                  S t r i n g   D i c t i o n a r y    C l a s s               *
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
#ifndef FXSTRINGDICTIONARY_H
#define FXSTRINGDICTIONARY_H

namespace FX {


/**
* The dictionary class maintains a fast-access hash table of entities
* indexed by a character string.
*/
class FXAPI FXStringDictionary {
protected:
  struct Entry {
    FXString key;       // Key
    FXString data;      // Value
    FXuint   hash;      // Hash of key
    FXuint   mark;      // Mark flag
    };
protected:
  Entry*     table;     // Hash table
protected:

  // Change size of the table
  FXbool no(FXival n);

  // Change number of used entries
  void used(FXival u){ ((FXival*)table)[-2]=u; }

  // Change number of free entries
  void free(FXival f){ ((FXival*)table)[-3]=f; }

  // Resize the table to the given size, keeping contents
  FXbool resize(FXival n);
public:

  /// Construct empty string dictionary
  FXStringDictionary();

  /// Construct from another string dictionary
  FXStringDictionary(const FXStringDictionary& other);

  /// Return the size of the table, including the empty slots
  FXival no() const { return ((FXival*)table)[-1]; }

  /// Return number of used slots in the table
  FXival used() const { return ((FXival*)table)[-2]; }

  /// Return number of free slots in the table
  FXival free() const { return ((FXival*)table)[-3]; }

  /// See if map is empty
  FXbool empty() const { return ((FXival*)table)[-1]<=1; }

  /// Assignment operator
  FXStringDictionary& operator=(const FXStringDictionary& other);

  /// Adopt string dictionary from another
  FXStringDictionary& adopt(FXStringDictionary& other);

  /// Find position of given key, returning -1 if not found
  FXival find(const FXchar* ky) const;

  /// Find position of given key, returning -1 if not found
  FXival find(const FXString& ky) const { return find(ky.text()); }

  /// Check if key is mapped
  FXbool has(const FXchar* ky) const { return 0<=find(ky); }

  /// Check if key is mapped
  FXbool has(const FXString& ky) const { return has(ky.text()); }

  /// Return reference to slot assocated with given key
  FXString& at(const FXchar* ky,FXbool mrk=false);

  /// Return constant reference to slot assocated with given key
  const FXString& at(const FXchar* ky) const;

  /// Return reference to slot assocated with given key
  FXString& at(const FXString& ky,FXbool mrk=false){ return at(ky.text(),mrk); }

  /// Return constant reference to slot assocated with given key
  const FXString& at(const FXString& ky) const { return at(ky.text()); }

  /// Return reference to slot assocated with given key
  FXString& operator[](const FXchar* ky){ return at(ky,false); }

  /// Return constant reference to slot assocated with given key
  const FXString& operator[](const FXchar* ky) const { return at(ky); }

  /// Return reference to slot assocated with given key
  FXString& operator[](const FXString& ky){ return at(ky,false); }

  /// Return constant reference to slot assocated with given key
  const FXString& operator[](const FXString& ky) const { return at(ky); }

  /// Insert association with given key; return reference to the string
  FXbool insert(const FXchar* ky,const FXchar* str,FXbool mrk=false){ at(ky,mrk)=str; return true; }

  /// Insert association with given key; return reference to the string
  FXbool insert(const FXString& ky,const FXchar* str,FXbool mrk=false){ at(ky,mrk)=str; return true; }

  /// Insert association with given key; return reference to the string
  FXbool insert(const FXString& ky,const FXString& str,FXbool mrk=false){ at(ky,mrk)=str; return true; }

  /// Remove association with given key
  FXbool remove(const FXchar* ky);

  /// Remove association with given key
  FXbool remove(const FXString& ky){ return remove(ky.text()); }

  /// Erase data at pos in the table
  FXbool erase(FXival pos);

  /// Return true if slot is empty.
  FXbool empty(FXival pos) const { return table[pos].key.empty(); }

  /// Return key at position pos
  const FXString& key(FXival pos) const { return table[pos].key; }

  /// Return reference to slot at position pos
  FXString& data(FXival pos){ return table[pos].data; }

  /// Return constant reference to slot at position pos
  const FXString& data(FXival pos) const { return table[pos].data; }

  /// Return mark flag of entry at position pos
  FXuint mark(FXival pos) const { return table[pos].mark; }

  /// Clear entire table
  FXbool clear();

  /// Destroy table
 ~FXStringDictionary();
  };


}

#endif
