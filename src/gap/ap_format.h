/*******************************************************************************
*                         Goggles Audio Player Library                         *
********************************************************************************
*           Copyright (C) 2010-2014 by Sander Jansen. All Rights Reserved      *
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
#ifndef AUDIOFORMAT_H
#define AUDIOFORMAT_H

namespace ap {


struct ReplayGain{
  FXdouble album;
  FXdouble album_peak;
  FXdouble track;
  FXdouble track_peak;

  ReplayGain() : album(NAN), album_peak(NAN), track(NAN), track_peak(NAN) {}

  FXbool empty() const { return isnan(album) && isnan(track); }

  void reset() { album=NAN; album_peak=NAN; track=NAN; track_peak=NAN; }
  };


namespace Codec {

  enum {
    Invalid   = 0,
    PCM       = 1,
    FLAC      = 2,
    Vorbis    = 3,
    Musepack  = 4,
    MPEG      = 5,
    AAC       = 6,
    Opus      = 7
    };

  extern const FXchar * name(FXuchar codec);
  }


namespace Channel {
  const FXuint None        =  0u;
  const FXuint Mono        =  1u;
  const FXuint FrontLeft   =  2u;
  const FXuint FrontRight  =  3u;
  const FXuint FrontCenter =  4u;
  const FXuint BackLeft    =  5u;
  const FXuint BackRight   =  6u;
  const FXuint BackCenter  =  7u;
  const FXuint SideLeft    =  8u;
  const FXuint SideRight   =  9u;
  const FXuint LFE         = 10u;
  const FXuint Reserved    = 15u; // Max 4 bits
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
  Musepack          = 7,
  WavPack           = 8,
  CDDA              = 9,
  M3U               = 10,
  PLS               = 11,
  XSPF              = 12,
  ASF               = 13,
  ASX               = 14,
  ASFX              = 15, // either ASX or ASF
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
  FXuint   rate;
  FXushort format;
  FXuchar  channels;
  FXuint   channelmap;  // up to 8 channels
public:
  AudioFormat();
  AudioFormat(const AudioFormat &);

  void set(FXushort datatype,FXushort bps,FXushort pack,FXuint rate,FXuchar channels,FXuint map=0);

  void set(FXushort format,FXuint rate,FXuchar channels,FXuint map=0);

  FXbool undefined() const { return ((rate==0) && (format==0) && (channels==0)); }

  FXbool set() const { return (rate!=0) && (format!=0) && (channels!=0); }

  FXuchar channeltype(FXuint c) const { return (FXuchar)((channelmap>>(c<<2))&0xF); }

  FXuchar byteorder() const {
    return (format>>Format::Order_Shift)&Format::Order_Mask;
    }

  FXuchar datatype() const {
    return format&Format::Type_Mask;
    }

  FXuchar bps() const {
    return 1+((format>>Format::Bits_Shift)&Format::Bits_Mask);
    }

  FXuchar packing() const {
    return 1+((format>>Format::Pack_Shift)&Format::Pack_Mask);
    }

  FXint framesize() const {
    return (FXint)channels * (FXint)packing();
    }

  /* Swap byte order. Return true if succesfull */
  FXbool swap();

  /* Change to compatible format */
  FXbool compatible();

  void debug() const;

  FXString debug_format() const;

  void reset();
  };

extern GMAPI FXbool operator!=(const AudioFormat& s1,const AudioFormat& s2);
extern GMAPI FXbool operator==(const AudioFormat& s1,const AudioFormat& s2);

}
#endif
