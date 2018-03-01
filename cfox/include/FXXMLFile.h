/********************************************************************************
*                                                                               *
*                             X M L - F i l e   I / O                           *
*                                                                               *
*********************************************************************************
* Copyright (C) 2016,2018 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#ifndef FXXMLFILE_H
#define FXXMLFILE_H

#ifndef FXXML_H
#include "FXXML.h"
#endif

namespace FX {


/**
* procesing XML file
*/
class FXAPI FXXMLFile : public FXXML {
private:
  FXFile file;
public:

  /**
  * Create XML file i/o object.
  */
  FXXMLFile();

  /**
  * Create XML file i/o object and open it.
  */
  FXXMLFile(const FXString& filename,Direction d=Load,FXuval sz=4096);

  /**
  * Open XML file handle for direction d.
  */
  FXbool open(FXInputHandle h,Direction d=Load,FXuval sz=4096);

  /**
  * Open XML file for direction d.
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
  * Close XML file and delete buffers.
  */
  virtual FXbool close();

  /**
  * Close XML file.
  */
  virtual ~FXXMLFile();
  };

}

#endif
