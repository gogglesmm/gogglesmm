/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2006-2010 by Sander Jansen. All Rights Reserved      *
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
#include "gmdefs.h"
#include "GMTrack.h"
#include "GMTrackDatabase.h"
#include "GMTrackList.h"
#include "GMSource.h"
#include "gmutils.h"

/// For listing default genres
#include <id3v1genres.h>


#define DEBUG_DB_GET() FXTRACE((51,"%s\n",__PRETTY_FUNCTION__))
#define DEBUG_DB_SET() FXTRACE((52,"%s\n",__PRETTY_FUNCTION__))


#define GOGGLESMM_DATABASE_SCHEMA_VERSION 2010
/*

*****************************************************
    Goggles Music Manager Database Schema v2010
*****************************************************

    TABLE tracks
        id          INTEGER NOT NULL *
        collection  INTEGER NOT NULL
        path        INTEGER NOT NULL REFERENCES pathlist (id),
        mrl         TEXT

        title       TEXT NOT NULL
        time        INTEGER
        no          INTEGER
        year        INTEGER
        replay_gain REAL
        replay_peak REAL
        bitrate     INTEGER

        album       INTEGER NOT NULL REFERENCES albums (id)
        artist      INTEGER NOT NULL REFERENCES artists (id)
        composer    INTEGER
        conductor   INTEGER

        playcount   INTEGER
        playdate    INTEGER
        importdate  INTEGER
        rating      INTEGER

    TABLE tags
        id        INTEGER NOT NULL *
        name      TEXT NOT NULL UNIQUE

    TABLE track_tags
        track     INTEGER NOT NULL REFERENCES tracks(id)*
        tag       INTEGER NOT NULL REFERENCES tags(id)*

    TABLE artists
        id        INTEGER NOT NULL *
        name      TEXT NOT NULL UNIQUE

    TABLE albums
        id        INTEGER NOT NULL *
        name      TEXT
        artists   INTEGER NOT NULL
        year      INTEGER NOT NULL
        replay_gain REAL
        replay_peak REAL

    TABLE playlists
        id        INTEGER NOT NULL *
        name      TEXT

    TABLE playlist_tracks
        playlist  INTEGER
        track     INTEGER
        queue     INTEGER

*/


const FXchar create_streams[]="CREATE TABLE IF NOT EXISTS streams ( id INTEGER NOT NULL, "
                                                                   "url TEXT, "
                                                                   "description TEXT, "
                                                                   "genre INTEGER, "
                                                                   "bitrate INTEGER, "
                                                                   "rating INTEGER, "
                                                                   "PRIMARY KEY (id) );";

const FXchar create_tracks[]="CREATE TABLE tracks ( id INTEGER NOT NULL,"
                                                  " collection INTEGER NOT NULL,"
                                                  " path INTEGER NOT NULL REFERENCES pathlist (id),"

                                                  " mrl TEXT,"
                                                  " title TEXT NOT NULL,"
                                                  " time INTEGER,"
                                                  " no INTEGER,"
                                                  " year INTEGER,"
                                                  " replay_gain REAL,"
                                                  " replay_peak REAL,"
                                                  " bitrate INTEGER,"

                                                  " album INTEGER NOT NULL REFERENCES albums (id),"
                                                  " artist INTEGER NOT NULL REFERENCES artists (id),"
//                                                  " album_artist INTEGER NOT NULL REFERENCES artists (id),"
                                                  " composer INTEGER,"
                                                  " conductor INTEGER,"

                                                  " playcount INTEGER,"
                                                  " playdate INTEGER,"
                                                  " importdate INTEGER,"
                                                  " rating INTEGER,"

                                                  " PRIMARY KEY (id) );";

const FXchar create_tags[]="CREATE TABLE tags ( id INTEGER NOT NULL,"
                                                  " name TEXT NOT NULL UNIQUE,"
                                                  " PRIMARY KEY (id) );";

const FXchar create_track_tags[]="CREATE TABLE track_tags ( track INTEGER NOT NULL REFERENCES tracks(id),"
                                                          " tag INTEGER NOT NULL REFERENCES tags(id),"
                                                          " PRIMARY KEY (track, tag) );";


const FXchar create_albums[]="CREATE TABLE albums ( id INTEGER NOT NULL,"
                                                  " name TEXT NOT NULL,"
                                                  " artist INTEGER NOT NULL REFERENCES artists (id),"
                                                  " year INTEGER,"
                                                  " replay_gain REAL,"
                                                  " replay_peak REAL,"
                                                  " PRIMARY KEY (id),"
                                                  " UNIQUE(name,artist) );";

const FXchar create_artists[]="CREATE TABLE artists ( id INTEGER NOT NULL,"
                                                    " name TEXT NOT NULL UNIQUE,"
                                                    " PRIMARY KEY (id) );";

const FXchar create_playlists[]="CREATE TABLE playlists ( id INTEGER NOT NULL,"
                                                         "name TEXT,"
                                                         "PRIMARY KEY (id));";

const FXchar create_playlist_tracks[]="CREATE TABLE playlist_tracks ( playlist INTEGER NOT NULL,"
                                                                     "track INTEGER NOT NULL,"
                                                                     "queue INTEGER );";

const FXchar create_pathlist[]="CREATE TABLE pathlist (id INTEGER NOT NULL,"
                                                      "name TEXT NOT NULL UNIQUE,"
                                                      "PRIMARY KEY (id));";





FXbool GMTrackDatabase::upgrade_to_2010() {
  return false;


//  if (!execute(create_playqueue))
//    return false;
#if 0
  execute("ALTER TABLE tracks ADD COLUMN collection INTEGER;");

  execute("ALTER TABLE tracks ADD COLUMN bitrate INTEGER;");

  execute("ALTER TABLE tracks ADD COLUMN composer INTEGER;");

  execute("ALTER TABLE tracks ADD COLUMN conductor INTEGER;");

  /// Convert 'directories' table to the new pathlist table
  ///-----------------------------------------------------------
  execute(create_pathlist);

  execute("UPDATE directories SET name='' WHERE name=='/';");

  execute("INSERT INTO pathlist SELECT d1.id,group_concat(d2.name,'/') FROM directories AS d1, directories AS d2 WHERE (d1.path || d1.id || '/')  LIKE (d2.path || d2.id || '/%') AND d1.id IN (SELECT DISTINCT(path) FROM tracks) GROUP BY d1.id;");

  execute("DROP TABLE directories;");

  ///-----------------------------------------------------------

  execute("CREATE INDEX IF NOT EXISTS tracks_album ON tracks(album);");
  execute("CREATE INDEX IF NOT EXISTS tracks_mrl ON tracks(mrl);");
  execute("CREATE INDEX IF NOT EXISTS playlist_tracks_idx ON playlist_tracks(track);");

  setVersion(GOGGLESMM_DATABASE_SCHEMA_VERSION);
#endif
  /// Rebuild the database
  execute("VACUUM;");
  return true;
  }

FXbool GMTrackDatabase::upgrade_to_2009() {

  execute("ALTER TABLE albums RENAME TO albums_old;");
  execute(create_albums);
  execute("INSERT INTO albums (id, name, artist, year) SELECT albums_old.id, name, artist,year FROM albums_old,album_artist,tracks WHERE album_artist.album == albums_old.id AND tracks.album==albums_old.id GROUP BY albums_old.id;");
  execute("DROP TABLE albums_old;");
  execute("DROP TABLE album_artist;");
  execute("ALTER TABLE tracks ADD COLUMN artist INTEGER;");
  execute("ALTER TABLE tracks ADD COLUMN replay_gain REAL;");
  execute("ALTER TABLE tracks ADD COLUMN replay_peak REAL;");
  execute("UPDATE tracks SET artist = ( SELECT DISTINCT(albums.artist) FROM albums WHERE albums.id == tracks.album );");

  FXdouble v=NAN;
  GMQuery q(this,"UPDATE tracks SET replay_gain = ?, replay_peak = ?;");
  q.set(0,v);
  q.set(1,v);
  q.execute();

  execute(create_streams);

  setVersion(2009);
  return true;
  }

























GMTrackDatabase::GMTrackDatabase()  {
  }

GMTrackDatabase::~GMTrackDatabase() {
  clear_path_lookup();
  clear_artist_lookup();
  }

FXbool GMTrackDatabase::init(const FXString & database) {

  if (!open(database))
    goto error;

  if ( getVersion()>GOGGLESMM_DATABASE_SCHEMA_VERSION) {
    if (FXMessageBox::question(FXApp::instance(),MBOX_OK_CANCEL,fxtr("Database Error"),fxtr("An incompatible (future) version of the database was found.\nThis usually happens when you try to downgrade to a older version of GMM\nPress OK to continue and reset the database (all information will be lost!).\nPress Cancel to quit now and leave the database as is."))==MBOX_CLICKED_CANCEL)
      return false;
    }

  /// No upgrade path yet
  if ( getVersion()<GOGGLESMM_DATABASE_SCHEMA_VERSION) {
    if (FXMessageBox::question(FXApp::instance(),MBOX_OK_CANCEL,fxtr("Database Error"),fxtr("An incompatible (older) version of the database was found.\nPress OK to continue and reset the database (all information will be lost!).\nPress Cancel to quit now and leave the database as is."))==MBOX_CLICKED_CANCEL)
      return false;
    }

  if (!init_database())
    goto error;

  if (!init_queries())
    goto error;

  return true;
error:
  FXMessageBox::error(FXApp::instance(),MBOX_OK,fxtr("Fatal Error"),fxtr("Goggles Music Manager was unable to open the database.\nThe database may have been corrupted. Please remove %s to try again.\nif the error keeps occuring, please file an issue at http://code.google.com/p/gogglesmm"),database.text());
  return false;
  }


