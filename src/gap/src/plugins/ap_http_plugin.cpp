#include "ap_defs.h"
#include "ap_utils.h"
#include "ap_event.h"
#include "ap_pipe.h"
#include "ap_format.h"
#include "ap_buffer.h"
#include "ap_input_plugin.h"
#include "ap_event_queue.h"
#include "ap_thread_queue.h"
#include "ap_packet.h"
#include "ap_engine.h"
#include "ap_thread.h"
#include "ap_thread_queue.h"
#include "ap_input_thread.h"
#include "ap_http.h"
#include "ap_http_plugin.h"


#ifndef WIN32
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#endif

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

using namespace ap;

namespace ap {

class HttpInputClient : public HttpClient {
friend class HttpInput;
protected:
  HttpInput * input;
protected:

  /// called from input->io_read
  FXival io_read(void*ptr,FXival count) {
    FXival nread=-1;
    do{
      nread=::recv(device,ptr,count,MSG_NOSIGNAL);
      }
    while(nread<0 && errno==EINTR);

    /// Block or not
    if (nread<0) {
      if (errno==EAGAIN || errno==EWOULDBLOCK) {
        return -2;
        }
      else
        GM_DEBUG_PRINT("[http] %s\n",strerror(errno));
      }
    else if (nread==0) {
      input->close(); // this indirectly calls ::reset()
      }
    return nread;
    }

  /// called from input->io_write
  FXival io_write(const void*ptr,FXival count) {
    FXival nwritten;
    do{
      nwritten=send(device,ptr,count,MSG_NOSIGNAL);
      }
    while(nwritten<0 && errno==EINTR);

    if (nwritten<0) {
      if (errno==EAGAIN || errno==EWOULDBLOCK)
        return -2;
      else
        GM_DEBUG_PRINT("[http] %s\n",strerror(errno));
      }
    else if (nwritten==0) {
      input->close(); // this indirectly calls ::reset()
      }
    return nwritten;
    }

  FXival io_buffer(FXival count) {
    return input->io_buffer(count);
    }

  FXbool io_wait_write() {
    return input->io_wait_write();
    }

  FXival read(void*ptr,FXival count) {
    return input->InputPlugin::read(ptr,count);
    }

  FXbool write(const void*ptr,FXival count) {
    return (input->io_write_block(ptr,count)==count);
    }

public:

  // Constructor
  HttpInputClient(HttpInput* http) : HttpClient(NULL),input(http) {
    buffer = (&input->buffer);
    options|=UseNonBlock;
    }

  // Destructor
  ~HttpInputClient() {
    buffer = NULL;
    }

