/********************************************************************************
*                                                                               *
*                       A b s t r a c t   I / O   C l a s s                     *
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
#ifndef FXIO_H
#define FXIO_H

namespace FX {


/**
* FXIO is a base class for a generic i/o device.
* The FXIO implementation provides an endless source or sink of data.
* You can use FXIO to tally the number of bytes required to save
* something to storage before actually doing so.
*/
class FXAPI FXIO {
public:

  /// Access modes
  enum {

    /// Basic access options
    NoAccess     = 0,                    /// No access
    ReadOnly     = 1,                    /// Open for reading
    WriteOnly    = 2,                    /// Open for writing
    ReadWrite    = ReadOnly|WriteOnly,   /// Open for both read and write
    Append       = 4,                    /// Open for append
    Truncate     = 8,                    /// Truncate to zero when writing
    Create       = 16,                   /// Create if it doesn't exist
    Exclusive    = 32,                   /// Fail if trying to create a file which already exists
    NonBlocking  = 64,                   /// Non-blocking i/o
    Executable   = 128,                  /// Executable (memory map)
    OwnHandle    = 256,                  /// File handle is ours
    NoAccessTime = 512,                  /// Don't change access time of file
    Inheritable  = 1024,                 /// Child process can inherit handle
    Reading      = ReadOnly,                    /// Normal options for reading
    Writing      = ReadWrite|Create|Truncate    /// Normal options for writing
    };

  /// Positioning modes
  enum {
    Begin   = 0,                /// Position from the begin (default)
    Current = 1,                /// Position relative to current position
    End     = 2                 /// Position from the end
    };

  /// Permissions
  enum {

    /// Other permissions
    OtherExec      = 0x00001,                   /// Others have execute permission
    OtherWrite     = 0x00002,                   /// Others have write permisson
    OtherRead      = 0x00004,                   /// Others have read permission
    OtherReadWrite = OtherRead|OtherWrite,      /// Others have read and write permission
    OtherFull      = OtherReadWrite|OtherExec,  /// Others have full access

    /// Group permissions
    GroupExec      = 0x00008,                   /// Group has execute permission
    GroupWrite     = 0x00010,                   /// Group has write permission
    GroupRead      = 0x00020,                   /// Group has read permission
    GroupReadWrite = GroupRead|GroupWrite,      /// Group has read and write permission
    GroupFull      = GroupReadWrite|GroupExec,  /// Group has full access

    /// Owner permissions
    OwnerExec      = 0x00040,                   /// Owner has execute permission
    OwnerWrite     = 0x00080,                   /// Owner has write permission
    OwnerRead      = 0x00100,                   /// Owner has read permission
    OwnerReadWrite = OwnerRead|OwnerWrite,      /// Owner has read and write permission
    OwnerFull      = OwnerReadWrite|OwnerExec,  /// Owner has full access

    /// Combined permissions
    AllRead        = OtherRead|GroupRead|OwnerRead,     /// Read permission for all
    AllWrite       = OtherWrite|GroupWrite|OwnerWrite,  /// Write permisson for all
    AllExec        = OtherExec|GroupExec|OwnerExec,     /// Execute permission for all
    AllReadWrite   = AllRead|AllWrite,                  /// Read and write permission for all
    AllFull        = AllReadWrite|AllExec,              /// Full access for all

    /// Other flags
    Hidden         = 0x00200,   /// Hidden file
    Directory      = 0x00400,   /// Is directory
    File           = 0x00800,   /// Is regular file
    SymLink        = 0x01000,   /// Is symbolic link

    /// Special mode bits
    SetUser        = 0x02000,   /// Set user id
    SetGroup       = 0x04000,   /// Set group id
    Sticky         = 0x08000,   /// Sticky bit

    /// Device special files
    Character      = 0x10000,   /// Character device
    Block          = 0x20000,   /// Block device
    Socket         = 0x40000,   /// Socket device
    Fifo           = 0x80000    /// Fifo device
    };

  /// Error return codes for readBlock() and writeBlock()
  enum {
    Error  = -1,                /// Error in operation
    Again  = -2,                /// Try again (for non-blocking handles)
    Broken = -3                 /// Broken pipe or socket
    };

protected:
  FXIO();
private:
  FXIO(const FXIO&);
  FXIO &operator=(const FXIO&);
public:

  /// Return true if open
  virtual FXbool isOpen() const;

  /// Return true if serial access only
  virtual FXbool isSerial() const;

  /// Return access mode
  virtual FXuint mode() const;

  /// Change access mode of open device
  virtual FXbool mode(FXuint m);

  /// Return permissions
  virtual FXuint perms() const;

  /// Set permissions
  virtual FXbool perms(FXuint p);

  /// Get current file position
  virtual FXlong position() const;

  /// Change file position, returning new position from start
  virtual FXlong position(FXlong offset,FXuint from=FXIO::Begin);

  /// Read block of bytes, returning number of bytes read
  virtual FXival readBlock(void* ptr,FXival count);

  /// Write block of bytes, returning number of bytes written
  virtual FXival writeBlock(const void* ptr,FXival count);

  /// Read character
  FXbool readChar(FXchar& ch);

  /// Write character
  FXbool writeChar(FXchar ch);

  /// Truncate file
  virtual FXlong truncate(FXlong sz);

  /// Flush to disk
  virtual FXbool flush();

  /// Test if we're at the end; -1 if error
  virtual FXint eof();

  /// Return size
  virtual FXlong size();

  /// Close handle
  virtual FXbool close();

  /// Destroy and close
  virtual ~FXIO();
  };

}

#endif
