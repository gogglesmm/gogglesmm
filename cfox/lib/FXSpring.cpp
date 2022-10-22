/********************************************************************************
*                                                                               *
*                S p r i n g   C o n t a i n e r   W i d g e t                  *
*                                                                               *
*********************************************************************************
* Copyright (C) 2003,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "FXDCWindow.h"
#include "FXApp.h"
#include "FXPacker.h"
#include "FXSpring.h"


/*
  Notes:
  - Based upon an idea from Amanda Ross.
*/

using namespace FX;

/*******************************************************************************/

namespace FX {


// Object implementation
FXIMPLEMENT(FXSpring,FXPacker,nullptr,0)


// Create child frame window
FXSpring::FXSpring(FXComposite* p,FXuint opts,FXint relw,FXint relh,FXint x,FXint y,FXint w,FXint h,FXint pl,FXint pr,FXint pt,FXint pb,FXint hs,FXint vs):FXPacker(p,opts,x,y,w,h,pl,pr,pt,pb,hs,vs){
  relWidth=relw;
  relHeight=relh;
  }


// Compute minimum width based on child layout hints
FXint FXSpring::getDefaultWidth(){
  return 0<relWidth ? relWidth : FXPacker::getDefaultWidth();
  }


// Compute minimum height based on child layout hints
FXint FXSpring::getDefaultHeight(){
  return 0<relHeight ? relHeight : FXPacker::getDefaultHeight();
  }


// Change relative width
void FXSpring::setRelativeWidth(FXint relw){
  if(relWidth!=relw){
    relWidth=relw;
    recalc();
    update();
    }
  }


// Change relative height
void FXSpring::setRelativeHeight(FXint relh){
  if(relHeight!=relh){
    relHeight=relh;
    recalc();
    update();
    }
  }


// Save object to stream
void FXSpring::save(FXStream& store) const {
  FXPacker::save(store);
  store << relWidth;
  store << relHeight;
  }


// Load object from stream
void FXSpring::load(FXStream& store){
  FXPacker::load(store);
  store >> relWidth;
  store >> relHeight;
  }

}
