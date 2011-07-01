#include "ap_defs.h"
#include "ap_utils.h"
#include "ap_event.h"
#include "ap_pipe.h"
#include "ap_format.h"
#include "ap_memory_buffer.h"
#include "ap_input_plugin.h"
#include "ap_event_queue.h"
#include "ap_thread_queue.h"
#include "ap_packet.h"
#include "ap_engine.h"
#include "ap_thread.h"
#include "ap_thread_queue.h"
#include "ap_input_thread.h"
#include "ap_http_plugin.h"


#ifndef WIN32
#include <unistd.h> // for close()
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h> // for getaddrinfo()
#endif

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

using namespace ap;

namespace ap {


static FXbool ap_set_nonblocking(FXint fd) {
  FXint flags;

  flags = fcntl(fd, F_GETFL);
  if (flags==-1) return false;

  flags |= O_NONBLOCK;

  if (fcntl(fd,F_SETFL,flags)==-1)
    return false;

  return true;
  }

static FXbool ap_set_closeonexec(FXint fd) {
  FXint flags;

  flags = fcntl(fd, F_GETFD);
  if (flags==-1) return false;

  flags |= FD_CLOEXEC;

  if (fcntl(fd,F_SETFD,flags)==-1)
    return false;

  return true;
  }

#if defined(SO_NOSIGPIPE)
static FXbool ap_set_nosignal(FXint fd) {
  int nosignal=1;
  socklen_t len=sizeof(nosignal);
  if (setsockopt(fd,SOL_SOCKET,SO_NOSIGPIPE,&nosignal,len)==0)
    return true;
  else
    return false;
  }
#else
static FXbool ap_set_nosignal(FXint)  {
  return true;
  }
#endif

HttpInput::HttpInput(InputThread *i) : InputPlugin(i,1024),device(BadHandle),
  content_type(Format::Unknown),
  content_length(-1),
  content_position(0),
  icy_interval(0),
  icy_count(0){
  }

HttpInput::~HttpInput() {
  close();
  }

void HttpInput::close() {
  if (device!=BadHandle) {
    shutdown(device,SHUT_RDWR); /// don't care if this fails.
    ::close(device);
    device=BadHandle;
    }

  /// Reset Headers
  content_type=Format::Unknown;
  content_length=-1;
  content_position=0;
  icy_interval=0;
  icy_count=0;
  }


FXival HttpInput::write_raw(void*data,FXival count){
  FXival nwritten;
  do{
    nwritten=send(device,data,count,MSG_NOSIGNAL);
    }
  while(nwritten<0 && errno==EINTR);

  if (nwritten<0) {
    if (errno==EAGAIN || errno==EWOULDBLOCK)
      return -2;
    else
      fxmessage("[http] %s\n",strerror(errno));
    }
  else if (nwritten==0) {
    close();
    }
  return nwritten;
  }


FXival HttpInput::read_raw(void* data,FXival count){
  if (device==BadHandle)
    return 0;

  FXival nread=-1;
  do{
    nread=::recv(device,data,count,MSG_NOSIGNAL);
    }
  while(nread<0 && errno==EINTR);

  /// Block or not
  if (nread<0) {
    if (errno==EAGAIN || errno==EWOULDBLOCK) {
      return -2;
      }
    else
      fxmessage("[http] %s\n",strerror(errno));
    }
  else if (nread==0) {
    close();
    }
  return nread;
  }





FXbool HttpInput::write(const FXString & data) {
  FXival nwritten;
  FXival ncount=data.length();
  FXchar * buffer = (FXchar*)data.text();
  while(ncount>0) {
    nwritten=write_raw(buffer,ncount);
    if (__likely(nwritten>0)) {
      buffer+=nwritten;
      ncount-=nwritten;
      }
    else if (nwritten==0) {
      return false;
      }
    else if (nwritten==-2) {
      if (!ap_wait_write(input->getFifoHandle(),handle()))
        return false;
      }
    else {
      return false;
      }
    }
  return true;
  }



static FXInputHandle try_connect(FXInputHandle fifo,struct addrinfo * item) {
  FXint  device = socket(item->ai_family,item->ai_socktype,item->ai_protocol);
  if (device==BadHandle) {
    return BadHandle;
    }

  if (!ap_set_nonblocking(device) || !ap_set_closeonexec(device)) {
    ::close(device);
    return BadHandle;
    }

  ap_set_nosignal(device);

  if (connect(device,item->ai_addr,item->ai_addrlen)==0)
    return device;

  if (errno==EINPROGRESS || errno==EINTR || errno==EWOULDBLOCK) {
    if (ap_wait_write(fifo,device,TIME_SEC(30))) {
      int socket_error=0;
      socklen_t socket_length=sizeof(socket_error);
      if (getsockopt(device,SOL_SOCKET,SO_ERROR,&socket_error,&socket_length)==0 && socket_error==0)
        return device;
      }
    }

  ::close(device);
  return BadHandle;
  }

FXbool HttpInput::open(const FXString & hostname,FXint port) {
  struct addrinfo   hints;
  struct addrinfo * list=NULL;
  struct addrinfo * item=NULL;

  memset(&hints,0,sizeof(struct addrinfo));
  hints.ai_family=AF_UNSPEC;
  hints.ai_socktype=SOCK_STREAM;
  hints.ai_flags|=(AI_NUMERICSERV|AI_ADDRCONFIG);

  FXint result=getaddrinfo(hostname.text(),FXString::value(port).text(),&hints,&list);
  if (result) {
    fxmessage("[http] getaddrinfo: %s\n",gai_strerror(result));
    return false;
    }

  for (item=list;item;item=item->ai_next){
    device=try_connect(input->getFifoHandle(),item);
    if (device!=BadHandle) {
      freeaddrinfo(list);
      return true;
      }
    }
  if (list)
    freeaddrinfo(list);
  return false;
  }

/// Maybe move to memory stream
FXbool HttpInput::next_header(FXString & header) {
  FXchar * buf  = (FXchar*)buffer.data_ptr;
  FXint    len  = buffer.size();
  FXint    size=0;
  FXint    end=0;
  FXint    i,h;
  FXbool   found = false;


  for (i=0;i<len;i++) {
    //fxmessage("buf[%d]=%c\n",i,buf[i]);
    if (buf[i]=='\n') {

      /// header may continue on the next line, so check first byte of next line
      if (size>0) {

        /// Not enough data, so we need to fetch more.
        if ((i+1)>=len)
          return false;

        /// Header continues, keep reading
        if (buf[i+1]==' ' || buf[i+1]=='\t')
          continue;
        }
      end=i+1;
      found=true;
      break;
      }
    else if (buf[i]!='\r') {
      size++;
      }
    }

  if (found) {
    if (size) {
      header.length(size);
      for (i=0,h=0;h<size;i++) {
        if (buf[i]=='\r' || buf[i]=='\n')
          continue;
        FXASSERT(h<size);
        header[h++]=buf[i];
        }
      }
    else {
      header.clear();
      }
    if (end>0) buffer.read(end);
    }
  return found;
  }

FXbool HttpInput::parse_response() {
  FXString header,location;
  FXbool eoh=false;
  FXbool redirect=false;

  FXint http_code=0;
  FXint http_major_version=0;
  FXint http_minor_version=0;

  /// First get response code
  do {
    /// Get some bytes
    if (fillBuffer(256)==-1)
      return false;
    }
  while(!next_header(header));

  fxmessage("[http] %s\n",header.text());

  if ( header.scan("HTTP/%d.%d %d",&http_major_version,&http_minor_version,&http_code)!=3 &&
       header.scan("ICY %d",&http_code)!=1 ){
    fxmessage("[http] invalid http response: %s\n",header.text());
    return false;
    }

  if (http_code>=300 && http_code<400) {
    if (http_code==301 || http_code==302 || http_code==303 || http_code==307){
      redirect=true;
      }
    else {
      fxmessage("[http] unhandled redirect (%d)\n",http_code);
      return false;
      }
    }
  else if (http_code>=400 && http_code<500) {
    if (http_code==404)
      fxmessage("[http] 404!!\n");
    else
      fxmessage("[http] client error (%d)\n",http_code);
    return false;
    }
  else if (http_code<200 || http_code>=300){
    fxmessage("[http] unhandled error (%d)\n",http_code);
    return false;
    }

  while(!eoh) {

    while(next_header(header)){
      if (header.empty()) {
        eoh=true;
        break;
        }
      else if (comparecase(header,"Content-Type:",13)==0) {
        FXString type = header.after(':').trim();
        content_type=ap_format_from_mime(type);
        }
      else if (comparecase(header,"Content-Length:",15)==0) {
        content_length=header.after(':').trim().toInt();
        }
      else if (comparecase(header,"Content-Encoding:",17)==0) {
        }
      else if (comparecase(header,"icy-metaint:",12)==0) {
        icy_count = icy_interval = header.after(':').trim().toInt();
        }
      else if (comparecase(header,"icy-genre:",10)==0) {
        icy_meta_genre = header.after(':').trim();
        }
      else if (comparecase(header,"icy-name:",9)==0) {
        icy_meta_name  = header.after(':').trim();
        }
      else if (comparecase(header,"Location:",9)==0){
        location=header.after(':').trim();
        }
      fxmessage("%s\n",header.text());
      }

    if (eoh) break;

    /// Get more bytes
    if (fillBuffer(256)<=0)
      return false;
    }

  /// Handle redirects
  if (redirect) {
    if (location.empty())
      return false;

    fxmessage("redirect: %s\n",location.text());
    close();
    buffer.clear();
    return open(location);
    }



  return true;
  }



void HttpInput::icy_parse(const FXString & buffer) {
  FXString title = buffer.after('=').before(';');
/*
  MetaInfo * meta = new MetaInfo();
  meta->title = title;
  input->post(meta);
*/  
  }


FXival HttpInput::icy_read(void*data,FXival count){
  FXchar * out = (FXchar*)data;
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


FXival HttpInput::read(void*data,FXival count){
  FXint n;

  /// Don't read past content
  if (content_length>0) {
    if (content_position>=content_length)
      return 0;
    else
      count=FXMIN((content_length-content_position),count);
    }

  if (icy_interval)
    n=icy_read(data,count);
  else
    n=InputPlugin::read(data,count);

  if (n>0)
    content_position+=n;

  return n;
  }




















FXbool HttpInput::open(const FXString & uri) {
  FXString host  = FXURL::host(uri);
  FXString path  = FXURL::path(uri);
  FXString query = FXURL::query(uri);
  FXint    port  = FXURL::port(uri);

  if (!query.empty())
    path+="?"+query;

  if (port==0) port=80;

  if (!open(host,port)) {
    fxmessage("Failed to open\n");
    return false;
    }

  if (path.empty()) path="/";

  FXString request = FXString::value("GET %s HTTP/1.1\r\n"
                                     "Host: %s\r\n"
                                     "User-agent: libgaplayer/%d.%d\r\n"

                                     /// For ice/shout cast this will give us metadata in stream.
                                     "Icy-MetaData: 1\r\n"
                                     "Accept: */*\r\n"

                                     "\r\n"
                                     ,path.text()
                                     ,host.text()
                                     ,0,1
                                     );

  fxmessage("request: %s\n",request.text());

  /// Send request
  if (!write(request)) {
    fxmessage("failed to send request\n");
    return false;
    }

  /// Parse response
  if (!parse_response())
    return false;


  if (content_type==Format::Unknown) {
    FXString extension = FXPath::extension(path);
    content_type=ap_format_from_extension(extension);
    }

  fxmessage("success\n");
  return true;
  }

FXival HttpInput::position(FXlong offset,FXuint from) {
  fxmessage("position %ld %ld\n",offset,content_position);
  FXASSERT(from==FXIO::Current && offset>0);
  if (from==FXIO::End) {
    fxmessage("cannot seek from end\n");
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
      fxmessage("cannot seek backwards\n");
      }
    return -1;
    }
  else if (offset<0) {
    fxmessage("cannot seek backwards\n");
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




FXival HttpInput::position() const {
  return content_position;
  }

FXlong HttpInput::size() {
  return content_length;
  }

FXbool HttpInput::eof()  {
  if ((content_length>0 && content_position>=content_length) ||
      (buffer.size()==0 && device==BadHandle)) {
    return true;
    }
  else
    return false;
  }

FXbool HttpInput::serial() const {
  return true;
  }

FXuint HttpInput::plugin() const {
  return content_type;
  }


}
