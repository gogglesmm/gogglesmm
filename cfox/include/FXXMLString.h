/********************************************************************************
*                                                                               *
*                             X M L   S t r i n g   I / O                       *
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
#ifndef FXXMLSTRING_H
#define FXXMLSTRING_H

#ifndef FXXML_H
#include "FXXML.h"
#endif

namespace FX {


/**
* Serialize a variant to or from XML formatted string.
*/
class FXAPI FXXMLString : public FXXML {
private:
  FXString buffer;
private:
  FXXMLString(const FXXMLString&);
  FXXMLString &operator=(const FXXMLString&);
public:
  /**
  * Create XML string i/o object.
  */
  FXXMLString();

  /**
  * Open XML string for direction d.
  */
  FXXMLString(const FXString& string,Direction d=Load);

  /**
  * Create XML i/o object and open it.
  */
  FXXMLString(const FXchar* string,FXuval length,Direction d=Load);

  /**
  * Open XML file for direction d.
  */
  FXbool open(const FXString& string,Direction d=Load);

  /**
  * Open XML character string of length for direction d.
  */
  FXbool open(const FXchar* string,FXuval length,Direction d=Load);

  /**
  * Return string.
  */
  const FXString text() const { return buffer; }

  /**
  * Read at least count bytes into buffer; return bytes available, or -1 for error.
  */
  virtual FXival fill(FXival count);

  /**
  * Write at least count bytes from buffer; return space available, or -1 for error.
  */
  virtual FXival flush(FXival count);

  /**
  * Close XML file and delete buffers.
  */
  virtual FXbool close();

  /**
  * Close XML string.
  */
  virtual ~FXXMLString();
  };

}

#endif
