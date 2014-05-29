/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2006-2014 by Sander Jansen. All Rights Reserved      *
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
#ifndef GMTRACKDATABASE_H
#define GMTRACKDATABASE_H

#ifndef GMDATABASE_H
#include "GMDatabase.h"
#endif

class GMTrackList;

enum {
  PLAYLIST_M3U,
  PLAYLIST_M3U_EXTENDED,
  PLAYLIST_XSPF,
  PLAYLIST_PLS,
  PLAYLIST_CSV
  };

enum {
  PLAYLIST_OPTIONS_RELATIVE = 0x1
  };

enum {
  INIT_MAIN,
  INIT_INSERT
  };


struct GMPlayListItem {
  FXint queue;
  FXint track;
  };

typedef FXArray<GMPlayListItem> GMPlayListItemList;


class GMTrackDatabase : public GMDatabase {
protected:
  FXHash   pathdict;
  FXHash   artistdict;
  FXString empty;
public:
  GMQuery insert_path;                  /// Insert Path
  GMQuery insert_artist;                /// Insert Artist;
  GMQuery insert_album;                 /// Insert Album;
  GMQuery insert_playlist_track_by_id;  /// Insert Track in Playlist
  GMQuery query_artist;                 /// Query unique artist
  GMQuery query_path;                   /// Query unique artist
  GMQuery query_filename;               /// Query filename
  GMQuery query_path_name;
  GMQuery query_album;                  /// Query unique album from artist
  GMQuery query_track;                  /// Query track
  GMQuery query_track_tags;             /// Query track tags
  //GMQuery query_playlist_queue;         /// Get the max playlist queue
  GMQuery query_track_filename;         /// Query filename by track id
  GMQuery query_album_artists;          /// Query artist and album for track
  GMQuery update_track_playcount;       /// Update Track as played
  GMQuery update_track_importdate;      /// Update Track as played
  GMQuery update_track_filename;  		  /// Update track filename

  GMQuery delete_track;						      /// Delete Track
  GMQuery delete_playlist_track;
  GMQuery delete_tag_track;
  GMQuery update_album;           /// Update Album
  GMQuery update_track_rating;    /// Update track rating

private: /// Called from init()
  FXbool init_database();
  FXbool init_queries();
  void   init_index();
  void   fix_empty_tags();
protected:
  FXbool reorderPlaylists();
  FXbool reorderPlaylist(FXint pl);
  FXbool reorderQueue();
protected:
  FXbool updateAlbum(FXint &album,const GMTrack&,FXint artist);

  void setup_path_lookup();
  void clear_path_lookup();
  void setup_artist_lookup();
  void clear_artist_lookup();

  void clean_tags();
public:
  /// Constructor
  GMTrackDatabase();

  /// Clear Tracks
  FXbool clearTracks(FXbool removeplaylists);

  /// Initialize the database. Return FALSE if failed else TRUE
  FXbool init(const FXString & filename);


  ///=======================================================================================
  ///   QUERY ITEMS
  ///=======================================================================================

  FXbool getStream(FXint id,GMStream & info);

  /// Return Track Info
  FXbool getTrack(FXint id,GMTrack & info);

  FXbool getTracks(const FXIntList &,GMTrackArray &);

  /// Return artist, album id
  FXbool getTrackAssociation(FXint id,FXint & artist,FXint & album);

  /// Return filename for track
  FXString getTrackFilename(FXint id);

  /// Return a list of filenames
  void getTrackFilenames(GMTrackFilenameList & list,const FXString & root=FXString::null);

  /// Return list of filenames
  void getTrackFilenames(const FXIntList & tracks,FXStringList & filenames);

  /// Return a filename from a given album
  FXbool getAlbumTrack(FXint id,FXString & filename);

  /// Return the track path
  void getTrackPath(FXint pid,FXString & path);

  /// Return the track path;
  const FXchar * getTrackPath(FXint pid) const;

  /// Return the track path;
  const FXString * getArtist(FXint aid);

  /// Get the track stats
  void getTrackStats(FXint & ntracks,FXint & nartists,FXint & nalbums,FXint & ntime,FXint playlist=0);

  /// Check if database is empty
  FXbool isEmpty();

  /// Return number of tracks in database
  FXint getNumTracks();

  /// Return number of albums in database
  FXint getNumAlbums();

  /// Return number of artists in database
  FXint getNumArtists();

  /// Return total time of all tracks
  FXint getTotalTime();

  /// Return Play Queue
  FXint getPlayQueue();


  /// Update Playlist Queue
  FXbool updateTrackPlaylists(FXint playlist,FXIntList & tracks);

  FXbool trackInPlaylist(FXint track,FXint playlist);

