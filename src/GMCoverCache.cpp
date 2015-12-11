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
#include "GMCoverLoader.h"

#define COVERCACHE_FILE_VERSION 20140400
#define COVERCACHE_JPG  1
#define COVERCACHE_WEBP 2
#define COVERCACHE_PNG  3
#define COVERCACHE_BMP  4

void GMCacheInfo::adopt(GMCacheInfo & info) {
  index.adopt(info.index);
  map.adopt(info.map);
  size=info.size;
  format=info.format;
  }

void GMCacheInfo::insert(FXint id,FXlong position,FXint length) {
  index.append(FileIndex(position,length));
  map.insert(id,index.no());
  }

void GMCacheInfo::clear(FXint sz){
  index.clear();
  map.clear();
  size=sz;
  format=0;
  }

void GMCacheInfo::save(FXStream & store) const {
  map.save(store);
  for (FXint i=0;i<index.no();i++) {
    store << index[i].position;
    store << index[i].length;
    }
  FXint n = index.no();
  store << n;
  }

void GMCacheInfo::load(FXStream & store) {
  FXint n;

  // read number of covers from the end of file
  store.position(-4,FXFromEnd);
  store >> n;

  // load map and index
  store.position(-((20*n)+8),FXFromEnd);
  map.load(store);
  index.no(n);
  for (FXint i=0;i<n;i++) {
    store >> index[i].position;
    store >> index[i].length;
    }
  }


GMCoverCacheWriter::GMCoverCacheWriter(FXint sz) : info(sz),pixels(NULL) {
  if (FXWEBPImage::supported)
    info.format = COVERCACHE_WEBP;
  else if (FXJPGImage::supported)
    info.format = COVERCACHE_JPG;
  else if (FXPNGImage::supported)
    info.format = COVERCACHE_PNG;
  else
    info.format = COVERCACHE_BMP; // Getting real desperate here...
  }

GMCoverCacheWriter::~GMCoverCacheWriter() {
  freeElms(pixels);
  }


FXbool GMCoverCacheWriter::open(const FXString & filename) {
  const FXuint version = COVERCACHE_FILE_VERSION;
  if ((info.format>0) && store.open(filename,FXStreamSave)) {
    store << version;
    store << info.size;
    store << info.format;
    return true;
    }
  return false;
  }


FXlong GMCoverCacheWriter::save(FXColor * buffer){
  FXlong offset = store.position();
  switch(info.format){
    case COVERCACHE_JPG : fxsaveJPG(store,buffer,info.size,info.size,75); break;
    case COVERCACHE_WEBP: fxsaveWEBP(store,buffer,info.size,info.size,75.0f); break;
    case COVERCACHE_PNG : fxsavePNG(store,buffer,info.size,info.size); break;
    case COVERCACHE_BMP : fxsaveBMP(store,buffer,info.size,info.size); break;
    }
  return store.position()-offset;
  }


FXlong GMCoverCacheWriter::fit(FXImage * image){
  if (pixels==NULL) allocElms(pixels,info.size*info.size);
  memset(pixels,255,4*info.size*info.size);

  FXuchar * dst = (FXuchar*)pixels;
  FXuchar * src = (FXuchar*)image->getData();
  FXint sw=image->getWidth()*4;
  FXint sh=image->getHeight();

  if (image->getHeight()<info.size)
    dst+=(info.size*4)*((info.size-image->getHeight())>>1);

  if (image->getWidth()<info.size)
    dst+=4*((info.size-image->getWidth())>>1);

  do {
    memcpy(dst,src,sw);
    dst+=info.size*4;
    src+=sw;
    }
  while(--sh);

  return save(pixels);
  }




FXbool GMCoverCacheWriter::insert(FXint id,GMCover * cover) {
  FXint length;
  FXImage * image = GMCover::toImage(cover,info.size,1);
  if (image && store.eof()==false) {

    if (image->getWidth()!=info.size || image->getHeight()!=info.size)
      length=fit(image);
    else
      length=save(image->getData());

    info.insert(id,store.position()-length,length);
    delete image;
    return true;
    }
  return false;
  }


FXbool GMCoverCacheWriter::finish() {
  info.save(store);
  return true;
  }

FXbool GMCoverCacheWriter::close() {
  store.close();
  return true;
  }




GMCoverCache::GMCoverCache(const FXString & name,FXint sz) : info(sz) {
  filename = GMApp::instance()->getCacheDirectory()+PATHSEPSTRING+name+".cache";
  }

GMCoverCache::~GMCoverCache(){
  }

FXbool GMCoverCache::contains(FXint id) {
  return (info.map.at(id)>0);
  }

FXbool GMCoverCache::render(FXint id,FXImage * image) {
  FXColor * pixels=NULL;
  FXbool result;
  FXint ww,hh,dd;
  FXint i = info.map.at(id) - 1;
  FXASSERT(i>=0);
  if (data.base()) {
    FXMemoryStream store(FXStreamLoad,((FXuchar*)data.base())+info.index[i].position,info.index[i].length);
    switch(info.format) {
      case COVERCACHE_JPG : result = fxloadJPG(store,pixels,ww,hh,dd); break;
      case COVERCACHE_WEBP: result = fxloadWEBP(store,pixels,ww,hh); break;
      case COVERCACHE_PNG : result = fxloadPNG(store,pixels,ww,hh); break;
      case COVERCACHE_BMP : result = fxloadBMP(store,pixels,ww,hh); break;
      default             : result = false; break;
      }
    if (result) {
      image->setData(pixels,IMAGE_OWNED);
      image->render();
      return true;
      }
    }
  return false;
  }

