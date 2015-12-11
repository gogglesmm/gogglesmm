/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2006-2016 by Sander Jansen. All Rights Reserved      *
*                               ---                                            *
* This program is free software: you can redistribute it and/or modify         *
* it under the terms of the GNU General Public License as published by         *
* the Free Software Foundation, either version 3 of the License, or            *
* (at your option) any later version.                                          *
*                                                                              *
* This program is distributed in the hope that it will be useful,              *
* but WITHOUT ANY WARRANTY; without even the implied warranty of               *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                *
* GNU General Public License for more details.                                 *
*                                                                              *
* You should have received a copy of the GNU General Public License            *
* along with this program.  If not, see http://www.gnu.org/licenses.           *
********************************************************************************/
#ifndef FXABOUTDIALOG_H
#define FXABOUTDIALOG_H

class GMAboutDialog : public FXDialogBox {
  FXDECLARE(GMAboutDialog)
private:
  FXFontPtr titlefont;
  FXFontPtr licensefont;
  FXIconPtr logo;
private:
  void setup();
private:
  GMAboutDialog(){}
  GMAboutDialog(const GMAboutDialog&);
  GMAboutDialog& operator=(const GMAboutDialog&);
public:
  enum {
    ID_HOMEPAGE=FXDialogBox::ID_LAST,
    ID_REPORT_ISSUE,
    };
public:
  long onCmdHomePage(FXObject*,FXSelector,void*);
  long onCmdReportIssue(FXObject*,FXSelector,void*);
public:
/// Construct free-floating About dialog
  GMAboutDialog(FXApp* a);

  /// Construct dialog which will always float over the owner window
  GMAboutDialog(FXWindow* owner);

  /// Destructor
  virtual ~GMAboutDialog();
  };

#endif
