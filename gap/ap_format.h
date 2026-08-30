/*******************************************************************************
*                         Goggles Audio Player Library                         *
********************************************************************************
*           Copyright (C) 2010-2026 by Sander Jansen. All Rights Reserved      *
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
*                               ---                                            *
* SPDX-License-Identifier: GPL-3.0-or-later                                    *
********************************************************************************/
#ifndef AUDIOFORMAT_H
#define AUDIOFORMAT_H

namespace ap {


struct ReplayGain{
  FXdouble album      = NAN;
  FXdouble album_peak = NAN;
  FXdouble track      = NAN;
  FXdouble track_peak = NAN;

  [[nodiscard]] FXbool empty() const { return isnan(album) && isnan(track); }

  void reset() { album=NAN; album_peak=NAN; track=NAN; track_peak=NAN; }
  };


namespace Codec {

  enum {
    Invalid   = 0,
    PCM       = 1,
    FLAC      = 2,
    Vorbis    = 3,
    MPEG      = 4,
    AAC       = 5,
    Opus      = 6,
    ALAC      = 7,
    DCA       = 8,
    A52       = 9
    };

  extern const FXchar * name(FXuchar codec);
  }


namespace Channel {
  constexpr FXuint None        =  0u;
  constexpr FXuint Mono        =  1u;
  constexpr FXuint FrontLeft   =  2u;
  constexpr FXuint FrontRight  =  3u;
  constexpr FXuint FrontCenter =  4u;
  constexpr FXuint BackLeft    =  5u;
  constexpr FXuint BackRight   =  6u;
  constexpr FXuint BackCenter  =  7u;
  constexpr FXuint SideLeft    =  8u;
  constexpr FXuint SideRight   =  9u;
  constexpr FXuint LFE         = 10u;
  constexpr FXuint Reserved    = 15u; // Max 4 bits
  }



#define AP_CMAP1(c1)                      (c1)
#define AP_CMAP2(c1,c2)                   (c1|(c2<<4))
#define AP_CMAP3(c1,c2,c3)                (c1|(c2<<4)|(c3<<8))
#define AP_CMAP4(c1,c2,c3,c4)             (c1|(c2<<4)|(c3<<8)|(c4<<12))
#define AP_CMAP5(c1,c2,c3,c4,c5)          (c1|(c2<<4)|(c3<<8)|(c4<<12)|(c5<<16))
#define AP_CMAP6(c1,c2,c3,c4,c5,c6)       (c1|(c2<<4)|(c3<<8)|(c4<<12)|(c5<<16)|(c6<<20))
#define AP_CMAP7(c1,c2,c3,c4,c5,c6,c7)    (c1|(c2<<4)|(c3<<8)|(c4<<12)|(c5<<16)|(c6<<20)|(c7<<24))
#define AP_CMAP8(c1,c2,c3,c4,c5,c6,c7,c8) (c1|(c2<<4)|(c3<<8)|(c4<<12)|(c5<<16)|(c6<<20)|(c7<<24)|(c8<<28))





namespace Format {

enum {

  /// Mask and shift definitions
  Type_Mask         = 7,
  Type_Shift        = 0,
  Order_Mask        = 1,
  Order_Shift       = 3,
  Bits_Mask         = 31,
  Bits_Shift        = 8,
  Pack_Mask         = 7,
  Pack_Shift        = 13,

  /// Data type (xxxx xxxx xxxx x111)
  Signed            = 0,
  Unsigned          = 1,
  Float             = 2,
  IEC958            = 3,
  Format_Reserved_1 = 4,
  Format_Reserved_2 = 5,
  Format_Reserved_3 = 6,
  Format_Reserved_4 = 7,

  /// Byte Order (xxxx xxxx xxxx 1xxx)
  Little            = ( 0 << Order_Shift),
  Big               = ( 1 << Order_Shift),
#if FOX_BIGENDIAN == 1
  Native            = Big,
  Other             = Little,
#else
  Native            = Little,
  Other             = Big,
#endif

  /// Bits per sample  (xxx1 1111 xxxx xxxx)
  Bits_8            = ( 7 << Bits_Shift),
  Bits_16           = (15 << Bits_Shift),
  Bits_24           = (23 << Bits_Shift),
  Bits_32           = (31 << Bits_Shift),

  /// Bytes per sample (111x xxxx xxxx xxxx)
  Pack_1            = ( 0 << Pack_Shift),
  Pack_2            = ( 1 << Pack_Shift),
  Pack_3            = ( 2 << Pack_Shift),
  Pack_4            = ( 3 << Pack_Shift),
  Pack_Reserved_1   = ( 4 << Pack_Shift),
  Pack_Reserved_2   = ( 5 << Pack_Shift),
  Pack_Reserved_3   = ( 6 << Pack_Shift),
  Pack_8            = ( 7 << Pack_Shift),


