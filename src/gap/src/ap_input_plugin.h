#ifndef INPUT_PLUGIN_H
#define INPUT_PLUGIN_H

namespace ap {

class InputPlugin {
protected:
  FXInputHandle fifo;
  MemoryStream  buffer;
private:
  InputPlugin(const InputPlugin&);
  InputPlugin &operator=(const InputPlugin&);
protected:
  virtual FXlong read_raw(void*data,FXival ncount)=0;
  virtual FXInputHandle handle() const { return BadHandle; }
protected:


  /// Read block of bytes directly from input
  FXlong readBlock(void*data,FXival ncount,FXbool wait=true);
  FXival fillBuffer(FXival);
protected:
  InputPlugin(FXInputHandle f,FXival size);
public:
  InputPlugin(FXInputHandle f);

  /// Read ncount bytes
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