FXbool GMTrackDatabase::init_database() {
  try {
    switch(getVersion()) {
      case GOGGLESMM_DATABASE_SCHEMA_VERSION:
        break;
/*
  /// DISABLED FOR NOW UNTIL WE HAVE AN UPGRADE PATH TO THE LATEST VERSION
      case 2008                             :
        upgrade_to_2009(); /// intentionally no break
      case 2009                             :
        //upgrade_to_2010();
        return false;
        break;
*/
      default                               :
        /// Some unknown database. Let's start from scratch
        reset();


        execute(create_tracks);
        execute(create_tags);
        execute(create_track_tags);
        execute(create_albums);
        execute(create_artists);
        execute(create_playlists);
        execute(create_playlist_tracks);
        execute(create_pathlist);
        execute(create_streams);



        execute("CREATE INDEX tracks_album ON tracks(album);");
//        execute("CREATE INDEX playlist_tracks_idx ON playlist_tracks(track);");
        execute("CREATE INDEX tracks_mrl ON tracks(mrl);");

        /// for hasTrack()
        execute("CREATE INDEX tracks_has_track ON tracks(path,mrl);");



//        execute("CREATE INDEX playlist_queue_idx ON playlist_tracks(queue DESC);");

#if 0
        /// Two triggers to automatically delete artists when they're not needed anymore.
        execute("CREATE TRIGGER IF NOT EXISTS artist_update_from_tracks AFTER DELETE ON tracks FOR EACH ROW "
                "WHEN 0 == ((SELECT count(id) FROM tracks WHERE artist == OLD.artist || composer == OLD.artist || conductor == OLD.artist) + (SELECT count(id) FROM albums WHERE artist == OLD.artist)) "
                "BEGIN "
                  "DELETE FROM artists WHERE id == OLD.artist;"
                "END;");

        execute("CREATE TRIGGER IF NOT EXISTS artist_update_from_albums AFTER DELETE ON albums FOR EACH ROW "
                "WHEN 0 == ((SELECT count(id) FROM tracks WHERE artist == OLD.artist || composer == OLD.artist || conductor == OLD.artist) + (SELECT count(id) FROM albums WHERE artist == OLD.artist)) "
                "BEGIN "
                  "DELETE FROM artists WHERE id == OLD.artist;"
                "END;");

         //// Remove album when tracks get deleted.
        execute("CREATE TRIGGER IF NOT EXISTS album_update AFTER DELETE ON tracks FOR EACH ROW "
                "WHEN 0 == (SELECT count(id) FROM tracks WHERE album == OLD.album) "
                "BEGIN "
                  "DELETE FROM albums WHERE id == OLD.album;"
                "END;");


        /// Remove pathlist item if not used anymore
        execute("CREATE TRIGGER IF NOT EXISTS pathlist_update AFTER DELETE ON tracks FOR EACH ROW "
                "WHEN 0 == (SELECT count(id) FROM tracks WHERE path == OLD.path) "
                "BEGIN "
                  "DELETE FROM pathlist WHERE id == OLD.path;"
                "END;");

        /// Remove genre if not used anymore
        execute("CREATE TRIGGER IF NOT EXISTS genre_update AFTER DELETE ON tracks FOR EACH ROW "
                "WHEN 0 == ((SELECT count(id) FROM tracks WHERE genre == OLD.genre) + (SELECT count(id) FROM streams WHERE genre == OLD.genre)) "
                "BEGIN "
                  "DELETE FROM genres WHERE id == OLD.genre;"
                "END;");

        /// Remove track from playlist if it gets deleted.
        execute("CREATE TRIGGER IF NOT EXISTS track_deleted AFTER DELETE ON tracks FOR EACH ROW "
                "BEGIN "
                  "DELETE FROM playlist_tracks WHERE track == OLD.id;"
                "END;");

        /// Remove tracks from album.
        execute("CREATE TRIGGER IF NOT EXISTS album_deleted AFTER DELETE ON albums FOR EACH ROW "
                "BEGIN "
                  "DELETE FROM tracks WHERE album == OLD.id;"
                "END;");

        /// Remove album and tracks from artist
        execute("CREATE TRIGGER IF NOT EXISTS artist_deleted AFTER DELETE ON artists FOR EACH ROW "
                "BEGIN "
                  "DELETE FROM albums WHERE artist == OLD.id;"
                  "DELETE FROM tracks WHERE artist == OLD.id;"
                "END;");

        /// Update the queue numbers when removing track from playlist
        /// execute("CREATE TRIGGER IF NOT EXISTS track_deleted_from_playlist AFTER DELETE ON playlist_tracks FOR EACH ROW "
        ///        "BEGIN "
        ///          "UPDATE playlist_tracks SET queue = queue - 1 WHERE playlist == OLD.playlist AND queue > OLD.queue; "
        ///        "END;");

        /// Remove tracks from playlists if playlist gets deleted.
        /// NEED A STATEMENT TRIGGER FOR THIS
        /// execute("CREATE TRIGGER IF NOT EXISTS playlist_deleted AFTER DELETE ON playlists FOR EACH ROW "
        ///        "BEGIN "
        ///          "DELETE FROM playlist_tracks WHERE playlist == OLD.id;"
        ///        "END;");

#endif
 //       execute("CREATE INDEX pathlist_path ON pathlist(name);");
 //       execute("CREATE INDEX artists_name ON artists(name);");
        setVersion(GOGGLESMM_DATABASE_SCHEMA_VERSION);
        break;
      }
    }
  catch(GMDatabaseException&) {
    return false;
    }
  return true;
  }


FXbool GMTrackDatabase::init_queries() {
  try {


  insert_genre                        = compile("INSERT OR IGNORE INTO tags VALUES ( NULL , ?  );");
  insert_artist                       = compile("INSERT OR IGNORE INTO artists VALUES ( NULL , ?  );");
  insert_album                        = compile("INSERT OR IGNORE INTO albums SELECT NULL, ?, (SELECT id FROM artists WHERE name == ?), ?, ?, ?;");
  insert_path                         = compile("INSERT OR IGNORE INTO pathlist VALUES (NULL,?);");

  insert_playlist_track_by_id         = compile("INSERT INTO playlist_tracks VALUES (?,?,?);");




  query_filename                      = compile("SELECT id FROM tracks WHERE path == ? AND mrl == ?;");






  query_path                          = compile("SELECT id FROM pathlist WHERE name = ?;");
  query_genre                         = compile("SELECT id FROM tags WHERE name == ?;");
  query_artist                        = compile("SELECT id FROM artists WHERE name == ?;");



  query_path_name                     = compile("SELECT name FROM pathlist WHERE id == ?;");

/*
  query_track                         = compile("SELECT pathlist.name || '" PATHSEPSTRING "' || mrl, albums.name, a1.name, a2.name, composer_artist.name, conductor_artist.name,genres.name, title, time, no, tracks.year, tracks.replay_gain, tracks.replay_peak, albums.replay_gain, albums.replay_peak, tracks.rating "
                                                "FROM tracks LEFT JOIN artists AS composer_artist ON tracks.composer == composer_artist.id LEFT JOIN artists AS conductor_artist ON tracks.conductor == conductor_artist.id,pathlist, albums, genres, artists AS a1, artists AS a2 "
                                                "WHERE tracks.path == pathlist.id "
                                                  "AND albums.id == tracks.album "
                                                  "ANd a1.id == albums.artist "
                                                  "AND genres.id == tracks.genre "
                                                  "AND a2.id == tracks.artist "
                                                  "AND tracks.id == ?;");
*/

  query_track                         = compile("SELECT pathlist.name || '" PATHSEPSTRING "' || mrl, albums.name, a1.name, a2.name, composer_artist.name, conductor_artist.name,title, time, no, tracks.year, tracks.replay_gain, tracks.replay_peak, albums.replay_gain, albums.replay_peak, tracks.rating "
                                                "FROM tracks LEFT JOIN artists AS composer_artist ON tracks.composer == composer_artist.id LEFT JOIN artists AS conductor_artist ON tracks.conductor == conductor_artist.id,pathlist, albums, artists AS a1, artists AS a2 "
                                                "WHERE tracks.path == pathlist.id "
                                                  "AND albums.id == tracks.album "
                                                  "ANd a1.id == albums.artist "
                                                  "AND a2.id == tracks.artist "
                                                  "AND tracks.id == ?;");

  query_track_tags                    = compile("SELECT name FROM tags WHERE id IN (SELECT tag FROM track_tags WHERE track == ?);");



  query_artist_name_album             = compile("SELECT name FROM artists WHERE id == ( SELECT artist FROM albums WHERE id == ? );");
  query_album_with_track              = compile("SELECT album,year FROM tracks WHERE id == ?;");
  query_track_filename                = compile("SELECT name ||'" PATHSEPSTRING "' || mrl FROM tracks,pathlist WHERE tracks.path == pathlist.id AND tracks.id == ?;");

  query_artist_year                   = compile("SELECT albums.artist,tracks.year FROM tracks,albums WHERE albums.id ==tracks.album AND tracks.id == ?;");
  query_album_artists                 = compile("SELECT albums.artist,album FROM tracks,albums WHERE albums.id ==tracks.album AND tracks.id == ?;");


  list_tags                           = compile("SELECT id,name FROM tags WHERE id IN (SELECT DISTINCT(tag) FROM track_tags);");
  list_artists                        = compile("SELECT id,name FROM artists;");

  list_albums_with_artist_from_track  = compile("SELECT albums.name FROM tracks AS t1 JOIN tracks AS t2 ON (t1.artist == t2.artist AND t1.id==?) JOIN albums ON (albums.id == t2.album) GROUP BY t2.album");

  query_playlist_queue                = compile("SELECT MAX(queue) FROM playlist_tracks WHERE playlist == ?;");
  insert_track_playlist               = compile("INSERT INTO playlist_tracks VALUES ( ? , ? , ?);");
  update_track_rating                 = compile("UPDATE tracks SET rating = ? WHERE id == ?;");


  update_track_filename               = compile("UPDATE tracks SET path = ?, mrl = ? WHERE id == ?;");
  update_track_playcount              = compile("UPDATE tracks SET playcount = playcount + 1, playdate = ? WHERE id == ?;");
  update_track_importdate             = compile("UPDATE tracks SET importdate = ? WHERE id == ?;");

  delete_track = compile("DELETE FROM tracks WHERE id == ?;");
  delete_playlist_track = compile("DELETE FROM playlist_tracks WHERE track == ?;");
  delete_tag_track = compile("DELETE FROM track_tags WHERE track == ?;");



  setup_path_lookup();
  setup_artist_lookup();
    }
  catch(GMDatabaseException&){
    return false;
    }
  return true;
  }




















