/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2006-2015 by Sander Jansen. All Rights Reserved      *
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
#include "GMImageView.h"

#include <GL/glew.h>




class GMBouncingImage {
protected:
  GMImageTexture * texture;
  FXVec2f     pos;
  FXVec2f     dir;
  FXfloat     size;
public:
  GMBouncingImage(GMImageTexture*t) : texture(t), pos(0.0f,0.0f),dir(0.001f,0.001f),size(0.4f) {}

  void move(FXVec2f range) {
    if (texture) {
      FXVec2f n = pos+dir;
      if (n.x+(size*texture->aspect)>range.x)
        dir.x=-dir.x;
      if (n.y+size>range.y)
        dir.y=-dir.y;
      if (n.x<0.0f)
        dir.x=-dir.x;
      if (n.y<0.0f)
        dir.y=-dir.y;
      pos+=dir;
      }
    }

  void setTexture(GMImageTexture * t) {
    texture=t;
    }

  void draw() {
    if (texture && texture->id)
      texture->drawQuad(pos.x,pos.y,(size*texture->aspect),size,0);
    }
  };



// Map
FXDEFMAP(GMPresenter) GMPresenterMap[]={
  FXMAPFUNC(SEL_PAINT,GMPresenter::ID_CANVAS,GMPresenter::onCanvasPaint),
  FXMAPFUNC(SEL_TIMEOUT,GMPresenter::ID_ANIMATION,GMPresenter::onAnimation),
  };

// Implementation
FXIMPLEMENT(GMPresenter,FXDialogBox,GMPresenterMap,ARRAYNUMBER(GMPresenterMap))

GMPresenter::GMPresenter(FXApp* a,FXGLContext * ctx,FXObject * /*tgt*/,FXSelector /*msg*/):FXDialogBox(a,FXString::null,DECOR_TITLE|DECOR_RESIZE,0,0,400,300,0,0,0,0,0,0){
  texture=NULL;
  effect=NULL;
  glcanvas = new FXGLCanvas(this,ctx,this,ID_CANVAS,LAYOUT_FILL);
  }

// Destroy main window
GMPresenter::~GMPresenter(){
  if (texture) {
    glcanvas->makeCurrent();
    texture->setImage(NULL);
    glcanvas->makeNonCurrent();
    delete texture;
    }
  if (effect)
    delete effect;
  getApp()->removeTimeout(this,ID_ANIMATION);
  }

void GMPresenter::create() {
  FXDialogBox::create();
  getApp()->addTimeout(this,ID_ANIMATION,100000000);
  }

void GMPresenter::setImage(FXImage * image) {
  if (glcanvas->makeCurrent()) {
    if (image) {
      if (texture==NULL) {
        texture = new GMImageTexture();
        }
      if (effect==NULL) {
        effect  = new GMBouncingImage(texture);
        }
      else {
        effect->setTexture(texture);
        }
      texture->setImage(image);
      }
    else {
      if (texture) {
        texture->setImage(NULL);
        delete texture;
        texture=NULL;
        }
      }
    glcanvas->makeNonCurrent();
    }
  recalc();
  update();
  }






FXuint GMPresenter::execute(FXuint placement) {
  create();
  show(placement);
#ifndef DEBUG
  fullScreen(true);
  glcanvas->showCursor(false);
#endif
  getApp()->refresh();
  return getApp()->runModalFor(this);
  }

long GMPresenter::onAnimation(FXObject*,FXSelector,void*){
  FXfloat aspect=getWidth()/(float)getHeight();
  if (effect) effect->move(FXVec2f(aspect*1.0f,1.0f));
  getApp()->addTimeout(this,ID_ANIMATION,10000000);
  glcanvas->update();
  return 1;
  }

long GMPresenter::onCanvasPaint(FXObject*,FXSelector,void*){
  const FXfloat aspect=getWidth()/(float)getHeight();
  FXVec4f background=colorToVec4f(FXRGBA(0,0,0,0));
  if (glcanvas->makeCurrent()) {
    glViewport(0,0,getWidth(),getHeight());
    glClearColor(background.x,background.y,background.z,background.w);
    glClear(GL_COLOR_BUFFER_BIT);
    if (effect) {
      glMatrixMode(GL_MODELVIEW);
      glLoadIdentity();
      gluOrtho2D(0.0f,1.0f*aspect,0.0f,1.0f);
      effect->draw();
      }
    if (glcanvas->getContext()->isDoubleBuffer()) glcanvas->swapBuffers();
    glcanvas->makeNonCurrent();
    }
  return 1;
  }

#endif
