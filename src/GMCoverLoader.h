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
#ifndef GMCOVERLOADER_H
#define GMCOVERLOADER_H

class GMCoverLoader : public GMTask {
protected:
  GMCoverCacheWriter writer;
  GMCoverPathList    list;
  FXString           filename;
  FXbool             folderonly;       
public:
  FXint run();
public:
  GMCoverLoader(const FXString & filename,GMCoverPathList & pathlist,FXint size,FXObject* tgt=NULL,FXSelector sel=0);

  void setFolderOnly(FXbool b) { folderonly=b; }

  GMCoverCacheWriter & getCacheWriter() { return writer; }
  };

#endif
