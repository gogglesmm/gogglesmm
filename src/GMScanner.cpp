/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2006-2015 by Sander Jansen. All Rights Reserved      *
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
#include <tag.h>
#include "gmdefs.h"
#include "GMTaskManager.h"
#include "GMTrack.h"
#include "GMPreferences.h"
#include "GMDatabase.h"
#include "GMTrackDatabase.h"
#include "GMFilename.h"
#include "GMPlayerManager.h"
#include "GMAudioPlayer.h"
#include "GMScanner.h"
#include "GMTag.h"

// ALL patterns
//#define FILE_EXTENSIONS "ogg,flac,opus,oga,mp3,m4a,mp4,m4p,m4b,aac,mpc,wma,asf"
//#define FILE_PATTERNS "*.(" FILE_EXTENSIONS ")"

// Just the ones we can playback at the moment.
#define FILE_EXTENSIONS "ogg,flac,opus,oga,mp3,m4a,mp4,m4p,m4b,aac"
#define FILE_PATTERNS "*.(" FILE_EXTENSIONS ")"

GMDBTracks::GMDBTracks() : database(nullptr) {
  }

GMDBTracks::~GMDBTracks(){
  }

void GMDBTracks::initPathDict(GMTrackDatabase * db) {
  FXint id;
  const FXchar * path;
  GMQuery query(db,"SELECT id,name FROM pathlist;" );
  while(query.row()) {
    query.get(0,id);
    path = query.get(1);
    pathdict.insert(path,(void*)(FXival)id);
    }
  }

void GMDBTracks::init(GMTrackDatabase*db) {
  database=db;
  insert_track                        = database->compile("INSERT INTO tracks VALUES( NULL, " // id
                                                                           "1,"    // collection
                                                                           "?,"    // path
                                                                           "?,"    // mrl
                                                                           "?," // title
                                                                           "?," // time
                                                                           "?," // no
                                                                           "?," // year
                                                                           "?," // bitrate
                                                                           "?," // album
                                                                           "?," // artist
                                                                           "?," // composer
                                                                           "?," // conductor
                                                                           "0," // playcount
                                                                           "0," // playdate
                                                                           "?," // importdate
                                                                           "0," // rating
                                                                           "?," // samplerate
                                                                           "?," // channels
                                                                           "?);"); // filetype


  insert_tag                          = database->compile("INSERT INTO tags VALUES ( NULL , ?  );");
  insert_artist                       = database->compile("INSERT INTO artists VALUES ( NULL , ?  );");
  insert_album                        = database->compile("INSERT INTO albums VALUES (NULL, ?, ?, ?, ?, ?, ?);");


  insert_path                         = database->compile("INSERT OR IGNORE INTO pathlist VALUES (NULL,?);");
  insert_playlist_track               = database->compile("INSERT INTO playlist_tracks SELECT ?,"
                                                                                   "(SELECT id FROM tracks WHERE path == ? AND mrl == ?),"
                                                                                   "? ;");
  insert_playlist_track_by_id         = database->compile("INSERT INTO playlist_tracks VALUES (?,?,?);");
  insert_track_tag                    = database->compile("INSERT OR IGNORE INTO track_tags VALUES ( ? , ? );");

  update_track                        = database->compile("UPDATE tracks SET title = ?,"
                                                                  "time = ?,"
                                                                  "no = ?,"
                                                                  "year = ?,"
                                                                  "bitrate = ?,"
                                                                  "samplerate = ?,"
                                                                  "channels = ?,"
                                                                  "filetype = ?,"
                                                                  "album =  ?,"
                                                                  "artist = ?," // artist
                                                                  "composer = ?," // composer
                                                                  "conductor = ?," // conductor
                                                                  "importdate = ? WHERE id == ?;");



  query_album                         = database->compile("SELECT id FROM albums WHERE artist == ? AND name == ? AND audio_channels == ? AND audio_rate == ? AND audio_format == ?;");

  query_artist                        = database->compile("SELECT id FROM artists WHERE name == ?;");
  query_tag                           = database->compile("SELECT id FROM tags WHERE name == ?;");

  delete_track_playlists              = database->compile("DELETE FROM playlist_tracks WHERE track == ?;");
  delete_track_tags                   = database->compile("DELETE FROM track_tags WHERE track == ?;");
  delete_track                        = database->compile("DELETE FROM tracks WHERE id == ?;");

  initPathDict(database);
  }



