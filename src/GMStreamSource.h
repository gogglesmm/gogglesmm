/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2007-2026 by Sander Jansen. All Rights Reserved      *
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
*                               ---                                            *
* SPDX-License-Identifier: GPL-3.0-or-later                                    *
********************************************************************************/
#ifndef GMSTREAMSOURCE_H
#define GMSTREAMSOURCE_H

class GMSource;

class GMStreamSource : public GMSource {
FXDECLARE(GMStreamSource)
  GMTrackDatabase * db = nullptr;
protected:
  GMStreamSource() = default;
public:
  enum {
    ID_NEW_STATION = GMSource::ID_LAST,
    ID_EDIT_STATION,
    ID_DELETE_STATION,
    ID_LAST
    };
public:
  long onCmdNewStation(FXObject*,FXSelector,void*);
  long onCmdEditStation(FXObject*,FXSelector,void*);
  long onCmdDeleteStation(FXObject*,FXSelector,void*);
  long onUpdExport(FXObject*,FXSelector,void*);
public:
  explicit GMStreamSource(GMTrackDatabase * db);

  void configure(GMColumnList&) override;

  FXbool hasCurrentTrack(GMSource * ) const override;

  FXbool getTrack(GMTrack & info) const override;

  FXbool setTrack(GMTrack & info) const override;

  [[nodiscard]] FXString getName() const override { return fxtr("Internet Radio"); }

  [[nodiscard]] FXint getType() const override { return SOURCE_INTERNET_RADIO; }

  [[nodiscard]] FXString settingKey() const override { return "internet-radio"; }

  [[nodiscard]] FXint getSortColumn(FXbool) const override { return HEADER_TRACK; }

  [[nodiscard]] FXbool canBrowse() const override { return false; }

  [[nodiscard]] FXbool defaultBrowse() const override { return false; }

  [[nodiscard]] FXbool autoPlay() const override { return false; }

  FXbool source_menu(FXMenuPane * pane) override;

  FXbool source_context_menu(FXMenuPane * pane) override;

  FXbool track_context_menu(FXMenuPane * pane) override;

  FXbool listTracks(GMTrackList * tracklist,const FXIntList & albumlist,const FXIntList & genrelist) override;
  };

#endif
