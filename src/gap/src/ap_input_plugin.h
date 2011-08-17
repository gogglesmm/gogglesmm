#ifndef INPUT_PLUGIN_H
#define INPUT_PLUGIN_H

namespace ap {

class InputThread;

class InputPlugin {
protected:
  InputThread*  input;
  MemoryStream  buffer;
private:
  InputPlugin(const InputPlugin&);
  InputPlugin &operator=(const InputPlugin&);
protected:
  virtual FXival read_raw(void*data,FXival ncount)=0;
  virtual FXInputHandle handle() const { return BadHandle; }
protected:
  /// Read block of bytes directly from input
  FXival readBlock(void*data,FXival ncount,FXbool wait=true);
  FXival fillBuffer(FXival);


  FXbool wait_read();
  FXbool wait_read(FXInputHandle);
  FXbool wait_write();
  FXbool wait_write(FXInputHandle);
protected:
  InputPlugin(InputThread*,FXival size);
public:
  InputPlugin(InputThread*);

  /// Read ncount bytes, returns -1 for error, -2 for interrupted
  virtual FXival read(void*data,FXival ncount);

  //// Read ncount preview bytes. Position of stream doesn't change
  virtual FXival preview(void*data,FXival ncount);

  /// Set Position
  virtual FXlong position(FXlong offset,FXuint from)=0;

  /// Get Position
  virtual FXlong position() const=0;

  /// Size
  virtual FXlong size()=0;

  /// End of InputPlugin
  virtual FXbool eof()=0;

  /// Serial
  virtual FXbool serial() const=0;

  /// Get plugin type
  virtual FXuint plugin() const { return Format::Unknown; }

  /// Destructor
  virtual ~InputPlugin();
  };



}
#endif
