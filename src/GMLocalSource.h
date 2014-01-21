/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2007-2010 by Sander Jansen. All Rights Reserved      *
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
  FXString path;
  FXString current_path;
private:
  GMLocalSource(const GMLocalSource&);
  GMLocalSource& operator=(const GMLocalSource&);
public:
  long onCmdRequestTrack(FXObject*,FXSelector,void*);
  long onCmdCopyTrack(FXObject*,FXSelector,void*);
public:
  GMLocalSource();

  virtual FXbool findCurrent(GMTrackList * list,GMSource * src);

  void markCurrent(const GMTrackItem*);

  void configure(GMColumnList&);

  FXbool hasCurrentTrack(GMSource * ) const;

  virtual FXbool getTrack(GMTrack & info) const;

//  virtual FXbool setTrack(GMTrack & info) const;

  FXString getName() const { return fxtr("File System"); }

  FXint getType() const { return SOURCE_FILESYSTEM; }

  FXString settingKey() const { return "file-system"; }

  void load(FXSettings&);

  void save(FXSettings&) const;

  FXint getSortColumn(FXbool) const { return HEADER_FILENAME; }

  FXbool canBrowse() const { return false; }

  FXbool defaultBrowse() const { return false; }

  FXbool autoPlay() const { return true; }

//  FXbool source_context_menu(FXMenuPane * pane);

//  FXbool track_context_menu(FXMenuPane * pane);

  FXbool listTracks(GMTrackList * tracklist,const FXIntList & albumlist,const FXIntList & genrelist);

  FXbool track_double_click();

  virtual FXuint dnd_provides(FXDragType types[]);

  virtual ~GMLocalSource();
  };

#endif
