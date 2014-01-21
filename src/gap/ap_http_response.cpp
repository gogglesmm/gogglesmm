/*******************************************************************************
*                         Goggles Audio Player Library                         *
********************************************************************************
*           Copyright (C) 2010-2013 by Sander Jansen. All Rights Reserved      *
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
#include "ap_buffer_base.h"
#include "ap_buffer_io.h"
#include "ap_http_response.h"

using namespace ap;

namespace ap {

HttpIO::HttpIO() : BufferIO(4096) {
  }

HttpIO::HttpIO(FXIO * io) : BufferIO(io,4096) {
  }

HttpIO::~HttpIO() {
  }

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
    for (p=rdptr;p<(wrptr-1);p++) {
      if (*p=='\r' && *(p+1)=='\n') {
        header.append((const FXchar*)rdptr,p-rdptr);
        rdptr=p+2;

        if (single || header.length()==0)
          return true;

        cnl=true;
        if (wrptr>rdptr)
          goto test_cnl;
        break;
        }
      }
    /* consume what we have so far */
    if (p>rdptr) {
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
    s=p=str.find(':')+1;

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
  mime = str.mid(s,p-s);

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


HttpContentRange::HttpContentRange() : first(-1),last(-1),length(-1) {}
HttpContentRange::HttpContentRange(const FXString & str,FXuint opts) : first(-1),last(-1),length(-1) {
  parse(str,opts);
  }

FXbool HttpContentRange::parse(const FXString & str,FXuint opts) {
  FXint s,p=0;

  // parse the field name
  if (opts&ParseFieldName)
    s=p=str.find(':')+1;

  // white space
  while(str[p]==' '||str[p]=='\t') p++;
  s=p;

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
  GM_DEBUG_PRINT("HttpResponse::clear()\n");
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
  fxmessage("Headers:\n");
  for (FXint pos=0;pos<headers.no();pos++) {
    if (!headers.key(pos).empty())
      fxmessage("\t%s: %s\n",headers.key(pos).text(),headers.data(pos).text());
    }
#endif
  FXint p;

  p = headers.find("content-length");
  if (p!=-1) content_length = content_remaining = headers.data(p).toInt();

  p = headers.find("transfer-encoding");
  if (p!=-1 && headers.data(p).contains("chunked"))
    flags|=ChunkedResponse;

  if (status.major==1 && status.minor==1) {
    if (headers["connection"].contains("close"))
      flags|=ConnectionClose;
    }
  else {
    if (headers["connection"].lower().contains("keep-alive"))
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
      GM_DEBUG_PRINT("http: missing line feed: %c%c\n",clrf[0],clrf[1]);
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
      GM_DEBUG_PRINT("Code: %d \nVersion: %d.%d\n",status.code,status.major,status.minor);
      return true;
      }
    else {
      GM_DEBUG_PRINT("HttpResponse::read_status() - Failed to parse http header: %s\n",header.text());
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
    if (io.read(content,content_length)!=content_length)
      return FXString::null;
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

  // Reading all content
  if (content_remaining>0)
    content_remaining=0;

  if (read_chunk_header(chunksize)) {

    while(chunksize) {

      if (io.read(content,chunksize)!=chunksize) {
        GM_DEBUG_PRINT("HttpResponse::read_body_chunked() - failed reading chunksize %d\n",chunksize);
        goto fail;
        }

      // Set to zero so read_chunk_header will check for crlf
      chunksize=0;

      // Next chunk header
      if (!read_chunk_header(chunksize))
        goto fail;
      }

    GM_DEBUG_PRINT("done with chunks...\n");

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



}
