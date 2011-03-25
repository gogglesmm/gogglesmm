#ifndef INPUT_PLUGIN_H
#define INPUT_PLUGIN_H

namespace ap {

class Packet;

enum InputStatus {
  InputError,
  InputOk,
  InputDone,
  InputInterrupted
  };

class InputPlugin {
public:
  AudioEngine * engine;
  AudioFormat   af;
protected:
  FXuchar flags;
  FXlong  stream_length;      /// Length of stream in samples
protected:
  enum {
    FLAG_PARSED = 0x1,
    };
public:
  InputPlugin(AudioEngine*);
  virtual FXuchar format() const=0;
  virtual FXbool init()=0;
  virtual FXbool can_seek() const { return false; }
  virtual FXbool seek(FXdouble)=0;
  virtual InputStatus process(Packet*);
  virtual ~InputPlugin();

  static InputPlugin* open(AudioEngine * engine,const FXString & uri);
  };
}
#endif
