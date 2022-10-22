/********************************************************************************
*                                                                               *
*                         W E B P   I m a g e   O b j e c t                     *
*                                                                               *
*********************************************************************************
* Copyright (C) 1999,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "FXWEBPIcon.h"


using namespace FX;

/*******************************************************************************/

namespace FX {


// Suggested file extension
const FXchar FXWEBPIcon::fileExt[]="webp";


// Suggested mime type
const FXchar FXWEBPIcon::mimeType[]="image/webp";


// Object implementation
FXIMPLEMENT(FXWEBPIcon,FXIcon,nullptr,0)


#ifdef HAVE_WEBP_H
const FXbool FXWEBPIcon::supported=true;
#else
const FXbool FXWEBPIcon::supported=false;
#endif


// Initialize
FXWEBPIcon::FXWEBPIcon(FXApp* a,const FXuchar *pix,FXColor clr,FXuint opts,FXint w,FXint h):FXIcon(a,nullptr,clr,opts,w,h){
  if(pix){
    FXMemoryStream ms(FXStreamLoad,const_cast<FXuchar*>(pix));
    loadPixels(ms);
    }
  }


// Save pixels only
FXbool FXWEBPIcon::savePixels(FXStream& store) const {
  if(fxsaveWEBP(store,data,width,height,75.0f)){
    return true;
    }
  return false;
  }


// Load pixels only
FXbool FXWEBPIcon::loadPixels(FXStream& store){
  FXColor *pixels; FXint w,h;
  if(fxloadWEBP(store,pixels,w,h)){
    setData(pixels,IMAGE_OWNED,w,h);
    if(options&IMAGE_ALPHAGUESS) setTransparentColor(guesstransp());
    if(options&IMAGE_THRESGUESS) setThresholdValue(guessthresh());
    return true;
    }
  return false;
  }


// Clean up
FXWEBPIcon::~FXWEBPIcon(){
  }

}
