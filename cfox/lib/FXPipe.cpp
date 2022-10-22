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

  - Obviously this will get fleshed out some more...
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
FXPipe::FXPipe(FXInputHandle h,FXuint m){
  attach(h,m);
  }


// Open pipes with access mode m for this one and the reverse for the other
FXbool FXPipe::open(FXPipe& other,FXuint m){
  if(device==BadHandle && other.device==BadHandle){
    access=NoAccess;
    other.access=NoAccess;
    pointer=0L;
    other.pointer=0L;
#if defined(WIN32)
    SECURITY_ATTRIBUTES sat;
    HANDLE hrd,hwr;
    sat.nLength=sizeof(SECURITY_ATTRIBUTES);
    sat.bInheritHandle=(m&Inheritable)?true:false;
    sat.lpSecurityDescriptor=nullptr;

    // Create connected pipe
    if(CreatePipe(&hrd,(HANDLE*)&hwr,&sat,0)!=0){
      if(m&ReadOnly){
        device=hrd;
        other.device=hwr;
        access=(m&~WriteOnly)|ReadOnly|OwnHandle;
        other.access=(m&~ReadOnly)|WriteOnly|OwnHandle;
        }
      else{
        device=hwr;
        other.device=hrd;
        access=(m&~ReadOnly)|WriteOnly|OwnHandle;
        other.access=(m&~WriteOnly)|ReadOnly|OwnHandle;
        }
      return true;
      }
#else
    FXint flags=0;
    FXint fd[2];

    // Non-blocking mode
    if(m&NonBlocking) flags|=O_NONBLOCK;

    // Inheritable only if specified
#if defined(O_CLOEXEC)
    if(!(m&Inheritable)) flags|=O_CLOEXEC;
#endif

    // Create connected pipe
#if defined(HAVE_PIPE2)
    if(pipe2(fd,flags)==0){
#else
    if(pipe(fd)==0){
#endif
      if(m&ReadOnly){
        device=fd[0];
        other.device=fd[1];
        access=(m&~WriteOnly)|ReadOnly|OwnHandle;
        other.access=(m&~ReadOnly)|WriteOnly|OwnHandle;
        }
      else{
        device=fd[1];
        other.device=fd[0];
        access=(m&~ReadOnly)|WriteOnly|OwnHandle;
        other.access=(m&~WriteOnly)|ReadOnly|OwnHandle;
        }
#if !defined(HAVE_PIPE2)
      if(m&NonBlocking){fcntl(device,F_SETFL,O_NONBLOCK);}
      if(!(m&Inheritable)){fcntl(device,F_SETFD,FD_CLOEXEC);}
#endif
      return true;
      }
#endif
    }
  return false;
  }


// Open pipe with access mode m and handle h
FXbool FXPipe::open(FXInputHandle h,FXuint m){
  return FXIODevice::open(h,m);
  }

/*******************************************************************************/


// Create a named pipe
FXbool FXPipe::create(const FXString& file,FXuint perm){
  if(!file.empty()){
#if defined(WIN32)
/*
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
#else
    return ::mkfifo(file.text(),perm)==0;
#endif
    }
  return false;
  }

}

