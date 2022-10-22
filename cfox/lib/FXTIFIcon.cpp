/********************************************************************************
*                                                                               *
*                          T I F F  I c o n   O b j e c t                       *
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
#include "FXTIFIcon.h"


/*
  Notes:
  - FXTIFIcon has an alpha channel.
*/

using namespace FX;

/*******************************************************************************/

namespace FX {


// Suggested file extension
const FXchar FXTIFIcon::fileExt[]="tif";


// Suggested mime type
const FXchar FXTIFIcon::mimeType[]="image/tiff";


// Object implementation
FXIMPLEMENT(FXTIFIcon,FXIcon,nullptr,0)


#ifdef HAVE_TIFF_H
const FXbool FXTIFIcon::supported=true;
#else
const FXbool FXTIFIcon::supported=false;
#endif


// Initialize
FXTIFIcon::FXTIFIcon(FXApp* a,const FXuchar *pix,FXColor clr,FXuint opts,FXint w,FXint h):FXIcon(a,nullptr,clr,opts,w,h),codec(0){
  if(pix){
    FXMemoryStream ms(FXStreamLoad,const_cast<FXuchar*>(pix));
    loadPixels(ms);
    }
  }


// Save pixels only
FXbool FXTIFIcon::savePixels(FXStream& store) const {
  if(fxsaveTIF(store,data,width,height,codec)){
    return true;
    }
  return false;
  }


// Load pixels only
FXbool FXTIFIcon::loadPixels(FXStream& store){
  FXColor *pixels; FXint w,h;
  if(fxloadTIF(store,pixels,w,h,codec)){
    setData(pixels,IMAGE_OWNED,w,h);
    if(options&IMAGE_ALPHAGUESS) setTransparentColor(guesstransp());
    if(options&IMAGE_THRESGUESS) setThresholdValue(guessthresh());
    return true;
    }
  return false;
  }


// Clean up
FXTIFIcon::~FXTIFIcon(){
  }

}
