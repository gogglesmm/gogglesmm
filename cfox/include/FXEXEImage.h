/********************************************************************************
*                                                                               *
*                          E X E   I m a g e   O b j e c t                      *
*                                                                               *
*********************************************************************************
* Copyright (C) 2014,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#ifndef FXEXEIMAGE_H
#define FXEXEIMAGE_H

#ifndef FXIMAGE_H
#include "FXImage.h"
#endif

namespace FX {

/// Bitmap from resource in Microsoft Windows executable
class FXAPI FXEXEImage : public FXImage {
  FXDECLARE(FXEXEImage)
protected:
  FXint rtype;          // Resource type
  FXint rid;            // Resource id
protected:
  FXEXEImage(){}
private:
  FXEXEImage(const FXEXEImage&);
  FXEXEImage &operator=(const FXEXEImage&);
public:
  static const FXchar fileExt[];
  static const FXchar mimeType[];
public:

  /// Construct image from memory stream comprising Microsoft Windows executable
  FXEXEImage(FXApp* a,const FXuchar *pix=nullptr,FXuint opts=0,FXint w=1,FXint h=1,FXint ri=-1,FXint rt=3);

  /// Set resource group (type) to load from
  void setResType(FXint rt){ rtype=rt; }

  /// Get resource group (type)
  FXint getResType() const { return rtype; }

  /// Set resource id to load
  void setResId(FXint ri){ rid=ri; }

  /// Get resource id
  FXint getResId() const { return rid; }

  /// Save pixels into stream in Microsoft icon format
  virtual FXbool savePixels(FXStream& store) const;

  /// Load pixels from stream in Microsoft icon format
  virtual FXbool loadPixels(FXStream& store);

  /// Destroy image
  virtual ~FXEXEImage();
  };


#ifndef FXLOADEXE
#define FXLOADEXE

/**
* Check if stream represents a Windows executable.
*/
extern FXAPI FXbool fxcheckEXE(FXStream& store);


/**
* Pull an icon from a Microsoft Windows executable; try and load resource id under resource group type.
*/
extern FXAPI FXbool fxloadEXE(FXStream& store,FXColor*& data,FXint& width,FXint& height,FXint type,FXint id);


#endif

}

#endif
