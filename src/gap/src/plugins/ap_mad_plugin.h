#ifdef HAVE_MAD_PLUGIN
#ifndef AP_MAD_PLUGIN_H
#define AP_MAD_PLUGIN_H

#include <mad.h>

namespace ap {

class XingHeader;
class VBRIHeader;
class LameHeader;
struct mpeg_frame;

class MadInput : public InputPlugin {
protected:
  FXuchar      buffer[4];
  FXbool       sync;
  FXint        bitrate;
protected:
  FXlong       input_start;
  FXlong       input_end;
  FXlong       stream_position;
protected:
  XingHeader * xing;
  VBRIHeader * vbri;
  LameHeader * lame;
  InputStatus parse(Packet*);
protected:
  FXbool readFrame(Packet*,const mpeg_frame&);
  void parseFrame(Packet*,const mpeg_frame&);
  FXbool parse_id3v1();
  FXbool parse_ape();
public:
  MadInput(AudioEngine*);
  FXbool init();
  FXbool can_seek() const;
  FXbool seek(FXdouble);
  InputStatus process(Packet*);
  virtual ~MadInput();
  };


class MadDecoder : public DecoderPlugin {
protected:
  MemoryBuffer buffer;
  Packet * out;
protected:
  mad_synth synth;
  mad_stream stream;
  mad_frame frame;
protected:
  FXuchar flags;
  FXlong  stream_position;
  FXshort stream_offset_start;
  FXshort stream_offset_end;
  FXint        frame_counter;

protected:
  enum {
    FLAG_INIT = 0x1,
    };
public:
  MadDecoder(AudioEngine*);
  FXuchar codec() const { return Codec::MPEG; }
  FXbool init(ConfigureEvent*);
  DecoderStatus process(Packet*);
  FXbool flush();
  virtual ~MadDecoder();
  };
  
}  
#endif
#endif