  //// Input Formats
  Unknown           = 0,
  WAV               = 1,
  OGG               = 2,
  FLAC              = 3,
  MP3               = 4,
  MP4               = 5,
  AAC               = 6,
  M3U               = 7,
  PLS               = 8,
  XSPF              = 9,
  AIFF              = 10,
  Matroska          = 11
  };

}

enum {
  AP_FORMAT_S8          = ( Format::Signed   | Format::Native | Format::Bits_8  | Format::Pack_1 ),
  AP_FORMAT_U8          = ( Format::Unsigned | Format::Native | Format::Bits_8  | Format::Pack_1 ),

  AP_FORMAT_S16         = ( Format::Signed   | Format::Native | Format::Bits_16 | Format::Pack_2 ),
  AP_FORMAT_S16_OTHER   = ( Format::Signed   | Format::Other  | Format::Bits_16 | Format::Pack_2 ),

  AP_FORMAT_S16_LE      = ( Format::Signed   | Format::Little | Format::Bits_16 | Format::Pack_2 ),
  AP_FORMAT_S16_BE      = ( Format::Signed   | Format::Big    | Format::Bits_16 | Format::Pack_2 ),

  AP_FORMAT_FLOAT       = ( Format::Float    | Format::Native | Format::Bits_32 | Format::Pack_4 ),
  AP_FORMAT_FLOAT_OTHER = ( Format::Float    | Format::Other  | Format::Bits_32 | Format::Pack_4 ),
  AP_FORMAT_FLOAT_LE    = ( Format::Float    | Format::Little | Format::Bits_32 | Format::Pack_4 ),
  AP_FORMAT_FLOAT_BE    = ( Format::Float    | Format::Big    | Format::Bits_32 | Format::Pack_4 ),

  AP_FORMAT_S24         = ( Format::Signed   | Format::Native | Format::Bits_24 | Format::Pack_4 ),
  AP_FORMAT_S24_LE      = ( Format::Signed   | Format::Little | Format::Bits_24 | Format::Pack_4 ),
  AP_FORMAT_S24_BE      = ( Format::Signed   | Format::Big    | Format::Bits_24 | Format::Pack_4 ),

  AP_FORMAT_S24_3       = ( Format::Signed   | Format::Native | Format::Bits_24 | Format::Pack_3 ),
  AP_FORMAT_S24_3LE     = ( Format::Signed   | Format::Little | Format::Bits_24 | Format::Pack_3 ),
  AP_FORMAT_S24_3BE     = ( Format::Signed   | Format::Big    | Format::Bits_24 | Format::Pack_3 ),

  AP_FORMAT_S32         = ( Format::Signed   | Format::Native | Format::Bits_32 | Format::Pack_4 ),
  AP_FORMAT_S32_LE      = ( Format::Signed   | Format::Little | Format::Bits_32 | Format::Pack_4 ),
  AP_FORMAT_S32_BE      = ( Format::Signed   | Format::Big    | Format::Bits_32 | Format::Pack_4 ),


  AP_CHANNELMAP_MONO    = ( Channel::Mono ),
  AP_CHANNELMAP_STEREO  = AP_CMAP2(Channel::FrontLeft,Channel::FrontRight)
  };


extern FXuint ap_format_from_extension(const FXString & extension);
extern FXuint ap_format_from_mime(const FXString & mime);
extern FXuint ap_format_from_buffer(const FXchar * buffer,FXival size);
extern const FXchar * ap_format_name(FXuint name);


class GMAPI AudioFormat {
public:
  FXuint   rate       = 0;
  FXushort format     = 0;
  FXuchar  channels   = 0;
  FXuint   channelmap = 0;  // up to 8 channels
public:
  AudioFormat() = default;

  void setBits(FXushort bps);

  void setChannels(FXuchar channels);

  void set(FXushort datatype,FXushort bps,FXushort pack,FXuint rate,FXuchar channels,FXuint map=0);

  void set(FXushort format,FXuint rate,FXuchar channels,FXuint map=0);

  [[nodiscard]] FXbool undefined() const { return ((rate==0) && (format==0) && (channels==0)); }

  [[nodiscard]] FXbool set() const { return (rate!=0) && (format!=0) && (channels!=0); }

  [[nodiscard]] FXuchar channeltype(FXuint c) const { return static_cast<FXuchar>((channelmap>>(c<<2))&0xF); }

  [[nodiscard]] FXuchar byteorder() const {
    return (format>>Format::Order_Shift)&Format::Order_Mask;
    }

  [[nodiscard]] FXuchar datatype() const {
    return format&Format::Type_Mask;
    }

  [[nodiscard]] FXuchar bps() const {
    return 1+((format>>Format::Bits_Shift)&Format::Bits_Mask);
    }

  [[nodiscard]] FXuchar packing() const {
    return 1+((format>>Format::Pack_Shift)&Format::Pack_Mask);
    }

  [[nodiscard]] FXint framesize() const {
    return static_cast<FXint>(channels) * static_cast<FXint>(packing());
    }

  /* Swap byte order. Return true if succesfull */
  FXbool swap();

  /* Change to compatible format */
  FXbool compatible();

  void debug() const;

  [[nodiscard]] FXString debug_format() const;

  void reset();
  };

extern GMAPI FXbool operator!=(const AudioFormat& s1,const AudioFormat& s2);
extern GMAPI FXbool operator==(const AudioFormat& s1,const AudioFormat& s2);

}
#endif
