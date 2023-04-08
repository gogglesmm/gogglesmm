/********************************************************************************
*                                                                               *
*                             I N I   F i l e   I / O                           *
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
#ifndef FXINIFILE_H
#define FXINIFILE_H

#ifndef FXINI_H
#include "FXINI.h"
#endif

namespace FX {


/**
* Serialize a variant to or from INI formatted file.
*/
class FXAPI FXINIFile : public FXINI {
private:
  FXFile file;
private:
  FXINIFile(const FXINIFile&);
  FXINIFile &operator=(const FXINIFile&);
public:

  /**
  * Create INI file i/o object.
  */
  FXINIFile();

  /**
  * Create INI file i/o object and open it.
  */
  FXINIFile(const FXString& filename,Direction d=Load,FXuval sz=8192);

  /**
  * Open INI file from given handle for direction d.
  */
  FXbool open(const FXString& filename,Direction d=Load,FXuval sz=8192);

  /// Read at least count bytes into buffer; return bytes available, or -1 for error
  virtual FXival fill(FXival count);

  /// Write at least count bytes from buffer; return space available, or -1 for error
  virtual FXival flush(FXival count);

  /**
  * Close INI file and delete buffers.
  */
  FXbool close();

  /**
  * Close INI file.
  */
  virtual ~FXINIFile();
  };

}

#endif
