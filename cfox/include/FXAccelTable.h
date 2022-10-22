/********************************************************************************
*                                                                               *
*                   A c c e l e r a t o r   T a b l e   C l a s s               *
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
#ifndef FXACCELTABLE_H
#define FXACCELTABLE_H

#ifndef FXOBJECT_H
#include "FXObject.h"
#endif

namespace FX {


/**
* The accelerator table sends a message to a specific
* target object when the indicated key and modifier combination
* is pressed.
*/
class FXAPI FXAccelTable : public FXObject {
  FXDECLARE(FXAccelTable)
protected:
  struct FXAccelKey {
    FXObject    *target;    // Target object of message
    FXSelector   messagedn; // Message being sent
    FXSelector   messageup; // Message being sent
    FXHotKey     code;      // Keysym and modifier mask to match
    };
private:
  FXAccelKey *key;          // Accelerator table
  FXuint      max;          // Largest table index
  FXuint      num;          // Number of entries
private:
  void resize(FXuint m);
private:
  FXAccelTable(const FXAccelTable&);
  FXAccelTable &operator=(const FXAccelTable&);
public:
  long onKeyPress(FXObject*,FXSelector,void*);
  long onKeyRelease(FXObject*,FXSelector,void*);
public:

  /**
  * Construct empty accelerator table.
  */
  FXAccelTable();

  /**
  * Add accelerator key-combination into the accelerator table.
  */
  void addAccel(FXHotKey hotkey,FXObject* target=nullptr,FXSelector seldn=0,FXSelector selup=0);

  /**
  * Parse key-combination description and add it into the accelerator table.
  */
  void addAccel(const FXchar* string,FXObject* target=nullptr,FXSelector seldn=0,FXSelector selup=0);
  void addAccel(const FXString& string,FXObject* target=nullptr,FXSelector seldn=0,FXSelector selup=0);

  /**
  * Remove accelerator key combination from the accelerator table.
  */
  void removeAccel(FXHotKey hotkey);

  /**
  * Parse key-combination description and remove it from the accelerator table.
  */
  void removeAccel(const FXchar* string);
  void removeAccel(const FXString& string);

  /**
  * Return true if accelerator accelerator key-combination is in accelerator table.
  */
  FXbool hasAccel(FXHotKey hotkey) const;

  /**
  * Parse key-combination description and return true if it is in the accelerator table.
  */
  FXbool hasAccel(const FXchar* string) const;
  FXbool hasAccel(const FXString& string) const;

  /**
  * Return target object of the given accelerator key-combination.
  */
  FXObject* targetOfAccel(FXHotKey hotkey) const;

  /**
  * Parse key-combination description and return its target.
  */
  FXObject* targetOfAccel(const FXchar* string) const;
  FXObject* targetOfAccel(const FXString& string) const;

  /**
  * Parse accelerator from string, yielding modifier and key code.
  * The syntax of the string is:
  *
  *  <Accelerator> ::= (<Modifier> ('-' | '+'))* <Key>
  *
  * where:
  *
  * <Modifier> ::= 'Ctl' | 'Ctrl' | 'Alt' | 'Meta' | 'Shift'
  *
  * <Key>      ::= 'Home' | 'End' | 'PgUp' | 'PgDn' | 'Left' | 'Right' |
  *                'Up' | 'Down' | 'Ins' | 'Del' | 'Esc' | 'Tab' | 'Return' |
  *                'Enter' | 'Back' | 'Spc' | 'Space' |
  *                'F'<Digit><Digit>? |
  *                '#'<HexDigit>+ |
  *                <Letter>
  *
  * <Digit>    ::= '0' ... '1'
  * <Letter>   ::= 'A' ... 'Z'
  * <HexDigit> ::= '0' ... '9', 'A' ... 'F'
  *
  * Case is not significant, but uppercase is preferred.
  * For example, parseAccel("Ctl+Shift+X") yields the same value as:
  * MKUINT(KEY_X,CONTROLMASK|SHIFTMASK).
  */
  static FXHotKey parseAccel(const FXchar* string);
  static FXHotKey parseAccel(const FXString& string);

  /**
  * Unparse hot key comprising modifier and key code back
  * into a string suitable for parsing with fxparseHotKey.
  * For example, an input of MKUINT(KEY_X,CONTROLMASK|SHIFTMASK)
  * will return the string "Ctl+Shift+X".
  */
  static FXString unparseAccel(FXHotKey key);

  /// Save table to a stream
  virtual void save(FXStream& store) const;

  /// Load table from a stream
  virtual void load(FXStream& store);

  /**
  * Destroy accelerator table.
  */
  virtual ~FXAccelTable();
  };


/**
* Parse hot key from string of the form "&Hotkey", yielding modifier and
* key code. If a '&' is to be just plain text, it should be doubled.
* For example, parseHotKey(""Salt && &Pepper!"") yields the same value as
* MKUINT(KEY_p,ALTMASK).
*/
extern FXAPI FXHotKey parseHotKey(const FXString& string);

/**
* Obtain hot key offset in string, or -1 if not found.
* For example, findHotKey("Salt && &Pepper!") yields 7.
* Note that this is the byte-offset, not the character
* index!
*/
extern FXAPI FXint findHotKey(const FXString& string);

/**
* Strip hot key combination from the string.
* For example, stripHotKey("Salt && &Pepper") should
* yield "Salt & Pepper".
*/
extern FXAPI FXString stripHotKey(const FXString& string);

}

#endif
