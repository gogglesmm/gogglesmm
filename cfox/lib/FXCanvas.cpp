/********************************************************************************
*                                                                               *
*                   C a n v a s   W i n d o w   O b j e c t                     *
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
#include "FXEvent.h"
#include "FXWindow.h"
#include "FXApp.h"
#include "FXCanvas.h"

using namespace FX;


/*******************************************************************************/

namespace FX {

// Map
FXDEFMAP(FXCanvas) FXCanvasMap[]={
  FXMAPFUNC(SEL_PAINT,0,FXCanvas::onPaint)
  };


// Object implementation
FXIMPLEMENT(FXCanvas,FXWindow,FXCanvasMap,ARRAYNUMBER(FXCanvasMap))


// For serialization
FXCanvas::FXCanvas(){
  flags|=FLAG_ENABLED|FLAG_SHOWN;
  }


// Make a canvas
FXCanvas::FXCanvas(FXComposite* p,FXObject* tgt,FXSelector sel,FXuint opts,FXint x,FXint y,FXint w,FXint h):FXWindow(p,opts,x,y,w,h){
  flags|=FLAG_ENABLED|FLAG_SHOWN;
  backColor=getApp()->getBackColor();
  target=tgt;
  message=sel;
  }



// It can be focused on
FXbool FXCanvas::canFocus() const { return true; }


// Canvas is an object drawn by another
long FXCanvas::onPaint(FXObject*,FXSelector,void* ptr){
  return target && target->handle(this,FXSEL(SEL_PAINT,message),ptr);
  }


}
