/********************************************************************************
*                                                                               *
*                     D i a l o g   B o x    O b j e c t                        *
*                                                                               *
*********************************************************************************
* Copyright (C) 1998,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "fxkeys.h"
#include "FXMutex.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXSize.h"
#include "FXPoint.h"
#include "FXRectangle.h"
#include "FXStringDictionary.h"
#include "FXSettings.h"
#include "FXRegistry.h"
#include "FXEvent.h"
#include "FXAccelTable.h"
#include "FXWindow.h"
#include "FXApp.h"
#include "FXDialogBox.h"

/*

  To do:
  - Iconified/normal.
  - Unmap when main window unmapped.
  - Transient For stuff.
  - Place so that cursor over dialog.
  - Hitting ESC will cancel out of the dialog.
  - Hitting RETURN will localate the default button, and then send it a RETURN;
    Note that the default button is initially assigned, but whichever button
    has the focus will be the default button; default-ness moves between buttons.
*/

using namespace FX;

/*******************************************************************************/

namespace FX {

// Map
FXDEFMAP(FXDialogBox) FXDialogBoxMap[]={
  FXMAPFUNC(SEL_CLOSE,0,FXDialogBox::onCmdCancel),
  FXMAPFUNC(SEL_COMMAND,FXDialogBox::ID_ACCEPT,FXDialogBox::onCmdAccept),
  FXMAPFUNC(SEL_CHORE,FXDialogBox::ID_CANCEL,FXDialogBox::onCmdCancel),
  FXMAPFUNC(SEL_TIMEOUT,FXDialogBox::ID_CANCEL,FXDialogBox::onCmdCancel),
  FXMAPFUNC(SEL_COMMAND,FXDialogBox::ID_CANCEL,FXDialogBox::onCmdCancel),
  };


// Object implementation
FXIMPLEMENT(FXDialogBox,FXTopWindow,FXDialogBoxMap,ARRAYNUMBER(FXDialogBoxMap))


// Contruct free floating dialog
FXDialogBox::FXDialogBox(FXApp* a,const FXString& name,FXuint opts,FXint x,FXint y,FXint w,FXint h,FXint pl,FXint pr,FXint pt,FXint pb,FXint hs,FXint vs):FXTopWindow(a,name,nullptr,nullptr,opts,x,y,w,h,pl,pr,pt,pb,hs,vs){
  getAccelTable()->addAccel(MKUINT(KEY_Escape,0),this,FXSEL(SEL_COMMAND,ID_CANCEL));
  }


// Contruct dialog which will stay on top of owner
FXDialogBox::FXDialogBox(FXWindow* own,const FXString& name,FXuint opts,FXint x,FXint y,FXint w,FXint h,FXint pl,FXint pr,FXint pt,FXint pb,FXint hs,FXint vs):FXTopWindow(own,name,nullptr,nullptr,opts,x,y,w,h,pl,pr,pt,pb,hs,vs){
  getAccelTable()->addAccel(MKUINT(KEY_Escape,0),this,FXSEL(SEL_COMMAND,ID_CANCEL));
  }


// Close dialog with an accept
long FXDialogBox::onCmdAccept(FXObject*,FXSelector,void*){
  getApp()->stopModal(this,true);
  hide();
  return 1;
  }


// Close dialog with a cancel
long FXDialogBox::onCmdCancel(FXObject*,FXSelector,void*){
  getApp()->stopModal(this,false);
  hide();
  return 1;
  }


// Execute dialog box modally; after creating the dialog and
// showing it on the screen, we call getApp()->refresh() to
// incur a GUI update pass over all the widgets; without an
// explicit call, the GUI update would only be scheduled after
// the message handler invoking this function would return.
FXuint FXDialogBox::execute(FXuint placement){
  create();
  show(placement);
  getApp()->refresh();
  return getApp()->runModalFor(this);
  }

}
