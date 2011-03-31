#include "ap_defs.h"
#include "ap_utils.h"
#include "ap_event.h"
#include "ap_pipe.h"
#include "ap_format.h"
#include "ap_memory_buffer.h"
#include "ap_input_plugin.h"
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


static FXbool ap_set_nosignal(FXint fd) {
#if defined(SO_NOSIGPIPE)
  int nosignal=1;
  socklen_t len=sizeof(nosignal);
  if (setsockopt(fd,SOL_SOCKET,SO_NOSIGPIPE,&nosignal,len)==0)
    return true;
  else
    return false;
#else
  return true;
#endif
  }


HttpInput::HttpInput(FXInputHandle f) : InputPlugin(f),device(BadHandle),
  content_type(Format::Unknown),
  content_length(-1),
  content_position(0) {
  }

HttpInput::~HttpInput() {
  if (device!=BadHandle){
    ::close(device);
    device=BadHandle;
    }
  }


FXival HttpInput::fill_buffer(FXuval count) {
  buffer.reserve(count);
  FXival nread;
  FXival ncount=count;
  FXuchar * buf = buffer.ptr();
  while(ncount>0) {
    nread=read_raw(buf,ncount);
    if (__likely(nread>0)) {
      buf+=nread;
      ncount-=nread;
      }
    else if (nread==0) {
      buffer.wrote(count-ncount);
      return count-ncount;
      }
    else if (nread==-2){
      if (!ap_wait_read(fifo,device))
        return -1;
      }
    else {
      return -1;
      }
    }
  buffer.wrote(count);
  return count;
  }


FXival HttpInput::write_raw(void*data,FXival count){
  FXival nwritten=-1;
  do{
    nwritten=send(device,data,count,MSG_NOSIGNAL);
    }
  while(nwritten<0 && errno==EINTR);
  return nwritten;
  }


FXival HttpInput::read_raw(void* data,FXival count){
  FXival nread=-1;
 // FXint flags=0;
  errno=0;
  do{
    nread=::read(device,data,count);
    }
  while(nread<0 && errno==EINTR);

  /// Block
  if (nread<0 && (errno==EAGAIN || errno==EWOULDBLOCK)){
    return -2;
    }
  return nread;
  }

FXival HttpInput::write(void*data,FXival count) {
  FXival nwritten;
  FXival ncount=count;
  FXchar * buffer = (FXchar *)data;
  while(ncount>0) {
    nwritten=write_raw(buffer,ncount);
    if (__likely(nwritten>0)) {
      buffer+=nwritten;
      ncount-=nwritten;
      }
    else if (nwritten==0) {
      return count-ncount;
      }
    else {
      if (errno==EAGAIN || errno==EWOULDBLOCK){
        if (!ap_wait_write(fifo,handle()))
          return -1;
        }
      else {
        return -1;
        }
      }
    }
  return count;
  }


#if 0
static FXint HttpInput::ap_connect(FXInputHandle & input,struct addrinfo * entry) {

  /// Connect Socket
  FXint result = connect(device,entry->ai_addr,entry->ai_addrlen);
  if (result!=0) {
    fxmessage("Failed to connect\n");
    ::close(device);
    return BadHandle;
    }

  if (!ap_set_nonblock(device) || !ap_set_closeonexec(device))
    ::close(device);
    return BadHandle;
    }


  /// Succes
  return device;
  }
#endif


