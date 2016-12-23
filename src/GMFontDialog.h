/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2009-2017 by Sander Jansen. All Rights Reserved      *
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
#ifndef GMFONTDIALOG_H
#define GMFONTDIALOG_H

class GMFontDialog : public FXDialogBox {
FXDECLARE(GMFontDialog)
protected:
  FXFontDesc    selected    = {};
  GMList      * familylist  = nullptr;
  GMList      * stylelist   = nullptr;
  GMList      * sizelist    = nullptr;
  GMListBox   * pitchlist   = nullptr;
  GMListBox   * scalelist   = nullptr;
  GMTextField * sizefield   = nullptr;
  FXLabel     * preview     = nullptr;
  FXFont      * previewfont = nullptr;
protected:
  GMFontDialog(){}
private:
  GMFontDialog(const GMFontDialog&);
  GMFontDialog &operator=(const GMFontDialog&);
protected:
  void listFontFamily();
  void listFontStyle();
  void listFontSize();
  void previewFont();
public:
  enum {
    ID_FAMILY = FXDialogBox::ID_LAST,
    ID_STYLE,
    ID_SIZE,
    ID_PITCH,
    ID_SCALABLE,
    ID_SIZE_TEXT
    };
public:
  long onCmdFamily(FXObject*,FXSelector,void*);
  long onCmdStyle(FXObject*,FXSelector,void*);
  long onCmdSize(FXObject*,FXSelector,void*);
  long onCmdSizeText(FXObject*,FXSelector,void*);
  long onCmdPitch(FXObject*,FXSelector,void*);
  long onUpdPitch(FXObject*,FXSelector,void*);
  long onCmdScalable(FXObject*,FXSelector,void*);
  long onUpdScalable(FXObject*,FXSelector,void*);
public:
  GMFontDialog(FXApp * app,const FXString& name,FXuint opts=0,FXint x=0,FXint y=0,FXint w=400,FXint h=400);

  GMFontDialog(FXWindow* owner,const FXString& name,FXuint opts=DECOR_BORDER|DECOR_TITLE|DECOR_RESIZE,FXint x=0,FXint y=0,FXint w=400,FXint h=400);

  virtual void create();

  /// Set the current font selection
  void setFontDesc(const FXFontDesc& fontdesc);

  /// Get the current font selection
  const FXFontDesc& getFontDesc() const;

  virtual ~GMFontDialog();
  };

#endif
