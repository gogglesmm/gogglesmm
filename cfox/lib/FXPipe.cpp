/********************************************************************************
*                                                                               *
*                             P i p e   C l a s s                               *
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
#include "fxascii.h"
#include "FXArray.h"
#include "FXHash.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXPath.h"
#include "FXIO.h"
#include "FXIODevice.h"
#include "FXPipe.h"



/*
  Notes:

  - Create a pipe for reading or writing; if created for reading,
    the "other" is the write-side.  If created for writing, the
    "other" is the read-side.

  - The pipe may be inherited by a sub-process if so indicated.

  - On Linux, we prefer to use pipe2(), elsewhere we use pipe()
    and set the flags properly afterwards, using fcntl().

  - Both our end the other end FXPipe instances should not be open
    yet prior to a call to open().

  - FIXME Probably need special class to implement unix socket
    or named pipe, this will be just for anonymous pipes.
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


// Construct and open pipes with access mode m for this one and the reverse for the other
FXPipe::FXPipe(FXPipe& other,FXuint m){
  open(other,m);
  }


// Construct file and attach existing handle h
FXPipe::FXPipe(FXInputHandle h){
  attach(h);
  }


#if defined(WIN32)

// Open pipes with access mode m for this one and the reverse for the other
FXbool FXPipe::open(FXPipe& other,FXuint m){
  if(device==BadHandle && other.device==BadHandle){
    HANDLE hnd[2];

    // Inheritable
    SECURITY_ATTRIBUTES security;
    security.nLength=sizeof(SECURITY_ATTRIBUTES);
    security.bInheritHandle=((m&FXIO::Inheritable)==0);
    security.lpSecurityDescriptor=nullptr;

    // Create connected pipe
    if(CreatePipe(&hnd[0],&hnd[1],&security,0)!=0){

      // Who's is the read-side?
      if(m&FXIO::ReadOnly){
        device=hnd[0];
        other.device=hnd[1];
        }
      else{
        device=hnd[1];
        other.device=hnd[0];
        }
      return true;
      }
    }
  return false;
  }


#else


// Open pipes with access mode m for this one and the reverse for the other
FXbool FXPipe::open(FXPipe& other,FXuint m){
  if(device==BadHandle && other.device==BadHandle){
    FXint hnd[2];
#if defined(HAVE_PIPE2)
    FXint flags=0;

    // Non-blocking
    if(m&FXIO::NonBlocking){ flags|=O_NONBLOCK; }

    // Inheritable
#if defined(O_CLOEXEC)
    if(!(m&FXIO::Inheritable)){ flags|=O_CLOEXEC; }
#endif

    // Create pipe
    if(pipe2(hnd,flags)==0){

      // Who's is the read-side?
      if(m&FXIO::ReadOnly){
        device=hnd[0];
        other.device=hnd[1];
        }
      else{
        device=hnd[1];
        other.device=hnd[0];
        }
      return true;
      }

#else

    // Create pipe
    if(pipe(hnd)==0){

      // Who's is the read-side?
      if(m&FXIO::ReadOnly){
        device=hnd[0];
        other.device=hnd[1];
        }
      else{
        device=hnd[1];
        other.device=hnd[0];
        }

      // Non-blocking
      if(m&FXIO::NonBlocking){
        fcntl(device,F_SETFL,O_NONBLOCK);
        }

      // Inheritable
#if defined(O_CLOEXEC)
      if(!(m&FXIO::Inheritable)){
        fcntl(device,F_SETFD,FD_CLOEXEC);
        }
#endif
      return true;
      }
#endif
    }
  return false;
  }

#endif

/*******************************************************************************/

// Create a named pipe (not available on Windows)
FXbool FXPipe::create(const FXString& file,FXuint perm){
#if !defined(WIN32)
  if(!file.empty()){
    return ::mkfifo(file.text(),perm&0777)==0;
    }
#endif
  return false;
  }


// Remove a named pipe (not available on Windows)
FXbool FXPipe::remove(const FXString& file){
#if !defined(WIN32)
  if(!file.empty()){
    return ::unlink(file.text())==0;
    }
#endif
  return false;
  }

/*

// FIXME move elsewhere
HANDLE WINAPI CreateNamedPipe(
  __in      LPCTSTR lpName,
  __in      DWORD dwOpenMode,
  __in      DWORD dwPipeMode,
  __in      DWORD nMaxInstances,
  __in      DWORD nOutBufferSize,
  __in      DWORD nInBufferSize,
  __in      DWORD nDefaultTimeOut,
  __in_opt  LPSECURITY_ATTRIBUTES lpSecurityAttributes
);
*/

}

