#ifdef HAVE_AAC_PLUGIN
#ifndef AAC_H
#define AAC_H

#include "neaacdec.h"
#include "mp4ff.h"

namespace ap {

enum {
  AAC_FLAG_CONFIG = 0x1
  };

class AacInput : public InputPlugin {
protected:
  mp4ff_callback_t callback;
  mp4ff_t*         handle;
protected:
  static FXuint mp4_read(void*,void*,FXuint);
  static FXuint mp4_write(void*,void*,FXuint);
  static FXuint mp4_seek(void*,FXulong);
	static FXuint mp4_truncate(void*);
protected:
  Packet              * packet;
  FXlong                datastart;
  FXint                 nframes;
  FXint                 frame;
  FXint                 track;
protected:
  InputStatus parse();
public:
  AacInput(AudioEngine*);
  FXbool init();
  FXbool can_seek() const;
  FXbool seek(FXdouble);
  InputStatus process(Packet*);
  ~AacInput();
  };

class AacDecoder : public DecoderPlugin {
protected:
  NeAACDecHandle handle;
protected:
  Packet * in;
  Packet * out;
public:
  AacDecoder(AudioEngine*);
  FXuchar codec() const { return Codec::AAC; }
  FXbool flush();
  FXbool init(ConfigureEvent*);
  DecoderStatus process(Packet*);
  ~AacDecoder();
  };
}
#endif
#endif
