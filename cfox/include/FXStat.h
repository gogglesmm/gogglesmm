/********************************************************************************
*                                                                               *
*                        F i l e   S t a t i s t i c s                          *
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
#ifndef FXSTAT_H
#define FXSTAT_H

namespace FX {


class FXFile;


/// Statistics about a file or directory
class FXAPI FXStat {
private:
  FXuint  modeFlags;            /// Mode bits
  FXuint  userNumber;           /// User number
  FXuint  groupNumber;          /// Group number
  FXuint  linkCount;            /// Number of links
  FXTime  createTime;           /// Create time (ns)
  FXTime  accessTime;           /// Access time (ns)
  FXTime  modifyTime;           /// Modify time (ns)
  FXlong  fileVolume;           /// File volume (device)
  FXlong  fileIndex;            /// File index (inode)
  FXlong  fileSize;             /// File size
public:

  /// Initialize
  FXStat():modeFlags(0),userNumber(0),groupNumber(0),linkCount(0),createTime(0),accessTime(0),modifyTime(0),fileVolume(0),fileIndex(0),fileSize(0){ }

  /// Return the mode flags for this file
  FXuint mode() const { return modeFlags; }

  /// Return user number
  FXuint user() const { return userNumber; }

  /// Return group number
  FXuint group() const { return groupNumber; }

  /// Return number of links to file
  FXuint links() const { return linkCount; }

  /// Return time when file was created, in nanoseconds
  FXTime created() const { return createTime; }

  /// Return time when last accessed, in nanoseconds
  FXTime accessed() const { return accessTime; }

  /// Return time when last modified, in nanoseconds
  FXTime modified() const { return modifyTime; }

  /// Return file volume number
  FXlong volume() const { return fileVolume; }

  /// Return file index number
  FXlong index() const { return fileIndex; }

  /// Return file size in bytes
  FXlong size() const { return fileSize; }

  /// Return true if it is a hidden file (Windows-only)
  FXbool isHidden() const;

  /// Return true if input path is a directory
  FXbool isDirectory() const;

  /// Return true if it is a regular file
  FXbool isFile() const;

  /// Return true if it is a link
  FXbool isLink() const;

  /// Return true if the file sets the user id on execution
  FXbool isSetUid() const;

  /// Return true if the file sets the group id on execution
  FXbool isSetGid() const;

  /// Return true if the file has the sticky bit set
  FXbool isSetSticky() const;

  /// Return true if special device (character or block device)
  FXbool isDevice() const;

  /// Return true if character device
  FXbool isCharacter() const;

  /// Return true if block device
  FXbool isBlock() const;

  /// Return true if socket device
  FXbool isSocket() const;

  /// Return true if fifo (pipe) device
  FXbool isFifo() const;

  /// Return true if file is readable
  FXbool isReadable() const;

  /// Return true if file is writable
  FXbool isWritable() const;

  /// Return true if file is executable
  FXbool isExecutable() const;

  /// Return true if owner has read-write-execute permissions
  FXbool isOwnerReadWriteExecute() const;

  /// Return true if owner has read permissions
  FXbool isOwnerReadable() const;

  /// Return true if owner has write permissions
  FXbool isOwnerWritable() const;

  /// Return true if owner has execute permissions
  FXbool isOwnerExecutable() const;

  /// Return true if group has read-write-execute permissions
  FXbool isGroupReadWriteExecute() const;

  /// Return true if group has read permissions
  FXbool isGroupReadable() const;

  /// Return true if group has write permissions
  FXbool isGroupWritable() const;

  /// Return true if group has execute permissions
  FXbool isGroupExecutable() const;

  /// Return true if others have read-write-execute permissions
  FXbool isOtherReadWriteExecute() const;

  /// Return true if others have read permissions
  FXbool isOtherReadable() const;

  /// Return true if others have write permissions
  FXbool isOtherWritable() const;

  /// Return true if others have execute permissions
  FXbool isOtherExecutable() const;


  /// Get statistics of the file into the stat buffer info
  static FXbool statFile(const FXString& file,FXStat& info);

  /// Get statistice of the link into the stat buffer info
  static FXbool statLink(const FXString& file,FXStat& info);

  /// Get statistics of already open file into stat buffer info
  static FXbool stat(const FXFile& file,FXStat& info);


  /// Return the mode flags for this file
  static FXuint mode(const FXString& file);

  /// Change the mode flags for this file
  static FXbool mode(const FXString& file,FXuint perm);

  /// Return true if file exists
  static FXbool exists(const FXString& file);

  /// Return file size in bytes
  static FXlong size(const FXString& file);

  /// Return file volume number
  static FXlong volume(const FXString& file);

  /// Return file index number
  static FXlong index(const FXString& file);

  /// Return number of links to file
  static FXuint links(const FXString& file);

  /**
  * Return last modified time for this file, on filesystems
  * where this is supported.  This is the time when any data
  * in the file was last modified, in nanoseconds since Epoch.
  */
  static FXTime modified(const FXString& file);

  /**
  * Change modified time for the given file, on filesystems
  * where this is supported.  Time is specified in nanoseconds
  * since Epoch.
  */
  static FXbool modified(const FXString& file,FXTime ns);

  /**
  * Return last accessed time for this file, on filesystems
  * where this is supported, in nanoseconds since Epoch.
  */
  static FXTime accessed(const FXString& file);

  /**
  * Change accessed time for the given file, on filesystems
  * where this is supported.  Time is specified in nanoseconds
  * since Epoch.
  */
  static FXbool accessed(const FXString& file,FXTime ns);

  /**
  * Return created time for this file, on filesystems
  * where this is supported.  This is also the time when
  * ownership, permissions, links, and other meta-data may
  * have changed, in nanoseconds since Epoch.
  */
  static FXTime created(const FXString& file);

  /**
  * Change created time for the given file, on filesystems
  * where this is supported.  Time is specified in nanoseconds
  * since Epoch.
  */
  static FXbool created(const FXString& file,FXTime ns);

  /// Return true if file is hidden
  static FXbool isHidden(const FXString& file);

  /// Return true if input path is a file name
  static FXbool isFile(const FXString& file);

  /// Return true if input path is a link
  static FXbool isLink(const FXString& file);

  /// Return true if input path is a directory
  static FXbool isDirectory(const FXString& file);

  /// Return true if file is readable
  static FXbool isReadable(const FXString& file);

  /// Return true if file is writable
  static FXbool isWritable(const FXString& file);

  /// Return true if file is executable
  static FXbool isExecutable(const FXString& file);

  /// Return true if owner has read-write-execute permissions
  static FXbool isOwnerReadWriteExecute(const FXString& file);

  /// Return true if owner has read permissions
  static FXbool isOwnerReadable(const FXString& file);

  /// Return true if owner has write permissions
  static FXbool isOwnerWritable(const FXString& file);

  /// Return true if owner has execute permissions
  static FXbool isOwnerExecutable(const FXString& file);

  /// Return true if group has read-write-execute permissions
  static FXbool isGroupReadWriteExecute(const FXString& file);

  /// Return true if group has read permissions
  static FXbool isGroupReadable(const FXString& file);

  /// Return true if group has write permissions
  static FXbool isGroupWritable(const FXString& file);

  /// Return true if group has execute permissions
  static FXbool isGroupExecutable(const FXString& file);

  /// Return true if others have read-write-execute permissions
  static FXbool isOtherReadWriteExecute(const FXString& file);

  /// Return true if others have read permissions
  static FXbool isOtherReadable(const FXString& file);

  /// Return true if others have write permissions
  static FXbool isOtherWritable(const FXString& file);

  /// Return true if others have execute permissions
  static FXbool isOtherExecutable(const FXString& file);

  /// Return true if the file sets the user id on execution
  static FXbool isSetUid(const FXString& file);

  /// Return true if the file sets the group id on execution
  static FXbool isSetGid(const FXString& file);

  /// Return true if the file has the sticky bit set
  static FXbool isSetSticky(const FXString& file);

  /// Return true if file is accessible for access mode m
  /// (FXIO::ReadOnly, FXIO::WriteOnly, FXIO::ReadWrite, FXIO::Executable, etc.)
  static FXbool isAccessible(const FXString& file,FXuint m=FXIO::ReadWrite);

  /// Obtain total amount of space on disk mounted at given path
  static FXbool getTotalDiskSpace(const FXString& path,FXulong& space);

  /// Obtain available amount of space on disk mounted at given path
  static FXbool getAvailableDiskSpace(const FXString& path,FXulong& space);
  };

}

#endif
