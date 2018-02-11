/********************************************************************************
*                                                                               *
*                            J S O N   F i l e   I / O                          *
*                                                                               *
*********************************************************************************
* Copyright (C) 2013,2017 by Jeroen van der Zijp.   All Rights Reserved.        *
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
  * Open JSON file from given handle for direction d.
  */
  FXbool open(FXInputHandle h,Direction d=Load,FXuval sz=4096);

  /**
  * Open JSON file for direction d.
  */
  FXbool open(const FXString& filename,Direction d=Load,FXuval sz=4096);

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
  * Close JSON file and delete buffers.
  */
  virtual FXbool close();

  /**
  * Close JSON file.
  */
  virtual ~FXJSONFile();
  };

}

#endif
