/********************************************************************************
*                                                                               *
*                        M e n u   P a n e   W i d g e t                        *
*                                                                               *
*********************************************************************************
* Copyright (C) 1997,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "fxkeys.h"
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
#include "FXAccelTable.h"
#include "FXFont.h"
#include "FXEvent.h"
#include "FXWindow.h"
#include "FXDCWindow.h"
#include "FXApp.h"
#include "FXIcon.h"
#include "FXMenuPane.h"


/*
  Notes:
  - Should FXPopup still exist?
*/

using namespace FX;

/*******************************************************************************/

namespace FX {

// Object implementation
FXIMPLEMENT(FXMenuPane,FXPopup,nullptr,0)



// Build empty one
FXMenuPane::FXMenuPane(FXWindow* own,FXuint opts):FXPopup(own,opts|FRAME_RAISED|FRAME_THICK){
  accelTable=new FXAccelTable;
  }


// Cursor is considered inside when it's in this window, or in any subwindow
// that's open; we'll find the latter through the cascade menu, by asking it for
// it's popup window.
FXbool FXMenuPane::contains(FXint parentx,FXint parenty) const {
  FXint x,y;
  if(FXPopup::contains(parentx,parenty)) return true;
  if(getFocus()){
    getParent()->translateCoordinatesTo(x,y,this,parentx,parenty);
    if(getFocus()->contains(x,y)) return true;
    }
  return false;
  }

}
