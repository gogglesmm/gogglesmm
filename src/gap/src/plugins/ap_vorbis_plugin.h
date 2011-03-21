#ifdef HAVE_VORBIS_PLUGIN
#ifndef OGG_H
#define OGG_H

#include <ogg/ogg.h>
#include <vorbis/codec.h>

namespace ap {

class VorbisDecoder : public DecoderPlugin{
protected:
  MemoryStream buffer;
protected:
  FXbool get_next_packet();
  FXbool is_vorbis_header();
protected:
  AudioEngine *     engine;
  vorbis_info       info;
  vorbis_comment    comment;
  vorbis_dsp_state  dsp;
  vorbis_block      block;
  ogg_packet        op;
  FXbool            has_info;
  FXbool            has_dsp;
  Packet *          out;
  FXint             stream_position;
public:
  VorbisDecoder(AudioEngine*);

  FXuchar codec() const { return Codec::Vorbis; }
  FXbool init(ConfigureEvent*);
  DecoderStatus process(Packet*);
  FXbool flush();

  virtual ~VorbisDecoder();
  };

}
#endif
#endif
