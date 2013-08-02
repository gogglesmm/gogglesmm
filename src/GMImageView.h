/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2009-2010 by Sander Jansen. All Rights Reserved      *
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
#ifndef GMIMAGEVIEW_H
#define GMIMAGEVIEW_H


class GMImageTexture {
public:
  FXuint  id;           // texture id
  FXfloat cw;           // max texture coordinate width
  FXfloat ch;           // max texture coordinate height
  FXfloat aspect;       // aspect ratio of image
public:
  GMImageTexture();

  FXbool setImage(FXImage*);

  void drawQuad(FXfloat x,FXfloat y,FXfloat width,FXfloat height,FXColor background);

  ~GMImageTexture();
  };


class GMImageView : public FXGLCanvas {
FXDECLARE(GMImageView)
protected:
  GMImageTexture * texture;
public:
  long onPaint(FXObject*,FXSelector,void*);
protected:
  GMImageView();
private:
  GMImageView(const GMImageView&);
  GMImageView &operator=(const GMImageView&);
public:
  GMImageView(FXComposite* p,FXGLContext *ctx,FXuint opts=0,FXint x=0,FXint y=0,FXint w=0,FXint h=0);

  FXint getDefaultWidth() const;

  FXint getDefaultHeight() const;

  void setImage(FXImage * img);

  ~GMImageView();
  };

#endif
