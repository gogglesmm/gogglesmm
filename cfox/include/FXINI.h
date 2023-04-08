/********************************************************************************
*                                                                               *
*                      I N I   R e a d e r  &  W r i t e r                      *
*                                                                               *
*********************************************************************************
* Copyright (C) 2022 by Jeroen van der Zijp.   All Rights Reserved.             *
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
#ifndef FXINI_H
#define FXINI_H

#ifndef FXPARSEBUFFER_H
#include "FXParseBuffer.h"
#endif

namespace FX {


/**
* The FXINI class loads or saves an FXVariant to an .INI text file.
* The FXVariant structure is subject to some limits:
*
*   - Must be a map of maps, of the form variant[SECTION][KEY] where
*     SECTION and KEY are both strings.
*
*   - Each KEY maps to boolean, integer, double, or string type, or
*     be empty (equal to FXVariant::null).
*
*   - The variant tree may contain other items, but only items of the
*     aforementioned types will be serialized to the .INI file.
*
* When saving, numeric values are printed with configurable precision;
* (default=15 digits which results in minimal information loss for real
* numbers).
* Values may be arbitrary strings, and this includes any legal UTF8-
* encoded value. When saving, strings may be escaped to ensure the
* information may be read back in unchanged.  The following applies
* to when strings are escaped:
*
*   - Escape mode 0: UTF8 characters are passed unescaped.
*   - Escape mode 1: UTF8 characters are escaped as \xXX.
*   - Escape mode 2: UTF8 will be escaed using Unicode escape sequences of
*     the for \uXXXX or \uXXXX\uXXXX (two surrogate-pairs  escape codes
*     for code points exceeding 16 bits).
*
* The default setting is to allow UTF8 characters in the output.
*/
class FXAPI FXINI : public FXParseBuffer {
public:
  enum Error {
    ErrOK,              /// No errors
    ErrSave,            /// Unable to save
    ErrLoad,            /// Unable to load
    ErrToken,           /// Illegal token
    ErrColon,           /// Expected colon ':'
    ErrComma,           /// Expected comma ','
    ErrBracket,         /// Expected closing bracket
    ErrBrace,           /// Expected closing brace
    ErrQuotes,          /// Expected closing quotes
    ErrQuote,           /// Expected closing quote
    ErrNumber,          /// Numeric conversion
    ErrEnd              /// Unexpected end of file
    };
protected:
  FXlong     offset;     // Position from start
  FXint      token;      // Current token
  FXint      column;     // Current column
  FXint      indent;     // Indentation level
  FXint      line;       // Line number
  FXuchar    state;      // Parse state
  FXuchar    prec;       // Numeric precision
  FXuchar    fmt;        // Numeric format
  FXuchar    esc;        // Escape mode
private:
  FXint next();
  Error loadVariant(FXVariant& var);
  Error saveVariant(const FXVariant& section);
  Error saveSection(const FXString& str);
  Error saveEntry(const FXVariant& var);
  Error saveString(const FXString& str);
private:
  static const FXchar *const errors[];
private:
  FXINI(const FXINI&);
  FXINI &operator=(const FXINI&);
public:

  /**
  * Initialize INI parser.
  */
  FXINI();

  /**
  * Initialize INI parser with buffer of size and direction.
  * Text location (column, line number, byte offset) is reset.
  */
  FXINI(FXchar* buffer,FXuval sz=8192,Direction d=Load);

  /**
  * Open INI parse buffer with given size and direction.
  * Text location (column, line number, byte offset) is reset.
  */
  FXbool open(FXchar* buffer,FXuval sz=8192,Direction d=Load);

  /**
  * Return current line number.
  */
  FXint getLine() const { return line; }

  /**
  * Return current column number.
  */
  FXint getColumn() const { return column; }

  /**
  * Return offset from begin of file.
  */
  FXlong getOffset() const { return offset; }

  /**
  * Load a variant from stream.
  * Return false if stream wasn't opened for loading, or syntax error.
  */
  Error load(FXVariant& variant);

  /**
  * Save a variant to stream.
  * Return false if stream wasn't opened for saving, or disk was full.
  */
  Error save(const FXVariant& variant);

  /**
  * Returns error code for given error.
  */
  static const FXchar* getError(Error err){ return errors[err]; }

  /**
  * Floating point output precision control.
  * This controls the number of significant digits written to
  * the output.  The default is 15.
  */
  void setNumericPrecision(FXint p){ prec=p; }
  FXint getNumericPrecision() const { return prec; }

  /**
  * Floating point output format control.
  * The format mode is interpreted as follows:
  *
  *   0   No exponent.
  *   1   Exponent.
  *   2   Output exponent when required.
  *
  * The default mode is 2.
  */
  void setNumericFormat(FXint f){ fmt=f; }
  FXint getNumericFormat() const { return fmt; }

  /**
  * Change string escape mode; 0=don't escape unicode in strings;
  * 1=escape unicode as \xHH, 2=escape UTF8 multi-byte characters
  * as \uXXXX or \uXXXX\uXXXX.
  * Default is to escape control characters only.
  */
  void setEscapeMode(FXint e){ esc=e; }
  FXint getEscapeMode() const { return esc; }

  /**
  * Close INI parser.
  * To permit diagnostics, text location not reset.
  */
  FXbool close();

  /**
  * Close INI parser and clean up.
  */
  virtual ~FXINI();
  };

}

#endif
