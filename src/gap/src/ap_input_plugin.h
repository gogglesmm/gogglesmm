#ifndef INPUT_PLUGIN_H
#define INPUT_PLUGIN_H

namespace ap {

class InputPlugin {
protected:
  FXInputHandle fifo;
private:
  InputPlugin(const InputPlugin&);
  InputPlugin &operator=(const InputPlugin&);
protected:
  virtual FXlong read_raw(void*data,FXival ncount)=0;
  virtual FXInputHandle handle() const { return BadHandle; }
public:
  InputPlugin(FXInputHandle f);

  /// Read ncount bytes
  virtual FXival read(void*data,FXival ncount);

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
