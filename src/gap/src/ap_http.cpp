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
  flags(0),
  content_length(-1),
  chunk_remaining(-1) {
  }

HttpResponse::~HttpResponse() {
  clear();
  }

void HttpResponse::clear() {
  flags=0;
  content_length=-1;
  chunk_remaining=-1;
  clear_headers();
  }


// Read (at most) nbytes from buffer or source
FXival HttpResponse::read(FXchar*ptr,FXival nbytes) {
  FXival nread=0;
  while(nbytes) {
    if (buffer.size()) {
      FXival n = buffer.read(ptr,nbytes);
      nread+=n;
      nbytes-=n;
      ptr+=n;
      }
    else {
      FXival n = readBlock(ptr,nbytes);
      if (n<=0) return (nread>0) ? nread : n;
      nread+=n;
      nbytes-=n;
      ptr+=n;
      }
    }
  return nread;
  }

// Fill buffer with (at most) nbytes
FXival HttpResponse::fill(FXival nbytes) {
  buffer.reserve(nbytes);
  FXival nread = readBlock((FXchar*)buffer.ptr(),nbytes);
  if (nread>0) {
    //fxmessage("buf: \"%s\"\n",buffer.data());
    buffer.wroteBytes(nread);
    }
  return nread;
  }


// Try parsing one HTTP header from buffer. If succesfull, returns
// headers. Mode can either be HEADER_SINGLE_LINE or HEADER_MULTIPLE_LINES,
// depending on whether headers may span multiple lines or not.
FXbool HttpResponse::parse_header(FXString & line,FXuint mode) {
  FXint len=0,i,j,l;
  for (i=0;i<buffer.size()-1;i++){
    if (buffer[i]=='\r' && buffer[i+1]=='\n') {

      // If header cannot span multiple lines, we're done here.
      if (mode==HEADER_SINGLE_LINE) {
        if (i>0) line.assign((FXchar*)buffer.data(),i);
        buffer.readBytes(i+2);
        return true;
        }

      // check if header continues on the next line
      if (len>0) {

        // need more bytes
        if ((i+2)>=buffer.size())
            return false;

        /// header continues on next line
        if (buffer[i+2]==' ' || buffer[i+2]=='\t')
            continue;
        }

      /// copy string
      line.length(len);
      for (j=0,l=0;l<len;j++){
        if (buffer[j]=='\r' || buffer[j]=='\n')
          continue;
        line[l++]=buffer[j];
        }
      buffer.readBytes(i+2);
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
    content_length = field->toInt();

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
      fxwarning("http: missing line feed: %c%c\n",clrf[0],clrf[2]);
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
      //fxmessage("Code: %d \nVersion: %d.%d\n",status.code,status.major,status.minor);
      return true;
      }
    }
  return false;
  }

// Read header
FXbool HttpResponse::read_header(FXString & header,FXuint mode) {
  while(!parse_header(header,mode)) {
    if (fill()==-1)
      return false;
    }
  return true;
  }

// Read the full message body non-chunked
FXString HttpResponse::read_body() {
  FXString body;
  if (content_length==0) {
    return FXString::null;
    }
  else if (content_length>0) {
    body.length(content_length);
    FXival n = read(&body[0],content_length);
    if (n<content_length)  // Partial Transfer...
      return FXString::null;
    }
  else {
    const FXint BLOCK=4096;
    FXint size=0;
    FXint pos=0;
    FXival n=0;
    do {
      body.length(body.length()+BLOCK);
      n = read(&body[pos],BLOCK);
      if (n>0) pos+=n;
      }
    while(n==BLOCK);

    if (pos>0)
      body.length(pos);
    }
  return body;
  }

// Read the full message body chunked
FXString HttpResponse::read_body_chunked() {
  FXString body;
  FXint    pos=0;
  FXint    chunksize=-1;
  FXchar   clrf[2];
  if (read_chunk_header(chunksize)) {
    while(chunksize) {
      body.length(body.length()+chunksize);
      FXival n = read(&body[pos],chunksize);
      if (n<chunksize) return FXString::null;
      pos+=chunksize;
      chunksize=0;
      if (!read_chunk_header(chunksize)) {
        fxwarning("http: reading next chunk failed\n");
        return FXString::null;
        }
      }

    // Possible... there may be a trailer
    ///fxmessage("now trying to read trailers...\n");
    FXString header;
    while(read_header(header,HEADER_MULTIPLE_LINES)) {
      // empty header indicates end of headers
      if (header.empty())
        break;
      insert_header(header);
      }
    }
  return body;
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
  if (flags&ChunkedResponse)
    return read_body_chunked();
  else
    return read_body();
  }

FXival HttpResponse::read_body_chunked(void * ptr,FXival len) {
  FXival n,nread=0;
  FXchar * data = (FXchar*)ptr;

  while(len){
    if (chunk_remaining<=0) {

      if (!read_chunk_header(chunk_remaining))
        return -1;

      if (chunk_remaining==0) {
        FXString header;
        while(read_header(header,HEADER_MULTIPLE_LINES)) {
          // empty header indicates end of headers
          if (header.empty())
              break;
          insert_header(header);
          }
        return nread;
        }
      }
    n = read(data,FXMIN(len,chunk_remaining));
    if (n<=0) return nread;
    data+=n;
    len-=n;
    nread+=n;
    chunk_remaining-=n;
    }
  return nread;
  }

