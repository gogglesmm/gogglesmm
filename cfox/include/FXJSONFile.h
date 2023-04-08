/********************************************************************************
*                                                                               *
*                            J S O N   F i l e   I / O                          *
*                                                                               *
*********************************************************************************
* Copyright (C) 2013,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#ifndef FXJSONFILE_H
#define FXJSONFILE_H

#ifndef FXJSON_H
#include "FXJSON.h"
#endif

namespace FX {


/**
* Serialize a variant to or from JSON formatted file.
*/
class FXAPI FXJSONFile : public FXJSON {
private:
  FXFile file;
private:
  FXJSONFile(const FXJSONFile&);
  FXJSONFile &operator=(const FXJSONFile&);
public:

  /**
  * Create JSON file i/o object.
  */
  FXJSONFile();

  /**
  * Create JSON file i/o object and open it.
  */
  FXJSONFile(const FXString& filename,Direction d=Load,FXuval sz=4096);

  /**
  * Open JSON file for direction d.
  */
  FXbool open(const FXString& filename,Direction d=Load,FXuval sz=4096);

  /**
  * Read at least count bytes into buffer; return bytes available, or -1 for error.
  */
  virtual FXival fill(FXival count);

  /**
  * Write at least count bytes from buffer; return space available, or -1 for error.
  */
  virtual FXival flush(FXival count);

  /**
  * Close JSON file and delete buffers.
  */
  FXbool close();

  /**
  * Close JSON file.
  */
  virtual ~FXJSONFile();
  };

}

#endif
