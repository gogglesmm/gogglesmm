/********************************************************************************
*                                                                               *
*                         F O X   E v e n t   L o o p                           *
*                                                                               *
*********************************************************************************
* Copyright (C) 2019,2020 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "FXAtomic.h"
#include "FXElement.h"
#include "FXHash.h"
#include "FXCallback.h"
#include "FXReactor.h"
#include "FXReactorCore.h"
#include "FXDispatcher.h"
#include "FXEventDispatcher.h"
#include "FXEventLoop.h"

/*
  Notes:

  - Manage nested event loops.

  - Exceptions will unroll through event loops.

*/

using namespace FX;

/*******************************************************************************/

namespace FX {



// Enter modal loop
FXEventLoop::FXEventLoop(FXEventLoop** inv,FXWindow* win,FXuint mode):dispatcher(NULL),invocation(inv),upper(*inv),window(win),modality(mode),code(0),done(false){
  *invocation=this;
  }


// Test if the window is involved in a modal invocation
FXbool FXEventLoop::isModal(FXWindow *win) const {
  for(const FXEventLoop* eventloop=this; eventloop; eventloop=eventloop->upper){
    if(eventloop->window==win && eventloop->modality) return true;
    }
  return false;
  }


// Break out of topmost event loop, closing all nested loops also
void FXEventLoop::stop(FXint value){
  for(FXEventLoop* eventloop=this; eventloop; eventloop=eventloop->upper){
    eventloop->done=true;
    eventloop->code=0;
    if(eventloop->upper==NULL){
      eventloop->code=value;
      return;
      }
    }
  }


// Break out of modal loop matching window, and all deeper ones
void FXEventLoop::stopModal(FXWindow* win,FXint value){
  if(isModal(window)){
    for(FXEventLoop* eventloop=this; eventloop; eventloop=eventloop->upper){
      eventloop->done=true;
      eventloop->code=0;
      if(eventloop->window==win && eventloop->modality){
        eventloop->code=value;
        return;
        }
      }
    }
  }


// Break out of innermost modal loop, and all deeper non-modal ones
void FXEventLoop::stopModal(FXint value){
  for(FXEventLoop* eventloop=this; eventloop; eventloop=eventloop->upper){
    eventloop->done=true;
    eventloop->code=0;
    if(eventloop->modality){
      eventloop->code=value;
      return;
      }
    }
  }


// Exit modal loop
FXEventLoop::~FXEventLoop(){
  *invocation=upper;
  }








#if 0



// Run application
FXint FXEventLoop::run(){
  FXEventLoop inv(&invocation);
  while(!inv.done){
    runOneEvent();
    }
  return inv.code;
  }


// Run till some flag becomes non-zero
FXint FXEventLoop::runUntil(FXuint& condition){
  FXEventLoop inv(&invocation);
  while(!inv.done && condition==0){
    runOneEvent();
    }
  return condition;
  }


// Run event loop while events are available
FXint FXEventLoop::runWhileEvents(FXTime blocking){
  FXEventLoop inv(&invocation,);
  while(!inv.done && runOneEvent(blocking)) blocking=1000;
  return !inv.done;
  }


// Run event loop while events are available
FXint FXEventLoop::runModalWhileEvents(FXWindow* window,FXTime blocking){
  FXEventLoop inv(&invocation,window,MODAL_FOR_WINDOW);
  while(!inv.done && runOneEvent(blocking)) blocking=1000;
  return !inv.done;
  }


// Perform one event dispatch
FXbool FXEventLoop::runOneEvent(FXTime blocking){
  FXRawEvent ev;
  if(getNextEvent(ev,blocking)){
    dispatchEvent(ev);
    return true;
    }
  return false;
  }


// Run modal event loop, blocking events to all windows, until stopModal is called.
FXint FXEventLoop::runModal(){
  FXEventLoop inv(&invocation);
  while(!inv.done){
    runOneEvent();
    }
  return inv.code;
  }


// Run modal for window
FXint FXEventLoop::runModalFor(FXWindow* window){
  FXEventLoop inv(&invocation,window,MODAL_FOR_WINDOW);
  while(!inv.done){
    runOneEvent();
    }
  return inv.code;
  }


// Run modal while window is shown, or until stopModal is called
FXint FXEventLoop::runModalWhileShown(FXWindow* window){
  FXEventLoop inv(&invocation,window,MODAL_FOR_WINDOW);
  while(!inv.done && window->shown()){
    runOneEvent();
    }
  return inv.code;
  }


// Run popup menu
FXint FXEventLoop::runPopup(FXWindow* window){
  FXEventLoop inv(&invocation,window,MODAL_FOR_POPUP);
  while(!inv.done && window->shown()){
    runOneEvent();
    }
  return inv.code;
  }

#endif


}
