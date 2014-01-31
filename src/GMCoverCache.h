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
#ifndef GMCOVERTHUMBS_H
#define GMCOVERTHUMBS_H

class GMImageMap;
class GMCompressedImage;

class GMCoverCache : FXObject {
FXDECLARE(GMCoverCache)
protected:
  FXPtrListOf<GMCompressedImage> covers;
  FXPtrListOf<FXImage>           buffers;
  FXIntMap                       map;
  FXint                          basesize;
  FXbool                         initialized;
protected:
  FXImage * getCoverImage(FXint id);
  void adopt(GMCoverCache &);
  FXbool load();
  FXString getCacheFile() const;
public:
  enum {
    ID_COVER_LOADER = 1
    };
public:
  long onCmdCoversLoaded(FXObject*,FXSelector,void*ptr);
public:
  GMCoverCache(FXint size=128);

  void init(GMTrackDatabase*);

  void refresh(GMTrackDatabase*);

  void drawCover(FXint id,FXDC & dc,FXint x,FXint y);

  void markCover(FXint id);

  void reset();

  void clear();

  FXint getCoverSize() const { return basesize; }

  void insertCover(FXint id,GMCompressedImage*);

  void save() const;

  ~GMCoverCache();
  };

#endif
