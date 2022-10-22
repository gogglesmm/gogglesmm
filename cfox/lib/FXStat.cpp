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
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "fxchar.h"
#include "fxmath.h"
#include "fxascii.h"
#include "FXArray.h"
#include "FXHash.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXPath.h"
#include "FXStat.h"
#include "FXFile.h"



/*
  Notes:
  - Find out stuff about files and directories.
*/


using namespace FX;

/*******************************************************************************/

namespace FX {


// Return true if it is a hidden file (note: Windows-only attribute)
FXbool FXStat::isHidden() const {
  return (modeFlags&FXIO::Hidden)!=0;
  }

// Return true if input path is a directory
FXbool FXStat::isDirectory() const {
  return (modeFlags&FXIO::Directory)!=0;
  }

// Return true if it is a regular file
FXbool FXStat::isFile() const {
  return (modeFlags&FXIO::File)!=0;
  }

// Return true if it is a link
FXbool FXStat::isLink() const {
  return (modeFlags&FXIO::SymLink)!=0;
  }

// Return true if the file sets the user id on execution
FXbool FXStat::isSetUid() const {
  return (modeFlags&FXIO::SetUser)!=0;
  }

// Return true if the file sets the group id on execution
FXbool FXStat::isSetGid() const {
  return (modeFlags&FXIO::SetGroup)!=0;
  }

// Return true if the file has the sticky bit set
FXbool FXStat::isSetSticky() const {
  return (modeFlags&FXIO::Sticky)!=0;
  }

// Return true if special device (character or block device)
FXbool FXStat::isDevice() const {
  return (modeFlags&(FXIO::Character|FXIO::Block))!=0;
  }

// Return true if character device
FXbool FXStat::isCharacter() const {
  return (modeFlags&FXIO::Character)!=0;
  }

// Return true if block device
FXbool FXStat::isBlock() const {
  return (modeFlags&FXIO::Block)!=0;
  }

// Return true if socket device
FXbool FXStat::isSocket() const {
  return (modeFlags&FXIO::Socket)!=0;
  }

// Return true if fifo device
FXbool FXStat::isFifo() const {
  return (modeFlags&FXIO::Fifo)!=0;
  }

// Return true if file is readable
FXbool FXStat::isReadable() const {
  return (modeFlags&(FXIO::OtherRead|FXIO::GroupRead|FXIO::OwnerRead))!=0;
  }

// Return true if file is writable
FXbool FXStat::isWritable() const {
  return (modeFlags&(FXIO::OtherWrite|FXIO::GroupWrite|FXIO::OwnerWrite))!=0;
  }

// Return true if file is executable
FXbool FXStat::isExecutable() const {
  return (modeFlags&(FXIO::OtherExec|FXIO::GroupExec|FXIO::OwnerExec))!=0;
  }

// Return true if owner has read-write-execute permissions
FXbool FXStat::isOwnerReadWriteExecute() const {
  return (modeFlags&FXIO::OwnerExec) && (modeFlags&FXIO::OwnerWrite) && (modeFlags&FXIO::OwnerRead);
  }

// Return true if owner has read permissions
FXbool FXStat::isOwnerReadable() const {
  return (modeFlags&FXIO::OwnerRead)!=0;
  }

// Return true if owner has write permissions
FXbool FXStat::isOwnerWritable() const {
  return (modeFlags&FXIO::OwnerWrite)!=0;
  }

// Return true if owner has execute permissions
FXbool FXStat::isOwnerExecutable() const {
  return (modeFlags&FXIO::OwnerExec)!=0;
  }

// Return true if group has read-write-execute permissions
FXbool FXStat::isGroupReadWriteExecute() const {
  return (modeFlags&FXIO::GroupExec) && (modeFlags&FXIO::GroupWrite) && (modeFlags&FXIO::GroupRead);
  }

// Return true if group has read permissions
FXbool FXStat::isGroupReadable() const {
  return (modeFlags&FXIO::GroupRead)!=0;
  }

// Return true if group has write permissions
FXbool FXStat::isGroupWritable() const {
  return (modeFlags&FXIO::GroupWrite)!=0;
  }

// Return true if group has execute permissions
FXbool FXStat::isGroupExecutable() const {
  return (modeFlags&FXIO::GroupExec)!=0;
  }

// Return true if others have read-write-execute permissions
FXbool FXStat::isOtherReadWriteExecute() const {
  return (modeFlags&FXIO::OtherExec) && (modeFlags&FXIO::OtherWrite) && (modeFlags&FXIO::OtherRead);
  }

// Return true if others have read permissions
FXbool FXStat::isOtherReadable() const {
  return (modeFlags&FXIO::OtherRead)!=0;
  }

// Return true if others have write permissions
FXbool FXStat::isOtherWritable() const {
  return (modeFlags&FXIO::OtherWrite)!=0;
  }

// Return true if others have execute permissions
FXbool FXStat::isOtherExecutable() const {
  return (modeFlags&FXIO::OtherExec)!=0;
  }


#if defined(WIN32)

// Convert 100ns since 01/01/1601 to ns since 01/01/1970
static inline FXTime fxunixtime(FXTime ft){
  return (ft-FXLONG(116444736000000000))*FXLONG(100);
  }

// Convert ns since 01/01/1970 to 100ns since 01/01/1601
static inline FXTime fxwintime(FXTime ut){
  return ut/FXLONG(100)+FXLONG(116444736000000000);
  }

#endif


// Get statistics of given file
FXbool FXStat::statFile(const FXString& file,FXStat& info){
  FXbool result=false;
  info.modeFlags=0;
  info.userNumber=0;
  info.groupNumber=0;
  info.linkCount=0;
  info.createTime=0;
  info.accessTime=0;
  info.modifyTime=0;
  info.fileVolume=0;
  info.fileIndex=0;
  info.fileSize=0;
  if(!file.empty()){
#ifdef WIN32
#ifdef UNICODE
    FXnchar unifile[MAXPATHLEN];
    HANDLE hfile;
    utf2ncs(unifile,file.text(),MAXPATHLEN);
    if((hfile=::CreateFile(unifile,FILE_READ_ATTRIBUTES,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL|FILE_FLAG_BACKUP_SEMANTICS,nullptr))!=INVALID_HANDLE_VALUE){
      BY_HANDLE_FILE_INFORMATION data;
      if(::GetFileInformationByHandle(hfile,&data)){
        info.modeFlags=FXIO::AllFull;
        if(data.dwFileAttributes&FILE_ATTRIBUTE_HIDDEN) info.modeFlags|=FXIO::Hidden;
        if(data.dwFileAttributes&FILE_ATTRIBUTE_READONLY) info.modeFlags&=~FXIO::AllWrite;
        if(data.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) info.modeFlags|=FXIO::Directory|FXIO::AllWrite; else info.modeFlags|=FXIO::File;     // Directories (folders) always writable on Windows
        if((info.modeFlags&FXIO::File) && !FXPath::hasExecExtension(file)) info.modeFlags&=~FXIO::AllExec;
        info.userNumber=0;
        info.groupNumber=0;
        info.linkCount=data.nNumberOfLinks;
        info.accessTime=fxunixtime(*((FXTime*)&data.ftLastAccessTime));
        info.modifyTime=fxunixtime(*((FXTime*)&data.ftLastWriteTime));
        info.createTime=fxunixtime(*((FXTime*)&data.ftCreationTime));
        info.fileVolume=data.dwVolumeSerialNumber;
        info.fileIndex=(((FXulong)data.nFileIndexHigh)<<32)|((FXulong)data.nFileIndexLow);
        info.fileSize=(((FXlong)data.nFileSizeHigh)<<32)|((FXlong)data.nFileSizeLow);
        result=true;
        }
      ::CloseHandle(hfile);
      }
#else
    HANDLE hfile;
    if((hfile=::CreateFile(file.text(),FILE_READ_ATTRIBUTES,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL|FILE_FLAG_BACKUP_SEMANTICS,nullptr))!=INVALID_HANDLE_VALUE){
      BY_HANDLE_FILE_INFORMATION data;
      if(::GetFileInformationByHandle(hfile,&data)){
        info.modeFlags=FXIO::AllFull;
        if(data.dwFileAttributes&FILE_ATTRIBUTE_HIDDEN) info.modeFlags|=FXIO::Hidden;
        if(data.dwFileAttributes&FILE_ATTRIBUTE_READONLY) info.modeFlags&=~FXIO::AllWrite;
        if(data.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) info.modeFlags|=FXIO::Directory|FXIO::AllWrite; else info.modeFlags|=FXIO::File;     // Directories (folders) always writable on Windows
        if((info.modeFlags&FXIO::File) && !FXPath::hasExecExtension(file)) info.modeFlags&=~FXIO::AllExec;
        info.userNumber=0;
        info.groupNumber=0;
        info.linkCount=data.nNumberOfLinks;
        info.accessTime=fxunixtime(*((FXTime*)&data.ftLastAccessTime));
        info.modifyTime=fxunixtime(*((FXTime*)&data.ftLastWriteTime));
        info.createTime=fxunixtime(*((FXTime*)&data.ftCreationTime));
        info.fileVolume=data.dwVolumeSerialNumber;
        info.fileIndex=(((FXulong)data.nFileIndexHigh)<<32)|((FXulong)data.nFileIndexLow);
        info.fileSize=(((FXlong)data.nFileSizeHigh)<<32)|((FXlong)data.nFileSizeLow);
        result=true;
        }
      ::CloseHandle(hfile);
      }
#endif
#else
    const FXTime seconds=1000000000;
    struct stat data;
    if(::stat(file.text(),&data)==0){
      info.modeFlags=(data.st_mode&FXIO::AllFull);
      if(S_ISDIR(data.st_mode)) info.modeFlags|=FXIO::Directory;
      if(S_ISREG(data.st_mode)) info.modeFlags|=FXIO::File;
      if(S_ISLNK(data.st_mode)) info.modeFlags|=FXIO::SymLink;
      if(S_ISCHR(data.st_mode)) info.modeFlags|=FXIO::Character;
      if(S_ISBLK(data.st_mode)) info.modeFlags|=FXIO::Block;
      if(S_ISFIFO(data.st_mode)) info.modeFlags|=FXIO::Fifo;
      if(S_ISSOCK(data.st_mode)) info.modeFlags|=FXIO::Socket;
      if(data.st_mode&S_ISUID) info.modeFlags|=FXIO::SetUser;
      if(data.st_mode&S_ISGID) info.modeFlags|=FXIO::SetGroup;
      if(data.st_mode&S_ISVTX) info.modeFlags|=FXIO::Sticky;
      info.userNumber=data.st_uid;
      info.groupNumber=data.st_gid;
      info.linkCount=data.st_nlink;
#if (_POSIX_C_SOURCE >= 200809L) || (_XOPEN_SOURCE >= 700)
      info.accessTime=data.st_atim.tv_sec*seconds+data.st_atim.tv_nsec;
      info.modifyTime=data.st_mtim.tv_sec*seconds+data.st_mtim.tv_nsec;
      info.createTime=data.st_ctim.tv_sec*seconds+data.st_ctim.tv_nsec;
#else
      info.accessTime=data.st_atime*seconds;
      info.modifyTime=data.st_mtime*seconds;
      info.createTime=data.st_ctime*seconds;
#endif
      info.fileVolume=(FXlong)data.st_dev;
      info.fileIndex=(FXlong)data.st_ino;
      info.fileSize=(FXlong)data.st_size;
      result=true;
      }
#endif
    }
  return result;
  }


// Get statistice of the linked file
FXbool FXStat::statLink(const FXString& file,FXStat& info){
  FXbool result=false;
  info.modeFlags=0;
  info.userNumber=0;
  info.groupNumber=0;
  info.linkCount=0;
  info.createTime=0;
  info.accessTime=0;
  info.modifyTime=0;
  info.fileVolume=0;
  info.fileIndex=0;
  info.fileSize=0;
  if(!file.empty()){
#ifdef WIN32
#ifdef UNICODE
    FXnchar unifile[MAXPATHLEN];
    HANDLE hfile;
    utf2ncs(unifile,file.text(),MAXPATHLEN);
    if((hfile=::CreateFile(unifile,FILE_READ_ATTRIBUTES,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL|FILE_FLAG_BACKUP_SEMANTICS,nullptr))!=INVALID_HANDLE_VALUE){
      BY_HANDLE_FILE_INFORMATION data;
      if(::GetFileInformationByHandle(hfile,&data)){
        info.modeFlags=FXIO::AllFull;
        if(data.dwFileAttributes&FILE_ATTRIBUTE_HIDDEN) info.modeFlags|=FXIO::Hidden;
        if(data.dwFileAttributes&FILE_ATTRIBUTE_READONLY) info.modeFlags&=~FXIO::AllWrite;
        if(data.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) info.modeFlags|=FXIO::Directory|FXIO::AllWrite; else info.modeFlags|=FXIO::File;     // Directories (folders) always writable on Windows
        if((info.modeFlags&FXIO::File) && !FXPath::hasExecExtension(file)) info.modeFlags&=~FXIO::AllExec;
        info.userNumber=0;
        info.groupNumber=0;
        info.linkCount=data.nNumberOfLinks;
        info.accessTime=fxunixtime(*((FXTime*)&data.ftLastAccessTime));
        info.modifyTime=fxunixtime(*((FXTime*)&data.ftLastWriteTime));
        info.createTime=fxunixtime(*((FXTime*)&data.ftCreationTime));
        info.fileVolume=data.dwVolumeSerialNumber;
        info.fileIndex=(((FXulong)data.nFileIndexHigh)<<32)|((FXulong)data.nFileIndexLow);
        info.fileSize=(((FXlong)data.nFileSizeHigh)<<32)|((FXlong)data.nFileSizeLow);
        result=true;
        }
      ::CloseHandle(hfile);
      }
#else
    HANDLE hfile;
    if((hfile=::CreateFile(file.text(),FILE_READ_ATTRIBUTES,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL|FILE_FLAG_BACKUP_SEMANTICS,nullptr))!=INVALID_HANDLE_VALUE){
      BY_HANDLE_FILE_INFORMATION data;
      if(::GetFileInformationByHandle(hfile,&data)){
        info.modeFlags=FXIO::AllFull;
        if(data.dwFileAttributes&FILE_ATTRIBUTE_HIDDEN) info.modeFlags|=FXIO::Hidden;
        if(data.dwFileAttributes&FILE_ATTRIBUTE_READONLY) info.modeFlags&=~FXIO::AllWrite;
        if(data.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) info.modeFlags|=FXIO::Directory|FXIO::AllWrite; else info.modeFlags|=FXIO::File;     // Directories (folders) always writable on Windows
        if((info.modeFlags&FXIO::File) && !FXPath::hasExecExtension(file)) info.modeFlags&=~FXIO::AllExec;
        info.userNumber=0;
        info.groupNumber=0;
        info.linkCount=data.nNumberOfLinks;
        info.accessTime=fxunixtime(*((FXTime*)&data.ftLastAccessTime));
        info.modifyTime=fxunixtime(*((FXTime*)&data.ftLastWriteTime));
        info.createTime=fxunixtime(*((FXTime*)&data.ftCreationTime));
        info.fileVolume=data.dwVolumeSerialNumber;
        info.fileIndex=(((FXulong)data.nFileIndexHigh)<<32)|((FXulong)data.nFileIndexLow);
        info.fileSize=(((FXlong)data.nFileSizeHigh)<<32)|((FXlong)data.nFileSizeLow);
        result=true;
        }
      ::CloseHandle(hfile);
      }
#endif
#else
    const FXTime seconds=1000000000;
    struct stat data;
    if(::lstat(file.text(),&data)==0){
      info.modeFlags=(data.st_mode&FXIO::AllFull);
      if(S_ISDIR(data.st_mode)) info.modeFlags|=FXIO::Directory;
      if(S_ISREG(data.st_mode)) info.modeFlags|=FXIO::File;
      if(S_ISLNK(data.st_mode)) info.modeFlags|=FXIO::SymLink;
      if(S_ISCHR(data.st_mode)) info.modeFlags|=FXIO::Character;
      if(S_ISBLK(data.st_mode)) info.modeFlags|=FXIO::Block;
      if(S_ISFIFO(data.st_mode)) info.modeFlags|=FXIO::Fifo;
      if(S_ISSOCK(data.st_mode)) info.modeFlags|=FXIO::Socket;
      if(data.st_mode&S_ISUID) info.modeFlags|=FXIO::SetUser;
      if(data.st_mode&S_ISGID) info.modeFlags|=FXIO::SetGroup;
      if(data.st_mode&S_ISVTX) info.modeFlags|=FXIO::Sticky;
      info.userNumber=data.st_uid;
      info.groupNumber=data.st_gid;
      info.linkCount=data.st_nlink;
#if (_POSIX_C_SOURCE >= 200809L) || (_XOPEN_SOURCE >= 700)
      info.accessTime=data.st_atim.tv_sec*seconds+data.st_atim.tv_nsec;
      info.modifyTime=data.st_mtim.tv_sec*seconds+data.st_mtim.tv_nsec;
      info.createTime=data.st_ctim.tv_sec*seconds+data.st_ctim.tv_nsec;
#else
      info.accessTime=data.st_atime*seconds;
      info.modifyTime=data.st_mtime*seconds;
      info.createTime=data.st_ctime*seconds;
#endif
      info.fileVolume=(FXlong)data.st_dev;
      info.fileIndex=(FXlong)data.st_ino;
      info.fileSize=(FXlong)data.st_size;
      result=true;
      }
#endif
    }
  return result;
  }


// Get statistice of the already open file
FXbool FXStat::stat(const FXFile& file,FXStat& info){
  info.modeFlags=0;
  info.userNumber=0;
  info.groupNumber=0;
  info.linkCount=0;
  info.createTime=0;
  info.accessTime=0;
  info.modifyTime=0;
  info.fileVolume=0;
  info.fileIndex=0;
  info.fileSize=0;
#ifdef WIN32
  BY_HANDLE_FILE_INFORMATION data;
  if(::GetFileInformationByHandle(file.handle(),&data)){
    info.modeFlags=FXIO::AllFull;
    if(data.dwFileAttributes&FILE_ATTRIBUTE_HIDDEN) info.modeFlags|=FXIO::Hidden;
    if(data.dwFileAttributes&FILE_ATTRIBUTE_READONLY) info.modeFlags&=~FXIO::AllWrite;
    if(data.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) info.modeFlags|=FXIO::Directory|FXIO::AllWrite; else info.modeFlags|=FXIO::File; // Directories (folders) always writable on Windows
    info.userNumber=0;
    info.groupNumber=0;
    info.linkCount=data.nNumberOfLinks;
    info.accessTime=fxunixtime(*((FXTime*)&data.ftLastAccessTime));
    info.modifyTime=fxunixtime(*((FXTime*)&data.ftLastWriteTime));
    info.createTime=fxunixtime(*((FXTime*)&data.ftCreationTime));
    info.fileVolume=data.dwVolumeSerialNumber;
    info.fileIndex=(((FXulong)data.nFileIndexHigh)<<32)|((FXulong)data.nFileIndexLow);
    info.fileSize=(((FXulong)data.nFileSizeHigh)<<32)|((FXulong)data.nFileSizeLow);
    return true;
    }
#else
  const FXTime seconds=1000000000;
  struct stat data;
  if(::fstat(file.handle(),&data)==0){
    info.modeFlags=(data.st_mode&FXIO::AllFull);
    if(S_ISDIR(data.st_mode)) info.modeFlags|=FXIO::Directory;
    if(S_ISREG(data.st_mode)) info.modeFlags|=FXIO::File;
    if(S_ISLNK(data.st_mode)) info.modeFlags|=FXIO::SymLink;
    if(S_ISCHR(data.st_mode)) info.modeFlags|=FXIO::Character;
    if(S_ISBLK(data.st_mode)) info.modeFlags|=FXIO::Block;
    if(S_ISFIFO(data.st_mode)) info.modeFlags|=FXIO::Fifo;
    if(S_ISSOCK(data.st_mode)) info.modeFlags|=FXIO::Socket;
    if(data.st_mode&S_ISUID) info.modeFlags|=FXIO::SetUser;
    if(data.st_mode&S_ISGID) info.modeFlags|=FXIO::SetGroup;
    if(data.st_mode&S_ISVTX) info.modeFlags|=FXIO::Sticky;
    info.userNumber=data.st_uid;
    info.groupNumber=data.st_gid;
    info.linkCount=data.st_nlink;
#if (_POSIX_C_SOURCE >= 200809L) || (_XOPEN_SOURCE >= 700)
    info.accessTime=data.st_atim.tv_sec*seconds+data.st_atim.tv_nsec;
    info.modifyTime=data.st_mtim.tv_sec*seconds+data.st_mtim.tv_nsec;
    info.createTime=data.st_ctim.tv_sec*seconds+data.st_ctim.tv_nsec;
#else
    info.accessTime=data.st_atime*seconds;
    info.modifyTime=data.st_mtime*seconds;
    info.createTime=data.st_ctime*seconds;
#endif
    info.fileVolume=(FXlong)data.st_dev;
    info.fileIndex=(FXlong)data.st_ino;
    info.fileSize=(FXlong)data.st_size;
    return true;
    }
#endif
  return false;
  }


// Return file mode flags
FXuint FXStat::mode(const FXString& file){
  FXStat data;
  statFile(file,data);
  return data.mode();
  }



// Change the mode flags for this file
FXbool FXStat::mode(const FXString& file,FXuint perm){
  if(!file.empty()){
#ifdef WIN32
/*
#ifdef UNICODE
    FXnchar unifile[MAXPATHLEN];
    utf2ncs(unifile,file.text(),MAXPATHLEN);
    FXuint flags=::GetFileAttributesW(unifile);
    if(flags!=INVALID_FILE_ATTRIBUTES){
      if(flags&FILE_ATTRIBUTE_DIRECTORY){
        }
      else{
        }

      if((flags&FILE_ATTRIBUTE_DIRECTORY) || (perm&FXIO::AllWrite)){
        flags&=~FILE_ATTRIBUTE_READONLY;
        }
      else{
        flags|=FILE_ATTRIBUTE_READONLY;
        }
      if(perm&FXIO::Hidden) flags|=FILE_ATTRIBUTE_HIDDEN; else flags&=~FILE_ATTRIBUTE_HIDDEN;
      flags&=FILE_ATTRIBUTE_ARCHIVE|FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_NORMAL|FILE_ATTRIBUTE_NOT_CONTENT_INDEXED|FILE_ATTRIBUTE_OFFLINE|FILE_ATTRIBUTE_READONLY|FILE_ATTRIBUTE_SYSTEM|FILE_ATTRIBUTE_TEMPORARY;
      return ::SetFileAttributesW(unifile,flags)!=0;
      }
    return false;
#else
    FXuint flags=::GetFileAttributesA(unifile);
    if(flags!=INVALID_FILE_ATTRIBUTES){
      if((flags&FILE_ATTRIBUTE_DIRECTORY) || (perm&FXIO::AllWrite)) flags&=~FILE_ATTRIBUTE_READONLY; else flags|=FILE_ATTRIBUTE_READONLY;
      if(perm&FXIO::Hidden) flags|=FILE_ATTRIBUTE_HIDDEN; else flags&=~FILE_ATTRIBUTE_HIDDEN;
      flags&=FILE_ATTRIBUTE_ARCHIVE|FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_NORMAL|FILE_ATTRIBUTE_NOT_CONTENT_INDEXED|FILE_ATTRIBUTE_OFFLINE|FILE_ATTRIBUTE_READONLY|FILE_ATTRIBUTE_SYSTEM|FILE_ATTRIBUTE_TEMPORARY;
      return ::SetFileAttributesA(file.text(),flags)!=0;
      }
    return false;
#endif
*/
#else
    FXuint bits=perm&0777;
    if(perm&FXIO::SetUser) bits|=S_ISUID;
    if(perm&FXIO::SetGroup) bits|=S_ISGID;
    if(perm&FXIO::Sticky) bits|=S_ISVTX;
    return ::chmod(file.text(),bits)==0;
#endif
    }
  return false;
  }


// Return true if file exists
FXbool FXStat::exists(const FXString& file){
  FXTRACE((100,"FXStat::exists(\"%s\"\n",file.text()));
  if(!file.empty()){
#ifdef WIN32
#ifdef UNICODE
    FXnchar unifile[MAXPATHLEN];
    utf2ncs(unifile,file.text(),MAXPATHLEN);
    FXTRACE((100,"FXStat::exists: %d\n",(::GetFileAttributesW(unifile)!=INVALID_FILE_ATTRIBUTES)));
    return ::GetFileAttributesW(unifile)!=INVALID_FILE_ATTRIBUTES;
#else
    FXTRACE((100,"FXStat::exists: %d\n",(::GetFileAttributesA(file.text())!=INVALID_FILE_ATTRIBUTES)));
    return ::GetFileAttributesA(file.text())!=INVALID_FILE_ATTRIBUTES;
#endif
#else
    struct stat status;
    FXTRACE((100,"FXStat::exists: %d\n",(::stat(file.text(),&status)==0)));
    return ::stat(file.text(),&status)==0;
#endif
    }
  return false;
  }


// Get file size
FXlong FXStat::size(const FXString& file){
  FXStat data;
  statFile(file,data);
  return data.size();
  }


// Return file volume number
FXlong FXStat::volume(const FXString& file){
  FXStat data;
  statFile(file,data);
  return data.volume();
  }


// Return file index number
FXlong FXStat::index(const FXString& file){
  FXStat data;
  statFile(file,data);
  return data.index();
  }


// Return number of links to file
FXuint FXStat::links(const FXString& file){
  FXStat data;
  statFile(file,data);
  return data.links();
  }


// Return time file was last modified
FXTime FXStat::modified(const FXString& file){
  FXStat data;
  statFile(file,data);
  return data.modified();
  }


// Change tiome when file was last modified
FXbool FXStat::modified(const FXString& file,FXTime ns){
  if(!file.empty()){
#ifdef WIN32
#ifdef UNICODE
    FXnchar unifile[MAXPATHLEN];
    utf2ncs(unifile,file.text(),MAXPATHLEN);
    FXInputHandle hnd=CreateFileW(unifile,GENERIC_READ|FILE_WRITE_ATTRIBUTES,0,nullptr,OPEN_EXISTING,0,nullptr);
#else
    FXInputHandle hnd=CreateFileA(file.text(),GENERIC_READ|FILE_WRITE_ATTRIBUTES,0,nullptr,OPEN_EXISTING,0,nullptr);
#endif
    if(hnd!=INVALID_HANDLE_VALUE){
      FILETIME wintime;
      *((FXTime*)&wintime)=fxwintime(ns);
      if(SetFileTime(hnd,nullptr,nullptr,&wintime)!=0){
        CloseHandle(hnd);
        return true;
        }
      CloseHandle(hnd);
      }
#else
#if (defined(_ATFILE_SOURCE) && defined(UTIME_OMIT))
    const FXTime seconds=1000000000;
    struct timespec values[2];
    values[0].tv_sec=UTIME_OMIT;
    values[0].tv_nsec=UTIME_OMIT;
    values[1].tv_sec=ns/seconds;
    values[1].tv_nsec=ns%seconds;
    return utimensat(AT_FDCWD,file.text(),values,0)==0;
#else
    const FXTime seconds=1000000;
    struct stat data;
    if(::stat(file.text(),&data)==0){
      struct timeval values[2];
      values[0].tv_sec=data.st_atime;
      values[0].tv_usec=0;
      values[1].tv_sec=ns/seconds;
      values[1].tv_usec=ns%seconds;
      return utimes(file.text(),values)==0;
      }
#endif
#endif
    }
  return false;
  }


// Return time file was last accessed
FXTime FXStat::accessed(const FXString& file){
  FXStat data;
  statFile(file,data);
  return data.accessed();
  }


// Change tiome when file was last accessed
FXbool FXStat::accessed(const FXString& file,FXTime ns){
  if(!file.empty()){
#ifdef WIN32
#ifdef UNICODE
    FXnchar unifile[MAXPATHLEN];
    utf2ncs(unifile,file.text(),MAXPATHLEN);
    FXInputHandle hnd=CreateFileW(unifile,GENERIC_READ|FILE_WRITE_ATTRIBUTES,0,nullptr,OPEN_EXISTING,0,nullptr);
#else
    FXInputHandle hnd=CreateFileA(file.text(),GENERIC_READ|FILE_WRITE_ATTRIBUTES,0,nullptr,OPEN_EXISTING,0,nullptr);
#endif
    if(hnd!=INVALID_HANDLE_VALUE){
      FILETIME wintime;
      *((FXTime*)&wintime)=fxwintime(ns);
      if(SetFileTime(hnd,nullptr,&wintime,nullptr)!=0){
        CloseHandle(hnd);
        return true;
        }
      CloseHandle(hnd);
      }
#else
#if (defined(_ATFILE_SOURCE) && defined(UTIME_OMIT))
    const FXTime seconds=1000000000;
    struct timespec values[2];
    values[0].tv_sec=ns/seconds;
    values[0].tv_nsec=ns%seconds;
    values[1].tv_sec=UTIME_OMIT;
    values[1].tv_nsec=UTIME_OMIT;
    return utimensat(AT_FDCWD,file.text(),values,0)==0;
#else
    const FXTime seconds=1000000;
    struct stat data;
    if(::stat(file.text(),&data)==0){
      struct timeval values[2];
      values[0].tv_sec=ns/seconds;
      values[0].tv_usec=ns%seconds;
      values[1].tv_sec=data.st_mtime;
      values[1].tv_usec=0;
      return utimes(file.text(),values)==0;
      }
#endif
#endif
    }
  return false;
  }


// Return time when created
FXTime FXStat::created(const FXString& file){
  FXStat data;
  statFile(file,data);
  return data.created();
  }


// Change time when file was last created
FXbool FXStat::created(const FXString& file,FXTime ns){
  if(!file.empty()){
#ifdef WIN32
#ifdef UNICODE
    FXnchar unifile[MAXPATHLEN];
    utf2ncs(unifile,file.text(),MAXPATHLEN);
    FXInputHandle hnd=CreateFileW(unifile,GENERIC_READ|FILE_WRITE_ATTRIBUTES,0,nullptr,OPEN_EXISTING,0,nullptr);
#else
    FXInputHandle hnd=CreateFileA(file.text(),GENERIC_READ|FILE_WRITE_ATTRIBUTES,0,nullptr,OPEN_EXISTING,0,nullptr);
#endif
    if(hnd!=INVALID_HANDLE_VALUE){
      FILETIME wintime;
      *((FXTime*)&wintime)=fxwintime(ns);
      if(SetFileTime(hnd,&wintime,nullptr,nullptr)!=0){
        CloseHandle(hnd);
        return true;
        }
      CloseHandle(hnd);
      }
#else
    return false;               // Not available on *NIX
#endif
    }
  return false;
  }


// Return true if file is hidden
FXbool FXStat::isHidden(const FXString& file){
  FXStat data;
  return statFile(file,data) && data.isHidden();
  }


// Check if file represents a file
FXbool FXStat::isFile(const FXString& file){
  FXStat data;
  return statFile(file,data) && data.isFile();
  }


// Check if file represents a link
FXbool FXStat::isLink(const FXString& file){
  FXStat data;
  return statLink(file,data) && data.isLink();
  }


// Check if file represents a directory
FXbool FXStat::isDirectory(const FXString& file){
  FXStat data;
  return statFile(file,data) && data.isDirectory();
  }


// Return true if file is readable
FXbool FXStat::isReadable(const FXString& file){
  FXStat data;
  return statFile(file,data) && data.isReadable();
  }


// Return true if file is writable
FXbool FXStat::isWritable(const FXString& file){
  FXStat data;
  return statFile(file,data) && data.isWritable();
  }


// Return true if file is executable
FXbool FXStat::isExecutable(const FXString& file){
  FXStat data;
  return statFile(file,data) && data.isExecutable();
  }


// Check if owner has full permissions
FXbool FXStat::isOwnerReadWriteExecute(const FXString& file){
  FXStat data;
  return statFile(file,data) && data.isOwnerReadWriteExecute();
  }


// Check if owner can read
FXbool FXStat::isOwnerReadable(const FXString& file){
  FXStat data;
  return statFile(file,data) && data.isOwnerReadable();
  }


// Check if owner can write
FXbool FXStat::isOwnerWritable(const FXString& file){
  FXStat data;
  return statFile(file,data) && data.isOwnerWritable();
  }


// Check if owner can execute
FXbool FXStat::isOwnerExecutable(const FXString& file){
  FXStat data;
  return statFile(file,data) && data.isOwnerExecutable();
  }


// Check if group has full permissions
FXbool FXStat::isGroupReadWriteExecute(const FXString& file){
  FXStat data;
  return statFile(file,data) && data.isGroupReadWriteExecute();
  }


// Check if group can read
FXbool FXStat::isGroupReadable(const FXString& file){
  FXStat data;
  return statFile(file,data) && data.isGroupReadable();
  }


// Check if group can write
FXbool FXStat::isGroupWritable(const FXString& file){
  FXStat data;
  return statFile(file,data) && data.isGroupWritable();
  }


// Check if group can execute
FXbool FXStat::isGroupExecutable(const FXString& file){
  FXStat data;
  return statFile(file,data) && data.isGroupExecutable();
  }


// Check if everybody has full permissions
FXbool FXStat::isOtherReadWriteExecute(const FXString& file){
  FXStat data;
  return statFile(file,data) && data.isOtherReadWriteExecute();
  }


// Check if everybody can read
FXbool FXStat::isOtherReadable(const FXString& file){
  FXStat data;
  return statFile(file,data) && data.isOtherReadable();
  }


// Check if everybody can write
FXbool FXStat::isOtherWritable(const FXString& file){
  FXStat data;
  return statFile(file,data) && data.isOtherWritable();
  }


// Check if everybody can execute
FXbool FXStat::isOtherExecutable(const FXString& file){
  FXStat data;
  return statFile(file,data) && data.isOtherExecutable();
  }


// Test if suid bit set
FXbool FXStat::isSetUid(const FXString& file){
  FXStat data;
  return statFile(file,data) && data.isSetUid();
  }


// Test if sgid bit set
FXbool FXStat::isSetGid(const FXString& file){
  FXStat data;
  return statFile(file,data) && data.isSetGid();
  }


// Test if sticky bit set
FXbool FXStat::isSetSticky(const FXString& file){
  FXStat data;
  return statFile(file,data) && data.isSetSticky();
  }


// Return true if file is accessible
FXbool FXStat::isAccessible(const FXString& file,FXuint m){
  if(!file.empty()){
#ifdef WIN32
#ifdef UNICODE
    FXnchar unifile[MAXPATHLEN];
    FXuint mode=0;
    if(m&FXIO::ReadOnly) mode|=4;
    if(m&FXIO::WriteOnly) mode|=2;
    utf2ncs(unifile,file.text(),MAXPATHLEN);
    return _waccess(unifile,mode)==0;
#else
    FXuint mode=0;
    if(m&FXIO::ReadOnly) mode|=4;
    if(m&FXIO::WriteOnly) mode|=2;
    return _access(file.text(),mode)==0;
#endif
#else
    FXuint mode=F_OK;
    if(m&FXIO::ReadOnly) mode|=R_OK;
    if(m&FXIO::WriteOnly) mode|=W_OK;
    if(m&FXIO::Executable) mode|=X_OK;
    return access(file.text(),mode)==0;
#endif
    }
  return false;
  }


// Obtain total amount of space on disk
FXbool FXStat::getTotalDiskSpace(const FXString& path,FXulong& space){
#ifdef WIN32
#ifdef UNICODE
  FXnchar unifile[MAXPATHLEN];
  utf2ncs(unifile,path.text(),MAXPATHLEN);
  if(GetDiskFreeSpaceExW(unifile,nullptr,(PULARGE_INTEGER)&space,nullptr)){
    return true;
    }
#else
  if(GetDiskFreeSpaceExA(path.text(),nullptr,(PULARGE_INTEGER)&space,nullptr)){
    return true;
    }
#endif
#else
#if defined(HAVE_STATVFS) && defined(HAVE_SYS_STATVFS_H)
  struct statvfs info;
  if(statvfs(path.text(),&info)==0){
    space=info.f_bsize*info.f_blocks;
    return true;
    }
#endif
#endif
  return false;
  }


// Obtain available amount of space on disk
FXbool FXStat::getAvailableDiskSpace(const FXString& path,FXulong& space){
#ifdef WIN32
#ifdef UNICODE
  FXnchar unifile[MAXPATHLEN];
  utf2ncs(unifile,path.text(),MAXPATHLEN);
  if(GetDiskFreeSpaceExW(unifile,(PULARGE_INTEGER)&space,nullptr,nullptr)){
    return true;
    }
#else
  if(GetDiskFreeSpaceExA(path.text(),(PULARGE_INTEGER)&space,nullptr,nullptr)){
    return true;
    }
#endif
#else
#if defined(HAVE_STATVFS) && defined(HAVE_SYS_STATVFS_H)
  struct statvfs info;
  if(statvfs(path.text(),&info)==0){
    space=info.f_bsize*info.f_bfree;
    return true;
    }
#endif
#endif
  return false;
  }

}
