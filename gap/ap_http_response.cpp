/*******************************************************************************
*                         Goggles Audio Player Library                         *
********************************************************************************
*           Copyright (C) 2010-2017 by Sander Jansen. All Rights Reserved      *
*                               ---                                            *
* This program is free software: you can redistribute it and/or modify         *
* it under the terms of the GNU General Public License as published by         *
* the Free Software Foundation, either version 3 of the License, or            *
* (at your option) any later version.                                          *
*                                                                              *
* This program is distributed in the hope that it will be useful,              *
* but WITHOUT ANY WARRANTY; without even the implied warranty of               *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                *
* GNU General Public License for more details.                                 *
*                                                                              *
* You should have received a copy of the GNU General Public License            *
* along with this program.  If not, see http://www.gnu.org/licenses.           *
********************************************************************************/
#include "ap_defs.h"
#include "ap_utils.h"
#include "ap_buffer_base.h"
#include "ap_buffer_io.h"
#include "ap_http_response.h"

#ifdef HAVE_ZLIB
#include <zlib.h>
#endif


using namespace ap;

namespace ap {


// apparently defined as macros by glibc/gcc.
#undef major
#undef minor

#ifdef HAVE_ZLIB

struct ZIO {
  z_stream stream;

  ZIO() {
    memset(&stream,0,sizeof(stream));
    }
  };

#endif


HttpIO::HttpIO() : BufferIO(4096), z(nullptr) {
  }

HttpIO::HttpIO(FXIO * dev) : BufferIO(dev,4096), z(nullptr) {
  }

HttpIO::~HttpIO() {
#ifdef HAVE_ZLIB
  if (z) {
    inflateEnd(&z->stream);
    delete z;
    z=nullptr;
    }
#endif
  }



#ifdef HAVE_ZLIB
FXival HttpIO::gzip_read(FXString & data,FXival & bytes_written,FXival bytes_available) {
  const FXint blocksize = 4096;

  FXival bytes_consumed = 0;

  if (dir!=DirWrite) {

    // Initialize inflater on first call
    if (z==nullptr) {
      z = new ZIO;
      if (inflateInit2(&z->stream,15+16)!=Z_OK) {
        delete z;
        return FXIO::Error;
        }
      bytes_written = 0;
      data.length(blocksize);
      }

    // Read until we consumed all available bytes
    while(bytes_available) {

      // update buffers
      z->stream.next_in   = rdptr;
      z->stream.avail_in  = FXMIN(bytes_available,wrptr-rdptr);
      z->stream.next_out  = (FXuchar*)&data[bytes_written];
      z->stream.avail_out = data.length()-bytes_written;

      // inflate
      int zerror = inflate(&z->stream,Z_NO_FLUSH);

      bytes_consumed  += (z->stream.next_in - rdptr);
      bytes_available -= (z->stream.next_in - rdptr);
      bytes_written   += ((data.length()-bytes_written) - z->stream.avail_out);

      // update read ptr
      rdptr = z->stream.next_in;

      // check result of inflate
      if (zerror!=Z_OK) {
        if (zerror==Z_STREAM_END) {
          inflateEnd(&z->stream);
          data.length(bytes_written);
          delete z;
          z=nullptr;
          return bytes_consumed;
          }
        else if (zerror==Z_BUF_ERROR) {

          if (bytes_available==0)
            return bytes_consumed;

          if (z->stream.avail_out==0)
            data.length(data.length()+blocksize);

          const FXival bytes_in_buffer = wrptr-rdptr;
          if (z->stream.avail_in==0 && (bytes_available>bytes_in_buffer)) {
            if (readBuffer()<=(FXuval)bytes_in_buffer) {
              inflateEnd(&z->stream);
              delete z;
              z=nullptr;
              return FXIO::Error;
              }
            }

          }
        else {
          inflateEnd(&z->stream);
          delete z;
          z=nullptr;
          return FXIO::Error;
          }
        }
      }
    return bytes_consumed;
    }
  return FXIO::Error;
  }
#else
FXival HttpIO::gzip_read(FXString &,FXival &,FXival) {
  return FXIO::Error;
  }
#endif




FXival HttpIO::read(FXString & str,FXival n) {
  if (0<n) {
    FXint pos = str.length();
    str.length(pos+n);
    return readBlock(&str[pos],n);
    }
  return 0;
  }

FXbool HttpIO::readHeader(FXString & header,FXbool single) {
  FXbool cnl=false; // continue next line
  FXuchar * p;
  do {

    /* check if we continue on the next line */
    if (cnl) {
test_cnl:
      FXASSERT(rdptr<wrptr);
      if (!(*rdptr=='\t' || *rdptr==' ')) {
        return true;
        }
      cnl=false;
      }

    /* find end of line \r\n */
    for (p=rdptr;p<wrptr;p++) {
      if (*p=='\n') {

        if (*(p-1)=='\r')
          header.append((const FXchar*)rdptr,p-rdptr-1);
        else
          header.append((const FXchar*)rdptr,p-rdptr);

        rdptr=p+1;

        /* HTTP status or end of header section */
        if (single || header.length()==0) {
          dir = (wrptr>rdptr) ? DirRead : DirNone;
          return true;
          }

        cnl=true;
        if (wrptr>rdptr)
          goto test_cnl;

        break;
        }
      }

    /* consume what we have so far */
    if (p>rdptr) {

      if (*p=='\r')
        header.append((const FXchar*)rdptr,p-rdptr-1);
      else
        header.append((const FXchar*)rdptr,p-rdptr);

      rdptr=p;
      }
    }
  while(readBuffer());
  return false;
  }


FXbool HttpIO::write(const FXString & str) {
  //fxmessage("[%d]%s\n",str.length(),str.text());
  FXint n = writeBlock(str.text(),str.length());
  if (n==-1) return false;
  FXASSERT(n==str.length());
  return flush();
  }



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


// HttpHeader

FXint HttpHeader::parseToken(const FXString & str,FXint & p){
  FXchar c;
  FXint  s=p;
  while(p<str.length()){
    c=str[p];
    if (!Ascii::isAlphaNumeric(c)) {
      switch(c) {
        case  '!':
        case  '#':
        case  '$':
        case  '%':
        case  '&':
        case '\'':
        case  '*':
        case  '+':
        case  '-':
        case  '.':
        case  '^':
        case  '_':
        case  '`':
        case  '|':
        case  '~': break;
        default  : return p-s; break;
        }
      }
    p++;
    }
  return p-s;
  }

FXint HttpHeader::parseQuotedString(const FXString & str,FXint & p){
  FXint s=p;
  while(p<str.length()) {
    if (str[p]=='\"' && str[p-1]!='\\')
      return p-s;
    p++;
    }
  return 0;
  }


HttpMediaType::HttpMediaType() {}

HttpMediaType::HttpMediaType(const FXString & str,FXuint opts) {
  parse(str,opts);
  }

FXbool HttpMediaType::parse(const FXString & str,FXuint opts) {
  FXint s,p=0,ks,kp;

  // parse the field name
  if (opts&ParseFieldName)
    p=str.find(':')+1;

  // white space
  while(str[p]==' '||str[p]=='\t') p++;
  s=p;

  // "type/"
  if (parseToken(str,p)==0 || str[p++]!='/')
    return false;

  // "subtype"
  if (parseToken(str,p)==0)
    return false;

  // Get mime
  mime = str.mid(s,p-s).lower(); // mime is case insensitive, force lower case

  // Parameters
  while(p<str.length()) {

    // white space
    while(str[p]==' '||str[p]=='\t') p++;

    // check for parameter separator
    if (str[p++]!=';') break;

    // eat space
    while(str[p]==' '||str[p]=='\t') p++;

    // get key and =
    ks=kp=p;
    if (parseToken(str,kp)==0 || str[kp]!='=')
      break;
    p=kp+1;

    // get the value
    if (str[p]=='"') {
      s=p;
      if (parseQuotedString(str,p)==0)
        break;

      parameters.insert(str.mid(ks,kp-ks),
                        unescape(str.mid(s,p-s),'\"','\"'));
      p++;
      }
    else {
      s=p;
      if (parseToken(str,p)==0)
        break;

      parameters.insert(str.mid(ks,kp-ks),
                        str.mid(s,p-s));
      p++;
      }
    }
  return true;
  }


HttpContentRange::HttpContentRange(const FXString & str,FXuint opts) : first(-1),last(-1),length(-1) {
  parse(str,opts);
  }

FXbool HttpContentRange::parse(const FXString & str,FXuint opts) {
  FXint s,p=0;

  // parse the field name
  if (opts&ParseFieldName)
    p=str.find(':')+1;

  // white space
  while(str[p]==' '||str[p]=='\t') p++;

  // Make sure ranges are in bytes
  if (str[p]!='b' || str[p+1]!='y' || str[p+2]!='t' || str[p+3]!='e' || str[p+4]!='s' || str[p+5]!=' ')
    return false;
  p+=6;

  // first and last byte pos
  if (str[p]!='*') {
    s=p;
    while(Ascii::isDigit(str[p])) p++;

    if (str[p]!='-')
      return false;

    first = str.mid(s,p-s).toLong();

    s=++p;
    while(Ascii::isDigit(str[p])) p++;
    if (str[p]!='/')
      goto failed;

    last = str.mid(s,p-s).toLong();

    if (last<first)
      goto failed;
    }
  p++;

  // Length
  if (str[p]!='*') {
    s=p;
    while(Ascii::isDigit(str[p])) p++;
    length = str.mid(s,p-s).toLong();

    if (last!=-1 && length<last)
      goto failed;
    }

  return true;
failed:
  length=first=last=-1;
  return false;
  }




HttpResponse::HttpResponse() :
  content_length(-1),
  content_remaining(-1),
  chunk_remaining(-1),
  flags(0){
  }

HttpResponse::~HttpResponse() {
  clear();
  }

void HttpResponse::clear() {
  flags=0;
  content_length=-1;
  content_remaining=-1;
  chunk_remaining=-1;
  clear_headers();
  }


// Store parsed header into headers.
void HttpResponse::insert_header(const FXString & header) {
  FXString key   = header.before(':').lower();
  FXString value = header.after(':').trim();
  FXint p = headers.find(key);
  if (p!=-1)
    headers.data(p).append(","+value);
  else
    headers[key] = value;
  }

void HttpResponse::clear_headers() {
  headers.clear();
  }

void HttpResponse::check_headers() {
#ifdef DEBUG
  FXbool firstline = true;
  for (FXint pos=0;pos<headers.no();pos++) {
    if (!headers.key(pos).empty()) {
      if (firstline) {
        fxmessage("[http] %s: %s\n",headers.key(pos).text(),headers.data(pos).text());
        firstline=false;
        }
      else {
        fxmessage("       %s: %s\n",headers.key(pos).text(),headers.data(pos).text());
        }
      }
    }
#endif
  FXint p;

  p = headers.find("content-length");
  if (p!=-1) content_length = content_remaining = headers.data(p).toInt();


  p = headers.find("content-encoding");
  if ((p!=-1) && headers.data(p).contains("gzip"))
    flags|=ContentEncodingGZip;

  p = headers.find("transfer-encoding");
  if (p!=-1 && headers.data(p).contains("chunked"))
    flags|=ChunkedResponse;

  if (status.major==1 && status.minor==1) {
    if (headers["connection"].contains("close"))
      flags|=ConnectionClose;
    }
  else {
    if (!headers["connection"].lower().contains("keep-alive"))
      flags|=ConnectionClose;
    }
  }

// Read a chunk header and set the chunksize
FXbool HttpResponse::read_chunk_header(FXint & chunksize) {
  FXString header;
  FXchar   clrf[2];

  // We've read a previous chunk
  if (chunksize==0) {
    if (io.readBlock(clrf,2)!=2 || clrf[0]!='\r' || clrf[1]!='\n') {
      GM_DEBUG_PRINT("[http] missing line feed: %c%c\n",clrf[0],clrf[1]);
      return false;
      }
    }

  if (io.readHeader(header,true)) {
    if (header.scan("%x",&chunksize)==1) {
      return true;
      }
    }
  return false;
  }


// Read status line and set status.
FXbool HttpResponse::read_status() {
  FXString header;
  if (io.readHeader(header,true)) {
    if (header.scan("HTTP/%d.%d %d",&status.major,&status.minor,&status.code)==3){
      GM_DEBUG_PRINT("[http] HTTP/%d.%d %d\n",status.major,status.minor,status.code);
      return true;
      }
    else if (header.scan("ICY %d",&status.code)==1){
      GM_DEBUG_PRINT("[http] ICY %d\n",status.code);
      return true;
      }
    else {
      GM_DEBUG_PRINT("[http] Failed to parse http header: %s\n",header.text());
      }
    }
  return false;
  }



// Read the full message body non-chunked
FXString HttpResponse::read_body() {
  FXString content;
  if (content_length==0) {
    return FXString::null;
    }
  else if (content_length>0) {
    content_remaining=0;
    if (flags&ContentEncodingGZip) {
#ifdef HAVE_ZLIB
      FXival bytes_written=0;
      if (io.gzip_read(content,bytes_written,content_length)!=content_length)
        return FXString::null;
#else
        return FXString::null;
#endif
      }
    else {
      if (io.read(content,content_length)!=content_length)
        return FXString::null;
      }
    }
  else {
    FXival n,c=0;
    const FXival BLOCK = 4096;
    while((n=io.read(content,BLOCK))==BLOCK) c+=n;
    if (0<n) c+=n;
    content.length(c);
    }
  return content;
  }

// Read the full message body chunked
FXString HttpResponse::read_body_chunked() {
  FXString header;
  FXString content;
  FXint    chunksize=-1;
#ifdef HAVE_ZLIB
  FXival   bytes_written=0;
#endif

  // Reading all content
  if (content_remaining>0)
    content_remaining=0;

  if (read_chunk_header(chunksize)) {

    while(chunksize) {

      if (flags&ContentEncodingGZip) {
#ifdef HAVE_ZLIB
        if (io.gzip_read(content,bytes_written,chunksize)!=chunksize) {
          GM_DEBUG_PRINT("[http] read_body_chunked() - failed reading chunksize %d\n",chunksize);
          goto fail;
          }
#else
        goto fail;
#endif
        }
      else {
        if (io.read(content,chunksize)!=chunksize) {
          GM_DEBUG_PRINT("[http] read_body_chunked() - failed reading chunksize %d\n",chunksize);
          goto fail;
          }
        }

      // Set to zero so read_chunk_header will check for crlf
      chunksize=0;

      // Next chunk header
      if (!read_chunk_header(chunksize))
        goto fail;
      }

    GM_DEBUG_PRINT("[http] done with chunks...\n");

    // Trailing Headers
    while(io.readHeader(header)) {

      // empty header indicates end of headers
      if (header.empty())
        break;

      insert_header(header);

      header.clear();
      }

    return content;
    }
fail:
  return FXString::null;
  }




FXival HttpResponse::read_body(void*ptr,FXival len) {
  if (content_remaining >= 0) {
    FXchar * data = (FXchar*)ptr;
    FXival n = io.readBlock(data,FXMIN(content_remaining,len));
    if (n>0) content_remaining-=n;
    return n;
    }
  else {
    return io.readBlock((FXchar*)ptr,len);
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
        while(io.readHeader(header)) {

          // empty header indicates end of headers
          if (header.empty())
            break;

          insert_header(header);

          header.clear();
          }
        return nbytes;
        }
      }

