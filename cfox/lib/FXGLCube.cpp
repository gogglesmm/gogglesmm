/********************************************************************************
*                                                                               *
*                      O p e n G L   C u b e   O b j e c t                      *
*                                                                               *
*********************************************************************************
* Copyright (C) 1999,2016 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "fxmath.h"
#include "FXArray.h"
#include "FXHash.h"
#include "FXMutex.h"
#include "FXStream.h"
#include "FXVec2f.h"
#include "FXVec3f.h"
#include "FXVec4f.h"
#include "FXQuatf.h"
#include "FXMat4f.h"
#include "FXRangef.h"
#include "FXString.h"
#include "FXSize.h"
#include "FXPoint.h"
#include "FXRectangle.h"
#include "FXStringDictionary.h"
#include "FXSettings.h"
#include "FXRegistry.h"
#include "FXAccelTable.h"
#include "FXObjectList.h"
#include "FXEvent.h"
#include "FXWindow.h"
#include "FXApp.h"
#include "FXGLViewer.h"
#include "FXGLCube.h"


using namespace FX;

/*******************************************************************************/

namespace FX {

// Object implementation
FXIMPLEMENT(FXGLCube,FXGLShape,NULL,0)


// Create cube
FXGLCube::FXGLCube():width(1.0f),height(1.0f),depth(1.0f){
  FXTRACE((100,"FXGLCube::FXGLCube\n"));
  range.set(-0.5f*width,0.5f*width,-0.5f*height,0.5f*height,-0.5f*depth,0.5f*depth);
  }


// Create cube
FXGLCube::FXGLCube(FXfloat x,FXfloat y,FXfloat z,FXfloat w,FXfloat h,FXfloat d):
  FXGLShape(x,y,z,SHADING_SMOOTH|STYLE_SURFACE),width(w),height(h),depth(d){
  FXTRACE((100,"FXGLCube::FXGLCube\n"));
  range.set(-0.5f*width,0.5f*width,-0.5f*height,0.5f*height,-0.5f*depth,0.5f*depth);
  }


// Create initialized line
FXGLCube::FXGLCube(FXfloat x,FXfloat y,FXfloat z,FXfloat w,FXfloat h,FXfloat d,const FXMaterial& mtl):
  FXGLShape(x,y,z,SHADING_SMOOTH|STYLE_SURFACE,mtl,mtl),width(w),height(h),depth(d){
  FXTRACE((100,"FXGLCube::FXGLCube\n"));
  range.set(-0.5f*width,0.5f*width,-0.5f*height,0.5f*height,-0.5f*depth,0.5f*depth);
  }


// Copy constructor
FXGLCube::FXGLCube(const FXGLCube& orig):FXGLShape(orig){
  FXTRACE((100,"FXGLCube::FXGLCube\n"));
  width=orig.width;
  height=orig.height;
  depth=orig.depth;
  }


// Change width
void FXGLCube::setWidth(FXfloat w){
  if(width!=w){
    range.lower.x=-0.5f*w;
    range.upper.x= 0.5f*w;
    width=w;
    }
  }


// Change height
void FXGLCube::setHeight(FXfloat h){
  if(height!=h){
    range.lower.y=-0.5f*h;
    range.upper.y= 0.5f*h;
    height=h;
    }
  }


// Change depth
void FXGLCube::setDepth(FXfloat d){
  if(depth!=d){
    range.lower.z=-0.5f*d;
    range.upper.z= 0.5f*d;
    depth=d;
    }
  }


// Draw
void FXGLCube::drawshape(FXGLViewer*){
#ifdef HAVE_GL_H
  FXfloat xmin =-0.5f*width;
  FXfloat xmax = 0.5f*width;
  FXfloat ymin =-0.5f*height;
  FXfloat ymax = 0.5f*height;
  FXfloat zmin =-0.5f*depth;
  FXfloat zmax = 0.5f*depth;
  glBegin(GL_TRIANGLE_STRIP);
    glNormal3f(0.0f,0.0f,-1.0f);
    glVertex3f(xmin, ymin, zmin);
    glVertex3f(xmin, ymax, zmin);
    glVertex3f(xmax, ymin, zmin);
    glVertex3f(xmax, ymax, zmin);
  glEnd();

  glBegin(GL_TRIANGLE_STRIP);
    glNormal3f(1.0f,0.0f,0.0f);
    glVertex3f(xmax, ymin, zmin);
    glVertex3f(xmax, ymax, zmin);
    glVertex3f(xmax, ymin, zmax);
    glVertex3f(xmax, ymax, zmax);
  glEnd();

  glBegin(GL_TRIANGLE_STRIP);
    glNormal3f(0.0f,0.0f,1.0f);
    glVertex3f(xmax, ymin, zmax);
    glVertex3f(xmax, ymax, zmax);
    glVertex3f(xmin, ymin, zmax);
    glVertex3f(xmin, ymax, zmax);
  glEnd();

  glBegin(GL_TRIANGLE_STRIP);
    glNormal3f(-1.0f,0.0f,0.0f);
    glVertex3f(xmin, ymin, zmax);
    glVertex3f(xmin, ymax, zmax);
    glVertex3f(xmin, ymin, zmin);
    glVertex3f(xmin, ymax, zmin);
  glEnd();

  glBegin(GL_TRIANGLE_STRIP);
    glNormal3f(0.0f,1.0f,0.0f);
    glVertex3f(xmin, ymax, zmin);
    glVertex3f(xmin, ymax, zmax);
    glVertex3f(xmax, ymax, zmin);
    glVertex3f(xmax, ymax, zmax);
  glEnd();

  glBegin(GL_TRIANGLE_STRIP);
    glNormal3f(0.0f,-1.0f,0.0f);
    glVertex3f(xmin, ymin, zmax);
    glVertex3f(xmin, ymin, zmin);
    glVertex3f(xmax, ymin, zmax);
    glVertex3f(xmax, ymin, zmin);
  glEnd();
#endif
  }


// Copy this object
FXGLObject* FXGLCube::copy(){
  return new FXGLCube(*this);
  }


// Save object to stream
void FXGLCube::save(FXStream& store) const {
  FXGLShape::save(store);
  store << width << height << depth;
  }


// Load object from stream
void FXGLCube::load(FXStream& store){
  FXGLShape::load(store);
  store >> width >> height >> depth;
  }


// Destroy
FXGLCube::~FXGLCube(){
  FXTRACE((100,"FXGLCube::~FXGLCube\n"));
  }

}