FXbool GMTrackDatabase::clearTracks(FXbool removeplaylists){
  DEBUG_DB_SET();
  try {
    begin();

    execute("DELETE FROM tracks;");
    execute("DELETE FROM albums;");
    execute("DELETE FROM artists;");

    execute("DELETE FROM playlist_tracks;");
    execute("DELETE FROM track_tags;");


    if (removeplaylists) {
      execute("DELETE FROM playlists;");
      }

    execute("DELETE FROM pathlist;");

    execute("DELETE FROM tags WHERE id NOT IN (SELECT DISTINCT(tag) FROM track_tags) AND id NOT IN (SELECT DISTINCT(genre) FROM streams);");

    commit();
    }
  catch(GMDatabaseException&) {
    rollback();
    return false;
    }
  clear_path_lookup();
  return true;
  }


void GMTrackDatabase::clear(){
  DEBUG_DB_SET();
  try {
    begin();
    execute("DROP tracks;");
    execute("DROP artists;");
    execute("DROP albums;");
    execute("DROP pathlist;");
    execute("DROP playlist;");
    execute("DROP playlist_tracks;");
    execute("DROP tags;");
    execute("DROP track_tags;");
    execute("DROP streams;");
    commit();
    }
  catch(GMDatabaseException&) {
    rollback();
    }
  }




// void GMTrackDatabase::beginDelete() {
//  delete_track_from_playlist.compile("DELETE FROM playlist_tracks WHERE track == ? AND playlist == ?;");
//  delete_track_from_playlists.compile("DELETE FROM playlist_tracks WHERE track == ?;");
//  query_track_no.compile("SELECT no FROM tracks WHERE id == ?;");
//  }

// void GMTrackDatabase::endDelete(FXbool vac){
//  if (vac) vacuum();
//  delete_track_from_playlist.clear();
//  delete_track_from_playlists.clear();
//  query_track_no.clear();
//  }



FXint GMTrackDatabase::insertPath(const FXString & path) {
  DEBUG_DB_SET();
  GM_TICKS_START();
  FXint pid = insert_path.insert(path);
  pathdict.insert((void*)(FXival)pid,strdup(path.text()));
  GM_TICKS_END();
  return pid;
  }


FXint GMTrackDatabase::hasPath(const FXString & path){
  DEBUG_DB_GET();
  GM_TICKS_START();
  FXint pid=0;
  query_path.execute(path,pid);
  GM_TICKS_END();
  return pid;
  }


FXint GMTrackDatabase::hasTrack(const FXString & filename,FXint pid) {
  DEBUG_DB_GET();
  FXASSERT(pid);
  GM_TICKS_START();
  FXint tid=0;
  query_filename.set(0,pid);
  query_filename.set(1,filename);
  query_filename.execute(tid);
  GM_TICKS_END();
  return tid;
  }


































//FIXME rollback
FXbool GMTrackDatabase::insertStream(const FXString & url,const FXString & description,const FXString & genre){
  DEBUG_DB_SET();
  try {
    begin();
    FXint genreid=0;
    if (!genre.empty() && !queryGenre(genreid,genre,true))
      return false;
    GMQuery q(this,"INSERT INTO streams VALUES(NULL,?,?,?,0,0);");
    q.set(0,url);
    q.set(1,description);
    q.set(2,genreid);
    q.execute();
    commit();
    }
  catch(GMDatabaseException & e){
    rollback();
    return false;
    }
  return true;
  }


/// Insert Playlist into database
FXbool GMTrackDatabase::insertPlaylist(const FXString & name,FXint & id) {
  DEBUG_DB_SET();
  try {
    begin();
    GMQuery query(this,"INSERT INTO playlists VALUES ( NULL, ? );");
    id = query.insert(name);
    commit();
    }
  catch (GMDatabaseException & e){
    rollback();
    return false;
    }
  return true;
  }


FXint GMTrackDatabase::getNextQueue(FXint playlist) {
  DEBUG_DB_GET();
  FXint queue=1;
  GMQuery q(this,"SELECT coalesce(MAX(queue)+1,1) FROM playlist_tracks WHERE playlist == ?;");
  q.execute(playlist,queue);
  return queue;
  }


FXbool GMTrackDatabase::insertPlaylistTracks(FXint playlist,const FXIntList & tracks){
  DEBUG_DB_SET();

  GM_TICKS_START();
  FXint max=1;
  try {
    begin();

    GMQuery q(this,"SELECT coalesce(MAX(queue)+1,1) FROM playlist_tracks WHERE playlist == ?;");
    q.execute(playlist,max);

    for (FXint i=0;i<tracks.no();i++) {
      insert_playlist_track_by_id.set(0,playlist);
      insert_playlist_track_by_id.set(1,tracks[i]);
      insert_playlist_track_by_id.set(2,max++);
      insert_playlist_track_by_id.execute();
      }

    commit();
    }
  catch(GMDatabaseException&){
    rollback();
    return false;
    }
  GM_TICKS_END();
  return true;
  }

#if 0
/// Insert Track in Playlist
FXbool GMTrackDatabase::insertTrackInPlaylist(FXint playlist,FXint & track) {
  FXint queue=0;
  try {

    GMQuery queu_query("SELECT MAX(queue) FROM playlist_tracks WHERE playlist == ?;");
    queu_query.set(0,playlist);
    queu_query.execute_with_result(queue);

    // Increment
    queue++;

    GMQuery insert_query("INSERT INTO playlist_tracks VALUES ( ? , ? , ?);");
    insert_query.set(0,playlist);
    insert_query.set(1,track);
    insert_query.set(2,queue);
    insert_query.execute();


/*
    GMQuery insert_query("INSERT INTO playlist_tracks SELECT ?, ?, MAX(ifnull(queue,1))+1 FROM playlist_tracks WHERE playlist == ?;");
    insert_query.set(0,playlist);
    insert_query.set(1,track);
    insert_query.set(2,playlist);
    insert_query.execute();
  */

    }
  catch (GMDatabaseException & e){
    return false;
    }
  return true;
  }


/// Insert Track in Playlist
FXbool GMTrackDatabase::insertTrackInPlaylist(FXint playlist,const FXIntList & tracks) {
  FXint queue=0;

  query_playlist_queue.set(0,playlist);
  query_playlist_queue.execute_with_result(queue);

    // Increment
    queue++;

  GM_TICKS_START();

  begin();
  for (int i=0;i<tracks.no();i++){
    insert_track_playlist.set(0,playlist);
    insert_track_playlist.set(1,tracks[i]);
    insert_track_playlist.set(2,queue++);
    insert_track_playlist.execute();
    }
  commit();

  GM_TICKS_END();
  return true;
  }

FXbool GMTrackDatabase::clearQueue(){
  try {
    execute("DELETE FROM playqueue;");
    }
  catch (GMDatabaseException & e){
    return false;
    }
  return true;
  }

FXbool GMTrackDatabase::reorderQueue() {

  execute("CREATE TEMP TABLE neworder AS SELECT COUNT(b.queue) AS newq,a.queue AS oldq FROM playqueue a JOIN playqueue b ON a.queue >= b.queue GROUP BY a.queue ORDER BY a.queue ASC;");

  execute("UPDATE playqueue SET queue == (SELECT newq FROM neworder WHERE oldq == queue);");

  execute("DROP TABLE neworder;");
  return true;
  }



FXbool GMTrackDatabase::queueTracks(const FXIntList & tracks){
  FXint queue=1;
  try {
    GMQuery max_query("SELECT MAX(queue)+1 FROM playqueue;");
    max_query.execute_with_result(queue);

    queue=FXMAX(1,queue);

    GMQuery insert_query("INSERT INTO playqueue VALUES (?,?) ;");
    begin();
    for (int i=0;i<tracks.no();i++,queue++){
      insert_query.set(0,queue);
      insert_query.set(1,tracks[i]);
      insert_query.execute();
      }
    commit();
    }
  catch (GMDatabaseException & e){
    return false;
    }
  return true;
  }


