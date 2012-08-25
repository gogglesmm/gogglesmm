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



class GMCover;
typedef FXArray<GMCover*> GMCoverList;

struct GMImageInfo {
  FXuint  width;
  FXuint  height;
  FXuchar bps;
  FXuchar colors;

  GMImageInfo() : width(0),height(0),bps(0),colors(0) {}
  };

enum {
  FILETYPE_UNKNOWN = 0,
  FILETYPE_PNG     = 1,
  FILETYPE_JPG     = 2,
  FILETYPE_BMP     = 3,
  FILETYPE_GIF     = 4
  };


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
protected:
  GMImageInfo info;
public:
  FXuchar*    data;
  FXuint      size;
  FXString    description;
  FXuchar     type;
public:
  // Empty Cover
  GMCover();

  /// Construct Cover
  GMCover(const void * data,FXuint sz,FXuint t=GMCover::Other,const FXString & label=FXString::null,FXbool owned=false);

  /// Destructor
  ~GMCover();

  /// Return Image Information
  FXbool getImageInfo(GMImageInfo &);

  /// Return file extension for image type.
  FXString fileExtension() const;

  /// Return mimetype for image type
  FXString mimeType() const;

  /// Return filetype
  FXuint fileType() const;

  FXbool save(const FXString & path);
public:

  static FXint fromTag(const FXString & mrl,GMCoverList & list);
#if 0
  static FXint fromPath(const FXString & mrl,GMCoverList & list);
#endif

  static GMCover * fromTag(const FXString & file);

  static GMCover * fromPath(const FXString & path);

  static GMCover * fromFile(const FXString & file);

  static FXImage * copyToImage(GMCover*,FXint scale=0,FXint crop=0);

  static FXImage * toImage(GMCover*,FXint scale=0,FXint crop=0);
  };

#endif

