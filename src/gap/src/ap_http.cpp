#include "ap_defs.h"
#include "ap_buffer.h"
#include "ap_http.h"


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

#if FOXVERSION < FXVERSION(1,7,0)
#define APIntVal(x)  FXIntVal((x))
#define APStringVal FXStringVal
#else
#define APIntVal(x) ((x).toInt())
#define APStringVal FXString::value
#endif


#ifndef BadHandle
#ifdef WIN32
#define BadHandle INVALID_HANDLE_VALUE
#else
#define BadHandle -1
#endif
#endif


using namespace ap;

namespace ap {


FXint HttpStatus::type() const {
  if (code>=100 && code<200)
    return HTTP_RESPONSE_INFORMATIONAL;
  else if (code>=200 && code<300)
    return HTTP_RESPONSE_SUCCESS;
  else if (code>=300 && code<400)
    return HTTP_RESPONSE_REDIRECT;
  else if (code>=400 && code<500)
    return HTTP_RESPONSE_CLIENT_ERROR;
  else if (code>=500 && code<600)
    return HTTP_RESPONSE_SERVER_ERROR;
  else
    return HTTP_RESPONSE_FAILED;
  }


HttpResponse::HttpResponse() :
  buffer(NULL),
  content_length(-1),
  chunk_remaining(-1),
  flags(0){
  }

HttpResponse::~HttpResponse() {
  clear();
  }

void HttpResponse::clear() {
  flags=0;
  content_length=-1;
  chunk_remaining=-1;
  clear_headers();
  buffer->clear();
  }


// Read (at most) nbytes from buffer or source
FXival HttpResponse::read(void*ptr,FXival nbytes) {
  register FXchar * dest = static_cast<FXchar*>(ptr);
  FXival nread=0;
  while(nbytes) {
    if (buffer->size()) {
      FXival n = buffer->read(ptr,nbytes);
      nread+=n;
      nbytes-=n;
      dest+=n;
      }
    else {
      FXival n = io_read(dest,nbytes);
      if (n<=0) return (nread>0) ? nread : n;
      nread+=n;
      nbytes-=n;
      dest+=n;
      }
    }
  return nread;
  }

// Fill buffer with (at most) nbytes
FXival HttpResponse::io_buffer(FXival nbytes) {
  buffer->reserve(nbytes);
  FXival nread = io_read(buffer->ptr(),nbytes);
  if (nread>0) {
    buffer->wroteBytes(nread);
    }
  return nread;
  }


// Try parsing one HTTP header from buffer. If succesfull, returns
// headers. Mode can either be HEADER_SINGLE_LINE or HEADER_MULTIPLE_LINES,
// depending on whether headers may span multiple lines or not.
FXbool HttpResponse::parse_header(FXString & line,FXuint mode) {
  MemoryBuffer & b = *(buffer);
  FXint len=0,i,j,l;
  for (i=0;i<buffer->size()-1;i++){
    if (b[i]=='\r' && b[i+1]=='\n') {

      // If header cannot span multiple lines, we're done here.
      if (mode==HEADER_SINGLE_LINE) {
        if (i>0) line.assign((FXchar*)buffer->data(),i);
        buffer->readBytes(i+2);
        return true;
        }

      // check if header continues on the next line
      if (len>0) {

        // need more bytes
        if ((i+2)>=buffer->size())
            return false;

        /// header continues on next line
        if (b[i+2]==' ' || b[i+2]=='\t')
            continue;
        }

      /// copy string
      line.length(len);
      for (j=0,l=0;l<len;j++){
        if (b[j]=='\r' || b[j]=='\n')
          continue;
        line[l++]=b[j];
        }
      buffer->readBytes(i+2);
      return true;
      }
    len++;
    }
  return false;
  }

// Store parsed header into headers.
void HttpResponse::insert_header(const FXString & header) {
  FXString key   = header.before(':').lower();
  FXString value = header.after(':').trim();
  FXString * existing = (FXString*)headers.find(key.text());
  if (existing) {
    (*existing) += ", " + value;
    }
  else {
    FXString * v = new FXString;
    v->adopt(value);
    headers.replace(key.text(),v);
    }
  }

void HttpResponse::clear_headers() {
  for (FXint pos=headers.first();pos<=headers.last();pos=headers.next(pos)) {
    FXString * field = (FXString*) headers.data(pos);
    delete field;
    }
  headers.clear();
  }

void HttpResponse::check_headers() {
  FXString * field = NULL;

  field = (FXString*) headers.find("content-length");
  if (field)
    content_length = APIntVal(*field);

  field = (FXString*) headers.find("transfer-encoding");
  if (field && field->contains("chunked") )
    flags|=ChunkedResponse;

  field = (FXString*) headers.find("connection");
  if (field && comparecase(*field,"close")==0)
    flags|=ConnectionClose;

#ifdef DEBUG
  for (FXint pos=headers.first();pos<=headers.last();pos=headers.next(pos)) {
    fxmessage("%s: %s\n",headers.key(pos),((FXString*)headers.data(pos))->text());
    }
#endif
  }




// Read a chunk header and set the chunksize
FXbool HttpResponse::read_chunk_header(FXint & chunksize) {
  FXString header;
  FXchar   clrf[2];

  // We've read a previous chunk
  if (chunksize==0) {
    if (read(clrf,2)!=2 || clrf[0]!='\r' || clrf[1]!='\n') {
      fxwarning("http: missing line feed: %c%c\n",clrf[0],clrf[1]);
      return false;
      }
    }

  if (read_header(header,HEADER_SINGLE_LINE)) {
    if (header.scan("%x",&chunksize)==1) {
      return true;
      }
    }
  return false;
  }


// Read status line and set status.
FXbool HttpResponse::read_status() {
  FXString header;
  if (read_header(header,HEADER_SINGLE_LINE)) {
    if (header.scan("HTTP/%d.%d %d",&status.major,&status.minor,&status.code)==3){
      GM_DEBUG_PRINT("Code: %d \nVersion: %d.%d\n",status.code,status.major,status.minor);
      return true;
      }
    }
  return false;
  }

// Read header
FXbool HttpResponse::read_header(FXString & header,FXuint mode) {
  while(!parse_header(header,mode)) {
    if (io_buffer(128)<=0)
      return false;
    }
  return true;
  }




// Read the full message body non-chunked
FXString HttpResponse::read_body() {
  FXString content;
  if (content_length==0) {
    return FXString::null;
    }
  else if (content_length>0) {
    content.length(content_length);
    FXival n = read(&content[0],content_length);
    if (n<content_length)  // Partial Transfer...
      return FXString::null;
    }
  else {
    const FXint BLOCK=4096;
    FXint pos=0;
    FXival n=0;
    do {
      content.length(content.length()+BLOCK);
      n = read(&content[pos],BLOCK);
      if (n>0) pos+=n;
      }
    while(n==BLOCK);

    if (pos>0)
      content.length(pos);
    }
  return content;
  }

// Read the full message body chunked
FXString HttpResponse::read_body_chunked() {
  FXString header;
  FXString content;
  FXint    chunksize=-1;

  if (read_chunk_header(chunksize)) {

    while(chunksize) {

      // Resize buffer
      content.length(content.length()+chunksize);

      // Anything less than chunksize is an error
      if (read(&content[content.length()-chunksize],chunksize)<chunksize)
        goto fail;

      // Set to zero so read_chunk_header will check for crlf
      chunksize=0;

      // Next chunk header
      if (!read_chunk_header(chunksize))
        goto fail;
      }

    // Trailing Headers
    while(read_header(header,HEADER_MULTIPLE_LINES)) {

      // empty header indicates end of headers
      if (header.empty())
        break;

      insert_header(header);
      }

    return content;
    }
fail:
  return FXString::null;
  }





FXival HttpResponse::read_body(FXchar *& content) {
  content=NULL;

  if (content_length>0) {
    allocElms(content,content_length);
    FXival n = read(content,content_length);
    if (n!=content_length) {
      freeElms(content);
      content=NULL;
      return -1;
      }
    return content_length;
    }
  else if (content_length<0) {
    const FXint BLOCK=4096;
    FXint  size = 0;
    FXival len  = 0;
    FXival n;
    do {
      resizeElms(content,size+BLOCK);
      size+=BLOCK;
      n = read(content+len,BLOCK);
      if (n>0) len+=n;
      }
    while(n==BLOCK);
    return len;
    }
  else {
    return 0;
    }
  }


FXival HttpResponse::read_body_chunked(FXchar *& content) {
  FXString header;
  FXint    size=0;
  FXint    chunksize=-1;

  content = NULL;

  if (read_chunk_header(chunksize)) {

    while(chunksize) {

      // Resize buffer
      size+=chunksize;
      resizeElms(content,size);

      // Anything less than chunksize is an error
      if (read(content+size-chunksize,chunksize)<chunksize)
        goto fail;

      // Set to zero so read_chunk_header will check for crlf
      chunksize=0;

      // Next chunk header
      if (!read_chunk_header(chunksize))
        goto fail;
      }

    // Trailing Headers
    while(read_header(header,HEADER_MULTIPLE_LINES)) {

      // empty header indicates end of headers
      if (header.empty())
        break;

      insert_header(header);
      }

    return size;
    }
fail:
  freeElms(content);
  return -1;
  }


FXival HttpResponse::read_body(void*ptr,FXival len) {
  if (content_length >= 0) {
    FXchar * data = (FXchar*)ptr;
    FXival n = read(data,FXMIN(content_length,len));
    if (n>0) content_length-=n;
    return n;
    }
  else {
    return read((FXchar*)ptr,len);
    }
  }


FXival HttpResponse::read_body_chunked(void * ptr,FXival len) {
  FXchar * data = (FXchar*)ptr;
  FXString header;
  FXival   nbytes=0;
  while(len) {

    if (chunk_remaining<=0) {

      if (!read_chunk_header(chunk_remaining))
        return -1;

      if (chunk_remaining==0) {

        // Trailing Headers
        while(read_header(header,HEADER_MULTIPLE_LINES)) {

          // empty header indicates end of headers
          if (header.empty())
            break;

          insert_header(header);
          }

        return nbytes;
        }
      }

    FXival n = read(data+nbytes,FXMIN(len,chunk_remaining));
    if (n<=0) return nbytes;
    chunk_remaining-=n;
    nbytes+=n;
    len-=n;
    }
  return nbytes;
  }


// Parse the response status and headers. Returns true if succesful
FXint HttpResponse::parse() {
  FXString header;
  if (read_status()) {
    while(read_header(header,HEADER_MULTIPLE_LINES)) {
      // empty header indicates end of headers
      if (header.empty()) {
        check_headers();
        return status.type();
        }
      insert_header(header);
      }
    }
  return HTTP_RESPONSE_FAILED;
  }

// Read the full message body into string
FXString HttpResponse::body() {
  if (flags&HeadRequest)
    return FXString::null;
  else if (flags&ChunkedResponse)
    return read_body_chunked();
  else
    return read_body();
  }

// Read the full message body into buffer
FXival HttpResponse::body(FXchar *& out) {
  if (flags&HeadRequest)
    return 0;
  else if (flags&ChunkedResponse)
    return read_body_chunked(out);
  else
    return read_body(out);
  }


FXival HttpResponse::readBody(void * ptr,FXival len) {
  if (flags&HeadRequest)
    return 0;
  else if (flags&ChunkedResponse)
    return read_body_chunked(ptr,len);
  else
    return read_body(ptr,len);
  }


void HttpResponse::discard() {
  if (!(flags&ConnectionClose) && !(flags&HeadRequest)) {
    FXchar b[1024];
    while(readBody(b,1024)==1024) ;
    }
  }

FXString HttpResponse::getHeader(const FXString & key) const {
  FXString * value = (FXString*)headers.find(key.text());
  if (value)
    return (*value);
  else
    return FXString::null;
  }


FXint HttpResponse::getContentLength() const {
  return content_length;
  }















HttpHost::HttpHost() : port(0) {
  }

HttpHost::HttpHost(const FXString & url) {
  set(url);
  }

FXbool HttpHost::set(const FXString & url) {
  FXString nn = FXURL::host(url);
#if FOXVERSION >= FXVERSION(1,7,31)
  FXint    np = FXURL::port(url,80);
#else
  FXint    np = FXURL::port(url);
  if (np==0) np=80;
#endif

  if (name!=nn || port!=np) {
    name.adopt(nn);
    port=np;
    return true;
    }
  return false;
  }




HttpClient::HttpClient() : device(BadHandle),options(0) {
  buffer = new MemoryBuffer;
  }

HttpClient::HttpClient(MemoryBuffer* buf) : device(BadHandle), options(0) {
  buffer = buf;
  }

HttpClient::~HttpClient() {
  close();
  delete buffer;
  }

void HttpClient::close() {
  if (device!=BadHandle) {
    shutdown(device,SHUT_RDWR);
    ::close(device);
    device=BadHandle;
    }
  }

void HttpClient::discard() {
  if (flags&ConnectionClose) {
    close();
    }
  else {
    HttpResponse::discard();
    }
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
  // will try to use MSG_NOSIGNAL instead...
  return true;
  }
#endif

static FXbool ap_set_socket_timeout(FXInputHandle handle,FXint timeout) {
  struct timeval tv;

  memset(&tv,0,sizeof(struct timeval));

  tv.tv_sec  = timeout;

  // Receiving
  if (setsockopt(handle,SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof(struct timeval)))
    return false;

  // Sending
  if (setsockopt(handle,SOL_SOCKET,SO_SNDTIMEO,&tv,sizeof(struct timeval)))
    return false;

  return true;
  }














static FXInputHandle ap_create_socket(FXint domain, FXint type, FXint protocol,FXbool nonblocking,FXuint timeout=10) {
  FXInputHandle device = BadHandle;

  // On linux 2.6.27 we can pass additional socket options
  int opts=0;

#ifdef SOCK_CLOEXEC
  opts|=SOCK_CLOEXEC;
#endif

#ifdef SOCK_NONBLOCK
  if (nonblocking) {
    opts|=SOCK_NONBLOCK;
    }
#endif

  device = socket(domain,type|opts,protocol);
  if (device==BadHandle)
    return BadHandle;

#ifndef SOCK_CLOEXEC
  if (!ap_set_closeonexec(device)){
    ::close(device);
    return BadHandle;
    }
#endif

#ifndef SOCK_NONBLOCK
  if (nonblocking && !ap_set_nonblocking(device)){
    ::close(device);
    return BadHandle;
    }
#endif

  // Don't want signals
  if (!ap_set_nosignal(device)) {
    ::close(device);
    return BadHandle;
    }

  // Probably best to always set a sane timeout, blocking or nonblocking
  if (!ap_set_socket_timeout(device,timeout)){
    ::close(device);
    return BadHandle;
    }

  // In case of blocking sockets, set a timeout
  // if (!nonblocking && !ap_set_timeout(device,timeout)) {
  //  ::close(device);
  //  return BadHandle;
  //  }

  return device;
  }


FXbool HttpClient::open_connection() {
  struct addrinfo   hints;
  struct addrinfo * list=NULL;
  struct addrinfo * item=NULL;
  FXint result;

  memset(&hints,0,sizeof(struct addrinfo));
  hints.ai_family=AF_UNSPEC;
  hints.ai_socktype=SOCK_STREAM;
  hints.ai_flags|=(AI_NUMERICSERV|AI_ADDRCONFIG);


  if (options&UseProxy)
    result=getaddrinfo(proxy.name.text(),APStringVal(proxy.port).text(),&hints,&list);
  else
    result=getaddrinfo(server.name.text(),APStringVal(server.port).text(),&hints,&list);

  if (result)
    return false;

  for (item=list;item;item=item->ai_next){

    device = ap_create_socket(item->ai_family,item->ai_socktype,item->ai_protocol,(options&UseNonBlock));
    if (device == BadHandle)
      continue;

    if (connect(device,item->ai_addr,item->ai_addrlen)==0){
      freeaddrinfo(list);
      return true;
      }

    // In case of non-blocking we need to wait for the socket to become ready to write.
    if (errno==EINPROGRESS || errno==EINTR || errno==EWOULDBLOCK) {
      if (io_wait_write()) {
        int socket_error=0;
        socklen_t socket_length=sizeof(socket_error);
        if (getsockopt(device,SOL_SOCKET,SO_ERROR,&socket_error,&socket_length)==0 && socket_error==0){
          freeaddrinfo(list);
          return true;
          }
        }
      else {
        ::close(device);
        device=BadHandle;
        freeaddrinfo(list);
        return false;
        }
      }

    // Failed, try some other one.
    ::close(device);
    device=BadHandle;
    }

  if (list)
    freeaddrinfo(list);
  return false;
  }


FXival HttpClient::io_write(const void * data,FXival count) {
  FXival nwritten=-1;
  do{
    nwritten=::write(device,data,count);
    }
  while(nwritten<0 && errno==EINTR);
  return nwritten;
  }

FXival HttpClient::io_read(void * data,FXival count) {
  FXival nread=-1;
  do{
    nread=::read(device,data,count);
    }
  while(nread<0 && errno==EINTR);
  return nread;
  }


FXbool HttpClient::write(const void * data,FXival len) {
  const FXchar * d = static_cast<const FXchar*>(data);
  do {
     FXival n = io_write(d,len);
     if (n<=0) return false;
     d += n;
     len -= n;
     }
  while(len);
  return true;
  }



void HttpClient::reset(FXbool forceclose){
  if (forceclose)
    close();
  else
    discard();

  clear();
  }


FXbool HttpClient::request(const FXchar * method,const FXString & url,const FXString & header,const FXString & message) {
  FXString command,path,query;

  // Set Server Host
  FXbool host_changed = server.set(url);
  if (options&UseProxy)
    host_changed = false;

  // Reset Client
  reset(host_changed);

  if (compare(method,"HEAD")==0) {
    flags|=HeadRequest;
    }

  // Open connection if necessary
  if (device==BadHandle && !open_connection())
    return false;

  // Extract path and query
  path  = FXURL::path(url);
  if (path.empty())
    path = "/";

  query = FXURL::query(url);
  if (!query.empty())
    path += "?" + query;


//  fxmessage("path: %s\n",path.text());

  // Method + Path
//  if (flags&HttpProxy)
//    command = method + url  + "\r\n"
  //else
  command = method;
  command += " " + path + " HTTP/1.1\r\n";

  // Add Host
  command += "Host: " + server.name + "\r\n";

  // Add Content Length
  if (message.length())
    command +=  "Content-Length: " + APStringVal(message.length()) + "\r\n";

  // Additional headers
  command+=header;

  // End of headers
  command += "\r\n";

  // Add body
  if (message.length())
    command += message;

  // Send Command
  return write(command.text(),command.length());
  }



static FXString ap_encode_base64(const FXString & source) {
  const FXchar base64[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  const FXuchar * in = (const FXuchar*)source.text();
  FXint remaining = source.length() % 3;
  FXint length    = source.length() - remaining;
  FXint n=0;
  FXString out;

  out.length(4*(source.length()/3));

  for (int i=0;i<length;i+=3) {
    out[n++]=base64[(in[i]>>2)];
    out[n++]=base64[((in[i]&0x3)<<4)|(in[i+1]>>4)];
    out[n++]=base64[((in[i+1]&0xf)<<2)|(in[i+2]>>6)];
    out[n++]=base64[(in[i+2]&0x3f)];
    }

  if (remaining) {
    out[n++]=base64[(in[length]>>2)];
    if (remaining>1) {
      out[n++]=base64[((in[length]&0x3)<<4)|(in[length+1]>>4)];
      out[n++]=base64[((in[length+1]&0xf)<<2)|in[length+2]>>6];
      out[n++]='=';
      }
    else {
      out[n++]=base64[((in[length]&0x3)<<4)];
      out[n++]='=';
      out[n++]='=';
      }
    }
  return out;
  }




FXbool HttpClient::basic(const FXchar*    method,
                         FXString         url,
                         const FXString & header,
                         const FXString & content) {

  if (request(method,url,header,content)) {
    do {
      switch(parse()) {
        case HTTP_RESPONSE_INFORMATIONAL:
          {
            if (status.code==HTTP_CONTINUE) {
              continue;
              }
            break;
          }
        case HTTP_RESPONSE_REDIRECT     :
          {
            // 304 - Document not changed. We're done
            // 305 - Need to use a proxy. Currently not handled
            // 306 - Unused
            if (status.code==HTTP_NOT_MODIFIED || status.code==HTTP_USE_PROXY || status.code==306)
              return true;

            url = getHeader("location");

            // No url given, done here
            if (url.empty())
              return false;

            // Don't do automatic redirections for non GET/HEAD requests
            if (comparecase(method,"GET") && comparecase(method,"HEAD"))
              return true;

            if (!request(method,url,header,content)) {
              return false;
              }
            continue;
            break;
          }
        case HTTP_RESPONSE_CLIENT_ERROR  :
          {
            if (status.code==HTTP_UNAUTHORIZED) {

              FXString user     = FXURL::username(url);
              FXString password = FXURL::password(url);

              if (user.empty() || password.empty())
                return true;

              FXString challenge = getHeader("www-authenticate");

              if (comparecase(challenge,"basic",5)==0) {
                FXString auth = "Authorization: Basic " + ap_encode_base64(user+":"+password) + "\r\n";

                if (!request(method,url,header+auth,content)) {
                  return false;
                  }
                continue;
                }
//              else if (comparecase(challenge,"digest",6)==0){
//                FXASSERT(0);
//                }
              }
          } break;

        default: break;
        }
      return true;
      }
    while(1);
    }
  return false;
  }




}





