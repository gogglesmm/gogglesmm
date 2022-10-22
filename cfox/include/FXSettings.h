/********************************************************************************
*                                                                               *
*                          S e t t i n g s   C l a s s                          *
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
#ifndef FXSETTINGS_H
#define FXSETTINGS_H

namespace FX {


/**
* The Settings class manages a key-value database.  This is normally used as
* part of Registry, but can also be used separately in applications that need
* to maintain a key-value database in a file of their own.
* String values can contain any character, and will be escaped when written
* to the file.
*/
class FXAPI FXSettings {
protected:
  struct Entry {
    FXString           key;             // Key
    FXStringDictionary data;            // Value
    FXuint             hash;            // Hash of key
    };
protected:
  Entry               *table;           // Hash table
  FXbool               modified;        // Changed
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

  /// Construct settings database.
  FXSettings();

  /// Construct from another settings database
  FXSettings(const FXSettings& other);

  /// Return the size of the table, including the empty slots
  FXival no() const { return ((FXival*)table)[-1]; }

  /// Return number of used slots in the table
  FXival used() const { return ((FXival*)table)[-2]; }

  /// Return number of free slots in the table
  FXival free() const { return ((FXival*)table)[-3]; }

  /// See if map is empty
  FXbool empty() const { return ((FXival*)table)[-1]<=1; }

  /// Assignment operator
  FXSettings& operator=(const FXSettings& other);

  /// Adopt string dictionary from another
  FXSettings& adopt(FXSettings& other);

  /// Is it modified
  FXbool isModified() const { return modified; }

  /// Mark as changed
  void setModified(FXbool mdfy=true){ modified=mdfy; }

  /// Parse a file containing a settings database.
  FXbool parseFile(const FXString& filename,FXbool mrk=true);

  /// Unparse settings database into given file.
  FXbool unparseFile(const FXString& filename);

  /// Parse single string to populate settings
  FXbool parse(const FXString& string,FXbool mrk=true);

  /// Unparse settings to a single string
  FXbool unparse(FXString& string) const;

  /// Find position of given key, returning -1 if not found
  FXival find(const FXchar* ky) const;

  /// Find position of given key, returning -1 if not found
  FXival find(const FXString& ky) const { return find(ky.text()); }

  /// Return reference to slot assocated with given key
  FXStringDictionary& at(const FXchar* ky);

  /// Return constant reference to slot assocated with given key
  const FXStringDictionary& at(const FXchar* ky) const;

  /// Return reference to slot assocated with given key
  FXStringDictionary& at(const FXString& ky){ return at(ky.text()); }

  /// Return constant reference to slot assocated with given key
  const FXStringDictionary& at(const FXString& ky) const { return at(ky.text()); }

  /// Return reference to slot assocated with given key
  FXStringDictionary& operator[](const FXchar* ky){ return at(ky); }

  /// Return constant reference to slot assocated with given key
  const FXStringDictionary& operator[](const FXchar* ky) const { return at(ky); }

  /// Return reference to slot assocated with given key
  FXStringDictionary& operator[](const FXString& ky){ return at(ky); }

  /// Return constant reference to slot assocated with given key
  const FXStringDictionary& operator[](const FXString& ky) const { return at(ky); }

  /// Return true if slot is empty.
  FXbool empty(FXival pos) const { return table[pos].key.empty(); }

  /// Return key at position pos
  const FXString& key(FXival pos) const { return table[pos].key; }

  /// Return reference to slot at position pos
  FXStringDictionary& data(FXival pos){ return table[pos].data; }

  /// Return constant reference to slot at position pos
  const FXStringDictionary& data(FXival pos) const { return table[pos].data; }

  /// Read a formatted registry entry, using scanf-style format
  FXint readFormatEntry(const FXchar* section,const FXchar* name,const FXchar* fmt,...) const FX_SCANF(4,5) ;
  FXint readFormatEntry(const FXString& section,const FXchar* name,const FXchar* fmt,...) const FX_SCANF(4,5) ;
  FXint readFormatEntry(const FXString& section,const FXString& name,const FXchar* fmt,...) const FX_SCANF(4,5) ;

  /// Write a formatted registry entry, using printf-style format
  FXint writeFormatEntry(const FXchar* section,const FXchar* name,const FXchar* fmt,...) FX_PRINTF(4,5) ;
  FXint writeFormatEntry(const FXString& section,const FXchar* name,const FXchar* fmt,...) FX_PRINTF(4,5) ;
  FXint writeFormatEntry(const FXString& section,const FXString& name,const FXchar* fmt,...) FX_PRINTF(4,5) ;

  /// Read a string registry entry; if no value is found, the default value def is returned
  const FXchar* readStringEntry(const FXchar* section,const FXchar* name,const FXchar* def=nullptr) const;
  const FXchar* readStringEntry(const FXString& section,const FXchar* name,const FXchar* def=nullptr) const;
  const FXchar* readStringEntry(const FXString& section,const FXString& name,const FXchar* def=nullptr) const;

  /// Write a string registry entry
  FXbool writeStringEntry(const FXchar* section,const FXchar* name,const FXchar* val);
  FXbool writeStringEntry(const FXString& section,const FXchar *name,const FXchar* val);
  FXbool writeStringEntry(const FXString& section,const FXString& name,const FXchar* val);

