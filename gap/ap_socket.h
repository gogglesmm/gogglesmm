/*******************************************************************************
*                         Goggles Audio Player Library                         *
********************************************************************************
*           Copyright (C) 2010-2016 by Sander Jansen. All Rights Reserved      *
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
#ifndef AP_SOCKET_H
#define AP_SOCKET_H

namespace ap {

class Socket : public FXIODevice {
private:
  Socket(const Socket&);
  Socket &operator=(const Socket&);
  Socket(FXInputHandle h,FXuint mode);
public:
  enum {
    EndOfStream = 2048
    };
public:
  static Socket* create(int domain,int type,int protocol,FXuint mode);

  FXbool setKeepAlive(FXbool);

  FXbool getKeepAlive() const;

  FXbool setReuseAddress(FXbool);

  FXbool getReuseAddress() const;

  FXbool setReceiveTimeout(FXTime);

  FXbool setSendTimeout(FXTime);

  FXbool setLinger(FXTime);

  FXTime getLinger() const;

  FXint getError() const;

  /// Read block of bytes, returning number of bytes read
  FXival readBlock(void* data,FXival count);

  /// Write block of bytes, returning number of bytes written
  FXival writeBlock(const void* data,FXival count);

  /// Close Socket
  FXbool close();

  /// Handle eof
  FXint eof();
  };

}
#endif