FXint GMDBTracks::insertPath(const FXString & path) {
  //GM_TICKS_START();
  FXint pid = insert_path.insert(path);
  pathdict.insert(path.text(),(void*)(FXival)pid);
  //GM_TICKS_END();
  return pid;
  }

FXint GMDBTracks::hasPath(const FXString & path) {
//  GM_TICKS_START();
  FXint id = (FXint)(FXival)pathdict[path];
//  GM_TICKS_END();
  return id;
  }


FXint GMDBTracks::insertArtist(const FXString & artist){
  FXint id=0;
  query_artist.execute(artist,id);
  if (id==0) {
    id = insert_artist.insert(artist);
    }
  return id;
  }


void GMDBTracks::insertTags(FXint track,const FXStringList & tags){
  FXIntList ids(tags.no());

  for (int i=0;i<tags.no();i++) {
    ids[i]=0;
    query_tag.execute(tags[i],ids[i]);
    if (ids[i]==0)
      ids[i] = insert_tag.insert(tags[i]);
    }

  for (FXint i=0;i<ids.no();i++) {
    insert_track_tag.set(0,track);
    insert_track_tag.set(1,ids[i]);
    insert_track_tag.execute();
    }
  }

void GMDBTracks::updateTags(FXint track,const FXStringList & tags){
  /// Remove current tags
  delete_track_tags.update(track);

  /// Insert new tags
  if (tags.no())
    insertTags(track,tags);
  }


void GMDBTracks::add(const FXString & filename,const GMTrack & track,FXint & pid,FXint playlist,FXint queue){
//  GM_TICKS_START();

  FXASSERT(!track.album_artist.empty());

  FXint artist_id;
  FXint album_artist_id;
  FXint composer_id=0;
  FXint conductor_id=0;
  FXint album_id=0;
  FXint track_id;


  /// Path
  if (!pid) {
    pid = insertPath(FXPath::directory(track.url));
    FXASSERT(pid);
    if (!pid) fxwarning("pid==0 for %s\n",FXPath::directory(track.url).text());
    }

  /// Artist

  album_artist_id = insertArtist(track.album_artist);

  if (track.album_artist!=track.artist)
    artist_id = insertArtist(track.artist);
  else
    artist_id = album_artist_id;

  if (!track.composer.empty())
    composer_id=insertArtist(track.composer);

  if (!track.conductor.empty())
    conductor_id=insertArtist(track.conductor);

  query_album.set(0,album_artist_id);
  query_album.set(1,track.album);
  query_album.set(2,track.channels);
  query_album.set(3,track.samplerate);
  query_album.set(4,track.sampleformat);
  query_album.execute(album_id);

  if (!album_id) {
    insert_album.set(0,track.album);
    insert_album.set(1,album_artist_id);
    insert_album.set(2,track.year);
    insert_album.set(3,track.channels);
    insert_album.set(4,track.samplerate);
    insert_album.set(5,track.sampleformat);
    album_id = insert_album.insert();
    }

  FXASSERT(artist_id);
  FXASSERT(album_id);

  /// Insert Track
  insert_track.set(0,pid);
  insert_track.set(1,filename);
  insert_track.set(2,track.title);
  insert_track.set(3,track.time);
  insert_track.set(4,track.no);
  insert_track.set(5,track.year);
  insert_track.set(6,(track.sampleformat) ? -track.sampleformat : track.bitrate);
  insert_track.set(7,album_id);
  insert_track.set(8,artist_id);
  insert_track.set_null(9,composer_id);
  insert_track.set_null(10,conductor_id);
  insert_track.set(11,FXThread::time());
  insert_track.set(12,track.samplerate);
  insert_track.set(13,track.channels);
  insert_track.set(14,track.filetype);
  track_id = insert_track.insert();

  /// Add to playlist
  if (playlist) {
    insert_playlist_track_by_id.set(0,playlist);
    insert_playlist_track_by_id.set(1,track_id);
    insert_playlist_track_by_id.set(2,queue);
    insert_playlist_track_by_id.execute();
    }

  // Tags
  if (track.tags.no())
    insertTags(track_id,track.tags);
//  GM_TICKS_END();
  }


