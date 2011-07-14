#ifndef INPUT_HTTP_DEVICE_H
#define INPUT_HTTP_DEVICE_H

struct addrinfo;

namespace ap {

class HttpInput : public InputPlugin {
protected:
  FXInputHandle device;
protected: /// Http
  FXuint        content_type;
  FXlong        content_length;
  FXlong        content_position;
protected: /// Icecast
  FXint         icy_interval;
  FXint         icy_count;
  FXString      icy_meta_genre;
  FXString      icy_meta_name;
private:
  HttpInput(const HttpInput&);
  HttpInput &operator=(const HttpInput&);
private:
  FXInputHandle open_connection(struct addrinfo*);
protected:
  FXlong read_raw(void*,FXival);
  FXlong write_raw(void*,FXival);
protected:
  FXival icy_read(void*,FXival);
  void icy_parse(const FXString & buffer);
  FXbool write(const FXString&);
  FXbool open(const FXString & hostname,FXint port);
  FXbool next_header(FXString & header);
  FXbool parse_response();

  void close();
protected:
  FXInputHandle handle() const { return device; }
public:
  /// Constructor
  HttpInput(InputThread*);

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