  /// Read a integer registry entry; if no value is found, the default value def is returned
  FXint readIntEntry(const FXchar* section,const FXchar* name,FXint def=0) const;
  FXint readIntEntry(const FXString& section,const FXchar* name,FXint def=0) const;
  FXint readIntEntry(const FXString& section,const FXString& name,FXint def=0) const;

  /// Write a integer registry entry
  FXbool writeIntEntry(const FXchar* section,const FXchar* name,FXint val);
  FXbool writeIntEntry(const FXString& section,const FXchar* name,FXint val);
  FXbool writeIntEntry(const FXString& section,const FXString& name,FXint val);

  /// Read a unsigned integer registry entry; if no value is found, the default value def is returned
  FXuint readUIntEntry(const FXchar* section,const FXchar* name,FXuint def=0) const;
  FXuint readUIntEntry(const FXString& section,const FXchar* name,FXuint def=0) const;
  FXuint readUIntEntry(const FXString& section,const FXString& name,FXuint def=0) const;

  /// Write a unsigned integer registry entry
  FXbool writeUIntEntry(const FXchar* section,const FXchar* name,FXuint val);
  FXbool writeUIntEntry(const FXString& section,const FXchar* name,FXuint val);
  FXbool writeUIntEntry(const FXString& section,const FXString& name,FXuint val);

  /// Read a 64-bit long integer registry entry; if no value is found, the default value def is returned
  FXlong readLongEntry(const FXchar* section,const FXchar* name,FXlong def=0) const;
  FXlong readLongEntry(const FXString& section,const FXchar* name,FXlong def=0) const;
  FXlong readLongEntry(const FXString& section,const FXString& name,FXlong def=0) const;

  /// Write a 64-bit long integer registry entry
  FXbool writeLongEntry(const FXchar* section,const FXchar* name,FXlong val);
  FXbool writeLongEntry(const FXString& section,const FXchar* name,FXlong val);
  FXbool writeLongEntry(const FXString& section,const FXString& name,FXlong val);

  /// Read a 64-bit unsigned long integer registry entry; if no value is found, the default value def is returned
  FXulong readULongEntry(const FXchar* section,const FXchar* name,FXulong def=0) const;
  FXulong readULongEntry(const FXString& section,const FXchar* name,FXulong def=0) const;
  FXulong readULongEntry(const FXString& section,const FXString& name,FXulong def=0) const;

  /// Write a 64-bit unsigned long integer registry entry
  FXbool writeULongEntry(const FXchar* section,const FXchar* name,FXulong val);
  FXbool writeULongEntry(const FXString& section,const FXchar* name,FXulong val);
  FXbool writeULongEntry(const FXString& section,const FXString& name,FXulong val);

  /// Read a double-precision floating point registry entry; if no value is found, the default value def is returned
  FXdouble readRealEntry(const FXchar* section,const FXchar* name,FXdouble def=0.0) const;
  FXdouble readRealEntry(const FXString& section,const FXchar* name,FXdouble def=0.0) const;
  FXdouble readRealEntry(const FXString& section,const FXString& name,FXdouble def=0.0) const;

  /// Write a double-precision floating point registry entry
  FXbool writeRealEntry(const FXchar* section,const FXchar* name,FXdouble val);
  FXbool writeRealEntry(const FXString& section,const FXchar* name,FXdouble val);
  FXbool writeRealEntry(const FXString& section,const FXString& name,FXdouble val);

  /// Read a color value registry entry; if no value is found, the default value def is returned
  FXColor readColorEntry(const FXchar* section,const FXchar* name,FXColor def=0) const;
  FXColor readColorEntry(const FXString& section,const FXchar* name,FXColor def=0) const;
  FXColor readColorEntry(const FXString& section,const FXString& name,FXColor def=0) const;

  /// Write a color value entry
  FXbool writeColorEntry(const FXchar* section,const FXchar* name,FXColor val);
  FXbool writeColorEntry(const FXString& section,const FXchar* name,FXColor val);
  FXbool writeColorEntry(const FXString& section,const FXString& name,FXColor val);

  /// Read a boolean registry entry
  FXbool readBoolEntry(const FXchar* section,const FXchar* name,FXbool def=false) const;
  FXbool readBoolEntry(const FXString& section,const FXchar* name,FXbool def=false) const;
  FXbool readBoolEntry(const FXString& section,const FXString& name,FXbool def=false) const;

  /// Write a boolean value entry
  FXbool writeBoolEntry(const FXchar* section,const FXchar* name,FXbool val);
  FXbool writeBoolEntry(const FXString& section,const FXchar* name,FXbool val);
  FXbool writeBoolEntry(const FXString& section,const FXString& name,FXbool val);

  /// See if entry exists
  FXbool existingEntry(const FXchar* section,const FXchar* name) const;
  FXbool existingEntry(const FXString& section,const FXchar* name) const;
  FXbool existingEntry(const FXString& section,const FXString& name) const;

  /// See if section exists
  FXbool existingSection(const FXchar* section) const;
  FXbool existingSection(const FXString& section) const;

  /// Delete a registry entry
  void deleteEntry(const FXchar* section,const FXchar* name);
  void deleteEntry(const FXString& section,const FXchar* name);
  void deleteEntry(const FXString& section,const FXString& name);

  /// Delete section
  void deleteSection(const FXchar* section);
  void deleteSection(const FXString& section);

  /// Clear all sections
  void clear();

  /// Cleanup
 ~FXSettings();
  };


}

#endif
