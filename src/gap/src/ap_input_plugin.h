#ifndef INPUT_PLUGIN_H
#define INPUT_PLUGIN_H

namespace ap {

class InputThread;

enum {
  AP_IO_ERROR = -1, // error occured
  AP_IO_BLOCK = -2  // nothing available
  };

class InputPlugin {
protected:
  InputThread*  input;
  MemoryBuffer  buffer;
private:
  InputPlugin(const InputPlugin&);
  InputPlugin &operator=(const InputPlugin&);
protected:
  virtual FXival        io_read(void*,FXival)=0;
  virtual FXival        io_read_block(void*,FXival);
  virtual FXival        io_write(const void*,FXival) { return -1; }
  virtual FXival        io_write_block(const void*,FXival);
  virtual FXival        io_buffer(FXival);
  FXbool                io_wait_read();
  FXbool                io_wait_write();
  virtual FXInputHandle io_handle() const { return BadHandle; }
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
