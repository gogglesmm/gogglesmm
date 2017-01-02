/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2007-2017 by Sander Jansen. All Rights Reserved      *
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
#ifndef GMLOCALSOURCE_H
#define GMLOCALSOURCE_H

class GMSource;

class GMLocalSource : public GMSource {
FXDECLARE(GMLocalSource)
protected:
  FXStringList files;
  FXString     path;
  FXString     current_path;
private:
  GMLocalSource(const GMLocalSource&);
  GMLocalSource& operator=(const GMLocalSource&);
public:
  long onCmdRequestTrack(FXObject*,FXSelector,void*);
  long onCmdCopyTrack(FXObject*,FXSelector,void*);
public:
  GMLocalSource();

  FXbool findCurrent(GMTrackList * list,GMSource * src) override;

  void markCurrent(const GMTrackItem*) override;

  void configure(GMColumnList&) override;

  FXbool hasCurrentTrack(GMSource * ) const override;

  FXbool getTrack(GMTrack & info) const override;

  FXString getName() const override { return fxtr("File System"); }

  FXint getType() const override { return SOURCE_FILESYSTEM; }

  FXString settingKey() const override { return "file-system"; }

  void load(FXSettings&) override;

  void save(FXSettings&) const override;

  FXint getSortColumn(FXbool) const override { return HEADER_FILENAME; }

  FXbool canBrowse() const override { return false; }

  FXbool defaultBrowse() const override { return false; }

  FXbool autoPlay() const override { return true; }

//  FXbool source_context_menu(FXMenuPane * pane);

//  FXbool track_context_menu(FXMenuPane * pane);

  FXbool listTracks(GMTrackList * tracklist,const FXIntList & albumlist,const FXIntList & genrelist) override;

  FXbool track_double_click() override;

  FXuint dnd_provides(FXDragType types[]) override;

  virtual ~GMLocalSource();
  };

#endif
