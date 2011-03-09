#ifndef DECODER_H
#define DECODER_H

class AudioEngine;
class DecoderPlugin;
class Packet;
class ConfigureEvent;

class DecoderThread : public EngineThread {
protected:
  PacketPool      packetpool;
  DecoderPlugin * plugin;
protected:
  FXuint stream;
protected:
  void configure(ConfigureEvent*);
public:
  DecoderThread(AudioEngine*);

  FXuchar codec() const { return Codec::Invalid; }

  virtual FXint run();
  virtual FXbool init();
  virtual void free();

  Event * wait_for_packet();
  Packet * get_output_packet();
  Packet * get_decoder_packet();
  virtual ~DecoderThread();
  };

#endif


