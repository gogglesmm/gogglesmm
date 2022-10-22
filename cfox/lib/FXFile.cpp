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
#include "FXIO.h"
#include "FXIODevice.h"
#include "FXStat.h"
#include "FXFile.h"
#include "FXPipe.h"
#include "FXDir.h"


/*
  Notes:

  - Implemented many functions in terms of FXFile and FXDir so we won't have to worry about
    unicode stuff.

*/

// Bad handle value
#ifdef WIN32
#define BadHandle INVALID_HANDLE_VALUE
#else
#define BadHandle -1
#endif

#ifdef WIN32
#ifndef INVALID_SET_FILE_POINTER
#define INVALID_SET_FILE_POINTER ((DWORD)-1)
#endif
#endif

using namespace FX;

/*******************************************************************************/

namespace FX {


// Construct file and attach existing handle h
FXFile::FXFile(FXInputHandle h,FXuint m){
  attach(h,m);
  }


// Construct and open a file
FXFile::FXFile(const FXString& file,FXuint m,FXuint perm){
  open(file,m,perm);
  }


// Open file
FXbool FXFile::open(const FXString& file,FXuint m,FXuint perm){
  if(__likely(device==BadHandle) && __likely(!file.empty())){
#if defined(WIN32)
    SECURITY_ATTRIBUTES sat;
    DWORD flags=GENERIC_READ;
    DWORD creation=OPEN_EXISTING;

    // Basic access mode
    switch(m&(ReadOnly|WriteOnly)){
      case ReadOnly: flags=GENERIC_READ; break;
      case WriteOnly: flags=GENERIC_WRITE; break;
      case ReadWrite: flags=GENERIC_READ|GENERIC_WRITE; break;
      }

    // Creation and truncation mode
    switch(m&(Create|Truncate|Exclusive)){
      case Create: creation=OPEN_ALWAYS; break;
      case Truncate: creation=TRUNCATE_EXISTING; break;
      case Create|Truncate: creation=CREATE_ALWAYS; break;
      case Create|Truncate|Exclusive: creation=CREATE_NEW; break;
      }

    // Inheritable
    sat.nLength=sizeof(SECURITY_ATTRIBUTES);
    sat.bInheritHandle=(m&Inheritable)?true:false;
    sat.lpSecurityDescriptor=nullptr;

    // Non-blocking mode
    if(m&NonBlocking){
      // FIXME
      }

    // Do it
    access=NoAccess;
    pointer=0L;
#if defined(UNICODE)
    FXnchar unifile[MAXPATHLEN];
    utf2ncs(unifile,file.text(),MAXPATHLEN);
    device=::CreateFileW(unifile,flags,FILE_SHARE_READ|FILE_SHARE_WRITE,&sat,creation,FILE_ATTRIBUTE_NORMAL,nullptr);
#else
    device=::CreateFileA(file.text(),flags,FILE_SHARE_READ|FILE_SHARE_WRITE,&sat,creation,FILE_ATTRIBUTE_NORMAL,nullptr);
#endif
    if(device!=BadHandle){
      if(m&Append){ position(0,FXIO::End); }    // Appending
      access=(m|OwnHandle);                     // Own handle
      return true;
      }
#else
    FXint bits=perm&0777;
    FXint flags=0;

    // Basic access mode
    switch(m&(ReadOnly|WriteOnly)){
      case ReadOnly: flags=O_RDONLY; break;
      case WriteOnly: flags=O_WRONLY; break;
      case ReadWrite: flags=O_RDWR; break;
      }

    // Appending and truncation
    if(m&Append) flags|=O_APPEND;
    if(m&Truncate) flags|=O_TRUNC;

    // Non-blocking mode
    if(m&NonBlocking) flags|=O_NONBLOCK;

    // Change access time
#if defined(O_NOATIME)
    if(m&NoAccessTime) flags|=O_NOATIME;
#endif

    // Inheritable only if specified
#if defined(O_CLOEXEC)
    if(!(m&Inheritable)) flags|=O_CLOEXEC;
#endif

    // Creation mode
    if(m&Create){
      flags|=O_CREAT;
      if(m&Exclusive) flags|=O_EXCL;
      }

    // Permission bits
    if(perm&FXIO::SetUser) bits|=S_ISUID;
    if(perm&FXIO::SetGroup) bits|=S_ISGID;
    if(perm&FXIO::Sticky) bits|=S_ISVTX;

    // Do it
    access=NoAccess;
    pointer=0L;
    device=::open(file.text(),flags,bits);
    if(device!=BadHandle){
      if(m&Append){ position(0,FXIO::End); }    // Appending
      access=(m|OwnHandle);                     // Own handle
      return true;
      }
#endif
    }
  return false;
  }


// Open device with access mode and handle
FXbool FXFile::open(FXInputHandle h,FXuint m){
  return FXIODevice::open(h,m);
  }


// Return true if serial access only
FXbool FXFile::isSerial() const {
  return false;
  }


// Get position
FXlong FXFile::position() const {
  return pointer;
  }


// Move to position
FXlong FXFile::position(FXlong offset,FXuint from){
  if(__likely(device!=BadHandle)){
#if defined(WIN32)
    LARGE_INTEGER pos;
    pos.QuadPart=offset;
    pos.LowPart=::SetFilePointer(device,pos.LowPart,&pos.HighPart,from);
    if(pos.LowPart!=INVALID_SET_FILE_POINTER || GetLastError()==NO_ERROR){
      pointer=pos.QuadPart;
      return pointer;
      }
#else
    FXlong pos;
    if(0<=(pos=::lseek(device,offset,from))){
      pointer=pos;
      return pointer;
      }
#endif
    }
  return FXIO::Error;
  }


// Truncate file
FXlong FXFile::truncate(FXlong sz){
  if(__likely(device!=BadHandle)){
#if defined(WIN32)
    LARGE_INTEGER pos;
    pos.QuadPart=sz;
    pos.LowPart=::SetFilePointer(device,pos.LowPart,&pos.HighPart,FILE_BEGIN);
    if(pos.LowPart!=INVALID_SET_FILE_POINTER || GetLastError()==NO_ERROR){
      if(::SetEndOfFile(device)!=0){
        position(pointer);
        return sz;
        }
      }
#else
    if(::ftruncate(device,sz)==0){
      position(pointer);
      return sz;
      }
#endif
    }
  return FXIO::Error;
  }


// Flush to disk
FXbool FXFile::flush(){
  if(__likely(device!=BadHandle)){
#if defined(WIN32)
    return ::FlushFileBuffers(device)!=0;
#elif defined(_BSD_SOURCE) || defined(_XOPEN_SOURCE) || (_POSIX_C_SOURCE >= 200112L)
    return ::fsync(device)==0;
#endif
    }
  return false;
  }


// Test if we're at the end; -1 if error
FXint FXFile::eof(){
  if(__likely(device!=BadHandle)){
    return !(pointer<size());
    }
  return FXIO::Error;
  }


// Return file size
FXlong FXFile::size(){
  if(__likely(device!=BadHandle)){
#if defined(WIN32)
    ULARGE_INTEGER result;
    result.LowPart=::GetFileSize(device,&result.HighPart);
    return result.QuadPart;
#else
    struct stat data;
    if(::fstat(device,&data)==0) return data.st_size;
#endif
    }
  return FXIO::Error;
  }

/*******************************************************************************/

// Create new (empty) file
FXbool FXFile::create(const FXString& file,FXuint perm){
  if(!file.empty()){
#if defined(WIN32)
#if defined(UNICODE)
    FXnchar unifile[MAXPATHLEN];
    utf2ncs(unifile,file.text(),MAXPATHLEN);
    FXInputHandle h=::CreateFileW(unifile,GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
#else
    FXInputHandle h=::CreateFileA(file.text(),GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
#endif
    if(h!=BadHandle){ ::CloseHandle(h); return true; }
#else
    FXInputHandle h=::open(file.text(),O_CREAT|O_WRONLY|O_TRUNC|O_EXCL,perm);
    if(h!=BadHandle){ ::close(h); return true; }
#endif
    }
  return false;
  }

/*******************************************************************************/

// Link file
FXbool FXFile::link(const FXString& srcfile,const FXString& dstfile){
  if(srcfile!=dstfile){
#if defined(WIN32)
#if defined(UNICODE)
    FXnchar srcname[MAXPATHLEN];
    FXnchar dstname[MAXPATHLEN];
    utf2ncs(srcname,srcfile.text(),MAXPATHLEN);
    utf2ncs(dstname,dstfile.text(),MAXPATHLEN);
    return CreateHardLinkW(dstname,srcname,nullptr)!=0;
#else
    return CreateHardLinkA(dstfile.text(),srcfile.text(),nullptr)!=0;
#endif
#else
    return ::link(srcfile.text(),dstfile.text())==0;
#endif
    }
  return false;
  }


// Read symbolic link
FXString FXFile::symlink(const FXString& file){
  if(!file.empty()){
#if !defined(WIN32)
    FXchar lnk[MAXPATHLEN+1];
    FXint len=::readlink(file.text(),lnk,MAXPATHLEN);
    if(0<=len){
      return FXString(lnk,len);
      }
#endif
    }
  return FXString::null;
  }


// Symbolic Link file
FXbool FXFile::symlink(const FXString& srcfile,const FXString& dstfile){
  if(dstfile!=srcfile){
#if !defined(WIN32)
    return ::symlink(srcfile.text(),dstfile.text())==0;
#endif
    }
  return false;
  }

/*******************************************************************************/

// Return true if files are identical (identical node on disk)
FXbool FXFile::identical(const FXString& file1,const FXString& file2){
  if(file1!=file2){
    FXStat info1;
    FXStat info2;
    if(FXStat::statFile(file1,info1) && FXStat::statFile(file2,info2)){
      return info1.index()==info2.index() && info1.volume()==info2.volume();
      }
    return false;
    }
  return true;
  }

/*******************************************************************************/

// Copy srcfile to dstfile, overwriting dstfile if allowed
FXbool FXFile::copy(const FXString& srcfile,const FXString& dstfile,FXbool overwrite){
  if(srcfile!=dstfile){
    FXFile src(srcfile,FXIO::Reading);
    if(src.isOpen()){
      FXStat stat;
      if(FXStat::stat(src,stat)){
        FXFile dst(dstfile,overwrite?FXIO::Writing:FXIO::Writing|FXIO::Exclusive,stat.mode());
        if(dst.isOpen()){
          FXuchar buffer[4096];
          FXival  nwritten;
          FXival  nread;
          while(1){
            nread=src.readBlock(buffer,sizeof(buffer));
            if(nread<0) return false;
            if(nread==0) break;
            nwritten=dst.writeBlock(buffer,nread);
            if(nwritten<nread) return false;
            }
          return true;
          }
        }
      }
    }
  return false;
  }


// Recursively copy files or directories from srcfile to dstfile, overwriting dstfile if allowed
FXbool FXFile::copyFiles(const FXString& srcfile,const FXString& dstfile,FXbool overwrite){
  if(srcfile!=dstfile){
    FXStat srcstat;
    FXStat dststat;

    // Source file information
    if(FXStat::statLink(srcfile,srcstat)){

      // Destination file information
      if(FXStat::statLink(dstfile,dststat)){

        // Destination is a directory?
        if(!dststat.isDirectory()){
          if(!overwrite) return false;
          if(!FXFile::remove(dstfile)) return false;
          }
        }

      // Source is a directory
      if(srcstat.isDirectory()){
        FXString name;
        FXDir dir;

        // Open source directory
        if(!dir.open(srcfile)) return false;

        // Make destination directory if needed
        if(!dststat.isDirectory()){

          // Make directory
          if(!FXDir::create(dstfile,srcstat.mode()|FXIO::OwnerWrite)) return false;
          }

        // Copy contents of source directory
        while(dir.next(name)){

          // Skip '.' and '..'
          if(name[0]=='.' && (name[1]=='\0' || (name[1]=='.' && name[2]=='\0'))) continue;

          // Recurse
          if(!FXFile::copyFiles(srcfile+PATHSEP+name,dstfile+PATHSEP+name,overwrite)) return false;
          }

        // OK
        return true;
        }

      // Source is a file
      if(srcstat.isFile()){

        // Simply copy
        if(!FXFile::copy(srcfile,dstfile,overwrite)) return false;

        // OK
        return true;
        }

      // Source is symbolic link: make a new one
      if(srcstat.isLink()){
        FXString lnkfile=FXFile::symlink(srcfile);

        // New symlink to whatever old one referred to
        if(!FXFile::symlink(lnkfile,dstfile)) return false;

        // OK
        return true;
        }

      // Source is fifo: make a new one
      if(srcstat.isFifo()){

        // Make named pipe
        if(!FXPipe::create(dstfile,srcstat.mode())) return false;

        // OK
        return true;
        }

      // Source is device/socket; only on UNIX
#if !defined(WIN32)
      if(srcstat.isDevice() || srcstat.isSocket()){
        struct stat data;
        if(::lstat(srcfile.text(),&data)==0){
          return ::mknod(dstfile.text(),data.st_mode,data.st_rdev)==0;
          }
        }
#endif
      }
    }
  return false;
  }

/*******************************************************************************/

// Move or rename srcfile to dstfile, overwriting dstfile if allowed
FXbool FXFile::move(const FXString& srcfile,const FXString& dstfile,FXbool overwrite){
  if(srcfile!=dstfile){
#if defined(WIN32)
#if defined(UNICODE)
    FXnchar srcname[MAXPATHLEN];
    FXnchar dstname[MAXPATHLEN];
    utf2ncs(srcname,srcfile.text(),MAXPATHLEN);
    utf2ncs(dstname,dstfile.text(),MAXPATHLEN);
    return ::MoveFileExW(srcname,dstname,overwrite?MOVEFILE_REPLACE_EXISTING:0)!=0;
#else
    return ::MoveFileExA(srcfile.text(),dstfile.text(),overwrite?MOVEFILE_REPLACE_EXISTING:0)!=0;
#endif
#else
    if(overwrite || !FXStat::exists(dstfile)){
      return ::rename(srcfile.text(),dstfile.text())==0;
      }
#endif
    }
  return false;
  }


// Recursively copy or move files or directories from srcfile to dstfile, overwriting dstfile if allowed
FXbool FXFile::moveFiles(const FXString& srcfile,const FXString& dstfile,FXbool overwrite){
  if(srcfile!=dstfile){
    if(FXFile::move(srcfile,dstfile,overwrite)) return true;
    if(FXFile::copyFiles(srcfile,dstfile,overwrite)){
      return FXFile::removeFiles(srcfile,true);
      }
    }
  return false;
  }

/*******************************************************************************/

// Remove a file
FXbool FXFile::remove(const FXString& file){
  if(!file.empty()){
#if defined(WIN32)
#if defined(UNICODE)
    FXnchar unifile[MAXPATHLEN];
    utf2ncs(unifile,file.text(),MAXPATHLEN);
    return ::DeleteFileW(unifile)!=0;
#else
    return ::DeleteFileA(file.text())!=0;
#endif
#else
    return ::unlink(file.text())==0;
#endif
    }
  return false;
  }


// Remove file or directory, recursively if allowed
FXbool FXFile::removeFiles(const FXString& path,FXbool recursive){
  FXStat stat;
  if(FXStat::statLink(path,stat)){
    if(stat.isDirectory()){
      if(recursive){
        FXDir dir(path);
        FXString name;
        while(dir.next(name)){
          if(name[0]=='.' && (name[1]=='\0' || (name[1]=='.' && name[2]=='\0'))) continue;
          if(!FXFile::removeFiles(path+PATHSEP+name,true)) return false;
          }
        }
      return FXDir::remove(path);
      }
    return FXFile::remove(path);
    }
  return false;
  }

/*******************************************************************************/

// Concatenate srcfile1 and srcfile2 to dstfile, overwriting dstfile if allowed
FXbool FXFile::concat(const FXString& srcfile1,const FXString& srcfile2,const FXString& dstfile,FXbool overwrite){
  FXuchar buffer[4096]; FXival nwritten,nread;
  if(srcfile1!=dstfile && srcfile2!=dstfile){
    FXFile src1(srcfile1,FXIO::Reading);
    if(src1.isOpen()){
      FXFile src2(srcfile2,FXIO::Reading);
      if(src2.isOpen()){
        FXFile dst(dstfile,overwrite?FXIO::Writing:FXIO::Writing|FXIO::Exclusive);
        if(dst.isOpen()){
          while(1){
            nread=src1.readBlock(buffer,sizeof(buffer));
            if(nread<0) return false;
            if(nread==0) break;
            nwritten=dst.writeBlock(buffer,nread);
            if(nwritten<0) return false;
            }
          while(1){
            nread=src2.readBlock(buffer,sizeof(buffer));
            if(nread<0) return false;
            if(nread==0) break;
            nwritten=dst.writeBlock(buffer,nread);
            if(nwritten<0) return false;
            }
          return true;
          }
        }
      }
    }
  return false;
  }

}
