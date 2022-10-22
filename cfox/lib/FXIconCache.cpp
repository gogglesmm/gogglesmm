/********************************************************************************
*                                                                               *
*                              I c o n   C a c h e                              *
*                                                                               *
*********************************************************************************
* Copyright (C) 1998,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
*********************************************************************************
* This library is free software; you can redistribute it and/or modify          *
* it under the terms of the GNU Lesser General Public License as published by   *
* the Free Software Foundation; either version 3 of the License, or             *
* (at your option) any later version.                                           *
*                                                                               *
* This library is distributed in the hope that it will be useful,               *
* but WITHOUT ANY WARRANTY; without even the implied warranty of                *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                 *
* GNU Lesser General Public License for more details.                           *
*                                                                               *
* You should have received a copy of the GNU Lesser General Public License      *
* along with this program.  If not, see <http://www.gnu.org/licenses/>          *
********************************************************************************/
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "fxmath.h"
#include "FXArray.h"
#include "FXHash.h"
#include "FXMutex.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXSize.h"
#include "FXPoint.h"
#include "FXRectangle.h"
#include "FXPath.h"
#include "FXDictionary.h"
#include "FXDictionaryOf.h"
#include "FXStringDictionary.h"
#include "FXSettings.h"
#include "FXRegistry.h"
#include "FXEvent.h"
#include "FXWindow.h"
#include "FXApp.h"
#include "FXIcon.h"
#include "FXIconSource.h"
#include "FXIconCache.h"


/*
  Notes:
  - Icon Cache provides fast mapping of icon name to icon; when encountering
    an icon name for the first time, the icon is loaded in using the facilities
    of the Icon Source icon loader.
  - Icon Source icon loader can be replaced with subclassed icon loader for
    additional file formats supported (outside of what FOX provides).
  - Icons loaded are not realized (created).  This is up to the user to do;
    rationale: not all applications need icons rendered for drawing purposes.
  - An icon search-path determines which directories are to be searched for
    the provided icon name.
  - Since loading an icon may fail, don't map the name till we've successfully
    obtained the icon.
*/

#define TOPIC_CONSTRUCT  1000

// You can override the default icon locations to search for your
// particular platform by specifying -DDEFAULTICONPATH="path" on
// the command line.
#ifndef DEFAULTICONPATH
#if defined(WIN32)
#define DEFAULTICONPATH   "%HOMEDRIVE%%HOMEPATH%\\FIXME;%SYSTEMDRIVE%%SYSTEMROOT%\\FIXME"
#else
#define DEFAULTICONPATH   "~/.icons:~/Pictures:/usr/local/share/icons:/usr/share/icons"
#endif
#endif

using namespace FX;

/*******************************************************************************/

namespace FX {


// Default icon path
const FXchar FXIconCache::defaultIconPath[]=DEFAULTICONPATH;


// Object implementation
FXIMPLEMENT(FXIconCache,FXObject,nullptr,0)


// Build icon cache
FXIconCache::FXIconCache():app((FXApp*)-1L),loader(&FXIconSource::defaultIconSource){
  FXTRACE((TOPIC_CONSTRUCT,"FXIconCache::FXIconCache\n"));
  }


// Build icon cache
FXIconCache::FXIconCache(FXApp* ap,const FXString& sp):app(ap),loader(&FXIconSource::defaultIconSource),path(sp){
  FXTRACE((TOPIC_CONSTRUCT,"FXIconCache::FXIconCache\n"));
  }


// Insert unique icon loaded from filename into dictionary
FXIcon* FXIconCache::insert(const FXchar* name){
  FXTRACE((200,"FXIconCache::insert(\"%s\")\n",name));
  FXIcon* result=dict[name];
  if(!result){
    result=loader->loadIconFile(getApp(),FXPath::search(path,name));
    if(result){
      dict[name]=result;
      }
    }
  return result;
  }


// Remove icon from cache and delete it
void FXIconCache::remove(const FXchar* name){
  FXTRACE((200,"FXIconCache::remove(\"%s\")\n",name));
  delete dict.remove(name);
  }


// Delete all icons
void FXIconCache::clear(){
  FXTRACE((200,"FXIconCache::clear()\n"));
  for(FXival i=0; i<dict.no(); ++i){
    delete dict.data(i);
    }
  dict.clear();
  }


// Save object to stream
void FXIconCache::save(FXStream& store) const {
  store << app;
  store << path;
  }


// Load object from stream
void FXIconCache::load(FXStream& store){
  store >> app;
  store >> path;
  }


// Destructor
FXIconCache::~FXIconCache(){
  FXTRACE((TOPIC_CONSTRUCT,"FXIconCache::~FXIconCache\n"));
  clear();
  app=(FXApp*)-1L;
  loader=(FXIconSource*)-1L;
  }

}
