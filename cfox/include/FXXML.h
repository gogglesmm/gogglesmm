/********************************************************************************
*                                                                               *
*                       X M L   R e a d e r  &  W r i t e r                     *
*                                                                               *
*********************************************************************************
* Copyright (C) 2016,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#ifndef FXXML_H
#define FXXML_H

#ifndef FXPARSEBUFFER_H
#include "FXParseBuffer.h"
#endif

namespace FX {


/**
* The XML serializer loads or saves data to xml text file.
*/
class FXAPI FXXML : public FXParseBuffer {
public:
  enum Error {
    ErrOK,              /// No errors
    ErrEmpty,           /// No data loaded
    ErrSave,            /// Unable to save
    ErrLoad,            /// Unable to load
    ErrSpace,           /// Expected space
    ErrEquals,          /// Expected equals sign '='
    ErrName,            /// Expected name
    ErrString,          /// Expected string
    ErrToken,           /// Illegal token
    ErrDigit,           /// Expected digit
    ErrHexDigit,        /// Expected hex digit
    ErrSemiColon,       /// Expected semicolon
    ErrReference,       /// Unknown reference
    ErrNoMatch,         /// Start and end tag not matching
    ErrEof              /// Unexpected end of file
    };
  enum {
    CRLF = 0x0001,      /// CRLF, LFCR, CR, LF map to LF
    REFS = 0x0002,      /// Character references processed
    };
  enum {
    UTF8    = 1,        /// UTF8 encoded
    UTF16LE = 2,        /// Little endian UTF16 encoded
    UTF16BE = 3,        /// Big endian UTF16 encoded
    UTF32LE = 4,        /// Little endian  UTF32 encoded
    UTF32BE = 5         /// Big endian UTF32 encoded
    };
protected:
  class Element;        // Element info
protected:
  FXlong     offset;    // Position from start
  Element   *current;   // Current element instance
  FXint      column;    // Column number
  FXint      line;      // Line number
  FXString   vers;      // Version
  FXuint     enc;       // Encoding
private:
  FXuint guess();
  void spaces();
  FXbool name();
  FXbool match(FXchar ch);
  FXbool match(const FXchar* str,FXint len);
  Error parsestring(FXString& str);
  Error parsexml();
  Error parseversion();
  Error parseencoding();
  Error parsestandalone();
  Error parseelementdecl();
  Error parseexternalid();
  Error parseinternalsubset();
  Error parsedeclarations();
  Error parseprocessing();
  Error parsecomment();
  Error parseattribute(Element& elm);
  Error parsestarttag(Element& elm);
  Error parseendtag(Element& elm);
  Error parsecdata(Element& elm);
  Error parsecontents(Element& elm);
  Error parseelement();
private:
  static const FXchar *const errors[];
private:
  FXXML(const FXXML&);
  FXXML& operator=(const FXXML&);
public:

  /// Called when start of document is recognized.
  FXCallback<Error () > startDocumentCB;

  /// Called when start of element is recognized.
  FXCallback<Error (const FXString&,const FXStringDictionary&) > startElementCB;

  /// Called to pass batch of decoded characters.
  FXCallback<Error (const FXString&) > charactersCB;

  /// Called to pass comment string.
  FXCallback<Error (const FXString&) > commentCB;

  /// Called to pass processing instruction.
  FXCallback<Error (const FXString&,const FXString&) > processingCB;

  /// Called when end of element is recognized.
  FXCallback<Error (const FXString&) > endElementCB;

  /// Called when end of document is recognized.
  FXCallback<Error () > endDocumentCB;

public:

  /**
  * Construct XML serializer.
  */
  FXXML();

  /**
  * Construct XML serializer and open for direction d.
  * Use given buffer data of size sz, or allocate a local buffer.
  */
  FXXML(FXchar* buffer,FXuval sz=4096,Direction d=Load);

  /**
  * Open XML stream for given direction d.
  * Use given buffer data of size sz, or allocate a local buffer.
  */
  FXbool open(FXchar* buffer=nullptr,FXuval sz=4096,Direction d=Load);

  /**
  * Return size of parse buffer.
  */
  FXuval size() const { return endptr-begptr; }

  /**
  * Return direction in effect.
  */
  Direction direction() const { return dir; }

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
  * Parse the file, return error code to indicate success or
  * failure.
  */
  Error parse();

  /// Document start
  Error startDocument();

  /// Element start w/no attributes
  Error startElement(const FXString& tag);

  /// Element start w/attributes
  Error startElement(const FXString& tag,const FXStringDictionary& atts);

  /// Characters
  Error characters(const FXString& text);

  /// Comment
  Error comment(const FXString& text);

  /// Processing instruction
  Error processing(const FXString& target,const FXString& text);

  /// Element end
  Error endElement(const FXString& tag);

  /// Document end
  Error endDocument();

  /// Returns error code for given error
  static const FXchar* getError(Error err){ return errors[err]; }

  /**
  * Close stream and delete buffer, if owned.
  */
  FXbool close();

  /**
  * Decode escaped special characters from XML stream.
  */
  static FXbool decode(FXString& dst,const FXString& src,FXuint flags=CRLF|REFS);

  /**
  * Encode special characters for inclusion into XML stream.
  */
  static FXbool encode(FXString& dst,const FXString& src,FXuint flags=CRLF|REFS);

  /**
  * Close XML stream and clean up.
  */
  virtual ~FXXML();
  };

}

#endif
