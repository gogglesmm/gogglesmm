/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2006-2010 by Sander Jansen. All Rights Reserved      *
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
#ifdef HAVE_XINE_LIB
#ifndef GMEQDIALOG_H
#define GMEQDIALOG_H

class GMEQPreset {
public:
  FXString  name;
  GMEQBands bands;
  };


class GMEQDialog : public FXDialogBox {
FXDECLARE(GMEQDialog)
  static GMEQDialog * myself;
protected:
  FXFontPtr           font_small;
protected:
  FXArray<GMEQPreset> presets;
  FXRealSlider      * eqslider[10];
  GMListBox         * presetlist;
protected:
  GMEQDialog(){}
private:
  GMEQDialog(const GMEQDialog&);
  GMEQDialog &operator=(const GMEQDialog&);
protected:
  void listPresets();
public:
  static GMEQDialog * instance();
public:
  enum {
    ID_PRESET_EQ=FXDialogBox::ID_LAST,
    ID_SAVE,
    ID_RESET,
    ID_DELETE,
    ID_EQ_30HZ,
    ID_EQ_60HZ,
    ID_EQ_125HZ,
    ID_EQ_250HZ,
    ID_EQ_500HZ,
    ID_EQ_1000HZ,
    ID_EQ_2000HZ,
    ID_EQ_4000HZ,
    ID_EQ_8000HZ,
    ID_EQ_16000HZ
    };
public:
  long onCmdEQ(FXObject*,FXSelector,void*);
  long onUpdEQ(FXObject*,FXSelector,void*);
  long onCmdPresetEQ(FXObject*,FXSelector,void*);
  long onCmdReset(FXObject*,FXSelector,void*);
  long onCmdSave(FXObject*,FXSelector,void*);
  long onCmdDelete(FXObject*,FXSelector,void*);
  long onUpdReset(FXObject*,FXSelector,void*);
  long onUpdSave(FXObject*,FXSelector,void*);
  long onUpdDelete(FXObject*,FXSelector,void*);
public:
  GMEQDialog(FXWindow * p);
  virtual void hide();
  virtual ~GMEQDialog();
  };
#endif
#endif
