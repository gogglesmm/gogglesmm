/********************************************************************************
*                                                                               *
*                        P C X   I c o n   O b j e c t                          *
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
#include "FXIcon.h"
#include "FXPCXIcon.h"


/*
  Notes:
  - PCX does not support alpha in the file format.
  - You can also let the system guess a transparancy color based on the corners.
  - If that doesn't work, you can force a specific transparency color.
*/

using namespace FX;

/*******************************************************************************/

namespace FX {


// Suggested file extension
const FXchar FXPCXIcon::fileExt[]="pcx";


// Suggested mime type
const FXchar FXPCXIcon::mimeType[]="image/vnd.zbrush.pcx";


// Object implementation
FXIMPLEMENT(FXPCXIcon,FXIcon,nullptr,0)


// Initialize nicely
FXPCXIcon::FXPCXIcon(FXApp* a,const FXuchar *pix,FXColor clr,FXuint opts,FXint w,FXint h):FXIcon(a,nullptr,clr,opts,w,h){
  if(pix){
    FXMemoryStream ms(FXStreamLoad,const_cast<FXuchar*>(pix));
    loadPixels(ms);
    }
  }


// Save object to stream
FXbool FXPCXIcon::savePixels(FXStream& store) const {
  if(fxsavePCX(store,data,width,height)){
    return true;
    }
  return false;
  }


// Load object from stream
FXbool FXPCXIcon::loadPixels(FXStream& store){
  FXColor *pixels; FXint w,h;
  if(fxloadPCX(store,pixels,w,h)){
    setData(pixels,IMAGE_OWNED,w,h);
    if(options&IMAGE_ALPHAGUESS) setTransparentColor(guesstransp());
    if(options&IMAGE_THRESGUESS) setThresholdValue(guessthresh());
    return true;
    }
  return false;
  }


// Clean up
FXPCXIcon::~FXPCXIcon(){
  }

}
