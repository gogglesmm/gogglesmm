/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2006-2018 by Sander Jansen. All Rights Reserved      *
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

/// For listing default genres
#include <id3v1genres.h>

#include "gmdefs.h"
#include "GMTrack.h"
#include "GMTrackDatabase.h"
#include "GMTrackList.h"
#include "GMSource.h"
#include "GMTag.h"
#include "gmutils.h"

#ifdef __GNUC__
#define DEBUG_DB_GET() FXTRACE((51,"%s\n",__PRETTY_FUNCTION__))
#define DEBUG_DB_SET() FXTRACE((52,"%s\n",__PRETTY_FUNCTION__))
#else
#define DEBUG_DB_GET() FXTRACE((51,"%s\n",__func__))
#define DEBUG_DB_SET() FXTRACE((52,"%s\n",__func__))
#endif


#define GOGGLESMM_DATABASE_SCHEMA_VERSION 2018  /* Lyrics */
#define GOGGLESMM_DATABASE_SCHEMA_V15     2017  /* Album Audio Quality*/
#define GOGGLESMM_DATABASE_SCHEMA_V14     2016  /* add autodownload to feed table*/
#define GOGGLESMM_DATABASE_SCHEMA_V13     2015  /* Fix empty tags and add foreign reference to feeds table*/
#define GOGGLESMM_DATABASE_SCHEMA_DEV4    2014  /* Foreign Keys Fix*/
#define GOGGLESMM_DATABASE_SCHEMA_DEV3    2013  /* Foreign Keys */
#define GOGGLESMM_DATABASE_SCHEMA_DEV2    2012  /* Feed Tables */
#define GOGGLESMM_DATABASE_SCHEMA_DEV1    2010  /* Dev DB before podcast manager */
#define GOGGLESMM_DATABASE_SCHEMA_V12     2009
#define GOGGLESMM_DATABASE_SCHEMA_V10     2008


const FXchar create_feed[]=           "CREATE TABLE IF NOT EXISTS feeds ("
                                          "id INTEGER NOT NULL,"
                                          "url TEXT,"
                                          "title TEXT,"
                                          "description TEXT,"
                                          "local TEXT,"
                                          "tag INTEGER REFERENCES tags(id),"
                                          "date INTEGER,"
                                          "http_etag TEXT,"
                                          "http_modified INTEGER,"
                                          "autodownload INTEGER,"
                                          "PRIMARY KEY (id) );";

const FXchar create_feed_items[] =    "CREATE TABLE IF NOT EXISTS feed_items ("
                                          "id INTEGER NOT NULL,"
                                          "feed INTEGER NOT NULL REFERENCES feeds(id), "
                                          "guid TEXT NOT NULL, "
                                          "url TEXT NOT NULL, "
                                          "local TEXT, "
                                          "title TEXT, "
                                          "description TEXT, "
                                          "size INTEGER,"
                                          "time INTEGER,"
                                          "date INTEGER,"
                                          "flags INTEGER,"
                                          "PRIMARY KEY (id) );";

const FXchar create_streams[]=        "CREATE TABLE IF NOT EXISTS streams ("
                                          "id INTEGER NOT NULL, "
                                          "url TEXT, "
                                          "description TEXT, "
                                          "genre INTEGER REFERENCES tags(id),"
                                          "bitrate INTEGER, "
                                          "rating INTEGER, "
                                          "PRIMARY KEY (id) );";

const FXchar create_tracks[]=         "CREATE TABLE tracks ( "
                                          "id INTEGER NOT NULL,"
                                          "collection INTEGER NOT NULL,"
                                          "path INTEGER NOT NULL REFERENCES pathlist (id),"
                                          "mrl TEXT,"
                                          "title TEXT NOT NULL,"
                                          "time INTEGER,"
                                          "no INTEGER,"
                                          "year INTEGER,"
                                          "bitrate INTEGER,"
                                          "album INTEGER NOT NULL REFERENCES albums (id),"
                                          "artist INTEGER NOT NULL REFERENCES artists (id),"
                                          "composer INTEGER REFERENCES artists (id),"
                                          "conductor INTEGER REFERENCES artists (id),"
                                          "playcount INTEGER,"
                                          "playdate INTEGER,"
                                          "importdate INTEGER,"
                                          "rating INTEGER,"
                                          "samplerate INTEGER,"
                                          "channels INTEGER,"
                                          "filetype INTEGER,"
                                          "lyrics TEXT,"
                                          "PRIMARY KEY (id) );";

const FXchar create_tags[]=           "CREATE TABLE tags ("
                                          "id INTEGER NOT NULL,"
                                          "name TEXT NOT NULL UNIQUE,"
                                          "PRIMARY KEY (id) );";

const FXchar create_track_tags[]=     "CREATE TABLE track_tags ("
                                          "track INTEGER NOT NULL REFERENCES tracks(id),"
                                          "tag INTEGER NOT NULL REFERENCES tags(id),"
                                          "PRIMARY KEY (track, tag) );";

const FXchar create_albums[]=         "CREATE TABLE albums ("
                                          "id INTEGER NOT NULL,"
                                          "name TEXT NOT NULL,"
                                          "artist INTEGER NOT NULL REFERENCES artists (id),"
                                          "year INTEGER,"
                                          "audio_channels INTEGER,"  /* 2,4,6*/
                                          "audio_rate INTEGER,"      /* 44100, 48000, 96 */
                                          "audio_format INTEGER,"    /* 16 / 24 */  //
                                          "PRIMARY KEY (id),"
                                          "UNIQUE(name,artist,year,audio_channels,audio_rate,audio_format));";

