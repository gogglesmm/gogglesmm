#ifndef INPUT_HTTP_DEVICE_H
#define INPUT_HTTP_DEVICE_H

namespace ap {

class HttpInput : public InputPlugin {
protected:
  FXInputHandle device;
  MemoryStream  buffer;
  FXuint        content_type;
  FXlong        content_length;
  FXlong        content_position;
private:
  HttpInput(const HttpInput&);
  HttpInput &operator=(const HttpInput&);
protected:
  FXlong read_raw(void*,FXival);
  FXlong write_raw(void*,FXival);
  FXival fill_buffer(FXuval);
  FXival write(void*data,FXival ncount);
  FXbool open(const FXString & hostname,FXint port);
  FXbool next_header(FXString & header);
  FXbool parse_response_headers();
public:
  /// Constructor
  HttpInput(FXInputHandle);

  FXbool open(const FXString & uri);

  /// Read ncount bytes
  FXival read(void*data,FXival ncount);

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
  virtual ~HttpInput();
  };

}
#endif
