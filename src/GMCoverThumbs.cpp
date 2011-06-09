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
#include "gmutils.h"
#include "GMTaskManager.h"
#include "GMTrack.h"
#include "GMApp.h"
#include "GMCover.h"
#include "GMTrackList.h"
#include "GMSource.h"
#include "GMTrackView.h"
#include "GMTrackDatabase.h"
#include "GMPlayerManager.h"
#include "GMIconTheme.h"
#include "GMCoverThumbs.h"

class UpdateThumbnails : public GMTask {
public:
  GMAlbumPathList list;
  GMCoverThumbs   thumbs;
protected:
  FXint run();
public:
  UpdateThumbnails(GMAlbumPathList & l,FXint sz,FXObject *tgt=NULL,FXSelector sel=0);
  };

class GenerateThumbnails : public GMTask {
public:
  GMAlbumPathList list;
  GMCoverThumbs   thumbs;
protected:
  FXint run();
public:
  GenerateThumbnails(GMAlbumPathList & l,FXint sz,FXObject *tgt=NULL,FXSelector sel=0);
  };

class GMImageMap {
public:
  FXImage * map;    // Main Image;
  FXint     size;   // Size of each image
  FXint     num;    // Number of subimages
  FXint     nused;  //
  FXlong    used;
protected:
  void append(FXImage * img) {
    FXASSERT(img->getWidth()<=size);
    FXASSERT(img->getHeight()<=size);

    FXuchar * dst = (FXuchar*)map->getData()+(4*(y(nused)*map->getWidth()+x(nused)));
    FXuchar * src = (FXuchar*)img->getData();
    FXint sw=img->getWidth()*4;
    FXint sh=img->getHeight();

    if (img->getHeight()<size)
      dst+=(size*num*4)*((size-img->getHeight())>>1);

    if (img->getWidth()<size)
      dst+=4*((size-img->getWidth())>>1);

    do {
      memcpy(dst,src,sw);
      dst+=size*num*4;
      src+=sw;
      }
    while(--sh);
    nused++;
    }
public:
  GMImageMap() : map(NULL), size(0), num(0), nused(0),used(0) {}

  GMImageMap(FXint sz,FXint n=8) : map(NULL), size(sz), num(n), nused(0) {
    FXASSERT(n>0 && n<=8);
    map=new FXJPGImage(FXApp::instance(),NULL,IMAGE_OWNED|IMAGE_SHMI|IMAGE_SHMP,(num*size),(num*size),75);
    map->fill(FXRGBA(255,255,255,0));
    }

  FXint insert(FXImage * img) {
    append(img);
    return nused-1;
    }

  FXID id() const { if (map) return map->id(); else return 0; }

  FXint x(FXint m) { return size*(m%num); }
  FXint y(FXint m) { return size*(m/num); }

  FXbool full() const { return (num*num==nused); }

  void create() { if (map) { map->blend(FXApp::instance()->getBackColor()); map->create(); }}

  void save(FXStream & store){
    store << size;
    store << num;
    store << nused;
    map->savePixels(store);
    }

  void load(FXStream & store){
    store >> size;
    store >> num;
    store >> nused;
    map = new FXJPGImage(FXApp::instance());
    map->loadPixels(store);
    map->setOptions(IMAGE_OWNED|IMAGE_SHMI|IMAGE_SHMP);
    }

  ~GMImageMap() {
    delete map;
    }
  };


void FXIntMap::save(FXStream & store) const {
  store << no();
  for (FXuint i=0;i<size();i++){
    if (!empty(i)) {
      store << key(i);
      store << value(i);
      }
    }
  }

void FXIntMap::load(FXStream & store) {
  FXint key,value,no;
  store >> no;
  for (FXint i=0;i<no;i++){
    store >> key;
    store >> value;
    insert(key,value);
    }
  }


FXDEFMAP(GMCoverThumbs) GMCoverThumbsMap[]={
  FXMAPFUNC(SEL_TASK_COMPLETED,GMCoverThumbs::ID_COVER_FETCHER,GMCoverThumbs::onCmdCoversFetched),
  };

FXIMPLEMENT(GMCoverThumbs,FXObject,GMCoverThumbsMap,ARRAYNUMBER(GMCoverThumbsMap));

GMCoverThumbs::GMCoverThumbs(FXint sz) : loaded(false),image_size(sz) {
  FXASSERT(image_size>0);
  }

GMCoverThumbs::~GMCoverThumbs() {
  for (FXint i=0;i<maps.no();i++){
    delete maps[i];
    }
  maps.clear();
  }

void GMCoverThumbs::init(GMTrackDatabase * database){
  if (!loaded && !load()) {
    refresh(database);
    }
  }

void GMCoverThumbs::refresh(GMTrackDatabase * database){
  GMAlbumPathList list;
  database->listAlbumPaths(list);
  if (list.no()){
    GenerateThumbnails * task = new GenerateThumbnails(list,image_size,this,ID_COVER_FETCHER);
    GMPlayerManager::instance()->runTask(task);
    }
  }

void GMCoverThumbs::adopt(GMCoverThumbs & src) {
  clear();

  maps.adopt(src.maps);
  gm_copy_hash(src.id2map,id2map);
  gm_copy_hash(src.id2album,id2album);

  src.maps.clear();
  src.id2map.clear();
  src.id2album.clear();

  loaded=true;
  }

void GMCoverThumbs::clear() {
  for (FXint i=0;i<maps.no();i++) {
    delete maps[i];
    }
  maps.clear();
  id2map.clear();
  id2album.clear();
  loaded=false;
  }