const FXchar create_artists[]=        "CREATE TABLE artists ("
                                          "id INTEGER NOT NULL,"
                                          "name TEXT NOT NULL UNIQUE,"
                                          "PRIMARY KEY (id) );";

const FXchar create_playlists[]=      "CREATE TABLE playlists ("
                                          "id INTEGER NOT NULL,"
                                          "name TEXT,"
                                          "PRIMARY KEY (id));";

const FXchar create_playlist_tracks[]="CREATE TABLE playlist_tracks ("
                                          "playlist INTEGER NOT NULL REFERENCES playlists(id),"
                                          "track INTEGER NOT NULL REFERENCES tracks(id),"
                                          "queue INTEGER );";

const FXchar create_pathlist[]=       "CREATE TABLE pathlist ("
                                          "id INTEGER NOT NULL,"
                                          "name TEXT NOT NULL UNIQUE,"
                                          "PRIMARY KEY (id));";



GMTrackDatabase::GMTrackDatabase()  {
  }


GMTrackDatabase::~GMTrackDatabase() {
  clear_path_lookup();
  clear_artist_lookup();
  }


FXbool GMTrackDatabase::init(const FXString & database) {
  FXint dbversion = 0;

  if (!open(database))
    goto error;

  dbversion = getVersion();

  if ( dbversion > GOGGLESMM_DATABASE_SCHEMA_VERSION) {
    if (FXMessageBox::question(FXApp::instance(),MBOX_OK_CANCEL,fxtr("Database Error"),fxtr("An incompatible (future) version of the database was found.\nThis usually happens when you try to downgrade to a older version of GMM\nPress OK to continue and reset the database (all information will be lost!).\nPress Cancel to quit now and leave the database as is."))==MBOX_CLICKED_CANCEL)
      return false;
    }

  // Warn if there's no upgrade path
  if ( dbversion>0 && dbversion<GOGGLESMM_DATABASE_SCHEMA_DEV1) {
    if (FXMessageBox::question(FXApp::instance(),MBOX_OK_CANCEL,fxtr("Database Error"),fxtr("An incompatible (older) version of the database was found.\nPress OK to continue and reset the database (all information will be lost!).\nPress Cancel to quit now and leave the database as is."))==MBOX_CLICKED_CANCEL)
      return false;
    }

  if (!init_database())
    goto error;

  if (!init_queries())
    goto error;

  return true;
error:
  FXMessageBox::error(FXApp::instance(),MBOX_OK,fxtr("Fatal Error"),fxtr("Goggles Music Manager was unable to open the database.\nThe database may have been corrupted. Please remove %s to try again.\nif the error keeps occuring, please file an issue at http://gogglesmm.github.io"),database.text());
  return false;
  }



void GMTrackDatabase::init_index() {
  /*
    https://www.sqlite.org/foreignkeys.html#fk_indexes
    So, in most real systems, an index should be created on the child key columns of each foreign key constraint.
  */
  execute("CREATE INDEX IF NOT EXISTS feeds_tag ON feeds(tag)");

  execute("CREATE INDEX IF NOT EXISTS feed_items_feed ON feed_items(feed)");
  execute("CREATE INDEX IF NOT EXISTS streams_genre ON streams(genre)");

  execute("CREATE INDEX IF NOT EXISTS tracks_album ON tracks(album)");
  execute("CREATE INDEX IF NOT EXISTS tracks_mrl ON tracks(mrl)");
  execute("CREATE INDEX IF NOT EXISTS tracks_has_track ON tracks(path,mrl)");
  execute("CREATE INDEX IF NOT EXISTS tracks_path ON tracks(path)");
  execute("CREATE INDEX IF NOT EXISTS tracks_artist ON tracks(artist)");
  execute("CREATE INDEX IF NOT EXISTS tracks_composer ON tracks(composer)");
  execute("CREATE INDEX IF NOT EXISTS tracks_conductor ON tracks(conductor)");

  execute("CREATE INDEX IF NOT EXISTS track_tags_track ON track_tags(track)");
  execute("CREATE INDEX IF NOT EXISTS track_tags_tag ON track_tags(tag)");

  execute("CREATE INDEX IF NOT EXISTS albums_artist ON albums(artist)");

  execute("CREATE INDEX IF NOT EXISTS playlist_tracks_track ON playlist_tracks(track)");
  execute("CREATE INDEX IF NOT EXISTS playlist_tracks_playlist ON playlist_tracks(playlist)");
  }

void GMTrackDatabase::fix_empty_tags(){

  // Remove empty string tags from tracks
  execute("DELETE FROM track_tags WHERE tag == (SELECT id FROM tags WHERE name == '')");

  // Reset tag in feeds with empty string tag
  execute("UPDATE streams SET genre = NULL WHERE genre == (SELECT id FROM tags WHERE name == '')");

  // Reset tag in feeds with empty string tag
  execute("UPDATE feeds SET tag = NULL WHERE tag == (SELECT id FROM tags WHERE name == '')");

  // Remove empty string tags
  execute("DELETE FROM tags WHERE name == ''");
  }