  FXInputHandle io_handle() const { return device; }
  };

//-------------------------------------------------------------------
//-------------------------------------------------------------------


HttpInput::HttpInput(InputThread * t) : InputPlugin(t,1024),
  content_position(0),
  content_type(Format::Unknown),
  icy_interval(0),
  icy_count(0){
  client = new HttpInputClient(this);
  }

HttpInput::~HttpInput() {
  close();
  delete client;
  }


void HttpInput::check_headers() {
  FXString * field = NULL;
  field = (FXString*) client->headers.find("icy-metaint");
  if (field) icy_count = icy_interval = field->toInt();

  field = (FXString*) client->headers.find("content-type");
  if (field) content_type = ap_format_from_mime(field->before(';'));

/*
       else if (comparecase(header,"icy-genre:",10)==0) {
        icy_meta_genre = header.after(':').trim();
        }
      else if (comparecase(header,"icy-name:",9)==0) {
        icy_meta_name  = header.after(':').trim();
*/
  }

/// Open uri
FXbool HttpInput::open(const FXString & uri) {
  FXString url = uri;
  FXString headers = FXString::value("User-agent: libgaplayer/%d.%d\r\n"
                                     "Icy-MetaData: 1\r\n"
                                     "Accept: */*\r\n",0,1);

  if (client->basic("GET",uri,headers)) {
    if (client->status.code==HTTP_OK){

      check_headers();

      /// Try to guess content from uri.
      if (content_type==Format::Unknown)
        content_type=ap_format_from_extension(FXPath::extension(uri));

      return true;
      }
    }
  return false;
  }

void HttpInput::close() {
  GM_DEBUG_PRINT("close http\n");
  client->reset(true); // force close and reset
  content_position=0;
  content_type=Format::Unknown;
  icy_interval=0;
  icy_count=0;
  }


/// Set Position
FXlong HttpInput::position(FXlong offset,FXuint from){
  GM_DEBUG_PRINT("position %ld %ld\n",offset,content_position);
  FXASSERT(from==FXIO::Current && offset>0);
  if (from==FXIO::End) {
    GM_DEBUG_PRINT("cannot seek from end\n");
    return -1;
    }
  else if (from==FXIO::Begin) {
    if (offset>content_position) {
      offset-=content_position;
      FXchar b;FXival n;
      while(offset) {
        n=InputPlugin::read(&b,1);
        if (n==-1) return -1;
        offset-=n;
        content_position+=n;
        }
      return content_position;
      }
    else {
      GM_DEBUG_PRINT("cannot seek backwards\n");
      }
    return -1;
    }
  else if (offset<0) {
    GM_DEBUG_PRINT("cannot seek backwards\n");
    return -1;
    }
  else {
    FXchar b;FXival n;
    while(offset) {
      n=InputPlugin::read(&b,1);
      if (n==-1) return -1;
      offset-=n;
      content_position+=n;
      }
    return content_position;
    }
  return -1;
  }

/// Get Position
FXlong HttpInput::position() const {
  return content_position;
  }

/// Size
FXlong HttpInput::size(){
  return client->getContentLength();
  }

/// End of Input
FXbool HttpInput::eof() {
  if ((client->content_length>=0 && content_position>=client->content_length) ||
      (buffer.size()==0 && client->io_handle()==BadHandle))
    return true;
  else
    return false;
  }

/// Serial
FXbool HttpInput::serial() const {
  return true;
  }

/// Get plugin type
FXuint HttpInput::plugin() const {
  return content_type;
  }


FXival HttpInput::io_read(void*ptr,FXival count) {
  return client->io_read(ptr,count);
  }

FXival HttpInput::io_write(const void*ptr,FXival count) {
  return client->io_write(ptr,count);
  }

FXInputHandle HttpInput::io_handle() const {
  return client->io_handle();
  }

FXival HttpInput::read(void*ptr,FXival count){
  FXival n;

  /// Don't read past content
  if (client->content_length>=0) {
    if (content_position>=client->content_length)
      return 0;
    else
      count=FXMIN((client->content_length-content_position),count);
    }

  if (icy_interval)
    n=icy_read(ptr,count);
  else
    n=InputPlugin::read(ptr,count);

  if (n>0)
    content_position+=n;

  return n;
  }


void HttpInput::icy_parse(const FXString & str) {
  FXString title = str.after('=').before(';');
  MetaInfo * meta = new MetaInfo();
  meta->title = title;
  input->post(meta);
  }

FXival HttpInput::icy_read(void*ptr,FXival count){
  FXchar * out = static_cast<FXchar*>(ptr);
  FXival nread=0,n=0;
  if (icy_count<count) {

    /// Read up to icy buffer
    nread=InputPlugin::read(out,icy_count);
    if (__unlikely(nread!=icy_count)) {
      if (nread>0) {
        icy_count-=nread;
        }
      return nread;
      }

    // Adjust output
    out+=nread;
    count-=nread;

    /// Read icy buffer size
    FXuchar b=0;
    n=InputPlugin::read(&b,1);
    if (__unlikely(n!=1)) return -1;

    /// Read icy buffer
    if (b) {
      FXushort icy_size=((FXushort)b)*16;
      FXString icy_buffer;
      icy_buffer.length(icy_size);
      n=InputPlugin::read(&icy_buffer[0],icy_size);
      if (__unlikely(n!=icy_size)) return -1;
      icy_parse(icy_buffer);
      }

    /// reset icy count
    icy_count=icy_interval;

    /// Read remaining bytes
    n=InputPlugin::read(out,count);
    if (__unlikely(n!=count)) return -1;
    nread+=n;
    icy_count-=n;
    }
  else {
    nread=InputPlugin::read(out,count);
    if (__likely(nread>0)) {
      icy_count-=nread;
      }
    }
  return nread;
  }


 }
