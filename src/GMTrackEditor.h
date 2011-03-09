/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2009-2010 by Sander Jansen. All Rights Reserved      *
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
#ifndef GMEDITTRACKDIALOG_H
#define GMEDITTRACKDIALOG_H

class GMEditTrackDialog : public FXDialogBox {
FXDECLARE(GMEditTrackDialog)
protected:
  GMTrackDatabase * db;
  FXuint samemask;
  FXIntList tracks;
  GMTrack info;
  FXImage * art;
public:
  GMComboBox    * trackartistbox;
  GMComboBox    * albumartistbox;
  GMComboBox    * composerbox;
  GMComboBox    * conductorbox;
  GMComboBox    * genrebox;
  GMComboBox    * albumbox;
  GMTextField   * yearfield;
  GMSpinner     * discspinner;
  GMTextField   * discfield;
  GMTextField   * titlefield;
  GMCheckButton * updatetags;
  GMCheckButton * updatefilename;
  GMCheckButton * autonumber;
  GMSpinner     * autonumberoffset;
  GMSpinner     * trackspinner;
protected:
  GMEditTrackDialog(){}
private:
  GMEditTrackDialog(const GMEditTrackDialog&);
  GMEditTrackDialog &operator=(const GMEditTrackDialog&);
public:
  enum {
    ID_FILENAME_TEMPLATE=FXDialogBox::ID_LAST,
    };
protected:
  enum {
    SAME_ALBUM      =0x1,
    SAME_ARTIST     =0x2,
    SAME_ALBUMARTIST=0x4,
    SAME_GENRE      =0x8,
    SAME_YEAR       =0x10,
    SAME_DISC       =0x20,
    SAME_COMPOSER   =0x40,
    SAME_CONDUCTOR  =0x80,
    };
public:
  long onCmdAccept(FXObject*,FXSelector,void*);
  long onCmdArtist(FXObject*,FXSelector,void*);
  long onCmdFilenameTemplate(FXObject*,FXSelector,void*);
public:
  GMEditTrackDialog(FXWindow *,GMTrackDatabase*);
  virtual ~GMEditTrackDialog();
  };

#endif
