/********************************************************************************
*                                                                               *
*                     F i l e   P r o g r e s s   D i a l o g                   *
*                                                                               *
*********************************************************************************
* Copyright (C) 2016,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "FXGIFIcon.h"
#include "FXFrame.h"
#include "FXSeparator.h"
#include "FXLabel.h"
#include "FXButton.h"
#include "FXProgressBar.h"
#include "FXIcon.h"
#include "FXPacker.h"
#include "FXMatrix.h"
#include "FXHorizontalFrame.h"
#include "FXVerticalFrame.h"
#include "FXFileProgressDialog.h"

/*
  Notes:
  - Dialog for file copy, move, and so on.
*/

// Padding for buttons
#define HORZ_PAD 20
#define VERT_PAD 2

using namespace FX;

/*******************************************************************************/

namespace FX {


// Map
FXDEFMAP(FXFileProgressDialog) FXFileProgressDialogMap[]={
  FXMAPFUNC(SEL_COMMAND,FXFileProgressDialog::ID_CANCEL,FXFileProgressDialog::onCmdCancel),
  };


// Object implementation
FXIMPLEMENT(FXFileProgressDialog,FXDialogBox,FXFileProgressDialogMap,ARRAYNUMBER(FXFileProgressDialogMap))


// Serialization
FXFileProgressDialog::FXFileProgressDialog(){
  cancelled=false;
  }


// Create progress dialog box
FXFileProgressDialog::FXFileProgressDialog(FXWindow* own,const FXString& caption,const FXString& label,FXIcon* ico,FXuint opts,FXint x,FXint y,FXint w,FXint h):FXDialogBox(own,caption,opts,x,y,w,h,5,5,5,5, 0,0){
  FXHorizontalFrame* horz=new FXHorizontalFrame(this,LAYOUT_SIDE_BOTTOM|LAYOUT_FILL_X|PACK_UNIFORM_WIDTH,0,0,0,0,10,10,5,5);
  FXButton* cancel=new FXButton(horz,tr("&Cancel"),nullptr,this,ID_CANCEL,BUTTON_INITIAL|BUTTON_DEFAULT|FRAME_RAISED|FRAME_THICK|LAYOUT_SIDE_BOTTOM|LAYOUT_RIGHT,0,0,0,0,HORZ_PAD,HORZ_PAD,VERT_PAD,VERT_PAD);
  progress=new FXProgressBar(horz,nullptr,0,PROGRESSBAR_HORIZONTAL|PROGRESSBAR_PERCENTAGE|PROGRESSBAR_NORMAL|LAYOUT_CENTER_Y|LAYOUT_FILL_X,0,0,0,0);
  FXVerticalFrame *vert=new FXVerticalFrame(this,LAYOUT_FILL_X|LAYOUT_SIDE_TOP,0,0,0,0, 10,10,5,5);
  activitylabel=new FXLabel(vert,label,ico,ICON_BEFORE_TEXT|JUSTIFY_LEFT|JUSTIFY_CENTER_Y|LAYOUT_FILL_X|LAYOUT_TOP);
  new FXHorizontalSeparator(vert,SEPARATOR_GROOVE|LAYOUT_FILL_X|LAYOUT_TOP);
  FXMatrix *matrix=new FXMatrix(this,2,MATRIX_BY_COLUMNS|LAYOUT_FILL_X|LAYOUT_FILL_Y|LAYOUT_SIDE_TOP,0,0,0,0, 10,10,5,5);
  originlabel=new FXLabel(matrix,tr("From:"),nullptr,JUSTIFY_LEFT|LAYOUT_CENTER_Y|LAYOUT_FILL_X|LAYOUT_FILL_ROW);
  originfile=new FXLabel(matrix,"/usr/include/stdio.h",nullptr,JUSTIFY_LEFT|LAYOUT_CENTER_Y|LAYOUT_FILL_X|LAYOUT_FILL_ROW|LAYOUT_FILL_COLUMN);
  targetlabel=new FXLabel(matrix,tr("To:"),nullptr,JUSTIFY_LEFT|LAYOUT_CENTER_Y|LAYOUT_FILL_X|LAYOUT_FILL_ROW);
  targetfile=new FXLabel(matrix,"/usr/include/stdio.h",nullptr,JUSTIFY_LEFT|LAYOUT_CENTER_Y|LAYOUT_FILL_X|LAYOUT_FILL_ROW|LAYOUT_FILL_COLUMN);
  byteslabel=new FXLabel(matrix,tr("Bytes:"),nullptr,JUSTIFY_LEFT|LAYOUT_CENTER_Y|LAYOUT_FILL_X|LAYOUT_FILL_ROW);
  numbytes=new FXLabel(matrix,"10,000,000",nullptr,JUSTIFY_LEFT|LAYOUT_CENTER_Y|LAYOUT_FILL_X|LAYOUT_FILL_ROW|LAYOUT_FILL_COLUMN);
  fileslabel=new FXLabel(matrix,tr("Files:"),nullptr,JUSTIFY_LEFT|LAYOUT_CENTER_Y|LAYOUT_FILL_X|LAYOUT_FILL_ROW);
  numfiles=new FXLabel(matrix,"10",nullptr,JUSTIFY_LEFT|LAYOUT_CENTER_Y|LAYOUT_FILL_X|LAYOUT_FILL_ROW|LAYOUT_FILL_COLUMN);
  cancel->setFocus();
  cancelled=false;
  }


// Close dialog, cancelling operation in progress
long FXFileProgressDialog::onCmdCancel(FXObject* sender,FXSelector sel,void* ptr){
  FXDialogBox::onCmdCancel(sender,sel,ptr);
  setCancelled(true);
  return 1;
  }


// Change the amount of progress
void FXFileProgressDialog::setProgress(FXuint value){
  progress->setProgress(value);
  }


// Get current progress
FXuint FXFileProgressDialog::getProgress() const {
  return progress->getProgress();
  }


// Set total amount of progress
void FXFileProgressDialog::setTotal(FXuint value){
  progress->setTotal(value);
  }


// Return total amount of progrss
FXuint FXFileProgressDialog::getTotal() const {
  return progress->getTotal();
  }


// Increment progress by given amount
void FXFileProgressDialog::increment(FXuint value){
  progress->increment(value);
  }


// Change activity description
void FXFileProgressDialog::setActivityText(const FXString& text){
  activitylabel->setText(text);
  }


// Change activity icon
void FXFileProgressDialog::setActivityIcon(FXIcon* ico){
  activitylabel->setIcon(ico);
  }


// Change origin filename
void FXFileProgressDialog::setOriginFile(const FXString& fn){
  originfile->setText(fn);
  }


// Change target filename
void FXFileProgressDialog::setTargetFile(const FXString& fn){
  targetfile->setText(fn);
  }


// Change number of bytes
void FXFileProgressDialog::setNumBytes(FXlong num){
  FXString string;
  string.format("%'lld",num);
  numfiles->setText(string);
  }


// Change the number of files
void FXFileProgressDialog::setNumFiles(FXlong num){
  FXString string;
  string.format("%'lld",num);
  numfiles->setText(string);
  }


// Change cancelled flag
void FXFileProgressDialog::setCancelled(FXbool flg){
  cancelled=flg;
  }


// Has operation been cancelled?
FXbool FXFileProgressDialog::isCancelled() const {
  return cancelled;
  }


// Destroy it
FXFileProgressDialog::~FXFileProgressDialog(){
  progress=(FXProgressBar*)-1L;
  activitylabel=(FXLabel*)-1L;
  originlabel=(FXLabel*)-1L;
  targetlabel=(FXLabel*)-1L;
  byteslabel=(FXLabel*)-1L;
  fileslabel=(FXLabel*)-1L;
  originfile=(FXLabel*)-1L;
  targetfile=(FXLabel*)-1L;
  numbytes=(FXLabel*)-1L;
  numfiles=(FXLabel*)-1L;
  }

}
