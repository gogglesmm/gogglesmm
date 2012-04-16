#ifdef HAVE_SMB_PLUGIN
#ifndef INPUT_SMB_DEVICE_H
#define INPUT_SMB_DEVICE_H

namespace ap {

class SMBInput : public InputPlugin {
protected:
  FXString      filename;
  FXInputHandle fd;
private:
  SMBInput(const SMBInput&);
  SMBInput &operator=(const SMBInput&);
protected:
  FXival        io_read(void*,FXival);
  FXInputHandle io_handle() const { return fd; }
public:
  /// Constructor
  SMBInput(InputThread*);

  FXbool open(const FXString & uri);

  /// Set Position
  FXlong position(FXlong offset,FXuint from);

  /// Get Position
  FXlong position() const;

  /// Size
  FXlong size();

  /// End of Input
  FXbool eof();

  /// Serial
  FXbool serial() const;

  /// Get plugin type
  FXuint plugin() const;

  /// Destructor
  virtual ~SMBInput();
  };

}
#endif
#endif