#define COVERTHUMBS_CACHE_FILE_VERSION 20110301

void GMCoverThumbs::save() const {
  const FXuint version=COVERTHUMBS_CACHE_FILE_VERSION;
  FXFileStream store;
  if (store.open(GMApp::getCacheDirectory()+PATHSEPSTRING+"albumcovers.cache",FXStreamSave)){
    store << version;
    store << image_size;
    id2map.save(store);
    id2album.save(store);
    store << (FXint)maps.no();
    for (FXint i=0;i<maps.no();i++){
      maps[i]->save(store);
      }
    }
  }

FXbool GMCoverThumbs::load() {
  GM_TICKS_START();
  FXFileStream store;
  loaded=false;
  if (store.open(GMApp::getCacheDirectory()+PATHSEPSTRING+"albumcovers.cache",FXStreamLoad)) {
    FXint no,imgsize;
    FXuint version;

    store >> version;
    if (version!=COVERTHUMBS_CACHE_FILE_VERSION)
      return false;

    store >> imgsize;
    if (imgsize!=image_size)
      return false;

    id2map.load(store);
    id2album.load(store);
    store >> no;
    for (FXint i=0;i<no;i++) {
      GMImageMap * map=new GMImageMap();
      map->load(store);
      maps.append(map);
      }
    loaded=true;
    }
  GM_TICKS_END();
  return loaded;
  }


void GMCoverThumbs::insert(FXint id,FXImage * img) {
  FXint m = id2map.find(id);
  if (m==0) {
    if (img==NULL) {
      id2map.insert(id,-1);
      id2album.insert(id,-1);
      }
    else {
      if (maps.no()==0 || maps[maps.no()-1]->full()) {
        maps.append(new GMImageMap(image_size,8));
        }
      FXint p = maps[maps.no()-1]->insert(img);
      id2map.insert(id,maps.no());
      id2album.insert(id,p);
      }
    }
  delete img;
  }


FXbool GMCoverThumbs::contains(FXint id) {
  FXint m = id2map.find(id);
  if (m!=0) return true;
  return false;
  }




void GMCoverThumbs::draw(FXDC & dc,FXint x,FXint y,FXint id) {
  FXint m = id2map.find(id);
  FXint a = id2album.find(id);
  if (m<=0) {
    FXIcon * ic = GMIconTheme::instance()->icon_nocover;
    if (ic->getHeight()<image_size)
      y+=(image_size-ic->getHeight())>>1;
    if (ic->getWidth()<image_size)
      x+=(image_size-ic->getWidth())>>1;
    dc.drawIcon(ic,x,y);
    }
  else {
    if (!maps[m-1]->id()) maps[m-1]->create();
    dc.drawArea(maps[m-1]->map,maps[m-1]->x(a),maps[m-1]->y(a),maps[m-1]->size,maps[m-1]->size,x,y);
    }
  }


long GMCoverThumbs::onCmdCoversFetched(FXObject*,FXSelector,void*ptr){
  GenerateThumbnails * task = reinterpret_cast<GenerateThumbnails*>(*((void**)ptr));
  adopt(task->thumbs);
  delete task;
  GMPlayerManager::instance()->getTrackView()->redrawAlbumList();
  return 1;
  }




GenerateThumbnails::GenerateThumbnails(GMAlbumPathList & albums,FXint sz,FXObject * tgt,FXSelector sel) : GMTask(tgt,sel), thumbs(sz) {
  list.adopt(albums);
  }

FXint GenerateThumbnails::run() {
  FXdouble fraction;
  GMCover * cover=NULL;
  fxmessage("Generating Thumbnails...\n");
  for (FXint i=0;i<list.no() && processing;i++){
    fraction = (i+1) / ((double)list.no());
    taskmanager->setStatus(FXString::value("Loading Covers %d%%",(FXint)(100.0*fraction)));
    cover = GMCover::fromTag(list[i].path,thumbs.size(),1);
    if (cover==NULL)
      cover = GMCover::fromPath(FXPath::directory(list[i].path),thumbs.size(),1);
    if (cover) {
      thumbs.insert(list[i].id,GMCover::toImage(cover));
      }
    else {
      thumbs.insert(list[i].id,NULL);
      }
    }
  if (processing) thumbs.save();
  fxmessage("Finished Generating Thumbnails!\n");
  return 0;
  }



UpdateThumbnails::UpdateThumbnails(GMAlbumPathList & albums,FXint sz,FXObject * tgt,FXSelector sel) : GMTask(tgt,sel), thumbs(sz) {
  list.adopt(albums);
  }

FXint UpdateThumbnails::run() {
  FXdouble fraction;
  GMCover * cover=NULL;
  fxmessage("Updating Thumbnails...\n");
  for (FXint i=0;i<list.no() && processing;i++){
    fraction = (i+1) / ((double)list.no());
    taskmanager->setStatus(FXString::value("Loading Covers %d%%",(FXint)(100.0*fraction)));

    cover = GMCover::fromTag(list[i].path,thumbs.size());
    if (cover==NULL)
      cover = GMCover::fromPath(FXPath::directory(list[i].path),thumbs.size());
    if (cover) {
      thumbs.insert(list[i].id,GMCover::toImage(cover));
      }
    else {
      thumbs.insert(list[i].id,NULL);
      }
    }
  if (processing) thumbs.save();
  fxmessage("Finished Generating Thumbnails!\n");
  return 0;
  }
