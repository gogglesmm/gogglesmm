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
#include "xincs.h"
#include "fxdefs.h"
#include "fxchar.h"
#include "fxmath.h"
#include "FXString.h"
#include "FXIODevice.h"
#include "FXStat.h"
#include "FXMappedFile.h"

/*
  Notes:
  - Memory-mapped file implementation.
*/

// Bad handle value
#if defined(WIN32)
#define BadHandle INVALID_HANDLE_VALUE
#else
#define BadHandle -1
#endif

using namespace FX;

/*******************************************************************************/

namespace FX {


// Create new map object
FXMappedFile::FXMappedFile():memhandle(BadHandle),mempointer(nullptr),memlength(0),memoffset(0){
  }


#if defined(WIN32)


// Open file and map it
FXptr FXMappedFile::open(const FXString& filename,FXuint m,FXuint perm,FXlong len,FXlong off){
  if(device==BadHandle){

    // Basic access mode
    DWORD access=0;
    switch(m&(FXIO::ReadOnly|FXIO::WriteOnly)){
      case FXIO::ReadOnly: access=GENERIC_READ; break;
      case FXIO::WriteOnly: access=GENERIC_WRITE; break;
      case FXIO::ReadWrite: access=GENERIC_READ|GENERIC_WRITE; break;
      }

    // Creation and truncation mode
    DWORD creation=OPEN_EXISTING;
    switch(m&(FXIO::Create|FXIO::Truncate|FXIO::Exclusive)){
      case FXIO::Create: creation=OPEN_ALWAYS; break;
      case FXIO::Exclusive: creation=CREATE_NEW; break;
      case FXIO::Truncate: creation=TRUNCATE_EXISTING; break;
      case FXIO::Create|FXIO::Truncate: creation=CREATE_ALWAYS; break;
      case FXIO::Create|FXIO::Exclusive: creation=CREATE_NEW; break;
      case FXIO::Create|FXIO::Truncate|FXIO::Exclusive: creation=CREATE_NEW; break;
      }

    // Attributes (hidden, read-only)
    DWORD attributes=FILE_ATTRIBUTE_NORMAL;
    if(!(perm&FXIO::AllWrite)){ attributes=(attributes&~FILE_ATTRIBUTE_NORMAL)|FILE_ATTRIBUTE_READONLY; }
    if(perm&FXIO::Hidden){ attributes=(attributes&~FILE_ATTRIBUTE_NORMAL)|FILE_ATTRIBUTE_HIDDEN; }

    // Inheritable
    SECURITY_ATTRIBUTES security;
    security.nLength=sizeof(SECURITY_ATTRIBUTES);
    security.bInheritHandle=((m&FXIO::Inheritable)==0);
    security.lpSecurityDescriptor=nullptr;

    // Open file
#if defined(UNICODE)
    FXnchar unifile[MAXPATHLEN];
    utf2ncs(unifile,filename.text(),MAXPATHLEN);
    device=::CreateFileW(unifile,access,FILE_SHARE_READ|FILE_SHARE_WRITE,&security,creation,attributes,nullptr);
#else
    device=::CreateFileA(filename.text(),access,FILE_SHARE_READ|FILE_SHARE_WRITE,&security,creation,attributes,nullptr);
#endif
    if(device!=BadHandle){

      // Prior file size
      FXlong filesize=size();

      // Make it bigger?
      if(filesize<off+len){
        filesize=off+len;
        }

      // Whole thing?
      if(len==0){
        len=filesize-off;
        }

      // Set access flags
      DWORD protect=0;
      switch(m&(FXIO::ReadOnly|FXIO::WriteOnly|FXIO::Executable)){
        case FXIO::ReadOnly: protect=PAGE_READONLY; break;
        case FXIO::ReadOnly|FXIO::Executable: protect=PAGE_EXECUTE_READ; break;
        case FXIO::WriteOnly: protect=PAGE_READWRITE; break;
        case FXIO::WriteOnly|FXIO::Executable: protect=PAGE_EXECUTE_READWRITE; break;
        case FXIO::ReadOnly|FXIO::WriteOnly: protect=PAGE_READWRITE; break;
        case FXIO::ReadOnly|FXIO::WriteOnly|FXIO::Executable: protect=PAGE_EXECUTE_READWRITE; break;
        }

      // Create mapping
      FXInputHandle hnd=::CreateFileMapping(device,nullptr,protect,(DWORD)((off+len)>>32),(DWORD)((off+len)&0xFFFFFFFF),nullptr);
      if(hnd!=BadHandle){

        // Protection bits
        DWORD bits=0;
        switch(m&(FXIO::ReadOnly|FXIO::WriteOnly|FXIO::Executable)){
          case FXIO::ReadOnly: bits=FILE_MAP_READ; break;
          case FXIO::ReadOnly|FXIO::Executable: bits=FILE_MAP_READ|FILE_MAP_EXECUTE; break;
          case FXIO::WriteOnly: bits=FILE_MAP_ALL_ACCESS; break;
          case FXIO::WriteOnly|FXIO::Executable: bits=FILE_MAP_ALL_ACCESS|FILE_MAP_EXECUTE; break;
          case FXIO::ReadWrite: bits=FILE_MAP_ALL_ACCESS; break;
          case FXIO::ReadWrite|FXIO::Executable: bits=FILE_MAP_ALL_ACCESS|FILE_MAP_EXECUTE; break;
          }

        // Open map
        FXptr ptr=::MapViewOfFile(hnd,bits,(DWORD)(off>>32),(DWORD)(off&0xFFFFFFFF),(DWORD)len);

        if(ptr!=nullptr){
          // MEMORY_BASIC_INFORMATION mbi;
          // if(VirtualQuery(ptr,&mbi,sizeof(mbi))!=0){ memlength=mbi.RegionSize; }
          memhandle=hnd;
          mempointer=ptr;
          memlength=len;
          memoffset=off;
          return mempointer;
          }
        ::CloseHandle(hnd);
        }
      ::CloseHandle(device);
      device=BadHandle;
      }
    }
  return nullptr;
  }


#else


// Open file and map it
FXptr FXMappedFile::open(const FXString& filename,FXuint m,FXuint perm,FXlong len,FXlong off){
  if(device==BadHandle){

    // Access modes
    FXint flags=0;
    switch(m&(FXIO::ReadOnly|FXIO::WriteOnly)){
      case FXIO::ReadOnly: flags=O_RDONLY; break;
      case FXIO::WriteOnly: flags=O_WRONLY; break;
      case FXIO::ReadWrite: flags=O_RDWR; break;
      }

    // Truncate it
    if(m&FXIO::Truncate){ flags|=O_TRUNC; }

    // Change access time
#if defined(O_NOATIME)
    if(m&FXIO::NoAccessTime){ flags|=O_NOATIME; }
#endif

    // Inheritable
#if defined(O_CLOEXEC)
    if(!(m&FXIO::Inheritable)){ flags|=O_CLOEXEC; }
#endif

    // Creation mode
    if(m&FXIO::Create){
      flags|=O_CREAT;
      if(m&FXIO::Exclusive){ flags|=O_EXCL; }
      }

    // Permission bits
    FXint bits=perm&0777;
    if(perm&FXIO::SetUser){ bits|=S_ISUID; }
    if(perm&FXIO::SetGroup){ bits|=S_ISGID; }
    if(perm&FXIO::Sticky){ bits|=S_ISVTX; }

    // Open file
    device=::open(filename.text(),flags,bits);
    if(device!=BadHandle){

      // Prior file size
      FXlong filesize=size();

      // Make it bigger?
      if(filesize<off+len){
        if(::ftruncate(device,off+len)!=0) goto fail;
        filesize=off+len;
        }

      // Whole whole thing?
      if(len==0){
        len=filesize-off;
        }

      // Non-zero map?
      if(0<len){

        // Protection bits
        FXint protect=PROT_NONE;
        switch(m&(FXIO::ReadOnly|FXIO::WriteOnly|FXIO::Executable)){
          case FXIO::ReadOnly: protect=PROT_READ; break;
          case FXIO::ReadOnly|FXIO::Executable: protect=PROT_READ|PROT_EXEC; break;
          case FXIO::WriteOnly: protect=PROT_WRITE; break;
          case FXIO::WriteOnly|FXIO::Executable: protect=PROT_WRITE|PROT_EXEC; break;
          case FXIO::ReadWrite: protect=PROT_READ|PROT_WRITE; break;
          case FXIO::ReadWrite|FXIO::Executable: protect=PROT_READ|PROT_WRITE|PROT_EXEC; break;
          }

        // Open map
        FXptr ptr=::mmap(nullptr,len,protect,MAP_SHARED,device,off);
        if(ptr!=MAP_FAILED){
#if defined(F_ADD_SEALS)
          //::fcntl(device,F_ADD_SEALS,F_SEAL_SHRINK|F_SEAL_GROW);   // Prevent size changes from here on
#endif
          mempointer=ptr;
          memlength=len;
          memoffset=off;
          return mempointer;
          }
        }
fail: ::close(device);
      device=BadHandle;
      }
    }
  return nullptr;
  }


#endif


// Return true if serial access only
FXbool FXMappedFile::isSerial() const {
  return false;
  }


// Return file size
FXlong FXMappedFile::size(){
  if(device!=BadHandle){
#if defined(WIN32)
    LARGE_INTEGER result;
    if(::GetFileSizeEx(device,&result)) return result.QuadPart;
#else
    struct stat data;
    if(::fstat(device,&data)==0) return data.st_size;
#endif
    }
  return FXIO::Error;
  }


// Synchronize disk
FXbool FXMappedFile::flush(){
  if(mempointer!=nullptr){
#if defined(WIN32)
    return ::FlushViewOfFile(mempointer,memlength)!=0;
#else
    return ::msync(mempointer,memlength,MS_SYNC|MS_INVALIDATE)==0;
#endif
    }
  return false;
  }


// Close file, and also the map
FXbool FXMappedFile::close(){
  if(mempointer!=nullptr){
#if defined(WIN32)
    if(::UnmapViewOfFile(mempointer)!=0){
      ::CloseHandle(memhandle);
      memhandle=BadHandle;
      mempointer=nullptr;
      memlength=0;
      memoffset=0;
      return FXIODevice::close();
      }
#else
    if(::munmap(mempointer,memlength)==0){
      memhandle=BadHandle;
      mempointer=nullptr;
      memlength=0;
      memoffset=0;
      return FXIODevice::close();
      }
#endif
    }
  return false;
  }


// Return memory mapping granularity
FXival FXMappedFile::granularity(){
#if defined(WIN32)
  SYSTEM_INFO SystemInfo;
  GetSystemInfo(&SystemInfo);
  return SystemInfo.dwAllocationGranularity;
#else
  return sysconf(_SC_PAGE_SIZE);
#endif
  }


// Delete the mapping
FXMappedFile::~FXMappedFile(){
  close();
  }

}
