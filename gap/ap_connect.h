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
#ifndef AP_CONNECT_H
#define AP_CONNECT_H

namespace ap {

class Socket;
class ThreadQueue;
struct IOContext;

/* Connection Factory */
class ConnectionFactory {
#if defined(HAVE_OPENSSL) || defined(HAVE_GNUTLS)
protected:
  FXbool use_ssl = false;
#endif
protected:
	virtual Socket * create(FXint domain,FXint type,FXint protocol);
public:
	ConnectionFactory();

	// Open connection to hostname and port
	FXIO * open(const FXString & hostname,FXint port);

	virtual ~ConnectionFactory();
	};

class ThreadConnectionFactory : public ConnectionFactory {
protected:
	IOContext * context;
protected:
	Socket * create(FXint domain,FXint type,FXint protocol) override;
public:
	ThreadConnectionFactory(IOContext*);
  };

}
#endif
