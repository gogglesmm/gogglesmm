/********************************************************************************
*                                                                               *
*                    F o n t   S e l e c t i o n   D i a l o g                  *
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
#ifndef FXFONTDIALOG_H
#define FXFONTDIALOG_H

#ifndef FXDIALOGBOX_H
#include "FXDialogBox.h"
#endif

namespace FX {


class FXFontSelector;


/// Font selection dialog
class FXAPI FXFontDialog : public FXDialogBox {
  FXDECLARE(FXFontDialog)
protected:
  FXFontSelector *fontbox;
protected:
  static const FXchar sectionName[];
protected:
  FXFontDialog(){}
  void loadSettings();
  void saveSettings();
private:
  FXFontDialog(const FXFontDialog&);
  FXFontDialog &operator=(const FXFontDialog&);
public:

  /// Constructor font dialog box
  FXFontDialog(FXWindow* owner,const FXString& name,FXuint opts=0,FXint x=0,FXint y=0,FXint w=0,FXint h=0);

  /// Constructor free-floating font dialog box
  FXFontDialog(FXApp* a,const FXString& name,FXuint opts=0,FXint x=0,FXint y=0,FXint w=0,FXint h=0);

  /// Set font selection as a string
  void setFont(const FXString& string);

  /// Get font selection as a string
  FXString getFont() const;

  /// Set the current font selection
  void setFontDesc(const FXFontDesc& fontdesc);

  /// Get the current font selection
  const FXFontDesc& getFontDesc() const;

  /// Set sample text for font sample
  void setSampleText(const FXString& sampletext);

  /// Get sample text for font sample
  FXString getSampleText() const;

  /// Save dialog to a stream
  virtual void save(FXStream& store) const;

  /// Load dialog from a stream
  virtual void load(FXStream& store);

  /// Destructor
  virtual ~FXFontDialog();
  };

}

#endif
