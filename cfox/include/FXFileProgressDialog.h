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
#ifndef FXFILEPROGRESSDIALOG_H
#define FXFILEPROGRESSDIALOG_H

#ifndef FXDIALOGBOX_H
#include "FXDialogBox.h"
#endif

namespace FX {


class FXHorizontalSeparator;
class FXProgressBar;
class FXLabel;
class FXIcon;

/**
* File Progress Dialog.
*/
class FXAPI FXFileProgressDialog : public FXDialogBox {
  FXDECLARE(FXFileProgressDialog)
protected:
  FXProgressBar *progress;      // Progress bar
  FXLabel       *activitylabel; // Label describing ongoing activity
  FXLabel       *originlabel;   // Original file name
  FXLabel       *targetlabel;   // Target file name
  FXLabel       *byteslabel;    // Bytes copied thus far
  FXLabel       *fileslabel;    // Files copied thus far
  FXLabel       *originfile;    // Origin file name
  FXLabel       *targetfile;    // Target file name
  FXLabel       *numbytes;      // Number of bytes
  FXLabel       *numfiles;      // Number of files
  FXbool         cancelled;     // Cancelled button was pressed
protected:
  FXFileProgressDialog();
private:
  FXFileProgressDialog(const FXFileProgressDialog&);
  FXFileProgressDialog &operator=(const FXFileProgressDialog&);
public:
  long onCmdCancel(FXObject*,FXSelector,void*);
public:

  /// Construct input dialog box with given caption, icon, and prompt text
  FXFileProgressDialog(FXWindow* owner,const FXString& caption,const FXString& label,FXIcon* ico=nullptr,FXuint opts=DECOR_TITLE|DECOR_BORDER|DECOR_RESIZE,FXint x=0,FXint y=0,FXint w=0,FXint h=0);

  /// Change the amount of progress
  void setProgress(FXuint value);

  /// Get current progress
  FXuint getProgress() const;

  /// Set total amount of progress
  void setTotal(FXuint value);

  /// Return total amount of progrss
  FXuint getTotal() const;

  /// Increment progress by given amount
  void increment(FXuint value);

  /// Change activity description
  void setActivityText(const FXString& text);

  /// Change activity icon
  void setActivityIcon(FXIcon* ico);

  /// Change origin filename
  void setOriginFile(const FXString& fn);

  /// Change target filename
  void setTargetFile(const FXString& fn);

  /// Change number of bytes
  void setNumBytes(FXlong num);

  /// Change number of files
  void setNumFiles(FXlong num);

  /// Access cancelled flag
  void setCancelled(FXbool flg);

  /// Check if operation is cancelled
  FXbool isCancelled() const;

  /// Destroy
  virtual ~FXFileProgressDialog();
  };

}

#endif
