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
#ifndef GMTAG_H
#define GMTAG_H


namespace TagLib {
  class File;
  class Tag;
  namespace MP4 {
    class Tag;
    }
  namespace Ogg {
    class XiphComment;
    }
  namespace ID3v2 {
    class Tag;
    }
  namespace APE {
    class Tag;
    }
  }


enum {
  FILETAG_TAGS            = 0x0, // Read TAGS from file
  FILETAG_AUDIOPROPERTIES = 0x1  // Determine audio properties from file
  };

enum {
  COVER_REPLACE_ALL   = 0,
  COVER_REPLACE_TYPE  = 1
  };


class GMCover;
typedef FXArray<GMCover*> GMCoverList;

class GMFileTag {
protected:
  TagLib::File              * file;
  TagLib::Tag               * tag;
  TagLib::MP4::Tag          * mp4;
  TagLib::Ogg::XiphComment  * xiph;
  TagLib::ID3v2::Tag        * id3v2;
  TagLib::APE::Tag          * ape;
protected:
  void id3v2_get_field(const FXchar * field,FXString &) const;
  void id3v2_get_field(const FXchar * field,FXStringList &) const;
  void id3v2_update_field(const FXchar * field,const FXString & value);
  void id3v2_update_field(const FXchar * field,const FXStringList & value);

  void xiph_get_field(const FXchar * field,FXString &) const;
  void xiph_get_field(const FXchar * field,FXStringList &) const;
  void xiph_update_field(const FXchar * field,const FXString & value);
  void xiph_update_field(const FXchar * field,const FXStringList & value);
  void xiph_add_field(const FXchar * field,const FXString & value);

  void mp4_get_field(const FXchar * field,FXString &) const;
  void mp4_get_field(const FXchar * field,FXStringList &) const;
  void mp4_update_field(const FXchar * field,const FXString & value);
  void mp4_update_field(const FXchar * field,const FXStringList & value);

  void ape_get_field(const FXchar * field,FXString &) const;
  void ape_get_field(const FXchar * field,FXStringList &) const;
  void ape_update_field(const FXchar * field,const FXString & value);
  void ape_update_field(const FXchar * field,const FXStringList & value);

  void parse_tagids(FXStringList&) const;

  void trimList(FXStringList & value) const;
public:
  GMFileTag();

  FXbool open(const FXString & filename,FXuint opts);
  FXbool save();

  void setComposer(const FXString & value);
  void getComposer(FXString &) const;

  void setConductor(const FXString & value);
  void getConductor(FXString &) const;

  void setAlbumArtist(const FXString & value);
  void getAlbumArtist(FXString &) const;

  void setArtist(const FXString &);
  void getArtist(FXString&) const;

  void setAlbum(const FXString &);
  void getAlbum(FXString&) const;

  void setTitle(const FXString &);
  void getTitle(FXString&) const;

  void setTags(const FXStringList & value);
  void getTags(FXStringList&) const;

  void setDiscNumber(FXushort disc);
  FXushort getDiscNumber() const;

  void setTrackNumber(FXushort no);
  FXushort getTrackNumber() const;

  void setYear(FXint);
  FXint getYear() const;

  FXint getTime() const;
  FXint getBitRate() const;
  FXint getChannels() const;
  FXint getSampleRate() const;
  FXint getSampleSize() const;

  FXuchar getFileType() const;

  GMCover * getFrontCover() const;
  FXint getCovers(GMCoverList &) const;

  void appendCover(GMCover*);

  void replaceCover(GMCover*,FXuint mode=COVER_REPLACE_ALL);
  void clearCovers();

  ~GMFileTag();
  };



class GMAudioProperties {
public:
  FXint    bitrate;
  FXint    samplerate;
  FXint    channels;
  FXint    samplesize;
  FXuchar  filetype;
public:
  GMAudioProperties();

  /// Load from tag in given filename.
  FXbool load(const FXString & filename);
  };


namespace GMTag {

void init();

void setID3v1Encoding(const FXTextCodec * codec);

FXbool length(GMTrack & info);

}

#endif
