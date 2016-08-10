/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2010-2016 by Sander Jansen. All Rights Reserved      *
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
#include "GMTrack.h"
#include "GMCover.h"
#include "GMCoverManager.h"

GMCoverManager::GMCoverManager() : cover(nullptr){
  }

GMCoverManager::~GMCoverManager(){
  clear();
  }

void GMCoverManager::clear() {
  if (cover) {
    delete cover;
    cover = nullptr;
    }

  if (!share.empty()){
    FXFile::remove(share);
    share.clear();
    }

  source.clear();
  }

FXbool GMCoverManager::load(const FXString & filename) {
  FXString path = FXPath::directory(filename);

  // Reuse existing
  if (source==filename || source==path)
    return false;

  // Clear existing
  clear();

  if (gm_is_local_file(filename)) {

    // Load
    cover = GMCover::fromTag(filename);
    if (cover==nullptr) {
      cover = GMCover::fromPath(path);
      if (cover) source=path;
      }
    else {
      source=filename;
      }

    if (cover) {
      share = "/dev/shm/gogglesmm/cover" + cover->fileExtension();
      if (!cover->save(share))
        share.clear();
      }
    }

  return true;
  }