void GMDBTracks::add2playlist(FXint playlist,FXint track,FXint queue) {
  insert_playlist_track_by_id.set(0,playlist);
  insert_playlist_track_by_id.set(1,track);
  insert_playlist_track_by_id.set(2,queue);
  insert_playlist_track_by_id.execute();
  }

void GMDBTracks::update(FXint id,const GMTrack & track){
  /// Artist
  FXint composer_id     = 0;
  FXint conductor_id    = 0;
  FXint album_artist_id = insertArtist(track.album_artist);
  FXint artist_id       = album_artist_id;
  FXint album_id        = 0;

  if (track.album_artist!=track.artist)
    artist_id = insertArtist(track.artist);

  if (!track.composer.empty())  composer_id=insertArtist(track.composer);
  if (!track.conductor.empty()) conductor_id=insertArtist(track.conductor);

  /// Album
  query_album.set(0,album_artist_id);
  query_album.set(1,track.album);
  query_album.set(2,track.channels);
  query_album.set(3,track.samplerate);
  query_album.set(4,track.sampleformat);
  query_album.execute(album_id);

  if (!album_id) {
    insert_album.set(0,track.album);
    insert_album.set(1,album_artist_id);
    insert_album.set(2,track.year);
    insert_album.set(3,track.channels);
    insert_album.set(4,track.samplerate);
    insert_album.set(5,track.sampleformat);
    album_id = insert_album.insert();
    }

  /// Update Tracks
  update_track.set(0,track.title);
  update_track.set(1,track.time);
  update_track.set(2,track.no);
  update_track.set(3,track.year);
  update_track.set(4,(track.sampleformat) ? -track.sampleformat : track.bitrate);
  update_track.set(5,track.samplerate);
  update_track.set(6,track.channels);
  update_track.set(7,track.filetype);
  update_track.set(8,album_id);
  update_track.set(9,artist_id);
  update_track.set_null(10,composer_id);
  update_track.set_null(11,conductor_id);
  update_track.set(12,FXThread::time());
  update_track.set(13,id);
  update_track.execute();

  /// Update Tags
  updateTags(id,track.tags);
  }

void GMDBTracks::remove(FXint track) {
  delete_track_playlists.update(track);
  delete_track_tags.update(track);
  delete_track.update(track);
  }


GMImportTask::GMImportTask(FXObject *tgt,FXSelector sel) : GMTask(tgt,sel),database(NULL),playlist(0),queue(0),count(0) {
  database = GMPlayerManager::instance()->getTrackDatabase();
  }

GMImportTask::~GMImportTask() {
  }

void GMImportTask::fixEmptyTags(GMTrack & track,FXint n) {
  if (track.title.empty())
    track.title=options.default_field;

  if (track.album.empty())
    track.album=options.default_field;

  if (track.album_artist.empty()){
    if (track.artist.empty()) {
      track.album_artist=options.default_field;
      track.artist=options.default_field;
      }
    else {
      track.album_artist=track.artist;
      }
    }

  if (track.artist.empty())
    track.artist=track.album_artist;

  if (options.track_from_filelist && n>=0)
    track.no=n+1;
  }


void GMImportTask::parse(const FXString & filename,FXint n,GMTrack & info){
  info.url=filename;
  switch(options.parse_method) {
    case GMImportOptions::PARSE_TAG      : info.loadTag(filename);
                                           break;
    case GMImportOptions::PARSE_FILENAME : GMFilename::parse(info,options.filename_template,(options.replace_underscores ? (GMFilename::OVERWRITE|GMFilename::REPLACE_UNDERSCORE) : (GMFilename::OVERWRITE)));
                                           info.loadProperties(filename);
                                           break;
    case GMImportOptions::PARSE_BOTH     : info.loadTag(filename);
                                           if (info.title.empty() ||
                                               info.artist.empty() ||
                                               info.album.empty() ||
                                               info.album_artist.empty() //||
                                               /*info.genre.empty()*/ ) {
                                             GMFilename::parse(info,options.filename_template,(options.replace_underscores ? (GMFilename::REPLACE_UNDERSCORE) : (0)));
                                             }
                                           break;
    }
  fixEmptyTags(info,n);
  }

