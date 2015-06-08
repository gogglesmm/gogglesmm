/*******************************************************************************
*                         Goggles Audio Player Library                         *
********************************************************************************
*           Copyright (C) 2010-2015 by Sander Jansen. All Rights Reserved      *
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
#ifndef AP_HTTP_RESPONSE_H
#define AP_HTTP_RESPONSE_H

namespace ap {

enum {
  // HTTP status types
  HTTP_RESPONSE_FAILED        = 0,
  HTTP_RESPONSE_INFORMATIONAL = 1,
  HTTP_RESPONSE_SUCCESS       = 2,
  HTTP_RESPONSE_REDIRECT      = 3,
  HTTP_RESPONSE_CLIENT_ERROR  = 4,
  HTTP_RESPONSE_SERVER_ERROR  = 5,

  // Informational 1xx
  HTTP_CONTINUE                           = 100,
  HTTP_SWITCHING_PROTOCOLS                = 101,
  HTTP_PROCESSING                         = 102, // RFC2518 - WebDAV

  // Succesful 2xx
  HTTP_OK                                 = 200,
  HTTP_CREATED                            = 201,
  HTTP_ACCEPTED                           = 202,
  HTTP_NONAUTHORITATIVE_INFORMATION       = 203,
  HTTP_NO_CONTENT                         = 204,
  HTTP_RESET_CONTENT                      = 205,
  HTTP_PARTIAL_CONTENT                    = 206,
  HTTP_MULTI_STATUS                       = 207, //WEBDAV
  HTTP_IM_USED                            = 226, //RFC3229

  // Redirect 3xx
  HTTP_MULTIPLE_CHOICES                   = 300,
  HTTP_MOVED_PERMANENTLY                  = 301,
  HTTP_FOUND                              = 302,
  HTTP_SEE_OTHER                          = 303,
  HTTP_NOT_MODIFIED                       = 304,
  HTTP_USE_PROXY                          = 305,
	HTTP_306																= 306,
  HTTP_TEMPORARY_REDIRECT                 = 307,

  // Client Error 4xx
  HTTP_BADREQUEST                         = 400,
  HTTP_UNAUTHORIZED                       = 401,
  HTTP_PAYMENTREQUIRED                    = 402,
  HTTP_FORBIDDEN                          = 403,
  HTTP_NOT_FOUND                          = 404,
  HTTP_METHOD_NOT_ALLOWED                 = 405,
  HTTP_NOT_ACCEPTABLE                     = 406,
  HTTP_PROXY_AUTHENTICATION_REQUIRED      = 407,
  HTTP_REQUEST_TIMEOUT                    = 408,
  HTTP_CONFLICT                           = 409,
  HTTP_GONE                               = 410,
  HTTP_LENGTH_REQUIRED                    = 411,
  HTTP_PRECONDITION_FAILED                = 412,
  HTTP_REQUESTENTITY_TOO_LARGE            = 413,
  HTTP_REQUESTURI_TOO_LONG                = 414,
  HTTP_UNSUPPORTED_MEDIATYPE              = 415,
  HTTP_REQUESTED_RANGE_NOT_SATISFIABLE    = 416,
  HTTP_EXPECTATION_FAILED                 = 417,
  HTTP_UNPROCESSABLE_ENTITY               = 422,
  HTTP_LOCKED                             = 423,
  HTTP_FAILED_DEPENDENCY                  = 424, //RFC4918 - WEBDAV
  HTTP_NO_CODE                            = 425,
  HTTP_UPGRADE_REQUIRED                   = 426,

  // Server Error 5xx
  HTTP_INTERNAL_SERVER_ERROR              = 500,
  HTTP_NOT_IMPLEMENTED                    = 501,
  HTTP_BAD_GATEWAY                        = 502,
  HTTP_SERVICE_UNAVAILABLE                = 503,
  HTTP_GATEWAY_TIMEOUT                    = 504,
  HTTP_VERSION_NOTSUPPORTED               = 505,
  HTTP_INSUFFICIENT_STORAGE               = 507, //RFC4918 - WEBDAV
  HTTP_BANDWITH_LIMIT_EXCEEDED            = 509,
  HTTP_NOT_EXTENDED                       = 510 //RFC 2774
  };

enum {
  HEADER_MULTIPLE_LINES = 0,
  HEADER_SINGLE_LINE    = 1
  };

/* HttpIO */
class GMAPI HttpIO : public BufferIO {
private:
  HttpIO(const HttpIO&);
  HttpIO &operator=(const HttpIO&);
public:
  HttpIO();
  HttpIO(FXIO * io);

