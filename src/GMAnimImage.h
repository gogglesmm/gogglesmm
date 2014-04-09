/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2006-2014 by Sander Jansen. All Rights Reserved      *
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
#ifndef GMANIMIMAGE_H
#define GMANIMIMAGE_H

#if 0
class GMAnimation : public FXObject {
FXDECLARE(GMAnimation)
protected:
  FXImage * image;
  FXuint    index;
  FXuint    base;
protected:
  GMAnimation(){}
private:
  GMAnimation(const GMAnimation&);
  GMAnimation& operator=(const GMAnimation&);
public:
  GMAnimation(FXImage * img,FXuint base);

  FXint getWidth() const { return base; }

  FXint getHeight() const { return base; }

  void advance();

  void draw(FXDC & dc,FXint x,FXint y); 
  };
#endif

class GMAnimImage : public FXImageFrame {
FXDECLARE(GMAnimImage)
protected:
  FXuint index;
  FXuint base;
protected:
  GMAnimImage();
private:
  GMAnimImage(const GMAnimImage&);
  GMAnimImage& operator=(const GMAnimImage&);
public:
  enum {
    ID_TIMER = FXImageFrame::ID_LAST,
    ID_LAST
    };
public:
  long onPaint(FXObject*,FXSelector,void*);
  long onTimer(FXObject*,FXSelector,void*);
public:
  /// Construct image frame and pass it an image
  GMAnimImage(FXComposite* p,FXImage *img,FXint base,FXuint opts=FRAME_SUNKEN|FRAME_THICK,FXint x=0,FXint y=0,FXint w=0,FXint h=0,FXint pl=0,FXint pr=0,FXint pt=0,FXint pb=0);

  virtual void hide();
  
  virtual void show();

  virtual void create();

  /// Get default width
  virtual FXint getDefaultWidth();

  /// Get default height
  virtual FXint getDefaultHeight();

  ~GMAnimImage();
  };



#endif
