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
#include "ap_connect.h"
#include "ap_http.h"
#include "ap_http_plugin.h"


namespace ap {


HttpInput::HttpInput(InputThread * i) : InputPlugin(i),
  content_position(0),
  content_type(Format::Unknown),
  icy_interval(0),
  icy_count(0) {
  client.setConnectionFactory(new ThreadConnectionFactory(&input->getFifo()));
  }

HttpInput::~HttpInput() {
  }


void HttpInput::check_headers() {
  FXString * field = NULL;
  field = (FXString*) client.headers.find("icy-metaint");
  if (field) icy_count = icy_interval = field->toInt();

  field = (FXString*) client.headers.find("content-type");
  if (field) fxmessage("%s\n",field->text());
  if (field) content_type = ap_format_from_mime(field->before(';'));
  }



FXbool HttpInput::open(const FXString & uri) {
  FXString url = uri;
  FXString headers = FXString::value("User-agent: libgaplayer/%d.%d\r\n"
                                     "Icy-MetaData: 1\r\n"
                                     "Accept: */*\r\n",0,1);

  if (client.basic("GET",uri,headers)) {
    if (client.status.code==HTTP_OK){

      check_headers();

      /// Try to guess content from uri.
      if (content_type==Format::Unknown)
        content_type=ap_format_from_extension(FXPath::extension(uri));

      return true;
      }
    }
  return false;
  }

FXival HttpInput::preview(void*data,FXival count) {
  FXival n;

  preview_buffer.reserve(count);

  if (client.getContentLength()>=0) {
    if (content_position>=client.getContentLength())
      return -1;
    else
      count=FXMIN((client.getContentLength()-content_position),count);
    }

  if (icy_interval)
    n=icy_read(data,count);
  else
    n=client.readBody(data,count);

  if (n>0)
    preview_buffer.append(data,n);

  return -1;
  }

FXival HttpInput::read(void * data,FXival count) {
  FXival n,t=0;
  FXuchar * p = (FXuchar*)data;

  /// Don't read past content
  if (client.getContentLength()>=0) {
    if (content_position>=client.getContentLength())
      return 0;
    else
      count=FXMIN((client.getContentLength()-content_position),count);
    }

  // Read from preview buffer
  if (preview_buffer.size()) {
    n=preview_buffer.read(p,count);
    if (0<n) { p+=n;count-=n; t+=n;}
    }

  // Regular Read
  if (icy_interval)
    n=icy_read(data,count);
  else
    n=client.readBody(data,count);

  if (n>0) {
    content_position+=n;
    t+=n;
    }
  return t;
  }

FXlong HttpInput::position(FXlong offset,FXuint from) {
  return -1;
  //return client->position(offset,from);
  }

FXlong HttpInput::position() const {
  return content_position;
  }

FXlong HttpInput::size() {
  return client.getContentLength();
  }

FXbool HttpInput::eof()  {
  if (client.getContentLength()>=0 && content_position>=client.getContentLength())
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


void HttpInput::icy_parse(const FXString & str) {
  FXString title = str.after('=').before(';');
  if (title.length()) {
    MetaInfo* meta = new MetaInfo();
    meta->title = title;
    input->post(meta);
    }
  }

FXival HttpInput::icy_read(void*ptr,FXival count){
  FXchar * out = static_cast<FXchar*>(ptr);
  FXival nread=0,n=0;
  if (icy_count<count) {

    /// Read up to icy buffer
    nread=client.readBody(out,icy_count);
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
    n=client.readBody(&b,1);
    if (__unlikely(n!=1)) return -1;

    /// Read icy buffer
    if (b) {
      FXushort icy_size=((FXushort)b)*16;
      FXString icy_buffer;
      icy_buffer.length(icy_size);
      n=client.readBody(&icy_buffer[0],icy_size);
      if (__unlikely(n!=icy_size)) return -1;
      icy_parse(icy_buffer);
      }

    /// reset icy count
    icy_count=icy_interval;

    /// Read remaining bytes
    n=client.readBody(out,count);
    if (__unlikely(n!=count)) return -1;
    nread+=n;
    icy_count-=n;
    }
  else {
    nread=client.readBody(out,count);
    if (__likely(nread>0)) {
      icy_count-=nread;
      }
    }
  return nread;
  }

}
