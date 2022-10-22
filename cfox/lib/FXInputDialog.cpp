/********************************************************************************
*                                                                               *
*                         I n p u t   D i a l o g   B o x                       *
*                                                                               *
*********************************************************************************
* Copyright (C) 2000,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "FXString.h"
#include "FXSize.h"
#include "FXPoint.h"
#include "FXRectangle.h"
#include "FXStringDictionary.h"
#include "FXSettings.h"
#include "FXRegistry.h"
#include "FXAccelTable.h"
#include "FXEvent.h"
#include "FXWindow.h"
#include "FXApp.h"
#include "FXSeparator.h"
#include "FXLabel.h"
#include "FXButton.h"
#include "FXTextField.h"
#include "FXPacker.h"
#include "FXHorizontalFrame.h"
#include "FXVerticalFrame.h"
#include "FXInputDialog.h"

/*
  Notes:
  - This is a useful little class which we should probably have created
    sooner; I just didn't think of it until now.
*/

// Padding for buttons
#define HORZ_PAD 20
#define VERT_PAD 2

using namespace FX;

/*******************************************************************************/

namespace FX {

// Map
FXDEFMAP(FXInputDialog) FXInputDialogMap[]={
  FXMAPFUNC(SEL_COMMAND,FXDialogBox::ID_ACCEPT,FXInputDialog::onCmdAccept),
  };


// Object implementation
FXIMPLEMENT(FXInputDialog,FXDialogBox,FXInputDialogMap,ARRAYNUMBER(FXInputDialogMap))


// Create input dialog box
FXInputDialog::FXInputDialog(FXWindow* own,const FXString& caption,const FXString& label,FXIcon* icn,FXuint opts,FXint x,FXint y,FXint w,FXint h):FXDialogBox(own,caption,opts|DECOR_TITLE|DECOR_BORDER,x,y,w,h,10,10,10,10, 10,10){
  initialize(label,icn);
  }


// Create free floating input dialog box
FXInputDialog::FXInputDialog(FXApp* ap,const FXString& caption,const FXString& label,FXIcon* icn,FXuint opts,FXint x,FXint y,FXint w,FXint h):FXDialogBox(ap,caption,opts|DECOR_TITLE|DECOR_BORDER,x,y,w,h,10,10,10,10, 10,10){
  initialize(label,icn);
  }


// Build contents
void FXInputDialog::initialize(const FXString& label,FXIcon* icn){
  FXuint textopts=TEXTFIELD_ENTER_ONLY|FRAME_SUNKEN|FRAME_THICK|LAYOUT_FILL_X;
  FXHorizontalFrame* buttons=new FXHorizontalFrame(this,LAYOUT_SIDE_BOTTOM|LAYOUT_FILL_X|PACK_UNIFORM_WIDTH,0,0,0,0,0,0,0,0);
  new FXButton(buttons,tr("&OK"),nullptr,this,ID_ACCEPT,BUTTON_INITIAL|BUTTON_DEFAULT|FRAME_RAISED|FRAME_THICK|LAYOUT_CENTER_Y|LAYOUT_RIGHT,0,0,0,0,HORZ_PAD,HORZ_PAD,VERT_PAD,VERT_PAD);
  new FXButton(buttons,tr("&Cancel"),nullptr,this,ID_CANCEL,BUTTON_DEFAULT|FRAME_RAISED|FRAME_THICK|LAYOUT_CENTER_Y|LAYOUT_RIGHT,0,0,0,0,HORZ_PAD,HORZ_PAD,VERT_PAD,VERT_PAD);
  new FXHorizontalSeparator(this,SEPARATOR_GROOVE|LAYOUT_SIDE_BOTTOM|LAYOUT_FILL_X);
  FXHorizontalFrame* toppart=new FXHorizontalFrame(this,LAYOUT_SIDE_TOP|LAYOUT_FILL_X|LAYOUT_CENTER_Y,0,0,0,0, 0,0,0,0, 10,10);
  new FXLabel(toppart,FXString::null,icn,ICON_BEFORE_TEXT|JUSTIFY_CENTER_X|JUSTIFY_BOTTOM|LAYOUT_FILL_Y|LAYOUT_FILL_X);
  FXVerticalFrame* entry=new FXVerticalFrame(toppart,LAYOUT_FILL_X|LAYOUT_CENTER_Y,0,0,0,0, 0,0,0,0);
  new FXLabel(entry,label,nullptr,JUSTIFY_LEFT|ICON_BEFORE_TEXT|LAYOUT_TOP|LAYOUT_LEFT|LAYOUT_FILL_X);
  if(options&INPUTDIALOG_PASSWORD) textopts|=TEXTFIELD_PASSWD;
  if(options&INPUTDIALOG_INTEGER) textopts|=TEXTFIELD_INTEGER|JUSTIFY_RIGHT;
  if(options&INPUTDIALOG_REAL) textopts|=TEXTFIELD_REAL|JUSTIFY_RIGHT;
  input=new FXTextField(entry,20,this,ID_ACCEPT,textopts,0,0,0,0, 3,3,3,3);
  limlo=1.0;
  limhi=0.0;
  }


// Get input string
FXString FXInputDialog::getText() const {
  return input->getText();
  }


// Set input string
void FXInputDialog::setText(const FXString& text){
  input->setText(text);
  }


// Change number of visible columns of text
void FXInputDialog::setNumColumns(FXint num){
  input->setNumColumns(num);
  }


// Return number of visible columns of text
FXint FXInputDialog::getNumColumns() const {
  return input->getNumColumns();
  }


// We have to do this so as to force the initial text to be seleced
FXuint FXInputDialog::execute(FXuint placement){
  create();
  input->setFocus();
  input->selectAll();
  show(placement);
  return getApp()->runModalFor(this);
  }


extern FXAPI FXint __sscanf(const FXchar* string,const FXchar* format,...);


// Close dialog with an accept, after an validation of the number
long FXInputDialog::onCmdAccept(FXObject* sender,FXSelector sel,void* ptr){
  if(options&INPUTDIALOG_INTEGER){
    FXint iresult;
    if((__sscanf(input->getText().text(),"%d",&iresult)!=1) || (limlo<=limhi && (iresult<limlo || limhi<iresult))){
      input->setFocus();
      input->selectAll();
      getApp()->beep();
      return 1;
      }
    }
  else if(options&INPUTDIALOG_REAL){
    FXdouble dresult;
    if((__sscanf(input->getText().text(),"%lf",&dresult)!=1) || (limlo<=limhi && (dresult<limlo || limhi<dresult))){
      input->setFocus();
      input->selectAll();
      getApp()->beep();
      return 1;
      }
    }
  FXDialogBox::onCmdAccept(sender,sel,ptr);
  return 1;
  }


/*******************************************************************************/


// Obtain a string
FXbool FXInputDialog::getString(FXString& result,FXWindow* owner,const FXString& caption,const FXString& label,FXIcon* icon){
  FXInputDialog inputdialog(owner,caption,label,icon,INPUTDIALOG_STRING,0,0,0,0);
  inputdialog.setText(result);
  if(inputdialog.execute(PLACEMENT_OWNER)){
    result=inputdialog.getText();
    return true;
    }
  return false;
  }


// Obtain a string, in free floating window
FXbool FXInputDialog::getString(FXString& result,FXApp* app,const FXString& caption,const FXString& label,FXIcon* icon){
  FXInputDialog inputdialog(app,caption,label,icon,INPUTDIALOG_STRING,0,0,0,0);
  inputdialog.setText(result);
  if(inputdialog.execute(PLACEMENT_SCREEN)){
    result=inputdialog.getText();
    return true;
    }
  return false;
  }


// Obtain an integer
FXbool FXInputDialog::getInteger(FXint& result,FXWindow* owner,const FXString& caption,const FXString& label,FXIcon* icon,FXint lo,FXint hi){
  FXInputDialog inputdialog(owner,caption,label,icon,INPUTDIALOG_INTEGER,0,0,0,0);
  inputdialog.setLimits(lo,hi);
  inputdialog.setText(FXString::value(FXCLAMP(lo,result,hi)));
  if(inputdialog.execute(PLACEMENT_OWNER)){
    result=inputdialog.getText().toInt();
    return true;
    }
  return false;
  }


// Obtain an integer, in free floating window
FXbool FXInputDialog::getInteger(FXint& result,FXApp* app,const FXString& caption,const FXString& label,FXIcon* icon,FXint lo,FXint hi){
  FXInputDialog inputdialog(app,caption,label,icon,INPUTDIALOG_INTEGER,0,0,0,0);
  inputdialog.setLimits(lo,hi);
  inputdialog.setText(FXString::value(FXCLAMP(lo,result,hi)));
  if(inputdialog.execute(PLACEMENT_SCREEN)){
    result=inputdialog.getText().toInt();
    return true;
    }
  return false;
  }


// Obtain a real
FXbool FXInputDialog::getReal(FXdouble& result,FXWindow* owner,const FXString& caption,const FXString& label,FXIcon* icon,FXdouble lo,FXdouble hi){
  FXInputDialog inputdialog(owner,caption,label,icon,INPUTDIALOG_REAL,0,0,0,0);
  inputdialog.setLimits(lo,hi);
  inputdialog.setText(FXString::value(FXCLAMP(lo,result,hi),10));
  if(inputdialog.execute(PLACEMENT_OWNER)){
    result=inputdialog.getText().toDouble();
    return true;
    }
  return false;
  }


// Obtain a real, in free floating window
FXbool FXInputDialog::getReal(FXdouble& result,FXApp* app,const FXString& caption,const FXString& label,FXIcon* icon,FXdouble lo,FXdouble hi){
  FXInputDialog inputdialog(app,caption,label,icon,INPUTDIALOG_REAL,0,0,0,0);
  inputdialog.setLimits(lo,hi);
  inputdialog.setText(FXString::value(FXCLAMP(lo,result,hi),10));
  if(inputdialog.execute(PLACEMENT_SCREEN)){
    result=inputdialog.getText().toDouble();
    return true;
    }
  return false;
  }

}
