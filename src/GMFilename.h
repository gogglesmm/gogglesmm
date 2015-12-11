/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2006-2016 by Sander Jansen. All Rights Reserved      *
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

namespace GMFilename {

  enum {
    NOSPACES  						= 0x00000001,
    LOWERCASE 						= 0x00000002,
    LOWERCASE_EXTENSION	  = 0x00000004
    };

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

