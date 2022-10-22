/********************************************************************************
*                                                                               *
*                   J P E G - 2 0 0 0   I m a g e   O b j e c t                 *
*                                                                               *
*********************************************************************************
* Copyright (C) 2009,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "FXJP2Image.h"


/*
  Notes:
  - Support for JPEG 2000 image file compression.
*/

using namespace FX;

/*******************************************************************************/

namespace FX {


// Suggested file extension
const FXchar FXJP2Image::fileExt[]="jp2";


// Suggested mime type
const FXchar FXJP2Image::mimeType[]="image/jp2";


// Object implementation
FXIMPLEMENT(FXJP2Image,FXImage,nullptr,0)


#ifdef HAVE_J2K_H
const FXbool FXJP2Image::supported=true;
#else
const FXbool FXJP2Image::supported=false;
#endif


// Initialize
FXJP2Image::FXJP2Image(FXApp* a,const FXuchar *pix,FXuint opts,FXint w,FXint h,FXint q):FXImage(a,nullptr,opts,w,h),quality(q){
  if(pix){
    FXMemoryStream ms(FXStreamLoad,const_cast<FXuchar*>(pix));
    loadPixels(ms);
    }
  }


// Save the pixels only
FXbool FXJP2Image::savePixels(FXStream& store) const {
  if(fxsaveJP2(store,data,width,height,quality)){
    return true;
    }
  return false;
  }


// Load pixels only
FXbool FXJP2Image::loadPixels(FXStream& store){
  FXColor *pixels; FXint w,h;
  if(fxloadJP2(store,pixels,w,h,quality)){
    setData(pixels,IMAGE_OWNED,w,h);
    return true;
    }
  return false;
  }


// Clean up
FXJP2Image::~FXJP2Image(){
  }

}
