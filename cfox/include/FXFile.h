/********************************************************************************
*                                                                               *
*                             F i l e   C l a s s                               *
*                                                                               *
*********************************************************************************
* Copyright (C) 2000,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#ifndef FXFILE_H
#define FXFILE_H

#ifndef FXIODEVICE_H
#include "FXIODevice.h"
#endif

namespace FX {


/**
* Low level file access.
*/
class FXAPI FXFile : public FXIODevice {
private:
  FXFile(const FXFile&);
  FXFile &operator=(const FXFile&);
public:

  /// Construct file
  FXFile(){ }

  /// Construct file and attach existing handle h
  FXFile(FXInputHandle h);

  /// Construct and open a file
  FXFile(const FXString& filename,FXuint m=FXIO::Reading,FXuint perm=FXIO::AllReadWrite);

  /// Open file
  virtual FXbool open(const FXString& filename,FXuint m=FXIO::Reading,FXuint perm=FXIO::AllReadWrite);

  /// Return true if serial access only
  virtual FXbool isSerial() const;

  /// Get current file position
  virtual FXlong position() const;

  /// Change file position, returning new position from start
  virtual FXlong position(FXlong offset,FXuint from=FXIO::Begin);

  /// Truncate file to size s
  virtual FXlong truncate(FXlong sz);

  /// Flush to disk
  virtual FXbool flush();

  /// Test if we're at the end; -1 if error
  virtual FXint eof();

  /// Return file size
  virtual FXlong size();

  /// Create new (empty) file
  static FXbool create(const FXString& file,FXuint perm=FXIO::AllReadWrite);

  /// Link file
  static FXbool link(const FXString& srcfile,const FXString& dstfile);

  /// Read symbolic link
  static FXString symlink(const FXString& file);

  /// Symbolic link file
  static FXbool symlink(const FXString& srcfile,const FXString& dstfile);

  /// Return true if files are identical (identical node on disk)
  static FXbool identical(const FXString& file1,const FXString& file2);

  /// Copy srcfile to dstfile, overwriting dstfile if allowed
  static FXbool copy(const FXString& srcfile,const FXString& dstfile,FXbool overwrite=false);

  /// Recursively copy files or directories from srcfile to dstfile, overwriting dstfile if allowed
  static FXbool copyFiles(const FXString& srcfile,const FXString& dstfile,FXbool overwrite=false);


  /// Move or rename srcfile to dstfile, overwriting dstfile if allowed
  static FXbool move(const FXString& srcfile,const FXString& dstfile,FXbool overwrite=false);

  /// Recursively copy or move files or directories from srcfile to dstfile, overwriting dstfile if allowed
  static FXbool moveFiles(const FXString& srcfile,const FXString& dstfile,FXbool overwrite=false);


  /// Remove file
  static FXbool remove(const FXString& file);

  /// Recursively remove file or directory, recurse if allowed
  static FXbool removeFiles(const FXString& path,FXbool recursive=false);


  /// Concatenate srcfile1 and srcfile2 to dstfile, overwriting dstfile if allowed
  static FXbool concat(const FXString& srcfile1,const FXString& srcfile2,const FXString& dstfile,FXbool overwrite=false);
  };

}

#endif
