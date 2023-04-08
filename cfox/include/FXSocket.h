/********************************************************************************
*                                                                               *
*                           S o c k e t   C l a s s                             *
*                                                                               *
*********************************************************************************
* Copyright (C) 2005,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
*********************************************************************************
* This library is free software; you can redistribute it and/or modify          *
* it under the terms of the GNU Lesser General Public License as published by   *
* the Free Software Foundation; either version 3 of the License, or             *
* (at your option) any later version.                                           *
*                                                                               *
* This library is distributed in the hope that it will be useful,               *
* but WITHOUT ANY WARRANTY; without even the implied warranty of                *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                 *
* GNU Lesser General Public License for more details.                           *
*                                                                               *
* You should have received a copy of the GNU Lesser General Public License      *
* along with this program.  If not, see <http://www.gnu.org/licenses/>          *
********************************************************************************/
#ifndef FXSOCKET_H
#define FXSOCKET_H

#ifndef FXIODEVICE_H
#include "FXIODevice.h"
#endif


////////////////////////////  UNDER DEVELOPMENT  ////////////////////////////////


namespace FX {


/**
* Socket i/o device.
*/
class FXAPI FXSocket : public FXIODevice {
private:
  FXSocket(const FXSocket&);
  FXSocket &operator=(const FXSocket&);
public:

  /// Construct socket
  FXSocket(){ }

  /// Construct file and attach existing handle h
  FXSocket(FXInputHandle h);
  };

}

#endif