void GMTrackDatabase::init_album_properties() {
  GMAudioProperties props;
  GMCoverPathList   albums;
  GMQuery update_album(this,"UPDATE albums SET audio_channels = ?, audio_rate = ?, audio_format = ? WHERE id = ?");

  if (listAlbumPaths(albums)) {
    for (FXint i=0;i<albums.no();i++){
      if (props.load(albums[i].path)) {
        update_album.set(0,props.channels);
        update_album.set(1,props.samplerate);
        update_album.set(2,props.samplesize);
        }
      else {
        update_album.set(0,0);
        update_album.set(1,0);
        update_album.set(2,0);
        }
      update_album.set(3,albums[i].id);
      update_album.execute();
      }
    }
  }


FXbool GMTrackDatabase::init_database() {
  try {

    switch(getVersion()) {

      // All's well.
      case GOGGLESMM_DATABASE_SCHEMA_VERSION:
        break;

      // These should never be encountered
      case GOGGLESMM_DATABASE_SCHEMA_V10    :
      case GOGGLESMM_DATABASE_SCHEMA_V12    :
        FXASSERT(0);
        return false;
        break;

      case GOGGLESMM_DATABASE_SCHEMA_DEV1 :

        // Create the feeds table
        execute(create_feed);
        execute(create_feed_items);

        // fallthrough - intentionally no break

      case GOGGLESMM_DATABASE_SCHEMA_DEV2 :

        recreate_table("tracks",create_tracks);
        recreate_table("streams",create_streams);
        recreate_table("feeds",create_feed);
        recreate_table("feed_items",create_feed_items);

        // fallthrough - intentionally no break

      case GOGGLESMM_DATABASE_SCHEMA_DEV3 :

        recreate_table("playlist_tracks",create_playlist_tracks);

        // fallthrough - intentionally no break

      case GOGGLESMM_DATABASE_SCHEMA_DEV4 :

        recreate_table("feeds",create_feed);
        fix_empty_tags();

        // fallthrough - intentionally no break

      case GOGGLESMM_DATABASE_SCHEMA_V13  :

        execute("ALTER TABLE feeds ADD COLUMN autodownload INTEGER");
        execute("UPDATE feeds SET autodownload = 0");

        // fallthrough -intentionally no break

      case GOGGLESMM_DATABASE_SCHEMA_V14  :

        execute("ALTER TABLE albums ADD COLUMN audio_channels INTEGER");
        execute("ALTER TABLE albums ADD COLUMN audio_rate INTEGER");
        execute("ALTER TABLE albums ADD COLUMN audio_format INTEGER");
        execute("ALTER TABLE tracks ADD COLUMN samplerate INTEGER");
        execute("ALTER TABLE tracks ADD COLUMN channels INTEGER");
        execute("ALTER TABLE tracks ADD COLUMN filetype INTEGER");

        init_album_properties();
        recreate_table("albums",create_albums); // fixup constraints

        // fallthrough - intentionally no break

      case GOGGLESMM_DATABASE_SCHEMA_V15  :

        if (!hasColumn("tracks","lyrics")) {
          execute("ALTER TABLE tracks ADD COLUMN lyrics TEXT");
          }

        setVersion(GOGGLESMM_DATABASE_SCHEMA_VERSION);
        break;

      // Unknown/Uninitialized database. Start from scratch
      default                               :
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
        execute(create_feed);
        execute(create_feed_items);
        setVersion(GOGGLESMM_DATABASE_SCHEMA_VERSION);
        break;
      }

    // Turn on forein keys
    enableForeignKeys();

    // Index if needed
    init_index();
    }
  catch(GMDatabaseException&) {
    return false;
    }
  return true;
  }


