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
#include "GMLyrics.h"

// ALL patterns
//#define FILE_EXTENSIONS "ogg,flac,opus,oga,mp3,m4a,mp4,m4p,m4b,aac,mpc,wma,asf"
//#define FILE_PATTERNS "*.(" FILE_EXTENSIONS ")"

// Just the ones we can playback at the moment.
#define FILE_EXTENSIONS "ogg,flac,opus,oga,mp3,m4a,mp4,m4p,m4b,aac"
#define FILE_PATTERNS "*.(" FILE_EXTENSIONS ")"

const FXuint matchflags = FXPath::PathName|FXPath::NoEscape|FXPath::CaseFold;


GMDBTracks::GMDBTracks() {
  default_artist = fxtr("Unknown Artist");
  default_album = fxtr("Unknown Album");
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

void GMDBTracks::init(GMTrackDatabase*db,FXbool afg) {
  album_format_grouping = afg;
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
                                                                           "?,"  // filetype
                                                                           "?);"); // lyrics


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
                                                                  "artist = ?,"
                                                                  "composer = ?,"
                                                                  "conductor = ?,"
                                                                  "lyrics = ?,"
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
  FXint pid = (FXint)(FXival)pathdict[path];
  if (pid==0) {
    pid = insert_path.insert(path);
    pathdict.insert(path.text(),(void*)(FXival)pid);
    }
  return pid;
  }


FXint GMDBTracks::hasPath(const FXString & path) {
  return (FXint)(FXival)pathdict[path];
  }


FXint GMDBTracks::insertArtist(const FXString & artist){
  FXint id=0;
  if (!artist.empty()) {
    query_artist.execute(artist,id);
    if (id==0) {
      id = insert_artist.insert(artist);
      }
    }
  return id;
  }


