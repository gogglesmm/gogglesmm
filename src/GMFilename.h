/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2006-2017 by Sander Jansen. All Rights Reserved      *
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
#ifndef GMFILENAME_H
#define GMFILENAME_H

extern const FXchar * gmcodecnames[];

namespace gm {

// Convert and filter text suitable to be used in filenames
class TextConverter {
protected:
  const FXTextCodec * codec     = nullptr;
  FXString            forbidden = "\'\\#~!\"$&();<>|`^*?[]/.:";
  FXuint              modifiers = 0;
public:
  enum {
    NOSPACE   = 0x1,
    LOWERCASE = 0x2,
    UPPERCASE = 0x4,
    };
protected:
  FXString apply_filters(const FXString & src) const;
  FXString apply_codec(const FXString & src) const;
  FXString convert_to_ascii(const FXString & input) const;
  FXString convert_to_codec(const FXString & input) const;
public:
  explicit TextConverter(FXuint m) : modifiers(m) {}

  explicit TextConverter(const FXString & f,FXuint m) : forbidden(f), modifiers(m) {}

  explicit TextConverter(const FXTextCodec * c,const FXString & f, FXuint m) : codec(c), forbidden(f), modifiers(m) {}

  FXString convert(const FXString & input) const;
  };


/*
 * Format Track according to pattern
 * Syntax:
 *
 *    %T => track title
 *    %A => album title
 *    %P => album artist name
 *    %p => track artist name
 *    %G => genre
 *    %N => 2 digit track number
 *    %n => track number
 *    %d => disc number
 *    %y => track year
 *    %w => composer
 *    %c => conductor
 *
 *  Conditionals
 *
 *   ?c<a|b> => display a if c is not empty, display b if c is empty)
 *   ?c      => display c if not empty
 *
 */
class TrackFormatter : public TextConverter {
private:
  static const FXchar valid[];
protected:
  FXString mask;
public:
  enum {
    LOWERCASE_EXTENSION = 0x08,
    };
protected:
  FXString get_field(FXchar field, const GMTrack &) const;
  FXbool   has_field(FXchar field, const GMTrack &, FXString & value) const;
  FXString format_fields(const GMTrack & track, const FXString & path) const;
public:
  explicit TrackFormatter(const FXString &, const FXTextCodec *, const FXString &, FXuint);

  // TrackFormatter without options or forbidden characters
  explicit TrackFormatter(const FXString &, const FXTextCodec *);

  // Format track to filename
  FXString getPath(const GMTrack & track) const;

  // Format track to simple name
  FXString getName(const GMTrack & track) const;
  };

}


namespace GMFilename {

  enum {
    ENCODING_ASCII=0,
    ENCODING_UTF8,
    ENCODING_8859_1,
    ENCODING_8859_2,
    ENCODING_8859_3,
    ENCODING_8859_4,
    ENCODING_8859_5,
    ENCODING_8859_6,
    ENCODING_8859_7,
    ENCODING_8859_8,
    ENCODING_8859_9,
    ENCODING_8859_10,
    ENCODING_8859_11,
    ENCODING_8859_13,
    ENCODING_8859_14,
    ENCODING_8859_15,
    ENCODING_8859_16,
    ENCODING_CP437,
    ENCODING_CP850,
    ENCODING_CP852,
    ENCODING_CP855,
    ENCODING_CP856,
    ENCODING_CP857,
    ENCODING_CP860,
    ENCODING_CP861,
    ENCODING_CP862,
    ENCODING_CP863,
    ENCODING_CP864,
    ENCODING_CP865,
    ENCODING_CP866,
    ENCODING_CP869,
    ENCODING_CP874,
    ENCODING_CP1250,
    ENCODING_CP1251,
    ENCODING_CP1252,
    ENCODING_CP1253,
    ENCODING_CP1254,
    ENCODING_CP1255,
    ENCODING_CP1256,
    ENCODING_CP1257,
    ENCODING_CP1258,
    ENCODING_KOIR8,
    ENCODING_LAST
    };

  /// Filter a string
  FXString filter(const FXString & input,const FXString & forbidden,FXuint options);

  /// Create Filename based on Track Information and Format String
  FXbool create(FXString & result,const GMTrack & track, const FXString & format,const FXString & forbidden,const FXuint & options,const FXTextCodec * codec=nullptr);

  FXString format_track(const GMTrack & track,const FXString & path,const FXString & forbidden,const FXuint & options,const FXTextCodec * textcodec);

  enum {
    REPLACE_UNDERSCORE = 0x1,
    OVERWRITE = 0x2
    };

  /// Extract info from filename based on mask and store in GMTrack.
  void parse(GMTrack & track,const FXString & mask,FXuint opts);
  }

#endif

