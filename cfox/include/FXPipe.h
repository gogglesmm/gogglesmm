/********************************************************************************
*                                                                               *
*                             P i p e   C l a s s                               *
*                                                                               *
*********************************************************************************
* Copyright (C) 2005,2020 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#ifndef FXPIPE_H
#define FXPIPE_H

#ifndef FXIODEVICE_H
#include "FXIODevice.h"
#endif

namespace FX {


/**
* Pipe i/o device.
*/
class FXAPI FXPipe : public FXIODevice {
private:
  FXPipe(const FXPipe&);
  FXPipe &operator=(const FXPipe&);
public:

  /// Construct pipe
  FXPipe(){ }

  /// Construct and open pipes with access mode m for this one and the reverse for the other
  FXPipe(FXPipe& other,FXuint m);

  /// Construct pipe and attach existing handle h
  FXPipe(FXInputHandle h,FXuint m=FXIO::Reading);

  /// Open pipes with access mode m for this one and the reverse for the other
  virtual FXbool open(FXPipe& other,FXuint m=FXIO::Reading);

  /// Open device with access mode and handle
  virtual FXbool open(FXInputHandle h,FXuint m=FXIO::Reading);

  /// Create a named pipe
  static FXbool create(const FXString& file,FXuint perm=FXIO::AllReadWrite);
  };

}

#endif
