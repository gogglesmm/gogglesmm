#ifndef INPUT_H
#define INPUT_H

namespace ap {

class AudioEngine;
class ReaderPlugin;
class InputPlugin;



class InputThread : public EngineThread {
protected:
  InputPlugin*  input;
  ReaderPlugin* reader;
  FXString      url;
  FXuint        streamid;
  PacketPool    packetpool;
  FXbool        use_mmap;
protected:
//  FXIO * open_io(const FXString & uri);
//  FXIO * open_file(const FXString & uri);
//#ifdef HAVE_CDDA_PLUGIN
//  FXIO * open_cdda(const FXString & uri);
//#endif
  InputPlugin  * open_input(const FXString & uri);
  ReaderPlugin * open_reader();
protected:
  enum {
    StateIdle       = 0,
    StateProcessing = 1
    };
  FXuchar state;
  void set_state(FXuchar s,FXbool notify=false);
public:
  /// Constructor
  InputThread(AudioEngine*);
  virtual FXint run();
  virtual FXbool init();
  virtual void free();

  Event * wait_for_event();
  Event * wait_for_packet();
  Packet * get_packet();

//  DecoderPacket * get_decoder_packet();


  FXival preview(void*data,FXival ncount);
  FXival read(void*data,FXival ncount);
  FXlong position(FXlong offset,FXuint from);
  FXlong position() const;
  FXlong size() const;
  FXbool eof() const;
  FXbool serial() const;


  void ctrl_open_inputs(const FXStringList & url);
  void ctrl_open_input(const FXString & url);
  void ctrl_close_input(FXbool notify=false);
  void ctrl_flush(FXbool close=false);
  void ctrl_seek(FXdouble pos);
  void ctrl_eos();


  /// Destructor
  virtual ~InputThread();
  };
}
#endif