FXbool GMTrackDatabase::removeQueueTracks(const FXIntList & queue){
  FXString query;

  begin();

  if (queue.no()==1) {

    query.format("DELETE FROM playqueue WHERE queue == %d;",queue[0]);
    execute(query);


    // Renumber queue following removed queue
    query.format("UPDATE playqueue SET queue = queue - 1 WHERE queue > %d;",queue[0]);
    execute(query);

    }
  else {

    query.format("DELETE FROM playqueue WHERE queue IN ( %d",queue[0]);
    for (FXint i=1;i<queue.no();i++){
      query+=",";
      query+=GMStringVal(queue[i]);
      }
    query+=");";

    execute(query);

    if (!reorderQueue())
      goto error;

    }
  commit();
  return true;
error:
  rollback();
  return false;
  }


#endif





///FIXME Insert Track in Playlist
FXbool GMTrackDatabase::updateTrackPlaylists(FXint playlist,FXIntList & tracks) {
  DEBUG_DB_SET();
  FXint queue=1;
  try {
    GMQuery update_queue(this,"UPDATE playlist_tracks SET queue = ? WHERE playlist == ? AND track == ?;");
    for (int i=0;i<tracks.no();i++){
      update_queue.set(0,queue++);
      update_queue.set(1,playlist);
      update_queue.set(2,tracks[i]);
      update_queue.execute();
      }
    }
  catch (GMDatabaseException & e){
    return false;
    }
  return true;
  }


/// List Playlists
FXbool GMTrackDatabase::listPlaylists(FXIntList & ids){
  DEBUG_DB_GET();
  FXint id=0;
  try {
    GMQuery query(this,"SELECT id FROM playlists WHERE name!='__buildin_playqueue__';");
    while(query.row()){
      query.get(0,id);
      ids.append(id);
      }
    }
  catch (GMDatabaseException & e){
    ids.clear();
    return false;
    }
  return true;
  }


FXbool GMTrackDatabase::setStreamFilename(FXint id,const FXString & filename){
  DEBUG_DB_SET();
  FXString query;
  query.format("UPDATE streams SET url = \"%s\" WHERE id==%d;",filename.text(),id);
  execute(query);
  return true;
  }

FXbool GMTrackDatabase::setStreamDescription(FXint id,const FXString & description){
  DEBUG_DB_SET();
  FXString query;
  query.format("UPDATE streams SET description = \"%s\" WHERE id==%d;",description.text(),id);
  execute(query);
  return true;
  }

FXbool GMTrackDatabase::setStreamBitrate(FXint id,FXint rate){
  DEBUG_DB_SET();
  FXString query;
  query.format("UPDATE streams SET bitrate = \"%d\" WHERE id==%d;",rate,id);
  execute(query);
  return true;
  }


FXbool GMTrackDatabase::setStreamGenre(FXint id,const FXString & name){
  DEBUG_DB_SET();
  FXint genreid=-1;
  FXString query;
  try {
    /// Query for existing genre
    query_genre.execute(name,genreid);
    if (genreid==-1) {
      genreid = insert_genre.insert(name);
      }
    query.format("UPDATE streams SET genre = %d WHERE id==%d;",genreid,id);
    execute(query);
    }
  catch (GMDatabaseException &){
    return false;
    }
  return true;
  }




FXbool GMTrackDatabase::removeStream(FXint track) {
  DEBUG_DB_SET();
  FXString query;
  query.format("DELETE FROM streams WHERE id = \"%d\"",track);
  execute(query);
  return true;
  }


void GMTrackDatabase::setTrackFilename(FXint id,const FXString & filename){
  DEBUG_DB_SET();
  FXString path = FXPath::directory(filename);
  FXint    pid  = 0;

  pid = hasPath(path);
  if (!pid)
    pid = insert_path.insert(path);

  update_track_filename.set(0,pid);
  update_track_filename.set(1,FXPath::name(filename));
  update_track_filename.set(2,id);
  update_track_filename.execute();
  }



/// Return filename for track
FXString GMTrackDatabase::getTrackFilename(FXint track) {
  DEBUG_DB_GET();
  FXString filename;
  try {
    query_track_filename.execute(track,filename);
    }
  catch(GMDatabaseException&){
    return FXString::null;
    }
  return filename;
  }

void GMTrackDatabase::getTrackFilenames(GMTrackFilenameList & result,const FXString & root) {
  DEBUG_DB_GET();
  GMQuery list;

  if (!root.empty())
    list = compile("SELECT tracks.id,pathlist.name || '" PATHSEPSTRING "' || mrl,importdate FROM tracks,pathlist WHERE tracks.path == pathlist.id AND pathlist.name LIKE '" + root + PATHSEPSTRING + "%' ORDER BY path;");
  else
    list = compile("SELECT tracks.id,pathlist.name || '" PATHSEPSTRING "' || mrl,importdate FROM tracks,pathlist WHERE tracks.path == pathlist.id ORDER BY path");

  while(list.row()){
    result.no(result.no()+1);
    list.get(0,result[result.no()-1].id);
    list.get(1,result[result.no()-1].filename);
    list.get(2,result[result.no()-1].date);
    }
  }

/// Return list of filenames
void GMTrackDatabase::getTrackFilenames(const FXIntList & tracks,FXStringList & filenames){
  DEBUG_DB_GET();
  filenames.no(tracks.no());
  for (FXint i=0;i<tracks.no();i++) {
    query_track_filename.set(0,tracks[i]);
    query_track_filename.execute(filenames[i]);
    }
  }


  /// Get the track stats
void GMTrackDatabase::getTrackStats(FXint & ntracks,FXint & nartists,FXint & nalbums,FXint & ntime,FXint playlist){
  DEBUG_DB_GET();
  try {
    GMQuery query;

    if (playlist) {
      query = compile("SELECT COUNT(id), COUNT(DISTINCT(artist)),COUNT(DISTINCT(album)),SUM(time) FROM tracks,playlist_tracks WHERE playlist_tracks.track == tracks.id AND playlist_tracks.playlist == ?;");
      query.set(0,playlist);
      }
    else {
      query = compile("SELECT COUNT(id), COUNT(DISTINCT(artist)),COUNT(DISTINCT(album)),SUM(time) FROM tracks;");
      }
    if (query.row()) {
      query.get(0,ntracks);
      query.get(1,nartists);
      query.get(2,nalbums);
      query.get(3,ntime);
      }
    }
  catch(GMDatabaseException&) {
    ntracks=0;
    nartists=0;
    nalbums=0;
    ntime=0;
    }
  }



FXint GMTrackDatabase::getNumTracks() {
  DEBUG_DB_GET();
  FXint total=0;
  execute("SELECT COUNT(*) FROM tracks;",total);
  return total;
  }

FXint GMTrackDatabase::getNumStreams() {
  DEBUG_DB_GET();
  FXint total=0;
  execute("SELECT COUNT(*) FROM streams;",total);
  return total;
  }

FXint GMTrackDatabase::getNumArtists() {
  DEBUG_DB_GET();
  FXint total=0;
  execute("SELECT COUNT(id) FROM artists;",total);
  return total;
  }

FXint GMTrackDatabase::getNumAlbums() {
  DEBUG_DB_GET();
  FXint total=0;
  execute("SELECT COUNT(id) FROM albums;",total);
  return total;
  }


FXint GMTrackDatabase::getTotalTime() {
  DEBUG_DB_GET();
  FXint total=0;
  execute("SELECT SUM(time) FROM tracks;",total);
  return total;
  }





FXint GMTrackDatabase::getPlayQueue() {
  DEBUG_DB_GET();
  FXint playqueue=0;
  try {
    GMQuery get_playqueue(this,"SELECT id FROM playlists WHERE name == '__buildin_playqueue__' ;");
    get_playqueue.execute(playqueue);
    if (!playqueue) {
      GMQuery create_playqueue(this,"INSERT INTO playlists VALUES(NULL,'__buildin_playqueue__');");
      playqueue = insert(create_playqueue);
      }
    }
  catch (GMDatabaseException & e){
    return 0;
    }
  return playqueue;
  }




FXbool GMTrackDatabase::trackInPlaylist(FXint track,FXint playlist) {
  DEBUG_DB_GET();
  FXint total;
  try {
    GMQuery query(this,"SELECT COUNT(queue) FROM playlist_tracks WHERE track == ? AND playlist == ? ");
    query.set(0,track);
    query.set(1,playlist);
    query.execute(total);
    }
  catch (GMDatabaseException & e){
    return false;
    }
  return (total>0);
  }


/// List Album Paths
FXbool GMTrackDatabase::listAlbumPaths(GMAlbumPathList & list){
  DEBUG_DB_GET();
  FXint n=0;
  try {
    FXint num_albums = getNumAlbums();
    if (num_albums>0) {
      list.no(num_albums);
      GMQuery query(this,"SELECT album,pathlist.name || '" PATHSEPSTRING "' || mrl FROM tracks,pathlist WHERE tracks.path == pathlist.id GROUP BY album;" );
      while(query.row()) {
        query.get(0,list[n].id);
        list[n].path = query.get(1);
        n++;
        }
      }
    }
  catch (GMDatabaseException & e){
    return false;
    }
  return true;
  }



FXbool GMTrackDatabase::getAlbumTrack(FXint id,FXString & path) {
  DEBUG_DB_GET();
  execute("SELECT pathlist.name || '" PATHSEPSTRING "' || mrl FROM tracks,pathlist WHERE tracks.path == pathlist.id AND album == ? ORDER BY no LIMIT(1);",id,path);
  return true;
  }

/// Return the track path;
const FXchar * GMTrackDatabase::getTrackPath(FXint pid) const {
  const void * ptr = pathdict.find((void*)(FXival)pid);
  if (ptr) return (const FXchar*)ptr;
  return "";
  }

