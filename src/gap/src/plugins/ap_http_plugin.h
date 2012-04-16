#ifndef INPUT_HTTP_DEVICE_H
#define INPUT_HTTP_DEVICE_H

namespace ap {

class HttpInputClient;

class HttpInput : public InputPlugin {
friend class HttpInputClient;
protected:
  HttpInputClient* client;
protected:
  FXlong        content_position;
  FXuint        content_type;
  FXint         icy_interval;
  FXint         icy_count;
private:
  HttpInput(const HttpInput&);
  HttpInput &operator=(const HttpInput&);
protected:
  FXival        io_read(void*,FXival);
  FXival        io_write(const void*,FXival);
  FXInputHandle io_handle() const;
protected:
  void          check_headers();
  FXival        icy_read(void*,FXival);
  void          icy_parse(const FXString &);
  void          close();
public:
  HttpInput(InputThread*);

  /// Open uri
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

  /// Read
  FXival read(void*,FXival);

  ~HttpInput();
  };
}
#endif