void GMCoverCache::clear(FXint sz) {
  info.clear(sz);
  data.close();
  }


void GMCoverCache::load(GMCoverCacheWriter & writer) {
  if (data.base())
    data.close();

  FXFile::rename(getTempFilename(),getFilename());

  if (data.openMap(getFilename()))
    info.adopt(writer.info);
  else
    info.clear(writer.info.size);
  }


FXbool GMCoverCache::load() {
  FXFileStream store;
  FXuint version;
  FXint  filesize;
  FXbool status=true;
  if (store.open(filename,FXStreamLoad)) {

    // check version
    store >> version;
    if (version!=COVERCACHE_FILE_VERSION)
      return false;

    // check cover size stored in this file
    store >> filesize;

    // if size doesn't match, indicate we need to generate covers again.
    if (filesize!=info.size)
      status=false;

    // use the cover size stored in the file
    info.size=filesize;

    // image format
    store >> info.format;

    // load info structure
    info.load(store);

    // Open memory map
    if (data.openMap(filename)==NULL)
      return false;

    return status;
    }
  return false;
  }


GMCoverLoader::GMCoverLoader(const FXString & file,GMCoverPathList & pathlist,FXint size,FXObject* tgt,FXSelector sel) : GMTask(tgt,sel), writer(size), filename(file),folderonly(false) {
  list.adopt(pathlist);
  }

FXint GMCoverLoader::run() {
  GMCover * cover;
  FXint percentage=0,p=-1;
  if (writer.open(filename)) {
    for (FXint i=0;i<list.no() && processing;i++){
      percentage = (FXint)(100.0f*((float)(i+1)/(float)list.no()));
      if (p!=percentage)
        taskmanager->setStatus(FXString::value("Loading Covers %d%%",percentage));
      if (__likely(folderonly==false)) {
        cover = GMCover::fromTag(list[i].path);
        if (cover==NULL) cover = GMCover::fromPath(FXPath::directory(list[i].path));
        }
      else {
        cover = GMCover::fromPath(list[i].path);
        }
       if (cover) writer.insert(list[i].id,cover);
      }
    if (processing) {
      writer.finish();
      writer.close();
      return 0;
      }
    else {
      writer.close();
      FXFile::remove(filename);
      return 1;
      }
    }
  return 1;
  }


GMCoverRender::GMCoverRender() : cache(NULL) {
  }

GMCoverRender::~GMCoverRender() {
  for (FXint i=0;i<buffers.no();i++) {
    delete buffers[i];
    }
  cache=NULL;
  }


FXint GMCoverRender::getSize() const {
  if (cache)
    return cache->getSize();
  else
    return GMPlayerManager::instance()->getPreferences().gui_coverdisplay_size;
  }


void GMCoverRender::setCache(GMCoverCache * c){
  if (c && buffers.no() && buffers[0]->getWidth()!=c->getSize()) {
    for (FXint i=0;i<buffers.no();i++) {
      delete buffers[i];
      }
    buffers.clear();
    }
  else {
    // To force reloading of cover art, set user data to 0
    // We can't use reset() here since cover art will be reused in the next onPaint
    // by the calls reset / markCover.
    for (FXint i=0;i<buffers.no();i++){
      buffers[i]->setUserData((void*)(FXival)0);
      }
    }
  cache=c;
  }

void GMCoverRender::markCover(FXint id) {
  for (FXint i=0,index;i<buffers.no();i++){
    index=(FXint)(FXival)buffers[i]->getUserData();
    if (index==-id) {
      buffers[i]->setUserData((void*)(FXival)id);
      break;
      }
    }
  }

void GMCoverRender::reset() {
  for (FXint i=0,index;i<buffers.no();i++){
    index=(FXint)(FXival)buffers[i]->getUserData();
    if (index>0) buffers[i]->setUserData((void*)(FXival)(-index));
    }
  }


void GMCoverRender::drawCover(FXint id,FXDC & dc,FXint x,FXint y) {
  FXImage * image;
  if (cache && cache->contains(id) && (image=getImage(id))!=NULL) {
    dc.drawImage(image,x,y);
    }
  else {
    dc.setForeground(FXApp::instance()->getBaseColor());
    dc.fillRectangle(x,y,getSize(),getSize());
    FXIcon * ic = GMIconTheme::instance()->icon_nocover;
    if (ic->getHeight()<getSize())
      y+=(getSize()-ic->getHeight())>>1;
    if (ic->getWidth()<getSize())
      x+=(getSize()-ic->getWidth())>>1;
    dc.drawIcon(ic,x,y);
    }
  }


FXImage* GMCoverRender::getImage(FXint id) {
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
    image = new FXImage(FXApp::instance(),NULL,0,getSize(),getSize());
    image->setUserData((void*)(FXival)id);
    image->create();
    buffers.append(image);
    }

  // Render Image
  if (!cache->render(id,image)) {
    image->setUserData(0);
    return NULL;
    }

  return image;
  }

