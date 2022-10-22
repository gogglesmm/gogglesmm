/********************************************************************************
*                                                                               *
*                     M a i n   W i n d o w   O b j e c t                       *
*                                                                               *
*********************************************************************************
* Copyright (C) 1998,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "FXMutex.h"
#include "FXArray.h"
#include "FXHash.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXSize.h"
#include "FXPoint.h"
#include "FXRectangle.h"
#include "FXStringDictionary.h"
#include "FXSettings.h"
#include "FXRegistry.h"
#include "FXAccelTable.h"
#include "FXCursor.h"
#include "FXEvent.h"
#include "FXWindow.h"
#include "FXApp.h"
#include "FXRootWindow.h"
#include "FXMainWindow.h"

/*
  Notes:
  - allow resize option..
  - Iconified/normal.
  - Want unlimited number of main windows.
  - Don't call X11/WIN32 unless xid and application is initialized.
*/


#define DISPLAY(app) ((Display*)((app)->display))


using namespace FX;

/*******************************************************************************/

namespace FX {


// Object implementation
FXIMPLEMENT(FXMainWindow,FXTopWindow,nullptr,0)


// Make main window
FXMainWindow::FXMainWindow(FXApp* a,const FXString& name,FXIcon *ic,FXIcon *mi,FXuint opts,FXint x,FXint y,FXint w,FXint h,FXint pl,FXint pr,FXint pt,FXint pb,FXint hs,FXint vs):FXTopWindow(a,name,ic,mi,opts,x,y,w,h,pl,pr,pt,pb,hs,vs){
  }


// Create server-side resources
void FXMainWindow::create(){
  FXTopWindow::create();
  if(xid){
    if(getApp()->isInitialized()){
#ifndef WIN32
      // Set the WM_COMMAND hint on non-owned toplevel windows
      XSetCommand(DISPLAY(getApp()),xid,const_cast<char**>((const char *const *)getApp()->getArgv()),getApp()->getArgc());
#endif
      }
    }
  }


// Destroy
FXMainWindow::~FXMainWindow(){
  }

}
