/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2009-2016 by Sander Jansen. All Rights Reserved      *
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
#ifndef GMPLAYQUEUE_H
#define GMPLAYQUEUE_H

class GMPlayListSource;

class GMPlayQueue : public GMPlayListSource {
FXDECLARE(GMPlayQueue)
protected:
  FXIntMap tracks;
  FXint   ntracks  = 0;
  FXbool  poptrack = false;
protected:
  GMPlayQueue(){}
  void updateTrackHash();
private:
  GMPlayQueue(const GMPlayQueue&);
  GMPlayQueue& operator=(const GMPlayQueue&);
public:
  long onCmdRemoveInPlaylist(FXObject*,FXSelector,void*);
  long onCmdClear(FXObject*,FXSelector,void*);
public:
  enum {
    ID_CLEAR = GMPlayListSource::ID_LAST,
    ID_LAST
    };
public:
  GMPlayQueue(GMTrackDatabase * db);

  void configure(GMColumnList& list);

  FXbool canPlaySource(GMSource * src) const;

  FXbool findCurrent(GMTrackList * tracklist,GMSource * src);

  virtual FXint getNumTracks() const;

  virtual FXbool canBrowse() const { return false; }

  virtual FXbool source_context_menu(FXMenuPane * pane);

  virtual FXbool track_context_menu(FXMenuPane * pane);

  FXString getName() const;

  void addTracks(GMSource * src,const FXIntList & tracks);

  using GMSource::hasTrack;

  FXbool hasTrack(FXint id) const;

  FXint getCurrent();

  FXint getNext();

  virtual FXint getType() const { return SOURCE_PLAYQUEUE; }

  virtual ~GMPlayQueue();
  };
#endif
