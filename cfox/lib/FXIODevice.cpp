/********************************************************************************
*                                                                               *
*                        I / O   D e v i c e   C l a s s                        *
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
#include "fxmath.h"
#include "fxascii.h"
#include "FXArray.h"
#include "FXHash.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXIO.h"
#include "FXIODevice.h"


/*
  Notes:

  - An abstract class for low-level IO.
  - You can change some access mode flags of the file descriptor.
*/

// Bad handle value
#ifdef WIN32
#define BadHandle INVALID_HANDLE_VALUE
#else
#define BadHandle -1
#endif

using namespace FX;

/*******************************************************************************/

namespace FX {



// Construct
FXIODevice::FXIODevice():device(BadHandle){
  }


// Construct with given handle and mode
FXIODevice::FXIODevice(FXInputHandle h,FXuint m):device(BadHandle){
  attach(h,m);
  }


// Return true if handle is valid
static FXbool isvalid(FXInputHandle h){
#if defined(WIN32)
  DWORD flags;
  return GetHandleInformation(h,&flags)!=0;
#else
//  return (fcntl(h,F_GETFD)!=-1) || (errno!=EBADF);
  return (fcntl(h,F_GETFD,0)!=-1) || (errno!=EBADF);
#endif
  }


#if 0
#if defined(WIN32)
  DWORD flags;
  FXuint mm=0;
  if(GetHandleInformation(h,&flags)==0){ return false }
  if(flags&HANDLE_FLAG_INHERIT){ mm|=FXIO::Inheritable; }
//  FILE_ATTRIBUTE_TAG_INFO attribs;
//  if(GetFileInformationByHandleEx(h,FileAttributeTagInfo,&attribs,sizeof(attribs))==0){ return false; }
  // FIXME //
  //https://stackoverflow.com/questions/9442436/windows-how-do-i-get-the-mode-access-rights-of-an-already-opened-file
#else
  FXint flags=::fcntl(h,F_GETFD,0);
  FXuint mm=0;
  if(flags==-1){ return false; }
  if(flags&O_RDONLY){ mm|=FXIO::ReadOnly; }
  if(flags&O_WRONLY){ mm|=FXIO::WriteOnly; }
  if(flags&O_RDWR){ mm|=FXIO::ReadWrite; }
  if(flags&O_APPEND){ mm|=FXIO::Append; }
  if(flags&O_TRUNC){ mm|=FXIO::Truncate; }
  if(flags&O_NONBLOCK){ mm|=FXIO::NonBlocking; }
#if defined(O_NOATIME)
  if(flags&O_NOATIME){ mm|=FXIO::NoAccessTime; }
#endif
#if defined(O_CLOEXEC)
  if(!(flags&O_CLOEXEC)){ mm|=FXIO::Inheritable; }
#endif
  if(flags&O_CREAT){ mm|=FXIO::Create; }
  if(flags&O_EXCL){ mm|=FXIO::Exclusive; }
#endif
#endif

// Open device with access mode m and existing handle h
FXbool FXIODevice::open(FXInputHandle h,FXuint m){
  if(__likely(h!=BadHandle && isvalid(h))){
    device=h;
    access=m;
    pointer=0L;
    return true;
    }
  return false;
  }


// Change access mode of open device
FXbool FXIODevice::setMode(FXuint m){
  if(__likely(device!=BadHandle)){
#if defined(WIN32)
    FXint flags=0;
    if(m&Inheritable) flags=HANDLE_FLAG_INHERIT;
    if(::SetHandleInformation(device,HANDLE_FLAG_INHERIT,flags)!=0) return false;
    access^=(access^m)&Inheritable;
    return true;
#else
    FXint flags=0;
    if(m&NonBlocking) flags|=O_NONBLOCK;
    if(m&Append) flags|=O_APPEND;
#if defined(O_NOATIME)
    if(m&NoAccessTime) flags|=O_NOATIME;
#endif
    if(::fcntl(device,F_SETFL,flags)<0) return false;
#if defined(O_CLOEXEC)
    flags=O_CLOEXEC;
    if(m&Inheritable) flags=0;
    if(::fcntl(device,F_SETFD,flags)<0) return false;
#endif
    access^=(access^m)&(NonBlocking|Append|NoAccessTime|Inheritable);
    return true;
#endif
    }
  return false;
  }


// Return true if open
FXbool FXIODevice::isOpen() const {
  return device!=BadHandle;
  }


// Return true if serial access only
FXbool FXIODevice::isSerial() const {
  return true;
  }


// Attach existing file handle
FXbool FXIODevice::attach(FXInputHandle h,FXuint m){
  if(__likely(h!=BadHandle && isvalid(h))){
    close();
    device=h;
    access=(m|OwnHandle);
    pointer=0L;
    return true;
    }
  return false;
  }


// Detach existing file handle
FXbool FXIODevice::detach(){
  device=BadHandle;
  access=NoAccess;
  pointer=0L;
  return true;
  }


// Get position
FXlong FXIODevice::position() const {
  return pointer;
  }


// Move to position
FXlong FXIODevice::position(FXlong,FXuint){
  return FXIO::Error;
  }


// Read block
FXival FXIODevice::readBlock(void* ptr,FXival count){
  if(__likely(device!=BadHandle) && __likely(access&ReadOnly)){
#if defined(WIN32)
    DWORD nread;
    if(::ReadFile(device,ptr,(DWORD)count,&nread,nullptr)==0){
      if(GetLastError()==ERROR_IO_PENDING) return FXIO::Again;
      return FXIO::Error;
      }
    pointer+=nread;
    return nread;
#else
    FXival nread;
a:  nread=::read(device,ptr,count);
    if(__unlikely(nread<0)){
      if(errno==EINTR) goto a;
      if(errno==EAGAIN) return FXIO::Again;
      if(errno==EWOULDBLOCK) return FXIO::Again;
      return FXIO::Error;
      }
    pointer+=nread;
    return nread;
#endif
    }
  return FXIO::Error;
  }


// Write block
FXival FXIODevice::writeBlock(const void* ptr,FXival count){
  if(__likely(device!=BadHandle) && __likely(access&WriteOnly)){
#if defined(WIN32)
    DWORD nwritten;
    if(::WriteFile(device,ptr,(DWORD)count,&nwritten,nullptr)==0){
      return FXIO::Error;
      }
    pointer+=nwritten;
    return nwritten;
#else
    FXival nwritten;
a:  nwritten=::write(device,ptr,count);
    if(__unlikely(nwritten<0)){
      if(errno==EINTR) goto a;
      if(errno==EAGAIN) return FXIO::Again;
      if(errno==EWOULDBLOCK) return FXIO::Again;
      if(errno==EPIPE) return FXIO::Broken;
      return FXIO::Error;
      }
    pointer+=nwritten;
    return nwritten;
#endif
    }
  return FXIO::Error;
  }


// Truncate file
FXlong FXIODevice::truncate(FXlong){
  return FXIO::Error;
  }


// Synchronize disk with cached data
FXbool FXIODevice::flush(){
  return false;
  }


// Test if we're at the end; -1 if error
FXint FXIODevice::eof(){
  return FXIO::Error;
  }


// Return file size
FXlong FXIODevice::size(){
  return FXIO::Error;
  }


// Close file
FXbool FXIODevice::close(){
  if(__likely(device!=BadHandle)){
    if(access&OwnHandle){
#if defined(WIN32)
      if(::CloseHandle(device)!=0){
        device=BadHandle;
        access=NoAccess;
        pointer=0L;
        return true;
        }
#else
      if(::close(device)==0){
        device=BadHandle;
        access=NoAccess;
        pointer=0L;
        return true;
        }
#endif
      }
    device=BadHandle;
    access=NoAccess;
    pointer=0L;
    }
  return false;
  }


// Destroy
FXIODevice::~FXIODevice(){
  close();
  }


}