/// Return artist;
const FXString * GMTrackDatabase::getArtist(FXint aid) const {
  const void * ptr = artistdict.find((void*)(FXival)aid);
  if (ptr) return (const FXString*)ptr;
  return &empty;
  }


void GMTrackDatabase::getTrackPath(FXint id,FXString & path) {
  DEBUG_DB_GET();
  static FXint last=-1;
  static FXString last_path;
  GM_TICKS_START();
  if (last==id) {
    fxmessage("same\n");
    path=last_path;
    GM_TICKS_END();
    return;
    }
  last=id;
  query_path_name.execute(last,last_path);
  path=last_path;
  GM_TICKS_END();
  }




FXbool GMTrackDatabase::getStream(FXint id,GMStream & stream){
  DEBUG_DB_GET();
  FXString query;
  query.format("SELECT url,description,tags.name,bitrate FROM streams, tags WHERE tags.id == streams.genre AND streams.id == %d;",id);
  GMQuery get_stream;
  try {
    get_stream = compile(query);
    if (get_stream.row()){
    stream.url=get_stream.get(0);
    stream.description=get_stream.get(1);
    stream.tag=get_stream.get(2);
    get_stream.get(3,stream.bitrate);
    }
    }
  catch (GMDatabaseException &){
    return false;
    }
  return true;
  }



FXbool GMTrackDatabase::getTrack(FXint tid,GMTrack & track){
  DEBUG_DB_GET();
  FXbool ok=false;
  try {
    begin();
    query_track.set(0,tid);
    if (query_track.row()) {
      query_track.get( 0,track.mrl);
      query_track.get( 1,track.album);
      query_track.get( 2,track.album_artist);
      query_track.get( 3,track.artist);
      query_track.get( 4,track.composer);
      query_track.get( 5,track.conductor);
      query_track.get( 6,track.title);
      query_track.get( 7,track.time);
      query_track.get( 8,track.no);
      query_track.get( 9,track.year);
      query_track.get(10,track.track_gain);
      query_track.get(11,track.track_peak);
      query_track.get(12,track.album_gain);
      query_track.get(13,track.album_peak);
      query_track.get(14,track.rating);
      ok=true;
      }
    query_track.reset();

    /// Get All tags
    query_track_tags.set(0,tid);
    while(query_track_tags.row()){
      track.tags.append(query_track_tags.get(0));
      }
    query_track_tags.reset();
    commit();
    }
  catch (GMDatabaseException & e){
    rollback();
    return false;
    }
  return ok;
  }

FXbool GMTrackDatabase::getTracks(const FXIntList & tids,GMTrackArray & tracks){
  DEBUG_DB_GET();
  try {
    tracks.no(tids.no());
    for (FXint i=0;i<tids.no();i++) {
      query_track.set(0,tids[i]);
      if (query_track.row()) {
        query_track.get( 0,tracks[i].mrl);
        query_track.get( 1,tracks[i].album);
        query_track.get( 2,tracks[i].album_artist);
        query_track.get( 3,tracks[i].artist);
        query_track.get( 4,tracks[i].composer);
        query_track.get( 5,tracks[i].conductor);
        query_track.get( 6,tracks[i].title);
        query_track.get( 7,tracks[i].time);
        query_track.get( 8,tracks[i].no);
        query_track.get( 9,tracks[i].year);
        query_track.get(10,tracks[i].track_gain);
        query_track.get(11,tracks[i].track_peak);
        query_track.get(12,tracks[i].album_gain);
        query_track.get(13,tracks[i].album_peak);
        query_track.get(14,tracks[i].rating);
        }
      query_track.reset();

      /// Get All tags
      query_track_tags.set(0,tids[i]);
      while(query_track_tags.row()){
        tracks[i].tags.append(query_track_tags.get(0));
        }
      query_track_tags.reset();
      }
    }
  catch (GMDatabaseException & e){
    return false;
    }
  return true;
  }


FXbool GMTrackDatabase::getTrackAssociation(FXint id,FXint & artist,FXint & album){
  DEBUG_DB_GET();
  try {
    query_album_artists.set(0,id);
    if (query_album_artists.row()) {
      query_album_artists.get(0,artist);
      query_album_artists.get(1,album);
      query_album_artists.reset();
      return true;
      }
    }
  catch (GMDatabaseException & e){
    return false;
    }
  return false;
  }



///FIXME
FXbool GMTrackDatabase::removeGenre(FXint id) {
  DEBUG_DB_SET();
  GMQuery remove_genre;
  try {
    begin();

/*
    remove_genre = compile("DELETE FROM tracks WHERE genre == ?;");
    remove_genre.execute_simple(id);

*/
    commit();
    }
  catch (GMDatabaseException & e){
    rollback();
    return false;
    }
  return true;
  }



///FIXME
FXbool GMTrackDatabase::removeArtist(FXint artist) {
  DEBUG_DB_SET();

  GMQuery query;
  try {

    begin();
/*
    /// Remove tracks from playlist
    query = compile("DELETE FROM playlist_tracks WHERE track IN (SELECT id FROM tracks WHERE artist == ? OR album IN ( SELECT id FROM albums WHERE artist == ?));");
    query.set(0,artist);
    query.set(1,artist);
    query.execute();

    /// Removes tracks with artist or with album from artist
    query = compile("DELETE FROM tracks WHERE artist == ?  OR album IN ( SELECT id FROM albums WHERE artist == ?);");
    query.set(0,artist);
    query.set(1,artist);
    query.execute();

    /// Remove albums with artist
    query = compile("DELETE FROM albums WHERE artist == ?;");
    query.execute_simple(artist);

    /// Remove artist itself
    query = compile("DELETE FROM artists WHERE id == ?;");
    query.execute_simple(artist);

    /// Cleanup genres and pathlist
    execute("DELETE FROM genres WHERE id NOT IN (SELECT genre FROM tracks UNION SELECT genre FROM streams);");
    execute("DELETE FROM pathlist WHERE id NOT IN (SELECT DISTINCT(path) FROM tracks);");
*/
    commit();
    }
  catch (GMDatabaseException & e){
    rollback();
    return false;
    }
  return true;
  }

FXbool GMTrackDatabase::removeAlbum(FXint album) {
  DEBUG_DB_SET();

  GMQuery query;
  try {
    begin();

    /// Remove tracks from playlist
    query = compile("DELETE FROM playlist_tracks WHERE track IN (SELECT id FROM tracks WHERE album == ?);");
    query.update(album);

    /// Removes tracks with album
    query = compile("DELETE FROM tracks WHERE album == ?;");
    query.update(album);

    /// Remove album itself
    query = compile("DELETE FROM albums WHERE id == ?;");
    query.update(album);

    /// Cleanup
    execute("DELETE FROM artists WHERE id NOT IN (SELECT artist FROM albums UNION SELECT artist FROM tracks UNION SELECT composer FROM tracks UNION SELECT conductor FROM tracks);");
    execute("DELETE FROM tags WHERE id NOT IN (SELECT tag FROM track_tags UNION SELECT genre FROM streams);");
    execute("DELETE FROM track_tags WHERE id NOT IN (SELECT id FROM tags);");
    execute("DELETE FROM pathlist WHERE id NOT IN (SELECT DISTINCT(path) FROM tracks);");

    commit();
    }
  catch (GMDatabaseException & e){
    rollback();
    return false;
    }
  return true;
  }






















/// For each playlist, we reset the queue number from 1 to max tracks.
FXbool GMTrackDatabase::reorderPlaylists(){
  DEBUG_DB_SET();

  GMQuery q;
  GM_TICKS_START();
  try {
    FXIntList playlists;
    FXIntList tracks;
    FXint num_tracks=0;
    FXint i=0,j=0;

    /// Count Rows
    GM_DEBUG_PRINT("counting records\n");


    q = compile("SELECT COUNT(*) FROM playlist_tracks;");
    q.execute(num_tracks);

    playlists.no(num_tracks);
    tracks.no(num_tracks);

    /// Get All Rows in proper order
    GM_DEBUG_PRINT("getting records\n");

    q = compile("SELECT playlist,track FROM playlist_tracks ORDER BY playlist,queue;");
    while(q.row()) {
      q.get(0,playlists[j]);
      q.get(1,tracks[j]);
      j++;
      }

    /// Delete Contents
    GM_DEBUG_PRINT("deleting records\n");
    execute("DELETE FROM playlist_tracks;");

    /// Reinsert playlists
    GM_DEBUG_PRINT("inserting records\n");

    q = compile("INSERT INTO playlist_tracks VALUES ( ?, ? ,?);");
    for (i=0,j=1;i<num_tracks;i++,j++) {
      if (i>0 && playlists[i-1]!=playlists[i]) j=1;
      q.set(0,playlists[i]);
      q.set(1,tracks[i]);
      q.set(2,j);
      q.execute();
      }
    }
  catch (GMDatabaseException & e){
    return false;
    }
  GM_TICKS_END();
  return true;
  }

