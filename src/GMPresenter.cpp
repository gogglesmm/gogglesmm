/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2006-2010 by Sander Jansen. All Rights Reserved      *
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
#include "icons.h"
#include "gmdefs.h"
#include <FXPNGIcon.h>
#include <FXPNGImage.h>
#include "GMTrack.h"
#include "GMList.h"
#include "GMSource.h"
#include "GMPlayerManager.h"
#include "GMCover.h"
#include "GMCoverManager.h"
#include "GMWindow.h"
#include "GMPresenter.h"
#include "GMIconTheme.h"

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glu.h>


// Map
FXDEFMAP(GMPresenter) GMPresenterMap[]={
  FXMAPFUNC(SEL_PAINT,GMPresenter::ID_CANVAS,GMPresenter::onCanvasPaint),
  FXMAPFUNC(SEL_TIMEOUT,GMPresenter::ID_ANIMATION,GMPresenter::onAnimation),
  };

// Implementation
FXIMPLEMENT(GMPresenter,FXDialogBox,GMPresenterMap,ARRAYNUMBER(GMPresenterMap))

GMPresenter::GMPresenter(FXApp* a,FXGLVisual * vis,FXObject * tgt,FXSelector msg):FXDialogBox(a,"Goggles Music Manager",DECOR_NONE,0,0,400,300,0,0,0,0,0,0){

  pos=FXVec2f(0.1f,0.1f);
  dir=FXVec2f(0.0003f,0.0005f);

  image_width=0;
  image_height=0;
  texture_id=0;


  setBackColor(FXRGB(0,0,0));
//  if (vis==NULL)
    glvisual = vis = new FXGLVisual(getApp(),VISUAL_DOUBLE_BUFFER);
//  else
//    glvisual = NULL;

  glcanvas = new FXGLCanvas(this,vis,this,ID_CANVAS,LAYOUT_FILL);
  }

// Destroy main window
GMPresenter::~GMPresenter(){
  delete glvisual;
  getApp()->removeTimeout(this,ID_ANIMATION);
  }

void GMPresenter::create() {
  FXDialogBox::create();
  getApp()->addTimeout(this,ID_ANIMATION,100000000);
  }




void GMPresenter::updateTexture(FXImage * image) {
  FXint texture_max;

  if (!glcanvas->makeCurrent()) return;
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
        image->scale(texture_max,(texture_max*image_height)/image_width,FOX_SCALE_BEST);
      else
        image->scale((texture_max*image_width)/image_height,texture_max,FOX_SCALE_BEST);

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
  glcanvas->makeNonCurrent();
  }





void GMPresenter::setImage(FXImage * image) {
  updateTexture(image);
  recalc();
  update();
  }






FXuint GMPresenter::execute(FXuint placement) {
  create();
  show(placement);
  fullScreen(true);
  glcanvas->showCursor(false);
  getApp()->refresh();
  return getApp()->runModalFor(this);
  }

long GMPresenter::onAnimation(FXObject*,FXSelector,void*){
  FXfloat aspect=getWidth()/(float)getHeight();
  FXVec2f n = pos+dir;
  if (n.x+0.4f>1.0*aspect)
    dir.x=-dir.x;
  if (n.y+0.4f>1.0)
    dir.y=-dir.y;
  if (n.x<0.0f)
    dir.x=-dir.x;
  if (n.y<0.0f)
    dir.y=-dir.y;

  pos+=dir;
  getApp()->addTimeout(this,ID_ANIMATION,10000000);
  glcanvas->update();
  return 1;
  }


long GMPresenter::onCanvasPaint(FXObject*,FXSelector,void*){
  FXfloat th=1.0f;
  FXfloat tw=1.0f;
  FXfloat aspect=getWidth()/(float)getHeight();
  FXVec4f background=colorToVec4f(FXRGBA(0,0,0,0));

  if (glcanvas->makeCurrent()) {
    glViewport(0,0,getWidth(),getHeight());
    glClearColor(0.0f,0.0f,0.0f,0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluOrtho2D(0.0f,1.0f*aspect,0.0f,1.0f);

   // glMatrixMode(GL_MODELVIEW);
   // glLoadIdentity();
/*  
  glBegin(GL_QUADS);
      glColor3f(1.0f,0.0f,0.00f);
      glVertex2f(0.0f,0.0f);
      glVertex2f(1.0f*aspect,0.0f);

      glColor3f(0.0f,0.0f,0.0f);
      glVertex2f(1.0f*aspect,0.5f);
      glVertex2f(0.0f,0.5f);
    glEnd();
*/



      //FXfloat scale =(float)image_width/(float)image_height;
//      FXfloat yscale =(height/(FXfloat)image_height);
      //FXint xmin=0,xmax=image_width*scale;
      //FXint ymin=0,ymax=image_height*scale;
/*
      xmin=(getWidth()-xmax)/2.0;
      ymin=(getHeight()-ymax)/2.0;
      xmax+=xmin;
      ymax+=ymin;

*/
      //fxmessage("scale %f\n",scale);
      FXfloat xmin=pos.x;
      FXfloat xmax=pos.x+0.4f;
      FXfloat ymin=pos.y;
      FXfloat ymax=pos.y+0.4f;
      //fxmessage("%g ymax=%g\n",aspect,ymax);


      glEnable(GL_TEXTURE_2D);
      glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_DECAL);
      glBindTexture(GL_TEXTURE_2D,texture_id);


      if (texture_width!=image_width || texture_height!=image_height){
        th = (1.0f / (FXfloat)(texture_height))*image_height;
        tw = (1.0f / (FXfloat)(texture_width))*image_width;
        }

#ifndef GL_VERSION_1_1
#if 0
      FXint      coords[8]={xmin,ymin,
                            xmin,ymax,
                            xmax,ymax,
                            xmax,ymin};
      FXfloat       tex[8]={0.0f,0.0f,
                            0.0f,th,
                            tw,th,
                            tw,0.0f};
/*
      FXfloat   colors[12]={background.x,background.y,background.z,
                            background.x,background.y,background.z,
                            background.x,background.y,background.z,
                            background.x,background.y,background.z};
*/
      glEnableClientState(GL_VERTEX_ARRAY);
  //    glEnableClientState(GL_COLOR_ARRAY);
      glEnableClientState(GL_TEXTURE_COORD_ARRAY);

      glVertexPointer(2,GL_INT,0,coords);
      //glColorPointer(3,GL_FLOAT,0,colors);
      glTexCoordPointer(2,GL_FLOAT,0,tex);

      glDrawArrays(GL_QUADS,0,4);

      glDisableClientState(GL_TEXTURE_COORD_ARRAY);
      //glDisableClientState(GL_COLOR_ARRAY);
      glDisableClientState(GL_VERTEX_ARRAY);
#endif
#else
      glColor4fv(background);
      glBegin(GL_QUADS);
        glTexCoord2f(0.0f,th); glVertex2f(xmin,ymin);
        glTexCoord2f(tw,th);  glVertex2f(xmax,ymin);
        glTexCoord2f(tw,0.0f);   glVertex2f(xmax,ymax);
        glTexCoord2f(0.0f,0.0f);  glVertex2f(xmin,ymax);
      glEnd();
#endif
    if (glcanvas->getContext()->isDoubleBuffer()) glcanvas->swapBuffers();
    glcanvas->makeNonCurrent();
    }
  return 1;
  }

