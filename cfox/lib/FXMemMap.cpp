/********************************************************************************
*                                                                               *
*                      M e m o r y   M a p p e d   F i l e                      *
*                                                                               *
*********************************************************************************
* Copyright (C) 2004,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "fxmath.h"
#include "FXString.h"
#include "FXIODevice.h"
#include "FXStat.h"
#include "FXFile.h"
#include "FXMemMap.h"

/*
  Notes:
  - A memory mapped region of a file, or anonymous memory map.
  - Maybe use long sz = sysconf(_SC_PAGESIZE);
  - msync().
  - Need to bring in line with FXIO esp. with interpretation of options and so on.
  - Need API's to open with existing file handles.
*/


#ifdef WIN32
#define BadHandle INVALID_HANDLE_VALUE
#else
#define BadHandle -1
#endif


using namespace FX;

/*******************************************************************************/

namespace FX {


// Create new map object
FXMemMap::FXMemMap():maphandle(BadHandle),mapbase(nullptr),mapoffset(0L),maplength(0){
  }


// Open file and map it
void *FXMemMap::openMap(const FXString& filename,FXlong off,FXival len,FXuint m,FXuint p){
  if(open(filename,m,p)){
    void* result=map(off,len);
    if(result){
      return result;
      }
    close();
    }
  return nullptr;
  }


// Attach to existing file handle and map it
void* FXMemMap::openMap(FXInputHandle h,FXlong off,FXival len,FXuint m){
  if(open(h,m)){
    void* result=map(off,len);
    if(result){
      return result;
      }
    close();
    }
  return nullptr;
  }


// Map an already open file
void *FXMemMap::map(FXlong off,FXival len){
  if(isOpen()){

    // Get file size
    FXlong filesize=size();
    if(0<=filesize){

#ifdef WIN32

      // Map whole file
      if(len==-1) len=filesize-off;

      // Set access flags
      DWORD prot=0;
      if(access&ReadOnly){ prot=PAGE_READONLY; }
      if(access&WriteOnly){ prot=PAGE_READWRITE; }

      DWORD flag=0;
      if(access&ReadOnly){ flag=FILE_MAP_READ; }
      if(access&WriteOnly){ flag=FILE_MAP_WRITE; }

      // Now map it
      FXInputHandle hnd=::CreateFileMapping(handle(),nullptr,prot,0,len,nullptr);
      if(hnd!=nullptr){
        FXuchar* ptr=(FXuchar*)::MapViewOfFile(hnd,flag,0,off,len);
        if(ptr!=nullptr){
          maphandle=hnd;
          mapbase=ptr;
          maplength=len;
          mapoffset=off;
          pointer=off;
          return mapbase;
          }
        ::CloseHandle(hnd);
        }
#else

      // Map whole file
      if(len==-1) len=filesize-off;

      // Trying to map region larger than the file
      if(filesize<off+len){
        if(access&WriteOnly){
          truncate(off+len);            // Extends the file if writing
          }
        else{
          len=filesize-off;             // Map smaller region when reading
          }
        }

      // Set access flags
      FXint prot=PROT_NONE;
      if(access&ReadOnly){ prot|=PROT_READ; }
      if(access&WriteOnly){ prot|=PROT_WRITE|PROT_READ; }
      if(access&Executable){ prot|=PROT_EXEC; }

      // Now map it
      FXuchar* ptr=(FXuchar*)::mmap(nullptr,len,prot,MAP_SHARED,handle(),off);
      if(ptr!=MAP_FAILED){
        mapbase=ptr;
        maplength=len;
        mapoffset=off;
        pointer=off;
        return mapbase;
        }
#endif
      }
    }
  return nullptr;
  }


// Unmap the view of the file
void* FXMemMap::unmap(){
  if(mapbase){
#ifdef WIN32
    ::UnmapViewOfFile(mapbase);
    ::CloseHandle(maphandle);
#else
    ::munmap(mapbase,maplength);
#endif
    maphandle=BadHandle;
    mapbase=nullptr;
    mapoffset=0L;
    pointer=0L;
    maplength=0;
    }
  return nullptr;
  }


// Get current file position
FXlong FXMemMap::position() const {
  return pointer;
  }


// Change file position, returning new position from start
FXlong FXMemMap::position(FXlong off,FXuint from){
  if(mapbase){
    if(from==Current) off=pointer+off;
    else if(from==End) off=mapoffset+maplength+off;     // FIXME is this what we want?
    if(mapoffset<=off && off<=mapoffset+maplength){
      pointer=off;
      return pointer;
      }
    }
  return -1;
  }


// Read block of bytes, returning number of bytes read
FXival FXMemMap::readBlock(void* ptr,FXival count){
  if(mapbase && mapoffset<=pointer && pointer<=mapoffset+maplength){
    if(pointer+count>mapoffset+maplength) count=mapoffset+maplength-pointer;
    memmove(ptr,mapbase+pointer-mapoffset,count);
    pointer+=count;
    return count;
    }
  return -1;
  }


// Write block of bytes, returning number of bytes written
FXival FXMemMap::writeBlock(const void* ptr,FXival count){
  if(mapbase && mapoffset<=pointer && pointer<=mapoffset+maplength){
    if(pointer+count>mapoffset+maplength) count=mapoffset+maplength-pointer;
    memmove(mapbase+pointer-mapoffset,ptr,count);
    pointer+=count;
    return count;
    }
  return -1;
  }


// Synchronize disk
FXbool FXMemMap::flush(){
  if(mapbase){
#ifdef WIN32
    return ::FlushViewOfFile(mapbase,(size_t)maplength)!=0;
#else
#ifndef __minix
    return ::msync((char*)mapbase,(size_t)maplength,MS_SYNC|MS_INVALIDATE)==0;
#endif
#endif
    }
  return false;
  }


// Close file, and also the map
FXbool FXMemMap::close(){
  unmap();
  return FXFile::close();
  }


// Delete the mapping
FXMemMap::~FXMemMap(){
  close();
  }


}

