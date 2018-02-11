/********************************************************************************
*                                                                               *
*                      O p e n G L   S p h e r e   O b j e c t                  *
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
#include "FXGLSphere.h"


// GLU versions prior to 1.1 have GLUquadric
#if !defined(GLU_VERSION_1_1) && !defined(GLU_VERSION_1_2) && !defined(GLU_VERSION_1_3)
#define GLUquadricObj GLUquadric
#endif


// Sphere fidelity
#define SPHERE_SLICES  20
#define SPHERE_STACKS  20

using namespace FX;

/*******************************************************************************/

namespace FX {

// Object implementation
FXIMPLEMENT(FXGLSphere,FXGLShape,NULL,0)


// Create sphere
FXGLSphere::FXGLSphere(void):radius(0.5f),slices(SPHERE_SLICES),stacks(SPHERE_STACKS){
  FXTRACE((100,"FXGLSphere::FXGLSphere\n"));
  range.set(-radius,radius,-radius,radius,-radius,radius);
  }


// Create initialized sphere
FXGLSphere::FXGLSphere(FXfloat x,FXfloat y,FXfloat z,FXfloat r):
  FXGLShape(x,y,z,SHADING_SMOOTH|STYLE_SURFACE),radius(r),slices(SPHERE_SLICES),stacks(SPHERE_STACKS){
  FXTRACE((100,"FXGLSphere::FXGLSphere\n"));
  range.set(-radius,radius,-radius,radius,-radius,radius);
  }


// Create initialized sphere
FXGLSphere::FXGLSphere(FXfloat x,FXfloat y,FXfloat z,FXfloat r,const FXMaterial& mtl):
  FXGLShape(x,y,z,SHADING_SMOOTH|STYLE_SURFACE,mtl,mtl),radius(r),slices(SPHERE_SLICES),stacks(SPHERE_STACKS){
  FXTRACE((100,"FXGLSphere::FXGLSphere\n"));
  range.set(-radius,radius,-radius,radius,-radius,radius);
  }


// Copy constructor
FXGLSphere::FXGLSphere(const FXGLSphere& orig):FXGLShape(orig){
  FXTRACE((100,"FXGLSphere::FXGLSphere\n"));
  radius=orig.radius;
  slices=orig.slices;
  stacks=orig.stacks;
  }


// Change radius
void FXGLSphere::setRadius(FXfloat r){
  if(radius!=r){
    range.set(-r,r,-r,r,-r,r);
    radius=r;
    }
  }


// Draw
void FXGLSphere::drawshape(FXGLViewer*){
#ifdef HAVE_GL_H
  GLUquadricObj* quad=gluNewQuadric();
  gluQuadricDrawStyle(quad,(GLenum)GLU_FILL);
  /*
    if (shading==FXGLShape::ID_SHADESMOOTH){
    gluQuadricNormals(quad,(GLenum)GLU_SMOOTH);
    gluQuadricOrientation(quad,(GLenum)GLU_OUTSIDE);
    }
  */
  gluSphere(quad,radius,slices,stacks);
  gluDeleteQuadric(quad);
#endif
  }


// Copy this object
FXGLObject* FXGLSphere::copy(){
  return new FXGLSphere(*this);
  }


// Save object to stream
void FXGLSphere::save(FXStream& store) const {
  FXGLShape::save(store);
  store << radius;
  store << slices;
  store << stacks;
  }


// Load object from stream
void FXGLSphere::load(FXStream& store){
  FXGLShape::load(store);
  store >> radius;
  store >> slices;
  store >> stacks;
  }


// Destroy
FXGLSphere::~FXGLSphere(){
  FXTRACE((100,"FXGLSphere::~FXGLSphere\n"));
  }

}
