/********************************************************************************
*                                                                               *
*                    D i r e c t o r y   E n u m e r a t o r                    *
*                                                                               *
*********************************************************************************
* Copyright (C) 2005,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#ifndef FXDIR_H
#define FXDIR_H

namespace FX {


/// Directory enumerator
class FXAPI FXDir {
private:
  FXuval space[256];
public:

  /// Options for listing files
  enum {
    MatchAll    = 0,              /// Only files and directories matching pattern
    NoFiles     = 1,              /// Don't list any files
    NoDirs      = 2,              /// Don't list any directories
    AllFiles    = 4,              /// List all files
    AllDirs     = 8,              /// List all directories
    HiddenFiles = 16,             /// List hidden files also
    HiddenDirs  = 32,             /// List hidden directories also
    NoParent    = 64,             /// Don't include '.' and '..' in the listing
    CaseFold    = 128             /// Matching is case-insensitive
    };

private:
  FXDir(const FXDir&);
  FXDir &operator=(const FXDir&);
public:

  /// Construct directory enumerator
  FXDir();

  /// Construct directory enumerator open on path
  FXDir(const FXString& path);

  /// Open directory to path, return true if ok.
  FXbool open(const FXString& path);

  /// Returns true if the directory is open
  FXbool isOpen() const;

  /// Go to next directory entry and return its name
  FXbool next(FXString& name);

  /// Close directory
  void close();


  /// Create directory
  static FXbool create(const FXString& path,FXuint perm=FXIO::AllFull);

  /// Remove directory
  static FXbool remove(const FXString& path);


  /**
  * List files in a given directory.
  * Returns the number of files in the string-array list which matched the
  * pattern or satisfied the flag conditions.
  */
  static FXint listFiles(FXString*& filelist,const FXString& path,const FXString& pattern="*",FXuint flags=FXDir::MatchAll);

  /**
  * List drives, i.e. the roots of directory trees.
  * On Windows, this returns an array of strings like {"C:\", "D:\", ..., ""},
  * while on Unix it will be just a two-element list like {"/", ""}.
  * The last element will be always be set to the empty string.
  * The list can be released by means of delete [] list.
  * Returns the number of non-empty elements in the array.
  */
  static FXint listDrives(FXString*& drivelist);


  /// Create a directories recursively
  static FXbool createDirectories(const FXString& path,FXuint perm=FXIO::AllFull);


  /// Destructor
 ~FXDir();
  };


}

#endif
