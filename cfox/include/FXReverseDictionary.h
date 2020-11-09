/********************************************************************************
*                                                                               *
*                 R e v e r s e   D i c t i o n a r y    C l a s s              *
*                                                                               *
*********************************************************************************
* Copyright (C) 2018,2020 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#ifndef REVERSEDICTIONARY_H
#define REVERSEDICTIONARY_H

namespace FX {


/**
* The reverse dictionary class is a fast-access hash table, mapping void-pointers to
* strings. Subclasses of dictionary can easily specialize the void-pointers to pointers
* to particular types; to this end subclasses must overload certain API's and
* perform the necessary type-casts for the proper interpretation of the stored
* pointer values.
* Two special key values are disallowed: NULL and the pointer value (-1L); NULL is
* used to designate an unoccupied slot, while (-1L) is used to designate a formerly
* occupied slot.
* Note that many complex containers in TL now fit inside a pointer, and thus
* these types can be used in dictionaries as well!
*/
class FXAPI FXReverseDictionary {
protected:
  struct Entry {
    FXptr    key;       // Key
    FXString data;      // Value
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
  FXReverseDictionary();

  /// Construct from another dictionary
  FXReverseDictionary(const FXReverseDictionary& other);

  /// Return the size of the table, including the empty slots
  FXival no() const { return ((FXival*)table)[-1]; }

  /// Return number of used slots in the table
  FXival used() const { return ((FXival*)table)[-2]; }

  /// Return number of free slots in the table
  FXival free() const { return ((FXival*)table)[-3]; }

  /// See if map is empty
  FXbool empty() const { return ((FXival*)table)[-1]<=1; }

  /// Assignment operator
  FXReverseDictionary& operator=(const FXReverseDictionary& other);

  /// Adopt dictionary from another
  FXReverseDictionary& adopt(FXReverseDictionary& other);

  /// Find position of given key, returning -1 if not found
  FXival find(FXptr ky) const;

  /// Check if key is mapped
  FXbool has(FXptr ky) const { return 0<=find(ky); }

  /// Return reference to slot assocated with given key
  FXString& at(FXptr ky);

  /// Return constant reference to slot assocated with given key
  const FXString& at(FXptr ky) const;

  /// Return reference to slot assocated with given key
  FXString& operator[](FXptr ky){ return at(ky); }

  /// Return constant reference to slot assocated with given key
  const FXString& operator[](FXptr ky) const { return at(ky); }

  /// Insert association with given key; return old value, if any
  FXString insert(FXptr ky,const FXString& str=FXString::null){ FXString ret(str); return swap(at(ky),ret); }

  /// Remove association with given key; return old value, if any
  FXString remove(FXptr ky);

  /// Erase data at pos in the table; return old value, if any
  FXString erase(FXival pos);

  /// Return true if slot is empty.
  FXbool empty(FXival pos) const { return (table[pos].key==(FXptr)0L)||(table[pos].key==(FXptr)-1L); }

  /// Return key at position pos
  FXptr key(FXival pos) const { return table[pos].key; }

  /// Return reference to slot at position pos
  FXString& data(FXival pos){ return table[pos].data; }

  /// Return constant reference to slot at position pos
  const FXString& data(FXival pos) const { return table[pos].data; }

  /// Clear entire table
  void clear();

  /// Destroy table
 ~FXReverseDictionary();
  };

}

#endif
