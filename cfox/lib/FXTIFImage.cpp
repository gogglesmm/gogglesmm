/********************************************************************************
*                                                                               *
*                          T I F F  I m a g e   O b j e c t                     *
*                                                                               *
*********************************************************************************
* Copyright (C) 2001,2022 Eric Gillet.   All Rights Reserved.                   *
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
#include "FXTIFImage.h"


/*
  Notes:
  - FXTIFImage has an alpha channel.
*/

using namespace FX;

/*******************************************************************************/

namespace FX {


// Suggested file extension
const FXchar FXTIFImage::fileExt[]="tif";


// Suggested mime type
const FXchar FXTIFImage::mimeType[]="image/tiff";


// Object implementation
FXIMPLEMENT(FXTIFImage,FXImage,nullptr,0)


#ifdef HAVE_TIFF_H
const FXbool FXTIFImage::supported=true;
#else
const FXbool FXTIFImage::supported=false;
#endif


// Initialize
FXTIFImage::FXTIFImage(FXApp* a,const FXuchar *pix,FXuint opts,FXint w,FXint h):FXImage(a,nullptr,opts,w,h),codec(0){
  if(pix){
    FXMemoryStream ms(FXStreamLoad,const_cast<FXuchar*>(pix));
    loadPixels(ms);
    }
  }


// Save the pixels only
FXbool FXTIFImage::savePixels(FXStream& store) const {
  if(fxsaveTIF(store,data,width,height,codec)){
    return true;
    }
  return false;
  }


// Load pixels only
FXbool FXTIFImage::loadPixels(FXStream& store){
  FXColor *pixels; FXint w,h;
  if(fxloadTIF(store,pixels,w,h,codec)){
    setData(pixels,IMAGE_OWNED,w,h);
    return true;
    }
  return false;
  }


// Clean up
FXTIFImage::~FXTIFImage(){
  }

}