    FXival n = io.readBlock(data+nbytes,FXMIN(len,chunk_remaining));
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
    while(io.readHeader(header)) {
      // empty header indicates end of headers
      if (header.empty()) {
        check_headers();
        return status.type();
        }
      insert_header(header);
      header.clear();
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

FXString HttpResponse::textBody() {
  HttpMediaType media;

  if (getContentType(media) && media.parameters.has("charset")) {
    const FXTextCodec * codec = ap_get_textcodec(media.parameters["charset"]);
    if (codec) return codec->mb2utf(body());

    discard();
    }
  else {
    return body();
    }
  return FXString::null;
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
  return headers[key];
  }


FXint HttpResponse::getContentLength() const {
  return content_length;
  }

FXbool HttpResponse::getContentType(HttpMediaType & media) const {
  return media.parse(headers["content-type"]);
  }

FXbool HttpResponse::getContentRange(HttpContentRange & range) const {
  return range.parse(headers["content-range"]);
  }



FXbool HttpResponse::eof() {
  if (content_remaining>=0)
    return (content_remaining==0);
  else
    return io.eof();
  }


// Return whether zlib support is available
FXbool HttpResponse::has_zlib_support() {
#ifdef HAVE_ZLIB
  return true;
#else
  return false;
#endif
  }


}
