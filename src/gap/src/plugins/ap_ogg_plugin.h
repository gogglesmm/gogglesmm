#ifdef HAVE_OGG_PLUGIN
#ifndef OGG_INPUT_H
#define OGG_INPUT_H

#include <ogg/ogg.h>
#include <vorbis/codec.h>

class AudioEngine;

class OggStream;


struct OggInputState {
  FXbool has_stream;
  FXbool has_eos;
  FXbool has_page;
  FXbool has_packet;
  FXbool header_written;
  FXuint bytes_written;
  OggInputState() : has_stream(false),has_eos(false),has_page(false),has_packet(false),header_written(false),bytes_written(0) {}
  void reset() {has_stream=false;has_eos=false;has_page=false; has_packet=false; header_written=false;bytes_written=0; }
  };

class OggInput : public InputPlugin {
protected:
  enum {
    FLAG_VORBIS_HEADER_INFO    = 0x2,
    FLAG_VORBIS_HEADER_COMMENT = 0x4,
    FLAG_VORBIS_HEADER_BLOCK   = 0x8,
    };
protected:
  OggInputState    state;
  ogg_stream_state stream;
  ogg_sync_state   sync;
  ogg_page         page;
  ogg_packet       op;
protected:
  vorbis_info      vi;
  vorbis_comment   vc;
protected:
  Packet *        packet;
  FXint           ogg_packet_written;
  InputStatus     status;
  FXuchar         codec;
  FXlong          stream_start;
  FXlong          input_position;
protected:
  FXbool match_page();
  FXbool fetch_next_page();
  FXbool fetch_next_packet();
  void   submit_ogg_packet();
  void   check_vorbis_length(vorbis_info*);
  InputStatus parse();
  InputStatus parse_vorbis_stream();
public:
  OggInput(AudioEngine *);
  FXbool init();
  FXbool seek(FXdouble);
  FXbool can_seek() const;
  InputStatus process(Packet*);
  virtual ~OggInput();
  };
#endif
#endif
