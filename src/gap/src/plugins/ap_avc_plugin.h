#ifdef HAVE_AVCODEC_PLUGIN
#ifndef AVC_PLUGIN_H
#define AVC_PLUGIN_H



/*
#ifdef _STDINT_H
#warning stdint defined
#undef _STDINT_H
#endif
#define __STDC_CONSTANT_MACROS
#include <stdint.h>
const FXlong test=UINT64_C(1000);
*/


extern "C" {
// Apparently we need this in order to use ffmpeg from c++.
// /usr/include/libavutil/common.h:168:47: error: ‘UINT64_C’ was not declared in this scope
#include <libavcodec/avcodec.h>
}

namespace ap {



class OutputPacket;

class AVDecoder : public DecoderPlugin {
protected:
  AVCodecContext * ctx;
protected:
  MemoryStream     buffer;
  MemoryBuffer     outbuf;
protected:
  Packet * out;
public:
  AVDecoder(AudioEngine*);
  FXuchar codec() const { return Codec::PCM; }
  FXbool flush();
  FXbool init(ConfigureEvent*);
  DecoderStatus process(Packet*);
  virtual ~AVDecoder();
  };
  
}  
#endif
#endif
