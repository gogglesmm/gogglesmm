/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2006-2017 by Sander Jansen. All Rights Reserved      *
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
  GMTrackDatabase * database = nullptr;
protected:
  FXDictionary pathdict;
  FXbool   album_format_grouping = true;
public:
  FXint    playlist       = 0;
  FXint    playlist_queue = 0;
  FXString default_artist;
  FXString default_album;
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
  FXint insertAlbum(const GMTrack & ,FXint album_artist_id);
  void insertTags(FXint,const FXStringList&);
  void updateTags(FXint,const FXStringList&);
  void initPathDict(GMTrackDatabase*);
public:
  GMDBTracks();

  // Init
  void init(GMTrackDatabase*,FXbool album_format_grouping=true);

  // Check for path
  FXint hasPath(const FXString & filename);

  // Remove Track
  void remove(FXint id);

  // Insert Track
  void insert(GMTrack & track);

  // Insert Track
  void insert(GMTrack & track,FXint & path_index);

  // Update Track
  void update(GMTrack & track);

  ~GMDBTracks(){}
  };


struct Seen;
class Lyrics;

class GMImportTask : public GMTask {
protected:
  GMTrackDatabase * database = nullptr;
  GMDBTracks        dbtracks;
  GMImportOptions   options;
  FXStringList      files;
  Lyrics*           lyrics = nullptr;
  FXint             count = 0;
protected:
  virtual FXint run();
protected:
  GMTrackArray  tracks;
  FXint        ntracks=0;
protected:
  // Return true if same composer is set on all tracks
  FXbool has_same_composer() const;
protected:
  // Import files and directories
  void import();

  // Parse track information
  void parse(const FXString & path,const FXString & file,FXint path_index);

  // Save scanned tracks. Pass path_index>=0 if from same folder.
  void import_tracks(FXint path_index=-1);

  // Scan path for files
  void scan(const FXString & path,Seen*,FXlong index);

  // Detect compilation from tracks found
  void detect_compilation();

  // Load track information from file or filename
  void load_track(const FXString & filename);

  // Initialize i3v1 encoding
  void setID3v1Encoding();
public:
  GMImportTask(FXObject*tgt=nullptr,FXSelector sel=0);

  void setOptions(const GMImportOptions & o);

  void setInput(const FXStringList & input) { files=input; }

  void setPlaylist(FXint p ) { dbtracks.playlist=p; }

  virtual ~GMImportTask();
  };

class GMSyncTask : public GMImportTask {
protected:
  GMSyncOptions  options_sync;
  FXbool         changed=false;
protected:
  virtual FXint run();
protected:

  // Insert or Update tracks
  void update_tracks(FXint pathindex);

  // Scan path for files
  void traverse(const FXString & path,Seen*,FXlong index);

  // Parse track information
  void parse_update(const FXString & path,const FXString & filename,FXTime modified,FXint pathindex);

  // Remove missing files from database
  void remove_missing();

  // Update files in database and import new ones
  void import_and_update();

  // Update files in database
  void update();

public:
  GMSyncTask(FXObject*tgt=nullptr,FXSelector sel=0);

  void setSyncOptions(const GMSyncOptions & o) { options_sync=o; }

  virtual ~GMSyncTask();
  };

class GMRemoveTask : public GMTask {
protected:
  GMDBTracks        dbtracks;
  GMTrackDatabase * database = nullptr;
  FXStringList      files;
  FXbool            changed = false;
protected:
  virtual FXint run();
public:
  GMRemoveTask(FXObject*tgt=NULL,FXSelector sel=0);

  void setInput(const FXStringList & input) { files=input; }

  void remove();

  virtual ~GMRemoveTask();
  };


#endif
