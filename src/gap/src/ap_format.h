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
    AAC       = 6
    };

  extern const FXchar * name(FXuchar codec);
  }

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
  CDDA              = 8,
  M3U               = 9,
  PLS               = 10,
  XSPF              = 11,
  ASF               = 12,
  ASX               = 13,
  ASFX              = 14 // either ASX or ASF
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
  };


//extern GMAPI FXushort ap_format_try_compatible(FXushort format);
//extern GMAPI FXushort ap_format_try_swap(FXushort format);


extern FXuint ap_format_from_extension(const FXString & extension);
extern FXuint ap_format_from_mime(const FXString & mime);
extern FXuint ap_format_from_buffer(const FXchar * buffer,FXival size);

class GMAPI AudioFormat {
public:
  FXuint   rate;
  FXushort format;
  FXuchar  channels;
public:
  AudioFormat();
  AudioFormat(const AudioFormat &);

  void set(FXushort datatype,FXushort bps,FXushort pack,FXuint rate,FXuchar channels);

  void set(FXushort format,FXuint rate,FXuchar channels);

  FXbool undefined() const { return ((rate==0) && (format==0) && (channels==0)); }

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

  FXuint framesize() const {
    return channels * packing();
    }


  /* Swap byte order. Return true if succesfull */
  FXbool swap();

  /* Change to compatible format */
  FXbool compatible();

  void debug() const;

  void reset();
  };

extern GMAPI FXbool operator!=(const AudioFormat& s1,const AudioFormat& s2);
extern GMAPI FXbool operator==(const AudioFormat& s1,const AudioFormat& s2);

}
#endif
