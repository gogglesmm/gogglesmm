#ifdef HAVE_FLAC_PLUGIN
#ifndef FLAC_H
#define FLAC_H

#include <stream_decoder.h>

namespace ap {

class FlacInput : public InputPlugin {
protected:
  FLAC__StreamDecoder * flac;
protected:
  static FLAC__StreamDecoderSeekStatus    flac_input_seek(const FLAC__StreamDecoder*,FLAC__uint64,void*);
  static FLAC__StreamDecoderTellStatus    flac_input_tell(const FLAC__StreamDecoder*,FLAC__uint64*,void*);
  static FLAC__StreamDecoderLengthStatus  flac_input_length(const FLAC__StreamDecoder*,FLAC__uint64*,void*);
  static FLAC__StreamDecoderWriteStatus   flac_input_write(const FLAC__StreamDecoder*,const FLAC__Frame*,const FLAC__int32*const[],void*);
  static FLAC__StreamDecoderReadStatus    flac_input_read(const FLAC__StreamDecoder*,FLAC__byte buffer[],size_t*,void*);
  static FLAC__bool                       flac_input_eof(const FLAC__StreamDecoder*,void*);
  static void                             flac_input_meta(const FLAC__StreamDecoder*,const FLAC__StreamMetadata*,void*);
  static void                             flac_input_error(const FLAC__StreamDecoder *, FLAC__StreamDecoderErrorStatus, void *);
protected:
  InputStatus parse();
public:
  FlacInput(AudioEngine*);
  FXbool init();
  FXbool seek(FXdouble);
  FXbool can_seek() const;
  InputStatus process(Packet*);
  ~FlacInput();
  };

class OutputPacket;

class FlacDecoder : public DecoderPlugin {
protected:
  FLAC__StreamDecoder * flac;
  FXint stream_length;
protected:
  Packet * in;
  Packet  * out;
protected:
  static FLAC__StreamDecoderWriteStatus   flac_decoder_write(const FLAC__StreamDecoder*,const FLAC__Frame*,const FLAC__int32*const[],void*);
  static FLAC__StreamDecoderReadStatus    flac_decoder_read(const FLAC__StreamDecoder*,FLAC__byte buffer[],size_t*,void*);
  static void                             flac_decoder_error(const FLAC__StreamDecoder *, FLAC__StreamDecoderErrorStatus, void *);
public:
  FlacDecoder(AudioEngine*);
  FXuchar codec() const { return Codec::FLAC; }
  FXbool flush();
  FXbool init(ConfigureEvent*);
  DecoderStatus process(Packet*);
  ~FlacDecoder();
  };
}

#endif
#endif
