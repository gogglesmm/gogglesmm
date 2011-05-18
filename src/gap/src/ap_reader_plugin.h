#ifndef READER_PLUGIN_H
#define READER_PLUGIN_H

namespace ap {

class Packet;

enum ReadStatus {
  ReadError,
  ReadOk,
  ReadDone,
  ReadInterrupted,
  ReadRedirect
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

  /// Init plugin
  virtual FXbool init()=0;

  /// Format type
  virtual FXuchar format() const=0;

  /// Return redirect lsit
  virtual FXbool redirect(FXStringList&) { return false; }

  /// Return whether plugin can seek
  virtual FXbool can_seek() const { return false; }

    /// Seek
  virtual FXbool seek(FXdouble) { return false; }

  /// Process Input
  virtual ReadStatus process(Packet*);

  /// Destructor
  virtual ~ReaderPlugin();

  /// Open plugin for given format
  static ReaderPlugin* open(AudioEngine * engine,FXuint format);
  };




class TextReader : public ReaderPlugin {
protected:
  FXString textbuffer;
public:
  TextReader(AudioEngine*);  
  FXbool init();
  ReadStatus process(Packet*);
  virtual ~TextReader();
  };









}
#endif
