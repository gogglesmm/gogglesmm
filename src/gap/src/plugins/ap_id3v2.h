#ifndef AP_ID3V2_H
#define AP_ID3V2_H

namespace ap {

#define ID3_SYNCSAFE_INT32(b) ( (((b)[0]&0x7f)<<21) | (((b)[1]&0x7f)<<14) | (((b)[2]&0x7f)<<7) | (((b)[3]&0x7f)))
#define ID3_INT32(b) ((((b)[0])<<24) | (((b)[1])<<16) | (((b)[2])<<8) | (((b)[3])))

#define DEFINE_FRAME(b1,b2,b3,b4) ((b4<<24) | (b3<<16) | (b2<<8) | (b1))
#define DEFINE_FRAME_V2(b1,b2,b3) ((b3<<16) | (b2<<8) | (b1))

class ID3V2 {
private:
  FXuchar * buffer;
  FXint     size;
  FXint     p;
  FXchar    version;
protected:
  void unsync(FXuchar * buffer,FXint & len);
  void parse_frame();
  void parse_text_frame(FXuint frameid,FXint framesize);
  void parse_rva2_frame(FXuint frameid,FXint framesize);
public:
  enum Encoding {
    ISO_8859_1     = 0,
    UTF16_BOM      = 1,
    UTF16          = 2,
    UTF8           = 3
    };

  enum Frames {

    /// Version 2 frames
    TT2 = DEFINE_FRAME_V2('T','T','2'),
    TP1 = DEFINE_FRAME_V2('T','P','1'),
    TAL = DEFINE_FRAME_V2('T','A','L'),

    /// Version 3 / 4 frames
    TPE1 = DEFINE_FRAME('T','P','E','1'),
    TCOM = DEFINE_FRAME('T','C','O','M'),
    TALB = DEFINE_FRAME('T','A','L','B'),
    TIT2 = DEFINE_FRAME('T','I','T','2'),
    RVA2 = DEFINE_FRAME('R','V','A','2')
    };

  enum {
    HAS_FOOTER          = (1<<4),
    HAS_EXTENDED_HEADER = (1<<6), /// Compression in v2
    HAS_UNSYNC          = (1<<7),
    };

  enum {
    FRAME_COMPRESSED = (1<<7),
    FRAME_ENCRYPTED  = (1<<6),
    FRAME_GROUP      = (1<<5)
    };

public:
  FXString    artist;
  FXString    album;
  FXString    title;
  ReplayGain  replaygain;
public:
  ID3V2(FXuchar * b,FXint len);
  ~ID3V2();

  FXbool empty() const;

  };

}

#endif









