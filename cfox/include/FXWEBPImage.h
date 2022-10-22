/********************************************************************************
*                                                                               *
*                         W E B P   I m a g e   O b j e c t                     *
*                                                                               *
*********************************************************************************
* Copyright (C) 1999,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#ifndef FXWEBPIMAGE_H
#define FXWEBPIMAGE_H

#ifndef FXIMAGE_H
#include "FXImage.h"
#endif

namespace FX {


/// Google WebP Image class
class FXAPI FXWEBPImage : public FXImage {
  FXDECLARE(FXWEBPImage)
protected:
  FXWEBPImage(){}
private:
  FXWEBPImage(const FXWEBPImage&);
  FXWEBPImage &operator=(const FXWEBPImage&);
public:
  static const FXchar fileExt[];
  static const FXchar mimeType[];
public:

  /// Construct an image from memory stream formatted in WEBP format
  FXWEBPImage(FXApp *a,const FXuchar *pix=nullptr,FXuint opts=0,FXint w=1,FXint h=1);

  /// True if format is supported
  static const FXbool supported;

  /// Load pixels from stream in WEBP format
  virtual FXbool savePixels(FXStream& store) const;

  /// Save pixels into stream in WEBP format
  virtual FXbool loadPixels(FXStream& store);

  /// Destroy
  virtual ~FXWEBPImage();
  };


#ifndef FXLOADWEBP
#define FXLOADWEBP

/**
* Check if stream contains a WEBP, return true if so.
*/
extern FXAPI FXbool fxcheckWEBP(FXStream& store);


/**
* Load an WEBP image file from a stream.
* Upon successful return, the pixel array and size are returned.
* If an error occurred, the pixel array is set to NULL.
*/
extern FXAPI FXbool fxloadWEBP(FXStream& store,FXColor*& data,FXint& width,FXint& height);


/**
* Save an WEBP image file to a stream.
*/
extern FXAPI FXbool fxsaveWEBP(FXStream& store,const FXColor* data,FXint width,FXint height,FXfloat quality);

#endif

}

#endif