FXbool GMTrackDatabase::init_queries() {
  try {
    insert_path                         = compile("INSERT OR IGNORE INTO pathlist VALUES ( NULL , ? );");
    insert_artist                       = compile("INSERT OR IGNORE INTO artists VALUES ( NULL , ? );");
    insert_playlist_track_by_id         = compile("INSERT INTO playlist_tracks VALUES (?,?,?);");

    query_filename                      = compile("SELECT id, importdate FROM tracks WHERE path == ? AND mrl == ?;");
    query_path                          = compile("SELECT id FROM pathlist WHERE name = ?;");
    query_artist                        = compile("SELECT id FROM artists WHERE name == ?;");
    query_path_name                     = compile("SELECT name FROM pathlist WHERE id == ?;");

    query_track                         = compile("SELECT pathlist.name || '" PATHSEPSTRING "' || mrl, albums.name, a1.name, a2.name, composer_artist.name, conductor_artist.name,title, time, no, tracks.year, tracks.rating, tracks.samplerate, tracks.channels, tracks.filetype, tracks.bitrate, tracks.lyrics "
                                                  "FROM tracks LEFT JOIN artists AS composer_artist ON tracks.composer == composer_artist.id LEFT JOIN artists AS conductor_artist ON tracks.conductor == conductor_artist.id,pathlist, albums, artists AS a1, artists AS a2 "
                                                  "WHERE tracks.path == pathlist.id "
                                                    "AND albums.id == tracks.album "
                                                    "ANd a1.id == albums.artist "
                                                    "AND a2.id == tracks.artist "
                                                    "AND tracks.id == ?;");

    query_track_tags                    = compile("SELECT name FROM tags WHERE id IN (SELECT tag FROM track_tags WHERE track == ?) ORDER BY name;");



    query_track_filename                = compile("SELECT name ||'" PATHSEPSTRING "' || mrl FROM tracks,pathlist WHERE tracks.path == pathlist.id AND tracks.id == ?;");

    query_album_artists                 = compile("SELECT albums.artist,album FROM tracks,albums WHERE albums.id ==tracks.album AND tracks.id == ?;");

    //query_playlist_queue                = compile("SELECT MAX(queue) FROM playlist_tracks WHERE playlist == ?;");
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
    GMLockTransaction transaction(this);
    execute("DELETE FROM playlist_tracks;");
    execute("DELETE FROM track_tags;");
    execute("DELETE FROM tracks;");
    execute("DELETE FROM pathlist;");
    execute("DELETE FROM albums;");
    execute("DELETE FROM artists;");
    execute("DELETE FROM tags WHERE id NOT IN (SELECT genre FROM streams UNION SELECT tag FROM feeds);");
    if (removeplaylists) {
      execute("DELETE FROM playlists;");
      }
    transaction.commit();
    }
  catch(GMDatabaseException&) {
    return false;
    }
  vacuum();
  return true;
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



FXint GMTrackDatabase::hasPath(const FXString & path){
  DEBUG_DB_GET();
  GM_TICKS_START();
  FXint pid=0;
  query_path.execute(path,pid);
  GM_TICKS_END();
  return pid;
  }


FXint GMTrackDatabase::hasTrack(const FXString & filename,FXint pid,FXTime & modified) {
  DEBUG_DB_GET();
  FXASSERT(pid);
  GM_TICKS_START();
  FXint tid=0;
  query_filename.set(0,pid);
  query_filename.set(1,filename);
  if (query_filename.row()){
    query_filename.get(0,tid);
    query_filename.get(1,modified);
    }
  query_filename.reset();
  GM_TICKS_END();
  return tid;
  }



//FIXME rollback
FXbool GMTrackDatabase::insertStream(const FXString & url,const FXString & description,const FXString & genre){
  DEBUG_DB_SET();
  try {
    GMLockTransaction transaction(this);
    FXint genreid=0;

    GMQuery insert_tag(this,"INSERT OR IGNORE INTO tags VALUES ( NULL, ? );");
    GMQuery query_tag(this,"SELECT id FROM tags WHERE name == ?;");

    if (!genre.empty()){
      query_tag.execute(genre,genreid);
      if (!genreid) genreid = insert_tag.insert(genre);
      }

    GMQuery q(this,"INSERT INTO streams VALUES(NULL,?,?,?,0,0);");
    q.set(0,url);
    q.set(1,description);
    q.set_null(2,genreid);
    q.execute();
    transaction.commit();
    }
  catch(GMDatabaseException & e){
    return false;
    }
  return true;
  }


/// Insert Playlist into database
FXbool GMTrackDatabase::insertPlaylist(const FXString & name,FXint & id) {
  DEBUG_DB_SET();
  try {
    GMLockTransaction transaction(this);
    GMQuery query(this,"INSERT INTO playlists VALUES ( NULL, ? );");
    id = query.insert(name);
    transaction.commit();
    }
  catch (GMDatabaseException & e){
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
    GMLockTransaction transaction(this);
    GMQuery q(this,"SELECT coalesce(MAX(queue)+1,1) FROM playlist_tracks WHERE playlist == ?;");
    q.execute(playlist,max);
    for (FXint i=0;i<tracks.no();i++) {
      insert_playlist_track_by_id.set(0,playlist);
      insert_playlist_track_by_id.set(1,tracks[i]);
      insert_playlist_track_by_id.set(2,max++);
      insert_playlist_track_by_id.execute();
      }
    transaction.commit();
    }
  catch(GMDatabaseException&){
    return false;
    }
  GM_TICKS_END();
  return true;
  }


///FIXME Insert Track in Playlist
#if 0
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
#endif

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
  FXint genreid=0;
  FXString query;
  try {
    /// Query for existing genre
    GMQuery insert_tag(this,"INSERT OR IGNORE INTO tags VALUES ( NULL, ? );");
    GMQuery query_tag(this,"SELECT id FROM tags WHERE name == ?;");

    if (!name.empty()){
      query_tag.execute(name,genreid);
      if (!genreid) genreid = insert_tag.insert(name);
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

void GMTrackDatabase::getPathList(const FXString & path,FXStringList & result) {
  DEBUG_DB_GET();
  GMQuery list;
  result.clear();
  list = compile("SELECT pathlist.name FROM pathlist WHERE pathlist.name == ? OR pathlist.name LIKE ? ORDER BY pathlist.name;");
  list.set(0,path);
  list.set(1,path+PATHSEPSTRING+'%');
  while(list.row()){
    result.no(result.no()+1);
    list.get(0,result[result.no()-1]);
    }
  }


void GMTrackDatabase::getFileList(const FXString & path,GMTrackFilenameList & result) {
  DEBUG_DB_GET();
  GMQuery list;
  result.clear();
  list = compile("SELECT tracks.id, mrl,importdate FROM tracks,pathlist WHERE tracks.path == pathlist.id AND pathlist.name == ?");
  list.set(0,path);
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

FXbool GMTrackDatabase::isEmpty() {
  DEBUG_DB_GET();
  FXint total=0;
  execute("SELECT SUM(rows) FROM ( "
              "SELECT COUNT(*) AS rows FROM tracks UNION ALL "
              "SELECT COUNT(*) AS rows FROM feeds UNION ALL "
              "SELECT COUNT(*) As rows FROM streams"
              ");",total);

  return (total==0);
  }

FXint GMTrackDatabase::getNumTracks() {
  DEBUG_DB_GET();
  FXint total=0;
  execute("SELECT COUNT(*) FROM tracks;",total);
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
FXbool GMTrackDatabase::listAlbumPaths(GMCoverPathList & list){
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
      return true;
      }
    return false;
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
const FXchar * GMTrackDatabase::getTrackPath(FXint pid) {
  const void * ptr = pathdict.at((void*)(FXival)pid);
  if (__likely(ptr)) return (const FXchar*)ptr;
  initPathLookup();
  ptr = pathdict.at((void*)(FXival)pid);
  if (__likely(ptr)) return (const FXchar*)ptr;
  return "";
  }

/// Return artist;
const FXString * GMTrackDatabase::getArtist(FXint aid) {
  if (__likely(aid>0)) {
    const void * ptr = artistdict.at((void*)(FXival)aid);
    if (__likely(ptr)) return (const FXString*)ptr;
    initArtistLookup();
    ptr = artistdict.at((void*)(FXival)aid);
    if (__likely(ptr)) return (const FXString*)ptr;
    }
  return &empty;
  }


void GMTrackDatabase::getTrackPath(FXint id,FXString & path) {
  DEBUG_DB_GET();
  static FXint last=-1;
  static FXString last_path;
  GM_TICKS_START();
  if (last==id) {
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
  FXint  value;
  try {
    //begin();
    query_track.set(0,tid);
    if (query_track.row()) {
      query_track.get( 0,track.url);
      query_track.get( 1,track.album);
      query_track.get( 2,track.album_artist);
      query_track.get( 3,track.artist);
      query_track.get( 4,track.composer);
      query_track.get( 5,track.conductor);
      query_track.get( 6,track.title);
      query_track.get( 7,track.time);
      query_track.get( 8,track.no);
      query_track.get( 9,track.year);
      query_track.get(10,track.rating);
      query_track.get(11,track.samplerate);

      query_track.get(12,value);
      track.channels=value;

      query_track.get(13,value);
      track.filetype=value;

      query_track.get(14,value);
      if (value<0) {
        track.sampleformat = -value;
        track.bitrate = 0;
        }
      else {
        track.bitrate = value;
        track.sampleformat = 0;
        }

      query_track.get_null(15,track.lyrics);

      ok=true;
      }
    query_track.reset();

    /// Get All tags
    query_track_tags.set(0,tid);
    track.tags.clear();
    while(query_track_tags.row()){
      track.tags.append(query_track_tags.get(0));
      }
    query_track_tags.reset();


    //commit();
    }
  catch (GMDatabaseException & e){
    //rollback();
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
        query_track.get( 0,tracks[i].url);
        query_track.get( 1,tracks[i].album);
        query_track.get( 2,tracks[i].album_artist);
        query_track.get( 3,tracks[i].artist);
        query_track.get( 4,tracks[i].composer);
        query_track.get( 5,tracks[i].conductor);
        query_track.get( 6,tracks[i].title);
        query_track.get( 7,tracks[i].time);
        query_track.get( 8,tracks[i].no);
        query_track.get( 9,tracks[i].year);
        query_track.get(10,tracks[i].rating);
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



FXbool GMTrackDatabase::removeArtist(FXint artist) {
  DEBUG_DB_SET();
  try {
    GMQuery query;
    GMLockTransaction transaction(this);

    query = compile("DELETE FROM playlist_tracks WHERE track IN (SELECT id FROM tracks WHERE artist == ? OR album IN (SELECT id FROM albums WHERE artist == ?))");
    query.set(0,artist);
    query.set(1,artist);
    query.execute();

    query = compile("DELETE FROM track_tags WHERE track IN (SELECT id FROM tracks WHERE artist == ? OR album IN (SELECT id FROM albums WHERE artist == ?))");
    query.set(0,artist);
    query.set(1,artist);
    query.execute();

    query = compile("DELETE FROM tracks WHERE artist == ? OR album IN ( SELECT id FROM albums WHERE artist == ?);");
    query.set(0,artist);
    query.set(1,artist);
    query.execute();

    query = compile("DELETE FROM albums WHERE artist == ?;");
    query.update(artist);

    sync_tracks_removed();
    transaction.commit();
    }
  catch (GMDatabaseException & e){
    return false;
    }
  return true;
  }

FXbool GMTrackDatabase::removeAlbum(FXint album) {
  DEBUG_DB_SET();
  try {
    GMQuery query;
    GMLockTransaction transaction(this);

    /// Remove tracks from playlist
    query = compile("DELETE FROM playlist_tracks WHERE track IN (SELECT id FROM tracks WHERE album == ?);");
    query.update(album);

    // Remove tracks from track_tags
    query = compile("DELETE FROM track_tags WHERE track IN (SELECT id FROM tracks WHERE album == ?);");
    query.update(album);

    /// Removes tracks with album
    query = compile("DELETE FROM tracks WHERE album == ?;");
    query.update(album);

    /// Remove album itself
    query = compile("DELETE FROM albums WHERE id == ?;");
    query.update(album);

    /// Cleanup
    execute("DELETE FROM artists WHERE id NOT IN (SELECT artist FROM albums UNION SELECT artist FROM tracks UNION SELECT composer FROM tracks UNION SELECT conductor FROM tracks);");
    clean_tags();
    execute("DELETE FROM pathlist WHERE id NOT IN (SELECT DISTINCT(path) FROM tracks);");

    transaction.commit();
    }
  catch (GMDatabaseException & e){
    return false;
    }
  return true;
  }

/// For each playlist, we reset the queue number from 1 to max tracks.
void GMTrackDatabase::reorderPlaylists(){
  DEBUG_DB_SET();

  GMQuery q;
  GM_TICKS_START();
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
  GM_TICKS_END();
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
  try {
    GMQuery q;
    GMLockTransaction transaction(this);

    q = compile("DELETE FROM playlist_tracks WHERE playlist == ?;");
    q.update(playlist);

    for (FXint i=0;i<items.no();i++){
      insert_playlist_track_by_id.set(0,playlist);
      insert_playlist_track_by_id.set(1,items[i].track);
      insert_playlist_track_by_id.set(2,items[i].queue);
      insert_playlist_track_by_id.execute();
      }
    transaction.commit();
    }
  catch (GMDatabaseException & e){
    return false;
    }
  return true;
  }



FXbool GMTrackDatabase::removePlaylist(FXint playlist){
  DEBUG_DB_SET();
  try {
    GMQuery q;
    GMLockTransaction transaction(this);

    q = compile("DELETE FROM playlist_tracks WHERE playlist == ?;");
    q.update(playlist);

    q = compile("DELETE FROM playlists WHERE id == ?;");
    q.update(playlist);

    transaction.commit();
    }
  catch (GMDatabaseException & e){
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


FXbool GMTrackDatabase::listTags(FXComboBox * list,FXbool insert_default){
  DEBUG_DB_GET();
  int i=0;
  FXDictionary tags;
  FXString name;
  FXint id;
  try {
    GMQuery list_tags(this,"SELECT id,name FROM tags WHERE id IN (SELECT DISTINCT(tag) FROM track_tags);");

    if (insert_default) {
      for (i=0;!TagLib::ID3v1::genre(i).isEmpty();i++) {
        tags.insert(TagLib::ID3v1::genre(i).toCString(true),(void*)(FXival)1);
        list->appendItem(TagLib::ID3v1::genre(i).toCString(true));
        }
      while(list_tags.row()) {
        list_tags.get(0,id);
        list_tags.get(1,name);
        if (!name.empty() && !tags.has(name))
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
    GMQuery list_artists(this,"SELECT id,name FROM artists;");
    while(list_artists.row()){
      list_artists.get(0,id);
      list_artists.get(1,name);
      list->appendItem(name);
      }
    list->sortItems();
    }
  catch(GMDatabaseException & e){
    list->clearItems();
    return false;
    }
  return true;
  }

FXbool GMTrackDatabase::listAlbums(FXComboBox * list,FXint track){
  DEBUG_DB_GET();
  try {
    GMQuery list_albums_with_artist_from_track(this,"SELECT albums.name FROM tracks AS t1 JOIN tracks AS t2 ON (t1.artist == t2.artist AND t1.id==?) JOIN albums ON (albums.id == t2.album) GROUP BY t2.album");
    list_albums_with_artist_from_track.set(0,track);
    while(list_albums_with_artist_from_track.row()) {
      list->appendItem(list_albums_with_artist_from_track.get(0));
      }
    }
  catch(GMDatabaseException & e){
    list->clearItems();
    return false;
    }
  return true;
  }





void GMTrackDatabase::clear_path_lookup() {
  DEBUG_DB_GET();
  for (FXint i=0;i<pathdict.no();i++) {
    if (!pathdict.empty(i) && pathdict.data(i)!=nullptr) {
      free(pathdict.data(i));
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

void GMTrackDatabase::initPathLookup() {
  clear_path_lookup();
  setup_path_lookup();
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
  for (FXint i=0;i<artistdict.no();i++) {
    if (!artistdict.empty(i) && artistdict.data(i)!=nullptr) {
      FXString * a = (FXString*)artistdict.data(i);
      delete a;
      }
    }
  artistdict.clear();
  }

void GMTrackDatabase::initArtistLookup() {
  clear_artist_lookup();
  setup_artist_lookup();
  }


FXbool GMTrackDatabase::vacuum() {
  DEBUG_DB_SET();
  GM_TICKS_START();
  try {
    GMLockTransaction transaction(this);

  /// Remove empty playlists ? FIXME: may be not...
  /// if (!execute("DELETE FROM playlists WHERE id NOT IN (SELECT DISTINCT(playlist) FROM playlist_tracks);"))
  //  return false;

    GM_DEBUG_PRINT("Reorder Playlists\n");

    /// Reorder Playlists
    reorderPlaylists();

    GM_DEBUG_PRINT("Remove Empty Albums\n");

    /// Remove empty albums
    execute("DELETE FROM albums WHERE id NOT IN (SELECT DISTINCT(album) FROM tracks);");

    GM_DEBUG_PRINT("Remove unused artists\n");

    /// Remove unused artists
    execute("DELETE FROM artists WHERE id NOT IN (SELECT artist FROM albums UNION SELECT artist FROM tracks UNION SELECT composer FROM tracks UNION SELECT conductor FROM tracks);");

    GM_DEBUG_PRINT("Remove unused genres\n");

    /// Remove unused tags
    clean_tags();

    GM_DEBUG_PRINT("Remove unused paths\n");

    /// Remove unused paths
    execute("DELETE FROM pathlist WHERE id NOT IN (SELECT DISTINCT(path) FROM tracks);");

    /// commit changes
    transaction.commit();

    /// Reinitialize path lookup
    clear_path_lookup();
    clear_artist_lookup();
    setup_path_lookup();
    setup_artist_lookup();
    }
  catch(GMDatabaseException &) {
    return false;
    }
  GM_TICKS_END();
  return true;
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
  FILE * fp = nullptr;
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
      query+=" AND playlist_tracks.playlist == "+FXString::value(playlist);
      query+=" ORDER BY playlist_tracks.queue; ";
      }
    else {
      query+=";";
      }

    list = compile(query);

    fp = fopen(filename.text(),"w");
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
        fprintf(fp,"\t\t\t<location>%s</location>\n",FXURL::fileToURL(file).text());
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
    fp=nullptr;
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
  //begin();
  update_track_importdate.set(0,tm);
  update_track_importdate.set(1,track);
  update_track_importdate.execute();
  //commit();
  }

void GMTrackDatabase::setTrackRating(FXint id,FXuchar rating){
  DEBUG_DB_SET();
  GMLockTransaction transaction(this);
  update_track_rating.set(0,(FXuint)rating);
  update_track_rating.set(1,id);
  update_track_rating.execute();
  transaction.commit();
  }

void GMTrackDatabase::setTrackPlayed(FXint track,FXlong time) {
  DEBUG_DB_SET();
  GMLockTransaction transaction(this);
  update_track_playcount.set(0,time);
  update_track_playcount.set(1,track);
  update_track_playcount.execute();
  transaction.commit();
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

/// Set the name of a track
void GMTrackDatabase::setTrackLyrics(FXint track,const FXString & lyrics){
  DEBUG_DB_SET();
  GMQuery update_track_lyrics(this,"UPDATE tracks SET lyrics = ? WHERE id == ?;");
  update_track_lyrics.set(0,lyrics);
  update_track_lyrics.set(1,track);
  update_track_lyrics.execute();
  }


void GMTrackDatabase::updateAlbumYear(const FXIntList & tracks) {
  DEBUG_DB_SET();
  FXint album;
  FXIntMap albums;
  GMQuery query_album(this,"SELECT album FROM tracks WHERE id == ?");
  // Set album year to minimum year that is not 0 or 0 if all are 0.
  GMQuery update_album_year(this,"UPDATE albums SET year = ( SELECT ifnull(MIN(year),0) FROM tracks WHERE album = ? ) WHERE id = ?;");
  for (FXint i=0;i<tracks.no();i++){
    album=0;
    query_album.execute(tracks[i],album);
    FXASSERT(album);
    if (album && albums.at(album)==0) {
      update_album_year.set(0,album);
      update_album_year.set(1,album);
      update_album_year.execute();
      albums.insert(album,1);
      }
    }
  }



void GMTrackDatabase::setTrackTags(const FXIntList & tracks,const FXStringList & tags) {
  DEBUG_DB_SET();

  GMQuery reset_track_tags(this,"DELETE FROM track_tags WHERE track == ?;");
  GMQuery insert_track_tags(this,"INSERT OR IGNORE INTO track_tags VALUES ( ?, ? );");
  GMQuery insert_tag(this,"INSERT OR IGNORE INTO tags VALUES ( NULL, ? );");
  GMQuery query_tag(this,"SELECT id FROM tags WHERE name == ?;");

  FXIntList ids;
  ids.no(tags.no());

  /// Insert all tags
  for (FXint i=0;i<tags.no();i++) {
    ids[i]=0;
    query_tag.execute(tags[i],ids[i]);
    if (!ids[i])
      ids[i] = insert_tag.insert(tags[i]);
    }

  for (FXint t=0;t<tracks.no();t++) {
    // clear existing tags
    reset_track_tags.set(0,tracks[t]);
    reset_track_tags.execute();

    // Set tags
    for (FXint i=0;i<tags.no();i++){
      insert_track_tags.set(0,tracks[t]);
      insert_track_tags.set(1,ids[i]);
      insert_track_tags.execute();
      }
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

  // Warning don't add "a1.id!=a2.id" since the track may already be assigned to the correct album.
  // This can happen when you do a batch update of tracks and some of them already are set to the correct album entry.
  GMQuery query_existing(this,"SELECT a2.id FROM albums AS a1 JOIN tracks ON tracks.album == a1.id "
                                                             "JOIN albums AS a2 ON a1.artist==a2.artist AND a1.audio_channels==a2.audio_channels "
                                                                                                       "AND a1.audio_rate==a2.audio_rate "
                                                                                                       "AND a1.audio_format==a2.audio_format "
                                                             "WHERE tracks.id == ? AND a2.name == ?;");

  GMQuery copy_album(this,"INSERT INTO albums (name,artist,year,audio_channels,audio_rate,audio_format) "
                          "SELECT ?,artist,year,audio_channels,audio_rate,audio_format "
                          "FROM albums "
                          "WHERE id = (SELECT album FROM tracks WHERE id == ?);");


  GMQuery update_track_album(this,"UPDATE tracks SET album = ? WHERE id == ?;");

  FXint album=0;
  if (sameartist) {
    query_existing.set(0,tracks[0]);
    query_existing.set(1,name);
    query_existing.execute(album);
    if (!album) {
      copy_album.set(0,name);
      copy_album.set(1,tracks[0]);
      copy_album.execute();
      query_existing.set(0,tracks[0]);
      query_existing.set(1,name);
      query_existing.execute(album);
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
      query_existing.set(0,tracks[0]);
      query_existing.set(1,name);
      query_existing.execute(album);
      if (!album) {
        copy_album.set(0,name);
        copy_album.set(1,tracks[i]);
        copy_album.execute();
        query_existing.set(0,tracks[i]);
        query_existing.set(1,name);
        query_existing.execute(album);
        FXASSERT(album);
        if (!album) throw GMDatabaseException();
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
  GMQuery query_existing(this,"SELECT a2.id FROM albums AS a1 JOIN tracks ON tracks.album == a1.id "
                                                                       "JOIN albums AS a2 ON a1.name == a2.name "
                                                                                        "AND a1.audio_rate==a2.audio_rate "
                                                                                        "AND a1.audio_format==a2.audio_format "
                                                                       "JOIN artists ON a2.artist == artists.id "
                                        "WHERE tracks.id == ? AND artists.name == ?;");
  GMQuery copy_album(this,"INSERT INTO albums (name,artist,year,audio_channels,audio_rate,audio_format) "
                          "SELECT name,"
                                "(SELECT id FROM artists WHERE name == ?),"
                                "albums.year,albums.audio_channels,albums.audio_rate,albums.audio_format "
                          "FROM albums JOIN tracks ON albums.id == tracks.album "
                          "WHERE tracks.id = ?;");

  GMQuery copy_album_with_title(this,"INSERT INTO albums (name,artist,year,audio_channels,audio_rate,audio_format) "
                          "SELECT ?,"
                                "(SELECT id FROM artists WHERE name == ?),"
                                "albums.year,albums.audio_channels,albums.audio_rate,albums.audio_format "
                          "FROM albums JOIN tracks ON albums.id == tracks.album "
                          "WHERE tracks.id = ?;");

  /// Make sure we have an artist.
  insert_artist.update(name);

  if (title.empty()) {

    for (FXint i=0;i<tracks.no();i++){
      album=0;
      query_existing.set(0,tracks[i]);
      query_existing.set(1,name);
      query_existing.execute(album);

      if (!album) {
        copy_album.set(0,name);
        copy_album.set(1,tracks[i]);
        copy_album.execute();

        query_existing.set(0,tracks[i]);
        query_existing.set(1,name);
        query_existing.execute(album);
        }
      FXASSERT(album);
      update_track_album.set(0,album);
      update_track_album.set(1,tracks[i]);
      update_track_album.execute();
      }
    }
  else {
    query_album.set(0,title);
    query_album.set(1,name);
    query_album.execute(album);

    if (!album) {
      copy_album_with_title.set(0,title);
      copy_album_with_title.set(1,name);
      copy_album_with_title.set(2,tracks[0]);
      copy_album_with_title.execute();

      query_album.set(0,title);
      query_album.set(1,name);
      query_album.execute(album);
      }

    FXASSERT(album);

    for (FXint i=0;i<tracks.no();i++){
      update_track_album.set(0,album);
      update_track_album.set(1,tracks[i]);
      update_track_album.execute();
      }
    }
  }




/*********************************************/



void GMTrackDatabase::clean_tags() {
  execute("DELETE FROM tags WHERE id NOT IN (SELECT tag FROM track_tags UNION SELECT genre FROM streams UNION SELECT tag FROM feeds);");
  }

void GMTrackDatabase::sync_tracks_removed() {
  DEBUG_DB_SET();
  GM_TICKS_START();
  execute("DELETE FROM albums WHERE id NOT IN (SELECT DISTINCT(album) FROM tracks);");
  execute("DELETE FROM artists WHERE id NOT IN (SELECT artist FROM albums UNION SELECT artist FROM tracks UNION SELECT composer FROM tracks WHERE composer!=0 UNION SELECT conductor FROM tracks WHERE conductor!=0 );");
  clean_tags();
  execute("DELETE FROM pathlist WHERE id NOT IN (SELECT DISTINCT(path) FROM tracks);");
  GM_TICKS_END();
  }

void GMTrackDatabase::sync_album_year() {
  DEBUG_DB_SET();
  GM_TICKS_START();
  execute("UPDATE albums SET year = (SELECT ifnull(MAX(year),0) FROM tracks WHERE album = albums.id AND year > 0);");
  GM_TICKS_END();
  }



void GMTrackDatabase::removeTracks(const FXIntList & tracks) {
  DEBUG_DB_SET();
  GM_TICKS_START();
  for (FXint i=0;i<tracks.no();i++){
    delete_playlist_track.update(tracks[i]);
    }
  for (FXint i=0;i<tracks.no();i++){
    delete_tag_track.update(tracks[i]);
    }
  for (FXint i=0;i<tracks.no();i++){
    delete_track.update(tracks[i]);
    }
  GM_TICKS_END();
  sync_tracks_removed();
  }

void GMTrackDatabase::removeTrack(FXint track) {
  DEBUG_DB_SET();
  delete_track.update(track);
  }

void GMTrackDatabase::removePlaylistTracks(FXint playlist,const FXIntList & tracks){
  DEBUG_DB_SET();
  GMQuery q;
  q = compile("DELETE FROM playlist_tracks WHERE playlist == ? AND track == ?;");
  for (FXint i=0;i<tracks.no();i++) {
    q.set(0,playlist);
    q.set(1,tracks[i]);
    q.execute();
    }
  reorderPlaylist(playlist);
  }


void GMTrackDatabase::removePlaylistQueue(FXint playlist,const FXIntList & queue){
  DEBUG_DB_SET();
  GMQuery q;
  q = compile("DELETE FROM playlist_tracks WHERE playlist == ? AND queue == ?;");
  for (FXint i=0;i<queue.no();i++) {
    q.set(0,playlist);
    q.set(1,queue[i]);
    q.execute();
    }
  reorderPlaylist(playlist);
  }
