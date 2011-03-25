/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2010-2010 by Sander Jansen. All Rights Reserved      *
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
#ifndef GMCOVER_H
#define GMCOVER_H

/// FlacPictureBlock structure.
struct FlacPictureBlock{
  FXString    mimetype;
  FXString    description;
  FXuint      type;
  FXuint      width;
  FXuint      height;
  FXuint      bps;
  FXuint      ncolors;
  FXuint      data_size;
  FXuchar*    data;

  FXuint size() const {
    return 64 + description.length() + mimetype.length() + data_size;
    }
  };

class GMCover;
typedef FXArray<GMCover*> GMCoverList;

class GMCover {
public:
  enum {
    Other             =  0,
    FileIcon          =  1,
    OtherFileIcon     =  2,
    FrontCover        =  3,
    BackCover         =  4,
    Leaflet           =  5,
    Media             =  6,
    LeadArtist        =  7,
    Artist            =  8,
    Conductor         =  9,
    Band              = 10,
    Composer          = 11,
    Lyricist          = 12,
    RecordingLocation = 13,
    DuringRecoding    = 14,
    DuringPerformance = 15,
    ScreenCapture     = 16,
    Fish              = 17,
    Illustration      = 18,
    BandLogo          = 19,
    PublisherLogo     = 20
    };
protected: // raw data
  FXuchar * data;
  FXuval    size;
  FXString  mime;
public:
  FXImage * image;
  FXString  description;
  FXuint    type;
public:
  GMCover();
  GMCover(FXImage * img,FXuint t=GMCover::Other,const FXString & label=FXString::null);
  ~GMCover();
public:
  static FXint fromTag(const FXString & mrl,GMCoverList & list,FXint scale=0,FXint crop=0);

  static FXint fromPath(const FXString & mrl,GMCoverList & list,FXint scale=0,FXint crop=0);

  static GMCover * fromTag(const FXString & mrl,FXint scale=0,FXint crop=0);

  static GMCover * fromPath(const FXString & mrl,FXint scale=0,FXint crop=0);

  static FXImage * toImage(GMCover*);

  static FXIcon * toIcon(GMCover*);
  };

#endif