void GMImportTask::fixAlbumArtist(GMTrack * tracks,FXint no){
  FXString albumname;
  FXint i;

  FXbool all_albumartist=true;

  /// Tracks should all have the same album name
  for (i=1;i<no;i++){
    if (!tracks[i].url.empty()) {
      if (tracks[i].album!=tracks[0].album) {
        return;
        }
      if (tracks[i].album_artist!=tracks[0].album_artist) {
        all_albumartist=false;
        }
      }
    }

  /// All tracks share the same album artist... we're ok
  if (all_albumartist)
    return;

  /// All tracks should have album_artist==artist
  for (i=0;i<no;i++){
    if (!tracks[i].url.empty()) continue;
    if (tracks[i].album_artist!=tracks[i].artist) {
      return;
      }
    }

  /// Fixit
  for (i=0;i<no;i++){
    tracks[i].album_artist="Various";
    }
  }




void GMImportTask::import() {
  GM_TICKS_START();
  FXint tid,pid=0;
  FXString filename,pathname;
  GMTrack info;

  database->beginTask();


  if (playlist)
    queue = database->getNextQueue(playlist);


  taskmanager->setStatus("Importing...");

  for (FXint i=0;(i<files.no()) && processing;i++){

    if (database->interrupt){
      database->waitTask();
      queue = database->getNextQueue(playlist);
      }

    if (FXStat::isDirectory(files[i])){
      listDirectory(files[i]);
      }
    else if (FXStat::isFile(files[i])) {
      if (strstr(FILE_EXTENSIONS,FXPath::extension(files[i]).lower().text())) {
        filename = FXPath::name(files[i]);
        pathname = FXPath::directory(files[i]);
        if ( (pid=dbtracks.hasPath(pathname)) && (tid=database->hasTrack(filename,pid))) {
          if (playlist) dbtracks.add2playlist(playlist,tid,queue++);
          }
        else {
          if (FXStat::isReadable(files[i])) {
            parse(files[i],i,info);
            dbtracks.add(filename,info,pid,playlist,queue++);
            }
          }
        }
      }
    }
  database->sync_album_year();
  database->commitTask();
  GM_TICKS_END();
  }


void GMImportTask::listDirectory(const FXString & path) {
  GMTrackArray  tracks;
  FXIntList     tids;
  GMTrack       info;

  FXString * items=NULL;
  FXint pid,i;

  const FXuint matchflags=FXPath::PathName|FXPath::NoEscape|FXPath::CaseFold;



  FXint no = FXDir::listFiles(items,path,"*",FXDir::AllDirs|FXDir::NoParent|FXDir::NoFiles);
  if (no) {
    for (i=0;(i<no)&&processing;i++){
      if (!FXStat::isLink(path+PATHSEPSTRING+items[i]) && (options.exclude_folder.empty() || !FXPath::match(items[i],options.exclude_folder,matchflags)))
        listDirectory(path+PATHSEPSTRING+items[i]);
      }

    delete [] items;
    items=NULL;
    }

  if (!processing) return;

  no=FXDir::listFiles(items,path,FILE_PATTERNS,FXDir::NoDirs|FXDir::NoParent|FXDir::CaseFold);
  if (no) {
    pid=dbtracks.hasPath(path);
    tracks.no(no);
    tids.no(no);
    tids.assign(0,tids.no());

    for (i=0;i<no;i++){
      if (!options.exclude_file.empty() && FXPath::match(items[i],options.exclude_file,matchflags))
        continue;
      if (pid==0 || (tids[i]=database->hasTrack(items[i],pid))==0)
        parse(path+PATHSEPSTRING+items[i],i,tracks[i]);
      }


    //fixAlbumArtist(tracks,no);
    for (i=0;i<no;i++){

      if (database->interrupt)
        database->waitTask();

      if (tracks[i].url.empty()) {
        if (playlist && tids[i])
          dbtracks.add2playlist(playlist,tids[i],queue++);
        }
      else {
        dbtracks.add(items[i],tracks[i],pid,playlist,queue++);
        count++;
        if (0==(count%100)) {
          taskmanager->setStatus(FXString::value("Importing %d",count));
          }
        }
      }
    delete [] items;
    items=NULL;
    }
  }

