/********************************************************************************
*                                                                               *
*                            S p l a s h    W i n d o w                         *
*                                                                               *
*********************************************************************************
* Copyright (C) 2004,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "fxkeys.h"
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
#include "FXAccelTable.h"
#include "FXEvent.h"
#include "FXWindow.h"
#include "FXDCWindow.h"
#include "FXApp.h"
#include "FXShell.h"
#include "FXIcon.h"
#include "FXTopWindow.h"
#include "FXSplashWindow.h"

/*
  Notes:

  - We need to allow some additional widgets inside it (and layout options).
  - We need to place it anywere, not just center of screen.
  - Perhaps signal user has clicked or timer expired.
*/

using namespace FX;


/*******************************************************************************/

namespace FX {


// Map
FXDEFMAP(FXSplashWindow) FXSplashWindowMap[]={
  FXMAPFUNC(SEL_PAINT,0,FXSplashWindow::onPaint),
  };


// Implementation
FXIMPLEMENT(FXSplashWindow,FXTopWindow,FXSplashWindowMap,ARRAYNUMBER(FXSplashWindowMap))


// For deserialization
FXSplashWindow::FXSplashWindow(){
  flags|=FLAG_ENABLED;
  icon=nullptr;
  delay=0;
  }


// Splash window
FXSplashWindow::FXSplashWindow(FXApp* ap,FXIcon* ic,FXuint opts,FXTime ns):FXTopWindow(ap,FXString::null,nullptr,nullptr,opts&~DECOR_ALL,0,0,ic->getWidth(),ic->getHeight(),0,0,0,0,0,0){
  flags|=FLAG_ENABLED;
  delay=ns;
  icon=ic;
  }


// Splash window
FXSplashWindow::FXSplashWindow(FXWindow* ow,FXIcon* ic,FXuint opts,FXTime ns):FXTopWindow(ow,FXString::null,nullptr,nullptr,opts&~DECOR_ALL,0,0,ic->getWidth(),ic->getHeight(),0,0,0,0,0,0){
  flags|=FLAG_ENABLED;
  delay=ns;
  icon=ic;
  }


// Create and show window
void FXSplashWindow::create(){
  FXTopWindow::create();
  icon->create();
  if(options&SPLASH_SHAPED) setShape(icon);
  }

//#ifndef WIN32
//  Atom list[2]={getApp()->wmWindowTypes[5],getApp()->wmWindowTypes[13]};
//  XChangeProperty((Display*)getApp()->getDisplay(),xid,getApp()->wmNetWindowType,XA_ATOM,32,PropModeReplace,(unsigned char*)list,2);
//#endif

// Detach window
void FXSplashWindow::detach(){
  FXTopWindow::detach();
  icon->detach();
  }


// Show splash window
void FXSplashWindow::show(){
  if(!shown()){
    FXTopWindow::show();
    if(options&SPLASH_DESTROY){
      getApp()->addTimeout(this,ID_DELETE,delay);
      }
    else{
      getApp()->addTimeout(this,ID_HIDE,delay);
      }
    }
  }


// Show splash window with a given placement
void FXSplashWindow::show(FXuint placement){
  if(!shown()){
    FXTopWindow::show(placement);
    if(options&SPLASH_DESTROY){
      getApp()->addTimeout(this,ID_DELETE,delay);
      }
    else{
      getApp()->addTimeout(this,ID_HIDE,delay);
      }
    }
  }


// Hide splash window
void FXSplashWindow::hide(){
  if(shown()){
    FXTopWindow::hide();
    if(options&SPLASH_DESTROY){
      getApp()->removeTimeout(this,ID_DELETE);
      }
    else{
      getApp()->removeTimeout(this,ID_HIDE);
      }
    }
  }


// Get default width
FXint FXSplashWindow::getDefaultWidth(){
  return icon->getWidth();
  }


// Get default height
FXint FXSplashWindow::getDefaultHeight(){
  return icon->getHeight();
  }


// Handle repaint
long FXSplashWindow::onPaint(FXObject*,FXSelector,void* ptr){
  FXDCWindow dc(this,(FXEvent*)ptr);
  dc.setForeground(backColor);
  dc.fillRectangle(0,0,width,height);
  dc.drawIcon(icon,0,0);
  return 1;
  }


// Change icon
void FXSplashWindow::setIcon(FXIcon* ic){
  if(icon!=ic){
    icon=ic;
    if(options&SPLASH_SHAPED) setShape(icon);
    resize(icon->getWidth(),icon->getHeight());
    update();
    }
  }


// Set or change delay
void FXSplashWindow::setDelay(FXTime ns){
  delay=ns;
  if(shown()){
    if(options&SPLASH_DESTROY){
      getApp()->addTimeout(this,ID_DELETE,delay);
      }
    else{
      getApp()->addTimeout(this,ID_HIDE,delay);
      }
    }
  }


// Save object to stream
void FXSplashWindow::save(FXStream& store) const {
  FXTopWindow::save(store);
  store << icon;
  store << delay;
  }


// Load object from stream
void FXSplashWindow::load(FXStream& store){
  FXTopWindow::load(store);
  store >> icon;
  store >> delay;
  }


// Destroy main window
FXSplashWindow::~FXSplashWindow(){
  getApp()->removeTimeout(this,ID_DELETE);
  getApp()->removeTimeout(this,ID_HIDE);
  if(options&SPLASH_OWNS_ICON) delete icon;
  icon=(FXIcon*)-1L;
  }

}
