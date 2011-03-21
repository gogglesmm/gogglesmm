#ifndef WAV_H
#define WAV_H

namespace ap {

class WavInput : public InputPlugin {
protected:
  FXuint datasize;    // size of the data section
  FXlong input_start;
protected:
  InputStatus parse();
public:
  WavInput(AudioEngine*);
  FXbool init();
  InputStatus process(Packet*);

  FXbool can_seek() const;
  FXbool seek(FXdouble);
  virtual ~WavInput();
  };

}
#endif
