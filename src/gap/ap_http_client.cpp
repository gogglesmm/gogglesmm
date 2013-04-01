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
#include "ap_connect.h"
#include "ap_buffer_io.h"
#include "ap_http_response.h"
#include "ap_http_client.h"

using namespace ap;

namespace ap {


HttpHost::HttpHost() : port(0) {
  }

HttpHost::HttpHost(const FXString & url) {
  set(url);
  }

void HttpHost::clear() {
  name.clear();
  port=0;
  }

FXbool HttpHost::set(const FXString & url) {
  FXString nn = FXURL::host(url);
  FXint    np = FXURL::port(url,80);
  if (name!=nn || port!=np) {
    name.adopt(nn);
    port=np;
    return true;
    }
  return false;
  }




HttpClient::HttpClient(ConnectionFactory * c) :  connection(c), options(0) {
  }

void HttpClient::setConnectionFactory(ConnectionFactory * c) {
	if (connection) {
		delete connection;
		connection = NULL;
		}
	connection=c;
	}

HttpClient::~HttpClient() {
  close();
	delete connection;
  }

void HttpClient::close() {
	GM_DEBUG_PRINT("HttpClient::close()\n");
	io.close();
  }

void HttpClient::discard() {
	GM_DEBUG_PRINT("HttpClient::discard()\n");
  if (flags&ConnectionClose) {
    close();
    }
  else if (io.isOpen()) {
    HttpResponse::discard();
    }
  }

FXbool HttpClient::open_connection() {
	GM_DEBUG_PRINT("HttpClient::open_connection()\n");
	FXIO * stream = NULL;

	if (connection==NULL)
		connection = new ConnectionFactory();

	if (options&UseProxy)
		stream = connection->open(proxy.name.text(),proxy.port);
	else
		stream = connection->open(server.name.text(),server.port);

	if (stream) {
		io.attach(stream);
		return true;
		}
	return false;
	}

void HttpClient::reset(FXbool forceclose){
	GM_DEBUG_PRINT("HttpClient::reset(%d)\n", forceclose ? 1 : 0);

  if (forceclose)
    close();
  else
    discard();

  clear();
  }


FXbool HttpClient::request(const FXchar * method,const FXString & url,const FXString & header,const FXString & message) {
	GM_DEBUG_PRINT("HttpClient::request(\"%s\",\"%s\")\n",method,url.text());
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
  if (io.isOpen()==false && !open_connection()){
    server.clear();
    return false;
    }

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
    command +=  "Content-Length: " + FXString::value(message.length()) + "\r\n";

  // Additional headers
  command+=header;

  // End of headers
  command += "\r\n";

  // Add body
  if (message.length())
    command += message;

  // Send Command
  return io.write(command);
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
            if (status.code==HTTP_NOT_MODIFIED || status.code==HTTP_USE_PROXY || status.code==HTTP_306)
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

        case HTTP_RESPONSE_FAILED:  /* something went wrong */
          {
						GM_DEBUG_PRINT("HttpClient::basic() failed\n");
            return false;
            break;
          }
        default: break;
        }
      return true;
      }
    while(1);
    }
  return false;
  }


}