FXint GMImportTask::run() {
  FXASSERT(database);
  try {
    dbtracks.init(database);

    // Set codec for id3v1
    if (options.id3v1_encoding!=GMFilename::ENCODING_8859_1) {
      const FXTextCodec * codec = ap_get_usercodec(options.id3v1_encoding);
      if (codec) {
        GMTag::setID3v1Encoding(codec);
        }
      }
    import();

    GMTag::setID3v1Encoding(NULL);
    }
  catch(GMDatabaseException&) {
    database->rollbackTask();
    return 1;
    }
  return 0;
  }



GMSyncTask::GMSyncTask(FXObject *tgt,FXSelector sel) : GMImportTask(tgt,sel),nchanged(0) {
  }

GMSyncTask::~GMSyncTask() {
  }

void GMSyncTask::syncDirectory(const FXString & path) {
  GMTrack info;
  GMTrackFilenameList list;
  FXStat stat;

  database->getTrackFilenames(list,path);

  FXint progress = -1;
  FXdouble fraction = 0;


  if (options_sync.remove_all) {
    for (FXint i=0;i<list.no() && processing ;i++){
      fraction = 100.0 * ((i+1) / (FXdouble)(list.no()));
      if (progress!=(FXint)fraction){
        progress=(FXint)fraction;
        taskmanager->setStatus(FXString::value("Syncing Files %d%%",progress));
        }

      if (database->interrupt)
        database->waitTask();

      dbtracks.remove(list[i].id);
      nchanged++;
      }
    }
  else if (options_sync.remove_missing) {
    for (FXint i=0;i<list.no() && processing ;i++){

      fraction = 100.0 * ((i+1) / (FXdouble)(list.no()));
      if (progress!=(FXint)fraction){
        progress=(FXint)fraction;
        taskmanager->setStatus(FXString::value("Syncing Files %d%%",progress));
        }

      if (database->interrupt)
        database->waitTask();

      if (!FXStat::statFile(list[i].filename,stat)){
        dbtracks.remove(list[i].id);
        nchanged++;
        }
      else if (options_sync.update && (options_sync.update_always || stat.modified() > list[i].date)) {
        parse(list[i].filename,-1,info);
        dbtracks.update(list[i].id,info);
        nchanged++;
        }
      }
    }
  else {
    for (FXint i=0;i<list.no() && processing ;i++){
      fraction = 100.0 * ((i+1) / (FXdouble)(list.no()));
      if (progress!=(FXint)fraction){
        progress=(FXint)fraction;
        taskmanager->setStatus(FXString::value("Syncing Files %d%%",progress));
        }

      if (database->interrupt)
        database->waitTask();

      if (FXStat::statFile(list[i].filename,stat) && options_sync.update && (options_sync.update_always || stat.modified() > list[i].date)) {
        parse(list[i].filename,-1,info);
        dbtracks.update(list[i].id,info);
        nchanged++;
        }
      }
    }
  }

void GMSyncTask::sync(){
  database->beginTask();
  if (files.no()) {
    taskmanager->setStatus("Syncing Files...");
    for (FXint i=0;(i<files.no()) && processing;i++){
      syncDirectory(files[i]);
      }
    if (nchanged) {
      database->sync_tracks_removed();
      database->sync_album_year();
      }
    }
  database->commitTask();
  }



FXint GMSyncTask::run() {
  FXASSERT(database);
  try {
    dbtracks.init(database);

    // Set codec for id3v1
    if (options.id3v1_encoding!=GMFilename::ENCODING_8859_1) {
      const FXTextCodec * codec = ap_get_usercodec(options.id3v1_encoding);
      if (codec) {
        GMTag::setID3v1Encoding(codec);
        }
      }

    sync();
    if (options_sync.import_new && !options_sync.remove_all)
      import();

    GMTag::setID3v1Encoding(NULL);
    }
  catch(GMDatabaseException&) {
    database->rollbackTask();
    return 1;
    }
  return 0;
  }
