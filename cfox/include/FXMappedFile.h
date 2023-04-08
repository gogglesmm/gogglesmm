/********************************************************************************
*                                                                               *
*                      M a p p e d   F i l e   C l a s s                        *
*                                                                               *
*********************************************************************************
* Copyright (C) 2023 by Jeroen van der Zijp.   All Rights Reserved.             *
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
#ifndef FXMAPPEDFILE_H
#define FXMAPPEDFILE_H

#ifndef FXFILE_H
#include "FXFile.h"
#endif

////////////////////////////  UNDER DEVELOPMENT  ////////////////////////////////

namespace FX {


/**
* Memory mapped file.
*/
class FXAPI FXMappedFile : public FXIODevice {
private:
  FXInputHandle memhandle;      // Handle for the map
  FXptr         mempointer;     // Memory base where it is mapped
  FXlong        memlength;      // Length of the map
  FXlong        memoffset;      // Offset of the map
private:
  FXMappedFile(const FXMappedFile&);
  FXMappedFile &operator=(const FXMappedFile&);
public:

  /// Construct a memory map
  FXMappedFile();

  /// Open a file, and map a view of it into memory; the offset must be a multiple of the page size
  FXptr open(const FXString& filename,FXuint m=FXIO::Reading,FXuint perm=FXIO::AllReadWrite,FXlong len=0,FXlong off=0);

  /// Return true if serial access only
  virtual FXbool isSerial() const;

  /// Return pointer to memory area
  FXptr data() const { return mempointer; }

  /// Obtain length of the map
  FXival length() const { return memlength; }

  /// Obtain offset of the map
  FXlong offset() const { return memoffset; }

  /// Return size
  virtual FXlong size();

  /// Flush to disk
  virtual FXbool flush();

  /// Close file, and also the map
  virtual FXbool close();

  /// Return memory mapping granularity
  static FXival granularity();

  /// Destroy the map
  virtual ~FXMappedFile();
  };

}

#endif
