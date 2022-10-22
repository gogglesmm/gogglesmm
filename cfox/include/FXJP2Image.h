/********************************************************************************
*                                                                               *
*                   J P E G - 2 0 0 0   I m a g e   O b j e c t                 *
*                                                                               *
*********************************************************************************
* Copyright (C) 2009,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#ifndef FXJP2IMAGE_H
#define FXJP2IMAGE_H

#ifndef FXIMAGE_H
#include "FXImage.h"
#endif

namespace FX {


/// JPEG-2000 Image class
class FXAPI FXJP2Image : public FXImage {
  FXDECLARE(FXJP2Image)
protected:
  FXint quality;
protected:
  FXJP2Image(){}
private:
  FXJP2Image(const FXJP2Image&);
  FXJP2Image &operator=(const FXJP2Image&);
public:
  static const FXchar fileExt[];
  static const FXchar mimeType[];
public:

  /// Construct an image from memory stream formatted in JPEG-2000 format
  FXJP2Image(FXApp *a,const FXuchar *pix=nullptr,FXuint opts=0,FXint w=1,FXint h=1,FXint q=100);

  /// True if format is supported
  static const FXbool supported;

  /// Set image quality to save with
  void setQuality(FXint q){ quality=q; }

  /// Get image quality setting
  FXint getQuality() const { return quality; }

  /// Load pixels from stream in JPEG format
  virtual FXbool savePixels(FXStream& store) const;

  /// Save pixels into stream in JPEG format
  virtual FXbool loadPixels(FXStream& store);

  /// Destroy
  virtual ~FXJP2Image();
  };


#ifndef FXLOADJP2
#define FXLOADJP2

/**
* Check if stream contains a JP2, return true if so.
*/
extern FXAPI FXbool fxcheckJP2(FXStream& store);


/**
* Load an JPEG-2000 file from a stream.
* Upon successful return, the pixel array and size are returned.
* If an error occurred, the pixel array is set to NULL.
*/
extern FXAPI FXbool fxloadJP2(FXStream& store,FXColor*& data,FXint& width,FXint& height,FXint& quality);


/**
* Save an JPEG-2000 file to a stream.
*/
extern FXAPI FXbool fxsaveJP2(FXStream& store,const FXColor* data,FXint width,FXint height,FXint quality);

#endif

}

#endif
