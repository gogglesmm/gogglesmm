/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2006-2016 by Sander Jansen. All Rights Reserved      *
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
#ifndef GMSCANNER_H
#define GMSCANNER_H

class GMTask;


class GMDBTracks {
protected:
  GMTrackDatabase * database;
protected:
  FXDictionary pathdict;
protected:
  GMQuery insert_artist;
  GMQuery insert_album;
  GMQuery insert_tag;
  GMQuery insert_path;
  GMQuery insert_track;
  GMQuery insert_playlist_track;
  GMQuery insert_playlist_track_by_id;
  GMQuery insert_track_tag;
  GMQuery update_track;
  GMQuery query_album;
  GMQuery query_album_by_year;
  GMQuery query_artist;
  GMQuery query_tag;
  GMQuery delete_track;
  GMQuery delete_track_tags;
  GMQuery delete_track_playlists;
protected:
  FXint insertPath(const FXString & path);
  FXint insertArtist(const FXString & name);
  void insertTags(FXint,const FXStringList&);
  void updateTags(FXint,const FXStringList&);
  void initPathDict(GMTrackDatabase*);
public:
  GMDBTracks();

  void init(GMTrackDatabase*);

  void add(const FXString & filename,const GMTrack & track,FXint & pid,FXint playlist,FXint queue);

  void add2playlist(FXint playlist,FXint track,FXint queue);

  FXint hasPath(const FXString & filename);

  void update(FXint id,const GMTrack & info);

  void remove(FXint id);

  ~GMDBTracks();
  };


class GMImportTask : public GMTask {
protected:
  GMTrackDatabase * database;
  GMDBTracks        dbtracks;
  GMImportOptions   options;
  FXStringList      files;
  FXint             playlist;
  FXint             queue;
  FXint             count;
protected:
  void parse(const FXString & filename,FXint n,GMTrack &);
  void fixEmptyTags(GMTrack&,FXint n=-1);
  void fixAlbumArtist(GMTrack*,FXint ntracks);
protected:
  virtual FXint run();
  void import();
  void listDirectory(const FXString &);
public:
  GMImportTask(FXObject*tgt=NULL,FXSelector sel=0);

  void setOptions(const GMImportOptions & o) { options=o; }

  void setInput(const FXStringList & input) { files=input; }

  void setPlaylist(FXint p ) { playlist=p; }

  virtual ~GMImportTask();
  };

class GMSyncTask : public GMImportTask {
protected:
  GMSyncOptions  options_sync;
  FXint nchanged;
protected:
  virtual FXint run();
  void sync();
  void syncDirectory(const FXString &);
public:
  GMSyncTask(FXObject*tgt=NULL,FXSelector sel=0);

  void setSyncOptions(const GMSyncOptions & o) { options_sync=o; }

  virtual ~GMSyncTask();
  };



#endif
