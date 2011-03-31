#ifndef READER_PLUGIN_H
#define READER_PLUGIN_H

namespace ap {

class Packet;

enum ReadStatus {
  ReadError,
  ReadOk,
  ReadDone,
  ReadInterrupted
  };

class ReaderPlugin {
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
  ReaderPlugin(AudioEngine*);
  virtual FXuchar format() const=0;
  virtual FXbool init()=0;
  virtual FXbool can_seek() const { return false; }
  virtual FXbool seek(FXdouble)=0;
  virtual ReadStatus process(Packet*);
  virtual ~ReaderPlugin();

  static ReaderPlugin* open(AudioEngine * engine,FXuint format);
  };
}
#endif
