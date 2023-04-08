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
FXIODevice::FXIODevice(FXInputHandle h):device(BadHandle){
  attach(h);
  }


// Return true if open
FXbool FXIODevice::isOpen() const {
  return device!=BadHandle;
  }


// Grab access attributes from the open handle.
// There doesn't seem to be a way to do that on Windows except
// through the use of the undocumented NtQueryObject() function.
// At the first call, dig up the function pointer and keep it
// for the next time.
#if defined(WIN32)

// Declare NtQueryObject()
typedef NTSTATUS (WINAPI *PFN_NTQUERYOBJECT)(HANDLE,OBJECT_INFORMATION_CLASS,void*,ULONG,ULONG*);

// Declare the stub function
static NTSTATUS WINAPI StubNtQueryObject(HANDLE,OBJECT_INFORMATION_CLASS,void*,ULONG,ULONG*);

// Pointer to (stub for) NtQueryObject
static PFN_NTQUERYOBJECT fxNtQueryObject=StubNtQueryObject;

// The stub gets the address of actual function, sets the function pointer, then calls
// actual function; next time around actual function will be called directly.
static NTSTATUS WINAPI StubNtQueryObject(HANDLE hnd,OBJECT_INFORMATION_CLASS oic,void* oi,ULONG oilen,ULONG* len){
  if(fxNtQueryObject==StubNtQueryObject){
    HMODULE ntdllDll=GetModuleHandleA("ntdll.dll");
    FXASSERT(ntdllDll);
    fxNtQueryObject=(PFN_NTQUERYOBJECT)GetProcAddress(ntdllDll,"NtQueryObject");
    FXASSERT(fxNtQueryObject);
    }
  return fxNtQueryObject(hnd,oic,oi,oilen,len);
  }

#endif


// Return access mode
FXuint FXIODevice::mode() const {
  if(device!=BadHandle){
#if defined(WIN32)
    FXuint result=FXIO::NoAccess;
    PUBLIC_OBJECT_BASIC_INFORMATION obi;
    DWORD flags=0;
    if(GetHandleInformation(device,&flags)!=0){
      result=FXIO::ReadWrite;
      if(flags&HANDLE_FLAG_INHERIT){ result|=FXIO::Inheritable; }
/*
      if(fxNtQueryObject(device,ObjectBasicInformation,&obi,sizeof(obi),nullptr)>=0){
        //FXTRACE((100,"obi.Attributes     = %08x\n",obi.Attributes));
        //FXTRACE((100,"obi.GrantedAccess  = %08x\n",obi.GrantedAccess));
        //FXTRACE((100,"obi.GrantedAccess  = %08x\n",obi.GrantedAccess));
        //FXTRACE((100,"obi.HandleCount    = %08x\n",obi.HandleCount));
        //FXTRACE((100,"obi.PointerCount   = %08x\n",obi.PointerCount));
        if(obi.GrantedAccess&GENERIC_READ){ result|=FXIO::ReadOnly; }
        if(obi.GrantedAccess&GENERIC_WRITE){ result|=FXIO::WriteOnly; }
        if(obi.GrantedAccess&GENERIC_ALL){ result|=FXIO::ReadWrite; }
        if(obi.GrantedAccess&GENERIC_EXECUTE){ result|=FXIO::Executable; }
        }
      FXTRACE((100,"result             = %08x\n",result));
*/
      return result;
      }
#else
    FXuint result=FXIO::Inheritable;
    FXint flags=::fcntl(device,F_GETFL,0);
    if(flags!=-1){
#if defined(O_NOATIME)
      if(flags&O_NOATIME){ result|=FXIO::NoAccessTime; }
#endif
      if(flags&O_APPEND){ result|=FXIO::Append; }
      if(flags&O_CREAT){ result|=FXIO::Create; }
      if(flags&O_EXCL){ result|=FXIO::Exclusive; }
      if(flags&O_RDONLY){ result|=FXIO::ReadOnly; }
      if(flags&O_WRONLY){ result|=FXIO::WriteOnly; }
      if(flags&O_TRUNC){ result|=FXIO::Truncate; }
      if(flags&O_NONBLOCK){ result|=FXIO::NonBlocking; }
#if defined(O_CLOEXEC)
      flags=::fcntl(device,F_GETFD,0);
      if(flags!=-1){
        if(flags&O_CLOEXEC){ result&=~FXIO::Inheritable; }
        }
#endif
      return result;
      }
#endif
    }
  return FXIO::Error;
  }


