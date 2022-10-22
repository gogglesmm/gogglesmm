/********************************************************************************
*                                                                               *
*                       T e x t   S e a r c h   D i a l o g                     *
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
#ifndef FXSEARCHDIALOG_H
#define FXSEARCHDIALOG_H

#ifndef FXREPLACEDIALOG_H
#include "FXReplaceDialog.h"
#endif

namespace FX {


/**
* Search Dialog.
* This dialog allows the user to enter a search string or pattern to
* search for; also, some common options such as case-sentitivity, search
* direction, and whether or not to perform a regular expression match may
* be entered.  The most recent couple of search paraneters are remembered
* for quick recall for convenience.
* Note that this dialog does not actually perform a search, it just provides
* an entry box for the search parameters.
*/
class FXAPI FXSearchDialog : public FXReplaceDialog {
  FXDECLARE(FXSearchDialog)
protected:
  FXSearchDialog(){}
private:
  FXSearchDialog(const FXSearchDialog&);
  FXSearchDialog &operator=(const FXSearchDialog&);
public:

  /// Construct search dialog box
  FXSearchDialog(FXWindow* owner,const FXString& caption,FXIcon* ic=nullptr,FXuint opts=0,FXint x=0,FXint y=0,FXint w=0,FXint h=0);

  /// Destructor
  virtual ~FXSearchDialog();
  };

}

#endif
