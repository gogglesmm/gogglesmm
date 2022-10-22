/********************************************************************************
*                                                                               *
*                          D i c t i o n a r y    C l a s s                     *
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
#ifndef FXDICTIONARY_H
#define FXDICTIONARY_H

namespace FX {


/**
* The dictionary class is a fast-access hash table, mapping strings to void-pointers.
* Subclasses of dictionary can easily specialize the void-pointers to pointers to
* particular types; to this end subclasses must overload certain API's and
* perform the necessary type-casts for the proper interpretation of the stored
* pointer values.
* Note that many complex containers in FOX now fit inside a pointer, and thus
* these types can be used in dictionaries as well!
*/
class FXAPI FXDictionary {
protected:
  struct Entry {
    FXString key;       // Key
    void*    data;      // Value
    FXuint   hash;      // Hash of key
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

  /// Construct empty dictionary
  FXDictionary();

  /// Construct from another dictionary
  FXDictionary(const FXDictionary& other);

  /// Return the size of the table, including the empty slots
  FXival no() const { return ((FXival*)table)[-1]; }

  /// Return number of used slots in the table
  FXival used() const { return ((FXival*)table)[-2]; }

  /// Return number of free slots in the table
  FXival free() const { return ((FXival*)table)[-3]; }

  /// See if map is empty
  FXbool empty() const { return ((FXival*)table)[-1]<=1; }

  /// Assignment operator
  FXDictionary& operator=(const FXDictionary& other);

  /// Adopt dictionary from another
  FXDictionary& adopt(FXDictionary& other);

  /// Find position of given key, returning -1 if not found
  FXival find(const FXchar* ky) const;

  /// Find position of given key, returning -1 if not found
  FXival find(const FXString& ky) const { return find(ky.text()); }

  /// Check if key is mapped
  FXbool has(const FXchar* ky) const { return 0<=find(ky); }

  /// Check if key is mapped
  FXbool has(const FXString& ky) const { return has(ky.text()); }

  /// Return reference to slot assocated with given key
  void*& at(const FXchar* ky);

  /// Return constant reference to slot assocated with given key
  void *const& at(const FXchar* ky) const;

  /// Return reference to slot assocated with given key
  void*& at(const FXString& ky){ return at(ky.text()); }

  /// Return constant reference to slot assocated with given key
  void *const& at(const FXString& ky) const { return at(ky.text()); }

  /// Return reference to slot assocated with given key
  void*& operator[](const FXchar* ky){ return at(ky); }

  /// Return constant reference to slot assocated with given key
  void *const& operator[](const FXchar* ky) const { return at(ky); }

  /// Return reference to slot assocated with given key
  void*& operator[](const FXString& ky){ return at(ky); }

  /// Return constant reference to slot assocated with given key
  void *const& operator[](const FXString& ky) const { return at(ky); }

  /// Insert association with given key; return old value, if any
  void* insert(const FXchar* ky,void* ptr=nullptr){ return swap(ptr,at(ky)); }

  /// Insert association with given key; return old value, if any
  void* insert(const FXString& ky,void* ptr=nullptr){ return swap(ptr,at(ky)); }

  /// Remove association with given key; return old value, if any
  void* remove(const FXchar* ky);

  /// Remove association with given key; return old value, if any
  void* remove(const FXString& ky){ return remove(ky.text()); }

  /// Erase data at pos in the table; return old value, if any
  void* erase(FXival pos);

  /// Return true if slot is empty.
  FXbool empty(FXival pos) const { return table[pos].key.empty(); }

  /// Return key at position pos
  const FXString& key(FXival pos) const { return table[pos].key; }

  /// Return reference to slot at position pos
  void*& data(FXival pos){ return table[pos].data; }

  /// Return constant reference to slot at position pos
  void *const& data(FXival pos) const { return table[pos].data; }

  /// Clear entire table
  FXbool clear();

  /// Destroy table
 ~FXDictionary();
  };

}

#endif
