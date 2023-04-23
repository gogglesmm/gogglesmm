/*******************************************************************************
*                         Goggles Audio Player Library                         *
********************************************************************************
*           Copyright (C) 2010-2021 by Sander Jansen. All Rights Reserved      *
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
#include "ap_common.h"
#include "ap_socket.h"
#include "ap_connect.h"
#include "ap_buffer_base.h"
#include "ap_buffer_io.h"
#include "ap_http_response.h"
#include "ap_http_client.h"

using namespace ap;

namespace ap {


HttpHost::HttpHost(const FXString & url) {
  set(url);
  }

void HttpHost::clear() {
  name.clear();
  port=0;
  ssl=false;
  }

FXbool HttpHost::set(const FXString & url) {
  FXbool   ns = FXString::comparecase(FXURL::scheme(url), "https") == 0;
  FXString nn = FXURL::host(url);
  FXint    np = FXURL::port(url, ns ? 443 : 80);
  if (name!=nn || port!=np || ssl!=ns) {
    name.adopt(nn);
    port=np;
    ssl=ns;
    return true;
    }
  return false;
  }




HttpClient::HttpClient(ConnectionFactory * c) :  connection(c), options(0) {
  }

void HttpClient::setConnectionFactory(ConnectionFactory * c) {
  if (connection) {
      delete connection;
      connection = nullptr;
      }
  connection=c;
  }

HttpClient::~HttpClient() {
  close();
  delete connection;
  }

#ifdef HAVE_ZLIB
void HttpClient::setAcceptEncoding(const FXuchar opts) {
  options = (options&~AcceptEncodingGZip) | (opts&AcceptEncodingGZip);
  }
#else
void HttpClient::setAcceptEncoding(const FXuchar) {
  options = (options&~AcceptEncodingGZip); // always turn off
  }
#endif


void HttpClient::close() {
  GM_DEBUG_PRINT("[http] close()\n");

  // Shutdown communication
  ap::Socket * s = dynamic_cast<ap::Socket*>(io.attached());
  if (s) s->shutdown();

  io.close();
  }

void HttpClient::discard() {
  GM_DEBUG_PRINT("[http] discard()\n");
  if (flags&ConnectionClose) {
    close();
    }
  else if (io.isOpen()) {
    HttpResponse::discard();
    }
  }

FXbool HttpClient::open_connection() {
  GM_DEBUG_PRINT("[http] open connection\n");
  FXIO * stream = nullptr;

  if (connection==nullptr)
    connection = new ConnectionFactory();

  if (options&UseProxy)
    stream = connection->open(proxy.name.text(),proxy.port, proxy.ssl);
  else
    stream = connection->open(server.name.text(),server.port, server.ssl);

  if (stream) {
    GM_DEBUG_PRINT("[http] connected\n");
    io.attach(stream);
    return true;
    }
  GM_DEBUG_PRINT("[http] connection failure\n");
  return false;
  }

void HttpClient::reset(FXbool forceclose){
  if (forceclose)
    close();
  else
    discard();
  clear();
  }


FXbool HttpClient::request(const FXchar * method,const FXString & url,const FXString & header,const FXString & message) {
  GM_DEBUG_PRINT("[http] request(\"%s\",\"%s\")\n",method,url.text());
  FXString command,path,query;

  // Set Server Host
  FXbool host_changed = server.set(url);
  if (options&UseProxy)
    host_changed = false;

  // Reset Client
  reset(host_changed);

  if (FXString::comparecase(method,"HEAD")==0) {
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
    command += "Content-Length: " + FXString::value(message.length()) + "\r\n";

  // Add Accept Encoding
  if (options&AcceptEncodingGZip)
    command += "Accept-Encoding: gzip\r\n";

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


FXbool HttpClient::basic(const FXchar*    method,
                         FXString         url,
                         const FXString & header,
                         const FXString & content,
                         FXString*        moved/*=nullptr*/) {

  int redirect = 0;

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

            // Prevent infinite redirects
            if (redirect>10)
              return false;

            // Save moved url (only on first permanent redirect)
            if (status.code==HTTP_MOVED_PERMANENTLY && redirect==0 && moved) {
              *moved=url;
              }
            // Don't do automatic redirections for non GET/HEAD requests
            if (FXString::comparecase(method,"GET") && FXString::comparecase(method,"HEAD"))
              return true;

            if (!request(method,url,header,content)) {
              return false;
              }
            redirect++;
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

              if (FXString::comparecase(challenge,"basic",5)==0) {
                FXString auth = "Authorization: Basic " + Base64Encoder::encodeString(user+":"+password) + "\r\n";

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
            GM_DEBUG_PRINT("[http] response failed\n");
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
