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
#ifndef GMCOVERCACHE_H
#define GMCOVERCACHE_H

class GMCompressedImage;
class GMCover;


/* Header */
class GMCacheInfo {
public:
  struct FileIndex {
    FXlong position;
    FXint  length;
    FileIndex() : position(0), length(0) {}
    FileIndex(FXlong pos,FXint len) : position(pos),length(len) {}
    };
public:
  FXArray<FileIndex> index;
  FXIntMap           map;
  FXint              size;
  FXuchar            format;
public:
  GMCacheInfo(FXint sz) : size(sz),format(0) {}

  void adopt(GMCacheInfo & info);

  void insert(FXint id,FXlong position,FXint length);

  void clear(FXint sz);

  void save(FXStream & store) const;

  void load(FXStream & store);
  };



/* Cover Cache Writer */
class GMCoverCacheWriter {
  friend class GMCoverCache;
private:
  FXFileStream store;
  GMCacheInfo  info;
  FXColor*     pixels;
private:
  FXlong fit(FXImage* image);
  FXlong save(FXColor * buffer);
public:
  GMCoverCacheWriter(FXint size);

  FXbool open(const FXString & filename);

  FXbool insert(FXint id,GMCover*);

  FXbool finish();

  FXbool close();

  ~GMCoverCacheWriter();
  };

/* Cover Cache */
class GMCoverCache {
protected:
  FXString    filename;
  GMCacheInfo info;
  FXMemMap    data;
public:
  GMCoverCache(const FXString & name,FXint size=128);

  // Get Image Size
  FXint getSize() const { return info.size; }

  // Render cover with id to image
  FXbool render(FXint id,FXImage * image);

  // Check if cover is contained in cache
  FXbool contains(FXint id);

  // Load cache from file
  FXbool load();

  // Clear cache
  void clear(FXint size=128);

  // Load from cache writer
  void load(GMCoverCacheWriter & writer);

  // Get filename for this cache
  const FXString & getFilename() const { return filename; }

  // Get Temp Filename
  const FXString getTempFilename() const { return filename+".tmp"; }

  // Destructor
  ~GMCoverCache();
  };


/* Cover Render */
class GMCoverRender {
protected:
  GMCoverCache*        cache;
  FXPtrListOf<FXImage> buffers;
protected:
  FXImage * getImage(FXint id);
public:
  GMCoverRender();

  // Get Cover Size
  FXint getSize() const;

  // Change the cache
  void setCache(GMCoverCache * cache);

  // Draw Cover
  void drawCover(FXint id,FXDC & dc,FXint x,FXint y);

  // Mark Cover
  void markCover(FXint id);

  // Reset
  void reset();

  ~GMCoverRender();
  };

#endif