// Change access mode of open device
FXbool FXIODevice::mode(FXuint m){
  if(device!=BadHandle){
#if defined(WIN32)
    DWORD flags=0;
    if(m&FXIO::Inheritable) flags=HANDLE_FLAG_INHERIT;
    if(::SetHandleInformation(device,HANDLE_FLAG_INHERIT,flags)){
      return true;
      }
#else
    FXint flags=0;
    if(m&FXIO::Append){ flags|=O_APPEND; }
    if(m&FXIO::Truncate){ flags|=O_TRUNC; }
    if(m&FXIO::NonBlocking){ flags|=O_NONBLOCK; }
#if defined(O_NOATIME)
    if(m&FXIO::NoAccessTime){ flags|=O_NOATIME; }
#endif
    if(::fcntl(device,F_SETFL,flags)==0){
#if defined(O_CLOEXEC)
      flags=O_CLOEXEC;
      if(m&FXIO::Inheritable) flags=0;
      if(::fcntl(device,F_SETFD,flags)==0){
        return true;
        }
#else
      return true;
#endif
      }
#endif
    }
  return false;
  }


// Return permissions
FXuint FXIODevice::perms() const {
  if(device!=BadHandle){
#if defined(WIN32)
    BY_HANDLE_FILE_INFORMATION data;
    if(::GetFileInformationByHandle(device,&data)){
      FXuint result=FXIO::AllFull;
      DWORD bits=::GetFileType(device);
      if(bits&FILE_TYPE_CHAR){ result|=FXIO::Character; }
      if(bits&FILE_TYPE_DISK){ result|=FXIO::Block; }
      if(bits&FILE_TYPE_PIPE){ result|=FXIO::Fifo; }
      if(data.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY){
        result|=FXIO::Directory;
        }
      else{
        result|=FXIO::File;
        }
      if(data.dwFileAttributes&FILE_ATTRIBUTE_HIDDEN){
        result|=FXIO::Hidden;
        }
      if(data.dwFileAttributes&FILE_ATTRIBUTE_READONLY){
        result&=~FXIO::AllWrite;
        }
      return result;
      }
#else
    struct stat data;
    if(::fstat(device,&data)==0){
      FXuint result=(data.st_mode&FXIO::AllFull);
      if(S_ISDIR(data.st_mode)){ result|=FXIO::Directory; }
      if(S_ISREG(data.st_mode)){ result|=FXIO::File; }
      if(S_ISLNK(data.st_mode)){ result|=FXIO::SymLink; }
      if(S_ISCHR(data.st_mode)){ result|=FXIO::Character; }
      if(S_ISBLK(data.st_mode)){ result|=FXIO::Block; }
      if(S_ISFIFO(data.st_mode)){ result|=FXIO::Fifo; }
      if(S_ISSOCK(data.st_mode)){ result|=FXIO::Socket; }
      if(data.st_mode&S_ISUID){ result|=FXIO::SetUser; }
      if(data.st_mode&S_ISGID){ result|=FXIO::SetGroup; }
      if(data.st_mode&S_ISVTX){ result|=FXIO::Sticky; }
      return result;
      }
#endif
    }
  return FXIO::Error;
  }


