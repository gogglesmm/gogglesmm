/********************************************************************************
*                                                                               *
*                     I R I S   R G B   I m a g e   O b j e c t                 *
*                                                                               *
*********************************************************************************
* Copyright (C) 2002,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#ifndef FXRGBIMAGE_H
#define FXRGBIMAGE_H

#ifndef FXIMAGE_H
#include "FXImage.h"
#endif

namespace FX {


/// IRIS RGB image
class FXAPI FXRGBImage : public FXImage {
  FXDECLARE(FXRGBImage)
protected:
  FXRGBImage(){}
private:
  FXRGBImage(const FXRGBImage&);
  FXRGBImage &operator=(const FXRGBImage&);
public:
  static const FXchar fileExt[];
  static const FXchar mimeType[];
public:

  /// Construct image from memory stream formatted in IRIS-RGB format
  FXRGBImage(FXApp* a,const FXuchar *pix=nullptr,FXuint opts=0,FXint w=1,FXint h=1);

  /// Save pixels into stream in IRIS-RGB format
  virtual FXbool savePixels(FXStream& store) const;

  /// Load pixels from stream in IRIS-RGB format
  virtual FXbool loadPixels(FXStream& store);

  /// Destroy icon
  virtual ~FXRGBImage();
  };


#ifndef FXLOADRGB
#define FXLOADRGB

/**
* Check if stream contains a RGB, return true if so.
*/
extern FXAPI FXbool fxcheckRGB(FXStream& store);


/**
* Load an RGB (SGI IRIS RGB) file from a stream.
* Upon successful return, the pixel array and size are returned.
* If an error occurred, the pixel array is set to NULL.
*/
extern FXAPI FXbool fxloadRGB(FXStream& store,FXColor*& data,FXint& width,FXint& height);


/**
* Save an RGB (SGI IRIS RGB) file to a stream.
*/
extern FXAPI FXbool fxsaveRGB(FXStream& store,const FXColor *data,FXint width,FXint height);

#endif

}

#endif
