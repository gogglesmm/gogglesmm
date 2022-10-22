/********************************************************************************
*                                                                               *
*               T h r e a d - L o c a l   S t o r a g e   C l a s s             *
*                                                                               *
*********************************************************************************
* Copyright (C) 2004,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#ifndef FXAUTOTHREADSTORAGEKEY_H
#define FXAUTOTHREADSTORAGEKEY_H


namespace FX {


/**
* Automatically generated thread-local storage key.
*
* This class manages a thread-local storage key, generating
* a new one when constructed, and deleting the storage key when
* destroyed; FXAutoThreadStorageKey is typically used to declare
* global variables to be used as thread-local storage keys.
* These keys can be used just like FXThreadStorageKey itself by
* virtue of the conversion operator.  Note that no assignment
* or copy-constructors have been defined; thus each instance of
* this class represents a unique thread-local storage key.
*/
class FXAPI FXAutoThreadStorageKey {
private:
  FXThreadStorageKey value;
private:
  FXAutoThreadStorageKey(const FXAutoThreadStorageKey&);
  FXAutoThreadStorageKey &operator=(const FXAutoThreadStorageKey&);
public:

  /// Acquire a unique thread-local storage key
  FXAutoThreadStorageKey();

  /// Return the thread-local storage key
  operator FXThreadStorageKey() const { return value; }

  /// Set thread local storage associated with this key
  void set(FXptr ptr) const;

  /// Get thread local storage associated with this key
  FXptr get() const;

  /// Release thread-local storage key
 ~FXAutoThreadStorageKey();
  };

}

#endif

