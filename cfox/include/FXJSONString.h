/********************************************************************************
*                                                                               *
*                            J S O N   S t r i n g   I / O                      *
*                                                                               *
*********************************************************************************
* Copyright (C) 2018,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#ifndef FXJSONSTRING_H
#define FXJSONSTRING_H

#ifndef FXJSON_H
#include "FXJSON.h"
#endif

namespace FX {


/**
* Serialize a variant to or from JSON formatted string.
*/
class FXAPI FXJSONString : public FXJSON {
private:
  FXString buffer;
private:
  FXJSONString(const FXJSONString&);
  FXJSONString &operator=(const FXJSONString&);
public:

  /**
  * Create JSON string i/o object.
  */
  FXJSONString();

  /**
  * Create JSON string i/o object and open it.
  */
  FXJSONString(const FXString& string,Direction d=Load);

  /**
  * Create JSON i/o object and open it.
  */
  FXJSONString(const FXchar* string,FXuval length,Direction d=Load);

  /**
  * Open JSON string for loading or saving.
  */
  FXbool open(const FXString& string,Direction d=Load);

  /**
  * Open JSON character string of length for direction d.
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
  * Close JSON string and delete buffers.
  */
  FXbool close();

  /**
  * Close JSON string.
  */
  virtual ~FXJSONString();
  };

}

#endif
