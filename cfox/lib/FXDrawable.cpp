/********************************************************************************
*                                                                               *
*                             D r a w a b l e   A r e a                         *
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
#include "FXDrawable.h"
#include "FXWindow.h"
#include "FXApp.h"

/*
  Notes:
  - Abstract drawable surface which may be used as drawing target in FXDCWindow;
    it may also be used as source in drawing operations.
*/

using namespace FX;

/*******************************************************************************/

namespace FX {

// Object implementation
FXIMPLEMENT_ABSTRACT(FXDrawable,FXId,nullptr,0)


// For deserialization
FXDrawable::FXDrawable():visual(nullptr),width(0),height(0){
  }


// Initialize nicely
FXDrawable::FXDrawable(FXApp* a,FXint w,FXint h):FXId(a),visual(nullptr),width(FXMAX(w,0)),height(FXMAX(h,0)){
  }


// Change visual
void FXDrawable::setVisual(FXVisual* vis){
  if(!vis){ fxerror("%s::setVisual: NULL visual\n",getClassName()); }
  if(xid){ fxerror("%s::setVisual: visual should be set before calling create()\n",getClassName()); }
  visual=vis;
  }


// Resize drawable to the specified width and height
void FXDrawable::resize(FXint w,FXint h){
  width=FXMAX(w,0);
  height=FXMAX(h,0);
  }


// Save data
void FXDrawable::save(FXStream& store) const {
  FXId::save(store);
  store << visual;
  store << width;
  store << height;
  }


// Load data
void FXDrawable::load(FXStream& store){
  FXId::load(store);
  store >> visual;
  store >> width;
  store >> height;
  }


// Clean up
FXDrawable::~FXDrawable(){
  visual=(FXVisual*)-1L;
  }

}