FXint GMDBTracks::insertAlbum(const GMTrack & track,FXint album_artist_id) {
  FXint id=0;
  query_album.set(0,album_artist_id);
  query_album.set(1,track.album.empty() ? default_album : track.album);
  if (album_format_grouping) {
    query_album.set(2,track.channels);
    query_album.set(3,track.samplerate);
    query_album.set(4,track.sampleformat);
    }
  else {
    query_album.set(2,0);
    query_album.set(3,0);
    query_album.set(4,0);
    }
  query_album.execute(id);
  if (id==0) {
    insert_album.set(0,track.album.empty() ? default_album : track.album);
    insert_album.set(1,album_artist_id);
    insert_album.set(2,track.year);
    if (album_format_grouping) {
      insert_album.set(3,track.channels);
      insert_album.set(4,track.samplerate);
      insert_album.set(5,track.sampleformat);
      }
    else {
      insert_album.set(3,0);
      insert_album.set(4,0);
      insert_album.set(5,0);
      }
    id = insert_album.insert();
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


void GMDBTracks::insert(GMTrack & track) {
  FXint path_index = 0;
  insert(track,path_index);
  }

void GMDBTracks::insert(GMTrack & track,FXint & path_index) {
  if (__likely(track.index==0)) {

    FXASSERT(!track.url.empty());
    FXASSERT(path_index>=0);

    if (path_index==0) {
      path_index = insertPath(FXPath::directory(track.url));
      FXASSERT(path_index);
      if (!path_index) fxwarning("pid==0 for %s\n",FXPath::directory(track.url).text());
      }

    /// Artist
    FXint album_artist_id = insertArtist(track.getAlbumArtist(default_artist));
    FXint artist_id       = insertArtist(track.getArtist(default_artist));
    FXint composer_id     = insertArtist(track.composer);
    FXint conductor_id    = insertArtist(track.conductor);
    FXint album_id        = insertAlbum(track,album_artist_id);

    FXASSERT(artist_id);
    FXASSERT(album_id);

    /// Insert Track
    insert_track.set(0,path_index);
    insert_track.set(1,path_index ? FXPath::name(track.url) : track.url);
    insert_track.set(2,track.title.empty() ? FXPath::title(track.url) : track.title);
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
    insert_track.set(15,track.lyrics);

    track.index = insert_track.insert();

    /// Tags
    if (track.tags.no())
      insertTags(track.index,track.tags);
    }

  /// Add to playlist
  if (playlist) {
    insert_playlist_track_by_id.set(0,playlist);
    insert_playlist_track_by_id.set(1,track.index);
    insert_playlist_track_by_id.set(2,playlist_queue);
    insert_playlist_track_by_id.execute();
    }
  }

void GMDBTracks::update(GMTrack & track) {

  /// Artist
  FXint album_artist_id = insertArtist(track.getAlbumArtist(default_artist));
  FXint artist_id       = insertArtist(track.getArtist(default_artist));
  FXint composer_id     = insertArtist(track.composer);
  FXint conductor_id    = insertArtist(track.conductor);
  FXint album_id        = insertAlbum(track,album_artist_id);

  /// Update Tracks
  update_track.set(0,track.title.empty() ? FXPath::title(track.url) : track.title);
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
  update_track.set(12,track.lyrics);
  update_track.set(13,FXThread::time());
  update_track.set(14,track.index);
  update_track.execute();

  /// Update Tags
  updateTags(track.index,track.tags);
  }


void GMDBTracks::remove(FXint track) {
  delete_track_playlists.update(track);
  delete_track_tags.update(track);
  delete_track.update(track);
  }




GMImportTask::GMImportTask(FXObject *tgt,FXSelector sel) : GMTask(tgt,sel) {
  database = GMPlayerManager::instance()->getTrackDatabase();
  pattern = FILE_PATTERNS;
  }


void GMImportTask::setOptions(const GMImportOptions & o) {
  options=o;
  if (options.fetch_lyrics) {
    if (lyrics==nullptr)
      lyrics = new Lyrics();
    }
  else {
    if (lyrics) {
      delete lyrics;
      lyrics=nullptr;
      }
    }
  if (options.playback_only) {
    pattern = ap_get_gogglesmm_supported_files();
    }
  }


GMImportTask::~GMImportTask() {
  delete lyrics;
  }


FXint GMImportTask::run() {
  FXASSERT(database);
  try {

    GM_DEBUG_PRINT("Using file pattern: %s\n",pattern.text());

    // Make sure we have something to do
    if (pattern.empty()) return 0;

    dbtracks.init(database,options.album_format_grouping);

    setID3v1Encoding();

    import();

    GMTag::setID3v1Encoding(nullptr);
    }
  catch(GMDatabaseException&) {
    database->rollbackTask();
    return 1;
    }
  return 0;
  }


void GMImportTask::load_track(const FXString & filename) {
  GMTrack & track = tracks[ntracks];
  track.url = filename;
  switch(options.parse_method) {
    case GMImportOptions::PARSE_TAG:
      track.loadTag(track.url);
      break;
    case GMImportOptions::PARSE_FILENAME:
      GMFilename::parse(track,options.filename_template,(options.replace_underscores ? (GMFilename::OVERWRITE|GMFilename::REPLACE_UNDERSCORE) : (GMFilename::OVERWRITE)));
      break;
    case GMImportOptions::PARSE_BOTH:
      track.loadTag(track.url);
      if (track.hasMissingData()) {
        GMFilename::parse(track,options.filename_template,(options.replace_underscores ? (GMFilename::REPLACE_UNDERSCORE) : 0));
        }
      break;
    }

  if (lyrics && track.lyrics.empty()) {
    lyrics->fetch(track);
    }
  }


struct Seen {
  Seen * next;
  FXlong node;
  };


void GMImportTask::import() {
  FXStat   data;
  FXString name;
  FXint    dircount=0;


  database->beginTask();

  taskmanager->setStatus("Importing...");

  if (dbtracks.playlist)
    dbtracks.playlist_queue = database->getNextQueue(dbtracks.playlist);

  // Run through all directories
  for (FXint i=0;i<files.no() && processing;i++) {
    if (FXStat::statLink(files[i],data)) {
      if (data.isDirectory()) {
        if(!(name[0]=='.' && (name[1]==0 || (name[1]=='.' && name[2]==0)))) {
          if(files[i].tail()==PATHSEP)
            scan(files[i].rafter(PATHSEP),nullptr,data.index());
          else
            scan(files[i],nullptr,data.index());
          }
        dircount++;
        }
      }
    }

  // Scan remaining files
  if (dircount<files.no()) {
    for (FXint i=0;i<files.no() && processing;i++) {
      if (FXStat::statFile(files[i],data)) {
        if (!data.isDirectory() && data.isReadable()) {
          const FXString path = FXPath::directory(files[i]);
          const FXString name = FXPath::name(files[i]);
          if (FXPath::match(name,pattern,matchflags)){
            FXint path_index = dbtracks.hasPath(path);
            parse(path,name,path_index);
            }
          }
        }
      }
    import_tracks();
    }
  database->sync_album_year();
  database->commitTask();
  }


void GMImportTask::scan(const FXString & path,Seen * seen,FXlong index) {
  FXDir    directory;
  FXStat   data;
  FXString name;

  // Location
  Seen here={seen,index};

  // First scan subfolders
  if (directory.open(path)) {
    while(directory.next(name) && processing) {
      if (FXStat::statLink(path+PATHSEPSTRING+name,data)) {
        if (data.isDirectory()) {
          if(!(name[0]=='.' && (name[1]==0 || (name[1]=='.' && name[2]==0)))){
            for(Seen *s=seen; s; s=s->next){
              if(data.index()==s->node) goto next_dir;
              }
            if (options.exclude_folder.empty() || !FXPath::match(name,options.exclude_folder,matchflags))
              scan(path+PATHSEPSTRING+name,&here,data.index());
            }
          }
        }
next_dir:;
      }
    directory.close();
    }

  // Scan files in current folder
  if(directory.open(path) && processing) {
    FXint path_index = dbtracks.hasPath(path);
    while(directory.next(name) && processing) {
      if (FXStat::statFile(path+PATHSEPSTRING+name,data)) {
        if (!data.isDirectory() && data.isReadable()) {
          if (options.exclude_file.empty() || !FXPath::match(name,options.exclude_file,matchflags)) {
            if (FXPath::match(name,pattern,matchflags)) {
              parse(path,name,path_index);
              }
            }
          }
        }
      }
    if (processing) import_tracks(path_index);
    }
  }


void GMImportTask::parse(const FXString & path,const FXString & filename,FXint path_index) {

  if(ntracks>=tracks.no()) {
    tracks.no(tracks.no()+25);
    }

  GMTrack & track = tracks[ntracks];

  if(path_index) {
    FXTime modified;
    track.index = database->hasTrack(filename,path_index,modified);
    }
  else {
    track.index = 0;
    }

  if(track.index==0) {
    load_track(path+PATHSEPSTRING+filename);
    ntracks++;
    }
  else {
    if (dbtracks.playlist) ntracks++;
    track.url.clear();
    }
  }


FXbool GMImportTask::has_same_composer() const {
  if (ntracks==0)
    return false;

  if (tracks[0].composer.empty())
    return false;

  for (FXint i=1;i<ntracks;i++) {
    if (tracks[0].composer!=tracks[i].composer)
      return false;
    }

  return true;
  }


void GMImportTask::detect_compilation() {
  if (ntracks) {
    FXbool same_artists=true;

    // Up to 10 discs and 255 tracks
    FXuchar position[10][255]={{0}};

    for (FXint i=0;i<ntracks;i++) {

      // Check to make sure album_artist isn't explicity set.
      // url is empty if loaded from database.
      if (!tracks[i].album_artist.empty() && tracks[i].url.empty()) {
        GM_DEBUG_PRINT("detect_compilation: album_artist explicitely set.\n");
        return;
        }

      const FXushort & disc = tracks[i].getDiscNumber();
      const FXushort & no   = tracks[i].getTrackNumber();

      // Check for duplicate track/disc numbers
      if (disc<10 && no>0 && no<256) {
        if (position[disc][no]>0) {
          GM_DEBUG_PRINT("detect_compilation: found multiple albums (track number already in use)\n");
          return;
          }
        position[disc][no]=1;
        }
      }

    // Check if albums are all the same and use the same artist
    for (FXint i=1;i<ntracks;i++) {

      if (tracks[0].album!=tracks[i].album) {
        GM_DEBUG_PRINT("detect_compilation: found multiple albums\n");
        return;
        }

      if (same_artists && tracks[0].getAlbumArtist(dbtracks.default_artist)!=tracks[i].getAlbumArtist(dbtracks.default_artist))
        same_artists=false;
      }

    // Bail out if we already use the same artist
    if (same_artists) {
      GM_DEBUG_PRINT("detect_compilation: same artist on all tracks\n");
      return;
      }

    // Compilation Album detected
    GM_DEBUG_PRINT("detect_compilation: found compilation album\n");

    // Default value for album_artist to use
    FXString album_artist_default = fxtr("Various Artists");

    // Check if composer is set and are all the same
    if (has_same_composer())
      album_artist_default = tracks[0].composer;

    // Use artist_album from database in case user already overwrote Various Artists
    for (FXint i=0;i<ntracks;i++) {
      if (tracks[i].url.empty()) {
        GM_DEBUG_PRINT("detect_compilation: using album_artist from database \"%s\"\n",tracks[i].album_artist.text());
        album_artist_default = tracks[i].album_artist;
        break;
        }
      }

    // Update Album Artist
    for (FXint i=0;i<ntracks;i++) {
      tracks[i].album_artist = album_artist_default;
      }

    }
  }


void GMImportTask::import_tracks(FXint path_index) {
  if (ntracks) {

    // Set track number based on import order
    if (options.track_from_filelist) {
      for (FXint i=0;i<ntracks;i++) {
        tracks[i].no=i+1;
        }
      }

    // Look for compilations
    if (options.detect_compilation)
      detect_compilation();

    // Store tracks into database
    for (FXint i=0;i<ntracks;i++) {

      // Check for interrupts
      if (database->interrupt) {
        database->waitTask();
        dbtracks.playlist_queue = database->getNextQueue(dbtracks.playlist);
        }

      // Insert Track
      if (path_index>=0)
        dbtracks.insert(tracks[i],path_index);
      else
        dbtracks.insert(tracks[i]);

      // Update Progress
      count++;
      if (0==(count%100)) {
        taskmanager->setStatus(FXString::value("Importing %d",count));
        }
      }
    ntracks=0;
    }
  }



void GMImportTask::setID3v1Encoding() {
  if (options.id3v1_encoding!=GMFilename::ENCODING_8859_1) {
    const FXTextCodec * codec = ap_get_usercodec(options.id3v1_encoding);
    if (codec) {
      GMTag::setID3v1Encoding(codec);
      }
    }
  }





static FXbool filter_path(const FXString & exclude_folder,FXString path) {
  const FXuint matchflags=FXPath::PathName|FXPath::NoEscape|FXPath::CaseFold;
  while(!FXPath::isTopDirectory(path)){
    if (FXPath::match(FXPath::name(path),exclude_folder,matchflags))
      return true;
    path=FXPath::upLevel(path);
    }
  return false;
  }


GMSyncTask::GMSyncTask(FXObject *tgt,FXSelector sel) : GMImportTask(tgt,sel) {
  }


GMSyncTask::~GMSyncTask() {
  }


FXint GMSyncTask::run() {
  FXASSERT(database);
  try {

    GM_DEBUG_PRINT("Using file pattern: %s\n",pattern.text());

    dbtracks.init(database,options.album_format_grouping);

    setID3v1Encoding();

    if (options_sync.import_new) {

      if (options_sync.update)
        import_and_update();
      else
        import();

      if (options_sync.remove_missing || !options.exclude_folder.empty() || !options.exclude_file.empty())
        remove_missing();
      }
    else if (options_sync.update) {
      update();
      }
    else if (options_sync.remove_missing) {
      remove_missing();
      }

    GMTag::setID3v1Encoding(nullptr);
    }
  catch(GMDatabaseException&) {
    database->rollbackTask();
    return 1;
    }
  return 0;
  }


void GMSyncTask::import_and_update(){
  FXStat   data;
  FXString name;

  database->beginTask();

  taskmanager->setStatus("Syncing Files..");

  for (FXint i=0;i<files.no() && processing;i++) {
    if (FXStat::statLink(files[i],data)) {
      if (data.isDirectory()) {
        if(!(name[0]=='.' && (name[1]==0 || (name[1]=='.' && name[2]==0)))) {
          if(files[i].tail()==PATHSEP)
            traverse(files[i].rafter(PATHSEP),nullptr,data.index());
          else
            traverse(files[i],nullptr,data.index());
          }
        }
      }
    }
  if (changed) {
    database->sync_album_year();
    database->sync_tracks_removed();
    }
  database->commitTask();
  }


void GMSyncTask::update() {
  GMTrackFilenameList tracklist;
  FXStringList        pathlist;
  FXStat              data;

  database->beginTask();

  taskmanager->setStatus("Updating Files..");

  for (FXint i=0;i<files.no() && processing;i++) {

    database->getPathList(files[i],pathlist);

    for (FXint p=0;p<pathlist.no() && processing;p++) {

      const FXString & path  = pathlist[p];

      const FXint path_index = dbtracks.hasPath(path);

      database->getFileList(path,tracklist);

      if (!options.exclude_folder.empty() && filter_path(options.exclude_folder,path)) {

        for (FXint t=0;t<tracklist.no();t++) {

          if (database->interrupt)
            database->waitTask();

          dbtracks.remove(tracklist[t].id);
          }

        changed=true;
        continue;
        }

      for (FXint t=0;t<tracklist.no() && processing;t++) {

        const FXString & name = tracklist[t].filename;

        if (database->interrupt)
          database->waitTask();

        if (options.exclude_file.empty() || !FXPath::match(name,options.exclude_file,matchflags)) {
          if (FXStat::statFile(path+PATHSEPSTRING+name,data)) {
            if (data.isReadable())
              parse_update(path,name,options_sync.update_always ? forever : data.modified(),path_index);
            else
              parse_update(path,name,0,path_index);
            }
          else if (options_sync.remove_missing) {
            dbtracks.remove(tracklist[t].id);
            changed=true;
            }
          }
        else {
          dbtracks.remove(tracklist[t].id);
          changed=true;
          }
        }
      if (processing) update_tracks(path_index);
      }
    }
  if (changed) {
    database->sync_album_year();
    database->sync_tracks_removed();
    }
  database->commitTask();
  }




void GMSyncTask::remove_missing() {
  FXStringList        pathlist;
  GMTrackFilenameList tracklist;

  database->beginTask();

  taskmanager->setStatus("Clearing Files..");

  for (FXint i=0;i<files.no() && processing;i++) {

    database->getPathList(files[i],pathlist);

    for (FXint p=0;p<pathlist.no() && processing;p++) {

      database->getFileList(pathlist[p],tracklist);

      if (!options.exclude_folder.empty() && filter_path(options.exclude_folder,pathlist[p])){
        for (FXint t=0;t<tracklist.no() && processing;t++) {

          if (database->interrupt)
            database->waitTask();

          dbtracks.remove(tracklist[t].id);
          }
        changed=true;
        continue;
        }

      for (FXint t=0;t<tracklist.no() && processing;t++) {

        const FXString & name = tracklist[t].filename;

        if (database->interrupt)
          database->waitTask();

        if ((options_sync.remove_missing && !FXStat::exists(pathlist[p]+PATHSEPSTRING+name)) ||
            (!options.exclude_file.empty() && FXPath::match(name,options.exclude_file,matchflags))){
          dbtracks.remove(tracklist[t].id);
          changed=true;
          }
        }
      }
    }
  if (changed) database->sync_tracks_removed();
  database->commitTask();
  }


void GMSyncTask::parse_update(const FXString & path,const FXString & filename,FXTime modified,FXint pathindex) {
  FXTime tracktime=0;

  // Allocate tracks
  if (ntracks>=tracks.no()) {
    tracks.no(tracks.no()+25);
    }

  // Current Track
  GMTrack & track = tracks[ntracks];

  // Check existing
  if (pathindex) {
    track.index = database->hasTrack(filename,pathindex,tracktime);
    }
  else {
    track.index = 0;
    }

  // Load Track if new or updated
  if (track.index==0 || modified>tracktime) {
    load_track(path+PATHSEPSTRING+filename);
    }
  else {
    track.url.clear();
    }

  ntracks++;
  }


void GMSyncTask::traverse(const FXString & path,Seen * seen,FXlong index) {
  FXDir    directory;
  FXStat   data;
  FXString name;

  // Location
  Seen here={seen,index};

  // First scan subfolders
  if (directory.open(path)) {
    while(directory.next(name) && processing) {
      if (FXStat::statLink(path+PATHSEPSTRING+name,data)) {
        if (data.isDirectory()) {
          if(!(name[0]=='.' && (name[1]==0 || (name[1]=='.' && name[2]==0)))){
            for(Seen *s=seen; s; s=s->next){
              if(data.index()==s->node) goto next_dir;
              }
            if (options.exclude_folder.empty() || !FXPath::match(name,options.exclude_folder,matchflags))
              traverse(path+PATHSEPSTRING+name,&here,data.index());
            }
          }
        }
next_dir:;
      }
    directory.close();
    }

  // Scan files in current folder
  if(directory.open(path) && processing) {
    FXint path_index = dbtracks.hasPath(path);
    while(directory.next(name) && processing) {
      if (FXStat::statFile(path+PATHSEPSTRING+name,data)) {
        if (!data.isDirectory() && data.isReadable()) {
          if (options.exclude_file.empty() || !FXPath::match(name,options.exclude_file,matchflags)) {
            if (FXPath::match(name,pattern,matchflags)) {
              parse_update(path,name,options_sync.update_always ? forever : data.modified(),path_index);
              }
            }
          }
        }
      }
    if (processing) update_tracks(path_index);
    }
  }


void GMSyncTask::update_tracks(FXint pathindex) {
  FXint nchanged=0;

  for (FXint i=0;i<ntracks;i++) {
    if (!tracks[i].url.empty()) nchanged++;
    }

  if (nchanged) {

    // Load existing tracks from database
    for (FXint i=0;i<ntracks;i++) {
      if (tracks[i].url.empty()) {
        database->getTrack(tracks[i].index,tracks[i]);
        tracks[i].url.clear();
        }
      }

    // Set track number based on import order
    if (options.track_from_filelist) {
      for (FXint i=0;i<ntracks;i++) {
        tracks[i].no=i+1;
        }
      }

    // Look for compilations
    if (options.detect_compilation)
      detect_compilation();


    // Update Database
    for (FXint i=0;i<ntracks;i++) {

      // Check for interrupts
      if (database->interrupt) {
        database->waitTask();
        }

      // Update or Insert
      if (tracks[i].index){
        dbtracks.update(tracks[i]);
        }
      else {
        dbtracks.insert(tracks[i],pathindex);
        }

      // Update Progress
      count++;
      if (0==(count%100)) {
        taskmanager->setStatus(FXString::value("Syncing %d",count));
        }
      }
    changed=true;
    }
  ntracks=0;
  }




GMRemoveTask::GMRemoveTask(FXObject *tgt,FXSelector sel) : GMTask(tgt,sel) {
  database = GMPlayerManager::instance()->getTrackDatabase();
  }


GMRemoveTask::~GMRemoveTask() {
  }


FXint GMRemoveTask::run() {
  FXASSERT(database);
  try {
    dbtracks.init(database);
    remove();
    }
  catch(GMDatabaseException&) {
    database->rollbackTask();
    return 1;
    }
  return 0;
  }


void GMRemoveTask::remove() {
  FXStringList        pathlist;
  GMTrackFilenameList tracklist;

  database->beginTask();

  taskmanager->setStatus("Clearing Files..");

  for (FXint i=0;i<files.no() && processing;i++) {

    database->getPathList(files[i],pathlist);

    for (FXint p=0;p<pathlist.no() && processing;p++) {

      database->getFileList(pathlist[p],tracklist);

      for (FXint t=0;t<tracklist.no() && processing;t++) {

        if (database->interrupt)
          database->waitTask();

        dbtracks.remove(tracklist[t].id);
        changed=true;
        }
      }
    }
  if (changed) database->sync_tracks_removed();
  database->commitTask();
  }

