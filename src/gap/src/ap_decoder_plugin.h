#ifndef DECODER_PLUGIN_H
#define DECODER_PLUGIN_H

class ConfigureEvent;
class DecoderPacket;

enum DecoderStatus {
  DecoderError,
  DecoderOk,
  DecoderDone,
  DecoderInterrupted
  };

class DecoderPlugin {
protected:
  AudioEngine * engine;
  AudioFormat   af;
public:
public:
  DecoderPlugin(AudioEngine*);
  
  virtual FXuchar codec() const { return Codec::Invalid; }

  virtual FXbool init(ConfigureEvent*)=0;

  virtual DecoderStatus process(Packet*)=0;

  virtual FXbool flush()=0;

  static DecoderPlugin* open(AudioEngine * engine,FXuchar codec);

  virtual ~DecoderPlugin() {}
  };

#endif


