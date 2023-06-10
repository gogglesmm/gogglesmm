/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2009-2021 by Sander Jansen. All Rights Reserved      *
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
  GMTrackDatabase * db = nullptr;
  FXuint samemask = 0;
  FXIntList tracks;
  GMTrack           info;
  FXString          infotags;
  GMAudioProperties properties;
public:
  GMComboBox    * trackartistbox   = nullptr;
  GMComboBox    * albumartistbox   = nullptr;
  GMComboBox    * composerbox      = nullptr;
  GMComboBox    * conductorbox     = nullptr;
  GMComboBox    * genrebox         = nullptr;
  GMComboBox    * albumbox         = nullptr;
  GMTextField   * yearfield        = nullptr;
  GMSpinner     * discspinner      = nullptr;
  GMTextField   * discfield        = nullptr;
  GMTextField   * titlefield       = nullptr;
  GMTextField   * tagsfield        = nullptr;
  GMCheckButton * updatetags       = nullptr;
  GMCheckButton * updatefilename   = nullptr;
  GMCheckButton * autonumber       = nullptr;
  GMSpinner     * autonumberoffset = nullptr;
  GMSpinner     * trackspinner     = nullptr;
  GMTextField   * bitratefield     = nullptr;
  GMTextField   * sampleratefield  = nullptr;
  GMTextField   * channelfield     = nullptr;
  GMTextField   * sizefield        = nullptr;
  GMTextField   * filenamefield    = nullptr;
  GMTextField   * typefield        = nullptr;
  FXLabel       * bitratelabel     = nullptr;
  FXText        * lyricsfield      = nullptr;
  GMTextField   * lyricsartist     = nullptr;
  GMTextField   * lyricstitle      = nullptr;
  GMButton      * lyricsfind       = nullptr;
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
    ID_RESET,
    ID_FETCH_LYRICS,
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
  long onCmdFetchLyrics(FXObject*,FXSelector,void*);
public:
  GMEditTrackDialog(FXWindow *,GMTrackDatabase*);

  virtual FXuint execute(FXuint placement=PLACEMENT_CURSOR);

  void setLyrics(const GMTrack & track);
  virtual ~GMEditTrackDialog();
  };

#endif
