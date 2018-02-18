/********************************************************************************
*                                                                               *
*                      O p e n G L   C y l i n d e r   O b j e c t              *
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
#include "FXGLCylinder.h"


// GLU versions prior to 1.1 have GLUquadric
#if !defined(GLU_VERSION_1_1) && !defined(GLU_VERSION_1_2) && !defined(GLU_VERSION_1_3)
#define GLUquadricObj GLUquadric
#endif


// Cylinder fidelity
#define FXGLCYLINDER_SLICES_NUMBER		20
#define FXGLCYLINDER_STACKS_NUMBER		20
#define FXGLCYLINDER_LOOPS			4

using namespace FX;

/*******************************************************************************/

namespace FX {

// Object implementation
FXIMPLEMENT(FXGLCylinder,FXGLShape,NULL,0)


// Create cylinder
FXGLCylinder::FXGLCylinder():height(1.0f),radius(1.0f){
  FXTRACE((100,"FXGLCylinder::FXGLCylinder\n"));
  range.set(-radius,radius,0.0f,height,-radius,radius);
  }


// Create initialized cylinder
FXGLCylinder::FXGLCylinder(FXfloat x,FXfloat y,FXfloat z,FXfloat h,FXfloat r):
  FXGLShape(x,y,z,SHADING_SMOOTH|STYLE_SURFACE),height(h),radius(r){
  FXTRACE((100,"FXGLCylinder::FXGLCylinder\n"));
  range.set(-radius,radius,0.0f,height,-radius,radius);
  }


// Create initialized cylinder
FXGLCylinder::FXGLCylinder(FXfloat x,FXfloat y,FXfloat z,FXfloat h,FXfloat r,const FXMaterial& mtl):
  FXGLShape(x,y,z,SHADING_SMOOTH|STYLE_SURFACE,mtl,mtl),height(h),radius(r){
  FXTRACE((100,"FXGLCylinder::FXGLCylinder\n"));
  range.set(-radius,radius,0.0f,height,-radius,radius);
  }


// Copy constructor
FXGLCylinder::FXGLCylinder(const FXGLCylinder& orig):FXGLShape(orig){
  FXTRACE((100,"FXGLCylinder::FXGLCylinder\n"));
  height=orig.height;
  radius=orig.radius;
  }

// Change radius
void FXGLCylinder::setRadius(FXfloat r){
  if(radius!=r){
    range.lower.x=range.lower.z=-r;
    range.upper.x=range.upper.z= r;
    radius=r;
    }
  }


// Change height
void FXGLCylinder::setHeight(FXfloat h){
  if(height!=h){
    range.upper.y=h;
    height=h;
    }
  }


// Draw
void FXGLCylinder::drawshape(FXGLViewer*){
#ifdef HAVE_GL_H
  GLUquadricObj* quad=gluNewQuadric();
  gluQuadricDrawStyle(quad,(GLenum)GLU_FILL);
  /*
    gluQuadricNormals(quad,GLU_SMOOTH);
    gluQuadricOrientation(quad,GLU_OUTSIDE);
  */
  glPushMatrix();
  glRotatef(-90.0f,1.0f,0.0f,0.0f);
  gluCylinder(quad,radius,radius,height,FXGLCYLINDER_SLICES_NUMBER,FXGLCYLINDER_STACKS_NUMBER);

  gluQuadricOrientation(quad,(GLenum)GLU_INSIDE);
  gluDisk(quad,0,radius,FXGLCYLINDER_SLICES_NUMBER,FXGLCYLINDER_LOOPS);

  glTranslatef(0.0f,0.0f,height);
  gluQuadricOrientation(quad,(GLenum)GLU_OUTSIDE);
  gluDisk(quad,0,radius,FXGLCYLINDER_SLICES_NUMBER,FXGLCYLINDER_LOOPS);
  glPopMatrix();
  gluDeleteQuadric(quad);
#endif
  }


// Copy this object
FXGLObject* FXGLCylinder::copy(){
  return new FXGLCylinder(*this);
  }


// Save object to stream
void FXGLCylinder::save(FXStream& store) const {
  FXGLShape::save(store);
  store << height << radius;
  }


// Load object from stream
void FXGLCylinder::load(FXStream& store){
  FXGLShape::load(store);
  store >> height >> radius;
  }


// Destroy
FXGLCylinder::~FXGLCylinder(){
  FXTRACE((100,"FXGLCylinder::~FXGLCylinder\n"));
  }

}
