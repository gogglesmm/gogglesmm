/********************************************************************************
*                                                                               *
*                           O p e n G L   O b j e c t                           *
*                                                                               *
*********************************************************************************
* Copyright (C) 1998,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "FXFont.h"
#include "FXEvent.h"
#include "FXWindow.h"
#include "FXApp.h"
#include "FXGLViewer.h"
#include "FXGLObject.h"

/*
  Notes:
  - Leaf objects don't push any names!
  - Group objects should do focus traversal.
*/

using namespace FX;

/*******************************************************************************/

namespace FX {


// Object implementation
FXIMPLEMENT(FXGLObject,FXObject,nullptr,0)


// Get bounding box
void FXGLObject::bounds(FXRangef& box){
  box.upper.x=box.lower.x=0.0f;
  box.upper.y=box.lower.y=0.0f;
  box.upper.z=box.lower.z=0.0f;
  }


// Draw the GL scene
void FXGLObject::draw(FXGLViewer*){ }


// Hit objects
void FXGLObject::hit(FXGLViewer* viewer){ draw(viewer); }


// Copy
FXGLObject* FXGLObject::copy(){ return new FXGLObject(*this); }


// Identify object by its path
FXGLObject* FXGLObject::identify(FXuint*){ return this; }


// Return true if it can be dragged
FXbool FXGLObject::canDrag() const { return false; }


// Return true if OK to delete object
FXbool FXGLObject::canDelete() const { return false; }


// Drag the object
FXbool FXGLObject::drag(FXGLViewer*,FXint,FXint,FXint,FXint){ return false; }



/*******************************************************************************/

// Object implementation
FXIMPLEMENT(FXGLGroup,FXGLObject,nullptr,0)


// Get bounding box
void FXGLGroup::bounds(FXRangef& box){
  FXRangef b;
  box.lower.x=box.lower.y=box.lower.z=0.0f;
  box.upper.x=box.upper.y=box.upper.z=0.0f;
  if(0<list.no()){
    box.lower.x=box.lower.y=box.lower.z= FLT_MAX;
    box.upper.x=box.upper.y=box.upper.z=-FLT_MAX;
    for(FXival i=0; i<list.no(); i++){
      list[i]->bounds(b);
      box.include(b);
      }
    }
  }


// Draw
void FXGLGroup::draw(FXGLViewer* viewer){
  for(FXival i=0; i<list.no(); i++) list[i]->draw(viewer);
  }


// Draw for hit
void FXGLGroup::hit(FXGLViewer* viewer){
#ifdef HAVE_GL_H
  glPushName(0xffffffff);
  for(FXival i=0; i<list.no(); i++){
    glLoadName((FXuint)i);
    list[i]->hit(viewer);
    }
  glPopName();
#endif
  }


// Copy
FXGLObject* FXGLGroup::copy(){
  return new FXGLGroup(*this);
  }



// Identify object by its path
FXGLObject* FXGLGroup::identify(FXuint* path){
  FXASSERT(path);
  FXASSERT((FXint)path[0]<list.no());
  return list[path[0]]->identify(path+1);
  }


// Return true if it can be dragged
FXbool FXGLGroup::canDrag() const { return true; }


// Drag group object
FXbool FXGLGroup::drag(FXGLViewer* viewer,FXint fx,FXint fy,FXint tx,FXint ty){
  for(FXival i=0; i<list.no(); i++){
    list[i]->drag(viewer,fx,fy,tx,ty);
    }
  return true;
  }


// Save object to stream
void FXGLGroup::save(FXStream& store) const {
  FXGLObject::save(store);
  list.save(store);
  }


// Load object from stream
void FXGLGroup::load(FXStream& store){
  FXGLObject::load(store);
  list.load(store);
  }


// Delete members of the group
FXGLGroup::~FXGLGroup(){
  for(FXival i=0; i<list.no(); i++) delete list[i];
  }

}
