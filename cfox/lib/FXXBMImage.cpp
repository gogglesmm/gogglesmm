/********************************************************************************
*                                                                               *
*                            X B M   I m a g e   O b j e c t                    *
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
#include "FXXBMImage.h"



/*
  Notes:
*/

using namespace FX;

/*******************************************************************************/

namespace FX {


// Suggested file extension
const FXchar FXXBMImage::fileExt[]="xbm";


// Suggested mime type
const FXchar FXXBMImage::mimeType[]="image/xbm";


// Object implementation
FXIMPLEMENT(FXXBMImage,FXImage,nullptr,0)


// Initialize
FXXBMImage::FXXBMImage(FXApp* a,const FXuchar *pixels,const FXuchar *mask,FXuint opts,FXint w,FXint h):FXImage(a,nullptr,opts,w,h){
  if(pixels && mask){
    fxloadXBM(data,pixels,mask,w,h);
    options|=IMAGE_OWNED;
    }
  }


// Save pixel data only
FXbool FXXBMImage::savePixels(FXStream& store) const {
  if(fxsaveXBM(store,data,width,height,-1,-1)){
    return true;
    }
  return false;
  }


// Load pixel data only
FXbool FXXBMImage::loadPixels(FXStream& store){
  FXColor *pixels; FXint w,h,hotx,hoty;
  if(fxloadXBM(store,pixels,w,h,hotx,hoty)){
    setData(pixels,IMAGE_OWNED,w,h);
    return true;
    }
  return false;
  }


// Clean up
FXXBMImage::~FXXBMImage(){
  }

}
