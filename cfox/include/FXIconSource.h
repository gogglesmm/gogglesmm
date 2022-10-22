/********************************************************************************
*                                                                               *
*                            I c o n   S o u r c e                              *
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
#ifndef FXICONSOURCE_H
#define FXICONSOURCE_H

#ifndef FXOBJECT_H
#include "FXObject.h"
#endif

namespace FX {

class FXApp;
class FXIcon;
class FXImage;

/**
* An icon source is a class that loads an icon of any type.
* It exists purely for convenience, to make loading icons simpler by concentrating the
* knowledge of the supported icon formats into a single place.
* Needless to say, this class is subclassable, allowing users to add additional icon
* types and make them available to all widgets which deal with icons.
* When icons are loaded, they are NOT created (realized) yet; this allows users to
* manipulate the pixel data prior to realizing the icons.
* Image/icon formats can be determined by file extension type, or by their contents.
* The latter method may be less reliable for some types of file formats which don't
* have well-defined file-header signatures. File signature recognition is always
* attempted when file extension method fails, as a fallback method.
*/
class FXAPI FXIconSource : public FXObject {
  FXDECLARE(FXIconSource)
private:
  FXImage *scaleToSize(FXImage *image,FXint size,FXint qual) const;
public:

  /**
  * Default icon source provides icons and images for all types built into
  * the FOX library at compile time.
  */
  static FXIconSource defaultIconSource;

  /**
  * Determine icon type from extension string, return icon of that type.
  * Return NULL if no match.
  */
  virtual FXIcon *iconFromType(FXApp* app,const FXString& type) const;

  /**
  * Determine icon type from header bytes in stream, return icon of that type.
  * Rewind the stream to the start. Return NULL if no match.
  */
  virtual FXIcon *iconFromStream(FXApp* app,FXStream& store) const;

  /**
  * Load an icon from the file filename. By default, the file extension is
  * stripped and used as the icon type; if an explicit icon type is forced,
  * then that type is used and the extension is ignored.
  * For example, loadIcon("icon","gif") will try to load a CompuServe GIF
  * file, since the filename does not give any clue as to the type of the
  * icon.
  */
  virtual FXIcon *loadIconFile(FXApp* app,const FXString& filename,const FXString& type=FXString::null) const;

  /**
  * Load an icon of a given type (e.g. "gif") from reswrapped data.
  * Returns NULL if there's some error loading the icon.  [The optional
  * parameter is actually mandatory at the time of this writing; future
  * versions will attempt to inspect the first few bytes of the stream
  * to divine the icon format if the parameter is omitted].
  */
  virtual FXIcon *loadIconData(FXApp* app,const FXuchar *pixels,const FXString& type=FXString::null) const;

  /**
  * Load an icon of a given type (e.g. "gif") from an already open stream.
  * Returns NULL if there's some error loading the icon.  [The optional
  * parameter is actually mandatory at the time of this writing; future
  * versions will attempt to inspect the first few bytes of the stream
  * to divine the icon format if the parameter is omitted].
  */
  virtual FXIcon *loadIconStream(FXApp* app,FXStream& store,const FXString& type=FXString::null) const;


  /**
  * Determine image type from extension string, return image of that type.
  * Return NULL if no match.
  */
  virtual FXImage *imageFromType(FXApp* app,const FXString& type) const;

  /**
  * Determine image type from header bytes in stream, return image of that type.
  * Rewind the stream to the start. Return NULL if no match.
  */
  virtual FXImage *imageFromStream(FXApp* app,FXStream& store) const;

  /**
  * Load an image from the file filename. By default, the file extension is
  * stripped and used as the image type; if an explicit image type is forced,
  * then that type is used and the extension is ignored.
  * For example, loadImage("image","gif") will try to load a CompuServe GIF
  * file, since the filename does not give any clue as to the type of the
  * image.
  */
  virtual FXImage *loadImageFile(FXApp* app,const FXString& filename,const FXString& type=FXString::null) const;

  /**
  * Load an image of a given type (e.g. "gif") from reswrapped data.
  * Returns NULL if there's some error loading the icon.  [The optional
  * parameter is actually mandatory at the time of this writing; future
  * versions will attempt to inspect the first few bytes of the stream
  * to divine the icon format if the parameter is omitted].
  */
  virtual FXImage *loadImageData(FXApp* app,const FXuchar *pixels,const FXString& type=FXString::null) const;

  /**
  * Load an image of a given type (e.g. "gif") from an already open stream.
  * Returns NULL if there's some error loading the image.  [The optional
  * parameter is actually mandatory at the time of this writing; future
  * versions will attempt to inspect the first few bytes of the stream
  * to divine the image format if the parameter is omitted].
  */
  virtual FXImage *loadImageStream(FXApp* app,FXStream& store,const FXString& type=FXString::null) const;


  /**
  * Load an icon from filename and scale it such that its dimensions does not exceed given size.
  * The icon type is determined from the filename extension unless explicitly forced.
  */
  virtual FXIcon *loadScaledIconFile(FXApp* app,const FXString& filename,FXint size=32,FXint qual=0,const FXString& type=FXString::null) const;

  /**
  * Load an icon from pixels and scale it such that its dimensions does not exceed given size.
  */
  virtual FXIcon *loadScaledIconData(FXApp* app,const FXuchar *pixels,FXint size=32,FXint qual=0,const FXString& type=FXString::null) const;

  /**
  * Load icon from stream and scale it such that its dimensions does not exceed given size.
  */
  virtual FXIcon *loadScaledIconStream(FXApp* app,FXStream& store,FXint size=32,FXint qual=0,const FXString& type=FXString::null) const;

  /**
  * Load image from filename and scale it such that its dimensions does not exceed given size.
  * The image type is determined from the filename extension unless explicitly forced.
  */
  virtual FXImage *loadScaledImageFile(FXApp* app,const FXString& filename,FXint size=32,FXint qual=0,const FXString& type=FXString::null) const;

  /**
  * Load image and scale it such that its dimensions does not exceed given size.
  */
  virtual FXImage *loadScaledImageData(FXApp* app,const FXuchar *pixels,FXint size=32,FXint qual=0,const FXString& type=FXString::null) const;

  /**
  * Load image and scale it such that its dimensions does not exceed given size.
  */
  virtual FXImage *loadScaledImageStream(FXApp* app,FXStream& store,FXint size=32,FXint qual=0,const FXString& type=FXString::null) const;
  };


}

#endif
