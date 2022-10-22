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
#include "FXObject.h"
#include "FXStringDictionary.h"
#include "FXSettings.h"
#include "FXRegistry.h"
#include "FXEvent.h"
#include "FXWindow.h"
#include "FXApp.h"
#include "FXImage.h"
#include "FXIcon.h"
#include "FXDDSIcon.h"


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
const FXchar FXDDSIcon::fileExt[]="dds";


// Suggested mime type
const FXchar FXDDSIcon::mimeType[]="image/x-dds";


// Object implementation
FXIMPLEMENT(FXDDSIcon,FXIcon,nullptr,0)


// Initialize nicely
FXDDSIcon::FXDDSIcon(FXApp* a,const FXuchar *pix,FXColor clr,FXuint opts,FXint w,FXint h):FXIcon(a,nullptr,clr,opts,w,h){
  if(pix){
    FXMemoryStream ms(FXStreamLoad,const_cast<FXuchar*>(pix));
    loadPixels(ms);
    }
  }


// Save object to stream
FXbool FXDDSIcon::savePixels(FXStream& store) const {
  if(fxsaveDDS(store,data,width,height,1)){
    return true;
    }
  return false;
  }


// Load object from stream
FXbool FXDDSIcon::loadPixels(FXStream& store){
  FXColor *pixels; FXint w,h,d;
  if(fxloadDDS(store,pixels,w,h,d)){
    setData(pixels,IMAGE_OWNED,w,h);
    if(options&IMAGE_ALPHAGUESS) setTransparentColor(guesstransp());
    if(options&IMAGE_THRESGUESS) setThresholdValue(guessthresh());
    return true;
    }
  return false;
  }


// Clean up
FXDDSIcon::~FXDDSIcon(){
  }

}
