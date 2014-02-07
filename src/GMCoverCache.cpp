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
#include "GMCoverCache.h"


class GMCompressedImage {
public:
  FXuchar * buffer;
  FXuint    len;
public:
  GMCompressedImage() : buffer(NULL),len(0) {}
public:
  GMCompressedImage(const FXuchar * b,FXuint l) : buffer(NULL), len(l) {
    allocElms(buffer,len);
    memcpy(buffer,b,len);
    }

  void make(FXImage * img) {
    FXASSERT(buffer);
    FXASSERT(len);
    if (buffer && len) {
      FXint ww,hh,dd;
      FXColor * data=NULL;
      FXMemoryStream ms(FXStreamLoad,buffer,len,false);
      if (fxloadJPG(ms,data,ww,hh,dd)) {
        img->setData(data,IMAGE_OWNED);
        img->render();
        }
      ms.close();
      FXASSERT(ww==128);
      FXASSERT(hh==128);
      }
    }

  ~GMCompressedImage() {
    freeElms(buffer);
    }

  void load(FXStream & store) {
    store >> len;
    allocElms(buffer,len);
    store.load(buffer,len);
    }

  void save(FXStream & store) const {
    store << len;
    store.save(buffer,len);
    }

  };




class CoverLoader: public GMTask {
protected:
  FXuchar * compress_buffer;
  FXuval    compress_buffer_length;
public:
  GMAlbumPathList albums;
  GMCoverCache    cache;
protected:
  GMCompressedImage*  compress(FXColor*,FXint,FXint);
  GMCompressedImage*  compress(FXImage*);
  GMCompressedImage*  compress_fit(FXImage*);
  FXint run();
public:
  CoverLoader(GMAlbumPathList & l,FXint sz,FXObject *tgt=NULL,FXSelector sel=0);
  ~CoverLoader();
  };




CoverLoader::CoverLoader(GMAlbumPathList & list,FXint sz,FXObject * tgt,FXSelector sel)
  : GMTask(tgt,sel), cache(sz) {
  albums.adopt(list);
  compress_buffer=NULL;
  compress_buffer_length=0;
  }

CoverLoader::~CoverLoader(){
  freeElms(compress_buffer);
  }


FXint CoverLoader::run() {
  FXdouble fraction;
  GMCover * cover=NULL;
  for (FXint i=0;i<albums.no() && processing;i++){
    fraction = (i+1) / ((double)albums.no());
    taskmanager->setStatus(FXString::value("Loading Covers %d%%",(FXint)(100.0*fraction)));
    cover = GMCover::fromTag(albums[i].path);
    if (cover==NULL)
      cover = GMCover::fromPath(FXPath::directory(albums[i].path));

    cache.insertCover(albums[i].id,compress(GMCover::toImage(cover,cache.getCoverSize(),1)));
    }
  if (processing) cache.save();
  return 0;
  }


GMCompressedImage * CoverLoader::compress_fit(FXImage*img) {
  FXint size = cache.getCoverSize();

  FXColor * pix=NULL;
  allocElms(pix,size*size);


  memset(pix,255,4*size*size);

  FXuchar * dst = (FXuchar*)pix;
  FXuchar * src = (FXuchar*)img->getData();
  FXint sw=img->getWidth()*4;
  FXint sh=img->getHeight();


  if (img->getHeight()<size)
    dst+=(size*4)*((size-img->getHeight())>>1);

  if (img->getWidth()<cache.getCoverSize())
    dst+=4*((size-img->getWidth())>>1);

  do {
    memcpy(dst,src,sw);
    dst+=size*4;
    src+=sw;
    }
  while(--sh);

  GMCompressedImage * c = compress(pix,size,size);
  freeElms(pix);
  return c;
  }

GMCompressedImage * CoverLoader::compress(FXColor*pix,FXint width,FXint height){
  FXMemoryStream ms(FXStreamSave,compress_buffer,compress_buffer_length,true);
  fxsaveJPG(ms,pix,width,height,75);
  ms.takeBuffer(compress_buffer,compress_buffer_length);
  ms.close();
  return new GMCompressedImage(compress_buffer,ms.position());
  }


GMCompressedImage * CoverLoader::compress(FXImage * img) {
  if (img) {
    GMCompressedImage * cimage = NULL;
    if (img->getWidth()!=cache.getCoverSize() || img->getHeight()!=cache.getCoverSize()) {
      cimage = compress_fit(img);
      }
    else {
      cimage = compress(img->getData(),cache.getCoverSize(),cache.getCoverSize());
      }
    delete img;
    return cimage;
    }
  else {
    return NULL;
    }
  }



FXDEFMAP(GMCoverCache) GMCoverCacheMap[]={
  FXMAPFUNC(SEL_TASK_COMPLETED,GMCoverCache::ID_COVER_LOADER,GMCoverCache::onCmdCoversLoaded),
  };

FXIMPLEMENT(GMCoverCache,FXObject,GMCoverCacheMap,ARRAYNUMBER(GMCoverCacheMap));




GMCoverCache::GMCoverCache(FXint size) : basesize(size),initialized(false) {
  }


GMCoverCache::~GMCoverCache() {
  for (FXint i=0;i<covers.no();i++) {
    delete covers[i];
    }
  for (FXint i=0;i<buffers.no();i++) {
    delete buffers[i];
    }
  }