/// For each playlist, we reset the queue number from 1 to max tracks.
FXbool GMTrackDatabase::reorderPlaylist(FXint pl){
  DEBUG_DB_SET();

  GMQuery q;
  GM_TICKS_START();
  try {
    FXIntList tracks;
    FXint num_tracks=0;
    FXint i=0,j=0;

    /// Count Rows
    GM_DEBUG_PRINT("counting records\n");

    q = compile("SELECT COUNT(*) FROM playlist_tracks WHERE playlist == ?;");
    q.execute(pl,num_tracks);

    tracks.no(num_tracks);

    /// Get All Rows in proper order
    GM_DEBUG_PRINT("getting records\n");

    q = compile("SELECT track FROM playlist_tracks WHERE playlist == ? ORDER BY queue;");
    q.set(0,pl);
    while(q.row()) {
      q.get(0,tracks[j]);
      j++;
      }
    q.reset();

    /// Delete Contents
    GM_DEBUG_PRINT("deleting records\n");
    q = compile("DELETE FROM playlist_tracks WHERE playlist == ?;");
    q.update(pl);

    /// Reinsert playlists
    GM_DEBUG_PRINT("inserting records\n");

    q = compile("INSERT INTO playlist_tracks VALUES ( ?, ? ,?);");
    for (i=0,j=1;i<num_tracks;i++,j++) {
      q.set(0,pl);
      q.set(1,tracks[i]);
      q.set(2,j);
      q.execute();
      }
    }
  catch (GMDatabaseException & e){
    return false;
    }
  GM_TICKS_END();
  return true;
  }




/// Update Playlist
FXbool GMTrackDatabase::updatePlaylist(FXint playlist,const GMPlayListItemList & items) {
  DEBUG_DB_SET();

  GMQuery q;
  try {
    begin();

    q = compile("DELETE FROM playlist_tracks WHERE playlist == ?;");
    q.update(playlist);

    for (FXint i=0;i<items.no();i++){
      insert_playlist_track_by_id.set(0,playlist);
      insert_playlist_track_by_id.set(1,items[i].track);
      insert_playlist_track_by_id.set(2,items[i].queue);
      insert_playlist_track_by_id.execute();
      }

    commit();
    }
  catch (GMDatabaseException & e){
    rollback();
    return false;
    }
  return true;
  }



FXbool GMTrackDatabase::removePlaylist(FXint playlist){
  DEBUG_DB_SET();

  GMQuery q;
  try {
    begin();

    q = compile("DELETE FROM playlists WHERE id == ?;");
    q.update(playlist);

    q = compile("DELETE FROM playlist_tracks WHERE playlist == ?;");
    q.update(playlist);

    commit();
    }
  catch (GMDatabaseException & e){
    rollback();
    return false;
    }
  return true;
  }



/// Get Playlist Name
void GMTrackDatabase::getPlaylistName(FXint playlist,FXString & name) {
  DEBUG_DB_GET();
  execute("SELECT name FROM playlists WHERE id == ?;",playlist,name);
  }

/// Set Playlist Name
FXbool GMTrackDatabase::setPlaylistName(FXint playlist,const FXString & name) {
  DEBUG_DB_SET();
  try {
    GMQuery query(this,"UPDATE playlists SET name = ? WHERE id == ?;");
    query.set(0,name);
    query.set(1,playlist);
    query.execute();
    }
  catch (GMDatabaseException & e){
    return false;
    }
  return true;
  }


#if 0
/// Move Track in playlist
FXbool GMTrackDatabase::moveTrack(FXint playlist,FXint oldq,FXint newq){
  FXString query;
  FXint row=0;
  if (oldq==newq) return true;

  query = "SELECT ROWID FROM playlist_tracks WHERE playlist == " + GMStringVal(playlist) + " AND queue == " + GMStringVal(oldq) + ";";
  execute_simple(query.text(),row);

  if (oldq<newq)
    query = "UPDATE playlist_tracks SET queue = queue - 1 WHERE playlist == "+ GMStringVal(playlist) +" AND queue > " +GMStringVal(oldq) + " AND queue <= " + GMStringVal(newq) + ";";
  else
    query = "UPDATE playlist_tracks SET queue = queue + 1 WHERE playlist == "+ GMStringVal(playlist) +" AND queue < " +GMStringVal(oldq) + " AND queue >= " + GMStringVal(newq) + ";";

  execute(query);

  query = "UPDATE playlist_tracks SET queue = " + GMStringVal(newq) + " WHERE playlist == " + GMStringVal(playlist) + " AND ROWID == " + GMStringVal(row) + ";";
  execute(query);

  return true;
  }
#endif

#if 0
/// Move Track in playlist
FXbool GMTrackDatabase::moveQueueTrack(FXint oldq,FXint newq){
  FXString query;
  if (oldq==newq) return true;

  query = "UPDATE playqueue SET queue = 0 WHERE queue == " +GMStringVal(oldq) + ";";
  execute(query);

  if (oldq<newq)
    query = "UPDATE playqueue SET queue = queue - 1 WHERE queue > " +GMStringVal(oldq) + " AND queue <= " + GMStringVal(newq) + ";";
  else
    query = "UPDATE playqueue SET queue = queue + 1 WHERE queue < " +GMStringVal(oldq) + " AND queue >= " + GMStringVal(newq) + ";";

  execute(query);

  query = "UPDATE playqueue SET queue = " + GMStringVal(newq) + " WHERE queue == 0;";
  execute(query);

  return true;
  }

#endif






FXbool GMTrackDatabase::listTags(FXComboBox * list,FXbool insert_default){
  DEBUG_DB_GET();
  register int i=0;
  FXDict tags;
  FXString name;
  FXint id;
  try {
    if (insert_default) {
      for (i=0;!TagLib::ID3v1::genre(i).isNull();i++) {
        tags.insert(TagLib::ID3v1::genre(i).toCString(true),(void*)(FXival)1);
        list->appendItem(TagLib::ID3v1::genre(i).toCString(true));
        }
      while(list_tags.row()) {
        list_tags.get(0,id);
        list_tags.get(1,name);
        if (!tags.find(name.text()))
          list->appendItem(name);
        }
      }
    else {
      while(list_tags.row()) {
        list_tags.get(0,id);
        list_tags.get(1,name);
        list->appendItem(name);
        }
      }
    list->sortItems();
    }
  catch(GMDatabaseException & e){
    list->clearItems();
    return false;
    }
  return true;
  }




FXbool GMTrackDatabase::listArtists(FXComboBox * list){
  DEBUG_DB_GET();
  FXString name;
  FXint id;
  try {
    while(list_artists.row()){
      list_artists.get(0,id);
      list_artists.get(1,name);
      list->appendItem(name);
      }
    list->sortItems();
    }
  catch(GMDatabaseException & e){
    list->clearItems();
    return FALSE;
    }
  return true;
  }

FXbool GMTrackDatabase::listAlbums(FXComboBox * list,FXint track){
  DEBUG_DB_GET();
  list_albums_with_artist_from_track.set(0,track);
  while(list_albums_with_artist_from_track.row()) {
    list->appendItem(list_albums_with_artist_from_track.get(0));
    }
  return true;
  }





void GMTrackDatabase::clear_path_lookup() {
  DEBUG_DB_GET();
  for (FXint i=0;i<pathdict.size();i++) {
    if (!pathdict.empty(i) && pathdict.value(i)!=NULL) {
      free(pathdict.value(i));
      }
    }
  pathdict.clear();
  }


void GMTrackDatabase::setup_path_lookup() {
  DEBUG_DB_GET();
  GM_TICKS_START();
  GMQuery q;
  FXint path;
  try {
    q = compile("SELECT id,name FROM pathlist;");
    while(q.row()) {
      q.get(0,path);
      pathdict.insert((void*)(FXival)path,strdup(q.get(1)));
      }
    }
  catch(GMDatabaseException & e){
    }
  GM_TICKS_END();
  }

void GMTrackDatabase::setup_artist_lookup() {
  DEBUG_DB_GET();
  GM_TICKS_START();
  GMQuery q;
  FXint id;
  try {
    q = compile("SELECT id,name FROM artists;");
    while(q.row()) {
      q.get(0,id);
      artistdict.insert((void*)(FXival)id,new FXString(q.get(1)));
      }
    }
  catch(GMDatabaseException & e){
    }
  GM_TICKS_END();
  }

void GMTrackDatabase::clear_artist_lookup() {
  DEBUG_DB_GET();
  for (FXint i=0;i<artistdict.size();i++) {
    if (!artistdict.empty(i) && artistdict.value(i)!=NULL) {
      FXString * a = (FXString*)artistdict.value(i);
      delete a;
      }
    }
  artistdict.clear();
  }



FXbool GMTrackDatabase::queryGenre(FXint & result,const FXString & name,FXbool insert){
  DEBUG_DB_SET();
  result=-1;
  try {
    query_genre.execute(name,result);
    if (result==-1 && insert) {
      insert_genre.set(0,name);
      insert_genre.execute();
      result = rowid();
      insert_genre.reset();
      }
    }
  catch (GMDatabaseException & e){
    result=-1;
    return FALSE;
    }
  return true;
  }



FXbool GMTrackDatabase::updateAlbum(FXint & result,const GMTrack & track,FXint artist){
  DEBUG_DB_SET();
  result=0;
  update_album = compile("UPDATE albums SET year = ?, replay_gain = ?, replay_peak = ? WHERE id == ?;");
  try {
    query_album.set(0,track.album);
    query_album.set(1,artist);
    query_album.execute(result);

    if (!result) {
      insert_album.set(0,track.album);
      insert_album.set(1,artist);
      insert_album.set(2,track.year);
      insert_album.set(3,track.album_gain);
      insert_album.set(4,track.album_peak);
      insert_album.execute();
      result = rowid();
      }
    else {
      update_album.set(0,track.year);
      update_album.set(1,track.album_gain);
      update_album.set(2,track.album_peak);
      update_album.set(3,result);
      update_album.execute();
      }
    }
  catch (GMDatabaseException & e){
    result=-1;
    return FALSE;
    }
  return true;
  }




