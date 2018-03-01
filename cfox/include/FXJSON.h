/********************************************************************************
*                                                                               *
*                      J S O N   R e a d e r  &  W r i t e r                    *
*                                                                               *
*********************************************************************************
* Copyright (C) 2013,2018 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#ifndef FXJSON_H
#define FXJSON_H

namespace FX {


/**
* The FXJSON serializer loads or saves an FXVariant to a json text file.
* Since FXVariant can contain an arbitrarily complex data structure, this
* provides applications with a convenient way to load and save state information
* in a well-defined and human-readable file format.
*/
class FXAPI FXJSON {
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
    ErrNumber,          /// Numeric conversion
    ErrEnd              /// Unexpected end of file
    };
  enum Flow {
    Stream,             /// Stream-of-consciousness output
    Compact,            /// Compact, human readable output (default)
    Pretty              /// Pretty printed, indented output
    };
  enum Direction {
    Stop = 0,           /// Not active
    Save = 1,           /// Save to device
    Load = 2            /// Load from device
    };
protected:
  FXchar     *begptr;           // Text buffer begin ptr
  FXchar     *endptr;           // Text buffer end ptr
  FXchar     *sptr;             // Text buffer start ptr
  FXchar     *rptr;             // Text buffer read ptr
  FXchar     *wptr;             // Text buffer write ptr
  FXint       token;            // Token
  FXint       column;           // Column number
  FXint       indent;           // Indent level
  FXint       line;             // Line number
  FXint       wrap;             // Line wrap column
  Direction   dir;              // Direction
  FXuchar     flow;             // Output flow
  FXuchar     prec;             // Float precision
  FXuchar     fmt;              // Float format
  FXuchar     dent;             // Indentation amount
  FXbool      owns;             // Owns the buffer
private:
  FXint next();
  FXbool need(FXival n);
  Error emit(const FXchar* str,FXint count);
  Error emit(FXchar ch,FXint count);
  Error loadString(FXString& str);
  Error loadMap(FXVariant& var);
  Error loadArray(FXVariant& var);
  Error loadVariant(FXVariant& var);
  Error saveString(const FXString& str);
  Error saveMap(const FXVariant& var);
  Error saveArray(const FXVariant& var);
  Error saveVariant(const FXVariant& var);
private:
  static const FXchar *const errors[];
private:
  FXJSON(const FXJSON&){}
  FXJSON &operator=(const FXJSON&);
public:

  /**
  * Construct JSON serializer.
  */
  FXJSON();

  /**
  * Construct JSON serializer and open for direction d.
  * Use given buffer data of size sz, or allocate a local buffer.
  */
  FXJSON(FXchar* data,FXuval sz=4096,Direction d=Load);

  /**
  * Open JSON stream for given direction d.
  * Use given buffer data of size sz, or allocate a local buffer.
  */
  FXbool open(FXchar* data=NULL,FXuval size=4096,Direction d=Load);

  /**
  * Return direction in effect.
  */
  Direction direction() const { return dir; }

  /**
  * Return size of parse buffer
  */
  FXuval size() const { return endptr-begptr; }

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
  * Return current line number.
  */
  FXint getLine() const { return line; }

  /**
  * Return current column number.
  */
  FXint getColumn() const { return column; }

  /// Returns error code for given error
  static const FXchar* getError(Error err){ return errors[err]; }

  /**
  * Floating point output precision control.
  */
  void setNumericPrecision(FXint p){ prec=p; }
  FXint getNumericPrecision() const { return prec; }

  /**
  * Floating point output precision control.
  */
  void setNumericFormat(FXint f){ fmt=f; }
  FXint getNumericFormat() const { return fmt; }

  /**
  * Change output flow format (Stream, Compact, Pretty).
  * Stream is the most compact, but pretty much unreadable by humans; it
  * aims to be compact.
  * Compact is very human-readable while at the same time using minimum number
  * of lines to represent the output.
  * Pretty will print one item on each line, with indentation.  It is very easily
  * readable but produces large numbers of text lines.
  */
  void setOutputFlow(FXint f){ flow=f; }
  FXint getOutputFlow() const { return flow; }

  /**
  * Change indentation level for pretty print flow, the amount of
  * indentation applied for each level.
  */
  void setIndentation(FXint d){ dent=d; }
  FXint getIndentation() const { return dent; }

  /**
  * Change column at which lines are wrapped.
  */
  void setLineWrap(FXint w){ wrap=w; }
  FXint getLineWrap() const { return wrap; }

  /**
  * Fill buffer from file.
  * Return false if not open for reading, or fail to read from disk.
  */
  virtual FXbool fill();

  /**
  * Flush buffer to file.
  * Return false if not open for writing, or if fail to write to disk.
  */
  virtual FXbool flush();

  /**
  * Close stream and delete buffer, if owned.
  */
  virtual FXbool close();

  /**
  * Close JSON stream and clean up.
  */
  virtual ~FXJSON();
  };

}

#endif
