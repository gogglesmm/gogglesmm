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
#include "gmdefs.h"
#include "GMImageView.h"

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glu.h>

// non power of two textures are currently broken in Mesa 7.10.1 in combination with mip mapping
// now fixed in Mesa 7.11
//#define DISABLE_TEXTURE_NON_POWER_OF_TWO 1

// Disable MIPMAP if needed
//#define DISABLE_MIPMAP 1

FXDEFMAP(GMImageView) GMImageViewMap[]={
  FXMAPFUNC(SEL_PAINT,0,GMImageView::onPaint)
  };

FXIMPLEMENT(GMImageView,FXGLCanvas,GMImageViewMap,ARRAYNUMBER(GMImageViewMap));


GMImageView::GMImageView(){
  image_width=0;
  image_height=0;
  texture_id=0;
  }

GMImageView::GMImageView(FXComposite* p,FXGLVisual *vis,FXuint opts,FXint x,FXint y,FXint w,FXint h) : FXGLCanvas(p,vis,NULL,0,opts,x,y,w,h){
  image_width=0;
  image_height=0;
  texture_id=0;
  }

GMImageView::~GMImageView(){
  image_width=0;
  image_height=0;
  texture_id=0;
  }

void GMImageView::updateTexture(FXImage * image) {
  FXint texture_max;

  if (!makeCurrent()) return;
  if (image) {

    image_width=image->getWidth();
    image_height=image->getHeight();

    /// Query Maximum Texture Size
    glGetIntegerv(GL_MAX_TEXTURE_SIZE,&texture_max);

#ifndef DISABLE_TEXTURE_NON_POWER_OF_TWO
    /// Check if non power of two textures are supported.
    const GLubyte * extensions = glGetString(GL_EXTENSIONS);
    texture_power_of_two = (strstr((const char*)extensions,"GL_ARB_texture_non_power_of_two")==NULL);
#else
    texture_power_of_two = true;
#endif

    /// Prescale to maximum texture size if necessary
    if((image_width>texture_max) || (image_height>texture_max)){

      if(image_width>image_height)
        image->scale(texture_max,(texture_max*image_height)/image_width,1);
      else
        image->scale((texture_max*image_width)/image_height,texture_max,1);

      image_width=image->getWidth();
      image_height=image->getHeight();
      }

    /// Get a nice texture size
    if (texture_power_of_two) {
      texture_width=1;
      texture_height=1;
      while(image_width>texture_width) texture_width<<=1;
      while(image_height>texture_height) texture_height<<=1;
      }
    else {
      texture_width=image_width;
      texture_height=image_height;
      }

    FXASSERT(texture_width<=texture_max);
    FXASSERT(texture_height<=texture_max);

    /// Generate a new texture if required.
    if (texture_id==0) {
      glGenTextures(1,&texture_id);
      }
    glBindTexture(GL_TEXTURE_2D,texture_id);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
#if (defined(GL_VERSION_3_0) || defined(GL_VERSION_1_4)) && !defined(DISABLE_MIPMAP)
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    #ifndef GL_VERSION_3_0
    glHint(GL_GENERATE_MIPMAP_HINT,GL_NICEST);
    glTexParameteri(GL_TEXTURE_2D,GL_GENERATE_MIPMAP,GL_TRUE);
    #endif
#else
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
#endif

   if (texture_width==image_width && texture_height==image_height) {
      glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,texture_width,texture_height,0,GL_BGRA,GL_UNSIGNED_INT_8_8_8_8_REV,image->getData());
      }
    else {
      glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,texture_width,texture_height,0,GL_BGRA,GL_UNSIGNED_INT_8_8_8_8_REV,NULL);
      glTexSubImage2D(GL_TEXTURE_2D,0,0,0,image_width,image_height,GL_BGRA,GL_UNSIGNED_INT_8_8_8_8_REV,image->getData());
      }

#if defined(GL_VERSION_3_0) && !defined(DISABLE_MIPMAP)
    glGenerateMipmap(GL_TEXTURE_2D);
#endif
    }
  else {
    image_width=0;
    image_height=0;
    if (texture_id) {
      glDeleteTextures(1,&texture_id);
      texture_id=0;
      }
    }
  makeNonCurrent();
  }


void GMImageView::setImage(FXImage * img) {
  updateTexture(img);
  recalc();
  update();
  }


FXint GMImageView::getDefaultWidth() const {
  return 256;
  }

FXint GMImageView::getDefaultHeight() const {
  return 256;
  }


// Repaint the GL window
long GMImageView::onPaint(FXObject*,FXSelector,void*){
  FXGLVisual *vis=(FXGLVisual*)getVisual();
  FXfloat th=1.0f;
  FXfloat tw=1.0f;
  FXVec4f background=colorToVec4f(backColor);
  FXASSERT(xid);
  if(makeCurrent()){
    glViewport(0,0,getWidth(),getHeight());

    glClearColor(background.x,background.y,background.z,background.w);
    glClear(GL_COLOR_BUFFER_BIT);

    if (image_width && image_height) {
      glMatrixMode(GL_MODELVIEW);
      glLoadIdentity();
      gluOrtho2D(0,getWidth(),getHeight(),0);

      glEnable(GL_TEXTURE_2D);
      glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_DECAL);
      glBindTexture(GL_TEXTURE_2D,texture_id);

      FXfloat scale = FXMIN(( width/(FXfloat)image_width),(height/(FXfloat)image_height));
      FXint xmin=0,xmax=image_width*scale;
      FXint ymin=0,ymax=image_height*scale;
      xmin=(getWidth()-xmax)/2.0;
      ymin=(getHeight()-ymax)/2.0;
      xmax+=xmin;
      ymax+=ymin;

      if (texture_width!=image_width || texture_height!=image_height){
        th = (1.0f / (FXfloat)(texture_height))*image_height;
        tw = (1.0f / (FXfloat)(texture_width))*image_width;
        }

#ifndef GL_VERSION_1_1
      FXint      coords[8]={xmin,ymin,
                            xmin,ymax,
                            xmax,ymax,
                            xmax,ymin};
      FXfloat       tex[8]={0.0f,0.0f,
                            0.0f,th,
                            tw,th,
                            tw,0.0f};
      FXfloat   colors[12]={background.x,background.y,background.z,
                            background.x,background.y,background.z,
                            background.x,background.y,background.z,
                            background.x,background.y,background.z};

      glEnableClientState(GL_VERTEX_ARRAY);
      glEnableClientState(GL_COLOR_ARRAY);
      glEnableClientState(GL_TEXTURE_COORD_ARRAY);

      glVertexPointer(2,GL_INT,0,coords);
      glColorPointer(3,GL_FLOAT,0,colors);
      glTexCoordPointer(2,GL_FLOAT,0,tex);

      glDrawArrays(GL_QUADS,0,4);

      glDisableClientState(GL_TEXTURE_COORD_ARRAY);
      glDisableClientState(GL_COLOR_ARRAY);
      glDisableClientState(GL_VERTEX_ARRAY);
#else
      glColor4fv(background);
      glBegin(GL_QUADS);
        glTexCoord2f(0.0,0.0); glVertex2i(xmin,ymin);
        glTexCoord2f(0.0,th);  glVertex2i(xmin,ymax);
        glTexCoord2f(tw,th);   glVertex2i(xmax,ymax);
        glTexCoord2f(tw,0.0);  glVertex2i(xmax,ymin);
      glEnd();
#endif

      glDisable(GL_TEXTURE_2D);
      }
    if(vis->isDoubleBuffer()) swapBuffers();
    makeNonCurrent();
    }
  return 1;
  }