FXbool GMTrackDatabase::vacuum() {
  DEBUG_DB_SET();

  GM_TICKS_START();

  begin();

  /// Remove empty playlists ? FIXME: may be not...
  /// if (!execute("DELETE FROM playlists WHERE id NOT IN (SELECT DISTINCT(playlist) FROM playlist_tracks);"))
  //  return false;

  GM_DEBUG_PRINT("Reorder Playlists\n");

  /// Reorder Playlists
  if (!reorderPlaylists()) goto error;

  GM_DEBUG_PRINT("Remove Empty Albums\n");

  /// Remove empty albums
  execute("DELETE FROM albums WHERE id NOT IN (SELECT DISTINCT(album) FROM tracks);");

  GM_DEBUG_PRINT("Remove unused artists\n");

  /// Remove unused artists
  execute("DELETE FROM artists WHERE id NOT IN (SELECT artist FROM albums UNION SELECT artist FROM tracks UNION SELECT composer FROM tracks UNION SELECT conductor FROM tracks);");

  GM_DEBUG_PRINT("Remove unused genres\n");

  /// Remove unused tags
  execute("DELETE FROM tags WHERE id NOT IN (SELECT tag FROM track_tags UNION SELECT genre FROM streams);");

  GM_DEBUG_PRINT("Remove unused paths\n");

  /// Remove unused paths
  execute("DELETE FROM pathlist WHERE id NOT IN (SELECT DISTINCT(path) FROM tracks);");

  /// commit changes
  commit();

  /// Reinitialize path lookup
  clear_path_lookup();
  clear_artist_lookup();
  setup_path_lookup();
  setup_artist_lookup();

  GM_TICKS_END();
  return true;
error:
  rollback();
  return false;
  }


