/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2009-2014 by Sander Jansen. All Rights Reserved      *
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
#ifdef HAVE_OPENGL
#include "GMImageView.h"
#include <GL/glew.h>

GMImageTexture::GMImageTexture() :
  id(0),
  cw(1.0f),
  ch(1.0f),
  aspect(1.0f) {}

GMImageTexture::~GMImageTexture() {
  FXASSERT(id==0);
  }

FXbool GMImageTexture::setImage(FXImage* image) {
  if (image) {
    FXint image_width  = image->getWidth();
    FXint image_height = image->getHeight();
    FXint texture_width,texture_height;
    FXint texture_max;


    /// Query Maximum Texture Size
    glGetIntegerv(GL_MAX_TEXTURE_SIZE,&texture_max);

    /// Prescale to maximum texture size if necessary
    if((image_width>texture_max) || (image_height>texture_max)){

      if(image_width>image_height)
        image->scale(texture_max,(texture_max*image_height)/image_width,FOX_SCALE_BEST);
      else
        image->scale((texture_max*image_width)/image_height,texture_max,FOX_SCALE_BEST);

      image_width=image->getWidth();
      image_height=image->getHeight();
      }

    // aspect ratio
    aspect = image->getWidth() / (FXfloat) image->getHeight();

    /// Get a nice texture size
    if (GLEW_ARB_texture_non_power_of_two) {
      texture_width=image_width;
      texture_height=image_height;
      }
    else {
      texture_width=1;
      texture_height=1;
      while(image_width>texture_width) texture_width<<=1;
      while(image_height>texture_height) texture_height<<=1;
      }

    FXASSERT(texture_width<=texture_max);
    FXASSERT(texture_height<=texture_max);

    /// Generate a new texture if required.
    if (id==0) {
      glGenTextures(1,&id);
      }

    glBindTexture(GL_TEXTURE_2D,id);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);

    FXbool use_mipmap = (glGenerateMipmap!=NULL || GLEW_VERSION_1_4 || GLEW_SGIS_generate_mipmap );

    if (use_mipmap) {
      glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
      glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
      if (glGenerateMipmap==NULL){
        glHint(GL_GENERATE_MIPMAP_HINT,GL_NICEST);
        glTexParameteri(GL_TEXTURE_2D,GL_GENERATE_MIPMAP,GL_TRUE);
        }
      }
    else {
      glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
      }

   if (texture_width==image_width && texture_height==image_height) {
      glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,texture_width,texture_height,0,GL_BGRA,GL_UNSIGNED_INT_8_8_8_8_REV,image->getData());
      cw=ch=1.0f;
      }
    else {
      glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,texture_width,texture_height,0,GL_BGRA,GL_UNSIGNED_INT_8_8_8_8_REV,NULL);
      glTexSubImage2D(GL_TEXTURE_2D,0,0,0,image_width,image_height,GL_BGRA,GL_UNSIGNED_INT_8_8_8_8_REV,image->getData());
      cw = (1.0f / (FXfloat)(texture_width))  * image_width;
      ch = (1.0f / (FXfloat)(texture_height)) * image_height;
      }

    if (glGenerateMipmap)
      glGenerateMipmap(GL_TEXTURE_2D);

    }
  else {
    if (id) {
      glDeleteTextures(1,&id);
      id=0;
      }
    }
  return true;
  }


void GMImageTexture::drawQuad(FXfloat x,FXfloat y,FXfloat width,FXfloat height,FXColor background) {
  const FXfloat coordinates[8] = { x,y,
                                   x,y+height,
                                   x+width,y+height,
                                   x+width,y };

  const FXfloat tex[8] = { 0.0f,ch,
                           0.0f,0.0f,
                           cw,0.0f,
                           cw,ch };

  const FXuchar colors[16] = { FXREDVAL(background),FXBLUEVAL(background),FXGREENVAL(background),
                               FXREDVAL(background),FXBLUEVAL(background),FXGREENVAL(background),
                               FXREDVAL(background),FXBLUEVAL(background),FXGREENVAL(background),
                               FXREDVAL(background),FXBLUEVAL(background),FXGREENVAL(background) };



  glEnable(GL_TEXTURE_2D);
  glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_DECAL);
  glBindTexture(GL_TEXTURE_2D,id);
  glEnableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);
  glEnableClientState(GL_COLOR_ARRAY);
  glColorPointer(3,GL_UNSIGNED_BYTE,0,colors);
  glVertexPointer(2,GL_FLOAT,0,coordinates);
  glTexCoordPointer(2,GL_FLOAT,0,tex);
  glDrawArrays(GL_QUADS,0,4);
  glDisableClientState(GL_COLOR_ARRAY);
  glDisableClientState(GL_TEXTURE_COORD_ARRAY);
  glDisableClientState(GL_VERTEX_ARRAY);
  }



FXDEFMAP(GMImageView) GMImageViewMap[]={
  FXMAPFUNC(SEL_PAINT,0,GMImageView::onPaint)
  };

FXIMPLEMENT(GMImageView,FXGLCanvas,GMImageViewMap,ARRAYNUMBER(GMImageViewMap));


GMImageView::GMImageView(){
  texture=NULL;
  }

GMImageView::GMImageView(FXComposite* p,FXGLContext *ctx,FXuint opts,FXint x,FXint y,FXint w,FXint h) : FXGLCanvas(p,ctx,NULL,0,opts,x,y,w,h){
  texture=NULL;
  }

GMImageView::~GMImageView(){
  delete texture;
  }

void GMImageView::setImage(FXImage * image) {
  if (makeCurrent()) {
    if (texture==NULL && image) {
      texture = new GMImageTexture();
      }
    if (texture) texture->setImage(image);
    makeNonCurrent();
    }
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
  FXfloat aspect = getWidth() / (float)getHeight();
  FXfloat xwidth = 1.0f*aspect;
  FXfloat size;

  FXVec4f background=colorToVec4f(backColor);
  FXASSERT(xid);
  if(makeCurrent()){
    glViewport(0,0,getWidth(),getHeight());
    glClearColor(background.x,background.y,background.z,background.w);
    glClear(GL_COLOR_BUFFER_BIT);

    if (texture && texture->id) {
      glMatrixMode(GL_MODELVIEW);
      glLoadIdentity();
      gluOrtho2D(0,xwidth,0.0f,1.0f);

      if (aspect>=texture->aspect) {
        size = 1.0f*texture->aspect;
        texture->drawQuad(0.5f*(xwidth-size),0.0f,size,1.0f,backColor);
        }
      else {
        size = xwidth / texture->aspect;
        texture->drawQuad(0.0f,0.5f * (1.0f-size),xwidth,size,backColor);
        }
      }
    if(vis->isDoubleBuffer()) swapBuffers();
    makeNonCurrent();
    }
  return 1;
  }
#endif
