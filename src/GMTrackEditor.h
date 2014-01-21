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
  GMTrack           info;
  FXString          infotags;
  GMAudioProperties properties;
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
  GMTextField   * tagsfield;
  GMCheckButton * updatetags;
  GMCheckButton * updatefilename;
  GMCheckButton * autonumber;
  GMSpinner     * autonumberoffset;
  GMSpinner     * trackspinner;
  GMTextField   * bitratefield;
  GMTextField   * sampleratefield;
  GMTextField   * channelfield;
  GMTextField   * sizefield;
  GMTextField   * filenamefield;
  GMTextField   * typefield;
  FXLabel       * bitratelabel;
protected:
  GMEditTrackDialog(){}
  void getTrackSelection();
  void displayTracks();
  FXbool saveTracks();
private:
  GMEditTrackDialog(const GMEditTrackDialog&);
  GMEditTrackDialog &operator=(const GMEditTrackDialog&);
public:
  enum {
    ID_FILENAME_TEMPLATE=FXDialogBox::ID_LAST,
    ID_NEXT_TRACK,
    ID_PREV_TRACK,
    ID_RESET
    };
protected:
  enum {
    SAME_ALBUM      =0x001,
    SAME_ARTIST     =0x002,
    SAME_ALBUMARTIST=0x004,
    SAME_GENRE      =0x008,
    SAME_YEAR       =0x010,
    SAME_DISC       =0x020,
    SAME_COMPOSER   =0x040,
    SAME_CONDUCTOR  =0x080,
    SAME_TAGS       =0x100,
    };
public:
  long onCmdAccept(FXObject*,FXSelector,void*);
  long onCmdArtist(FXObject*,FXSelector,void*);
  long onCmdFilenameTemplate(FXObject*,FXSelector,void*);
  long onCmdSwitchTrack(FXObject*,FXSelector,void*);
  long onCmdResetTrack(FXObject*,FXSelector,void*);
public:
  GMEditTrackDialog(FXWindow *,GMTrackDatabase*);
  virtual ~GMEditTrackDialog();
  };

#endif
