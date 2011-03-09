/********************************************************************************
*                                                                               *
*         I n t e r - T h r e a d    M e s s a g i n g    C h a n n e l         *
*                                                                               *
*********************************************************************************
* Copyright (C) 2006-2010 by Jeroen van der Zijp.   All Rights Reserved.        *
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
*********************************************************************************
* $Id: FXMessageChannel.cpp,v 1.11 2007/07/09 16:31:34 fox Exp $                *
********************************************************************************/
#include <unistd.h>
#include <fcntl.h>
#include "gmdefs.h"
#include "GMMessageChannel.h"


/*
  Notes:
  - Inter-thread messaging is handy to have.
  - Redo this in terms of FXPipe when that becomes possible.
  - Because of unbelievably retarded design of Windows, we need to
    use an Event-object to actually signal the GUI thread when we've
    written something to the pipe.
  - Possible problem: should probably NOT reset Event unless pipe is
    empty.  But are we actually falling out of MsgWaitForMultipleObject if
    Event is already signalled when we enter MsgWaitForMultipleObject?
*/


// Maximum message size
#define MAXMESSAGE 8192

// Bad handle value
#ifdef WIN32
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
  if(::pipe(h)!=0){ throw FXResourceException("unable to create pipe."); }
  ::fcntl(h[0],F_SETFD,FD_CLOEXEC);
  ::fcntl(h[1],F_SETFD,FD_CLOEXEC);
#if FOXVERSION < FXVERSION(1,7,0)
  app->addInput(h[0],INPUT_READ,this,ID_IO_READ);
#else
  app->addInput(this,ID_IO_READ,h[0],INPUT_READ,NULL);
#endif
  }


// Fire signal message to target
long FXMessageChannel::onMessage(FXObject*,FXSelector,void*){
  FXDataMessage pkg;
  if(::read(h[0],&pkg,sizeof(FXMessage))==sizeof(FXMessage)){
    if(0<pkg.size && (::read(h[0],pkg.data,pkg.size)==pkg.size)){
      return pkg.target && pkg.target->tryHandle(this,pkg.message,pkg.data);
      }
    return pkg.target && pkg.target->tryHandle(this,pkg.message,NULL);
    }
  return 0;
  }


// Send a message to a target
FXbool FXMessageChannel::message(FXObject* tgt,FXSelector msg,const void* data,FXint size){
  FXMutexLock locker(m);
  FXMessage pkg;
  pkg.target=tgt;
  pkg.message=msg;
#if !(defined(__LP64__) || defined(_LP64) || (_MIPS_SZLONG == 64) || (__WORDSIZE == 64) || defined(_WIN64))
  pkg.pad=0;
#endif
  pkg.size=size;
  if(::write(h[1],&pkg,sizeof(FXMessage))==sizeof(FXMessage)){
    if(pkg.size<=0 || (::write(h[1],data,pkg.size)==pkg.size)){
      return true;
      }
    }
  return false;
  }


// Remove handler from application
FXMessageChannel::~FXMessageChannel(){
  app->removeInput(h[0],INPUT_READ);
  ::close(h[0]);
  ::close(h[1]);
  app=(FXApp*)-1L;
  }

}
