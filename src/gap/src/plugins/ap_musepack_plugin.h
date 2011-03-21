#ifdef HAVE_MUSEPACK_PLUGIN
#ifndef AP_MUSEPACK_H
#define AP_MUSEPACK_H

#include <mpcdec/mpcdec.h>

namespace ap {


class AudioEngine;

/*
Due to the way musepack decoder is designed
the input plugin actually decodes the data.
*/
class MusepackInput : public InputPlugin {
protected:
  FXfloat              buffer[MPC_DECODER_BUFFER_LENGTH];
  FXuint               nframes;
  FXint                frame;

  FXlong               stream_position;
  Packet*              packet;
  mpc_streaminfo       si;
  mpc_reader_t         reader;
  mpc_decoder          decoder;
protected:
  InputStatus parse();
protected:
  static mpc_int32_t  mpc_input_read(void *t, void *ptr, mpc_int32_t size);
  static mpc_bool_t   mpc_input_seek(void *t, mpc_int32_t offset);
  static mpc_int32_t  mpc_input_tell(void *t);
  static mpc_int32_t  mpc_input_size(void *t);
  static mpc_bool_t   mpc_input_canseek(void*);
public:
  MusepackInput(AudioEngine*);
  FXbool init();
  FXbool can_seek() const;
  FXbool seek(FXdouble);
  InputStatus process(Packet*);
  };
}
#endif
#endif
