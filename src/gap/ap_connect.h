/*******************************************************************************
*                         Goggles Audio Player Library                         *
********************************************************************************
*           Copyright (C) 2010-2014 by Sander Jansen. All Rights Reserved      *
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

/* Connection Factory */
class ConnectionFactory {
protected:
	enum {
		Connected = 1,
		Error = 2,
	 	Abort	= 3
		};
protected:
	virtual FXIO* create(FXint domain,FXint type,FXint protocol);
	virtual FXuint connect(FXIO * socket,const struct sockaddr * address,FXint address_len);
public:
	ConnectionFactory();

	// Open connection to hostname and port
	FXIO * open(const FXString & hostname,FXint port);

	virtual ~ConnectionFactory();
	};

/* Non-Blocking Connection Factory */
class NBConnectionFactory : public ConnectionFactory {
protected:
	FXInputHandle watch;
protected:
	virtual FXIO * create(FXint domain,FXint type,FXint protocol);
	virtual FXuint connect(FXIO * io,const struct sockaddr * address,FXint address_len);
public:
	NBConnectionFactory(FXInputHandle);
	};

class ThreadQueue;

/* InputThread Connection Factory */
class ThreadConnectionFactory  : public NBConnectionFactory {
protected:
	ThreadQueue*fifo;
protected:
	virtual FXIO * create(FXint domain,FXint type,FXint protocol);
public:
	ThreadConnectionFactory(ThreadQueue*);
  };


}
#endif