static FXInputHandle try_connect(FXInputHandle fifo,struct addrinfo * item) {
  FXint  device = socket(item->ai_family,item->ai_socktype,item->ai_protocol);
  if (device==BadHandle) {
    fxmessage("Failed to create socket\n");
    return BadHandle;
    }
    
  if (!ap_set_nonblocking(device) || !ap_set_closeonexec(device)) {
    ::close(device);
    return BadHandle;
    }

  ap_set_nosignal(device);

  FXint result = connect(device,item->ai_addr,item->ai_addrlen);
  if (result==-1) {        
    if (errno==EINPROGRESS || errno==EINTR || errno==EWOULDBLOCK) {
      if (ap_wait_write(fifo,device)) {   
        /// Check error after select
        int socket_error=0;
        socklen_t socket_length=sizeof(socket_error);        
        if (getsockopt(device,SOL_SOCKET,SO_ERROR,&socket_error,&socket_length)==0 && socket_error==0)
          return device;
        }
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

  FXint result=getaddrinfo(hostname.text(),FXString::value(port).text(),&hints,&list);
  if (result) {
    fxmessage("getaddrinfo failed\n");
    return false;
    }
  
  for (item=list;item;item=item->ai_next){
    device=try_connect(fifo,item);
    if (device!=BadHandle) {
      freeaddrinfo(list);
      return true;
      }
    }       
  if (list) 
    freeaddrinfo(list);
  return false;
  }


FXbool HttpInput::next_header(FXString & header) {
  FXchar * buf  = (FXchar*)buffer.data_ptr;
  FXint    len  = buffer.size();
  FXint    size=0;
  FXint    end=0;
  FXint    i,h;
  FXbool   found = false;

  header.clear();

  for (i=0;i<len;i++) {
    //fxmessage("buf[%d]=%c\n",i,buf[i]);
    if (buf[i]=='\n') {

      /// not enough data
      if ((i+1)>=len)
        return false;

      /// header continues on line below
      if (size>0 && (buf[i+1]==' ' || buf[i+1]=='\t'))
        continue;

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
    if (end>0) buffer.read(end);
    }
  return found;
  }

FXbool HttpInput::parse_response_headers() {
  FXString header;
  FXbool eoh=false;

  while(!eoh) {

    /// Get some bytes
    if (fill_buffer(256)==-1)
      return false;

    while(next_header(header)){
      if (header.empty()) {
        eoh=true;
        break;
        }
      else if (comparecase(header,"Content-Type:",13)==0) {
        FXString type = header.after(':').trim();
        if (comparecase(type,"audio/mpeg")==0) {
          content_type=Format::MP3;
          }
        else if (comparecase(type,"audio/ogg")==0){
          content_type=Format::OGG;
          }
        }
      else if (comparecase(header,"Content-Length:",15)==0) {
        FXString length = header.after(':').trim();
        content_length=length.toInt();
        if (content_length<=0) content_length=-1;
        }
      else if (comparecase(header,"Content-Encoding:",17)==0) {
        }
      fxmessage("%s\n",header.text());
      }
    }
  return true;
  }











FXival HttpInput::read(void* data,FXival count){
  if (count>buffer.size()) {
    if (fill_buffer(count-buffer.size())==-1)
      return -1;
    }
  FXival nread = buffer.read(data,count);
  content_position+=nread;
  return nread;
  }


FXbool HttpInput::open(const FXString & uri) {
  FXString host = FXURL::host(uri);
  FXString path = FXURL::path(uri);
  FXint    port = FXURL::port(uri);
  if (port==0) port=80;

  if (!open(host,port)) {
    fxmessage("Failed to open\n");
    return false;
    }

  if (path.empty()) path="/";

  FXString request = FXString::value("GET %s HTTP/1.1\r\n"
                                     "Host: %s\r\n"
                                     "User-agent: libgap/%d.%d\r\n"
                                     "\r\n"
                                     ,path.text()
                                     ,host.text()
                                     ,0,1
                                     );


  /// Wait for reply
//  if (!ap_wait_write(fifo,device)){
//    fxmessage("waiting for write failed\n");
//    return false;
//    }

  fxmessage("request: %s\n",request.text());

  /// Send request
  if (write(request.text(),request.length())==-1) {
    fxmessage("failed to send request\n");
    return false;
    }

  /// Parse headers
  if (!parse_response_headers())
    return false;

  fxmessage("success\n");
  return true;
  }

FXival HttpInput::position(FXlong offset,FXuint from) {
  FXASSERT(from==FXIO::Current && offset>0);
  if (from==FXIO::End) {
    fxmessage("cannot seek from end\n");
    return -1;
    }
  else if (from==FXIO::Begin) {
    fxmessage("cannot seek from begin\n");
    return -1;
    }
  else if (offset<0) {
    fxmessage("cannot seek backwards\n");
    return -1;
    }
  else {
    while(offset) {
      if (buffer.size()==0) {
        FXint r = fill_buffer(256);
        if (r==-1) return -1;
        else if (r==0) return content_position;
        }
      FXint n=FXMIN(offset,buffer.size());
      buffer.read(n);
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
  if (buffer.size()==0 && device==BadHandle)
    return true;
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