  /// Return next playlist queue
  FXint getNextQueue(FXint playlist);

  ///=======================================================================================
  ///   INSERTING ITEMS
  ///=======================================================================================

  FXint hasPath(const FXString & path);

  FXint hasTrack(const FXString &,FXint path);


  FXbool insertPlaylistTracks(FXint playlist,const FXIntList & tracks);

  FXbool insertStream(const FXString & url,const FXString & description,const FXString & genre);

  /// Set Track Import Date
  void setTrackImported(FXint id,FXlong tm);

  ///=======================================================================================
  ///   DELETING ITEMS
  ///=======================================================================================


  /// Remove Track from database
  void removeTrack(FXint id);
  void removeTracks(const FXIntList &ids);
  void removePlaylistTracks(FXint playlist,const FXIntList &);
  void removePlaylistQueue(FXint playlist,const FXIntList &);


  /// Remove Album from database
  FXbool removeAlbum(FXint id);

  /// Remove Artist from database
  FXbool removeArtist(FXint id);

  /// Remove Genre from database
  FXbool removeGenre(FXint id);

  /// Remove Playlist from database
  FXbool removePlaylist(FXint id);

  /// Remove Stream
  FXbool removeStream(FXint id);

  /// Synchronize all tables and makes sure no empty entries are left behind.
  FXbool vacuum();

  ///=======================================================================================
  ///   EDITING ITEMS
  ///=======================================================================================


  /// Set Track Disc
  void setTrackDiscNumber(const FXIntList & ids,FXushort disc);

  /// Set Track Number
  void setTrackTrackNumber(const FXIntList & ids,FXushort track,FXbool auto_increment=false);

  /// Set Disc and Track Number
  void setTrackNumber(const FXIntList & ids,FXuint disc,FXuint track,FXbool auto_increment=false);

  /// Set Track Rating
  void setTrackRating(FXint id,FXuchar rating);

  /// Set Track Played
  void setTrackPlayed(FXint id,FXlong time);

  /// Change the filename of id
  void setTrackFilename(FXint id,const FXString & filename);

  /// Set Track Title
  void setTrackTitle(FXint id,const FXString & name);

  /// Set Track Album
  void setTrackAlbum(const FXIntList & ids,const FXString & name,FXbool sameartist);

  /// Set Track Album Artist
  void setTrackAlbumArtist(const FXIntList & ids,const FXString & name,const FXString & album_title);

  /// Set Track Artist
  void setTrackArtist(const FXIntList & ids,const FXString & name);

  /// Set Track Composer
  void setTrackComposer(const FXIntList & ids,const FXString & name);

  /// Set Track Conductor
  void setTrackConductor(const FXIntList & ids,const FXString & name);

  /// Set Track Year
  void setTrackYear(const FXIntList & ids,FXuint year);

  /// Set Track Tags
  void setTrackTags(const FXIntList & ids,const FXStringList & tags);

  /// Update Album Year for tracks
  void updateAlbumYear(const FXIntList & ids);


  ///=======================================================================================
  /// Play Lists
  ///=======================================================================================

  /// Insert Playlist into database
  FXbool insertPlaylist(const FXString & name,FXint & id);

  /// Set Playlist Name
  FXbool setPlaylistName(FXint id,const FXString & name);

  /// Return Play list name for given id
  void getPlaylistName(FXint id,FXString &);

  /// Update Playlist
  FXbool updatePlaylist(FXint id,const GMPlayListItemList &);


  ///=======================================================================================
  ///  STREAMS
  ///=======================================================================================

  FXbool setStreamFilename(FXint id,const FXString & filename);

  FXbool setStreamDescription(FXint id,const FXString & description);

  FXbool setStreamGenre(FXint id,const FXString & genre);

  FXbool setStreamBitrate(FXint id,FXint rate);

  ///=======================================================================================
  ///   LISTING ITEMS
  ///=======================================================================================

  /// List Tags
  FXbool listTags(FXComboBox * list,FXbool insert_default=true);

  /// List Artists
  FXbool listArtists(FXComboBox * list);

  /// List album with artist appearing from track.
  FXbool listAlbums(FXComboBox * list,FXint track);

  /// List Playlists
  FXbool listPlaylists(FXIntList & ids);

  /// List Album Paths
  FXbool listAlbumPaths(GMCoverPathList & list);

  FXbool exportList(const FXString & filename,FXint playlist,FXuint format,FXuint opts=0);

  ///=======================================================================================
  ///  Sync API
  ///=======================================================================================
  void sync_tracks_removed();
  void sync_album_year();

  void initArtistLookup();

  /// Destructor
  ~GMTrackDatabase();
  };
#endif
