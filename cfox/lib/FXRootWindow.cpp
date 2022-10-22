/********************************************************************************
*                                                                               *
*                       R o o t   W i n d o w   O b j e c t                     *
*                                                                               *
*********************************************************************************
* Copyright (C) 1997,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "FXMutex.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXSize.h"
#include "FXPoint.h"
#include "FXRectangle.h"
#include "FXStringDictionary.h"
#include "FXSettings.h"
#include "FXRegistry.h"
#include "FXVisual.h"
#include "FXEvent.h"
#include "FXWindow.h"
#include "FXApp.h"
#include "FXRootWindow.h"

/*
  Notes:

  - Size of FXRootWindow is now size of the entire virtual display, which
    is a tiled virtual area of primary and secondary display adapters.
  - Maybe getDefaultWidth() and getDefaultHeight() now returns virtual
    screen size.
*/


#define DISPLAY(app) ((Display*)((app)->display))

using namespace FX;

/*******************************************************************************/

namespace FX {

// Object implementation
FXIMPLEMENT(FXRootWindow,FXComposite,nullptr,0)


// Construct root window
FXRootWindow::FXRootWindow(FXApp* a,FXVisual *vis):FXComposite(a,vis){
  }


#ifdef WIN32

// Returns device context
FXID FXRootWindow::GetDC() const {
  LockWindowUpdate(GetDesktopWindow());
  return GetDCEx(GetDesktopWindow(),nullptr,DCX_CACHE|DCX_LOCKWINDOWUPDATE);
  }


// Release DC
int FXRootWindow::ReleaseDC(FXID hdc) const {
  int status=::ReleaseDC(GetDesktopWindow(),(HDC)hdc);
  LockWindowUpdate(nullptr);
  return status;
  }


/*

// EnumDisplayMonitors callback routine to determine biggest screen size
static BOOL maxScreenSizeCallback(HMONITOR hmon,HDC hdc,LPRECT rect,LPARAM args){
  MONITORINFO moninfo; FXint w,h;
  moninfo.cbSize=sizeof(moninfo);
  GetMonitorInfo(hmon,&moninfo);
  w=moninfo.rcWork.right-moninfo.rcWork.left;
  h=moninfo.rcWork.bottom-moninfo.rcWork.top;
  if(((FXint*)args)[0]<w) ((FXint*)args)[0]=w;
  if(((FXint*)args)[1]<h) ((FXint*)args)[1]=h;
  return true;
  }


// Obtain largest dimensions of all screens (bounding box)
static FXbool getMaxScreenSize(FXint& width,FXint& height){
  FXint args[2]={0,0};
  if(EnumDisplayMonitors(nullptr,nullptr,(MONITORENUMPROC)maxScreenSizeCallback,(LPARAM)args)){
    width=args[0];
    height=args[1];
    return true;
    }
  return false;
  }

*/


#endif


// When created, create subwindows ONLY
void FXRootWindow::create(){
  if(!xid){
    if(getApp()->isInitialized()){
      FXTRACE((100,"%s::create %p\n",getClassName(),this));

      // Got to have a visual
      if(!visual){ fxerror("%s::create: trying to create window without a visual.\n",getClassName()); }

      // Initialize visual
      visual->create();

#ifdef WIN32

      // Get handle of desktop window
      xid=GetDesktopWindow();

      // Obtain desktop window size
      width=GetSystemMetrics(SM_CXVIRTUALSCREEN);
      height=GetSystemMetrics(SM_CYVIRTUALSCREEN);

      //HDC hdc=::GetDC((HWND)xid);
      //width=GetDeviceCaps(hdc,HORZRES);
      //height=GetDeviceCaps(hdc,VERTRES);
      //::ReleaseDC((HWND)xid,hdc);

      // Size of primary display screen
      //width=GetSystemMetrics(SM_CXSCREEN);
      //height=GetSystemMetrics(SM_CYSCREEN);

      // Size bigger than all display screens
      //getMaxScreenSize(width,height);

      // Store for xid to C++ object mapping
      getApp()->hash.insert((void*)xid,this);

#else

      // Get handle of desktop window
      xid=RootWindow(DISPLAY(getApp()),DefaultScreen(DISPLAY(getApp())));

      // Obtain desktop window size
      width=DisplayWidth(DISPLAY(getApp()),DefaultScreen(DISPLAY(getApp())));
      height=DisplayHeight(DISPLAY(getApp()),DefaultScreen(DISPLAY(getApp())));

      // Store for xid to C++ object mapping
      getApp()->hash.insert((void*)xid,this);

#endif

      // Normally create children
      for(FXWindow *c=getFirst(); c; c=c->getNext()) c->create();
      }
    }
  }


// Can not attach the root window
void FXRootWindow::attach(FXID){
  }


// Can not detach the root window
void FXRootWindow::detach(){
  }


// When destroyed, destroy subwindows ONLY
void FXRootWindow::destroy(){
  if(xid){
    if(getApp()->isInitialized()){
      FXTRACE((100,"%s::destroy %p\n",getClassName(),this));

      // Normally destroy children
      for(FXWindow *c=getFirst(); c; c=c->getNext()) c->destroy();

      // Remove from xid to C++ object mapping
      getApp()->hash.remove((void*)xid);
      }
    xid=0;
    }
  }


// Get default width
FXint FXRootWindow::getDefaultWidth(){
#ifdef WIN32
//  HDC hdc=::GetDC(GetDesktopWindow());
//  FXint w=GetDeviceCaps(hdc,HORZRES);
//  ::ReleaseDC(GetDesktopWindow(),hdc);
//  return w;
  return GetSystemMetrics(SM_CXVIRTUALSCREEN);
#else
  return DisplayWidth(DISPLAY(getApp()),DefaultScreen(DISPLAY(getApp())));
#endif
  }

// Get default height
FXint FXRootWindow::getDefaultHeight(){
#ifdef WIN32
//  HDC hdc=::GetDC(GetDesktopWindow());
//  FXint h=GetDeviceCaps(hdc,VERTRES);
//  ::ReleaseDC(GetDesktopWindow(),hdc);
//  return h;
  return GetSystemMetrics(SM_CYVIRTUALSCREEN);
#else
  return DisplayHeight(DISPLAY(getApp()),DefaultScreen(DISPLAY(getApp())));
#endif
  }


// Moving root has no effect
void FXRootWindow::move(FXint,FXint){ }


// Move and resize root has no effect
void FXRootWindow::position(FXint,FXint,FXint,FXint){ }


// Resize root window has no effect
void FXRootWindow::resize(FXint,FXint){ }


// Layout of root window
void FXRootWindow::layout(){ }


// Mark as dirty
void FXRootWindow::recalc(){ }


// Root can not be focused on
void FXRootWindow::setFocus(){ }


// Root can not be unfocused
void FXRootWindow::killFocus(){ }


// Does not destroy root window
FXRootWindow::~FXRootWindow(){
  xid=0;
  }

}
