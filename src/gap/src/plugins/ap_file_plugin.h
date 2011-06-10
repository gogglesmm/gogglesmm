#ifndef INPUT_FILE_DEVICE_H
#define INPUT_FILE_DEVICE_H

namespace ap {

class FileInput : public InputPlugin {
protected:
  FXFile   file;
  FXString filename;
private:
  FileInput(const FileInput&);
  FileInput &operator=(const FileInput&);
protected:
  FXlong read_raw(void*data,FXival ncount);
  FXInputHandle handle() const { return file.handle(); }
public:
  /// Constructor
  FileInput(InputThread*);

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
  virtual ~FileInput();
  };

}
#endif
