#ifndef PCM_PLUGIN_H
#define PCM_PLUGIN_H

class OutputPacket;

class PCMDecoder : public DecoderPlugin {
protected:
  Packet * out;
public:
  PCMDecoder(AudioEngine*);
  FXuchar codec() const { return Codec::PCM; }
  FXbool flush();
  FXbool init(ConfigureEvent*);
  DecoderStatus process(Packet*);
  virtual ~PCMDecoder();
  };

#endif