void GMCoverCache::clear() {
  for (FXint i=0;i<covers.no();i++) {
    delete covers[i];
    }
  covers.clear();
  }


FXString GMCoverCache::getCacheFile() const {
  return GMApp::getCacheDirectory()+PATHSEPSTRING+"albumcovers.cache";
  }

void GMCoverCache::adopt(GMCoverCache & src) {

  // To force reloading of cover art, set user data to 0
  // We can't use reset() here since cover art will be reused in the next onPaint
  // by the calls reset / markCover.
  for (FXint i=0;i<buffers.no();i++){
    buffers[i]->setUserData((void*)(FXival)0);
    }

  // Clear Covers
  clear();

  // Adopt Covers
  covers.adopt(src.covers);

  // Adopt Map
  map.adopt(src.map);
  }



void GMCoverCache::drawCover(FXint id,FXDC & dc,FXint x,FXint y){
  FXint index = map.find(id);
  if (index>0) {
    FXImage * image = getCoverImage(id);
    dc.drawImage(image,x,y);
    return;
    }
  else {
    FXIcon * ic = GMIconTheme::instance()->icon_nocover;
    if (ic->getHeight()<basesize)
      y+=(basesize-ic->getHeight())>>1;
    if (ic->getWidth()<basesize)
      x+=(basesize-ic->getWidth())>>1;
    dc.drawIcon(ic,x,y);
    }
  }


void GMCoverCache::insertCover(FXint id,GMCompressedImage * image) {
  if (image) {
    covers.append(image);
    map.insert(id,covers.no());
    }
  }

void GMCoverCache::markCover(FXint id) {
  for (FXint i=0,index;i<buffers.no();i++){
    index=(FXint)(FXival)buffers[i]->getUserData();
    if (index==-id) {
      buffers[i]->setUserData((void*)(FXival)id);
      break;
      }
    }
  }


void GMCoverCache::reset() {
  for (FXint i=0,index;i<buffers.no();i++){
    index=(FXint)(FXival)buffers[i]->getUserData();
    if (index>0) buffers[i]->setUserData((void*)(FXival)(-index));
    }
  }

FXImage* GMCoverCache::getCoverImage(FXint id) {
  FXint i,index;
  FXImage * image=NULL;

  /// existing
  for (i=0;i<buffers.no();i++){
    index=(FXint)(FXival)buffers[i]->getUserData();
    if (index==id) return buffers[i];
    }

  /// find empty
  for (i=0;i<buffers.no();i++){
    index=(FXint)(FXival)buffers[i]->getUserData();
    if (index<=0) {
      buffers[i]->setUserData((void*)(FXival)id);
      image=buffers[i];
      break;
      }
    }

  /// Create new one
  if (image==NULL) {
    image = new FXImage(FXApp::instance(),NULL,0,basesize,basesize);
    image->setUserData((void*)(FXival)id);
    image->create();
    buffers.append(image);
    }

  index = map.find(id);
  covers[index-1]->make(image);
  return image;
  }


void GMCoverCache::init(GMTrackDatabase * database){
  if (!initialized && !load()) {
    refresh(database);
    }
  }

void GMCoverCache::refresh(GMTrackDatabase * database){

  /// Remove the cache file.
  if (FXStat::exists(getCacheFile()))
    FXFile::remove(getCacheFile());

  /// Only scan for covers if needed
  if (initialized) {
    GMAlbumPathList list;
    database->listAlbumPaths(list);
    if (list.no()){
      CoverLoader * task = new CoverLoader(list,basesize,this,ID_COVER_LOADER);
      GMPlayerManager::instance()->runTask(task);
      }
    }
  }

#define COVERTHUMBS_CACHE_FILE_VERSION 20120824

void GMCoverCache::save() const {
  const FXuint version=COVERTHUMBS_CACHE_FILE_VERSION;
  FXFileStream store;
  if (store.open(getCacheFile(),FXStreamSave)){
    store << version;
    store << basesize;
    map.save(store);
    FXint n = covers.no();
    store << n;
    for (FXint i=0;i<covers.no();i++){
      covers[i]->save(store);
      }
    }
  }

FXbool GMCoverCache::load() {
  GM_TICKS_START();
  initialized=true;
  FXFileStream store;
  if (store.open(getCacheFile(),FXStreamLoad)) {
    FXint no,size;
    FXuint version;

    store >> version;
    if (version!=COVERTHUMBS_CACHE_FILE_VERSION)
      return false;

    store >> size;
    if (basesize!=size)
      return false;

    map.load(store);
    store >> no;
    for (FXint i=0;i<no;i++) {
      GMCompressedImage * cover=new GMCompressedImage();
      cover->load(store);
      covers.append(cover);
      }
    return true;
    }
  GM_TICKS_END();
  return false;
  }


long GMCoverCache::onCmdCoversLoaded(FXObject*,FXSelector,void*ptr){
  CoverLoader * task = reinterpret_cast<CoverLoader*>(*((void**)ptr));
  adopt(task->cache);
  delete task;
  GMPlayerManager::instance()->getTrackView()->redrawAlbumList();
  return 1;
  }