// Set permissions
FXbool FXIODevice::perms(FXuint p){
  if(device!=BadHandle){
#if defined(WIN32)
/*
    BY_HANDLE_FILE_INFORMATION data;
    if(::GetFileInformationByHandle(device,&data)){
      FILE_BASIC_INFO info;
      info.CreationTime.LowPart=data.ftCreationTime.dwLowDateTime;
      info.CreationTime.HighPart=data.ftCreationTime.dwHighDateTime;
      info.LastAccessTime.LowPart=data.ftLastAccessTime.dwLowDateTime;
      info.LastAccessTime.HighPart=data.ftLastAccessTime.dwHighDateTime;
      info.LastWriteTime.LowPart=data.ftLastWriteTime.dwLowDateTime;
      info.LastWriteTime.HighPart=data.ftLastWriteTime.dwHighDateTime;
      info.ChangeTime.LowPart=data.ftLastWriteTime.dwLowDateTime;       // ??
      info.ChangeTime.HighPart=data.ftLastWriteTime.dwHighDateTime;
      info.FileAttributes=data.dwFileAttributes;        // FIXME with changes
      if(p&FXIO::Hidden) info.FileAttributes|=FILE_ATTRIBUTE_HIDDEN; else info.FileAttributes&=~FILE_ATTRIBUTE_HIDDEN;
      if(!(p&FXIO::AllWrite)) info.FileAttributes|=FILE_ATTRIBUTE_READONLY; else info.FileAttributes&=~FILE_ATTRIBUTE_READONLY;
      return ::SetFileInformationByHandle(device,FileBasicInfo,&info,sizeof(info))!=0;
      }
*/
#else
    FXint bits=p&0777;
    if(p&FXIO::SetUser){ bits|=S_ISUID; }
    if(p&FXIO::SetGroup){ bits|=S_ISGID; }
    if(p&FXIO::Sticky){ bits|=S_ISVTX; }
    return (::fchmod(device,bits)==0);
#endif
    }
  return false;
  }


// Return true if handle is valid
FXbool FXIODevice::valid(FXInputHandle hnd){
  if(hnd!=BadHandle){
#if defined(WIN32)
    DWORD flags;
    return ::GetHandleInformation(hnd,&flags)!=0;
#else
    return ::fcntl(hnd,F_GETFD,0)>=0;
#endif
    }
  return false;
  }


// Attach existing file handle
FXbool FXIODevice::attach(FXInputHandle h){
  if(valid(h)){
    close();
    device=h;
    return true;
    }
  return false;
  }


// Detach existing file handle
FXbool FXIODevice::detach(){
  device=BadHandle;
  return true;
  }


// Read block
FXival FXIODevice::readBlock(void* ptr,FXival count){
  if(device!=BadHandle){
#if defined(WIN32)
    DWORD nread;
    if(::ReadFile(device,ptr,(DWORD)count,&nread,nullptr)==0){
      if(GetLastError()==ERROR_IO_PENDING) return FXIO::Again;
      return FXIO::Error;
      }
    return nread;
#else
    FXival nread;
a:  nread=::read(device,ptr,count);
    if(nread<0){
      if(errno==EINTR) goto a;
      if(errno==EAGAIN) return FXIO::Again;
      if(errno==EWOULDBLOCK) return FXIO::Again;
      return FXIO::Error;
      }
    return nread;
#endif
    }
  return FXIO::Error;
  }


// Write block
FXival FXIODevice::writeBlock(const void* ptr,FXival count){
  if(device!=BadHandle){
#if defined(WIN32)
    DWORD nwritten;
    if(::WriteFile(device,ptr,(DWORD)count,&nwritten,nullptr)==0){
      return FXIO::Error;
      }
    return nwritten;
#else
    FXival nwritten;
a:  nwritten=::write(device,ptr,count);
    if(nwritten<0){
      if(errno==EINTR) goto a;
      if(errno==EAGAIN) return FXIO::Again;
      if(errno==EWOULDBLOCK) return FXIO::Again;
      if(errno==EPIPE) return FXIO::Broken;
      return FXIO::Error;
      }
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
  return true;
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
  if(device!=BadHandle){
#if defined(WIN32)
    if(::CloseHandle(device)!=0){
      device=BadHandle;
      return true;
      }
#else
    if(::close(device)==0){
      device=BadHandle;
      return true;
      }
#endif
    }
  return false;
  }


// Destroy
FXIODevice::~FXIODevice(){
  close();
  }

}

