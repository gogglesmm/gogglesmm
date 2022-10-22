/********************************************************************************
*                                                                               *
*                        D D S   I c o n   O b j e c t                          *
*                                                                               *
*********************************************************************************
* Copyright (C) 2008,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "FXDDSImage.h"



/*
  Notes:
  - Support for displaying texture images; note, FOX decompresses these back to RGB;
    for direct use, the non-decompressed image data should be handed directly to
    OpenGL.
*/

using namespace FX;

/*******************************************************************************/

namespace FX {


// Suggested file extension
const FXchar FXDDSImage::fileExt[]="dds";


// Suggested mime type
const FXchar FXDDSImage::mimeType[]="image/x-dds";


// Object implementation
FXIMPLEMENT(FXDDSImage,FXImage,nullptr,0)


// Initialize
FXDDSImage::FXDDSImage(FXApp* a,const FXuchar *pix,FXuint opts,FXint w,FXint h):FXImage(a,nullptr,opts,w,h){
  if(pix){
    FXMemoryStream ms(FXStreamLoad,const_cast<FXuchar*>(pix));
    loadPixels(ms);
    }
  }


// Save pixel data only
FXbool FXDDSImage::savePixels(FXStream& store) const {
  if(fxsaveDDS(store,data,width,height,1)){
    return true;
    }
  return false;
  }


// Load pixel data only
FXbool FXDDSImage::loadPixels(FXStream& store){
  FXColor *pixels; FXint w,h,d;
  if(fxloadDDS(store,pixels,w,h,d)){
    setData(pixels,IMAGE_OWNED,w,h);
    return true;
    }
  return false;
  }


// Clean up
FXDDSImage::~FXDDSImage(){
  }

}
