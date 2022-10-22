/********************************************************************************
*                                                                               *
*                            P C X   I m a g e   O b j e c t                    *
*                                                                               *
*********************************************************************************
* Copyright (C) 2001,2022 by Janusz Ganczarski.   All Rights Reserved.          *
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
#include "FXMemoryStream.h"
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
#include "FXImage.h"
#include "FXPCXImage.h"



/*
  Notes:
  - Use corner color as transparency color, unless override.
*/

using namespace FX;

/*******************************************************************************/

namespace FX {


// Suggested file extension
const FXchar FXPCXImage::fileExt[]="pcx";


// Suggested mime type
const FXchar FXPCXImage::mimeType[]="image/vnd.zbrush.pcx";


// Object implementation
FXIMPLEMENT(FXPCXImage,FXImage,nullptr,0)


// Initialize
FXPCXImage::FXPCXImage(FXApp* a,const FXuchar *pix,FXuint opts,FXint w,FXint h):FXImage(a,nullptr,opts,w,h){
  if(pix){
    FXMemoryStream ms(FXStreamLoad,const_cast<FXuchar*>(pix));
    loadPixels(ms);
    }
  }


// Save pixel data only
FXbool FXPCXImage::savePixels(FXStream& store) const {
  if(fxsavePCX(store,data,width,height)){
    return true;
    }
  return false;
  }


// Load pixel data only
FXbool FXPCXImage::loadPixels(FXStream& store){
  FXColor *pixels; FXint w,h;
  if(fxloadPCX(store,pixels,w,h)){
    setData(pixels,IMAGE_OWNED,w,h);
    return true;
    }
  return false;
  }


// Clean up
FXPCXImage::~FXPCXImage(){
  }

}
