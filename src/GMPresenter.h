/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2006-2021 by Sander Jansen. All Rights Reserved      *
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
#ifndef GMPRESENTER_H
#define GMPRESENTER_H
#ifdef HAVE_OPENGL


class GMImageTexture;
class GMBouncingImage;

class GMPresenter : public FXDialogBox {
FXDECLARE(GMPresenter)
protected:
  GMImageTexture  * texture = nullptr;
  GMBouncingImage * effect = nullptr;
protected:
  FXGLCanvas * glcanvas = nullptr;
public:
  enum {
    ID_CANVAS = FXMainWindow::ID_LAST,
    ID_ANIMATION,
    };
public:
  long onCanvasPaint(FXObject*,FXSelector,void*);
  long onAnimation(FXObject*,FXSelector,void*);
protected:
  GMPresenter(){}
  GMPresenter(const GMPresenter&);
  GMPresenter& operator=(const GMPresenter&);
public:
  /// Construct Remote Window
  GMPresenter(FXApp* a,FXGLContext*,FXObject*,FXSelector);

  /// Create
  virtual void create();

  FXuint execute(FXuint placement=PLACEMENT_CURSOR);

  void setImage(FXImage * image);

  /// Destroy calculator
  virtual ~GMPresenter();
  };

#endif
#endif
