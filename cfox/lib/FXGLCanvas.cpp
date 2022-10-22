/********************************************************************************
*                                                                               *
*                    O p e n G L   C a n v a s   O b j e c t                    *
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
#include "FXAccelTable.h"
#include "FXCursor.h"
#include "FXEvent.h"
#include "FXWindow.h"
#include "FXApp.h"
#include "FXException.h"
#include "FXVisual.h"
#include "FXGLVisual.h"
#include "FXGLCanvas.h"
#include "FXGLContext.h"


/*
  Notes:
  - Since this only adds SetPixelFormat, perhaps not a bad idea to contemplate
    moving this call to SetPixelFormat somewhere else [candidates are FXGLVisual,
    FXWindow, or FXGLContext].
*/


using namespace FX;

/*******************************************************************************/

namespace FX {


// Object implementation
FXIMPLEMENT(FXGLCanvas,FXCanvas,nullptr,0)


// For serialization
FXGLCanvas::FXGLCanvas(){
  flags|=FLAG_ENABLED|FLAG_SHOWN;
  context=nullptr;
  }


// Construct a GL canvas with its private context and private display lists
FXGLCanvas::FXGLCanvas(FXComposite* p,FXGLVisual *vis,FXObject* tgt,FXSelector sel,FXuint opts,FXint x,FXint y,FXint w,FXint h):FXCanvas(p,tgt,sel,opts|GLCANVAS_OWN_CONTEXT,x,y,w,h){
  flags|=FLAG_ENABLED|FLAG_SHOWN;
  context=new FXGLContext(getApp(),vis);
  visual=vis;
  }


// Construct a GL canvas with its private context but shared display lists
FXGLCanvas::FXGLCanvas(FXComposite* p,FXGLVisual *vis,FXGLCanvas* share,FXObject* tgt,FXSelector sel,FXuint opts,FXint x,FXint y,FXint w,FXint h):FXCanvas(p,tgt,sel,opts|GLCANVAS_OWN_CONTEXT,x,y,w,h){
  flags|=FLAG_ENABLED|FLAG_SHOWN;
  context=new FXGLContext(getApp(),vis,share ? share->getContext() : nullptr);
  visual=vis;
  }


// Construct a GL canvas with a shared context
FXGLCanvas::FXGLCanvas(FXComposite* p,FXGLContext* ctx,FXObject* tgt,FXSelector sel,FXuint opts,FXint x,FXint y,FXint w,FXint h):FXCanvas(p,tgt,sel,opts,x,y,w,h){
  flags|=FLAG_ENABLED|FLAG_SHOWN;
  context=ctx;
  visual=ctx->getVisual();
  }


#ifdef WIN32
const void* FXGLCanvas::GetClass() const { return TEXT("FXGLCanvas"); }
#endif


// Change context
void FXGLCanvas::setContext(FXGLContext *ctx,FXbool owned){
  if(!ctx){ fxerror("%s::setContext: NULL context\n",getClassName()); }
  if(xid){ fxerror("%s::setContext: context should be set before calling create()\n",getClassName()); }
  if(context!=ctx){
    if(options&GLCANVAS_OWN_CONTEXT) delete context;
    context=ctx;
    visual=ctx->getVisual();
    }
  options^=((0-owned)^options)&GLCANVAS_OWN_CONTEXT;
  }


// Return true if it is sharing display lists
FXbool FXGLCanvas::isShared() const {
  return (context->getShared()!=nullptr);
  }


// Create X window (GL CANVAS)
void FXGLCanvas::create(){
  FXCanvas::create();
  if(xid){
    if(getApp()->isInitialized()){

      // Create context
      context->create();

      // Change frame buffer according to visual
#ifdef HAVE_GL_H
#if defined(WIN32)
      PIXELFORMATDESCRIPTOR pfd={sizeof(PIXELFORMATDESCRIPTOR),1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
      HDC hdc=::GetDC((HWND)xid);       // FIXME should this be this->GetDC()
      SetPixelFormat(hdc,(FXint)(FXival)visual->id(),&pfd);
      ::ReleaseDC((HWND)xid,hdc);       // FIXME should this be this->ReleaseDC()
//#elif defined(GLX_VERSION_1_3)
//      xid=glXCreateWindow((Display*)getApp()->getDisplay(),(GLXFBConfig)visual->id(),xid,nullptr);
#endif
#endif
      }
    }
  }


// Detach the GL Canvas
void FXGLCanvas::detach(){
  context->detach();
  FXCanvas::detach();
  }


// Destroy the GL Canvas
void FXGLCanvas::destroy(){
  if(xid){
    if(getApp()->isInitialized()){
      if(options&GLCANVAS_OWN_CONTEXT) context->destroy();
#ifdef HAVE_GL_H
//#if defined(GLX_VERSION_1_3)
//      glXDestroyWindow((Display*)getApp()->getDisplay(),xid);
//      xid=0;
//#endif
#endif
      }
    }
  FXCanvas::destroy();
  }


//  Make the rendering context of GL Canvas current
FXbool FXGLCanvas::makeCurrent(){
  return context->begin(this);
  }


//  Make the rendering context of GL Canvas current
FXbool FXGLCanvas::makeNonCurrent(){
  return context->end();
  }


//  Return true if this window's context is current
FXbool FXGLCanvas::isCurrent() const {
  return context->isCurrent();
  }


// Used by GL to swap the buffers in double buffer mode, or flush a single buffer
void FXGLCanvas::swapBuffers(){
  context->swapBuffers();
  }


// Save object to stream
void FXGLCanvas::save(FXStream& store) const {
  FXCanvas::save(store);
  store << context;
  }


// Load object from stream
void FXGLCanvas::load(FXStream& store){
  FXCanvas::load(store);
  store >> context;
  }


// Close and release any resources
FXGLCanvas::~FXGLCanvas(){
  destroy();
  if(options&GLCANVAS_OWN_CONTEXT) delete context;
  context=(FXGLContext*)-1L;
  }

}