  // Read single or multiline header
  FXbool readHeader(FXString & header,FXbool single=false);

  // Send request
  FXbool write(const FXString & request);

  // Read and append n bytes to str
  FXival read(FXString & str,FXival n);

  // Destructor
  ~HttpIO();
  };


/* Http Status */
struct HttpStatus {
  FXint major;
  FXint minor;
  FXint code;
  FXint type() const;
  };


class GMAPI HttpHeader {
public:
  enum {
    ParseFieldName = 0x1,
    };
protected:
  FXint parseToken(const FXString & str,FXint & p);
  FXint parseQuotedString(const FXString & str,FXint & p);
  };


class GMAPI HttpMediaType : public HttpHeader {
public:
  FXString           mime;
  FXStringDictionary parameters;
public:
  HttpMediaType();
  HttpMediaType(const FXString & str,FXuint opts=0);
  FXbool parse(const FXString & str,FXuint opts=0);
  };


class GMAPI HttpContentRange : public HttpHeader {
public:
  FXlong first;
  FXlong last;
  FXlong length;
public:
  HttpContentRange();
  HttpContentRange(const FXString & str,FXuint opts=0);
  FXbool parse(const FXString & str,FXuint opts=0);
  };


/* Http Response Parser */
class GMAPI HttpResponse {
protected:
	HttpIO       io;								// IO Stream
  FXint        content_length;    // Content Length from header
  FXint        content_remaining; // Content left to read
  FXint        chunk_remaining;   // Remaining bytes left to read in chunk
  FXuchar      flags;             // Options flags used by parser
public:
  HttpStatus          status;     // Response Status. Valid after response() returns true
  FXStringDictionary  headers;    // Dictionary of all headers
protected:

  // Internal Parser Flags
  enum {
    ChunkedResponse  = (1<<0),      // Chunked Response
    ConnectionClose  = (1<<1),      // Connection Closes
    ResponseComplete = (1<<2),
    HeadRequest      = (1<<3),
    };

private:

  // Try reading complete header from buffer
  FXbool parse_header(FXString & header,FXuint span);

  // Add header to dictionary
  void insert_header(const FXString &);

  // Check all headers we're interested in
  void check_headers();

  // Clear Headers
  void clear_headers();

private:

  // Reads chunk header and gives back size.
  FXbool read_chunk_header(FXint & chunksize);

  // Read status line
  FXbool read_status();

private:

  // Read body using normal transfer. Returns a string
  FXString read_body();

  // Read body using chunked transfer. Returns a string
  FXString read_body_chunked();

  // Read body using chunked transfer
  FXival   read_body_chunked(void * ptr,FXival len);

  // Read body using normal transfer
  FXival   read_body(void * ptr,FXival len);

protected:
  HttpResponse();

  // Clear Response
  void clear();

public:

  // Completes reading response
  virtual void discard();

  // Read response status and headers.
  FXint parse();

  // Return the complete message body as string
  FXString body();

  // Return the complete message body as string.
  FXString textBody();

  /// Read partial body
  FXival readBody(void*ptr,FXival len);

  // Return header for given key
  FXString getHeader(const FXString & key) const;

  // Return Content Length if known or -1
  FXint getContentLength() const;

  // Return Content Type if known or return false
  FXbool getContentType(HttpMediaType &) const;

  // Return Content Range if known or return false
  FXbool getContentRange(HttpContentRange &) const;

  // Check end-of-response
  FXbool eof();

  // Destructor
  virtual ~HttpResponse();
  };

}

#endif
