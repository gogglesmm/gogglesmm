/********************************************************************************
*                                                                               *
*              P r i v a t e   I n t e r n a l   F u n c t i o n s              *
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
#include "fxmath.h"
#include "FXArray.h"
#include "FXHash.h"
#include "FXElement.h"
#include "FXMutex.h"
#include "FXAutoThreadStorageKey.h"
#include "FXRunnable.h"
#include "FXThread.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXSize.h"
#include "FXPoint.h"
#include "FXRectangle.h"
#include "FXObject.h"
#include "FXStringDictionary.h"
#include "FXSettings.h"
#include "FXRegistry.h"
#include "FXEvent.h"
#include "FXWindow.h"
#include "FXApp.h"
#include "fxpriv.h"


/*
  Notes:
  - This file does actual data transfer for clipboard, selection, and drag and drop.
  - Perhaps we should also implement INCR for sending; however, we don't know for
    sure if the other side supports this.
*/

using namespace FX;


/*******************************************************************************/

namespace FX {


#ifdef WIN32            // WIN32


// Send data via shared memory
HANDLE fxsenddata(HWND window,FXuchar* data,FXuint size){
  HANDLE hMap=0,hMapCopy=0;
  FXuchar *ptr;
  DWORD processid;
  HANDLE process;

  if(data && size){
    hMap=CreateFileMappingA(INVALID_HANDLE_VALUE,nullptr,PAGE_READWRITE,0,size+sizeof(FXuint),"_FOX_DDE");
    if(hMap){
      ptr=(FXuchar*)MapViewOfFile((HANDLE)hMap,FILE_MAP_WRITE,0,0,size+sizeof(FXuint));
      if(ptr){
        *((FXuint*)ptr)=size;
        memcpy(ptr+sizeof(FXuint),data,size);
        UnmapViewOfFile(ptr);
        }
      GetWindowThreadProcessId((HWND)window,&processid);
      process=OpenProcess(PROCESS_DUP_HANDLE,true,processid);
      DuplicateHandle(GetCurrentProcess(),hMap,process,&hMapCopy,FILE_MAP_ALL_ACCESS,true,DUPLICATE_CLOSE_SOURCE|DUPLICATE_SAME_ACCESS);
      CloseHandle(process);
      }
    return hMapCopy;
    }
  return 0;
  }


// Receive data via shared memory
HANDLE fxrecvdata(HANDLE hMap,FXuchar*& data,FXuint& size){
  FXuchar *ptr;
  data=nullptr;
  size=0;
  if(hMap){
    ptr=(FXuchar*)MapViewOfFile(hMap,FILE_MAP_READ,0,0,0);
    if(ptr){
      size=*((FXuint*)ptr);
      if(allocElms(data,size)){
        memcpy(data,ptr+sizeof(FXuint),size);
        }
      UnmapViewOfFile(ptr);
      }
    CloseHandle(hMap);
    return hMap;
    }
  return 0;
  }


// Send request for data
HANDLE fxsendrequest(HWND window,HWND requestor,WPARAM type){
  FXuint loops=100;
  MSG msg;
  PostMessage((HWND)window,WM_DND_REQUEST,type,(LPARAM)requestor);
  while(!PeekMessage(&msg,nullptr,WM_DND_REPLY,WM_DND_REPLY,PM_REMOVE)){
    if(loops==0){ fxwarning("timed out\n"); return 0; }
    FXThread::sleep(10000000);  // Don't burn too much CPU here:- the other guy needs it more....
    loops--;
    }
  return (HANDLE)msg.wParam;
  }


/*******************************************************************************/


// Change PRIMARY selection data
void FXApp::selectionSetData(const FXWindow*,FXDragType,FXuchar* data,FXuint size){
  freeElms(ddeData);
  ddeData=data;
  ddeSize=size;
  }


// Retrieve PRIMARY selection data
void FXApp::selectionGetData(const FXWindow*,FXDragType type,FXuchar*& data,FXuint& size){
  data=nullptr;
  size=0;
  if(selectionWindow){
    event.type=SEL_SELECTION_REQUEST;
    event.target=type;
    ddeData=nullptr;
    ddeSize=0;
    selectionWindow->handle(this,FXSEL(SEL_SELECTION_REQUEST,0),&event);
    data=ddeData;
    size=ddeSize;
    ddeData=nullptr;
    ddeSize=0;
    }
  }



// Retrieve PRIMARY selection types
void FXApp::selectionGetTypes(const FXWindow*,FXDragType*& types,FXuint& numtypes){
  types=nullptr;
  numtypes=0;
  if(selectionWindow){
    dupElms(types,xselTypeList,xselNumTypes);
    numtypes=xselNumTypes;
    }
  }

/*******************************************************************************/


// Change CLIPBOARD selection data
void FXApp::clipboardSetData(const FXWindow*,FXDragType type,FXuchar* data,FXuint size){
  HGLOBAL hGlobalMemory=GlobalAlloc(GMEM_MOVEABLE,size);
  if(hGlobalMemory){
    void *pGlobalMemory=GlobalLock(hGlobalMemory);
    FXASSERT(pGlobalMemory);
    memcpy((FXchar*)pGlobalMemory,data,size);
    GlobalUnlock(hGlobalMemory);
    SetClipboardData(type,hGlobalMemory);
    freeElms(data);
    }
  }


// Retrieve CLIPBOARD selection data
void FXApp::clipboardGetData(const FXWindow* window,FXDragType type,FXuchar*& data,FXuint& size){
  data=nullptr;
  size=0;
  if(IsClipboardFormatAvailable(type)){
    if(OpenClipboard((HWND)window->id())){
      HANDLE hClipMemory=GetClipboardData(type);
      if(hClipMemory){
        size=(FXuint)GlobalSize(hClipMemory);
        if(allocElms(data,size)){
          void *pClipMemory=GlobalLock(hClipMemory);
          FXASSERT(pClipMemory);
          memcpy((void*)data,pClipMemory,size);
          GlobalUnlock(hClipMemory);
          CloseClipboard();
          }
        }
      }
    }
  }


// Retrieve CLIPBOARD selection types
void FXApp::clipboardGetTypes(const FXWindow* window,FXDragType*& types,FXuint& numtypes){
  FXuint count;
  types=nullptr;
  numtypes=0;
  if(OpenClipboard((HWND)window->id())){
    count=CountClipboardFormats();
    if(count){
      allocElms(types,count);
      UINT format=0;
      while(numtypes<count && (format=EnumClipboardFormats(format))!=0){
        types[numtypes++]=format;
        }
      }
    CloseClipboard();
    }
  }

/*******************************************************************************/


// Change DND selection data
void FXApp::dragdropSetData(const FXWindow*,FXDragType,FXuchar* data,FXuint size){
  freeElms(ddeData);
  ddeData=data;
  ddeSize=size;
  }


// Retrieve DND selection data
void FXApp::dragdropGetData(const FXWindow* window,FXDragType type,FXuchar*& data,FXuint& size){
  HANDLE answer;
  data=nullptr;
  size=0;
  if(dragWindow){
    event.type=SEL_DND_REQUEST;
    event.target=type;
    ddeData=nullptr;
    ddeSize=0;
    dragWindow->handle(this,FXSEL(SEL_DND_REQUEST,0),&event);
    data=ddeData;
    size=ddeSize;
    ddeData=nullptr;
    ddeSize=0;
    }
  else{
    answer=fxsendrequest((HWND)xdndSource,(HWND)window->id(),(WPARAM)type);
    fxrecvdata(answer,data,size);
    }
  }


// Retrieve DND selection types
void FXApp::dragdropGetTypes(const FXWindow*,FXDragType*& types,FXuint& numtypes){
  dupElms(types,ddeTypeList,ddeNumTypes);
  numtypes=ddeNumTypes;
  }


/*******************************************************************************/

#else                   // X11


// Wait for event of certain type
static FXbool fxwaitforevent(Display *display,Window window,int type,XEvent& event){
  FXuint loops=100;
  while(!XCheckTypedWindowEvent(display,window,type,&event)){
    if(loops==0){ fxwarning("timed out\n"); return false; }
    FXThread::sleep(10000000);  // Don't burn too much CPU here:- the other guy needs it more....
    loops--;
    }
  return true;
  }


// Send request for selection info
Atom fxsendrequest(Display *display,Window window,Atom selection,Atom prop,Atom type,FXuint time){
  FXuint loops=100;
  XEvent ev;
  XConvertSelection(display,selection,type,prop,window,time);
  while(!XCheckTypedWindowEvent(display,window,SelectionNotify,&ev)){
    if(loops==0){ fxwarning("timed out\n"); return None; }
    FXThread::sleep(10000000);  // Don't burn too much CPU here:- the other guy needs it more....
    loops--;
    }
  return ev.xselection.property;
  }


// Reply to request for selection info
Atom fxsendreply(Display *display,Window window,Atom selection,Atom prop,Atom target,FXuint time){
  XEvent se;
  se.xselection.type=SelectionNotify;
  se.xselection.send_event=true;
  se.xselection.display=display;
  se.xselection.requestor=window;
  se.xselection.selection=selection;
  se.xselection.target=target;
  se.xselection.property=prop;
  se.xselection.time=time;
  XSendEvent(display,window,True,NoEventMask,&se);
  XFlush(display);
  return prop;
  }


// Send types via property
Atom fxsendtypes(Display *display,Window window,Atom prop,FXDragType* types,FXuint numtypes){
  if(types && numtypes){
    XChangeProperty(display,window,prop,XA_ATOM,32,PropModeReplace,(unsigned char*)types,numtypes);
    return prop;
    }
  return None;
  }


// Send data via property
Atom fxsenddata(Display *display,Window window,Atom prop,Atom type,FXuchar* data,FXuint size){
  unsigned long maxtfrsize,tfrsize,tfroffset;
  int mode;
  if(data && size){
    maxtfrsize=4*XMaxRequestSize(display);
    mode=PropModeReplace;
    tfroffset=0;
    while(size){
      tfrsize=size;
      if(tfrsize>maxtfrsize) tfrsize=maxtfrsize;
      XChangeProperty(display,window,prop,type,8,mode,&data[tfroffset],tfrsize);
      mode=PropModeAppend;
      tfroffset+=tfrsize;
      size-=tfrsize;
      }
    return prop;
    }
  return None;
  }


// Read type list from property
Atom fxrecvtypes(Display *display,Window window,Atom prop,FXDragType*& types,FXuint& numtypes,FXbool del){
  unsigned long numitems,bytesleft;
  unsigned char *ptr;
  int actualformat;
  Atom actualtype;
  types=nullptr;
  numtypes=0;
  if(prop){
    if(XGetWindowProperty(display,window,prop,0,1024,del,XA_ATOM,&actualtype,&actualformat,&numitems,&bytesleft,&ptr)==Success){
      if(actualtype==XA_ATOM && actualformat==32 && numitems>0){
        if(allocElms(types,numitems)){
          memcpy(types,ptr,sizeof(Atom)*numitems);
          numtypes=numitems;
          }
        }
      XFree(ptr);
      }
    return prop;
    }
  return None;
  }


// Read property in chunks smaller than maximum transfer length,
// appending to data array; returns amount read from the property.
static FXuint fxrecvprop(Display *display,Window window,Atom prop,Atom& type,FXuchar*& data,FXuint& size){
  unsigned long maxtfrsize=XMaxRequestSize(display)*4;
  unsigned long tfroffset,tfrsize,tfrleft;
  unsigned char *ptr;
  int format;
  tfroffset=0;

  // Read next chunk of data from property
  while(XGetWindowProperty(display,window,prop,tfroffset>>2,maxtfrsize>>2,False,AnyPropertyType,&type,&format,&tfrsize,&tfrleft,&ptr)==Success && type!=None){
    tfrsize*=(format>>3);

    // Grow the array to accomodate new data
    if(!resizeElms(data,size+tfrsize+1)){ XFree(ptr); break; }

    // Append new data at the end, plus the extra 0.
    memcpy(&data[size],ptr,tfrsize+1);
    size+=tfrsize;
    tfroffset+=tfrsize;
    XFree(ptr);
    if(tfrleft==0) break;
    }

  // Delete property after we're done
  XDeleteProperty(display,window,prop);
  XFlush(display);
  return tfroffset;
  }


// Receive data via property
Atom fxrecvdata(Display *display,Window window,Atom prop,Atom incr,Atom& type,FXuchar*& data,FXuint& size){
  unsigned long  tfrsize,tfrleft;
  unsigned char *ptr;
  XEvent ev;
  int format;
  data=nullptr;
  size=0;
  if(prop){

    // First, see what we've got
    if(XGetWindowProperty(display,window,prop,0,0,False,AnyPropertyType,&type,&format,&tfrsize,&tfrleft,&ptr)==Success && type!=None){
      XFree(ptr);

      // Incremental transfer
      if(type==incr){

        // Delete the INCR property
        XDeleteProperty(display,window,prop);
        XFlush(display);

        // Wait for the next batch of data
        while(fxwaitforevent(display,window,PropertyNotify,ev)){

          // Wrong type of notify event; perhaps stale event
          if(ev.xproperty.atom!=prop || ev.xproperty.state!=PropertyNewValue) continue;

          // See what we've got
          if(XGetWindowProperty(display,window,prop,0,0,False,AnyPropertyType,&type,&format,&tfrsize,&tfrleft,&ptr)==Success && type!=None){
            XFree(ptr);

            // if empty property, its the last one
            if(tfrleft==0){

              // Delete property so the other side knows we've got the data
              XDeleteProperty(display,window,prop);
              XFlush(display);
              break;
              }

            // Read and delete the property
            fxrecvprop(display,window,prop,type,data,size);
            }
          }
        }

      // All data in one shot
      else{
        // Read and delete the property
        fxrecvprop(display,window,prop,type,data,size);
        }
      }
    return prop;
    }
  return None;
  }


/*******************************************************************************/


// Change PRIMARY selection data
void FXApp::selectionSetData(const FXWindow*,FXDragType,FXuchar* data,FXuint size){
  freeElms(ddeData);
  ddeData=data;
  ddeSize=size;
  }


// Retrieve PRIMARY selection data
void FXApp::selectionGetData(const FXWindow* window,FXDragType type,FXuchar*& data,FXuint& size){
  FXID answer;
  data=nullptr;
  size=0;
  if(selectionWindow){
    event.type=SEL_SELECTION_REQUEST;
    event.target=type;
    ddeData=nullptr;
    ddeSize=0;
    selectionWindow->handle(this,FXSEL(SEL_SELECTION_REQUEST,0),&event);
    data=ddeData;
    size=ddeSize;
    ddeData=nullptr;
    ddeSize=0;
    }
  else{
    answer=fxsendrequest((Display*)display,window->id(),XA_PRIMARY,ddeAtom,type,event.time);
    fxrecvdata((Display*)display,window->id(),answer,ddeIncr,type,data,size);
    }
  }


// Retrieve PRIMARY selection types
void FXApp::selectionGetTypes(const FXWindow* window,FXDragType*& types,FXuint& numtypes){
  FXID answer;
  types=nullptr;
  numtypes=0;
  if(selectionWindow){
    dupElms(types,xselTypeList,xselNumTypes);
    numtypes=xselNumTypes;
    }
  else{
    answer=fxsendrequest((Display*)display,window->id(),XA_PRIMARY,ddeAtom,ddeTargets,event.time);
    fxrecvtypes((Display*)display,window->id(),answer,types,numtypes,true);
    }
  }


/*******************************************************************************/


// Change CLIPBOARD selection data
void FXApp::clipboardSetData(const FXWindow*,FXDragType,FXuchar* data,FXuint size){
  freeElms(ddeData);
  ddeData=data;
  ddeSize=size;
  }


// Retrieve CLIPBOARD selection data
void FXApp::clipboardGetData(const FXWindow* window,FXDragType type,FXuchar*& data,FXuint& size){
  FXID answer;
  data=nullptr;
  size=0;
  if(clipboardWindow){
    event.type=SEL_CLIPBOARD_REQUEST;
    event.target=type;
    ddeData=nullptr;
    ddeSize=0;
    clipboardWindow->handle(this,FXSEL(SEL_CLIPBOARD_REQUEST,0),&event);
    data=ddeData;
    size=ddeSize;
    ddeData=nullptr;
    ddeSize=0;
    }
  else{
    answer=fxsendrequest((Display*)display,window->id(),xcbSelection,ddeAtom,type,event.time);
    fxrecvdata((Display*)display,window->id(),answer,ddeIncr,type,data,size);
    }
  }


// Retrieve CLIPBOARD selection types
void FXApp::clipboardGetTypes(const FXWindow* window,FXDragType*& types,FXuint& numtypes){
  FXID answer;
  types=nullptr;
  numtypes=0;
  if(clipboardWindow){
    dupElms(types,xcbTypeList,xcbNumTypes);
    numtypes=xcbNumTypes;
    }
  else{
    answer=fxsendrequest((Display*)display,window->id(),xcbSelection,ddeAtom,ddeTargets,event.time);
    fxrecvtypes((Display*)display,window->id(),answer,types,numtypes,true);
    }
  }


/*******************************************************************************/


// Change DND selection data
void FXApp::dragdropSetData(const FXWindow*,FXDragType,FXuchar* data,FXuint size){
  freeElms(ddeData);
  ddeData=data;
  ddeSize=size;
  }


// Retrieve DND selection data
void FXApp::dragdropGetData(const FXWindow* window,FXDragType type,FXuchar*& data,FXuint& size){
  FXID answer;
  data=nullptr;
  size=0;
  if(dragWindow){
    event.type=SEL_DND_REQUEST;
    event.target=type;
    ddeData=nullptr;
    ddeSize=0;
    dragWindow->handle(this,FXSEL(SEL_DND_REQUEST,0),&event);
    data=ddeData;
    size=ddeSize;
    ddeData=nullptr;
    ddeSize=0;
    }
  else{
    answer=fxsendrequest((Display*)display,window->id(),xdndSelection,ddeAtom,type,event.time);
    fxrecvdata((Display*)display,window->id(),answer,ddeIncr,type,data,size);
    }
  }


// Retrieve DND selection types
void FXApp::dragdropGetTypes(const FXWindow*,FXDragType*& types,FXuint& numtypes){
  dupElms(types,ddeTypeList,ddeNumTypes);
  numtypes=ddeNumTypes;
  }

#endif

}
