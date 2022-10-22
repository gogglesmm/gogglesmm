/********************************************************************************
*                                                                               *
*                              V a r i a n t - M a p                            *
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
#ifndef FXVARIANTMAP_H
#define FXVARIANTMAP_H

namespace FX {


/**
* Variant map associates strings to variants using fast hash
* table.
*/
class FXAPI FXVariantMap {
protected:
  struct Entry {
    FXString  key;      // Lookup key
    FXVariant data;     // Variant data
    FXuint    hash;     // Hash of key
    };
protected:
  Entry      *table;    // Hash table
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

  /// Construct an empty map
  FXVariantMap();

  /// Construct from another map
  FXVariantMap(const FXVariantMap& other);

  /// Return the size of the table, including the empty slots
  FXival no() const { return ((FXival*)table)[-1]; }

  /// Return number of used slots in the table
  FXival used() const { return ((FXival*)table)[-2]; }

  /// Return number of free slots in the table
  FXival free() const { return ((FXival*)table)[-3]; }

  /// See if map is empty
  FXbool empty() const { return ((FXival*)table)[-1]<=1; }

  /// Assignment operator
  FXVariantMap& operator=(const FXVariantMap& other);

  /// Adopt map from another map; the other map becomes empty
  FXVariantMap& adopt(FXVariantMap& other);

  /// Find slot index for key; return -1 if not found
  FXival find(const FXchar* ky) const;

  /// Find slot index for key; return -1 if not found
  FXival find(const FXString& ky) const { return find(ky.text()); }

  /// Check if key is mapped
  FXbool has(const FXchar* ky) const { return 0<=find(ky); }

  /// Check if key is mapped
  FXbool has(const FXString& ky) const { return has(ky.text()); }

  /// Return reference to variant assocated with key
  FXVariant& at(const FXchar* ky);

  /// Return constant reference to variant assocated with key
  const FXVariant& at(const FXchar* ky) const;

  /// Return reference to variant assocated with key
  FXVariant& at(const FXString& ky){ return at(ky.text()); }

  /// Return constant reference to variant assocated with key
  const FXVariant& at(const FXString& ky) const { return at(ky.text()); }

  /// Return reference to variant assocated with key
  FXVariant& operator[](const FXchar* ky){ return at(ky); }

  /// Return constant reference to variant assocated with key
  const FXVariant& operator[](const FXchar* ky) const { return at(ky); }

  /// Return reference to variant assocated with key
  FXVariant& operator[](const FXString& ky){ return at(ky); }

  /// Return constant reference to variant assocated with key
  const FXVariant& operator[](const FXString& ky) const { return at(ky); }

  /// Remove entry from the table
  FXbool remove(const FXchar* ky);

  /// Remove entry from the table
  FXbool remove(const FXString& ky){ return remove(ky.text()); }

  /// Erase entry at pos in the table
  FXbool erase(FXival pos);

  /// Return true if slot at pos is empty.
  FXbool empty(FXival pos) const { return table[pos].key.empty(); }

  /// Return key value at slot pos; may be empty!
  const FXString& key(FXival pos) const { return table[pos].key; }

  /// Return reference to data at slot s; but careful as assignment
  /// to empty slot is dangerous!!
  FXVariant& data(FXival pos){ return table[pos].data; }

  /// Return value at slot pos; may be empty!
  const FXVariant& data(FXival pos) const { return table[pos].data; }

  /// Clear the table
  FXbool clear();

  /// Destructor
 ~FXVariantMap();
  };


}

#endif
