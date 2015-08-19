/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2009-2015 by Sander Jansen. All Rights Reserved      *
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
#ifndef GMIMPORTDIALOG_H
#define GMIMPORTDIALOG_H

class GMFileSelector;
class GMDirSelector;

enum {
  IMPORT_FROMPASTE = 0x0,
  IMPORT_FROMFILE  = 0x1,
  IMPORT_FROMDIR   = 0x2,
  IMPORT_SYNC      = 0x4,
  IMPORT_PLAYLIST  = 0x8
  };

class GMFileDialog : public FXFileDialog {
FXDECLARE(FXFileDialog)
protected:
  GMFileDialog(){}
private:
  GMFileDialog(const GMFileDialog&);
  GMFileDialog &operator=(const GMFileDialog&);
public:
  GMFileDialog(FXWindow* owner,const FXString& name,FXuint opts=0,FXint x=0,FXint y=0,FXint w=500,FXint h=300);
  GMFileDialog(FXApp* a,const FXString& name,FXuint opts=0,FXint x=0,FXint y=0,FXint w=500,FXint h=300);
  };

class GMExportDialog : public GMFileDialog {
FXDECLARE(GMExportDialog)
protected:
  GMCheckButton * check_relative = nullptr;
protected:
  GMExportDialog(){}
private:
  GMExportDialog(const GMExportDialog&);
  GMExportDialog &operator=(const GMExportDialog&);
public:
  GMExportDialog(FXWindow* owner,const FXString& name,FXuint opts=0,FXint x=0,FXint y=0,FXint w=500,FXint h=300);
  GMExportDialog(FXApp* a,const FXString& name,FXuint opts=0,FXint x=0,FXint y=0,FXint w=500,FXint h=300);

  void setRelativePath(FXbool b) { check_relative->setCheck(b); }
  FXbool getRelativePath() const { return check_relative->getCheck(); }
  };


class GMImportDialog : public FXDialogBox {
FXDECLARE(GMImportDialog)
protected:
  FXuint mode = IMPORT_FROMDIR;
protected:
  FXDataTarget target_track_from_filelist;
  FXDataTarget target_replace_underscores;
  FXDataTarget target_default_field;
  FXDataTarget target_exclude_dir;
  FXDataTarget target_exclude_file;
  FXDataTarget target_parse_method;
  FXDataTarget target_filename_template;
  FXDataTarget target_id3v1_encoding;
protected:
  GMFileSelector  * fileselector = nullptr;
  GMDirSelector   * dirselector = nullptr;
  FXGroupBox      * template_grpbox = nullptr;
  FXListBox       * id3v1_listbox = nullptr;
protected:
  FXFontPtr         font_fixed;
protected:
  GMImportDialog(){}
  void getDefaultSearchDirectory(FXString&);
private:
  GMImportDialog(const GMImportDialog&);
  GMImportDialog &operator=(const GMImportDialog&);
public:
  enum {
    ID_SYNC_NEW = FXDialogBox::ID_LAST,
    ID_SYNC_REMOVE_MISSING,
    ID_SYNC_UPDATE,
    ID_SYNC_UPDATE_ALL,
    ID_SYNC_UPDATE_MODIFIED,
    ID_SYNC_REMOVE_ALL,
    ID_PARSE_METHOD
    };
public:
  long onCmdSync(FXObject*,FXSelector,void*);
  long onUpdSync(FXObject*,FXSelector,void*);
  long onCmdAccept(FXObject*,FXSelector,void*);
  long onUpdAccept(FXObject*,FXSelector,void*);
  long onCmdParseMethod(FXObject*,FXSelector,void*);
public:
  GMImportDialog(FXWindow * p,FXuint mode=IMPORT_FROMDIR);

  FXString getFilename() const;
  void getSelectedFiles(FXStringList & files);
  };

#endif
