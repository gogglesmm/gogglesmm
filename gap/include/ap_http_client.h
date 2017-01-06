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
#ifndef AP_HTTP_CLIENT_H
#define AP_HTTP_CLIENT_H

namespace ap {

class GMAPI HttpHost {
public:
  FXString name;
  FXint    port;
public:
  HttpHost();

  // HttpHost from url
  HttpHost(const FXString & url);

  // Set from url. returns true if changed
  FXbool set(const FXString & url);

  // Clear
  void clear();
  };


class ConnectionFactory;

class GMAPI HttpClient : public HttpResponse {
protected:
  ConnectionFactory* connection;
  HttpHost      		 server;
  HttpHost      		 proxy;
  FXuchar       		 options;
protected:
  enum {
    UseProxy           = (1<<0),
    };
protected:
  FXbool open_connection();
  void reset(FXbool forceclose);
public:
  enum {
    AcceptEncodingGZip = (1<<1),
    };
public:
  HttpClient(ConnectionFactory * c=nullptr);

  /// Change the Connection Factory
  void setConnectionFactory(ConnectionFactory*);

  // Close Connection
  void close();

  // Discard response
  void discard() override;

  // Set Accept Encoding
  void setAcceptEncoding(const FXuchar encodings);

  // Send a request. Response can be
  FXbool request(const FXchar * method,const FXString & url,const FXString & headers=FXString::null,const FXString & message=FXString::null);

  // Perform basic request and handles some basic HTTP features
  FXbool basic(const FXchar *   method,
               FXString         url,
               const FXString & headers=FXString::null,
               const FXString & body=FXString::null,
               FXString*        moved=nullptr);
  ~HttpClient();
  };

}

#endif
