/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2010-2017 by Sander Jansen. All Rights Reserved      *
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
#ifndef GMCOVER_MANAGER_H
#define GMCOVER_MANAGER_H

class GMCoverManager {
protected:
  GMCover* cover;
  FXString source;
  FXString share;
public:
  GMCoverManager();

  // Clear
  void clear();

  // Load Cover
  FXbool load(const FXString & filename);

  // Get the share filename
  FXString getShareFilename() const { return share; }

  // Get the cover
  GMCover* getCover() const { return cover; }

  ~GMCoverManager();
  };

#endif

