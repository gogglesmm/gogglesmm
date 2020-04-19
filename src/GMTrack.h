/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2010-2018 by Sander Jansen. All Rights Reserved      *
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
#ifndef GMTRACK_H
#define GMTRACK_H

/// Album number consists of a disc number + track number
#define GMALBUMNO(disc,track) ((((FX::FXuint)(track))&0xffff) | (((FX::FXuint)(disc))<<16))

/// Get the disc number from a album no.
#define GMDISCNO(s) ((FX::FXushort)(((s)>>16)&0xffff))

/// Get the track number from a album no.
#define GMTRACKNO(s) ((FX::FXushort)((s)&0xffff))

enum {
  TAG_STRIP_ID3v1 = 0x1,
  TAG_STRIP_ID3v2 = 0x2,
  TAG_STRIP_APE   = 0x4,
  };

/// Used in Database, keep values constant
enum {
  FILETYPE_UNKNOWN    = 0,
  FILETYPE_OGG_VORBIS = 1,
  FILETYPE_OGG_OPUS   = 2,
  FILETYPE_OGG_SPEEX  = 3,
  FILETYPE_OGG_FLAC   = 4,
  FILETYPE_FLAC       = 5,
  FILETYPE_MP3        = 6,
  FILETYPE_MP4_AAC    = 7,
  FILETYPE_MP4_ALAC   = 8,

  FILETYPE_PNG        = 1,
  FILETYPE_JPG        = 2,
  FILETYPE_BMP        = 3,
  FILETYPE_GIF        = 4
  };


class GMTrack{
public:
  FXString      url;
  FXString      title;
  FXString      artist;
  FXString      album;
  FXString      album_artist;
  FXString      composer;
  FXString      conductor;
  FXStringList  tags;
  FXString      lyrics;
  FXuint        index         = 0;
  FXint         year          = 0;
  FXint 	      no            = 0;
  FXint		      time          = 0;
  FXint         bitrate       = 0;
  FXuint        rating        = 0;
  FXuchar       channels      = 0;
  FXuchar       sampleformat  = 0;
  FXint         samplerate    = 0;
  FXuchar       filetype      = FILETYPE_UNKNOWN;
public:
  GMTrack() = default;

  FXbool hasMissingLyrics() const;

  FXbool hasMissingData() const;

  /// Clear the track
  void clear();

  /// Adopt from track
  void adopt(GMTrack &);

  /// Get track number
  FXushort getTrackNumber() const { return (FXushort)(no&0xffff); }

  /// Set Track Number
  void setTrackNumber(FXushort track) { no|=((FXuint)track)&0xffff; }

  /// Set Disc Number
  void setDiscNumber(FXushort disc) { no|=((FXuint)disc)<<16; }

  /// Get disc number
  FXushort getDiscNumber() const { return (FXushort)(no>>16); }

  /// Get Album Artist
  const FXString & getAlbumArtist(const FXString & def) const;

  /// Get Artist
  const FXString & getArtist(const FXString & def) const;

  void setTagsFromString(const FXString &);

  /// Load from tag in given filename. Note that mrl is not set
  FXbool loadTag(const FXString & filename);

  /// Load properties from given filename
  FXbool loadProperties(const FXString & filename);

  /// Save to tag in given filename. Note that mrl is not set
  FXbool saveTag(const FXString & filename,FXuint opts=0);
  };


typedef FXArray<GMTrack> GMTrackArray;


struct GMTrackFilename {
  FXString filename;
  FXlong   date;
  FXint    id;
  };

typedef FXArray<GMTrackFilename> GMTrackFilenameList;


struct GMCoverPath {
  FXint    id;
  FXString path;
  };
typedef FXArray<GMCoverPath> GMCoverPathList;


struct GMStream{
  FXString url;
  FXString description;
  FXString tag;
  FXint    bitrate;
  };


#endif