FXbool GMTrackDatabase::exportList(const FXString & filename,FXint playlist,FXuint filetype,FXuint opts){
  DEBUG_DB_GET();
  const FXchar * c_artist;
  const FXchar * c_album;
  const FXchar * c_title;
  FXint time;
  FXint no;
  FXint cnt=1;
  FXint year;
  FILE * fp = NULL;
  FXString query;
  FXString title;
  FXString file;
  FXString basepath=FXPath::directory(filename);
  GMQuery list;
  try {

    if (playlist)
      getPlaylistName(playlist,title);

    query = "SELECT pathlist.name || '" PATHSEPSTRING "' || mrl,title,artists.name,albums.name,no,time,tracks.year "
            "FROM tracks,pathlist, albums, artists";

    if (playlist)
      query+=", playlist_tracks";

    query+=" WHERE "
                  "albums.artist == artists.id AND "
                  "tracks.album == albums.id AND "
                  "pathlist.id == tracks.path ";

    if (playlist) {
      query+=" AND playlist_tracks.track == tracks.id";
      query+=" AND playlist_tracks.playlist == "+GMStringVal(playlist);
      query+=" ORDER BY playlist_tracks.queue; ";
      }
    else {
      query+=";";
      }

    list = compile(query);

    FILE * fp = fopen(filename.text(),"w");
    if (!fp) return false;

    if (filetype==PLAYLIST_XSPF) {
      fprintf(fp,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
      fprintf(fp,"<playlist version=\"1\" xmlns=\"http://xspf.org/ns/0/\">\n");
      if (!title.empty()) fprintf(fp,"\t<title>%s</title>\n",title.text());
      fprintf(fp,"\t<trackList>\n");
      }
    else if (filetype==PLAYLIST_M3U_EXTENDED) {
      fprintf(fp,"#EXTM3U\n");
      }
    else if (filetype==PLAYLIST_PLS) {
      fprintf(fp,"[playlist]\n");
      }
    else if (filetype==PLAYLIST_CSV) {
      fprintf(fp,"No,Title,Album,Artist,Genre,Duration,Year,Filename\n");
      }

    while(list.row()){
      file  = list.get(0);
      c_title = list.get(1);
      c_artist = list.get(2);
      c_album = list.get(3);
      list.get(4,no);
      list.get(5,time);
      list.get(6,year);

      if (opts&PLAYLIST_OPTIONS_RELATIVE)
        file = FXPath::relative(basepath,file);

      if (filetype==PLAYLIST_XSPF) {
        fprintf(fp,"\t\t<track>\n");
        fprintf(fp,"\t\t\t<location>%s</location>\n",GMURL::fileToURL(gm_url_encode(file)).text());
        fprintf(fp,"\t\t\t<creator>%s</creator>\n",c_artist);
        fprintf(fp,"\t\t\t<album>%s</album>\n",c_album);
        fprintf(fp,"\t\t\t<title>%s</title>\n",c_title);
        fprintf(fp,"\t\t\t<duration>%d</duration>\n",time*1000);
        fprintf(fp,"\t\t\t<trackNum>%u</trackNum>\n",no);
        fprintf(fp,"\t\t</track>\n");
        }
      else if (filetype==PLAYLIST_M3U_EXTENDED) {
        fprintf(fp,"#EXTINF:%d,%s - %s\n",time,c_artist,c_title);
        fprintf(fp,"%s\n",file.text());
        }
      else if (filetype==PLAYLIST_M3U) {
        fprintf(fp,"%s\n",file.text());
        }
      else if (filetype==PLAYLIST_PLS) {
        fprintf(fp,"File%d=%s\n",cnt,file.text());
        fprintf(fp,"Title%d=%s - %s\n",cnt,c_artist,c_title);
        fprintf(fp,"Length%d=%d\n",cnt,time);
        cnt++;
        }
      else if (filetype==PLAYLIST_CSV){
        fprintf(fp,"%d,\"%s\",\"%s\",\"%s\",%d,%d,\"%s\"\n",no,c_title,c_album,c_artist,time,year,file.text());
        }
      }
    if (filetype==PLAYLIST_XSPF) {
      fprintf(fp,"\t</trackList>\n");
      fprintf(fp,"</playlist>");
      }
    else if (filetype==PLAYLIST_PLS){
      fprintf(fp,"Version=2\n");
      fprintf(fp,"NumberOfEntries=%d",cnt-1);
      }
    fclose(fp);
    fp=NULL;
    }
  catch (GMDatabaseException & e){
    if (fp) fclose(fp);
    return false;
    }
  return true;
  }





















/*********************************************/













void GMTrackDatabase::setTrackImported(FXint track,FXlong tm){
  DEBUG_DB_SET();
  begin();
  update_track_importdate.set(0,tm);
  update_track_importdate.set(1,track);
  update_track_importdate.execute();
  commit();
  }

void GMTrackDatabase::setTrackRating(FXint id,FXuchar rating){
  DEBUG_DB_SET();
  begin();
  update_track_rating.set(0,(FXuint)rating);
  update_track_rating.set(1,id);
  update_track_rating.execute();
  commit();
  }

void GMTrackDatabase::setTrackPlayed(FXint track,FXlong time) {
  DEBUG_DB_SET();
  begin();
  update_track_playcount.set(0,time);
  update_track_playcount.set(1,track);
  update_track_playcount.execute();
  commit();
  }



void GMTrackDatabase::setTrackDiscNumber(const FXIntList & tracks,FXushort disc){
  DEBUG_DB_SET();
  GMQuery update_track_discnumber(this,"UPDATE tracks SET no = ((no&65535)|(?<<16)) WHERE id == ?;");
  for (FXint i=0;i<tracks.no();i++) {
    update_track_discnumber.set(0,disc);
    update_track_discnumber.set(1,tracks[i]);
    update_track_discnumber.execute();
    }
  }

//  update_track_disc.compile(db,"UPDATE tracks SET no = ((no&65535)|(?<<16)) WHERE id == ?;");
//  update_track_track.compile(db,"UPDATE tracks SET no = ((no&1048560)|?) WHERE id == ?;");

void GMTrackDatabase::setTrackTrackNumber(const FXIntList & tracks,FXushort track,FXbool auto_increment){
  DEBUG_DB_SET();
  GMQuery update_track_tracknumber(this,"UPDATE tracks SET no = ((no&(65535<<16))|?) WHERE id == ?;");
  FXint n = auto_increment ? 1 : 0;
  for (FXint i=0;i<tracks.no();i++,track+=n) {
    update_track_tracknumber.set(0,track);
    update_track_tracknumber.set(1,tracks[i]);
    update_track_tracknumber.execute();
    }
  }


void GMTrackDatabase::setTrackNumber(const FXIntList & tracks,FXuint disc,FXuint track,FXbool auto_increment){
  DEBUG_DB_SET();
  GMQuery update_track_number(this,"UPDATE tracks SET no = ? WHERE id == ?;");
  FXint n = auto_increment ? 1 : 0;
  for (FXint i=0;i<tracks.no();i++,track+=n) {
    update_track_number.set(0,GMALBUMNO(disc,track));
    update_track_number.set(1,tracks[i]);
    update_track_number.execute();
    }
  }



/// Set the name of a track
void GMTrackDatabase::setTrackTitle(FXint track,const FXString & name){
  DEBUG_DB_SET();
  GMQuery update_track_title(this,"UPDATE tracks SET title = ? WHERE id == ?;");
  update_track_title.set(0,name);
  update_track_title.set(1,track);
  update_track_title.execute();
  }

void GMTrackDatabase::setTrackYear(const FXIntList & tracks,FXuint year){
  DEBUG_DB_SET();
  GMQuery update_track_year(this,"UPDATE tracks SET year = ? WHERE id == ?;");
  for (FXint i=0;i<tracks.no();i++) {
    update_track_year.set(0,year);
    update_track_year.set(1,tracks[i]);
    update_track_year.execute();
    }
  }

void GMTrackDatabase::setTrackGenre(const FXIntList & tracks,const FXString & name){
  DEBUG_DB_SET();
  GMQuery update_track_genre(this,"UPDATE tracks SET genre == ? WHERE id == ?;");

  /// Make sure genre exists
  FXint genre=0;
  insert_genre.update(name);
  query_genre.execute(name,genre);

  FXASSERT(genre);

  /// Set Genre
  for (FXint i=0;i<tracks.no();i++) {
    update_track_genre.set(0,genre);
    update_track_genre.set(1,tracks[i]);
    update_track_genre.execute();
    }
  }

/// Set Track Artist
void GMTrackDatabase::setTrackArtist(const FXIntList & tracks,const FXString & name){
  DEBUG_DB_SET();

  GMQuery update_track_artist(this,"UPDATE tracks SET artist == ? WHERE id == ?;");

  /// Make sure artist exists
  FXint artist=0;
  insert_artist.update(name);
  query_artist.execute(name,artist);

  FXASSERT(artist);

  for (FXint i=0;i<tracks.no();i++) {
    update_track_artist.set(0,artist);
    update_track_artist.set(1,tracks[i]);
    update_track_artist.execute();
    }
  }


/// Set Track Composer
void GMTrackDatabase::setTrackComposer(const FXIntList & tracks,const FXString & name){
  DEBUG_DB_SET();

  GMQuery update_track_composer(this,"UPDATE tracks SET composer == ? WHERE id == ?;");

  /// Make sure artist exists
  FXint artist=0;
  insert_artist.update(name);
  query_artist.execute(name,artist);

  FXASSERT(artist);

  for (FXint i=0;i<tracks.no();i++) {
    update_track_composer.set(0,artist);
    update_track_composer.set(1,tracks[i]);
    update_track_composer.execute();
    }
  }

/// Set Track Composer
void GMTrackDatabase::setTrackConductor(const FXIntList & tracks,const FXString & name){
  DEBUG_DB_SET();

  GMQuery update_track_conductor(this,"UPDATE tracks SET conductor == ? WHERE id == ?;");

  /// Make sure artist exists
  FXint artist=0;
  insert_artist.update(name);
  query_artist.execute(name,artist);

  FXASSERT(artist);

  for (FXint i=0;i<tracks.no();i++) {
    update_track_conductor.set(0,artist);
    update_track_conductor.set(1,tracks[i]);
    update_track_conductor.execute();
    }
  }





/// Set Track Album
void GMTrackDatabase::setTrackAlbum(const FXIntList & tracks,const FXString & name,FXbool sameartist){
  DEBUG_DB_SET();

  GMQuery query_album_by_same_artist(this,"SELECT a2.id FROM albums AS a1 JOIN tracks ON tracks.album == a1.id JOIN albums AS a2 ON a1.artist==a2.artist AND a1.id!=a2.id WHERE tracks.id == ? AND a2.name == ?;");
  GMQuery copy_album(this,"INSERT INTO albums (name,artist,year,replay_gain,replay_peak) "
                          "SELECT ?,"
                                "artist,"
                                "year,"
                                "replay_gain,"
                                "replay_peak "
                          "FROM albums "
                          "WHERE id = (SELECT album FROM tracks WHERE id == ?);");
  GMQuery update_track_album(this,"UPDATE tracks SET album = ? WHERE id == ?;");

  FXint album=0;
  if (sameartist) {
    query_album_by_same_artist.set(0,tracks[0]);
    query_album_by_same_artist.set(1,name);
    query_album_by_same_artist.execute(album);

    if (!album) {
      copy_album.set(0,name);
      copy_album.set(1,tracks[0]);
      copy_album.execute();

      query_album_by_same_artist.set(0,tracks[0]);
      query_album_by_same_artist.set(1,name);
      query_album_by_same_artist.execute(album);

      FXASSERT(album);
      if (!album) throw GMDatabaseException();
      }
    for (FXint i=0;i<tracks.no();i++) {
      update_track_album.set(0,album);
      update_track_album.set(1,tracks[i]);
      update_track_album.execute();
      }
    }
  else {
    for (FXint i=0;i<tracks.no();i++) {
      album=0;
      query_album_by_same_artist.set(0,tracks[i]);
      query_album_by_same_artist.set(1,name);
      query_album_by_same_artist.execute(album);
      if (!album) {
        copy_album.set(0,name);
        copy_album.set(1,tracks[i]);
        copy_album.execute();
        query_album_by_same_artist.set(0,tracks[i]);
        query_album_by_same_artist.set(1,name);
        query_album_by_same_artist.execute(album);
        FXASSERT(album);
        }
      update_track_album.set(0,album);
      update_track_album.set(1,tracks[i]);
      update_track_album.execute();
      }
    }
  }



void GMTrackDatabase::setTrackAlbumArtist(const FXIntList & tracks,const FXString & name,const FXString & title){
  DEBUG_DB_SET();

  FXint album=0;

  GMQuery update_track_album(this,"UPDATE tracks SET album = ? WHERE id == ?;");
  GMQuery query_album(this,"SELECT id FROM albums WHERE name == ? AND artist == (SELECT id FROM artists WHERE name == ?);");

  /// Don't do "a1.id!=a2.id" since the album entry may already exists.
  GMQuery query_album_by_same_name(this,"SELECT a2.id FROM albums AS a1 JOIN tracks ON tracks.album == a1.id JOIN albums AS a2 ON a1.name == a2.name JOIN artists ON a2.artist == artists.id WHERE tracks.id == ? AND artists.name == ?;");
  GMQuery copy_album(this,"INSERT INTO albums (name,artist,year,replay_gain,replay_peak)"
                          "SELECT name,"
                                "(SELECT id FROM artists WHERE name == ?),"
                                "albums.year,"
                                "albums.replay_gain,"
                                "albums.replay_peak "
                          "FROM albums JOIN tracks ON albums.id == tracks.album "
                          "WHERE tracks.id = ?;");

  GMQuery copy_album_with_title(this,"INSERT INTO albums (name,artist,year,replay_gain,replay_peak)"
                          "SELECT ?,"
                                "(SELECT id FROM artists WHERE name == ?),"
                                "albums.year,"
                                "albums.replay_gain,"
                                "albums.replay_peak "
                          "FROM albums JOIN tracks ON albums.id == tracks.album "
                          "WHERE tracks.id = ?;");

  /// Make sure we have an artist.
  insert_artist.update(name);

  if (title.empty()) {

    for (FXint i=0;i<tracks.no();i++){
      album=0;
      query_album_by_same_name.set(0,tracks[i]);
      query_album_by_same_name.set(1,name);
      query_album_by_same_name.execute(album);

      if (!album) {
        copy_album.set(0,name);
        copy_album.set(1,tracks[i]);
        copy_album.execute();

        query_album_by_same_name.set(0,tracks[i]);
        query_album_by_same_name.set(1,name);
        query_album_by_same_name.execute(album);
        }
      FXASSERT(album);
      update_track_album.set(0,album);
      update_track_album.set(1,tracks[i]);
      update_track_album.execute();
      }
    }
  else {
    copy_album_with_title.set(0,title);
    copy_album_with_title.set(1,name);
    copy_album_with_title.set(2,tracks[0]);
    copy_album_with_title.execute();

    query_album.set(0,title);
    query_album.set(1,name);
    query_album.execute(album);

    FXASSERT(album);

    for (FXint i=0;i<tracks.no();i++){
      update_track_album.set(0,album);
      update_track_album.set(1,tracks[i]);
      update_track_album.execute();
      }
    }
  }




/*********************************************/





void GMTrackDatabase::sync_tracks_removed() {
  DEBUG_DB_SET();
  GM_TICKS_START();
  execute("DELETE FROM albums WHERE id NOT IN (SELECT DISTINCT(album) FROM tracks);");
  execute("DELETE FROM artists WHERE id NOT IN (SELECT artist FROM albums UNION SELECT artist FROM tracks UNION SELECT composer FROM tracks WHERE composer!=NULL UNION SELECT conductor FROM tracks WHERE conductor!=NULL );");
  execute("DELETE FROM tags WHERE id NOT IN (SELECT tag FROM track_tags UNION SELECT genre FROM streams);");
  execute("DELETE FROM pathlist WHERE id NOT IN (SELECT DISTINCT(path) FROM tracks);");
  GM_TICKS_END();
  }

void GMTrackDatabase::removeTracks(const FXIntList & tracks) {
  DEBUG_DB_SET();
  GM_TICKS_START();
  for (FXint i=0;i<tracks.no();i++){
    delete_track.update(tracks[i]);
    }
  for (FXint i=0;i<tracks.no();i++){
    delete_playlist_track.update(tracks[i]);
    }
  for (FXint i=0;i<tracks.no();i++){
    delete_tag_track.update(tracks[i]);
    }
  GM_TICKS_END();
  sync_tracks_removed();
  }

void GMTrackDatabase::removeTrack(FXint track) {
  DEBUG_DB_SET();
  delete_track.update(track);
  }

void GMTrackDatabase::removePlaylistTracks(FXint playlist,const FXIntList & queue){
  DEBUG_DB_SET();
  GMQuery q;
  q = compile("DELETE FROM playlist_tracks WHERE playlist == ? AND queue == ?;");
  for (FXint i=0;i<queue.no();i++) {
    q.set(0,playlist);
    q.set(1,queue[i]);
    q.execute();
    }
  }
