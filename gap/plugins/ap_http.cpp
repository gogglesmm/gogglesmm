/*******************************************************************************
*                         Goggles Audio Player Library                         *
********************************************************************************
*           Copyright (C) 2010-2018 by Sander Jansen. All Rights Reserved      *
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
#include "ap_buffer.h"
#include "ap_connect.h"
#include "ap_http.h"
#include "ap_input_plugin.h"
#include "ap_input_thread.h"

namespace ap {


class HttpInput : public InputPlugin {
protected:
  HttpClient client;
	FXlong 		 content_position;
	FXuint		 content_type;
  FXint 		 icy_interval;
  FXint      icy_count;
  MemoryBuffer preview_buffer;
private:
  HttpInput(const HttpInput&);
  HttpInput &operator=(const HttpInput&);
protected:
	void check_headers();
	FXival icy_read(void*,FXival);
	void icy_parse(const FXString&);
public:
  /// Constructor
  HttpInput(IOContext*);

  FXbool open(const FXString & uri) override;

	/// Read
	FXival read(void*,FXival) override;

	/// Preview
	FXival preview(void*,FXival) override;

  /// Set Position
  FXlong position(FXlong offset,FXuint from) override;

  /// Get Position
  FXlong position() const override;

  /// Size
  FXlong size() override;

  /// End of Input
  FXbool eof() override;

  /// Serial
  FXbool serial() const override;

  /// Get plugin type
  FXuint plugin() const override;

  /// Destructor
  virtual ~HttpInput();
  };


#define HTTP_TOKEN "[\\w!#$%&'*+-.^`|~]+"
#define HTTP_MEDIA_TYPE HTTP_TOKEN "/" HTTP_TOKEN


HttpInput::HttpInput(IOContext * ctx) : InputPlugin(ctx),
  content_position(0),
  content_type(Format::Unknown),
  icy_interval(0),
  icy_count(0) {
  client.setConnectionFactory(new ThreadConnectionFactory(context));
  }

HttpInput::~HttpInput() {
  }


void HttpInput::check_headers() {
  FXint p = client.headers.find("icy-metaint");
  if (p!=-1) icy_count = icy_interval = client.headers.data(p).toInt();

  HttpMediaType media;
  if (client.getContentType(media)) {
    content_type = ap_format_from_mime(media.mime);
    }
  }



FXbool HttpInput::open(const FXString & uri) {
  FXString url = uri;
  FXString headers = FXString::value("User-agent: gogglesmm/%d.%d\r\n"
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
    n=icy_read(p,count);
  else
    n=client.readBody(p,count);

  if (n>0) {
    content_position+=n;
    t+=n;
    }
  return t;
  }

FXlong HttpInput::position(FXlong offset,FXuint from) {
  if (from==FXIO::Current) {
    FXuchar b;
    for (FXlong i=0;i<offset;i++) {
      if (read(&b,1)!=1)
        return -1;
      }
    }
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
  else if (preview_buffer.size())
    return false;
  else
    return client.eof();
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
    context->post_meta(meta);
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


InputPlugin * ap_http_plugin(IOContext * context) {
  return new HttpInput(context);
  }

}
