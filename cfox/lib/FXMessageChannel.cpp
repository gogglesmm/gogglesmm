/********************************************************************************
*                                                                               *
*         I n t e r - T h r e a d    M e s s a g i n g    C h a n n e l         *
*                                                                               *
*********************************************************************************
* Copyright (C) 2006,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "FXException.h"
#include "FXArray.h"
#include "FXHash.h"
#include "FXMutex.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXSize.h"
#include "FXPoint.h"
#include "FXRectangle.h"
#include "FXStringDictionary.h"
#include "FXSettings.h"
#include "FXRegistry.h"
#include "FXEvent.h"
#include "FXWindow.h"
#include "FXApp.h"
#include "FXMessageChannel.h"


/*
  Notes:
  - Inter-thread messaging is handy to have.
  - A pipe is used to pass data from worker thread to main GUI thread, so that
    small messages may be sent from worker thread to main GUI thread asynchronously.
  - On windows, an additional semaphore is needed to provide a watchable handle
    for FXApp to block on, in MsgWaitForMultipleObjects().
  - Main GUI thread blocks on a set of input sources, and wakes up and dispatches
    to a handler when any one of them is raised.  For the message channel, control
    will dispatch to onMessage(), which will pull the message from the pipe, and
    invoke the handler read from the FXMessage struct.
  - Note that the handler may get called later than message(); it depends on when
    the main GUI thread returns to the event processing loop.
  - Based on suggestion from Axel Schmidt <axel.schmidt@analytica-karlsruhe.de>,
    the Event object was replaced by counting Semaphore.
    This way, the number of calls to message() has to be equal to the number of calls
    to MsgWaitForMultipleObjects.  Thus, each call to message() results in exactly
    one callback in the GUI thread.
  - FIXME technically, FXMessageChannel should refer to an event loop instance (or
    FXDispatcher instance), not FXApp.  Not all applications are GUI applications.
*/


// Maximum message size
#define MAXMESSAGE 8192

// Bad handle value
#if defined(WIN32)
#define BadHandle  INVALID_HANDLE_VALUE
#else
#define BadHandle  -1
#endif


using namespace FX;


/*******************************************************************************/

namespace FX {


// Structure of message
struct FXMessage {
  FXObject  *target;            // Message target
  FXSelector message;           // Message type,id
#if !(defined(__LP64__) || defined(_LP64) || (_MIPS_SZLONG == 64) || (__WORDSIZE == 64) || defined(_WIN64))
  FXint      pad;               // Padding for 32-bit
#endif
  FXint      size;              // Message size
  };


// Structure of message+payload
struct FXDataMessage {
  FXObject  *target;            // Message target
  FXSelector message;           // Message type,id
#if !(defined(__LP64__) || defined(_LP64) || (_MIPS_SZLONG == 64) || (__WORDSIZE == 64) || defined(_WIN64))
  FXint      pad;               // Padding for 32-bit
#endif
  FXint      size;              // Message size
  FXlong     data[MAXMESSAGE/sizeof(FXlong)];
  };


// Map
FXDEFMAP(FXMessageChannel) FXMessageChannelMap[]={
  FXMAPFUNC(SEL_IO_READ,FXMessageChannel::ID_IO_READ,FXMessageChannel::onMessage)
  };


// Object implementation
FXIMPLEMENT(FXMessageChannel,FXObject,FXMessageChannelMap,ARRAYNUMBER(FXMessageChannelMap));


// Initialize to empty
FXMessageChannel::FXMessageChannel():app((FXApp*)-1L){
  h[0]=h[1]=h[2]=BadHandle;
  }


// Add handler to application
FXMessageChannel::FXMessageChannel(FXApp* a):app(a){
#if defined(WIN32)
  if((h[2]=::CreateSemaphore(nullptr,0,2147483647,nullptr))==nullptr){ throw FXResourceException("unable to create semaphore."); }
  if(::CreatePipe(&h[0],&h[1],nullptr,0)==0){ throw FXResourceException("unable to create pipe."); }
  app->addInput(this,ID_IO_READ,h[2],INPUT_READ,nullptr);
#else
  if(::pipe(h)!=0){ throw FXResourceException("unable to create pipe."); }
  ::fcntl(h[0],F_SETFD,FD_CLOEXEC);
  ::fcntl(h[1],F_SETFD,FD_CLOEXEC);
  app->addInput(this,ID_IO_READ,h[0],INPUT_READ,nullptr);
#endif
  }


// Fire signal message to target
long FXMessageChannel::onMessage(FXObject*,FXSelector,void*){
  FXDataMessage pkg;
#if defined(WIN32)
  DWORD nread=-1;
  if(::ReadFile(h[0],&pkg,sizeof(FXMessage),&nread,nullptr) && nread==sizeof(FXMessage)){
    if(0<pkg.size && (::ReadFile(h[0],pkg.data,pkg.size,&nread,nullptr) && nread==pkg.size)){
      return pkg.target && pkg.target->tryHandle(this,pkg.message,pkg.data);
      }
    return pkg.target && pkg.target->tryHandle(this,pkg.message,nullptr);
    }
#else
  if(::read(h[0],&pkg,sizeof(FXMessage))==sizeof(FXMessage)){
    if(0<pkg.size && (::read(h[0],pkg.data,pkg.size)==pkg.size)){
      return pkg.target && pkg.target->tryHandle(this,pkg.message,pkg.data);
      }
    return pkg.target && pkg.target->tryHandle(this,pkg.message,nullptr);
    }
#endif
  return 0;
  }


// Send a message to a target
FXbool FXMessageChannel::message(FXObject* tgt,FXSelector msg,const void* data,FXint size){
  FXScopedMutex locker(m);
  FXMessage pkg;
  pkg.target=tgt;
  pkg.message=msg;
#if !(defined(__LP64__) || defined(_LP64) || (_MIPS_SZLONG == 64) || (__WORDSIZE == 64) || defined(_WIN64))
  pkg.pad=0;
#endif
  pkg.size=FXMIN(size,MAXMESSAGE);
#if defined(WIN32)
  DWORD nwritten=-1;
  if(::WriteFile(h[1],&pkg,sizeof(FXMessage),&nwritten,nullptr) && nwritten==sizeof(FXMessage)){
    if(pkg.size<=0 || (::WriteFile(h[1],data,pkg.size,&nwritten,nullptr) && nwritten==pkg.size)){
      ::ReleaseSemaphore(h[2],1,nullptr);
      return true;
      }
    }
#else
  if(::write(h[1],&pkg,sizeof(FXMessage))==sizeof(FXMessage)){
    if(pkg.size<=0 || (::write(h[1],data,pkg.size)==pkg.size)){		// Write with one call...
      return true;
      }
    }
#endif
  return false;
  }


// Remove handler from application
FXMessageChannel::~FXMessageChannel(){
#if defined(WIN32)
  app->removeInput(h[2],INPUT_READ);
  ::CloseHandle(h[0]);
  ::CloseHandle(h[1]);
  ::CloseHandle(h[2]);
#else
  app->removeInput(h[0],INPUT_READ);
  ::close(h[0]);
  ::close(h[1]);
#endif
  app=(FXApp*)-1L;
  }

}
