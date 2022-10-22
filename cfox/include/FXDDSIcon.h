/********************************************************************************
*                                                                               *
*                        D D S   I c o n   O b j e c t                          *
*                                                                               *
*********************************************************************************
* Copyright (C) 2008,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#ifndef FXDDSICON_H
#define FXDDSICON_H

#ifndef FXICON_H
#include "FXIcon.h"
#endif

namespace FX {


/**
* The DDS Icon class is a convenience class for working with icons in the
* Direct Draw Surface (DDS) file format.
*/
class FXAPI FXDDSIcon : public FXIcon {
  FXDECLARE(FXDDSIcon)
protected:
  FXDDSIcon(){}
private:
  FXDDSIcon(const FXDDSIcon&);
  FXDDSIcon &operator=(const FXDDSIcon&);
public:
  static const FXchar fileExt[];
  static const FXchar mimeType[];
public:

  /// Construct icon from memory stream formatted in DDS format
  FXDDSIcon(FXApp* a,const FXuchar *pix=nullptr,FXColor clr=FXRGB(192,192,192),FXuint opts=0,FXint w=1,FXint h=1);

  /// Save pixels into stream in DDS format
  virtual FXbool savePixels(FXStream& store) const;

  /// Load pixels from stream in DDS format
  virtual FXbool loadPixels(FXStream& store);

  /// Destroy icon
  virtual ~FXDDSIcon();
  };


#ifndef FXLOADDDS
#define FXLOADDDS

/**
* Check if stream contains a DDS format, return true if so.
*/
extern FXAPI FXbool fxcheckDDS(FXStream& store);


/**
* Load an DDS (Direct Draw Surface) file from a stream.
* Upon successful return, the pixel array and size are returned.
* If an error occurred, the pixel array is set to NULL.
*/
extern FXAPI FXbool fxloadDDS(FXStream& store,FXColor*& data,FXint& width,FXint& height,FXint& depth);

/**
* Save a DDS (Direct Draw Surface) file to a stream.
*/
extern FXAPI FXbool fxsaveDDS(FXStream& store,FXColor* data,FXint width,FXint height,FXint depth);

#endif

}

#endif