FXival HttpResponse::read_body(void*ptr,FXival len) {
  if (content_length >= 0) {
    //return read(ptr,FXMIN(content_length_remaining,len);
    return 0;
    }
  else {
    return read((FXchar*)ptr,len);
    }
  }


FXival HttpResponse::readBody(void * ptr,FXival len) {
  if (flags&ChunkedResponse)
    return read_body_chunked(ptr,len);
  else
    return read_body(ptr,len);
  }


void HttpResponse::discard() {
  if (!(flags&ConnectionClose)) {
    FXchar buffer[1024];
    while(readBody(buffer,1024)==1024) ;
    }
  clear();
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
  FXint    np = FXURL::port(url);
  if (np==0) np=80;

  if (name!=nn || port!=np) {
    name.adopt(nn);
    port=np;
    return true;
    }
  return false;
  }




HttpClient::HttpClient() : device(BadHandle) {
    }


HttpClient::~HttpClient() {
  close();
  }

void HttpClient::close() {
  if (device!=BadHandle) {
    shutdown(device,SHUT_RDWR);
    ::close(device);
    device=BadHandle;
    }
  }

void HttpClient::discard() {
  if (flags&ConnectionClose)
    close();
  else
    HttpResponse::discard();
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


  if (flags&UseProxy)
    result=getaddrinfo(proxy.name.text(),FXString::value(proxy.port).text(),&hints,&list);
  else
    result=getaddrinfo(server.name.text(),FXString::value(server.port).text(),&hints,&list);


  if (result)
    return false;

  for (item=list;item;item=item->ai_next){

    device = socket(item->ai_family,item->ai_socktype,item->ai_protocol);
    if (device == BadHandle)
      continue;

    if (connect(device,item->ai_addr,item->ai_addrlen)==0){
      freeaddrinfo(list);
      return true;
      }

    ::close(device);
    device=BadHandle;
    }

  if (list)
    freeaddrinfo(list);
  return false;
  }


FXival HttpClient::writeBlock(const void * data,FXival count) {
  FXival nwritten=-1;
  do{
    nwritten=::write(device,data,count);
    }
  while(nwritten<0 && errno==EINTR);
  return nwritten;
  }

FXival HttpClient::readBlock(void * data,FXival count) {
  FXival nread=-1;
  do{
    nread=::read(device,data,count);
    }
  while(nread<0 && errno==EINTR);
  return nread;
  }


FXbool HttpClient::send(const FXchar * data,FXint len) {
  do {
     FXival n = writeBlock(data,len);
     if (n<=0) return false;
     data += n;
     len -= n;
     }
  while(len);
  return true;
  }


FXbool HttpClient::request(const FXchar * method,const FXString & url,const FXString & headers,const FXString & body) {
  FXString command,path,query;

  // Set Server Host
  if (server.set(url)) {
//    if (!flags&HttpProxy)
      close();
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
  if (body.length())
    command +=  "Content-Length: " + FXString::value(body.length()) + "\r\n";

  // Additional headers
  command+=headers;

  // End of headers
  command += "\r\n";

  // Add body
  if (body.length())
    command += body;

  // Send Command
  return send(command.text(),command.length());
  }



FXString ap_encode_base64(const FXString & source) {
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
    out[n++]=base64[(in[i+1]&0xf)|(in[i+2]>>6)];
    out[n++]=base64[(in[i+2]&0x3f)];
    }

  if (remaining) {
    out[n++]=base64[(in[length]>>2)];
    if (remaining>1) {
      out[n++]=base64[((in[length]&0x3)<<4)|(in[length+1]>>4)];
      out[n++]=base64[(in[length+1]&0xf)|in[length+2]>>6];
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




FXbool HttpClient::basic(const FXchar *   method,
                         const FXString & url,
                         const FXString & headers,
                         const FXString & body) {
  FXString auth;
  FXString location=url;

  close();
  clear();

  if (request(method,url,headers,body)) {
    do {
      switch(parse()) {
        case HTTP_RESPONSE_INFORMATIONAL:
          {
            if (status.code==HTTP_CONTINUE) {
              discard();
              continue;
              }
            break;
          }
        case HTTP_RESPONSE_REDIRECT     :
          {
            FXString location = getHeader("location");
            if (location.empty() || (comparecase(method,"GET") && comparecase(method,"HEAD")) ) {
              return false;
              }

            discard();

            if (!request(method,location,headers,body)) {
              return false;
              }

            break;
          }
        case HTTP_RESPONSE_CLIENT_ERROR  :
          {
#if 0
            if (status.code==HTTP_UNAUTHORIZED) {
              FXString challenge = getHeader("www-authenticate");
              if (comparecase(challenge,"basic",5)==0) {
                FXString auth = "Basic " + ap_encode_base64(FXURL::username(url) + ":" + FXURL::password(url)) + "\r\n";
                discard();
                if (!request(method,location,headers+auth,body)) {
                  return false;
                  }
                }
              else if (comparecase(challenge,"digest",6)==0){
                FXASSERT(0);
                }
              }
            else {
              return false;
              }
#endif
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





