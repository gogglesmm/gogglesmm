/********************************************************************************
*                                                                               *
*                           C o l o r   D i a l o g                             *
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
#include "FXEvent.h"
#include "FXWindow.h"
#include "FXApp.h"
#include "FXFrame.h"
#include "FXLabel.h"
#include "FXButton.h"
#include "FXComposite.h"
#include "FXPacker.h"
#include "FXShell.h"
#include "FXTopWindow.h"
#include "FXDialogBox.h"
#include "FXColorSelector.h"
#include "FXColorDialog.h"


/*
  Notes:
  - Need shared instance of this dialog to pop up when double-clicking
    on a color well.
*/

using namespace FX;

/*******************************************************************************/

namespace FX {

// Color dialog registry section name
const FXchar FXColorDialog::sectionName[]="Color Dialog";


// Map
FXDEFMAP(FXColorDialog) FXColorDialogMap[]={
  FXMAPFUNC(SEL_CHANGED,FXColorDialog::ID_COLORSELECTOR,FXColorDialog::onChgColor),
  FXMAPFUNC(SEL_COMMAND,FXColorDialog::ID_COLORSELECTOR,FXColorDialog::onCmdColor),
  FXMAPFUNC(SEL_COMMAND,FXColorDialog::ID_SETINTVALUE,FXColorDialog::onCmdSetIntValue),
  FXMAPFUNC(SEL_COMMAND,FXColorDialog::ID_GETINTVALUE,FXColorDialog::onCmdGetIntValue),
  };


// Object implementation
FXIMPLEMENT(FXColorDialog,FXDialogBox,FXColorDialogMap,ARRAYNUMBER(FXColorDialogMap))


// Construct color dialog box
FXColorDialog::FXColorDialog(FXWindow* own,const FXString& name,FXuint opts,FXint x,FXint y,FXint w,FXint h):FXDialogBox(own,name,opts|DECOR_TITLE|DECOR_BORDER|DECOR_RESIZE|DECOR_CLOSE,x,y,w,h,0,0,0,0,4,4){
  colorbox=new FXColorSelector(this,this,ID_COLORSELECTOR,LAYOUT_FILL_X|LAYOUT_FILL_Y);
  colorbox->acceptButton()->setTarget(this);
  colorbox->acceptButton()->setSelector(FXDialogBox::ID_ACCEPT);
  colorbox->cancelButton()->setTarget(this);
  colorbox->cancelButton()->setSelector(FXDialogBox::ID_CANCEL);
  }


// Construct free-floating color dialog box
FXColorDialog::FXColorDialog(FXApp* a,const FXString& name,FXuint opts,FXint x,FXint y,FXint w,FXint h):FXDialogBox(a,name,opts|DECOR_TITLE|DECOR_BORDER|DECOR_RESIZE|DECOR_CLOSE,x,y,w,h,0,0,0,0,4,4){
  colorbox=new FXColorSelector(this,this,ID_COLORSELECTOR,LAYOUT_FILL_X|LAYOUT_FILL_Y);
  colorbox->acceptButton()->setTarget(this);
  colorbox->acceptButton()->setSelector(FXDialogBox::ID_ACCEPT);
  colorbox->cancelButton()->setTarget(this);
  colorbox->cancelButton()->setSelector(FXDialogBox::ID_CANCEL);
  }


// Create server-side resources
void FXColorDialog::create(){
  readRegistry();
  FXDialogBox::create();
  }


// Destroy server-side resources
void FXColorDialog::destroy(){
  FXDialogBox::destroy();
  writeRegistry();
  }

// Load settings from registry
void FXColorDialog::readRegistry(){
  setWidth(getApp()->reg().readIntEntry(sectionName,"width",getWidth()));
  setHeight(getApp()->reg().readIntEntry(sectionName,"height",getHeight()));
  setWellColor( 0,getApp()->reg().readColorEntry(sectionName,"WA",FXRGBA(255,255,255,255)));
  setWellColor( 1,getApp()->reg().readColorEntry(sectionName,"WB",FXRGBA(204,204,204,255)));
  setWellColor( 2,getApp()->reg().readColorEntry(sectionName,"WC",FXRGBA(153,153,153,255)));
  setWellColor( 3,getApp()->reg().readColorEntry(sectionName,"WD",FXRGBA(102,102,102,255)));
  setWellColor( 4,getApp()->reg().readColorEntry(sectionName,"WE",FXRGBA( 51, 51, 51,255)));
  setWellColor( 5,getApp()->reg().readColorEntry(sectionName,"WF",FXRGBA(  0,  0,  0,255)));
  setWellColor( 6,getApp()->reg().readColorEntry(sectionName,"WG",FXRGBA(255,  0,  0,255)));
  setWellColor( 7,getApp()->reg().readColorEntry(sectionName,"WH",FXRGBA(  0,255,  0,255)));
  setWellColor( 8,getApp()->reg().readColorEntry(sectionName,"WI",FXRGBA(  0,  0,255,255)));
  setWellColor( 9,getApp()->reg().readColorEntry(sectionName,"WJ",FXRGBA(  0,255,255,255)));
  setWellColor(10,getApp()->reg().readColorEntry(sectionName,"WK",FXRGBA(255,255,  0,255)));
  setWellColor(11,getApp()->reg().readColorEntry(sectionName,"WL",FXRGBA(255,  0,255,255)));
  setWellColor(12,getApp()->reg().readColorEntry(sectionName,"WM",FXRGBA(255,165,  0,255)));
  setWellColor(13,getApp()->reg().readColorEntry(sectionName,"WN",FXRGBA(153,  0,  0,255)));
  setWellColor(14,getApp()->reg().readColorEntry(sectionName,"WO",FXRGBA(  0,153,  0,255)));
  setWellColor(15,getApp()->reg().readColorEntry(sectionName,"WP",FXRGBA(  0,  0,153,255)));
  setWellColor(16,getApp()->reg().readColorEntry(sectionName,"WQ",FXRGBA(  0,153,153,255)));
  setWellColor(17,getApp()->reg().readColorEntry(sectionName,"WR",FXRGBA(153,153,  0,255)));
  setWellColor(18,getApp()->reg().readColorEntry(sectionName,"WS",FXRGBA(153,  0,153,255)));
  setWellColor(19,getApp()->reg().readColorEntry(sectionName,"WT",FXRGBA(255,175,175,255)));
  setWellColor(20,getApp()->reg().readColorEntry(sectionName,"WU",FXRGBA(175,255,175,255)));
  setWellColor(21,getApp()->reg().readColorEntry(sectionName,"WV",FXRGBA(175,175,255,255)));
  setWellColor(22,getApp()->reg().readColorEntry(sectionName,"WW",FXRGBA(175,255,255,255)));
  setWellColor(23,getApp()->reg().readColorEntry(sectionName,"WX",FXRGBA(255,255,175,255)));
  setActivePanel(getApp()->reg().readIntEntry(sectionName,"activecolorpane",COLORTAB_COLOR_RING));
  }


// Save settings to registry
void FXColorDialog::writeRegistry(){
  getApp()->reg().writeIntEntry(sectionName,"width",getWidth());
  getApp()->reg().writeIntEntry(sectionName,"height",getHeight());
  getApp()->reg().writeColorEntry(sectionName,"WA",getWellColor( 0));
  getApp()->reg().writeColorEntry(sectionName,"WB",getWellColor( 1));
  getApp()->reg().writeColorEntry(sectionName,"WC",getWellColor( 2));
  getApp()->reg().writeColorEntry(sectionName,"WD",getWellColor( 3));
  getApp()->reg().writeColorEntry(sectionName,"WE",getWellColor( 4));
  getApp()->reg().writeColorEntry(sectionName,"WF",getWellColor( 5));
  getApp()->reg().writeColorEntry(sectionName,"WG",getWellColor( 6));
  getApp()->reg().writeColorEntry(sectionName,"WH",getWellColor( 7));
  getApp()->reg().writeColorEntry(sectionName,"WI",getWellColor( 8));
  getApp()->reg().writeColorEntry(sectionName,"WJ",getWellColor( 9));
  getApp()->reg().writeColorEntry(sectionName,"WK",getWellColor(10));
  getApp()->reg().writeColorEntry(sectionName,"WL",getWellColor(11));
  getApp()->reg().writeColorEntry(sectionName,"WM",getWellColor(12));
  getApp()->reg().writeColorEntry(sectionName,"WN",getWellColor(13));
  getApp()->reg().writeColorEntry(sectionName,"WO",getWellColor(14));
  getApp()->reg().writeColorEntry(sectionName,"WP",getWellColor(15));
  getApp()->reg().writeColorEntry(sectionName,"WQ",getWellColor(16));
  getApp()->reg().writeColorEntry(sectionName,"WR",getWellColor(17));
  getApp()->reg().writeColorEntry(sectionName,"WS",getWellColor(18));
  getApp()->reg().writeColorEntry(sectionName,"WT",getWellColor(19));
  getApp()->reg().writeColorEntry(sectionName,"WU",getWellColor(20));
  getApp()->reg().writeColorEntry(sectionName,"WV",getWellColor(21));
  getApp()->reg().writeColorEntry(sectionName,"WW",getWellColor(22));
  getApp()->reg().writeColorEntry(sectionName,"WX",getWellColor(23));
  getApp()->reg().writeIntEntry(sectionName,"activecolorpane",getActivePanel());
  }


// Change RGBA color
void FXColorDialog::setRGBA(FXColor clr){
  colorbox->setRGBA(clr);
  }


// Retrieve RGBA color
FXColor FXColorDialog::getRGBA() const {
  return colorbox->getRGBA();
  }


// Forward ColorSelector color change to target [a color well]
long FXColorDialog::onChgColor(FXObject*,FXSelector,void* ptr){
  return target && target->tryHandle(this,FXSEL(SEL_CHANGED,message),ptr);
  }


// Forward ColorSelector color command to target [a color well]
long FXColorDialog::onCmdColor(FXObject*,FXSelector,void* ptr){
  return target && target->tryHandle(this,FXSEL(SEL_COMMAND,message),ptr);
  }


// Update color dialog from a message
long FXColorDialog::onCmdSetIntValue(FXObject*,FXSelector,void* ptr){
  setRGBA(*((FXColor*)ptr));
  return 1;
  }


// Obtain value from color dialog
long FXColorDialog::onCmdGetIntValue(FXObject*,FXSelector,void* ptr){
  *((FXColor*)ptr)=getRGBA();
  return 1;
  }


// Change active panel
void FXColorDialog::setActivePanel(FXint pnl){
  colorbox->setActivePanel(pnl);
  }


// Return active panel
FXint FXColorDialog::getActivePanel() const {
  return colorbox->getActivePanel();
  }


// Change well color
void FXColorDialog::setWellColor(FXint w,FXColor clr){
  colorbox->setWellColor(w,clr);
  }


// Return well color
FXColor FXColorDialog::getWellColor(FXint w) const {
  return colorbox->getWellColor(w);
  }


// Change opaque only mode
void FXColorDialog::setOpaqueOnly(FXbool forceopaque){
  colorbox->setOpaqueOnly(forceopaque);
  }


// Return true if only opaque colors allowed
FXbool FXColorDialog::isOpaqueOnly() const {
  return colorbox->isOpaqueOnly();
  }


// Save data
void FXColorDialog::save(FXStream& store) const {
  FXDialogBox::save(store);
  store << colorbox;
  }


// Load data
void FXColorDialog::load(FXStream& store){
  FXDialogBox::load(store);
  store >> colorbox;
  }


// Cleanup
FXColorDialog::~FXColorDialog(){
  destroy();
  colorbox=(FXColorSelector*)-1L;
  }

}
